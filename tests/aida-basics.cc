// Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
#include <bse/testing.hh>

using namespace Aida;

static void
test_aida_strings()
{
  StringVector sv;
  sv = string_split_any ("a, b, c", ", ");
  TCMP (string_join (";", sv), ==, "a;;b;;c");
  sv = string_split_any ("a, b, c", ", ", 1);
  TCMP (string_join (";", sv), ==, "a; b, c");
  sv = string_split_any ("abcdef", "");
  TCMP (string_join (";", sv), ==, "a;b;c;d;e;f");
  sv = string_split_any ("abcdef", "", 2);
  TCMP (string_join (";", sv), ==, "a;b;cdef");
  sv = string_split_any ("  foo  , bar     , \t\t baz \n", ",");
  TCMP (string_join (";", sv), ==, "  foo  ; bar     ; \t\t baz \n");
  TASSERT (string_option_check (" foo ", "foo") == true);
  TASSERT (string_option_check (" foo9 ", "foo9") == true);
  TASSERT (string_option_check (" foo7 ", "foo9") == false);
  TASSERT (string_option_check (" bar ", "bar") == true);
  TASSERT (string_option_check (" bar= ", "bar") == true);
  TASSERT (string_option_check (" bar=0 ", "bar") == false);
  TASSERT (string_option_check (" bar=no ", "bar") == false);
  TASSERT (string_option_check (" bar=false ", "bar") == false);
  TASSERT (string_option_check (" bar=off ", "bar") == false);
  TASSERT (string_option_check (" bar=1 ", "bar") == true);
  TASSERT (string_option_check (" bar=2 ", "bar") == true);
  TASSERT (string_option_check (" bar=3 ", "bar") == true);
  TASSERT (string_option_check (" bar=4 ", "bar") == true);
  TASSERT (string_option_check (" bar=5 ", "bar") == true);
  TASSERT (string_option_check (" bar=6 ", "bar") == true);
  TASSERT (string_option_check (" bar=7 ", "bar") == true);
  TASSERT (string_option_check (" bar=8 ", "bar") == true);
  TASSERT (string_option_check (" bar=9 ", "bar") == true);
  TASSERT (string_option_check (" bar=09 ", "bar") == true);
  TASSERT (string_option_check (" bar=yes ", "bar") == true);
  TASSERT (string_option_check (" bar=true ", "bar") == true);
  TASSERT (string_option_check (" bar=on ", "bar") == true);
  TASSERT (string_option_check (" bar=1false ", "bar") == true);
  TASSERT (string_option_check (" bar=0true ", "bar") == false);
  TASSERT (string_option_check (" foo ", "foo") == true);
  TASSERT (string_option_check (" foo9 ", "foo9") == true);
  TASSERT (string_option_check (" foo7 ", "foo9") == false);
  TASSERT (string_option_check (" bar ", "bar") == true);
  TASSERT (string_option_check (" bar= ", "bar") == true);
  TASSERT (string_option_check (" bar=0 ", "bar") == false);
  TASSERT (string_option_check (" bar=no ", "bar") == false);
  TASSERT (string_option_check (" bar=false ", "bar") == false);
  TASSERT (string_option_check (" bar=off ", "bar") == false);
  TASSERT (string_option_check (" bar=1 ", "bar") == true);
  TASSERT (string_option_check (" bar=2 ", "bar") == true);
  TASSERT (string_option_check (" bar=3 ", "bar") == true);
  TASSERT (string_option_check (" bar=4 ", "bar") == true);
  TASSERT (string_option_check (" bar=5 ", "bar") == true);
  TASSERT (string_option_check (" bar=6 ", "bar") == true);
  TASSERT (string_option_check (" bar=7 ", "bar") == true);
  TASSERT (string_option_check (" bar=8 ", "bar") == true);
  TASSERT (string_option_check (" bar=9 ", "bar") == true);
  TASSERT (string_option_check (" bar=09 ", "bar") == true);
  TASSERT (string_option_check (" bar=yes ", "bar") == true);
  TASSERT (string_option_check (" bar=true ", "bar") == true);
  TASSERT (string_option_check (" bar=on ", "bar") == true);
  TASSERT (string_option_check (" bar=1false ", "bar") == true);
  TASSERT (string_option_check (" bar=0true ", "bar") == false);
  TASSERT (typeid_name<int>() == "int");
  TASSERT (typeid_name<bool>() == "bool");
  TASSERT (typeid_name<SharedFromThisIface>() == "Aida::SharedFromThisIface");
  TASSERT (string_from_double (1.0) == "1");
  TASSERT (string_from_double (-1.0) == "-1");
  TASSERT (string_from_double (0.0) == "0");
  TASSERT (string_from_double (0.5) == "0.5");
  TASSERT (string_from_double (-0.5) == "-0.5");
  TASSERT (string_to_int ("-1") == -1);
  TASSERT (string_to_int ("9223372036854775807") == 9223372036854775807LL);
  TASSERT (string_to_int ("-9223372036854775808") == -9223372036854775807LL - 1);
  TASSERT (string_to_uint ("0") == 0);
  TASSERT (string_to_uint ("1") == 1);
  TASSERT (string_to_uint ("18446744073709551615") == 18446744073709551615ULL);
  TASSERT (string_to_bool ("0") == false);
  TASSERT (string_to_bool ("1") == true);
  TASSERT (string_to_bool ("true") == true);
  TASSERT (string_to_bool ("false") == false);
  TASSERT (string_to_bool ("on") == 1);
  TASSERT (string_to_bool ("off") == 0);
  TCMP (string_to_cquote ("\""), ==, "\"\\\"\"");
  TCMP (string_to_cquote ("\1"), ==, "\"\\001\"");
  TCMP (string_to_cquote ("A\nB"), ==, "\"A\\nB\"");
  TASSERT (string_startswith ("foo", "fo") == true);
  TASSERT (string_startswith ("foo", "o") == false);
  TASSERT (string_match_identifier_tail ("x.FOO", "Foo") == true);
  TASSERT (string_match_identifier_tail ("x.FOO", "X-Foo") == true);
  TASSERT (string_match_identifier_tail ("xFOO", "Foo") == false);
}
TEST_ADD (test_aida_strings);

static void
test_aida_posix_printf ()
{
  // check Aida's posix_printf
  String f0911 = Aida::Any (0.911).to_string();
  TASSERT (f0911.size() >= 3);
  TASSERT (f0911[0] == '0');
  TASSERT (f0911[1] == '.');
  TASSERT (f0911[2] == '9');
}
TEST_ADD (test_aida_posix_printf);
