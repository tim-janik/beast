/* BSE - Bedevilled Sound Engine                        -*-mode: c++;-*-
 * Copyright (C) 2003 Tim Janik
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
#include "bsecxxbase.h"

namespace {
using namespace Bse;

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
  // FIXME: GObjectClass *object_class = G_OBJECT_CLASS (g_class);
}

void
CxxBase::class_init (CxxBaseClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  bse_cxx_base_parent_class = g_type_class_peek_parent (object_class);
  object_class->finalize = bse_cxx_base_instance_finalize;
  item_class->compat_setup = bse_cxx_base_compat_setup;
}

/*Con*/
CxxBase::CxxBase()
{
}

void
CxxBase::set_property (guint        prop_id,
                       const Value &value,
                       GParamSpec  *pspec)
{
}

void
CxxBase::get_property (guint       prop_id,
                       Value      &value,
                       GParamSpec *pspec)
{
}

void
CxxBase::compat_setup (guint          vmajor,
                       guint          vminor,
                       guint          vmicro)
{
}

gulong
CxxBase::connect (const gchar   *signal,
                  CxxClosure    *closure,
                  bool           after)
{
  GClosure *gclosure = closure->gclosure();
  g_closure_ref (gclosure);
  g_closure_sink (gclosure);
  String sid = tokenize_signal (signal), cid = closure->signature();
  gulong id = 0;
  if (sid == cid)
    id = g_signal_connect_closure (gobject(), signal, gclosure, after != 0);
  else
    g_warning ("%s: ignoring invalid signal connection (\"%s\" != \"%s\")", G_STRLOC, sid.c_str(), cid.c_str());
  g_closure_unref (gclosure);
  return id;
}

#if 0
gulong
CxxBase::connect (const gchar   *signal,
                  GClosure      *closure,
                  bool           after)
{
  g_closure_ref (closure);
  g_closure_sink (closure);
  gulong id = g_signal_connect_closure (gobject(), signal, closure, after != 0);
  g_closure_unref (closure);
  return id;
}
#endif

const String
CxxBase::tokenize_signal (const gchar *signal)
{
  GSignalQuery query;
  GType t;
  String s;
  g_signal_query (g_signal_lookup (signal, type()), &query);
  if (!query.signal_id)
    return "";
  t = query.return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE;
  if (t && t != G_TYPE_NONE) /* void check */
    s += tokenize_gtype (t);
  s += '|';
  for (guint i = 0; i < query.n_params; i++)
    s += tokenize_gtype (query.param_types[i] & ~G_SIGNAL_TYPE_STATIC_SCOPE);
  return s;
}

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
CxxBase::gobject ()
{
  return (GObject*) cast_to_gobject ();
}

BseItem*
CxxBase::item ()
{
  return (BseItem*) cast_to_gobject ();
}

void
CxxBaseClass::add (const char *group,
                   guint       prop_id,
                   GParamSpec *pspec)
{
  g_return_if_fail (pspec->owner_type == 0);
  pspec->flags = (GParamFlags) (pspec->flags | G_PARAM_CONSTRUCT);
  bse_object_class_add_property ((BseObjectClass*) this, group, prop_id, pspec);
}

void
CxxBaseClass::add (guint       prop_id,
                   GParamSpec *grouped_pspec)
{
  g_return_if_fail (grouped_pspec->owner_type == 0);
  grouped_pspec->flags = (GParamFlags) (grouped_pspec->flags | G_PARAM_CONSTRUCT);
  bse_object_class_add_grouped_property ((BseObjectClass*) this, prop_id, grouped_pspec);
}

void
CxxBaseClass::add_ochannel (const char *name,
                            const char *blurb,
                            int         assert_id)
{
  int channel_id = bse_source_class_add_ochannel ((BseSourceClass*) this,
                                                  name, blurb);
  if (assert_id >= 0)
    g_assert (assert_id == channel_id);
}

void
CxxBaseClass::add_ichannel (const char *name,
                            const char *blurb,
                            int         assert_id)
{
  int channel_id = bse_source_class_add_ichannel ((BseSourceClass*) this,
                                                  name, blurb);
  if (assert_id >= 0)
    g_assert (assert_id == channel_id);
}

void
CxxBaseClass::add_jchannel (const char *name,
                            const char *blurb,
                            int         assert_id)
{
  int channel_id = bse_source_class_add_jchannel ((BseSourceClass*) this,
                                                  name, blurb);
  if (assert_id >= 0)
    g_assert (assert_id == channel_id);
}

} // namespace
