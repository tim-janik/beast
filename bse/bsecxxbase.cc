// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecxxbase.hh"
#include "bse/internal.hh"

namespace Bse {

static void     bse_cxx_base_class_base_init (gpointer g_class);

BSE_CXX_TYPE_REGISTER_INITIALIZED (CxxBase, "BseSource", NULL, bse_cxx_base_class_base_init, TypeRegistry::ABSTRACT);

static gpointer bse_cxx_base_parent_class = NULL;

static void
bse_cxx_base_compat_setup (BseItem         *item,
                           guint            vmajor,
                           guint            vminor,
                           guint            vmicro)
{
  CxxBase *self = cast (item);

  self->compat_setup (vmajor, vminor, vmicro);
}

static void
bse_cxx_base_restore_finish (BseObject *object,
                             guint      vmajor,
                             guint      vminor,
                             guint      vmicro)
{
  CxxBase *self = cast (object);

  // chain parent class' handler
  BSE_OBJECT_CLASS (bse_cxx_base_parent_class)->restore_finish (object, vmajor, vminor, vmicro);

  // notify C++ modules about finished restoration
  self->restore_finished (vmajor, vminor, vmicro);
}

static void
bse_cxx_base_instance_finalize (GObject *object)
{
  CxxBase *self = cast (object);

  self->~CxxBase ();

  // chain parent class' handler
  G_OBJECT_CLASS (bse_cxx_base_parent_class)->finalize (object);
}

static void
bse_cxx_base_class_base_init (gpointer g_class)
{
  // GObjectClass *object_class = G_OBJECT_CLASS (g_class);
}

void
CxxBase::class_init (CxxBaseClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BseObjectClass *bse_object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);

  bse_cxx_base_parent_class = g_type_class_peek_parent (object_class);
  object_class->finalize = bse_cxx_base_instance_finalize;
  bse_object_class->restore_finish = bse_cxx_base_restore_finish;
  item_class->compat_setup = bse_cxx_base_compat_setup;
}

/*Con*/
CxxBase::CxxBase()
{}

void
CxxBase::set_property (guint        prop_id,
                       const Value &value,
                       GParamSpec  *pspec)
{}

void
CxxBase::get_property (guint       prop_id,
                       Value      &value,
                       GParamSpec *pspec)
{}

void
CxxBase::compat_setup (guint          vmajor,
                       guint          vminor,
                       guint          vmicro)
{}

void
CxxBase::restore_finished (guint          vmajor,
                           guint          vminor,
                           guint          vmicro)
{}

GType
CxxBase::type ()
{
  return G_OBJECT_TYPE (gobject());
}

CxxBase*
CxxBase::ref()
{
  g_object_ref (gobject ());
  return this;
}

void
CxxBase::unref()
{
  g_object_unref (gobject ());
}

void
CxxBase::freeze_notify()
{
  g_object_freeze_notify (gobject ());
}

void
CxxBase::notify (const gchar *property)
{
#undef g_object_notify
  g_object_notify (gobject (), property);
}

void
CxxBase::thaw_notify()
{
  g_object_thaw_notify (gobject ());
}

void
CxxBase::set (const gchar   *first_property_name,
              ...)
{
  va_list var_args;
  va_start (var_args, first_property_name);
  g_object_set_valist (gobject (), first_property_name, var_args);
  va_end (var_args);
}

void
CxxBase::get (const gchar   *first_property_name,
              ...)
{
  va_list var_args;
  va_start (var_args, first_property_name);
  g_object_get_valist (gobject (), first_property_name, var_args);
  va_end (var_args);
}

/*Des*/
CxxBase::~CxxBase()
{
}

CxxBase*
CxxBase::cast_from_gobject (void *o)
{
  CxxBase *self = NULL;
  if (G_TYPE_CHECK_INSTANCE_TYPE (o, BSE_TYPE_CXX_BASE))
    self = (CxxBase*) (BSE_CXX_INSTANCE_OFFSET + (char*) o);
  else // make GObject throw an apropriate warning
    G_TYPE_CHECK_INSTANCE_CAST (o, BSE_TYPE_CXX_BASE, void);
  return self;
}

void*
CxxBase::cast_to_gobject ()
{
  return -BSE_CXX_INSTANCE_OFFSET + (char*) this;
}

GObject*
CxxBase::gobject () const
{
  return (GObject*) const_cast<CxxBase*> (this)->cast_to_gobject ();
}

BseItem*
CxxBase::item ()
{
  return (BseItem*) cast_to_gobject ();
}

void
CxxBaseClass::add_param (const char *group,
                         guint       prop_id,
                         GParamSpec *pspec)
{
  assert_return (pspec->owner_type == 0);
  if (pspec->flags & G_PARAM_WRITABLE)
    pspec->flags = (GParamFlags) (pspec->flags | G_PARAM_CONSTRUCT);
  bse_object_class_add_property ((BseObjectClass*) this, group, prop_id, pspec);
}

void
CxxBaseClass::add_param (guint       prop_id,
                         GParamSpec *grouped_pspec)
{
  assert_return (grouped_pspec->owner_type == 0);
  if (grouped_pspec->flags & G_PARAM_WRITABLE)
    grouped_pspec->flags = (GParamFlags) (grouped_pspec->flags | G_PARAM_CONSTRUCT);
  bse_object_class_add_grouped_property ((BseObjectClass*) this, prop_id, grouped_pspec);
}

void
CxxBaseClass::set_accessors (void       (*get_property)      (GObject*,   guint,       GValue*,          GParamSpec*),
                             void       (*set_property)      (GObject*,   guint, const GValue*,          GParamSpec*),
                             gboolean   (*editable_property) (BseObject*, guint,                         GParamSpec*),
                             void       (*get_candidates)    (BseItem*,   guint, Bse::PropertyCandidates&, GParamSpec*),
                             void       (*property_updated)  (BseSource*, guint, guint64, double,        GParamSpec*))
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (this);
  gobject_class->get_property = get_property;
  gobject_class->set_property = set_property;
  BseObjectClass *bobject_class = BSE_OBJECT_CLASS (this);
  bobject_class->editable_property = editable_property;
  BseItemClass *item_class = BSE_ITEM_CLASS (this);
  item_class->get_candidates = get_candidates;
  BseSourceClass *source_class = BSE_SOURCE_CLASS (this);
  source_class->property_updated = property_updated;
}

void
CxxBaseClass::add_ochannel (const char *ident,
                            const char *label,
                            const char *blurb,
                            int         assert_id)
{
  int channel_id = bse_source_class_add_ochannel ((BseSourceClass*) this, ident, label, blurb);
  if (assert_id >= 0)
    assert_return (assert_id == channel_id);
}

void
CxxBaseClass::add_ichannel (const char *ident,
                            const char *label,
                            const char *blurb,
                            int         assert_id)
{
  int channel_id = bse_source_class_add_ichannel ((BseSourceClass*) this, ident, label, blurb);
  if (assert_id >= 0)
    assert_return (assert_id == channel_id);
}

void
CxxBaseClass::add_jchannel (const char *ident,
                            const char *label,
                            const char *blurb,
                            int         assert_id)
{
  int channel_id = bse_source_class_add_jchannel ((BseSourceClass*) this, ident, label, blurb);
  if (assert_id >= 0)
    assert_return (assert_id == channel_id);
}

} // Bse
