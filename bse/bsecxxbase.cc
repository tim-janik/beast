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
#include <new>

namespace Bse {

static void     bse_cxx_base_class_base_init (gpointer g_class);

BSE_CXX_TYPE_REGISTER_INTERN (CxxBase, "BseSource", NULL, bse_cxx_base_class_base_init, TypeRegistry::ABSTRACT);

static gpointer bse_cxx_base_parent_class = NULL;

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
  bse_cxx_base_parent_class = g_type_class_peek_parent (object_class);
  object_class->finalize = bse_cxx_base_instance_finalize;
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

SfiNum
Value::get_num () const
{
  GValue *v = gvalue();
  if (SFI_VALUE_HOLDS_BOOL (v))
    return sfi_value_get_bool (v);
  else if (SFI_VALUE_HOLDS_INT (v))
    return sfi_value_get_int (v);
  else if (G_VALUE_HOLDS_ENUM (v))
    return g_value_get_enum (v);
  else if (SFI_VALUE_HOLDS_REAL (v))
    return (SfiNum) sfi_value_get_real (v);
  else if (SFI_VALUE_HOLDS_NUM (v))
    return sfi_value_get_num (v);
  else
    std::unexpected ();
}

SfiReal
Value::get_real () const
{
  GValue *v = gvalue();
  if (SFI_VALUE_HOLDS_INT (v))
    return sfi_value_get_int (v);
  else if (SFI_VALUE_HOLDS_REAL (v))
    return sfi_value_get_real (v);
  else if (SFI_VALUE_HOLDS_NUM (v))
    return sfi_value_get_num (v);
  else
    std::unexpected ();
}

const SfiString
Value::get_string () const
{
  GValue *v = gvalue();
  if (SFI_VALUE_HOLDS_STRING (v))
    return sfi_value_get_string (v);
  else if (SFI_VALUE_HOLDS_CHOICE (v))
    return sfi_value_get_choice (v);
  else
    std::unexpected ();
}

void
Value::set_num (SfiNum n)
{
  GValue *v = gvalue();
  if (SFI_VALUE_HOLDS_BOOL (v))
    sfi_value_set_bool (v, n);
  else if (SFI_VALUE_HOLDS_INT (v))
    sfi_value_set_int (v, n);
  else if (G_VALUE_HOLDS_ENUM (v))
    g_value_set_enum (v, n);
  else if (SFI_VALUE_HOLDS_REAL (v))
    sfi_value_set_real (v, n);
  else if (SFI_VALUE_HOLDS_NUM (v))
    sfi_value_set_num (v, n);
  else
    std::unexpected ();
}

void
Value::set_real (SfiReal r)
{
  GValue *v = gvalue();
  if (SFI_VALUE_HOLDS_INT (v))
    sfi_value_set_int (v, (SfiInt) r);
  else if (SFI_VALUE_HOLDS_REAL (v))
    sfi_value_set_real (v, r);
  else if (SFI_VALUE_HOLDS_NUM (v))
    sfi_value_set_num (v, (SfiNum) r);
  else
    std::unexpected ();
}

void
Value::set_string (const char      *s)
{
  GValue *v = gvalue();
  if (SFI_VALUE_HOLDS_STRING (v))
    sfi_value_set_string (v, s);
  else if (SFI_VALUE_HOLDS_CHOICE (v))
    sfi_value_set_choice (v, s);
  else
    std::unexpected ();
}

} // Bse
