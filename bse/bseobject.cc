// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseobject.hh"
#include "bseexports.hh"
#include "bsestorage.hh"
#include "bsecategories.hh"
#include "bsesource.hh"		/* debug hack */
#include "bsestartup.hh"
#include "bsemain.hh"
#include "bse/internal.hh"
#include <algorithm>
#include <string.h>

#define LDEBUG(...)     Bse::debug ("leaks", __VA_ARGS__)
#define CHECK_LDEBUG()  Bse::debug_key_enabled ("leaks")

void
BseObject::change_flags (uint16 f, bool ason)
{
  if (ason)
    flags_ |= f;
  else
    flags_ &= ~f;
}

namespace Bse {

static void
object_impl_unref_bse_object (Bse::LegacyObjectImpl *cxxo)
{
  BseObject *const object = cxxo->as_bse_object();
  g_object_unref (object);
}

Aida::SharedFromThisP
LegacyObjectImpl::__shared_from_this__ ()
{
  BseObject *const object = as_bse_object();
  if (!object->cxxobjref_)
    {
      g_object_ref (object);
      object->cxxobjref_ = new Bse::LegacyObjectImplP (this, object_impl_unref_bse_object);
    }
  return *object->cxxobjref_;
}

static void (LegacyObjectImpl::*object_impl_post_init) () = NULL;

LegacyObjectImpl::LegacyObjectImpl (BseObject *bobj) :
  gobject_ (bobj)
{
  assert_return (gobject_);
  assert_return (gobject_->cxxobject_ == NULL);
  gobject_->cxxobject_ = this;
  if (BSE_UNLIKELY (!object_impl_post_init))
    object_impl_post_init = &LegacyObjectImpl::post_init;
}

void
LegacyObjectImpl::post_init ()
{
  // this->BasetypeImpl::post_init(); // must chain
}

LegacyObjectImpl::~LegacyObjectImpl ()
{
  assert_return (gobject_->cxxobject_ == this);
  gobject_->cxxobject_ = NULL;
  // LegacyObjectImplP keeps BseObject alive until it is destroyed
  // BseObject keeps LegacyObjectImpl alive until finalize()
}

std::string
LegacyObjectImpl::debug_name ()
{
  return bse_object_debug_name (this->as<BseObject*>());
}

int32_t
LegacyObjectImpl::unique_id ()
{
  BseObject *bo = *this;
  return bo->unique_id;
}

int64_t
LegacyObjectImpl::proxy_id ()
{
  BseObject *bo = *this;
  return bo->unique_id;
}

std::string
LegacyObjectImpl::uname () const
{
  BseObject *object = *const_cast<LegacyObjectImpl*> (this);
  // avoid during finalize: g_object_get (object, "uname", &gstring, NULL);
  const gchar *gstring = BSE_OBJECT_UNAME (object); // works during ref_count==0
  std::string u = gstring ? gstring : "";
  return u;
}

void
LegacyObjectImpl::uname (const std::string &newname)
{
  BseObject *object = *this;
  g_object_set (object, "uname", newname.c_str(), NULL);
}

StringSeq
LegacyObjectImpl::find_typedata (const std::string &type_name)
{
  const Aida::StringVector &sv = Aida::Introspection::find_type (type_name);
  StringSeq kvinfo;
  kvinfo.reserve (sv.size());
  std::copy (sv.begin(), sv.end(), std::back_inserter (kvinfo));
  return kvinfo;
}

void
objects_debug_leaks ()
{
  if (CHECK_LDEBUG())
    {
      GList *list, *objects = bse_objects_list (BSE_TYPE_OBJECT);
      for (list = objects; list; list = list->next)
	{
	  BseObject *object = (BseObject*) list->data;
	  LDEBUG ("stale %s:\t prepared=%u locked=%u ref_count=%u id=%u ((BseObject*)%p)",
                  G_OBJECT_TYPE_NAME (object),
                  BSE_IS_SOURCE (object) && BSE_SOURCE_PREPARED (object),
                  object->lock_count > 0,
                  G_OBJECT (object)->ref_count,
                  BSE_OBJECT_ID (object),
                  object);
	}
      g_list_free (objects);
    }
}

} // Bse

enum
{
  PROP_0,
  PROP_UNAME,
  PROP_BLURB
};

/* --- variables --- */
GQuark             bse_quark_uname = 0;
static GQuark	   bse_quark_icon = 0;
static gpointer	   parent_class = NULL;
static GQuark	   quark_blurb = 0;
static GHashTable *object_unames_ht = NULL;
static SfiUStore  *object_id_ustore = NULL;
static GQuark	   quark_property_changed_queue = 0;


/* --- functions --- */
/**
 * @param object supposedly valid #GObject pointer
 * @return       newly allocated string
 *
 * Construct a debugging identifier for @a object. No mutable
 * object members are accessed, so as long as the caller
 * keeps @a object alive for the duration of the function call,
 * this function is MT-safe and may be called from any thread.
 */
gchar*
bse_object_strdup_debug_handle (gpointer object)
{
  GTypeInstance *instance = (GTypeInstance*) object;
  if (!instance)
    return g_strdup ("<NULL>");
  if (!instance->g_class)
    return g_strdup ("<NULL-Class>");
  if (!g_type_is_a (instance->g_class->g_type, G_TYPE_OBJECT))
    return g_strdup ("<Non-GObject>");
  /* we may not access GObject.data (includes BSE_OBJECT_UNAME()) */
  return g_strdup_format ("%s(%p)\"", G_OBJECT_TYPE_NAME (instance), object);
}

const gchar*
bse_object_debug_name (gpointer object)
{
  GTypeInstance *instance = (GTypeInstance*) object;
  gchar *debug_name;

  if (!instance)
    return "<NULL>";
  if (!instance->g_class)
    return "<NULL-Class>";
  if (!g_type_is_a (instance->g_class->g_type, BSE_TYPE_OBJECT))
    return "<Non-BseObject>";
  debug_name = (char*) g_object_get_data (G_OBJECT (instance), "bse-debug-name");
  if (!debug_name)
    {
      const gchar *uname = BSE_OBJECT_UNAME (instance);
      debug_name = g_strdup_format ("\"%s::%s\"", G_OBJECT_TYPE_NAME (instance), uname ? uname : "");
      g_object_set_data_full (G_OBJECT (instance), "bse-debug-name", debug_name, g_free);
    }
  return debug_name;
}

static inline void
object_unames_ht_insert (BseObject *object)
{
  GSList *object_slist = (GSList*) g_hash_table_lookup (object_unames_ht, BSE_OBJECT_UNAME (object));
  if (object_slist)
    g_hash_table_remove (object_unames_ht, BSE_OBJECT_UNAME (object_slist->data));
  object_slist = g_slist_prepend (object_slist, object);
  g_hash_table_insert (object_unames_ht, BSE_OBJECT_UNAME (object_slist->data), object_slist);
}

static __thread uint in_bse_object_new = 0;

static void
bse_object_init (BseObject *object)
{
  assert_return (in_bse_object_new);
  assert_return (object->cxxobjref_ == NULL);
  assert_return (object->cxxobject_ == NULL);
  object->unset_flag (BseObjectFlags (~0)); // flags_ = 0;
  object->lock_count = 0;
  object->unique_id = bse_id_alloc ();
  sfi_ustore_insert (object_id_ustore, object->unique_id, object);

  object_unames_ht_insert (object);
}

static inline void
object_unames_ht_remove (BseObject *object)
{
  GSList *orig_slist, *object_slist = (GSList*) g_hash_table_lookup (object_unames_ht, BSE_OBJECT_UNAME (object));
  orig_slist = object_slist;
  object_slist = g_slist_remove (object_slist, object);
  if (object_slist != orig_slist)
    {
      g_hash_table_remove (object_unames_ht, BSE_OBJECT_UNAME (object));
      if (object_slist)
	g_hash_table_insert (object_unames_ht, BSE_OBJECT_UNAME (object_slist->data), object_slist);
    }
}

static void
bse_object_do_dispose (GObject *gobject)
{
  BseObject *object = BSE_OBJECT (gobject);
  assert_return (BSE_OBJECT_IN_RESTORE (object) == false);

  object->set_flag (BSE_OBJECT_FLAG_DISPOSING);

  {
    Bse::LegacyObjectImpl *self = object->as<Bse::LegacyObjectImpl*>();
    if (self)
      self->emit_event ("dispose");
  }

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (gobject);

  object->unset_flag (BSE_OBJECT_FLAG_DISPOSING);

  if (object->cxxobjref_)
    {
      Bse::LegacyObjectImplP *cxxobjref = object->cxxobjref_;
      object->cxxobjref_ = NULL;
      cxxobjref->reset();
      delete cxxobjref;
    }
}

static void
bse_object_do_finalize (GObject *gobject)
{
  BseObject *object = BSE_OBJECT (gobject);

  if (object->cxxobject_)
    {
      delete object->cxxobject_;
      assert_return (object->cxxobject_ == NULL);
    }

  const uint unique_id = object->unique_id;
  object->unique_id = 0;
  sfi_ustore_remove (object_id_ustore, unique_id);
  bse_id_free (unique_id);

  /* remove object from hash list *before* clearing data list,
   * since the object uname is kept in the datalist!
   */
  object_unames_ht_remove (object);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
bse_object_do_set_uname (BseObject   *object,
			 const gchar *uname)
{
  Bse::LegacyObjectImpl *self = object->as<Bse::LegacyObjectImpl*>();
  g_object_set_qdata_full ((GObject*) object, bse_quark_uname, g_strdup (uname), uname ? g_free : NULL);
  if (self)
    self->notify ("uname");
}

static void
bse_object_do_set_property (GObject      *gobject,
			    guint         property_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  BseObject *object = BSE_OBJECT (gobject);
  switch (property_id)
    {
      gchar *string;

    case PROP_UNAME:
      if (!(object->get_flags() & BSE_OBJECT_FLAG_FIXED_UNAME))
	{
	  object_unames_ht_remove (object);
	  string = g_strdup_stripped (g_value_get_string (value));
	  if (string)
	    {
	      gchar *p = strchr (string, ':');
	      /* get rid of colons in the string (invalid reserved character) */
	      while (p)
		{
		  *p++ = '?';
		  p = strchr (p, ':');
		}
              /* initial character must be >= \007 */
              if (string[0] > 0 && string[0] < 7)
                string[0] = '_';
	    }
	  BSE_OBJECT_GET_CLASS (object)->set_uname (object, string);
	  g_free (string);
	  g_object_set_data (G_OBJECT (object), "bse-debug-name", NULL);
	  object_unames_ht_insert (object);
	}
      break;
    case PROP_BLURB:
      string = g_value_dup_string (value);
      if (string && !string[0])
        {
          g_free (string);
          string = NULL;
        }
      g_object_set_qdata_full ((GObject*) object, quark_blurb, string, string ? g_free : NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bse_object_do_get_property (GObject     *gobject,
			    guint        property_id,
			    GValue      *value,
			    GParamSpec  *pspec)
{
  BseObject *object = BSE_OBJECT (gobject);
  switch (property_id)
    {
      char *string;
    case PROP_UNAME:
      g_value_set_string (value, BSE_OBJECT_UNAME (object));
      break;
    case PROP_BLURB:
      string = (char*) g_object_get_qdata ((GObject*) object, quark_blurb);
      g_value_set_string (value, string ? string : "");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
bse_object_class_add_grouped_property (BseObjectClass *klass,
                                       guint	       property_id,
                                       GParamSpec     *pspec)
{
  assert_return (BSE_IS_OBJECT_CLASS (klass));
  assert_return (G_IS_PARAM_SPEC (pspec));
  assert_return (property_id > 0);

  g_object_class_install_property (G_OBJECT_CLASS (klass), property_id, pspec);
}

void
bse_object_class_add_property (BseObjectClass *klass,
			       const gchar    *property_group,
			       guint	       property_id,
			       GParamSpec     *pspec)
{
  assert_return (BSE_IS_OBJECT_CLASS (klass));
  assert_return (G_IS_PARAM_SPEC (pspec));
  assert_return (sfi_pspec_get_group (pspec) == NULL);

  sfi_pspec_set_group (pspec, property_group);
  bse_object_class_add_grouped_property (klass, property_id, pspec);
}

void
bse_object_lock (gpointer _object)
{
  BseObject *object = (BseObject*) _object;
  GObject *gobject = (GObject*) _object;

  assert_return (BSE_IS_OBJECT (object));
  assert_return (gobject->ref_count > 0);

  assert_return (object->lock_count < 65535);	// if this breaks, we need to fix the guint16

  if (!object->lock_count)
    {
      g_object_ref (object);
      // locking any object also freezes configuration
      Bse::global_prefs->lock();
    }

  object->lock_count += 1;
}

void
bse_object_unlock (gpointer _object)
{
  BseObject *object = (BseObject*) _object;

  assert_return (BSE_IS_OBJECT (object));
  assert_return (object->lock_count > 0);

  object->lock_count -= 1;

  if (!object->lock_count)
    {
      // tentatively also unfreeze global configuration
      Bse::global_prefs->unlock();

      if (BSE_OBJECT_GET_CLASS (object)->unlocked)
	BSE_OBJECT_GET_CLASS (object)->unlocked (object);

      g_object_unref (object);
    }
}

BseObject*
bse_object_from_id (guint unique_id)
{
  return (BseObject*) sfi_ustore_lookup (object_id_ustore, unique_id);
}

GList*
bse_objects_list_by_uname (GType	type,
			   const gchar *uname)
{
  GList *object_list = NULL;

  assert_return (BSE_TYPE_IS_OBJECT (type) == TRUE, NULL);

  if (object_unames_ht)
    {
      GSList *slist, *object_slist = (GSList*) g_hash_table_lookup (object_unames_ht, uname);

      for (slist = object_slist; slist; slist = slist->next)
	if (g_type_is_a (BSE_OBJECT_TYPE (slist->data), type))
	  object_list = g_list_prepend (object_list, slist->data);
    }

  return object_list;
}

static void
list_objects (gpointer key,
	      gpointer value,
	      gpointer user_data)
{
  gpointer *data = (void**) user_data;

  for (GSList *slist = (GSList*) value; slist; slist = slist->next)
    if (g_type_is_a (BSE_OBJECT_TYPE (slist->data), GType (data[1])))
      data[0] = g_list_prepend ((GList*) data[0], slist->data);
}

GList* /* list_free result */
bse_objects_list (GType	  type)
{
  assert_return (BSE_TYPE_IS_OBJECT (type) == TRUE, NULL);
  if (object_unames_ht)
    {
      gpointer data[2] = { NULL, (gpointer) type, };
      g_hash_table_foreach (object_unames_ht, list_objects, data);
      return (GList*) data[0];
    }
  return NULL;
}

static gboolean
object_check_pspec_editable (BseObject      *object,
                             GParamSpec     *pspec)
{
  if (sfi_pspec_check_option (pspec, "ro"))     /* RDONLY option (GUI) */
    return FALSE;
  BseObjectClass *klass = (BseObjectClass*) g_type_class_peek (pspec->owner_type);
  if (klass && klass->editable_property)
    return klass->editable_property (object, pspec->param_id, pspec) != FALSE;
  else
    return TRUE;
}

gboolean
bse_object_editable_property (gpointer        object,
                              const gchar    *property)
{
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), property);
  if (!pspec || !(pspec->flags & G_PARAM_WRITABLE))
    return FALSE;
  return BSE_OBJECT_GET_CLASS (object)->check_pspec_editable ((BseObject*) object, pspec);
}

void
bse_object_notify_icon_changed (BseObject *object)
{
  assert_return (BSE_IS_OBJECT (object));

  Bse::LegacyObjectImpl *self = object->as<Bse::LegacyObjectImpl*>();
  if (self)
    self->notify ("icon");
}

Bse::Icon
bse_object_get_icon (BseObject *object)
{
  assert_return (BSE_IS_OBJECT (object), Bse::Icon());
  g_object_ref (object);
  Bse::Icon icon = BSE_OBJECT_GET_CLASS (object)->get_icon (object);
  g_object_unref (object);
  return icon;
}

static Bse::Icon
bse_object_do_get_icon (BseObject *object)
{
  assert_return (BSE_IS_OBJECT (object), Bse::Icon());
  Bse::Icon *icon = (Bse::Icon*) g_object_get_qdata (G_OBJECT (object), bse_quark_icon);
  if (!icon)
    {
      /* FIXME: this is a bit of a hack, we could store the first per-type
       * category icon as static type-data and fetch that from here
       */
      Bse::CategorySeq cseq = bse_categories_from_type (G_OBJECT_TYPE (object));
      for (uint i = 0; i < cseq.size(); i++)
	if (cseq[i].icon.pixels.size())
	  {
            icon = new Bse::Icon (cseq[i].icon);
	    g_object_set_qdata_full (G_OBJECT (object), bse_quark_icon, icon, [] (void *d) { delete (Bse::Icon*) d; });
	    break;
	  }
    }
  return icon ? *icon : Bse::Icon();
}

static void
bse_object_store_private (BseObject	*object,
			  BseStorage *storage)
{
}

void
bse_object_restore_start (BseObject  *object,
                          BseStorage *storage)
{
  assert_return (BSE_IS_STORAGE (storage));
  if (!BSE_OBJECT_IN_RESTORE (object))
    {
      object->set_flag (BSE_OBJECT_FLAG_IN_RESTORE);
      bse_storage_add_restorable (storage, object);
      BSE_OBJECT_GET_CLASS (object)->restore_start (object, storage);
    }
}

static void
object_restore_start (BseObject      *object,
                      BseStorage     *storage)
{
}

static GTokenType
object_restore_private (BseObject      *object,
                        BseStorage     *storage,
                        GScanner       *scanner)
{
  return SFI_TOKEN_UNMATCHED;
}

static void
object_restore_finish (BseObject      *object,
                       guint           vmajor,
                       guint           vminor,
                       guint           vmicro)
{
}

void
bse_object_restore_finish (BseObject *object,
                           guint      vmajor,
                           guint      vminor,
                           guint      vmicro)
{
  if (BSE_OBJECT_IN_RESTORE (object))
    {
      BSE_OBJECT_GET_CLASS (object)->restore_finish (object, vmajor, vminor, vmicro);
      object->unset_flag (BSE_OBJECT_FLAG_IN_RESTORE);
    }
}

static void
bse_object_class_base_init (BseObjectClass *klass)
{
  klass->editable_property = NULL;
}

static void
bse_object_class_base_finalize (BseObjectClass *klass)
{
}

static void
bse_object_class_init (BseObjectClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  bse_quark_uname = g_quark_from_static_string ("bse-object-uname");
  bse_quark_icon = g_quark_from_static_string ("bse-object-icon");
  quark_property_changed_queue = g_quark_from_static_string ("bse-property-changed-queue");
  quark_blurb = g_quark_from_static_string ("bse-object-blurb");
  object_unames_ht = g_hash_table_new (bse_string_hash, bse_string_equals);
  object_id_ustore = sfi_ustore_new ();

  gobject_class->set_property = bse_object_do_set_property;
  gobject_class->get_property = bse_object_do_get_property;
  gobject_class->dispose = bse_object_do_dispose;
  gobject_class->finalize = bse_object_do_finalize;

  klass->check_pspec_editable = object_check_pspec_editable;
  klass->set_uname = bse_object_do_set_uname;
  klass->store_private = bse_object_store_private;
  klass->restore_start = object_restore_start;
  klass->restore_private = object_restore_private;
  klass->restore_finish = object_restore_finish;
  klass->unlocked = NULL;
  klass->get_icon = bse_object_do_get_icon;

  bse_object_class_add_param (klass, NULL,
			      PROP_UNAME,
			      sfi_pspec_string ("uname", _("Name"), _("Unique name of this object"),
						NULL,
						SFI_PARAM_GUI ":lax-validation"
						/* watch out, unames are specially
						 * treated within the various
						 * objects, specifically BseItem
						 * and BseContainer.
						 */));
  bse_object_class_add_param (klass, NULL,
			      PROP_BLURB,
			      sfi_pspec_string ("blurb", _("Comment"), _("Free form comment or description"),
						"",
						SFI_PARAM_STANDARD ":skip-default"));
}

BSE_BUILTIN_TYPE (BseObject)
{
  static const GTypeInfo object_info = {
    sizeof (BseObjectClass),

    (GBaseInitFunc) bse_object_class_base_init,
    (GBaseFinalizeFunc) bse_object_class_base_finalize,
    (GClassInitFunc) bse_object_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseObject),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_object_init,
  };

  return bse_type_register_abstract (G_TYPE_OBJECT,
                                     "BseObject",
                                     "BSE Object Hierarchy base type",
                                     __FILE__, __LINE__,
                                     &object_info);
}

// == BseObject -> C++ class Glue ==
GObject*
bse_object_new (GType object_type, const gchar *first_property_name, ...)
{
  assert_return (G_TYPE_IS_OBJECT (object_type), NULL);
  va_list var_args;
  va_start (var_args, first_property_name);
  GObject *object = bse_object_new_valist (object_type, first_property_name, var_args);
  va_end (var_args);
  return object;
}

#include "bseserver.hh"
#include "bseproject.hh"
#include "bsepcmwriter.hh"
#include "bseeditablesample.hh"
#include "bsesong.hh"
#include "bsewaveosc.hh"
#include "bsecsynth.hh"
#include "bsetrack.hh"
#include "bsecontextmerger.hh"
#include "bsewave.hh"
#include "bsemidisynth.hh"
#include "bsewaverepo.hh"
#include "bsesoundfont.hh"
#include "bsesoundfontrepo.hh"
#include "bsebus.hh"
#include "bsesnet.hh"
#include "bsepart.hh"

GObject*
bse_object_new_valist (GType object_type, const gchar *first_property_name, va_list var_args)
{
  if (!g_type_is_a (object_type, BSE_TYPE_OBJECT)) // e.g. BsePlugin
    return g_object_new_valist (object_type, first_property_name, var_args);
  // object_type is derived from BSE_TYPE_OBJECT from here on
  in_bse_object_new++;
  BseObject *object = (BseObject*) g_object_new_valist (object_type, first_property_name, var_args);
  in_bse_object_new--;
  assert_return (object->cxxobject_ == NULL, NULL);
  assert_return (object->cxxobjref_ == NULL, NULL);
  Bse::LegacyObjectImpl *cxxo;
  if      (g_type_is_a (object_type, BSE_TYPE_SERVER))
    cxxo = new Bse::ServerImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_PCM_WRITER))
    cxxo = new Bse::PcmWriterImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_PROJECT))
    cxxo = new Bse::ProjectImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_SONG))
    cxxo = new Bse::SongImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_EDITABLE_SAMPLE))
    cxxo = new Bse::EditableSampleImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_WAVE))
    cxxo = new Bse::WaveImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_WAVE_REPO))
    cxxo = new Bse::WaveRepoImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_SOUND_FONT))
    cxxo = new Bse::SoundFontImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_SOUND_FONT_REPO))
    cxxo = new Bse::SoundFontRepoImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_MIDI_SYNTH))
    cxxo = new Bse::MidiSynthImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_WAVE_OSC))
    cxxo = new Bse::WaveOscImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_CSYNTH))
    cxxo = new Bse::CSynthImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_SNET))
    cxxo = new Bse::SNetImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_SUPER))
    cxxo = new Bse::SuperImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_TRACK))
    cxxo = new Bse::TrackImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_CONTEXT_MERGER))
    cxxo = new Bse::ContextMergerImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_CONTAINER))
    cxxo = new Bse::ContainerImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_PART))
    cxxo = new Bse::PartImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_BUS))
    cxxo = new Bse::BusImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_SUB_SYNTH))
    cxxo = new Bse::SubSynthImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_SOURCE))
    cxxo = new Bse::SourceImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_ITEM))
    cxxo = new Bse::ItemImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_OBJECT))
    cxxo = new Bse::LegacyObjectImpl (object);
  else
    assert_return_unreached (NULL);
  assert_return (object->cxxobject_ == cxxo, NULL);
  assert_return (object->cxxobjref_ == NULL, NULL);
  Aida::ImplicitBaseP cxxobjref = cxxo->shared_from_this();
  assert_return (cxxo == &*cxxobjref, NULL);
  assert_return (cxxo == object->cxxobjref_->get(), NULL);
  assert_return (cxxo == *object, NULL);
  assert_return (object == *cxxo, NULL);
  (cxxo->*Bse::object_impl_post_init) ();
  return object;
}
