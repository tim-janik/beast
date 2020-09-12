// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseitem.hh"
#include "bsesuper.hh"
#include "bsesnet.hh"
#include "bsestorage.hh"
#include "bsemain.hh"
#include "bseproject.hh"
#include "bsesong.hh" // for song->musical_tuning
#include "bseundostack.hh"
#include "bse/internal.hh"
#include <gobject/gvaluecollector.h>
#include <string.h>

/* --- prototypes --- */
static void             bse_item_class_init_base        (BseItemClass           *klass);
static void             bse_item_class_finalize_base    (BseItemClass           *klass);
static void             bse_item_class_init             (BseItemClass           *klass);
static void             bse_item_init                   (BseItem                *item);
static void             bse_item_update_state           (BseItem                *self);
static gboolean         bse_item_real_needs_storage     (BseItem                *self,
                                                         BseStorage             *storage);
static void             bse_item_do_dispose             (GObject                *object);
static void             bse_item_do_finalize            (GObject                *object);
static void             bse_item_do_set_uname           (BseObject              *object,
                                                         const char             *uname);
static uint             bse_item_do_get_seqid           (BseItem                *item);
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

  assert_return (BSE_ITEM_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT, 0);

  return bse_type_register_abstract (BSE_TYPE_OBJECT,
                                     "BseItem",
                                     "Base type for objects managed by a container",
                                     __FILE__, __LINE__,
                                     &item_info);
}

static void
bse_item_class_init_base (BseItemClass *klass)
{
  klass->get_candidates = NULL;
}

static void
bse_item_class_finalize_base (BseItemClass *klass)
{
}

static void
bse_item_class_init (BseItemClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);

  parent_class = (GTypeClass*) g_type_class_peek_parent (klass);

  gobject_class->dispose = bse_item_do_dispose;
  gobject_class->finalize = bse_item_do_finalize;

  object_class->set_uname = bse_item_do_set_uname;

  klass->set_parent = bse_item_do_set_parent;
  klass->get_seqid = bse_item_do_get_seqid;
  klass->get_undo = bse_item_default_get_undo;
  klass->needs_storage = bse_item_real_needs_storage;
}

static void
bse_item_init (BseItem *item)
{
  item->parent = NULL;
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

  assert_return (item->use_count == 0);
}

static void
bse_item_do_set_uname (BseObject  *object,
                       const char *uname)
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
bse_item_do_set_parent (BseItem *self,
                        BseItem *parent)
{
  self->parent = parent;
  bse_item_update_state (self);
}

static gboolean
recurse_update_state (BseItem *self,
                      void    *data)
{
  bse_item_update_state (self);
  return TRUE;
}

static void
bse_item_update_state (BseItem *self)
{
  /* save original state */
  gboolean old_internal = BSE_ITEM_INTERNAL (self);

  /* update state */
  if ((BSE_OBJECT_FLAGS (self) & BSE_ITEM_FLAG_INTERN) ||
      (self->parent && BSE_ITEM_INTERNAL (self->parent)))
    self->set_flag (BSE_ITEM_FLAG_INTERN_BRANCH);
  else
    self->unset_flag (BSE_ITEM_FLAG_INTERN_BRANCH);

  /* compare state and recurse if necessary */
  if (BSE_IS_CONTAINER (self) && (old_internal != BSE_ITEM_INTERNAL (self)))
    bse_container_forall_items ((BseContainer*) self, recurse_update_state, NULL);
}

/**
 * @param item	   valid BseItem
 * @param internal TRUE or FALSE
 *
 * Set whether an item should be considered internal to the BSE
 * implementation (or implementation of another BSE object).
 * Internal items are not stored with their parents and undo
 * is not recorded for internal items either. Marking containers
 * internal also affects any children they contain, in effect,
 * the whole posterity spawned by the container is considered
 * internal.
 */
void
bse_item_set_internal (void    *item,
                       gboolean internal)
{
  BseItem *self = BSE_ITEM (item);

  assert_return (BSE_IS_ITEM (self));

  if (internal)
    self->set_flag (BSE_ITEM_FLAG_INTERN);
  else
    self->unset_flag (BSE_ITEM_FLAG_INTERN);
  bse_item_update_state (self);
}

static gboolean
bse_item_real_needs_storage (BseItem    *self,
                             BseStorage *storage)
{
  return TRUE;
}

gboolean
bse_item_needs_storage (BseItem    *self,
                        BseStorage *storage)
{
  assert_return (BSE_IS_ITEM (self), FALSE);
  assert_return (BSE_IS_STORAGE (storage), FALSE);

  return BSE_ITEM_GET_CLASS (self)->needs_storage (self, storage);
}

void
bse_item_compat_setup (BseItem         *self,
                       uint             vmajor,
                       uint             vminor,
                       uint             vmicro)
{
  assert_return (BSE_IS_ITEM (self));

  if (BSE_ITEM_GET_CLASS (self)->compat_setup)
    BSE_ITEM_GET_CLASS (self)->compat_setup (self, vmajor, vminor, vmicro);
}

struct GatherData {
  BseItem              *item;
  void                 *data;
  Bse::ItemSeq         &iseq;
  GType                 base_type;
  BseItemCheckContainer ccheck;
  BseItemCheckProxy     pcheck;
  GatherData (Bse::ItemSeq &is) : iseq (is) {}
};

static gboolean
gather_child (BseItem *child,
              void    *data)
{
  GatherData *gdata = (GatherData*) data;

  if (child != gdata->item && !BSE_ITEM_INTERNAL (child) &&
      g_type_is_a (G_OBJECT_TYPE (child), gdata->base_type) &&
      (!gdata->pcheck || gdata->pcheck (child, gdata->item, gdata->data)))
    gdata->iseq.push_back (child->as<Bse::ItemIfaceP>());
  return TRUE;
}

/**
 * @param item	        valid BseItem from which to start gathering
 * @param items	        sequence of items to append to
 * @param base_type	base type of the items to gather
 * @param ccheck	container filter function
 * @param pcheck	proxy filter function
 * @param data	        @a data pointer to @a ccheck and @a pcheck
 *
 * This function gathers items from an object hierachy, walking upwards,
 * starting out with @a item. For each container passing @a ccheck(), all
 * immediate children are tested for addition with @a pcheck.
 */
static void
bse_item_gather_items (BseItem *item, Bse::ItemSeq &iseq, GType base_type, BseItemCheckContainer ccheck, BseItemCheckProxy pcheck, void *data)
{
  GatherData gdata (iseq);
  assert_return (BSE_IS_ITEM (item));
  assert_return (g_type_is_a (base_type, BSE_TYPE_ITEM));

  gdata.item = item;
  gdata.data = data;
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
}

static gboolean
gather_typed_ccheck (BseContainer   *container,
                     BseItem        *item,
                     void           *data)
{
  GType type = (GType) data;
  return g_type_is_a (G_OBJECT_TYPE (container), type);
}

static gboolean
gather_typed_acheck (BseItem  *proxy,
                     BseItem  *item,
                     void     *data)
{
  return proxy != item && !bse_item_has_ancestor (item, proxy);
}

/**
 * @param item	         valid BseItem from which to start gathering
 * @param items	         sequence of items to append to
 * @param proxy_type	 base type of the items to gather
 * @param container_type base type of the containers to check for items
 * @param allow_ancestor if FALSE, ancestors of @a item are omitted
 *
 * Variant of bse_item_gather_items(), the containers and items
 * are simply filtered by checking derivation from @a container_type
 * and @a proxy_type respectively. Gathered items may not be ancestors
 * of @a item if @a allow_ancestor is @a false.
 */
void
bse_item_gather_items_typed (BseItem *item, Bse::ItemSeq &iseq, GType proxy_type, GType container_type, bool allow_ancestor)
{
  if (allow_ancestor)
    bse_item_gather_items (item, iseq, proxy_type, gather_typed_ccheck, NULL, (void*) container_type);
  else
    bse_item_gather_items (item, iseq, proxy_type, gather_typed_ccheck, gather_typed_acheck, (void*) container_type);
}

gboolean
bse_item_get_candidates (BseItem *item, const Bse::String &property, Bse::PropertyCandidates &pc)
{
  BseItemClass *klass;
  GParamSpec *pspec;

  assert_return (BSE_IS_ITEM (item), FALSE);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (item), property.c_str());
  if (!pspec)
    return FALSE;
  klass = (BseItemClass*) g_type_class_peek (pspec->owner_type);
  if (klass && klass->get_candidates)
    klass->get_candidates (item, pspec->param_id, pc, pspec);
  return TRUE;
}

BseItem*
bse_item_use (BseItem *item)
{
  assert_return (BSE_IS_ITEM (item), NULL);
  assert_return (G_OBJECT (item)->ref_count > 0, NULL);

  if (!item->use_count)
    g_object_ref (item);
  item->use_count++;
  return item;
}

void
bse_item_unuse (BseItem *item)
{
  assert_return (BSE_IS_ITEM (item));
  assert_return (item->use_count > 0);

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
  assert_return (BSE_IS_ITEM (item));
  if (parent)
    {
      assert_return (item->parent == NULL);
      assert_return (BSE_IS_CONTAINER (parent));
    }
  else
    assert_return (item->parent != NULL);
  assert_return (BSE_ITEM_GET_CLASS (item)->set_parent != NULL); /* paranoid */

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

static uint
bse_item_do_get_seqid (BseItem *item)
{
  if (item->parent)
    return bse_container_get_item_seqid (BSE_CONTAINER (item->parent), item);
  else
    return 0;
}

static gboolean
idle_handler_seqid_changed (void *data)
{
  BSE_THREADS_ENTER ();

  while (item_seqid_changed_queue)
    {
      BseItem *item = (BseItem*) g_slist_pop_head (&item_seqid_changed_queue);
      auto impl = item->as<Bse::ItemImpl*>();
      impl->notify ("seqid");
    }

  BSE_THREADS_LEAVE ();

  return FALSE;
}

void
bse_item_queue_seqid_changed (BseItem *item)
{
  assert_return (BSE_IS_ITEM (item));
  assert_return (BSE_ITEM (item)->parent != NULL);

  if (!item_seqid_changed_queue)
    bse_idle_notify (idle_handler_seqid_changed, NULL);

  if (!g_slist_find (item_seqid_changed_queue, item))
    item_seqid_changed_queue = g_slist_prepend (item_seqid_changed_queue, item);
}

uint
bse_item_get_seqid (BseItem *item)
{
  assert_return (BSE_IS_ITEM (item), 0);
  assert_return (BSE_ITEM_GET_CLASS (item)->get_seqid != NULL, 0); /* paranoid */

  return BSE_ITEM_GET_CLASS (item)->get_seqid (item);
}

BseItem*
bse_item_common_ancestor (BseItem *item1,
                          BseItem *item2)
{
  assert_return (BSE_IS_ITEM (item1), NULL);
  assert_return (BSE_IS_ITEM (item2), NULL);

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
 * @param owner	        reference owner
 * @param link	        item to be referenced by @a owner
 * @param uncross_func	notifier to be executed on uncrossing
 *
 * Install a weak cross reference from @a owner to @a link.
 * The two items must have a common ancestor when the cross
 * link is installed. Once their ancestry changes so that
 * they don't have a common ancestor anymore, @a uncross_func()
 * is executed.
 */
void
bse_item_cross_link (BseItem         *owner,
                     BseItem         *link,
                     BseItemUncross   uncross_func)
{
  BseItem *container;

  assert_return (BSE_IS_ITEM (owner));
  assert_return (BSE_IS_ITEM (link));
  assert_return (uncross_func != NULL);

  container = bse_item_common_ancestor (owner, link);

  if (container)
    _bse_container_cross_link (BSE_CONTAINER (container), owner, link, uncross_func);
  else
    Bse::warning ("%s: %s and %s have no common anchestor", G_STRLOC,
                  bse_object_debug_name (owner),
                  bse_object_debug_name (link));
}

/**
 * @param owner	        reference owner
 * @param link	        item referenced by @a owner
 * @param uncross_func	notifier queued to be executed on uncrossing
 *
 * Removes a cross link previously installed via
 * bse_item_cross_link() without executing @a uncross_func().
 */
void
bse_item_cross_unlink (BseItem        *owner,
                       BseItem        *link,
                       BseItemUncross  uncross_func)
{
  BseItem *container;

  assert_return (BSE_IS_ITEM (owner));
  assert_return (BSE_IS_ITEM (link));
  assert_return (uncross_func != NULL);

  container = bse_item_common_ancestor (owner, link);

  if (container)
    _bse_container_cross_unlink (BSE_CONTAINER (container), owner, link, uncross_func);
  else
    Bse::warning ("%s: `%s' and `%s' have no common anchestor", G_STRLOC,
                  BSE_OBJECT_TYPE_NAME (owner),
                  BSE_OBJECT_TYPE_NAME (link));
}

/**
 * @param owner	reference owner
 * @param link	item referenced by @a owner
 *
 * Destroys all existing cross links from @a owner to
 * @a link by executing the associated notifiers.
 */
void
bse_item_uncross_links (BseItem *owner,
                        BseItem *link)
{
  BseItem *container;

  assert_return (BSE_IS_ITEM (owner));
  assert_return (BSE_IS_ITEM (link));

  container = bse_item_common_ancestor (owner, link);

  if (container)
    _bse_container_uncross (BSE_CONTAINER (container), owner, link);
}

BseSuper*
bse_item_get_super (BseItem *item)
{
  assert_return (BSE_IS_ITEM (item), NULL);

  while (!BSE_IS_SUPER (item) && item)
    item = item->parent;

  return item ? BSE_SUPER (item) : NULL;
}

BseSNet*
bse_item_get_snet (BseItem *item)
{
  assert_return (BSE_IS_ITEM (item), NULL);

  while (!BSE_IS_SNET (item) && item)
    item = item->parent;

  return item ? BSE_SNET (item) : NULL;
}

BseItem*
bse_item_get_toplevel (BseItem *item)
{
  assert_return (BSE_IS_ITEM (item), NULL);

  while (item->parent)
    item = item->parent;

  return item;
}

BseProject*
bse_item_get_project (BseItem *item)
{
  assert_return (BSE_IS_ITEM (item), NULL);

  while (item->parent)
    item = item->parent;

  return BSE_IS_PROJECT (item) ? (BseProject*) item : NULL;
}

gboolean
bse_item_has_ancestor (BseItem *item,
                       BseItem *ancestor)
{
  assert_return (BSE_IS_ITEM (item), FALSE);
  assert_return (BSE_IS_ITEM (ancestor), FALSE);

  while (item->parent)
    {
      item = item->parent;
      if (item == ancestor)
        return TRUE;
    }

  return FALSE;
}

/**
 * @param self  a valid Item
 * @return      the current BseMusicalTuningType, defaulting to BSE_MUSICAL_TUNING_12_TET
 * Find out about the musical tuning that is currently used for this item.
 * The musical tuning depends on project wide settings that may change after
 * this funciton has been called, so the result should be used with caution.
 */
Bse::MusicalTuning
bse_item_current_musical_tuning (BseItem *self)
{
  assert_return (BSE_IS_ITEM (self), Bse::MusicalTuning::OD_12_TET);
  /* finding the musical tuning *should* be possible by just visiting
   * an items parents. however, .bse objects are not currently (0.7.1)
   * structured that way, so we get the tuning from the first song in
   * a project, or simply provide a default.
   */
  BseProject *project = bse_item_get_project (self);
  if (project)
    {
      GSList *slist;
      for (slist = project->supers; slist; slist = slist->next)
        if (BSE_IS_SONG (slist->data))
          return BSE_SONG (slist->data)->musical_tuning;
    }
  return Bse::MusicalTuning::OD_12_TET;
}

static GValue*
pack_value_for_undo (GValue       *value,
                     BseUndoStack *ustack)
{
  GType type = G_VALUE_TYPE (value);
  if (G_TYPE_IS_OBJECT (type))
    {
      char *p = bse_undo_pointer_pack (g_value_get_object (value), ustack);
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
      BseItem *item = (BseItem*) bse_undo_pointer_unpack (g_value_get_string (value), ustack);
      g_value_unset (value);
      g_value_init (value, G_TYPE_OBJECT);
      g_value_set_object (value, item);
    }
  return value;
}

void
bse_item_set_undoable (void          *object,
                       const char    *first_property_name,
                       ...)
{
  va_list var_args;

  assert_return (BSE_IS_ITEM (object));

  va_start (var_args, first_property_name);
  bse_item_set_valist_undoable (object, first_property_name, var_args);
  va_end (var_args);
}

void
bse_item_set_valist_undoable (void       *object,
                              const char *first_property_name,
                              va_list     var_args)
{
  BseItem *self = BSE_ITEM (object);
  const char *name;

  assert_return (BSE_IS_ITEM (self));

  g_object_ref (object);
  g_object_freeze_notify (G_OBJECT (object));

  name = first_property_name;
  while (name)
    {
      GValue value = { 0, };
      GParamSpec *pspec;
      char *error = NULL;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self), name);
      if (!pspec)
        {
          Bse::warning ("item %s has no property named `%s'",
                        bse_object_debug_name (self), name);
          break;
        }
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      G_VALUE_COLLECT (&value, var_args, 0, &error);
      if (error)
        {
          Bse::warning ("while setting property `%s' on %s: %s",
                        name, bse_object_debug_name (self), error);
          g_free (error);
          g_value_unset (&value);
          break;
        }
      bse_item_set_property_undoable (self, pspec->name, &value);
      g_value_unset (&value);
      name = va_arg (var_args, char*);
    }
  g_object_thaw_notify (G_OBJECT (object));
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
  bse_item_set_property_undoable ((BseItem*) bse_undo_pointer_unpack ((const char*) ustep->data[0].v_pointer, ustack),
                                  (const char*) ustep->data[1].v_pointer,
                                  unpack_value_from_undo ((GValue*) ustep->data[2].v_pointer, ustack));
}

static void
unde_free_property (BseUndoStep *ustep)
{
  g_free (ustep->data[0].v_pointer);
  g_free (ustep->data[1].v_pointer);
  g_value_unset ((GValue*) ustep->data[2].v_pointer); /* may or may not be unpacked */
  g_free (ustep->data[2].v_pointer);
}

static inline gboolean
item_property_check_skip_undo (BseItem    *self,
                               const char *name)
{
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self), name);
  return pspec && sfi_pspec_check_option (pspec, "skip-undo");
}

void
bse_item_set_property_undoable (BseItem      *self,
                                const char   *name,
                                const GValue *value)
{
  BseUndoStack *ustack = bse_item_undo_open (self, "set-property(%s,\"%s\")", bse_object_debug_name (self), name);
  BseUndoStep *ustep;
  GValue *tvalue = g_new0 (GValue, 1);
  g_value_init (tvalue, G_VALUE_TYPE (value));
  g_object_get_property (G_OBJECT (self), name, tvalue);
  if (BSE_ITEM_INTERNAL (self) ||
      item_property_check_skip_undo (self, name) ||
      values_equal_for_undo (value, tvalue))
    {
      /* we're about to set a value on an internal item or
       * to set the same value again => skip undo
       */
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
bse_item_undo_open_str (void *item, const std::string &string)
{
  BseItem *self = BSE_ITEM (item);
  BseUndoStack *ustack = BSE_ITEM_GET_CLASS (self)->get_undo (self);
  if (ustack)
    bse_undo_group_open (ustack, string.c_str());
  else
    {
      ustack = bse_undo_stack_dummy ();
      bse_undo_group_open (ustack, Bse::string_format ("DUMMY-GROUP(%s)", string).c_str());
    }
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
  BseItem *item = (BseItem*) bse_undo_pointer_unpack ((const char*) ustep->data[0].v_pointer, ustack);
  BseStorage *storage = BSE_STORAGE (ustep->data[1].v_pointer);
  GTokenType expected_token = G_TOKEN_NONE;

  expected_token = bse_storage_restore_item (storage, item);
  if (expected_token != G_TOKEN_NONE)
    bse_storage_unexp_token (storage, expected_token);

  bse_storage_finish_parsing (storage);
}

static void
unde_free_item (BseUndoStep *ustep)
{
  BseStorage *storage = BSE_STORAGE (ustep->data[1].v_pointer);
  g_free (ustep->data[0].v_pointer);
  bse_storage_reset (storage);
  g_object_unref (storage);
}

void
bse_item_push_undo_storage (BseItem         *self,
                            BseUndoStack    *ustack,
                            BseStorage      *storage)
{
  if (!BSE_ITEM_INTERNAL (self) && !BSE_UNDO_STACK_VOID (ustack))
    {
      BseUndoStep *ustep = bse_undo_step_new (undo_restore_item, unde_free_item, 2);
      bse_storage_turn_readable (storage, "<undo-storage>");
      ustep->data[0].v_pointer = bse_undo_pointer_pack (self, ustack);
      ustep->data[1].v_pointer = g_object_ref (storage);
      bse_undo_stack_push (ustack, ustep);
    }
  else
    bse_storage_reset (storage);
}

void
bse_item_backup_to_undo (BseItem      *self,
                         BseUndoStack *ustack)
{
  if (!BSE_UNDO_STACK_VOID (ustack))
    {
      BseStorage *storage = (BseStorage*) bse_object_new (BSE_TYPE_STORAGE, NULL);
      bse_storage_prepare_write (storage, BseStorageMode (BSE_STORAGE_DBLOCK_CONTAINED |
                                                          BSE_STORAGE_SELF_CONTAINED));
      bse_storage_store_item (storage, self);

      bse_item_push_undo_storage (self, ustack, storage);
      g_object_unref (storage);
    }
}

namespace Bse {

ItemImpl::ItemImpl (BseObject *bobj) :
  LegacyObjectImpl (bobj)
{}

ItemImpl::~ItemImpl ()
{}

ItemIfaceP
ItemImpl::use ()
{
  BseItem *self = as<BseItem*>();
  ItemIfaceP iface = self->as<ItemIfaceP>();
  assert_return (self->parent || self->use_count, iface);
  bse_item_use (self);
  return iface;
}

void
ItemImpl::unuse ()
{
  BseItem *self = as<BseItem*>();
  assert_return (self->use_count >= 1);
  bse_item_unuse (self);
}

void
ItemImpl::set_name (const std::string &name)
{
  BseItem *self = as<BseItem*>();
  if (name != BSE_OBJECT_UNAME (self))
    bse_item_set (self, "uname", name.c_str(), NULL);
}

bool
ItemImpl::editable_property (const std::string &property)
{
  BseItem *self = as<BseItem*>();
  return bse_object_editable_property (self, property.c_str());
}

ContainerImpl*
ItemImpl::parent ()
{
  BseItem *self = as<BseItem*>();
  return self->parent ? self->parent->as<ContainerImpl*>() : NULL;
}

ItemImpl::UndoDescriptorData
ItemImpl::make_undo_descriptor_data (ItemImpl &item)
{
  // sync with bse_undo_pointer_pack
  UndoDescriptorData udd;
  BseItem *bitem = item.as<BseItem*>();
  BseProject *bproject = bse_item_get_project (this->as<BseItem*>());
  if (!bproject) // may happen during destruction
    return udd;  // this UndoDescriptorData is constructed but will never be used
  assert_return (bproject == bse_item_get_project (bitem), udd); // undo descriptors work only for items within same project
  ProjectImpl *project = bproject->as<ProjectImpl*>();
  udd.projectid = ptrdiff_t (project);
  if (&item == project)
    udd.upath = "\002project\003";
  else
    {
      gchar *upath = bse_container_make_upath (bproject, bitem);
      udd.upath = upath;
      g_free (upath);
    }
  return udd;
}

ItemImpl&
ItemImpl::resolve_undo_descriptor_data (const UndoDescriptorData &udd)
{
  // sync with bse_undo_pointer_unpack
  if (udd.projectid == 0)
    fatal_error ("UndoDescriptorData.projectid must be > 0");
  BseProject *bproject = bse_item_get_project (this->as<BseItem*>());
  ProjectImpl *project = bproject ? bproject->as<ProjectImpl*>() : NULL;
  if (udd.projectid != ptrdiff_t (project)) // undo cannot work on orphans
    fatal_error ("item must belong to UndoDescriptorData.projectid");
  if (udd.upath == "\002project\003")
    return *project;
  BseItem *bitem = bse_container_resolve_upath (bproject, udd.upath.c_str());
  if (!bitem)    // undo descriptor for NULL objects is not currently supported
    fatal_error ("item undo path failed to resolve");
  return *bitem->as<ItemImpl*>();
}

static void
undo_lambda_free (BseUndoStep *ustep)
{
  delete (ItemImpl::UndoDescriptor<ItemImpl>*) ustep->data[0].v_pointer;
  delete (ItemImpl::UndoLambda*) ustep->data[1].v_pointer;
  delete (String*) ustep->data[2].v_pointer;
}

static void
undo_lambda_call (BseUndoStep *ustep, BseUndoStack *ustack)
{
  ProjectImpl &project = *ustack->project->as<ProjectImpl*>();
  ItemImpl &self = project.undo_resolve (*(ItemImpl::UndoDescriptor<ItemImpl>*) ustep->data[0].v_pointer);
  auto *lambda = (ItemImpl::UndoLambda*) ustep->data[1].v_pointer;
  // invoke undo function
  const Bse::Error error = (*lambda) (self, ustack);
  if (error != 0) // undo errors shouldn't happen
    {
      String *blurb = (String*) ustep->data[2].v_pointer;
      Bse::warning ("error during undo '%s' of item %s: %s", blurb->c_str(),
                    self.debug_name().c_str(), bse_error_blurb (error));
    }
}

void
ItemImpl::push_item_undo (const String &blurb, const UndoLambda &lambda)
{
  BseItem *self = as<BseItem*>();
  BseUndoStack *ustack = bse_item_undo_open (self, "undo: %s", blurb.c_str());
  if (BSE_UNDO_STACK_VOID (ustack) || BSE_ITEM_INTERNAL (self))
    {
      bse_item_undo_close (ustack);
      return;
    }
  BseUndoStep *ustep = bse_undo_step_new (undo_lambda_call, undo_lambda_free, 3);
  ustep->data[0].v_pointer = new UndoDescriptor<ItemImpl> (undo_descriptor (*this));
  ustep->data[1].v_pointer = new UndoLambda (lambda);
  ustep->data[2].v_pointer = new String (blurb);
  bse_undo_stack_push (ustack, ustep);
  bse_item_undo_close (ustack);
}

void
ItemImpl::push_property_undo (const String &property_name)
{
  assert_return (property_name.empty() == false);
  Any saved_value = get_prop (property_name);
  if (saved_value.empty())
    Bse::warning ("%s: invalid property name: %s", __func__, property_name);
  else
    {
      auto lambda = [property_name, saved_value] (ItemImpl &self, BseUndoStack *ustack) -> Error {
        const bool success = self.set_prop (property_name, saved_value);
        if (!success)
          Bse::warning ("%s: failed to undo property change for '%s': %s", __func__, property_name, saved_value.repr());
        return Error::NONE;
      };
      push_undo (__func__, *this, lambda);
    }
}

ProjectIfaceP
ItemImpl::get_project  ()
{
  BseItem *self = as<BseItem*>();
  BseProject *project = bse_item_get_project (self);
  return project ? project->as<Bse::ProjectIfaceP>() : NULL;
}

ItemIfaceP
ItemImpl::common_ancestor (ItemIface &other)
{
  BseItem *self = as<BseItem*>();
  BseItem *bo = other.as<BseItem*>();
  BseItem *common = bse_item_common_ancestor (self, bo);
  return common ? common->as<ItemIfaceP>() : NULL;
}

bool
ItemImpl::check_is_a (const String &type_name)
{
  BseItem *self = as<BseItem*>();
  const GType type = g_type_from_name (type_name.c_str());
  const bool is_a = g_type_is_a (G_OBJECT_TYPE (self), type);
  return is_a;
}

void
ItemImpl::group_undo (const std::string &name)
{
  BseItem *self = as<BseItem*>();
  BseUndoStack *ustack = bse_item_undo_open (self, "item-group-undo");
  bse_undo_stack_add_merger (ustack, name.c_str());
  bse_item_undo_close (ustack);
}

void
ItemImpl::ungroup_undo ()
{
  BseItem *self = as<BseItem*>();
  BseUndoStack *ustack = bse_item_undo_open (self, "item-ungroup-undo");
  bse_undo_stack_remove_merger (ustack);
  bse_item_undo_close (ustack);
}

class CustomIconKey : public DataKey<Icon*> {
  virtual void destroy (Icon *icon) override    { delete icon; }
};
static CustomIconKey custom_icon_key;

Icon
ItemImpl::icon () const
{
  BseItem *self = const_cast<ItemImpl*> (this)->as<BseItem*>();
  Icon *icon = get_data (&custom_icon_key);
  return icon ? *icon : bse_object_get_icon (self);
}

void
ItemImpl::icon (const Icon &icon)
{
  Icon *custom_icon = new Icon (icon);
  icon_sanitize (custom_icon);
  if (custom_icon->width != 0)
    set_data (&custom_icon_key, custom_icon);
  else
    {
      delete custom_icon;
      delete_data (&custom_icon_key);
    }
}

ItemIfaceP
ItemImpl::get_parent ()
{
  return parent() ? parent()->as<ContainerIfaceP>() : NULL;
}

int
ItemImpl::get_seqid ()
{
  BseItem *self = as<BseItem*>();
  return bse_item_get_seqid (self);
}

String
ItemImpl::get_type ()
{
  BseItem *self = as<BseItem*>();
  return g_type_name (G_OBJECT_TYPE (self));
}

String
ItemImpl::get_type_authors ()
{
  BseItem *self = as<BseItem*>();
  const char *r = bse_type_get_authors (G_OBJECT_TYPE (self));
  return r ? r : "";
}

String
ItemImpl::get_type_blurb ()
{
  BseItem *self = as<BseItem*>();
  const char *r = bse_type_get_blurb (G_OBJECT_TYPE (self));
  return r ? r : "";
}

String
ItemImpl::get_type_license ()
{
  BseItem *self = as<BseItem*>();
  const char *r = bse_type_get_license (G_OBJECT_TYPE (self));
  return r ? r : "";
}

String
ItemImpl::get_type_name ()
{
  BseItem *self = as<BseItem*>();
  return g_type_name (G_OBJECT_TYPE (self));
}

String
ItemImpl::get_uname_path ()
{
  BseItem *self = as<BseItem*>();
  BseProject *project = bse_item_get_project (self);
  gchar *upath = project ? bse_container_make_upath (BSE_CONTAINER (project), self) : NULL;
  const String result = upath ? upath : "";
  g_free (upath);
  return result;
}

String
ItemImpl::get_name ()
{
  BseItem *self = as<BseItem*>();
  const char *r = BSE_OBJECT_UNAME (self);
  return r ? r : "";
}

String
ItemImpl::get_name_or_type ()
{
  BseItem *self = as<BseItem*>();
  const char *name = BSE_OBJECT_UNAME (self);
  const String result = name ? name : BSE_OBJECT_TYPE_NAME (self);
  return result;
}

String
ItemImpl::get_custom (const String &key)
{
  return customkv_[key];
}

void
ItemImpl::set_custom (const String &key, const String &value)
{
  customkv_[key] = value;
}

bool
ItemImpl::internal ()
{
  BseItem *self = as<BseItem*>();
  return BSE_ITEM_INTERNAL (self);
}

PropertyCandidates
ItemImpl::get_property_candidates (const String &property_name)
{
  BseItem *self = as<BseItem*>();
  PropertyCandidates pc;
  if (bse_item_get_candidates (self, property_name, pc))
    return pc;
  return PropertyCandidates();
}

int
ItemImpl::seqid() const
{
  BseItem *self = const_cast<ItemImpl*> (this)->as<BseItem*>();

  return bse_item_get_seqid (self);
}

void
ItemImpl::seqid (int val)
{
  assert_return_unreached(); // readonly property
}

bool
ItemImpl::constrain_idl_enum (int64_t &i, const StringVector &kvlist)
{
  const String &type_name = Aida::Introspection::find_value ("type", kvlist);
  if (!type_name.empty())
    {
      const String &enumerator = Aida::Introspection::enumerator_from_value (type_name, i);
      if (!enumerator.empty())
        return true;
    }
  return false;
}

bool
ItemImpl::constrain_idl_int (int64_t &i, const StringVector &kvlist)
{
  const String &smin = Aida::Introspection::find_value ("min", kvlist);
  if (!smin.empty())
    {
      size_t consumed = 0;
      const int64_t b = string_to_int (smin, &consumed);
      if (consumed && i < b)
        i = b;
    }
  const String &smax = Aida::Introspection::find_value ("max", kvlist);
  if (!smax.empty())
    {
      size_t consumed = 0;
      const int64_t b = string_to_int (smax, &consumed);
      if (consumed && i > b)
        i = b;
    }
  return true;
}

bool
ItemImpl::constrain_idl_double (double &d, const StringVector &kvlist)
{
  const String &smin = Aida::Introspection::find_value ("min", kvlist);
  if (!smin.empty())
    {
      const char *const cmin = smin.c_str(), *end = NULL;
      const double b = string_to_double (cmin, &end);
      if (end > cmin && d < b)
        d = b;
    }
  const String &smax = Aida::Introspection::find_value ("max", kvlist);
  if (!smax.empty())
    {
      const char *const cmax = smax.c_str(), *end = NULL;
      const double b = string_to_double (cmax, &end);
      if (end > cmax && d > b)
        d = b;
    }
  return true;
}

bool
ItemImpl::apply_idl_property_need_undo (const StringVector &kvlist)
{
  return !Aida::aux_vector_check_options (kvlist, "", "hints", "skip-undo");
}

} // Bse
