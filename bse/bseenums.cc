// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#define BSE_IDL_SURROGATES 1 // dummy values to satisfy the old code generator
#include "bseenums.hh"
#include "gslcommon.hh"
#include "bse/internal.hh"

/* include generated enum value arrays and *.h files the enums steam from */
#include "bse/bseenum_arrays.cc"

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
#include "bse/bseenum_list.cc"
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
	assert_return_unreached ();
    }
}

String
bse_error_blurb (Bse::Error error_value)
{
  return Aida::Introspection::enumerator_blurb_from_value ("Bse.Error", int64_t (error_value));
}

Bse::Error
bse_error_from_errno (int             v_errno,
		      Bse::Error    fallback)
{
  return gsl_error_from_errno (v_errno, fallback);
}
