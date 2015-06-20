// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseenums.hh"
#include "gslcommon.hh"
#include <errno.h>

/* include generated enum value arrays and *.h files the enums steam from */
#include "bseenum_arrays.cc"

/* --- functions --- */
void
bse_type_register_enums (void)
{
  static const struct {
    const char *name;
    GType       parent_type;
    GType      *type_p;
    void       *values;
  } enums[] = {
    /* include generated enum list */
#include "bseenum_list.cc"
  };
  uint n_enums = sizeof (enums) / sizeof (enums[0]);
  uint i;

  for (i = 0; i < n_enums; i++)
    {
      if (enums[i].parent_type == G_TYPE_ENUM)
	{
	  *(enums[i].type_p) = g_enum_register_static (enums[i].name, (GEnumValue*) enums[i].values);
	  g_value_register_transform_func (SFI_TYPE_CHOICE, *(enums[i].type_p), sfi_value_choice2enum_simple);
	  g_value_register_transform_func (*(enums[i].type_p), SFI_TYPE_CHOICE, sfi_value_enum2choice);
	}
      else if (enums[i].parent_type == G_TYPE_FLAGS)
	*(enums[i].type_p) = g_flags_register_static (enums[i].name, (GFlagsValue*) enums[i].values);
      else
	g_assert_not_reached ();
    }
}

/* BseErrorType is a static type */
static GEnumClass *bse_error_class = NULL;

const char*
bse_error_name (BseErrorType error_value)
{
  GEnumValue *ev;

  if (!bse_error_class)
    bse_error_class = (GEnumClass*) g_type_class_ref (BSE_TYPE_ERROR_TYPE);

  ev = g_enum_get_value (bse_error_class, error_value);
  return ev ? ev->value_name : NULL;
}

const char*
bse_error_nick (BseErrorType error_value)
{
  GEnumValue *ev;

  if (!bse_error_class)
    bse_error_class = (GEnumClass*) g_type_class_ref (BSE_TYPE_ERROR_TYPE);

  ev = g_enum_get_value (bse_error_class, error_value);
  return ev ? ev->value_nick : NULL;
}

const char*
bse_error_blurb (BseErrorType error_value)
{
  const Rapicorn::Aida::EnumValue *ev = Rapicorn::Aida::enum_info<Bse::ErrorType>().find_value (error_value);
  return ev ? ev->blurb : NULL;
}

BseErrorType
bse_error_from_errno (int             v_errno,
		      BseErrorType    fallback)
{
  return gsl_error_from_errno (v_errno, fallback);
}
