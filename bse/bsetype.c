/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include        "bsetype.h"

#include        "bseenums.h"
#include        "bseplugin.h"
#include        <string.h>

/* FIXME: don't worry about ref_count in destructors, the ref_count is
 * zero already and thus _ref and _unref functions will fail anyways
 * FIXME: interface should be called tables
 * FIXME: s/object/tstruct/ inside type system (maybe use tobject, but not BseObject)
 */

/* WARNING: some functions (some internal variants and non-static ones)
 * invalidate data portions of the TypeNodes. if external functions/callbacks
 * are called, pointers to memory maintained by TypeNodes have to be looked up
 * again. this affects most of the struct TypeNode fields, e.g. ->children or
 * ->ifaces (not ->supers[] as of recently), as all those memory portions can
 * get realloc()ed during callback invocation.
 */

#define TYPE_NODES_BLOCK_SIZE   (1)     /* FIXME: 16 */
#define FIXME_DISABLE_PREALLOCATIONS

#define IFACE_PARENT_INFO       ((BseInterfaceInfo*) 41)

typedef struct _TypeNode        TypeNode;
typedef struct _CommonData      CommonData;
typedef struct _IFaceData       IFaceData;
typedef struct _ClassedData     ClassedData;
typedef struct _ObjectData      ObjectData;
typedef union  _TypeData        TypeData;
typedef struct _IFaceEntry      IFaceEntry;

struct _TypeNode
{
  BsePlugin  *plugin;
  guint       n_supers : 7;
  guint       n_children : 12;
  guint       n_ifaces : 9;
  guint       is_procedure_type : 1;
  guint       is_iface : 1;
  guint       is_classed : 1;
  guint       is_object : 1;
  BseType    *children; /* for ifaces, these are the conforming objects */
  IFaceEntry *ifaces;
  GQuark      qname;
  GQuark      qblurb;
  TypeData   *data;
  BseType     supers[1]; /* flexible array */
};
#define SIZEOF_BASE_TYPE_NODE()    (G_STRUCT_OFFSET (TypeNode, supers))
#define MAX_N_SUPERS    127
#define MAX_N_CHILDREN  4095
#define MAX_N_IFACES    511

struct _CommonData
{
  guint                   ref_count;
  guint                   last_ref_handler;
};
struct _IFaceData
{
  CommonData         common_data;
  guint              vtable_size;
  BseBaseInitFunc    vtable_init_base;
  BseBaseDestroyFunc vtable_destroy_base;
};
struct _ClassedData
{
  CommonData          common_data;
  guint               class_size;
  BseBaseInitFunc     class_init_base;
  BseBaseDestroyFunc  class_destroy_base;
  BseClassInitFunc    class_init;
  BseClassDestroyFunc class_destroy;
  gconstpointer       class_data;
  
  gpointer                class;
};
struct _ObjectData
{
  CommonData          common_data;
  guint               class_size;
  BseBaseInitFunc     class_init_base;
  BseBaseDestroyFunc  class_destroy_base;
  BseClassInitFunc    class_init;
  BseClassDestroyFunc class_destroy;
  gconstpointer       class_data;
  
  gpointer            class;
  
  guint16             object_size;
  guint16             n_preallocs;
  BseObjectInitFunc   object_init;
  
  GMemChunk          *mem_chunk;
};
union _TypeData
{
  guint                  ref_count;
  CommonData             common;
  ObjectData             object;
  ClassedData            classed;
  IFaceData              iface;
};
struct _IFaceEntry
{
  BseType iface;
  BseInterfaceInfo *info;
  BseTypeInterface *vtable;
};


/* --- prototypes --- */
static inline gchar*    type_descriptive_name           (BseType         type);
static inline IFaceEntry* type_lookup_iface_entry       (TypeNode       *node,
                                                         TypeNode       *iface);
static TypeNode*        type_node_new                   (BseType         parent_type,
                                                         const gchar    *name,
                                                         const gchar    *blurb,
                                                         BsePlugin      *plugin);
static void             type_class_init                 (BseType         type,
                                                         BseTypeClass   *pclass);
static void             type_class_destroy              (TypeNode       *node);
static void             type_iface_vtable_init          (BseType         object_type,
                                                         BseType         interface_type);
static void             type_iface_vtable_destroy       (TypeNode       *node,
                                                         IFaceEntry     *entry);
static void             type_data_make                  (TypeNode       *node,
                                                         const BseTypeInfo *info);
static void             type_data_blow                  (BseType         type);
static void             type_iface_entry_info_make      (TypeNode       *node,
                                                         IFaceEntry     *entry,
                                                         BseInterfaceInfo *info);
static void             type_iface_entry_info_blow      (BseType         object_type,
                                                         BseType         iface_type);
static void             bse_type_class_lastref          (gpointer        bse_class);


/* --- type nodes --- */
static GHashTable       *type_nodes_ht = NULL;
static GSList           *bse_debug_classes = NULL;
static TypeNode        **bse_type_nodes = NULL;
static guint             bse_n_type_nodes = 0;

static inline TypeNode*
LOOKUP_TYPE_NODE (register BseType utype)
{
  register BseType type = BSE_TYPE_SEQNO (utype);

  return type < bse_n_type_nodes ? bse_type_nodes[type] : (TypeNode*) NULL;
}
#define NODE_PARENT_TYPE(node)  (node->supers[1])
#define NODE_TYPE(node)         (node->supers[0])
#define NODE_NAME(node)         (g_quark_to_string (node->qname))


/* --- functions --- */
static void
bse_type_debug (void)
{
  if (bse_debug_classes)
    BSE_DEBUG (CLASSES, {
      GSList *slist;
      
      for (slist = bse_debug_classes; slist; slist = slist->next)
	g_message ("stale class: `%s'", BSE_CLASS_NAME (slist->data));
    });
}

static inline gchar*
type_descriptive_name (BseType type)
{
  gchar *name;
  
  if (type)
    {
      name = bse_type_name (type);
      if (!name)
        name = "(unknown)";
    }
  else
    name = "(invalid)";
  
  return name;
}

static inline IFaceEntry*
type_lookup_iface_entry (TypeNode       *node,
                         TypeNode       *iface)
{
  if (iface->is_iface && node->n_ifaces)
    {
      IFaceEntry *ifaces;
      guint n_ifaces;
      BseType iface_type;
      
      n_ifaces = node->n_ifaces;
      ifaces = node->ifaces - 1;
      iface_type = NODE_TYPE (iface);
      
      do
        {
          guint i;
          IFaceEntry *check;
          
          i = (n_ifaces + 1) / 2;
          check = ifaces + i;
          if (iface_type == check->iface)
            return check;
          else if (iface_type > check->iface)
            {
              n_ifaces -= i;
              ifaces = check;
            }
          else if (iface_type < check->iface)
            n_ifaces = i - 1;
        }
      while (n_ifaces);
    }
  
  return NULL;
}

static TypeNode*
type_node_new (BseType      parent_type,
	       const gchar *name,
	       const gchar *blurb,
	       BsePlugin   *plugin)
{
  static guint n_type_slots = 0;
  TypeNode *pnode;
  TypeNode *node;
  guint n_supers;
  BseType type;

  while (n_type_slots <= bse_n_type_nodes)
    {
      n_type_slots += TYPE_NODES_BLOCK_SIZE;
      bse_type_nodes = g_renew (TypeNode*, bse_type_nodes, n_type_slots);
    }
  
  if (parent_type)
    {
      pnode = LOOKUP_TYPE_NODE (parent_type);
      g_assert (pnode->n_supers < MAX_N_SUPERS);
      g_assert (pnode->n_children < MAX_N_CHILDREN);
      n_supers = pnode->n_supers + 1;
    }
  else
    {
      pnode = NULL;
      n_supers = 0;
    }

  node = g_malloc0 (SIZEOF_BASE_TYPE_NODE () + sizeof (BseType[1 + n_supers + 1]));
  node->n_supers = n_supers;

  type = bse_n_type_nodes;
  bse_type_nodes[bse_n_type_nodes++] = node;

  if (!pnode)
    {
      node->supers[0] = type;
      node->supers[1] = 0;
      node->n_ifaces = 0;
      node->is_procedure_type = BSE_TYPE_IS_PROCEDURE (type);
      node->is_iface = type == BSE_TYPE_INTERFACE;
      node->is_classed = BSE_TYPE_IS_CLASSED (type);
      node->is_object = type == BSE_TYPE_OBJECT;
      node->ifaces = NULL;
    }
  else
    {
      guint i;
      
      type = BSE_FUNDAMENTAL_TYPE (NODE_TYPE (pnode)) | type << 8;
      node->supers[0] = type;
      node->n_ifaces = pnode->n_ifaces;
      node->is_procedure_type = pnode->is_procedure_type;
      node->is_classed = pnode->is_classed;
      node->is_iface = pnode->is_iface;
      node->is_object = pnode->is_object;

      memcpy (node->supers + 1, pnode->supers, sizeof (BseType[1 + pnode->n_supers + 1]));
      
      node->ifaces = g_memdup (pnode->ifaces, sizeof (pnode->ifaces[0]) * node->n_ifaces); /* FIXME */
      for (i = 0; i < node->n_ifaces; i++)
        node->ifaces[i].info = IFACE_PARENT_INFO;
      
      pnode->n_children++;
      pnode->children = g_renew (BseType, pnode->children, pnode->n_children);
      pnode->children[pnode->n_children - 1] = type;
    }
  
  node->plugin = plugin;
  node->qname = g_quark_from_string (name);
  node->n_children = 0;
  node->children = NULL;
  node->qblurb = blurb ? g_quark_from_string (blurb) : 0;
  node->data = NULL;
  
  g_hash_table_insert (type_nodes_ht,
                       GUINT_TO_POINTER (node->qname),
                       GUINT_TO_POINTER (type));

  return node;
}

BseType
bse_type_register_static (BseType            parent_type,
                          const gchar       *type_name,
                          const gchar       *type_blurb,
                          const BseTypeInfo *info)
{
  TypeNode *pnode;
  BseType type;
  
  g_return_val_if_fail (parent_type > 0, 0);
  g_return_val_if_fail (type_name != NULL, 0);
  g_return_val_if_fail (info != NULL, 0);
  
  if (bse_type_from_name (type_name))
    {
      g_warning ("cannot register existing type `%s'", type_name);
      return 0;
    }
  pnode = LOOKUP_TYPE_NODE (parent_type);
  if (!pnode)
    {
      g_warning ("cannot derive type `%s' from invalid parent type `%s'",
                 type_name,
                 type_descriptive_name (parent_type));
      return 0;
    }
  if (!pnode->is_object &&
      (info->object_size || info->n_preallocs || info->object_init))
    {
      g_warning ("cannot derive object type `%s' from non-object parent type `%s'",
                 type_name,
                 type_descriptive_name (parent_type));
      return 0;
    }
  if (!pnode->is_classed &&
      (info->class_init || info->class_destroy || info->class_data ||
       (!pnode->is_iface && (info->base_init || info->base_destroy))))
    {
      g_warning ("cannot derive classed type `%s' from non-classed parent type `%s'",
                 type_name,
                 type_descriptive_name (parent_type));
      return 0;
    }
  if (pnode->is_classed)
    {
      if ((info->class_size < sizeof (BseTypeClass)) ||
          (pnode->data && info->class_size < pnode->data->classed.class_size))
        {
          g_warning ("specified class size for type `%s' is smaller "
                     "than the parent type's `%s' class size",
                     type_name,
                     type_descriptive_name (parent_type));
          return 0;
        }
      if (!pnode->is_object && BSE_FUNDAMENTAL_TYPE (parent_type) != parent_type)
        {
          g_warning ("parent type `%s' for type `%s' is not a fundamental type, "
                     "expected `%s'",
                     type_descriptive_name (parent_type),
                     type_name,
                     type_descriptive_name (BSE_FUNDAMENTAL_TYPE (parent_type)));
          return 0;
        }
    }
  else if (!pnode->is_iface)
    {
      g_warning ("cannot derive type `%s' from unclassed parent type `%s'",
                 type_name,
                 type_descriptive_name (parent_type));
      return 0;
    }
  else if (parent_type != BSE_TYPE_INTERFACE)
    {
      g_warning ("cannot derive from interface types (`%s' from `%s')",
                 type_name,
                 NODE_NAME (pnode));
      return 0;
    }
  else if (info->class_size < sizeof (BseTypeInterface))
    {
      g_warning ("specified class size for type `%s' is smaller "
                 "than the parent's `BseTypeInterface' size",
                 type_name);
      return 0;
    }
  
  pnode = type_node_new (parent_type, type_name, type_blurb, NULL);
  type = NODE_TYPE (pnode);
  type_data_make (pnode, info);
  
  return type;
}

gpointer
bse_type_class_peek (BseType type)
{
  TypeNode *node;
  
  node = LOOKUP_TYPE_NODE (type);
  if (node && node->is_classed && node->data)
    return node->data->classed.class;
  else
    return NULL;
}

gpointer
bse_type_class_ref (BseType type)
{
  TypeNode *node;
  
  /* attempt a fast, successfull lookup first,
   * to optimize for the common code path
   */
  node = LOOKUP_TYPE_NODE (type);
  if (node && node->is_classed && node->data && node->data->classed.class &&
      !node->data->common.last_ref_handler)
    {
      node->data->ref_count++;
      
      return node->data->classed.class;
    }
  
  /* now do the moron checks
   */
  if (!node || !node->is_classed)
    {
      g_warning ("cannot retrive class for invalid (unclassed) type `%s'",
                 type_descriptive_name (type));
      return NULL;
    }

  if (!node->data)
    {
      type_data_make (node, NULL);
      g_assert (node->data != NULL);
    }

  if (node->data->classed.class)
    {
      g_return_val_if_fail (node->data->ref_count > 0, NULL);
      
      /* FIXME: last_ref_handler */
    }
  
  if (!node->data->classed.class)
    {
      BseType ptype;
      BseTypeClass *pclass;
      
      g_assert (node->data->ref_count == 0);

      ptype = NODE_PARENT_TYPE (node);
      pclass = ptype ? bse_type_class_ref (ptype) : NULL;
      
      type_class_init (type, pclass);
    }
  
  node->data->ref_count++;

  return node->data->classed.class;
}

static void
type_class_init (BseType       type,
		 BseTypeClass *pclass)
{
  TypeNode *node;
  BseTypeClass *class;
  TypeNode *bnode;
  GSList *slist;
  
  node = LOOKUP_TYPE_NODE (type);
  
  g_assert (node->is_classed && node->data &&
            node->data->classed.class_size &&
            !node->data->classed.class);
  
  class = g_malloc0 (node->data->classed.class_size);
  node->data->classed.class = class;

  BSE_DEBUG (CLASSES, bse_debug_classes = g_slist_prepend (bse_debug_classes, class));
  
  if (pclass)
    {
      TypeNode *pnode;
      
      pnode = LOOKUP_TYPE_NODE (pclass->bse_type);
      
      if (node->data->classed.class_size < pnode->data->classed.class_size)
        g_error ("specified class size for type `%s' is smaller "
                 "than the parent's type `%s' class size",
                 NODE_NAME (node), NODE_NAME (pnode));
      
      memcpy (class, pclass, pnode->data->classed.class_size);
    }
  
  class->bse_type = type;
  
  /* stack all base class initialization functions, so we
   * call them in ascending order.
   */
  bnode = node;
  slist = NULL;
  while (bnode)
    {
      if (bnode->data->classed.class_init_base)
        slist = g_slist_prepend (slist, bnode->data->classed.class_init_base);
      bnode = LOOKUP_TYPE_NODE (NODE_PARENT_TYPE (bnode));
    }
  if (slist)
    {
      GSList *walk;
      
      for (walk = slist; walk; walk = walk->next)
        {
          BseBaseInitFunc class_init_base;
          
          class_init_base = walk->data;
          class_init_base (class);
        }
      g_slist_free (slist);
    }
  
  if (node->data->classed.class_init)
    node->data->classed.class_init (class, (gpointer) node->data->classed.class_data);
}

static void
type_class_destroy (TypeNode *node)
{
  BseType type;
  BseTypeClass *class;
  TypeNode *bnode;
  
  g_assert (node->is_classed && node->data && node->data->classed.class &&
            node->data->ref_count == 0);
  
  type = NODE_TYPE (node);
  class = node->data->classed.class;

  BSE_DEBUG (CLASSES, {
    bse_debug_classes = g_slist_remove (bse_debug_classes, class);
    g_message ("destroying %sClass `%s'",
	       type_descriptive_name (BSE_FUNDAMENTAL_TYPE (type)),
	       type_descriptive_name (type));
  });
  
  if (node->data->classed.class_destroy)
    node->data->classed.class_destroy (class, (gpointer) node->data->classed.class_data);

  /* call all base class destruction functions in descending order
   */
  bnode = node;
  while (bnode)
    {
      if (bnode->data->classed.class_destroy_base)
        bnode->data->classed.class_destroy_base (class);
      bnode = LOOKUP_TYPE_NODE (NODE_PARENT_TYPE (bnode));
    }
  
  node->data->classed.class = NULL;
  
  g_return_if_fail (node->data->ref_count == 0);
  
  class->bse_type = 0;
  g_free (class);
}

void
bse_type_class_unref (gpointer bse_class)
{
  TypeNode *node;
  BseTypeClass *class = bse_class;
  
  g_return_if_fail (bse_class != NULL);
  
  /* optimize for common code path
   */
  node = LOOKUP_TYPE_NODE (class->bse_type);
  if (node && node->is_classed && node->data && node->data->ref_count > 1 &&
      !node->data->common.last_ref_handler)
    {
      node->data->ref_count--;
      
      return;
    }
  
  /* now do the moron checks
   */
  if (!node || !node->is_classed || !node->data || node->data->classed.class != class)
    {
      g_warning ("cannot unreference class of invalid (unclassed) type `%s'",
                 type_descriptive_name (class->bse_type));
      return;
    }
  g_return_if_fail (node->data->ref_count > 0);
  
  node->data->ref_count--;
  
  if (!node->data->ref_count)
    {
      node->data->ref_count++;

      /* FIXME: last_ref_handler */
      bse_type_class_lastref (class);
    }
}

static void
bse_type_class_lastref (gpointer bse_class)
{
  TypeNode *node;
  BseTypeClass *class = bse_class;
  
  g_return_if_fail (bse_class != NULL);
  
  /* first, do the moron checks
   */
  node = LOOKUP_TYPE_NODE (class->bse_type);
  if (!node || !node->is_classed || !node->data || node->data->classed.class != class)
    {
      g_warning ("cannot unreference class of invalid (unclassed) type `%s'",
                 type_descriptive_name (class->bse_type));
      return;
    }
  g_return_if_fail (node->data->ref_count > 0);

  node->data->ref_count--;
  if (!node->data->ref_count)
    {
      BseType ptype;

      /* FIXME: last_ref_handler */
      
      if (node->is_object && node->data->object.mem_chunk)
        {
          g_mem_chunk_destroy (node->data->object.mem_chunk);
          node->data->object.mem_chunk = NULL;
        }
      
      ptype = NODE_PARENT_TYPE (node);

      type_class_destroy (node);
      type_data_blow (NODE_TYPE (node));
      
      /* unreference parent class
       */
      if (ptype)
        {
          node = LOOKUP_TYPE_NODE (ptype);
          bse_type_class_unref (node->data->classed.class);
        }
    }
}

void
bse_type_add_interface (BseType           object_type,
                        BseType           interface_type,
                        BseInterfaceInfo *info)
{
  BseType parent_type;
  TypeNode *node;
  TypeNode *iface;
  
  g_return_if_fail (BSE_TYPE_IS_OBJECT (object_type));
  g_return_if_fail (bse_type_parent (interface_type) == BSE_TYPE_INTERFACE);
  
  node = LOOKUP_TYPE_NODE (object_type);
  if (!node || !node->is_object)
    {
      g_warning ("cannot add interfaces to invalid (non-object) type `%s'",
                 type_descriptive_name (object_type));
      return;
    }
  parent_type = NODE_PARENT_TYPE (node);
  
  iface = LOOKUP_TYPE_NODE (interface_type);
  if (!iface || !iface->is_iface || !iface->data)
    {
      g_warning ("cannot add invalid interface `%s' to type `%s'",
                 type_descriptive_name (interface_type),
                 NODE_NAME (node));
      return;
    }
  
  if (info == IFACE_PARENT_INFO)
    g_return_if_fail (bse_type_conforms_to (parent_type, interface_type));
  else
    {
      if (iface->plugin)
        g_return_if_fail (info == NULL);
      else
        g_return_if_fail (info != NULL);
    }
  
  if (bse_type_conforms_to (object_type, interface_type))
    {
      g_warning ("type `%s' already conforms to interface `%s'",
                 NODE_NAME (node),
                 type_descriptive_name (interface_type));
    }
  else
    {
      guint i;
      IFaceEntry *ifaces;

      /* we may never call any functions of the BseInterfaceInfo* from within
       * here, since we are most probably called from _within_ a type registering
       * function
       */
      
      g_assert (iface->n_children < MAX_N_CHILDREN);
      iface->n_children++;
      iface->children = g_renew (BseType, iface->children, iface->n_children);
      iface->children[iface->n_children - 1] = NODE_TYPE (node);
      
      g_assert (node->n_ifaces < MAX_N_IFACES);
      node->n_ifaces++;
      node->ifaces = g_renew (IFaceEntry, node->ifaces, node->n_ifaces);
      
      ifaces = node->ifaces;
      for (i = 0; i < node->n_ifaces - 1; i++)
        if (ifaces[i].iface > interface_type)
          break;
      
      g_memmove (ifaces + i, ifaces + i + 1, sizeof (IFaceEntry) * (node->n_ifaces - i - 1));
      ifaces[i].iface = interface_type;
      ifaces[i].vtable = NULL;
      
      if (!info || info == IFACE_PARENT_INFO)
        ifaces[i].info = info;
      else
        {
          ifaces[i].info = NULL;
          type_iface_entry_info_make (node, ifaces + i, info);
        }
      
      for (i = 0; i < node->n_children; i++)
        bse_type_add_interface (node->children[i], interface_type, IFACE_PARENT_INFO);
    }
}

gpointer
bse_type_interface_peek (gpointer object_class,
                         BseType  iface_type)
{
  TypeNode *node;
  TypeNode *iface;
  BseTypeClass *class = object_class;
  
  g_return_val_if_fail (object_class != NULL, NULL);
  
  node = LOOKUP_TYPE_NODE (class->bse_type);
  iface = LOOKUP_TYPE_NODE (iface_type);
  if (node && iface)
    {
      IFaceEntry *entry;
      
      entry = type_lookup_iface_entry (node, iface);
      if (entry && entry->vtable)
        return entry->vtable;
    }
  
  return NULL;
}

gpointer
bse_type_interface_ref (gpointer object_class,
                        BseType  iface_type)
{
  TypeNode *node;
  TypeNode *iface;
  IFaceEntry *entry;
  BseTypeClass *class = object_class;
  
  g_return_val_if_fail (object_class != NULL, NULL);
  
  /* attempt a fast, successfull lookup first to optimize
   * for the common code path
   */
  node = LOOKUP_TYPE_NODE (class->bse_type);
  iface = LOOKUP_TYPE_NODE (iface_type);
  if (node && iface)
    {
      entry = type_lookup_iface_entry (node, iface);
      if (entry && entry->vtable && entry->vtable->ref_count) /* FIXME: last_ref_handler */
        {
          entry->vtable->ref_count++;
          
          return entry->vtable;
        }
    }
  else
    entry = NULL;
  
  /* now do all the moron checks, node, iface and entry are already setup
   */
  if (!node || !node->is_object || !node->data || node->data->classed.class != object_class)
    {
      g_warning ("cannot retrive interface for invalid (unclassed) type `%s'",
                 type_descriptive_name (class->bse_type));
      return NULL;
    }
  if (!iface || !iface->is_iface)
    {
      g_warning ("cannot retrive interface for type `%s' from invalid interface type `%s'",
                 type_descriptive_name (class->bse_type),
                 type_descriptive_name (iface_type));
      return NULL;
    }
  if (!entry)
    {
      g_warning ("cannot retrive interface `%s' for non conforming type `%s'",
                 type_descriptive_name (iface_type),
                 type_descriptive_name (class->bse_type));
      
      return NULL;
    }
  
  if (entry->vtable)
    g_return_val_if_fail (entry->vtable->ref_count > 0, NULL);
  else
    {
      /* lookup the interface conforming node, might be a perent
       */
      while (entry->info == IFACE_PARENT_INFO)
        {
          node = LOOKUP_TYPE_NODE (NODE_PARENT_TYPE (node));
          entry = type_lookup_iface_entry (node, iface);
        }

      /* reference class of conforming type */
      bse_type_class_ref (NODE_TYPE (node));
      
      if (!iface->data)
        {
          type_data_make (iface, NULL);
          g_assert (iface->data != NULL);
          entry = type_lookup_iface_entry (node, iface);
        }
      
      iface->data->ref_count++;
      /* FIXME: last_ref_handler */
      
      if (!entry->info)
        type_iface_entry_info_make (node, entry, NULL);
      
      type_iface_vtable_init (class->bse_type, iface_type);
      
      node = LOOKUP_TYPE_NODE (class->bse_type);
      entry = type_lookup_iface_entry (node, iface);
    }
  
  entry->vtable->ref_count++;
  /* FIXME: last_ref_handler */
  
  return entry->vtable;
}

static void
type_propagate_iface_vtable (TypeNode         *pnode,
                             TypeNode         *iface,
                             BseTypeInterface *vtable)
{
  IFaceEntry *entry;
  guint i;
  
  entry = type_lookup_iface_entry (pnode, iface);
  entry->vtable = vtable;
  
  for (i = 0; i < pnode->n_children; i++)
    {
      TypeNode *node;
      
      node = LOOKUP_TYPE_NODE (pnode->children[i]);
      type_propagate_iface_vtable (node, iface, vtable);
    }
}

static void
type_iface_vtable_init (BseType type,
			BseType iface_type)
{
  TypeNode *node;
  TypeNode *iface;
  IFaceEntry *entry;
  BseTypeInterface *vtable;
  
  iface = LOOKUP_TYPE_NODE (iface_type);
  node = LOOKUP_TYPE_NODE (type);
  entry = type_lookup_iface_entry (node, iface);
  
  g_assert (entry && entry->vtable == NULL && entry->info && entry->info != IFACE_PARENT_INFO);
  
  vtable = g_malloc0 (iface->data->iface.vtable_size);
  vtable->bse_type = iface_type;
  vtable->object_type = type;
  vtable->ref_count = 0;
  
  type_propagate_iface_vtable (node, iface, vtable);
  
  if (iface->data->iface.vtable_init_base)
    {
      iface->data->iface.vtable_init_base (vtable);
      entry = type_lookup_iface_entry (node, iface); /* FIXME: do we need this line? */
    }
  
  if (entry->info->interface_init)
    entry->info->interface_init (vtable, entry->info->interface_data);
}

static void
type_iface_vtable_destroy (TypeNode   *node,
			   IFaceEntry *entry)
{
  BseType type;
  BseType iface_type;
  TypeNode *iface;
  BseTypeInterface *vtable;
  
  type = NODE_TYPE (node);
  iface_type = entry->iface;
  vtable = entry->vtable;
  iface = LOOKUP_TYPE_NODE (iface_type);

  /* FIXME: propagate NULL class *before* or *after* actuall destruction? */
  type_propagate_iface_vtable (node, iface, NULL);
  
  if (entry->info->interface_destroy)
    entry->info->interface_destroy (vtable, entry->info->interface_data);
  
  if (iface->data->iface.vtable_destroy_base)
    iface->data->iface.vtable_destroy_base (vtable);
  
  if (vtable->ref_count != 0)
    {
      g_warning ("interface `%s' destructors for `%s' leak references",
                 NODE_NAME (iface),
                 NODE_NAME (node));
      vtable->ref_count = 0;
    }
  
  vtable->bse_type = 0;
  vtable->object_type = 0;
  g_free (vtable);
}

void
bse_type_interface_unref (gpointer interface)
{
  BseTypeInterface *vtable = interface;
  TypeNode *iface;
  TypeNode *node;
  IFaceEntry *entry;
  BseType iface_type;
  BseType type;
  
  g_return_if_fail (interface != NULL);
  g_return_if_fail (vtable->ref_count > 0);
  
  /* optimize for common code path
   */
  iface = LOOKUP_TYPE_NODE (vtable->bse_type);
  if (iface && iface->is_iface && vtable->ref_count > 1)
    {
      vtable->ref_count--;
      return;
    }
  
  /* finish lookups
   */
  node = LOOKUP_TYPE_NODE (vtable->object_type);
  if (node && iface)
    entry = type_lookup_iface_entry (node, iface);
  else
    entry = NULL;
  iface_type = vtable->bse_type;
  type = vtable->object_type;
  
  /* now do the moron checks, lookups are already performed
   */
  if (!iface || !iface->is_iface || !iface->data)
    {
      g_warning ("cannot unreference class of invalid (non-interface) type `%s'",
                 type_descriptive_name (iface_type));
      return;
    }
  if (!node || !node->is_object || !node->data)
    {
      g_warning ("cannot unreference interface `%s' which "
                 "contains invalid object type `%s'",
                 type_descriptive_name (iface_type),
                 type_descriptive_name (type));
      return;
    }
  if (!entry || entry->vtable != vtable ||
      !entry->info || entry->info == IFACE_PARENT_INFO)
    {
      /* FIXME: probably need to handle IFACE_PARENT_INFO here */
      g_warning ("cannot unreference interface class `%s' which "
                 "is not recognized by object type `%s'",
                 type_descriptive_name (iface_type),
                 type_descriptive_name (type));
      return;
    }
  
  g_return_if_fail (vtable->ref_count > 0);
  
  vtable->ref_count--;

  /* FIXME: last_ref_handler */
  
  if (!vtable->ref_count)
    {
      BseTypeClass *object_class;
      
      object_class = node->data->classed.class;
      
      type_iface_vtable_destroy (node, entry);
      
      type_iface_entry_info_blow (type, iface_type);
      
      iface->data->ref_count--;
      if (!iface->data->ref_count)
        type_data_blow (iface_type);
      
      /* unreference class of conforming type */
      bse_type_class_unref (object_class);
    }
}

BseType
bse_type_register_dynamic (BseType      parent_type,
                           const gchar *type_name,
                           const gchar *type_blurb,
                           BsePlugin   *plugin)
{
  TypeNode *pnode;
  BseType type;
  
  g_return_val_if_fail (parent_type > 0, 0);
  g_return_val_if_fail (type_name != NULL, 0);
  g_return_val_if_fail (plugin != NULL, 0);
  
  if (bse_type_from_name (type_name))
    {
      g_warning ("cannot register existing type `%s'", type_name);
      return 0;
    }
  pnode = LOOKUP_TYPE_NODE (parent_type);
  if (!pnode)
    {
      g_warning ("cannot derive type `%s' from invalid parent type `%s'",
                 type_name,
                 type_descriptive_name (parent_type));
      return 0;
    }
  
  if (!pnode->is_object &&
      BSE_FUNDAMENTAL_TYPE (parent_type) != parent_type)
    {
      g_warning ("parent type `%s' for type `%s' is not a fundamental type, "
                 "expected `%s'",
                 type_descriptive_name (parent_type),
                 type_name,
                 type_descriptive_name (BSE_FUNDAMENTAL_TYPE (parent_type)));
      return 0;
    }
  
#ifdef __FIXME  // need to adjust these checks
  if (!pnode->is_object &&
      (info->object_size || info->n_preallocs || info->object_init))
    {
      g_warning ("cannot derive object type `%s' from non-object parent type `%s'",
                 type_name,
                 type_descriptive_name (parent_type));
      return 0;
    }
  if (!pnode->is_classed &&
      (info->class_init || info->class_destroy || info->class_data ||
       (!pnode->is_iface && (info->base_init || info->base_destroy))))
    {
      g_warning ("cannot derive classed type `%s' from non-classed parent type `%s'",
                 type_name,
                 type_descriptive_name (parent_type));
      return 0;
    }
#endif
  if (pnode->is_classed)
    {
#ifdef __FIXME  // need to adjust these checks
      if ((info->class_size < sizeof (BseTypeClass)) ||
          (pnode->data && info->class_size < pnode->data->classed.class_size))
        {
          g_warning ("specified class size for type `%s' is smaller "
                     "than the parent's type `%s' class size",
                     type_name,
                     type_descriptive_name (parent_type));
          return 0;
        }
#endif
    }
  else if (!pnode->is_iface)
    {
      g_warning ("cannot derive type `%s' from unclassed parent type `%s'",
                 type_name,
                 type_descriptive_name (parent_type));
      return 0;
    }
  else if (parent_type != BSE_TYPE_INTERFACE)
    {
      g_warning ("cannot derive from interface types (`%s' from `%s')",
                 type_name,
                 NODE_NAME (pnode));
      return 0;
    }
#ifdef __FIXME  // need to adjust these checks
  else if (info->class_size < sizeof (BseTypeInterfaceClass))
    {
      g_warning ("specified class size for type `%s' is smaller "
                 "than the parent's `BseTypeInterfaceClass' class size",
                 type_name);
      return 0;
    }
#endif
  
  pnode = type_node_new (parent_type, type_name, type_blurb, plugin);
  type = NODE_TYPE (pnode);
  
  /* if (node->is_iface)
   *   type_data_make (node, NULL);
   */
  
  return type;
}

static void
type_data_make (TypeNode          *node,
		const BseTypeInfo *info)
{
  BseTypeInfo tmpinfo;
  TypeData *data;

  if (node->plugin)
    g_assert (node->data == NULL && info == NULL);
  else
    g_assert (node->data == NULL && info != NULL);
  
  if (node->plugin)
    {
      memset (&tmpinfo, 0, sizeof (tmpinfo));
      bse_plugin_ref (node->plugin);
      bse_plugin_complete_info (node->plugin, NODE_TYPE (node), &tmpinfo);
      info = &tmpinfo;
    }
  
  switch (BSE_FUNDAMENTAL_TYPE (NODE_TYPE (node)))
    {
    case BSE_TYPE_INTERFACE:
      data = g_malloc0 (sizeof (IFaceData));
      data->ref_count = 0;
      data->common.last_ref_handler = 0;
      data->iface.vtable_size = info->class_size;
      data->iface.vtable_init_base = info->base_init;
      data->iface.vtable_destroy_base = info->base_destroy;
      break;
    case BSE_TYPE_PROCEDURE:
    case BSE_TYPE_ENUM:
    case BSE_TYPE_FLAGS:
      data = g_malloc0 (sizeof (ClassedData));
      data->ref_count = 0;
      data->common.last_ref_handler = 0;
      data->classed.class_size = info->class_size;
      data->classed.class_init_base = info->base_init;
      data->classed.class_destroy_base = info->base_destroy;
      data->classed.class_init = info->class_init;
      data->classed.class_destroy = info->class_destroy;
      data->classed.class_data = info->class_data;
      data->classed.class = NULL;
      break;
    case BSE_TYPE_OBJECT:
      data = g_malloc0 (sizeof (ObjectData));
      data->ref_count = 0;
      data->common.last_ref_handler = 0;
      data->object.class_size = info->class_size;
      data->object.class_init_base = info->base_init;
      data->object.class_destroy_base = info->base_destroy;
      data->object.class_init = info->class_init;
      data->object.class_destroy = info->class_destroy;
      data->object.class_data = info->class_data;
      data->object.class = NULL;
      data->object.object_size = info->object_size;
      data->object.n_preallocs = MIN (info->n_preallocs, 1024);
#ifdef FIXME_DISABLE_PREALLOCATIONS
      data->object.n_preallocs = 0;
#endif
      data->object.object_init = info->object_init;
      data->object.mem_chunk = NULL;
      break;
    default:
      data = NULL;
      break;
    }
  node->data = data;
}

static void
type_data_blow (BseType type)
{
  TypeNode *node;
  
  node = LOOKUP_TYPE_NODE (type);
  
  g_assert (node && node->data && !node->data->ref_count && !node->data->common.last_ref_handler);
  g_assert (!node->is_classed || node->data->classed.class == NULL);
  g_assert (!node->is_object || node->data->object.mem_chunk == NULL);

  if (node->plugin)
    {
      bse_plugin_unref (node->plugin);
      g_free (node->data);
      node->data = NULL;
    }
}

static void
type_iface_entry_info_make (TypeNode         *node,
			    IFaceEntry       *entry,
			    BseInterfaceInfo *info)
{
  TypeNode *iface;
  
  iface = LOOKUP_TYPE_NODE (entry->iface);
  g_assert (iface && entry->info == NULL && info != IFACE_PARENT_INFO);
  if (iface->plugin)
    g_assert (info == NULL);
  else
    g_assert (info != NULL);

  /* keep in mind that we probably got called from bse_type_add_interface(),
   * which may be inside a type registering function, so certain restrictions
   * apply here
   */
  
  if (iface->plugin)
    {
      FIXME (unimplemented dynamic types);
    }
  
  entry->info = g_memdup (info, sizeof (BseInterfaceInfo));
}

static void
type_iface_entry_info_blow (BseType type,
			    BseType iface_type)
{
  TypeNode *node;
  TypeNode *iface;
  IFaceEntry *entry;
  
  node = LOOKUP_TYPE_NODE (type);
  iface = LOOKUP_TYPE_NODE (iface_type);
  entry = type_lookup_iface_entry (node, iface);
  
  g_assert (entry && entry->info && entry->info != IFACE_PARENT_INFO && entry->vtable == NULL);

  if (iface->plugin)
    {
      FIXME (unimplemented dynamic types);
      
      g_free (entry->info);
      entry->info = NULL;
    }
}

BseType* /* free result */
bse_type_children (BseType type,
                   guint  *n_children)
{
  TypeNode *node;
  
  node = LOOKUP_TYPE_NODE (type);
  if (node)
    {
      BseType *children;

      if (n_children)
        *n_children = node->n_children;
      
      children = g_new (BseType, node->n_children + 1);
      memcpy (children, node->children, sizeof (BseType) * node->n_children);
      children[node->n_children] = 0;

      return children;
    }
  else
    {
      if (n_children)
        *n_children = 0;
      
      return NULL;
    }
}

BseType* /* free result */
bse_type_interfaces (BseType type,
                     guint  *n_interfaces)
{
  TypeNode *node;
  
  node = LOOKUP_TYPE_NODE (type);
  if (node)
    {
      BseType *ifaces;
      guint i;

      if (n_interfaces)
        *n_interfaces = node->n_ifaces;

      ifaces = g_new (BseType, node->n_ifaces + 1);
      for (i = 0; i < node->n_ifaces; i++)
        ifaces[i] = node->ifaces[i].iface;
      ifaces[i] = 0;
      
      return ifaces;
    }
  else
    {
      if (n_interfaces)
        *n_interfaces = 0;
      
      return NULL;
    }
}

BseObject*
bse_type_create_object (BseType type)
{
  TypeNode *node;
  BseTypeStruct *tstruct;
  BseTypeClass *class;
  
  node = LOOKUP_TYPE_NODE (type);
  if (!node || !node->is_object)
    {
      g_warning ("cannot create new object for non-object type `%s'",
                 type_descriptive_name (type));
      return NULL;
    }
  
  class = bse_type_class_ref (type);
  
  if (node->n_supers)
    {
      TypeNode *pnode;
      
      pnode = LOOKUP_TYPE_NODE (NODE_PARENT_TYPE (node));
      if (node->data->object.object_size < pnode->data->object.object_size)
        {
          g_error ("specified object size for type `%s' is smaller "
                   "than the parent's type `%s' object size",
                   NODE_NAME (node), NODE_NAME (pnode));
        }
    }
  
  if (node->data->object.n_preallocs)
    {
      if (!node->data->object.mem_chunk)
        node->data->object.mem_chunk = g_mem_chunk_new (NODE_NAME (node),
                                                        node->data->object.object_size,
                                                        (node->data->object.object_size *
                                                         node->data->object.n_preallocs),
                                                        G_ALLOC_AND_FREE);
      tstruct = g_chunk_new0 (BseTypeStruct, node->data->object.mem_chunk);
    }
  else
    tstruct = g_malloc0 (node->data->object.object_size);
  
  if (node->n_supers)
    {
      guint i;
      
      for (i = node->n_supers; i > 0; i--)
        {
          TypeNode *pnode;
          
          pnode = LOOKUP_TYPE_NODE (node->supers[i]);
          if (pnode->data->object.object_init)
            {
              tstruct->bse_class = pnode->data->object.class;
              pnode->data->object.object_init ((BseObject*) tstruct, class);
            }
        }
      node = LOOKUP_TYPE_NODE (type);
    }
  tstruct->bse_class = class;
  if (node->data->object.object_init)
    node->data->object.object_init ((BseObject*) tstruct, class);
  
  return (BseObject*) tstruct;
}

void
bse_type_free_object (BseObject *object)
{
  TypeNode *node;
  BseTypeStruct *tstruct = (BseTypeStruct*) object;
  BseTypeClass *class;
  
  g_return_if_fail (BSE_CHECK_STRUCT_TYPE (object, BSE_TYPE_OBJECT));
  
  class = tstruct->bse_class;
  tstruct->bse_class = NULL;
  
  node = LOOKUP_TYPE_NODE (class->bse_type);
  if (!node || !node->is_object)
    {
      g_warning ("cannot free object of non-object type `%s'",
                 type_descriptive_name (class->bse_type));
      return;
    }
  
  if (node->data->object.n_preallocs)
    g_chunk_free (tstruct, node->data->object.mem_chunk);
  else
    g_free (tstruct);
  
  bse_type_class_unref (class);
}

BseType
bse_type_from_name (const gchar *name)
{
  GQuark quark;
  
  g_return_val_if_fail (name != NULL, 0);
  
  quark = g_quark_try_string (name);
  if (quark)
    {
      BseType type;
      TypeNode *node;
      
      type = GPOINTER_TO_UINT (g_hash_table_lookup (type_nodes_ht, GUINT_TO_POINTER (quark)));
      node = LOOKUP_TYPE_NODE (type);
      
      return node ? NODE_TYPE (node) : 0;
    }
  else
    return 0;
}

gchar*
bse_type_name (BseType type)
{
  TypeNode *node;
  
  node = LOOKUP_TYPE_NODE (type);
  
  return node ? NODE_NAME (node) : NULL;
}

BsePlugin*
bse_type_plugin (BseType type)
{
  TypeNode *node;

  node = LOOKUP_TYPE_NODE (type);

  return node ? node->plugin : NULL;
}

GQuark
bse_type_quark (BseType type)
{
  TypeNode *node;
  
  node = LOOKUP_TYPE_NODE (type);
  
  return node ? node->qname : 0;
}

gchar*
bse_type_blurb (BseType type)
{
  TypeNode *node;
  
  node = LOOKUP_TYPE_NODE (type);
  
  return node && node->qblurb ? g_quark_to_string (node->qblurb) : "";
}

BseType
bse_type_parent (BseType type)
{
  TypeNode *node;
  
  node = LOOKUP_TYPE_NODE (type);
  
  return node ? NODE_PARENT_TYPE (node) : 0;
}

gpointer
bse_type_class_peek_parent (gpointer type_class)
{
  TypeNode *node;
  
  g_return_val_if_fail (type_class != NULL, NULL);
  
  node = LOOKUP_TYPE_NODE (BSE_CLASS_TYPE (type_class));
  if (node && NODE_PARENT_TYPE (node))
    {
      node = LOOKUP_TYPE_NODE (NODE_PARENT_TYPE (node));
      
      return node->data->classed.class;
    }
  
  return NULL;
}

gboolean
bse_type_is_a (BseType type,
               BseType is_a_type)
{
  if (type != is_a_type)
    {
      TypeNode *node;
      
      node = LOOKUP_TYPE_NODE (type);
      if (node)
        {
          TypeNode *a_node;
          
          a_node = LOOKUP_TYPE_NODE (is_a_type);
          if (a_node && a_node->n_supers <= node->n_supers)
            return node->supers[node->n_supers - a_node->n_supers] == is_a_type;
        }
    }
  else
    return LOOKUP_TYPE_NODE (type) != NULL;
  
  return FALSE;
}

gboolean
bse_type_conforms_to (BseType type,
                      BseType iface_type)
{
  if (type != iface_type)
    {
      TypeNode *node;
      
      node = LOOKUP_TYPE_NODE (type);
      if (node)
        {
          TypeNode *check_node;
          
          check_node = LOOKUP_TYPE_NODE (iface_type);
          if (check_node)
            {
              if (check_node->is_iface && node->is_object)
                return type_lookup_iface_entry (node, check_node) != NULL;
              else if (node->is_iface && iface_type == BSE_TYPE_OBJECT)
                return TRUE;
              else if (check_node->n_supers <= node->n_supers)
                return node->supers[node->n_supers - check_node->n_supers] == iface_type;
            }
        }
    }
  else
    return ((BSE_TYPE_IS_INTERFACE (type) ||
	     BSE_TYPE_IS_OBJECT (type)) &&
	    LOOKUP_TYPE_NODE (type));
  
  return FALSE;
}

gboolean
bse_type_struct_conforms_to (BseTypeStruct *type_struct,
                             BseType        iface_type)
{
  return (type_struct && type_struct->bse_class &&
          bse_type_conforms_to (type_struct->bse_class->bse_type, iface_type));
}

gboolean
bse_type_class_is_a (BseTypeClass *type_class,
                     BseType       is_a_type)
{
  return (type_class && bse_type_is_a (type_class->bse_type, is_a_type));
}

BseTypeStruct*
bse_type_check_struct_cast (BseTypeStruct *bse_struct,
                            BseType        iface_type)
{
  if (!bse_struct)
    {
      g_warning ("invalid cast from (NULL) pointer to `%s'",
                 type_descriptive_name (iface_type));
      return bse_struct;
    }
  if (!bse_struct->bse_class)
    {
      g_warning ("invalid unclassed pointer in cast to `%s'",
                 type_descriptive_name (iface_type));
      return bse_struct;
    }
  if (!BSE_TYPE_IS_CLASSED (bse_struct->bse_class->bse_type))
    {
      g_warning ("invalid unclassed type `%s' in cast to `%s'",
                 type_descriptive_name (bse_struct->bse_class->bse_type),
                 type_descriptive_name (iface_type));
      return bse_struct;
    }
  if (!bse_type_conforms_to (bse_struct->bse_class->bse_type, iface_type))
    {
      g_warning ("invalid cast from `%s' to `%s'",
                 type_descriptive_name (bse_struct->bse_class->bse_type),
                 type_descriptive_name (iface_type));
      return bse_struct;
    }
  
  return bse_struct;
}

BseTypeClass*
bse_type_check_class_cast (BseTypeClass *bse_class,
                           BseType       is_a_type)
{
  if (!bse_class)
    {
      g_warning ("invalid class cast from (NULL) pointer to `%s'",
                 type_descriptive_name (is_a_type));
      return bse_class;
    }
  if (!BSE_TYPE_IS_CLASSED (bse_class->bse_type))
    {
      g_warning ("invalid unclassed type `%s' in class cast to `%s'",
                 type_descriptive_name (bse_class->bse_type),
                 type_descriptive_name (is_a_type));
      return bse_class;
    }
  if (!bse_type_is_a (bse_class->bse_type, is_a_type))
    {
      g_warning ("invalid class cast from `%s' to `%s'",
                 type_descriptive_name (bse_class->bse_type),
                 type_descriptive_name (is_a_type));
      return bse_class;
    }
  
  return bse_class;
}


static inline void
reset_info (BseTypeInfo *info)
{
  memset (info, 0, sizeof (BseTypeInfo));
  info->base_init = (BseBaseInitFunc) NULL;
  info->base_destroy = (BseBaseDestroyFunc) NULL;
  info->class_init = (BseClassInitFunc) NULL;
  info->class_destroy = (BseClassDestroyFunc) NULL;
  info->class_data = NULL;
  info->object_init = (BseObjectInitFunc) NULL;
}

/* include type id builtin variable declarations */
#include        "bsegentypes.c"

/* extern decls for other *.h files that implement fundamentals */
extern void     bse_type_register_procedure_info        (BseTypeInfo    *info);
extern void     bse_type_register_object_info           (BseTypeInfo    *info);
extern void     bse_type_register_enums                 (void);

void
bse_type_init (void)
{
  BseTypeInfo info;
  TypeNode *node;
  BseType type;
  static const struct {
    BseType      type;
    const gchar *name;
    const gchar *blurb;
  } param_types[] = {
    { BSE_TYPE_PARAM_BOOL,      "BseParamSpecBool",     "Boolean parameter", },
    { BSE_TYPE_PARAM_INT,       "BseParamSpecInt",      "Integer parameter", },
    { BSE_TYPE_PARAM_UINT,      "BseParamSpecUInt",     "Unsigned integer parameter", },
    { BSE_TYPE_PARAM_ENUM,      "BseParamSpecEnum",     "Enumeration parameter", },
    { BSE_TYPE_PARAM_FLAGS,     "BseParamSpecFlags",    "Flag enumeration parameter", },
    { BSE_TYPE_PARAM_FLOAT,     "BseParamSpecFloat",    "Floating point parameter", },
    { BSE_TYPE_PARAM_DOUBLE,    "BseParamSpecDouble",   "Double precision floating point parameter", },
    { BSE_TYPE_PARAM_TIME,      "BseParamSpecTime",     "Time parameter", },
    { BSE_TYPE_PARAM_NOTE,      "BseParamSpecNote",     "Note parameter", },
    { BSE_TYPE_PARAM_INDEX_2D,  "BseParamSpecIndex2D",  "Packed 2D index parameter", },
    { BSE_TYPE_PARAM_STRING,    "BseParamSpecString",   "String parameter", },
    { BSE_TYPE_PARAM_DOTS,      "BseParamSpecDots",     "Line graph parameter", },
    { BSE_TYPE_PARAM_ITEM,      "BseParamSpecItem",     "Item object reference", },
  };
  const guint n_param_types = sizeof (param_types) / sizeof (param_types[0]);
  static const struct {
    BseType *const type_p;
    BseType (*register_type) (void);
  } builtin_types[] = {
    /* include type id builtin variable declarations */
#include "bsegentype_array.c"
  };
  const guint n_builtin_types = sizeof (builtin_types) / sizeof (builtin_types[0]);
  guint i;

  g_return_if_fail (bse_n_type_nodes == 0);
  
  type_nodes_ht = g_hash_table_new (g_direct_hash, g_direct_equal);
  g_atexit (bse_type_debug);

  /* invalid type (0)
   */
  bse_n_type_nodes = 1;
  bse_type_nodes = g_renew (TypeNode*, NULL, 1);
  bse_type_nodes[0] = NULL;
  
  /* internal types
   */
  reset_info (&info);
  node = type_node_new (0, "BseNone", "", NULL);
  type = NODE_TYPE (node);
  type_data_make (node, &info);
  g_assert (type == BSE_TYPE_NONE);
  
  for (i = 0; i < n_param_types; i++)
    {
      reset_info (&info);
      node = type_node_new (0, param_types[i].name, param_types[i].blurb, NULL);
      type = NODE_TYPE (node);
      type_data_make (node, &info);
      g_assert (type == param_types[i].type);
    }
  
  reset_info (&info);
  node = type_node_new (0, "BseInterface", "BSE Interface base type", NULL);
  type = NODE_TYPE (node);
  type_data_make (node, &info);
  g_assert (type == BSE_TYPE_INTERFACE);
  
  reset_info (&info);
  bse_type_register_procedure_info (&info);
  node = type_node_new (0, "BseProcedure", "BSE Procedure base type", NULL);
  type = NODE_TYPE (node);
  type_data_make (node, &info);
  g_assert (type == BSE_TYPE_PROCEDURE);

  reset_info (&info);
  info.class_size = sizeof (BseEnumClass);
  info.class_init = (BseClassInitFunc) NULL;
  node = type_node_new (0, "BseEnum", "Enumeration base type", NULL);
  type = NODE_TYPE (node);
  type_data_make (node, &info);
  g_assert (type == BSE_TYPE_ENUM);
  
  reset_info (&info);
  info.class_size = sizeof (BseFlagsClass);
  info.class_init = (BseClassInitFunc) NULL;
  node = type_node_new (0, "BseFlags", "Flag enumeration base type", NULL);
  type = NODE_TYPE (node);
  type_data_make (node, &info);
  g_assert (type == BSE_TYPE_FLAGS);
  
  reset_info (&info);
  bse_type_register_object_info (&info);
  node = type_node_new (0, "BseObject", "BSE Object Hierarchy base type", NULL);
  type = NODE_TYPE (node);
  type_data_make (node, &info);
  g_assert (type == BSE_TYPE_OBJECT);

  /* initialize assistant fundamentals */
  bse_type_register_enums ();
  
  /* initialize builtin types */
  for (i = 0; i < n_builtin_types; i++)
    *(builtin_types[i].type_p) = builtin_types[i].register_type ();
}
