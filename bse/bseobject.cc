// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseobject.hh"
#include "bseexports.hh"
#include "bsestorage.hh"
#include "bsecategories.hh"
#include "bsegconfig.hh"
#include "bsesource.hh"		/* debug hack */
#include "bsestartup.hh"
#include "bse/internal.hh"
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

static void (ObjectImpl::*object_impl_post_init) () = NULL;

ObjectImpl::ObjectImpl (BseObject *bobj) :
  gobject_ (bobj)
{
  assert_return (gobject_);
  assert_return (gobject_->cxxobject_ == NULL);
  g_object_ref (gobject_);
  gobject_->cxxobject_ = this;
  if (BSE_UNLIKELY (!object_impl_post_init))
    object_impl_post_init = &ObjectImpl::post_init;
}

void
ObjectImpl::post_init ()
{
  // this->BasetypeImpl::post_init(); // must chain
}

ObjectImpl::~ObjectImpl ()
{
  assert_return (gobject_->cxxobject_ == this);
  gobject_->cxxobject_ = NULL;
  g_object_unref (gobject_);
  // ObjectImpl keeps BseObject alive until it is destroyed
  // BseObject keeps ObjectImpl alive until dispose()
}

Aida::ExecutionContext&
ObjectImpl::__execution_context_mt__ () const
{
  return execution_context();
}

Aida::IfaceEventConnection
ObjectImpl::__attach__ (const String &eventselector, EventHandlerF handler)
{
  return event_dispatcher_.attach (eventselector, handler);
}

std::string
ObjectImpl::debug_name ()
{
  return bse_object_debug_name (this->as<BseObject*>());
}

int32_t
ObjectImpl::unique_id ()
{
  BseObject *bo = *this;
  return bo->unique_id;
}

int64_t
ObjectImpl::proxy_id ()
{
  BseObject *bo = *this;
  return bo->unique_id;
}

void
ObjectImpl::emit_event (const std::string &type, const KV &a1, const KV &a2, const KV &a3,
                        const KV &a4, const KV &a5, const KV &a6, const KV &a7)
{
  const char ident_chars[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char *const ctype = type.c_str(), *const colon = strchr (ctype, ':');
  const String name = colon ? type.substr (0, colon - ctype) : type;
  const String detail = colon ? type.substr (colon - ctype + 1) : "";
  for (size_t i = 0; name[i]; i++)
    if (!strchr (ident_chars, name[i]))
      {
        Bse::warning ("invalid characters in Event type: %s", type);
        break;
      }
  for (size_t i = 0; detail[i]; i++)
    if (!strchr (ident_chars, detail[i]) and detail[i] != '_')
      {
        Bse::warning ("invalid characters in Event type: %s", type);
        break;
      }
  Aida::Event ev (type);
  const KV *args[] = { &a1, &a2, &a3, &a4, &a5, &a6, &a7 };
  for (size_t i = 0; i < sizeof (args) / sizeof (args[0]); i++)
    if (!args[i]->key.empty())
      ev[args[i]->key] = args[i]->value;
  ev["name"] = name;
  ev["detail"] = detail;
  event_dispatcher_.emit (ev);  // emits "notify:detail" as type="notify:detail" name="notify" detail="detail"
  // using namespace Aida::KeyValueArgs; emit_event ("notification", "value"_v = 5);
}

void
ObjectImpl::notify (const String &detail)
{
  assert_return (detail.empty() == false);
  emit_event ("notify:" + detail);
}

std::string
ObjectImpl::uname () const
{
  BseObject *object = *const_cast<ObjectImpl*> (this);
  gchar *gstring = NULL;
  g_object_get (object, "uname", &gstring, NULL);
  std::string u = gstring ? gstring : "";
  g_free (gstring);
  return u;
}

void
ObjectImpl::uname (const std::string &newname)
{
  BseObject *object = *this;
  g_object_set (object, "uname", newname.c_str(), NULL);
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
enum
{
  SIGNAL_RELEASE,
  SIGNAL_ICON_CHANGED,
  SIGNAL_LAST
};


/* --- prototypes --- */
static guint		eclosure_hash			(gconstpointer	 c);
static gint		eclosure_equals			(gconstpointer	 c1,
							 gconstpointer	 c2);


/* --- variables --- */
GQuark             bse_quark_uname = 0;
static GQuark	   bse_quark_icon = 0;
static gpointer	   parent_class = NULL;
static GQuark	   quark_blurb = 0;
static GHashTable *object_unames_ht = NULL;
static GHashTable *eclosures_ht = NULL;
static SfiUStore  *object_id_ustore = NULL;
static GQuark	   quark_property_changed_queue = 0;
static guint       object_signals[SIGNAL_LAST] = { 0, };


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
  object->cxxobject_ = NULL;
  object->cxxobjref_ = NULL;
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

  /* perform release notification */
  g_signal_emit (object, object_signals[SIGNAL_RELEASE], 0);

  {
    Bse::ObjectImpl *self = object->as<Bse::ObjectImpl*>();
    if (self)
      self->emit_event ("dispose");
  }

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (gobject);

  object->unset_flag (BSE_OBJECT_FLAG_DISPOSING);

  if (object->cxxobjref_)
    {
      object->cxxobjref_->reset();
      Bse::ObjectImplP *cxxobjref = object->cxxobjref_;
      object->cxxobjref_ = NULL;
      delete cxxobjref;
    }
}

static void
bse_object_do_finalize (GObject *gobject)
{
  BseObject *object = BSE_OBJECT (gobject);

  bse_id_free (object->unique_id);
  sfi_ustore_remove (object_id_ustore, object->unique_id);
  object->unique_id = 0;

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
  Bse::ObjectImpl *self = object->as<Bse::ObjectImpl*>();
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
bse_object_marshal_signal (GClosure       *closure,
                           GValue /*out*/ *return_value,
                           guint           n_param_values,
                           const GValue   *param_values,
                           gpointer        invocation_hint,
                           gpointer        marshal_data)
{
  gpointer arg0, argN;

  assert_return (return_value == NULL);
  assert_return (n_param_values >= 1 && n_param_values <= 1 + SFI_VMARSHAL_MAX_ARGS);
  assert_return (G_VALUE_HOLDS_OBJECT (param_values));

  arg0 = g_value_get_object (param_values);
  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      argN = arg0;
      arg0 = closure->data;
    }
  else
    argN = closure->data;
  sfi_vmarshal_void (((GCClosure*) closure)->callback,
		     arg0,
		     n_param_values - 1,
		     param_values + 1,
		     argN);
}

guint
bse_object_class_add_signal (BseObjectClass    *oclass,
			     const gchar       *signal_name,
			     GType              return_type,
			     guint              n_params,
			     ...)
{
  va_list args;
  guint signal_id;

  assert_return (BSE_IS_OBJECT_CLASS (oclass), 0);
  assert_return (n_params <= SFI_VMARSHAL_MAX_ARGS, 0);
  assert_return (signal_name != NULL, 0);

  va_start (args, n_params);
  signal_id = g_signal_new_valist (signal_name,
				   G_TYPE_FROM_CLASS (oclass),
				   GSignalFlags (G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
				   NULL, NULL, NULL,
				   bse_object_marshal_signal,
				   return_type,
				   n_params, args);
  va_end (args);

  return signal_id;
}

guint
bse_object_class_add_asignal (BseObjectClass    *oclass,
			      const gchar       *signal_name,
			      GType              return_type,
			      guint              n_params,
			      ...)
{
  va_list args;
  guint signal_id;

  assert_return (BSE_IS_OBJECT_CLASS (oclass), 0);
  assert_return (n_params <= SFI_VMARSHAL_MAX_ARGS, 0);
  assert_return (signal_name != NULL, 0);

  va_start (args, n_params);
  signal_id = g_signal_new_valist (signal_name,
				   G_TYPE_FROM_CLASS (oclass),
				   GSignalFlags (G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS | G_SIGNAL_ACTION),
				   NULL, NULL, NULL,
				   bse_object_marshal_signal,
				   return_type,
				   n_params, args);
  va_end (args);

  return signal_id;
}

guint
bse_object_class_add_dsignal (BseObjectClass    *oclass,
			      const gchar       *signal_name,
			      GType              return_type,
			      guint              n_params,
			      ...)
{
  va_list args;
  guint signal_id;

  assert_return (BSE_IS_OBJECT_CLASS (oclass), 0);
  assert_return (n_params <= SFI_VMARSHAL_MAX_ARGS, 0);
  assert_return (signal_name != NULL, 0);

  va_start (args, n_params);
  signal_id = g_signal_new_valist (signal_name,
				   G_TYPE_FROM_CLASS (oclass),
				   GSignalFlags (G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS | G_SIGNAL_DETAILED),
				   NULL, NULL, NULL,
				   bse_object_marshal_signal,
				   return_type,
				   n_params, args);
  va_end (args);

  return signal_id;
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

      /* we also keep the globals locked so we don't need to duplicate
       * this all over the place
       */
      bse_gconfig_lock ();
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
      /* release global lock */
      bse_gconfig_unlock ();

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

  g_signal_emit (object, object_signals[SIGNAL_ICON_CHANGED], 0);
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

typedef struct {
  GClosure closure;
  guint    dest_signal;
  GQuark   dest_detail;
  guint    erefs;
  gpointer src_object;
  gulong   handler;
  guint    src_signal;
  guint    src_detail;
} EClosure;

static guint
eclosure_hash (gconstpointer c)
{
  const EClosure *e = (const EClosure*) c;
  guint h = G_HASH_LONG ((long) e->src_object) >> 2;
  h += G_HASH_LONG ((long) e->closure.data) >> 1;
  h += e->src_detail;
  h += e->dest_detail << 1;
  h += e->src_signal << 12;
  h += e->dest_signal << 13;
  return h;
}

static gint
eclosure_equals (gconstpointer c1,
		 gconstpointer c2)
{
  const EClosure *e1 = (const EClosure*) c1, *e2 = (const EClosure*) c2;
  return (e1->src_object   == e2->src_object &&
	  e1->closure.data == e2->closure.data &&
	  e1->src_detail   == e2->src_detail &&
	  e1->dest_detail  == e2->dest_detail &&
	  e1->src_signal   == e2->src_signal &&
	  e1->dest_signal  == e2->dest_signal);
}

static void
eclosure_marshal (GClosure       *closure,
		  GValue /*out*/ *return_value,
		  guint           n_param_values,
		  const GValue   *param_values,
		  gpointer        invocation_hint,
		  gpointer        marshal_data)
{
  EClosure *e = (EClosure*) closure;
  if (e->dest_signal)
    g_signal_emit (e->closure.data, e->dest_signal, e->dest_detail);
  else
    g_object_notify ((GObject*) e->closure.data, g_quark_to_string (e->dest_detail));
}

void
bse_object_reemit_signal (gpointer     src_object,
			  const gchar *src_signal,
			  gpointer     dest_object,
			  const gchar *dest_signal)
{
  EClosure key;
  if (g_signal_parse_name (src_signal, G_OBJECT_TYPE (src_object), &key.src_signal, &key.src_detail, TRUE) &&
      g_signal_parse_name (dest_signal, G_OBJECT_TYPE (dest_object), &key.dest_signal, &key.dest_detail, TRUE))
    {
      EClosure *e;
      key.closure.data = dest_object;
      key.src_object = src_object;
      e = (EClosure*) g_hash_table_lookup (eclosures_ht, &key);
      if (!e)
	{
	  GSignalQuery query;
          gboolean property_notify = key.dest_detail && strncmp (dest_signal, "notify", 6) == 0;
	  g_signal_query (key.dest_signal, &query);
	  if (!(query.return_type == G_TYPE_NONE &&
		((query.n_params == 0 &&
                  query.signal_flags & G_SIGNAL_ACTION) ||
		 (property_notify &&
                  g_object_class_find_property (G_OBJECT_GET_CLASS (dest_object),
                                                g_quark_to_string (key.dest_detail))))))
	    {
	      Bse::warning ("%s: invalid signal for reemission: \"%s\"", G_STRLOC, dest_signal);
	      return;
	    }
	  e = (EClosure*) g_closure_new_simple (sizeof (EClosure), dest_object);
	  e->erefs = 1;
	  e->closure.data = dest_object;
	  e->src_object = src_object;
	  e->dest_signal = property_notify ? 0 : key.dest_signal;
	  e->dest_detail = key.dest_detail;
	  e->src_signal = key.src_signal;
	  e->src_detail = key.src_detail;
	  g_closure_set_marshal (&e->closure, eclosure_marshal);
	  g_closure_ref (&e->closure);
	  g_closure_sink (&e->closure);
	  g_signal_connect_closure_by_id (e->src_object,
					  e->src_signal, e->src_detail,
					  &e->closure, G_CONNECT_AFTER);
	  g_hash_table_insert (eclosures_ht, e, e);
	}
      else
	e->erefs++;
    }
  else
    Bse::warning ("%s: invalid signal specs: \"%s\", \"%s\"", G_STRLOC, src_signal, dest_signal);
}

void
bse_object_remove_reemit (gpointer     src_object,
			  const gchar *src_signal,
			  gpointer     dest_object,
			  const gchar *dest_signal)
{
  EClosure key;
  if (g_signal_parse_name (dest_signal, G_OBJECT_TYPE (dest_object), &key.dest_signal, &key.dest_detail, TRUE) &&
      g_signal_parse_name (src_signal, G_OBJECT_TYPE (src_object), &key.src_signal, &key.src_detail, TRUE))
    {
      gboolean property_notify = key.dest_detail && strncmp (dest_signal, "notify", 6) == 0;
      EClosure *e;
      key.closure.data = dest_object;
      key.src_object = src_object;
      key.dest_signal = property_notify ? 0 : key.dest_signal;
      e = (EClosure*) g_hash_table_lookup (eclosures_ht, &key);
      if (e)
	{
	  assert_return (e->erefs > 0);

	  e->erefs--;
	  if (!e->erefs)
	    {
	      g_hash_table_remove (eclosures_ht, e);
	      g_signal_handlers_disconnect_matched (e->src_object,
                                                    G_SIGNAL_MATCH_CLOSURE | G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_DETAIL,
						    e->src_signal, e->src_detail,
						    &e->closure, NULL, NULL);
	      g_closure_invalidate (&e->closure);
	      g_closure_unref (&e->closure);
	    }
	}
      else
	Bse::warning ("%s: no reemission for object %s signal \"%s\" to object %s signal \"%s\"", G_STRLOC,
                      bse_object_debug_name (src_object), src_signal,
                      bse_object_debug_name (dest_object), dest_signal);
    }
  else
    Bse::warning ("%s: invalid signal specs: \"%s\", \"%s\"", G_STRLOC, src_signal, dest_signal);
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
  eclosures_ht = g_hash_table_new (eclosure_hash, eclosure_equals);
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

  object_signals[SIGNAL_RELEASE] = bse_object_class_add_signal (klass, "release", G_TYPE_NONE, 0);
  object_signals[SIGNAL_ICON_CHANGED] = bse_object_class_add_signal (klass, "icon_changed", G_TYPE_NONE, 0);
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
#include "bsemidinotifier.hh"
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
  Bse::ObjectImpl *cxxo;
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
  else if (g_type_is_a (object_type, BSE_TYPE_MIDI_NOTIFIER))
    cxxo = new Bse::MidiNotifierImpl (object);
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
  else if (g_type_is_a (object_type, BSE_TYPE_CONTAINER))
    cxxo = new Bse::ContainerImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_TRACK))
    cxxo = new Bse::TrackImpl (object);
  else if (g_type_is_a (object_type, BSE_TYPE_CONTEXT_MERGER))
    cxxo = new Bse::ContextMergerImpl (object);
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
    cxxo = new Bse::ObjectImpl (object);
  else
    assert_return_unreached (NULL);
  assert_return (object->cxxobject_ == cxxo, NULL);
  assert_return (object->cxxobjref_ == NULL, NULL);
  object->cxxobjref_ = new Bse::ObjectImplP (cxxo); // shared_ptr that allows enable_shared_from_this
  assert_return (cxxo == *object, NULL);
  assert_return (object == *cxxo, NULL);
  (cxxo->*Bse::object_impl_post_init) ();
  return object;
}
