// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseconstvalues.hh"

#include "bse/internal.hh"

#include "bse/glib-extra.cc"


// == Testing ==
#include "testing.hh"
namespace { // Anon
using namespace Bse;

BSE_INTEGRITY_TEST (bse_test_type_renames);
static void
bse_test_type_renames (void)
{
  gchar *str;
  str = g_type_name_to_cname ("PrefixTypeName");
  TASSERT (strcmp (str, "prefix_type_name") == 0);
  g_free (str);
  str = g_type_name_to_sname ("PrefixTypeName");
  TASSERT (strcmp (str, "prefix-type-name") == 0);
  g_free (str);
  str = g_type_name_to_cupper ("PrefixTypeName");
  TASSERT (strcmp (str, "PREFIX_TYPE_NAME") == 0);
  g_free (str);
  str = g_type_name_to_type_macro ("PrefixTypeName");
  TASSERT (strcmp (str, "PREFIX_TYPE_TYPE_NAME") == 0);
  g_free (str);
  str = g_type_name_to_sname ("prefix_type_name");
  TASSERT (strcmp (str, "prefix-type-name") == 0);
  g_free (str);
  str = g_type_name_to_cname ("prefix-type-name");
  TASSERT (strcmp (str, "prefix_type_name") == 0);
  g_free (str);
}

} // Anon
