/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2003 Tim Janik
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
#include "bseitem.h"
#include "bsesuper.h"
#include "bsestorage.h"
#include "bseprocedure.h"
#include "bsemain.h"
#include "bseproject.h"
#include "bseundostack.h"
#include <gobject/gvaluecollector.h>
#include <string.h>

#define UNDO_DEBUG      sfi_debug_keyfunc ("undo")

enum {
  PROP_0,
  PROP_SEQID,
};


/* --- prototypes --- */
static void             bse_item_class_init_base        (BseItemClass           *class);
static void             bse_item_class_finalize_base    (BseItemClass           *class);
static void             bse_item_class_init             (BseItemClass           *class);
static void             bse_item_init                   (BseItem                *item);
static void             bse_item_set_property_internal  (GObject                *object,
                                                         guint                   param_id,
                                                         const GValue           *value,
                                                         GParamSpec             *pspec);
static void             bse_item_get_property_internal  (GObject                *object,
                                                         guint                   param_id,
                                                         GValue                 *value,
                                                         GParamSpec             *pspec);
static BseProxySeq*     bse_item_list_no_proxies        (BseItem                *item,
                                                         guint                   param_id,
                                                         GParamSpec             *pspec);
static void             bse_item_do_dispose             (GObject                *object);
static void             bse_item_do_finalize            (GObject                *object);
static void             bse_item_do_set_uname           (BseObject              *object,
                                                         const gchar            *uname);
static guint            bse_item_do_get_seqid           (BseItem                *item);
static void             bse_item_do_set_parent          (BseItem                *item,
                                                         BseItem                *parent);
static BseUndoStack*    bse_item_default_get_undo       (BseItem                *self);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static GSList     *item_seqid_changed_queue = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseItem)
{
  static const GTypeInfo item_info = {
    sizeof (BseItemClass),
    
    (GBaseInitFunc) bse_item_class_init_base,
    (GBaseFinalizeFunc) bse_item_class_finalize_base,
    (GClassInitFunc) bse_item_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseItem),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_item_init,
  };
  
  g_assert (BSE_ITEM_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT);
  
  return bse_type_register_abstract (BSE_TYPE_OBJECT,
                                     "BseItem",
                                     "Base type for objects managed by a container",
                                     &item_info);
}

static void
bse_item_class_init_base (BseItemClass *class)
{
  class->list_proxies = bse_item_list_no_proxies;
}

static void
bse_item_class_finalize_base (BseItemClass *class)
{
}

static void
bse_item_class_init (BseItemClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->get_property = bse_item_get_property_internal;
  gobject_class->set_property = bse_item_set_property_internal;
  gobject_class->dispose = bse_item_do_dispose;
  gobject_class->finalize = bse_item_do_finalize;
  
  object_class->set_uname = bse_item_do_set_uname;
  
  class->set_parent = bse_item_do_set_parent;
  class->get_seqid = bse_item_do_get_seqid;
  class->get_undo = bse_item_default_get_undo;

  bse_object_class_add_param (object_class, NULL,
                              PROP_SEQID,
                              sfi_pspec_int ("seqid", "Sequential ID", NULL,
                                             0, 0, SFI_MAXINT, 1, SFI_PARAM_GUI_READABLE));
}

static void
bse_item_init (BseItem *item)
{
  item->parent = NULL;
}

static void
bse_item_set_property_internal (GObject                *object,
                                guint                   param_id,
                                const GValue           *value,
                                GParamSpec             *pspec)
{
  // BseItem *self = BSE_ITEM (object);
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_item_get_property_internal (GObject                *object,
                                guint                   param_id,
                                GValue                 *value,
                                GParamSpec             *pspec)
{
  BseItem *self = BSE_ITEM (object);
  switch (param_id)
    {
    case PROP_SEQID:
      sfi_value_set_int (value, bse_item_get_seqid (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_item_do_dispose (GObject *gobject)
{
  BseItem *item = BSE_ITEM (gobject);

  /* force removal from parent */
  if (item->parent)
    bse_container_remove_item (BSE_CONTAINER (item->parent), item);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (gobject);
}

static void
bse_item_do_finalize (GObject *object)
{
  BseItem *item = BSE_ITEM (object);
  
  item_seqid_changed_queue = g_slist_remove (item_seqid_changed_queue, item);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
  
  g_return_if_fail (item->use_count == 0);
}

static BseProxySeq*
bse_item_list_no_proxies (BseItem    *item,
                          guint       param_id,
                          GParamSpec *pspec)
{
  return bse_proxy_seq_new ();
}

static void
bse_item_do_set_uname (BseObject   *object,
                       const gchar *uname)
{
  BseItem *item = BSE_ITEM (object);
  
  /* ensure that item names within their container are unique,
   * and that we don't end up with a NULL uname
   */
  if (!BSE_IS_CONTAINER (item->parent) ||
      (uname && !bse_container_lookup_item (BSE_CONTAINER (item->parent), uname)))
    {
      /* chain parent class' set_uname handler */
      BSE_OBJECT_CLASS (parent_class)->set_uname (object, uname);
    }
}

static void
bse_item_do_set_parent (BseItem *item,
                        BseItem *parent)
{
  item->parent = parent;
}

typedef struct {
  BseItem              *item;
  gpointer              data;
  BseProxySeq          *proxies;
  GType                 base_type;
  BseItemCheckContainer ccheck;
  BseItemCheckProxy     pcheck;
} GatherData;

static gboolean
gather_child (BseItem *child,
              gpointer data)
{
  GatherData *gdata = data;
  
  if (child != gdata->item &&
      g_type_is_a (G_OBJECT_TYPE (child), gdata->base_type) &&
      (!gdata->pcheck || gdata->pcheck (child, gdata->item, gdata->data)))
    bse_proxy_seq_append (gdata->proxies, BSE_OBJECT_ID (child));
  return TRUE;
}

/**
 * bse_item_gather_proxies
 * @item:      valid #BseItem from which to start gathering
 * @proxies:   sequence of proxies to append to
 * @base_type: base type of the proxies to gather
 * @ccheck:    container filter function
 * @pcheck:    proxy filter function
 * @data:      @data pointer to @ccheck and @pcheck
 * @RETURNS:   returns @proxies
 *
 * This function gathers items from an object hirachy, walking upwards,
 * starting out with @item. For each container passing @ccheck(), all
 * immediate children are tested for addition with @pcheck.
 */
BseProxySeq*
bse_item_gather_proxies (BseItem              *item,
                         BseProxySeq          *proxies,
                         GType                 base_type,
                         BseItemCheckContainer ccheck,
                         BseItemCheckProxy     pcheck,
                         gpointer              data)
{
  GatherData gdata;
  
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);
  g_return_val_if_fail (proxies != NULL, NULL);
  g_return_val_if_fail (g_type_is_a (base_type, BSE_TYPE_ITEM), NULL);
  
  gdata.item = item;
  gdata.data = data;
  gdata.proxies = proxies;
  gdata.base_type = base_type;
  gdata.ccheck = ccheck;
  gdata.pcheck = pcheck;
  
  item = BSE_IS_CONTAINER (item) ? item : item->parent;
  while (item)
    {
      BseContainer *container = BSE_CONTAINER (item);
      if (!gdata.ccheck || gdata.ccheck (container, gdata.item, gdata.data))
        bse_container_forall_items (container, gather_child, &gdata);
      item = item->parent;
    }
  return proxies;
}

static gboolean
gather_typed_ccheck (BseContainer   *container,
                     BseItem        *item,
                     gpointer        data)
{
  GType type = (GType) data;
  return g_type_is_a (G_OBJECT_TYPE (container), type);
}

static gboolean
gather_typed_acheck (BseItem  *proxy,
                     BseItem  *item,
                     gpointer  data)
{
  return proxy != item && !bse_item_has_ancestor (item, proxy);
}

/**
 * bse_item_gather_proxies_typed
 * @item:           valid #BseItem from which to start gathering
 * @proxies:        sequence of proxies to append to
 * @proxy_type:     base type of the proxies to gather
 * @container_type: base type of the containers to check for proxies
 * @RETURNS:   returns @proxies
 *
 * Variant of bse_item_gather_proxies(), the containers and proxies
 * are simply filtered by checking derivation from @container_type
 * and @proxy_type respectively.
 */
BseProxySeq*
bse_item_gather_proxies_typed (BseItem              *item,
                               BseProxySeq          *proxies,
                               GType                 proxy_type,
                               GType                 container_type,
                               gboolean              allow_ancestor)
{
  if (allow_ancestor)
    return bse_item_gather_proxies (item, proxies, proxy_type, gather_typed_ccheck, NULL, (gpointer) container_type);
  else
    return bse_item_gather_proxies (item, proxies, proxy_type,
                                    gather_typed_ccheck, gather_typed_acheck, (gpointer) container_type);
}

BseProxySeq*
bse_item_list_proxies (BseItem     *item,
                       const gchar *property)
{
  BseItemClass *class;
  GParamSpec *pspec;
  
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);
  g_return_val_if_fail (property != NULL, NULL);
  
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (item), property);
  if (!pspec)
    return NULL;
  class = g_type_class_peek (pspec->owner_type);
  return class->list_proxies (item, pspec->param_id, pspec);
}

BseItem*
bse_item_use (BseItem *item)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);
  g_return_val_if_fail (G_OBJECT (item)->ref_count > 0, NULL);
  
  if (!item->use_count)
    g_object_ref (item);
  item->use_count++;
  return item;
}

void
bse_item_unuse (BseItem *item)
{
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (item->use_count > 0);
  
  item->use_count--;
  if (!item->use_count)
    {
      if (!item->parent)
        g_object_run_dispose (G_OBJECT (item));
      g_object_unref (item);
    }
}

void
bse_item_set_parent (BseItem *item,
                     BseItem *parent)
{
  g_return_if_fail (BSE_IS_ITEM (item));
  if (parent)
    {
      g_return_if_fail (item->parent == NULL);
      g_return_if_fail (BSE_IS_CONTAINER (parent));
    }
  else
    g_return_if_fail (item->parent != NULL);
  g_return_if_fail (BSE_ITEM_GET_CLASS (item)->set_parent != NULL); /* paranoid */
  
  g_object_ref (item);
  if (parent)
    g_object_ref (parent);
  
  BSE_ITEM_GET_CLASS (item)->set_parent (item, parent);
  
  if (parent)
    g_object_unref (parent);
  else
    g_object_run_dispose (G_OBJECT (item));

  g_object_unref (item);
}

static guint
bse_item_do_get_seqid (BseItem *item)
{
  if (item->parent)
    return bse_container_get_item_seqid (BSE_CONTAINER (item->parent), item);
  else
    return 0;
}

static gboolean
idle_handler_seqid_changed (gpointer data)
{
  BSE_THREADS_ENTER ();
  
  while (item_seqid_changed_queue)
    {
      BseItem *item = g_slist_pop_head (&item_seqid_changed_queue);
      g_object_notify (item, "seqid");
    }
  
  BSE_THREADS_LEAVE ();
  
  return FALSE;
}

void
bse_item_queue_seqid_changed (BseItem *item)
{
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (BSE_ITEM (item)->parent != NULL);
  
  if (!item_seqid_changed_queue)
    bse_idle_notify (idle_handler_seqid_changed, NULL);
  
  if (!g_slist_find (item_seqid_changed_queue, item))
    item_seqid_changed_queue = g_slist_prepend (item_seqid_changed_queue, item);
}

guint
bse_item_get_seqid (BseItem *item)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), 0);
  g_return_val_if_fail (BSE_ITEM_GET_CLASS (item)->get_seqid != NULL, 0); /* paranoid */
  
  return BSE_ITEM_GET_CLASS (item)->get_seqid (item);
}

BseItem*
bse_item_common_ancestor (BseItem *item1,
                          BseItem *item2)
{
  g_return_val_if_fail (BSE_IS_ITEM (item1), NULL);
  g_return_val_if_fail (BSE_IS_ITEM (item2), NULL);
  
  do
    {
      BseItem *item = item2;
      
      do
        {
          if (item == item1)
            return item;
          item = item->parent;
        }
      while (item);
      item1 = item1->parent;
    }
  while (item1);
  
  return NULL;
}

/**
 * bse_item_cross_link
 * @owner:    reference owner
 * @link:     item to be referenced by @owner
 * @uncross_func: notifier to be executed on uncrossing
 *
 * Install a weak cross reference from @owner to @link.
 * The two items must have a common ancestor when the cross
 * link is installed. Once their ancestry changes so that
 * they don't have a common ancestor anymore, @uncross_func()
 * is executed.
 */
void
bse_item_cross_link (BseItem         *owner,
                     BseItem         *link,
                     BseItemUncross   uncross_func)
{
  BseItem *container;
  
  g_return_if_fail (BSE_IS_ITEM (owner));
  g_return_if_fail (BSE_IS_ITEM (link));
  g_return_if_fail (uncross_func != NULL);
  
  container = bse_item_common_ancestor (owner, link);
  
  if (container)
    _bse_container_cross_link (BSE_CONTAINER (container), owner, link, uncross_func);
  else
    g_warning ("%s: %s and %s have no common anchestor", G_STRLOC,
               bse_object_debug_name (owner),
               bse_object_debug_name (link));
}

/**
 * bse_item_cross_unlink
 * @owner:    reference owner
 * @link:     item referenced by @owner
 * @uncross_func: notifier queued to be executed on uncrossing
 *
 * Removes a cross link previously installed via
 * bse_item_cross_link() without executing @uncross_func().
 */
void
bse_item_cross_unlink (BseItem        *owner,
                       BseItem        *link,
                       BseItemUncross  uncross_func)
{
  BseItem *container;
  
  g_return_if_fail (BSE_IS_ITEM (owner));
  g_return_if_fail (BSE_IS_ITEM (link));
  g_return_if_fail (uncross_func != NULL);
  
  container = bse_item_common_ancestor (owner, link);
  
  if (container)
    _bse_container_cross_unlink (BSE_CONTAINER (container), owner, link, uncross_func);
  else
    g_warning ("%s: `%s' and `%s' have no common anchestor", G_STRLOC,
               BSE_OBJECT_TYPE_NAME (owner),
               BSE_OBJECT_TYPE_NAME (link));
}

/**
 * bse_item_uncross
 * @owner:    reference owner
 * @link:     item referenced by @owner
 *
 * Destroys all existing cross links from @owner to
 * @link by executing the associated notifiers.
 */
void
bse_item_uncross (BseItem *owner,
                  BseItem *link)
{
  BseItem *container;
  
  g_return_if_fail (BSE_IS_ITEM (owner));
  g_return_if_fail (BSE_IS_ITEM (link));
  
  container = bse_item_common_ancestor (owner, link);
  
  if (container)
    _bse_container_uncross (BSE_CONTAINER (container), owner, link);
}

BseSuper*
bse_item_get_super (BseItem *item)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);
  
  while (!BSE_IS_SUPER (item) && item)
    item = item->parent;
  
  return item ? BSE_SUPER (item) : NULL;
}

BseItem*
bse_item_get_toplevel (BseItem *item)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);

  while (item->parent)
    item = item->parent;

  return item;
}

BseProject*
bse_item_get_project (BseItem *item)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), NULL);

  while (item->parent)
    item = item->parent;

  return BSE_IS_PROJECT (item) ? (BseProject*) item : NULL;
}

gboolean
bse_item_has_ancestor (BseItem *item,
                       BseItem *ancestor)
{
  g_return_val_if_fail (BSE_IS_ITEM (item), FALSE);
  g_return_val_if_fail (BSE_IS_ITEM (ancestor), FALSE);
  
  while (item->parent)
    {
      item = item->parent;
      if (item == ancestor)
        return TRUE;
    }
  
  return FALSE;
}

static inline GType
find_method_procedure (GType        object_type,
                       const gchar *method_name)
{
  guint l2 = strlen (method_name);
  GType proc_type, type = object_type; /* assumed to be *derived* from BSE_TYPE_ITEM */
  do
    {
      gchar *name, *type_name = g_type_name (type);
      guint l1 = strlen (type_name);

      name = g_new (gchar, l1 + 1 + l2 + 1);
      memcpy (name, type_name, l1);
      name[l1] = '+';
      memcpy (name + l1 + 1, method_name, l2);
      name[l1 + 1 + l2] = 0;

      proc_type = bse_procedure_lookup (name);
      g_free (name);
      if (proc_type)
        break;
      type = g_type_parent (type);
    }
  while (type != BSE_TYPE_ITEM); /* type will become BSE_TYPE_ITEM eventually */
  return proc_type;
}

static inline BseErrorType
bse_item_execva_i (BseItem     *item,
                   const gchar *procedure,
                   va_list      var_args,
                   gboolean     skip_oparams)
{
  BseErrorType error;
  GType proc_type = find_method_procedure (BSE_OBJECT_TYPE (item), procedure);
  GValue obj_value;

  if (!proc_type)
    {
      g_warning ("no such method \"%s\" of item %s",
                 procedure, bse_object_debug_name (item));
      return BSE_ERROR_INTERNAL;
    }

  /* setup first arg (the object) */
  obj_value.g_type = 0;
  g_value_init (&obj_value, BSE_TYPE_ITEM);
  g_value_set_object (&obj_value, item);
  /* invoke procedure */
  error = bse_procedure_marshal_valist (proc_type, &obj_value, NULL, NULL, skip_oparams, var_args);
  g_value_unset (&obj_value);
  return error;
}

BseErrorType
bse_item_exec (gpointer     _item,
               const gchar *procedure,
               ...)
{
  BseItem *item = _item;
  va_list var_args;
  BseErrorType error;
  
  g_return_val_if_fail (BSE_IS_ITEM (item), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (procedure != NULL, BSE_ERROR_INTERNAL);
  
  va_start (var_args, procedure);
  error = bse_item_execva_i (item, procedure, var_args, FALSE);
  va_end (var_args);
  
  return error;
}

BseErrorType
bse_item_exec_void_proc (gpointer     _item,
                         const gchar *procedure,
                         ...)
{
  BseItem *item = _item;
  va_list var_args;
  BseErrorType error;
  
  g_return_val_if_fail (BSE_IS_ITEM (item), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (procedure != NULL, BSE_ERROR_INTERNAL);
  
  va_start (var_args, procedure);
  error = bse_item_execva_i (item, procedure, var_args, TRUE);
  va_end (var_args);
  
  return error;
}

static GValue*
pack_value_for_undo (GValue       *value,
                     BseUndoStack *ustack)
{
  GType type = G_VALUE_TYPE (value);
  if (G_TYPE_IS_OBJECT (type))
    {
      gchar *p = bse_undo_pointer_pack (g_value_get_object (value), ustack);
      g_value_unset (value);
      g_value_init (value, BSE_TYPE_PACKED_POINTER);
      sfi_value_take_string (value, p);
    }
  return value;
}

static GValue*
unpack_value_from_undo (GValue       *value,
                        BseUndoStack *ustack)
{
  GType type = G_VALUE_TYPE (value);
  if (type == BSE_TYPE_PACKED_POINTER)
    {
      BseItem *item = bse_undo_pointer_unpack (g_value_get_string (value), ustack);
      g_value_unset (value);
      g_value_init (value, G_TYPE_OBJECT);
      g_value_set_object (value, item);
    }
  return value;
}

static void
unde_free_proc (BseUndoStep *ustep)
{
  BseProcedureClass *proc = ustep->data[0].v_pointer;
  GValue *ivalues = ustep->data[1].v_pointer; /* may or may not packed for undo */
  if (ivalues && proc)
    {
      guint i;
      for (i = 0; i < proc->n_in_pspecs; i++)
        g_value_unset (ivalues + i);
      g_free (ivalues);
      g_type_class_unref (proc);
    }
}

static void
undo_call_proc (BseUndoStep  *ustep,
                BseUndoStack *ustack)
{
  BseProcedureClass *proc = ustep->data[0].v_pointer;
  GValue *ivalues = ustep->data[1].v_pointer; /* packed for undo */
  gboolean commit_as_redo = ustep->data[2].v_num;
  if (commit_as_redo)
    {
      const gchar *packed_item_pointer = g_value_get_string (ivalues + 0);
      BseItem *item = bse_undo_pointer_unpack (packed_item_pointer, ustack);
      BseUndoStack *redo_stack = bse_item_undo_open (item, proc->name);
      BseUndoStep *redo_step;
      redo_step = bse_undo_step_new (undo_call_proc, unde_free_proc, 3);
      redo_step->data[0].v_pointer = proc;
      redo_step->data[1].v_pointer = ivalues;
      redo_step->data[2].v_num = FALSE; /* not commit_as_redo again */
      bse_undo_stack_push (redo_stack, redo_step);
      bse_item_undo_close (redo_stack);
      /* prevent premature deletion */
      ustep->data[0].v_pointer = NULL;
      ustep->data[1].v_pointer = NULL;
    }
  else /* invoke procedure */
    {
      GValue ovalue = { 0, };
      BseErrorType error;
      guint i;
      /* convert values from undo */
      for (i = 0; i < proc->n_in_pspecs; i++)
        unpack_value_from_undo (ivalues + i, ustack);
      /* setup return value (maximum one) */
      if (proc->n_out_pspecs)
        g_value_init (&ovalue, G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[0]));
      /* invoke procedure */
      error = bse_procedure_marshal (BSE_PROCEDURE_TYPE (proc), ivalues, &ovalue, NULL, NULL);
      /* clenup return value */
      if (proc->n_out_pspecs)
        {
          /* check returned error if any */
          if (G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[0]) == BSE_TYPE_ERROR_TYPE && !error)
            error = g_value_get_enum (&ovalue);
          g_value_unset (&ovalue);
        }
      /* we're not tolerating any errors */
      if (error)
        g_warning ("while executing undo method \"%s\" of item %s: %s", proc->name,
                   bse_object_debug_name (g_value_get_object (ivalues + 0)), bse_error_blurb (error));
    }
}

static void
bse_item_push_undo_proc_valist (gpointer     item,
                                const gchar *procedure,
                                gboolean     commit_as_redo,
                                va_list      var_args)
{
  GType proc_type = find_method_procedure (BSE_OBJECT_TYPE (item), procedure);
  BseUndoStack *ustack = bse_item_undo_open (item, "%s: %s", commit_as_redo ? "redo-proc" : "undo-proc", procedure);
  BseProcedureClass *proc;
  GValue *ivalues;
  BseErrorType error;
  guint i;
  if (BSE_UNDO_STACK_VOID (ustack))
    {
      bse_item_undo_close (ustack);
      return;
    }
  if (!proc_type)
    {
      g_warning ("no such method \"%s\" of item %s",
                 procedure, bse_object_debug_name (item));
      bse_item_undo_close (ustack);
      return;
    }

  proc = g_type_class_ref (proc_type);
  /* we allow one return value */
  if (proc->n_out_pspecs > 1)
    {
      g_warning ("method \"%s\" of item %s called with more than one return value",
                 procedure, bse_object_debug_name (item));
      g_type_class_unref (proc);
      bse_item_undo_close (ustack);
      return;
    }

  ivalues = g_new (GValue, proc->n_in_pspecs);
  /* setup first arg (the object) */
  ivalues[0].g_type = 0;
  g_value_init (ivalues + 0, BSE_TYPE_ITEM);
  g_value_set_object (ivalues + 0, item);

  /* collect procedure args */
  error = bse_procedure_collect_input_args (proc, ivalues + 0, var_args, ivalues);
  if (!error)
    {
      BseUndoStep *ustep = bse_undo_step_new (undo_call_proc, unde_free_proc, 3);
      /* convert values for undo */
      for (i = 0; i < proc->n_in_pspecs; i++)
        pack_value_for_undo (ivalues + i, ustack);
      ustep->data[0].v_pointer = proc;
      ustep->data[1].v_pointer = ivalues;
      ustep->data[2].v_num = commit_as_redo;
      bse_undo_stack_push (ustack, ustep);
    }
  else /* urg shouldn't happen */
    {
      g_warning ("while collecting arguments for method \"%s\" of item %s: %s",
                 procedure, bse_object_debug_name (item), bse_error_blurb (error));
      for (i = 0; i < proc->n_in_pspecs; i++)
        g_value_unset (ivalues + i);
      g_free (ivalues);
      g_type_class_unref (proc);
    }
  bse_item_undo_close (ustack);
}

void
bse_item_push_undo_proc (gpointer         item,
                         const gchar     *procedure,
                         ...)
{
  va_list var_args;

  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (procedure != NULL);

  UNDO_DEBUG ("push undo: %s %s", bse_object_debug_name (item), procedure);

  va_start (var_args, procedure);
  bse_item_push_undo_proc_valist (item, procedure, FALSE, var_args);
  va_end (var_args);
}

void
bse_item_push_redo_proc (gpointer         item,
                         const gchar     *procedure,
                         ...)
{
  va_list var_args;

  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (procedure != NULL);

  UNDO_DEBUG ("push redo: %s %s", bse_object_debug_name (item), procedure);

  va_start (var_args, procedure);
  bse_item_push_undo_proc_valist (item, procedure, TRUE, var_args);
  va_end (var_args);
}

void
bse_item_set (gpointer        object,
              const gchar    *first_property_name,
              ...)
{
  va_list var_args;
  
  g_return_if_fail (BSE_IS_ITEM (object));

  va_start (var_args, first_property_name);
  bse_item_set_valist_undoable (object, first_property_name, var_args);
  va_end (var_args);
}

void
bse_item_set_valist_undoable (gpointer     object,
                              const gchar *first_property_name,
                              va_list      var_args)
{
  BseItem *self = object;
  const gchar *name;

  g_return_if_fail (BSE_IS_ITEM (self));

  g_object_ref (object);
  g_object_freeze_notify (object);

  name = first_property_name;
  while (name)
    {
      GValue value = { 0, };
      GParamSpec *pspec;
      gchar *error = NULL;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self), name);
      if (!pspec)
        {
          g_warning ("item %s has no property named `%s'",
                     bse_object_debug_name (self), name);
          break;
        }
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      G_VALUE_COLLECT (&value, var_args, 0, &error);
      if (error)
        {
          g_warning ("while setting property `%s' on %s: %s",
                     name, bse_object_debug_name (self), error);
          g_free (error);
          g_value_unset (&value);
          break;
        }
      bse_item_set_property_undoable (self, pspec->name, &value);
      g_value_unset (&value);
      name = va_arg (var_args, gchar*);
    }
  g_object_thaw_notify (object);
  g_object_unref (object);
}

static BseUndoStack*
bse_item_default_get_undo (BseItem *self)
{
  if (self->parent)
    return BSE_ITEM_GET_CLASS (self->parent)->get_undo (self->parent);
  else
    return NULL;
}

static gboolean
values_equal_for_undo (const GValue *v1,
                       const GValue *v2)
{
  SfiSCategory sc1 = sfi_categorize_type (G_VALUE_TYPE (v1));
  SfiSCategory sc2 = sfi_categorize_type (G_VALUE_TYPE (v2));
  if (sc1 != sc2)
    return FALSE;
  switch (sc1)
    {
    case SFI_SCAT_BOOL:         return sfi_value_get_bool (v1) == sfi_value_get_bool (v2);
    case SFI_SCAT_INT:          return sfi_value_get_int (v1) == sfi_value_get_int (v2);
    case SFI_SCAT_NUM:          return sfi_value_get_num (v1) == sfi_value_get_num (v2);
    case SFI_SCAT_REAL:         return sfi_value_get_real (v1) == sfi_value_get_real (v2); /* *no* epsilon! */
    case SFI_SCAT_CHOICE:       
    case SFI_SCAT_STRING:       return bse_string_equals (sfi_value_get_string (v1), sfi_value_get_string (v2));
    default:
      if (G_TYPE_IS_OBJECT (G_VALUE_TYPE (v1)) &&
          G_TYPE_IS_OBJECT (G_VALUE_TYPE (v2)))
        return g_value_get_object (v1) == g_value_get_object (v2);
    }
  return FALSE;
}

static void
undo_set_property (BseUndoStep  *ustep,
                   BseUndoStack *ustack)
{
  bse_item_set_property_undoable (bse_undo_pointer_unpack (ustep->data[0].v_pointer, ustack),
                                  ustep->data[1].v_pointer,
                                  unpack_value_from_undo (ustep->data[2].v_pointer, ustack));
}

static void
unde_free_property (BseUndoStep *ustep)
{
  g_free (ustep->data[0].v_pointer);
  g_free (ustep->data[1].v_pointer);
  g_value_unset (ustep->data[2].v_pointer); /* may or may not be unpacked */
  g_free (ustep->data[2].v_pointer);
}

void
bse_item_set_property_undoable (BseItem      *self,
                                const gchar  *name,
                                const GValue *value)
{
  BseUndoStack *ustack = bse_item_undo_open (self, "set-property(%s,\"%s\")", bse_object_debug_name (self), name);
  BseUndoStep *ustep;
  GValue *tvalue = g_new0 (GValue, 1);
#if 0
  const BseUndoStep *lstep = bse_undo_group_peek_last_atom (ustack, NULL);
  if (lstep && lstep->undo_func == undo_set_property &&
      strcmp (lstep->data[1].v_pointer, name) == 0 &&
      bse_undo_pointer_unpack (lstep->data[0].v_pointer, ustack) == (gpointer) self)
    ; /* try to coalesce with last... */
#endif
  g_value_init (tvalue, G_VALUE_TYPE (value));
  g_object_get_property (G_OBJECT (self), name, tvalue);
  /* try to coalesce with last */
  if (values_equal_for_undo (value, tvalue))
    {
      /* we're about to set the same value again => skip undo */
      g_value_unset (tvalue);
      g_free (tvalue);
      bse_item_undo_close (ustack);
      g_object_set_property (G_OBJECT (self), name, value);
      return;
    }
  g_object_set_property (G_OBJECT (self), name, value);
  /* pointer-pack must be called *after* property update (could be "uname") */
  ustep = bse_undo_step_new (undo_set_property, unde_free_property, 3);
  ustep->data[0].v_pointer = bse_undo_pointer_pack (self, ustack);
  ustep->data[1].v_pointer = g_strdup (name);
  ustep->data[2].v_pointer = pack_value_for_undo (tvalue, ustack);
  bse_undo_stack_push (ustack, ustep);
  bse_item_undo_close (ustack);
}

BseUndoStack*
bse_item_undo_open (gpointer     item,
                    const gchar *format,
                    ...)
{
  BseItem     *self = item;
  BseUndoStack *ustack;
  gchar *buffer;
  va_list args;

  g_return_val_if_fail (format != NULL, NULL);

  ustack = BSE_ITEM_GET_CLASS (self)->get_undo (self);
  va_start (args, format);
  buffer = g_strdup_vprintf (format, args);
  va_end (args);
  if (ustack)
    bse_undo_group_open (ustack, buffer);
  else
    {
      gchar *str = g_strconcat ("DUMMY-GROUP(", buffer, ")", NULL);
      ustack = bse_undo_stack_dummy ();
      bse_undo_group_open (ustack, str);
      g_free (str);
    }
  g_free (buffer);
  return ustack;
}

void
bse_item_undo_close (BseUndoStack *ustack)
{
  if (ustack)
    bse_undo_group_close (ustack);
}

static void
undo_restore_item (BseUndoStep  *ustep,
                   BseUndoStack *ustack)
{
  BseItem *item = bse_undo_pointer_unpack (ustep->data[0].v_pointer, ustack);
  BseStorage *storage = g_object_new (BSE_TYPE_STORAGE, NULL);
  GTokenType expected_token = G_TOKEN_NONE;
  GScanner *scanner;

  bse_storage_input_text (storage, ustep->data[1].v_pointer);
  scanner = bse_storage_get_scanner (storage);

  expected_token = bse_storage_restore_item (storage, item);
  if (expected_token != G_TOKEN_NONE)
    bse_storage_unexp_token (storage, expected_token);

  bse_storage_resolve_item_links (storage);

  bse_storage_reset (storage);
  g_object_unref (storage);
}

static void
unde_free_item (BseUndoStep *ustep)
{
  g_free (ustep->data[0].v_pointer);
  g_free (ustep->data[1].v_pointer);
}

void
bse_item_push_undo_storage (BseItem         *self,
                            BseUndoStack    *ustack,
                            BseStorage      *storage)
{
  BseUndoStep *ustep;
  gpointer mem = bse_storage_mem_flush (storage);
  bse_storage_reset (storage);
  ustep = bse_undo_step_new (undo_restore_item, unde_free_item, 2);
  ustep->data[0].v_pointer = bse_undo_pointer_pack (self, ustack);
  ustep->data[1].v_pointer = mem;
  bse_undo_stack_push (ustack, ustep);
}

void
bse_item_backup_to_undo (BseItem      *self,
                         BseUndoStack *ustack)
{
  if (!BSE_UNDO_STACK_VOID (ustack))
    {
      BseStorage *storage = g_object_new (BSE_TYPE_STORAGE, NULL);
      bse_storage_prepare_write (storage, BSE_STORAGE_SKIP_DEFAULTS);
      bse_storage_store_item (storage, self);
      
      bse_item_push_undo_storage (self, ustack, storage);
      g_object_unref (storage);
    }
}
