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
#include "bsecxxvalue.hh"
#include "bsecxxbase.hh"

namespace {
using namespace Bse;

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
    throw WrongTypeGValue (G_STRLOC);
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
    throw WrongTypeGValue (G_STRLOC);
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
    throw WrongTypeGValue (G_STRLOC);
}

gpointer
Value::get_pointer () const
{
  GValue *v = gvalue();
  if (G_VALUE_HOLDS_POINTER (v))
    return g_value_get_pointer (v);
  else
    throw WrongTypeGValue (G_STRLOC);
}

CxxBase*
Value::get_base () const
{
  GValue *v = gvalue();
  if (G_VALUE_HOLDS_OBJECT (v))
    {
      GObject *object = (GObject*) g_value_get_object (v);
      if (object && G_TYPE_CHECK_INSTANCE_TYPE (object, BSE_TYPE_CXX_BASE))
        return cast (object);
      return NULL;
    }
  else
    throw WrongTypeGValue (G_STRLOC);
}

GObject*
Value::get_object () const
{
  GValue *v = gvalue();
  if (G_VALUE_HOLDS_OBJECT (v))
    return (GObject*) g_value_get_object (v);
  else
    throw WrongTypeGValue (G_STRLOC);
}

GParamSpec*
Value::get_pspec () const
{
  GValue *v = gvalue();
  if (G_VALUE_HOLDS_PARAM (v))
    return g_value_get_param (v);
  else
    throw WrongTypeGValue (G_STRLOC);
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
    throw WrongTypeGValue (G_STRLOC);
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
    throw WrongTypeGValue (G_STRLOC);
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
    throw WrongTypeGValue (G_STRLOC);
}

void
Value::set_pointer (gpointer p)
{
  GValue *v = gvalue();
  if (G_VALUE_HOLDS_POINTER (v))
    g_value_set_pointer (v, p);
  else
    throw WrongTypeGValue (G_STRLOC);
}

void
Value::set_base (CxxBase *b)
{
  GValue *v = gvalue();
  if (G_VALUE_HOLDS_OBJECT (v))
    {
      GObject *o = NULL;
      if (b)
        o = cast (b);
      g_value_set_object (v, o);
    }
  else
    throw WrongTypeGValue (G_STRLOC);
}

void
Value::set_object (GObject *o)
{
  GValue *v = gvalue();
  if (G_VALUE_HOLDS_OBJECT (v))
    g_value_set_object (v, o);
  else
    throw WrongTypeGValue (G_STRLOC);
}

void
Value::set_pspec (GParamSpec *p)
{
  GValue *v = gvalue();
  if (G_VALUE_HOLDS_PARAM (v))
    g_value_set_param (v, p);
  else
    throw WrongTypeGValue (G_STRLOC);
}

} // namespace
