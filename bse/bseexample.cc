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
#include "bseexample.h"

#define DEBUG   sfi_debug_keyfunc ("cxx")

namespace Bse {

const ClassInfo cinfo ("/Modules/C++ Example",
                       "BseExample is an example for creating C++ Bse Objects.");
BSE_CXX_TYPE_REGISTER (Example, "BseCxxBase", &cinfo);

/*Con*/
Example::Example()
{
  DEBUG ("Example Constructor: %p\n", this);
}

/*Des*/
Example::~Example()
{
  DEBUG ("Example Destructor: %p\n", this);
}

#define PROP_NUM1 1
#define PROP_NUM2 2

void
Example::class_init (CxxBaseClass *klass)
{
  DEBUG ("Example class-init: %p\n", klass);
  klass->add ("Example Numbers", PROP_NUM1,
              sfi_pspec_int ("num1", "Num1", NULL,
                             0, 0, 100, 10, SFI_PARAM_GUI));
  klass->add ("Example Numbers", PROP_NUM2,
              sfi_pspec_real ("num2", "Num2", NULL,
                              33, -50, 50, 5, SFI_PARAM_GUI));
}

void
Example::set_property (guint        prop_id,
                       const Value &value,
                       GParamSpec  *pspec)
{
  DEBUG ("Example::set_property\n");
  switch (prop_id)
    {
    case PROP_NUM1:
      n1 = value.get_num();
      break;
    case PROP_NUM2:
      n2 = value.get_real();
      break;
    }
}

void
Example::get_property (guint       prop_id,
                       Value      &value,
                       GParamSpec *pspec)
{
  DEBUG ("Example::get_property\n");
  switch (prop_id)
    {
    case PROP_NUM1:
      value = n1;
      break;
    case PROP_NUM2:
      value = n2;
      break;
    }
}


const ClassInfo cinfo2 ("/Modules/C++ Example 2",
                        "BseDerived2 is derived from the C++ Example BseExample.");

BSE_CXX_TYPE_REGISTER (Derive2, "BseExample", &cinfo2);

/*Con*/
Derive2::Derive2()
{
  DEBUG ("Derive2 Constructor: %p\n", this);
}

/*Des*/
Derive2::~Derive2()
{
  DEBUG ("Derive2 Destructor: %p\n", this);
}

void
Derive2::class_init (CxxBaseClass *klass)
{
  DEBUG ("Derive2 class-init: %p\n", klass);
  klass->add ("Derive2", PROP_LABEL,
              sfi_pspec_string ("label", "Label", NULL,
                                "Huhu", SFI_PARAM_GUI));
  klass->add ("Derive2", PROP_TOGGLE,
              sfi_pspec_bool ("toggle", "Toggle", NULL,
                              FALSE, SFI_PARAM_GUI));
  klass->add ("Derive2", PROP_CHOICE,
              bse_param_spec_genum ("choice", "Choice", NULL,
                                    BSE_TYPE_BIQUAD_FILTER_NORM, 0, SFI_PARAM_GUI));
}

void
Derive2::set_property (guint        prop_id,
                       const Value &value,
                       GParamSpec  *pspec)
{
  DEBUG ("Derive2::set_property\n");
  switch (prop_id)
    {
    case PROP_LABEL:
      label = value.get_string();
      break;
    case PROP_TOGGLE:
      toggle = value.get_bool();
      break;
    case PROP_CHOICE:
      enu = value.get_enum();
      break;
    }
}

void
Derive2::get_property (guint       prop_id,
                       Value      &value,
                       GParamSpec *pspec)
{
  DEBUG ("Derive2::get_property\n");
  switch (prop_id)
    {
    case PROP_LABEL:
      value = label;
      break;
    case PROP_TOGGLE:
      value = toggle;
      break;
    case PROP_CHOICE:
      value = enu;
      break;
    }
}

} // "C"
