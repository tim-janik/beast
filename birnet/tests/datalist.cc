// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
//#define TEST_VERBOSE
#include <birnet/birnettests.h>
namespace {
using namespace Birnet;
class MyKey : public DataKey<int> {
  void destroy (int i)
  {
    TPRINT ("%s: delete %d;\n", STRFUNC, i);
  }
  int  fallback()
  {
    return -1;
  }
};
class StringKey : public DataKey<String> {
  void destroy (String s)
  {
    TPRINT ("%s: delete \"%s\";\n", STRFUNC, s.c_str());
  }
};
template<class DataListContainer> static void
data_list_test_strings (DataListContainer &r)
{
  static StringKey strkey;
  r.set_data (&strkey, String ("otto"));
  TASSERT (String ("otto") == r.get_data (&strkey).c_str());
  String dat = r.swap_data (&strkey, String ("BIRNET"));
  TASSERT (String ("otto") == dat);
  TASSERT (String ("BIRNET") == r.get_data (&strkey).c_str());
  r.delete_data (&strkey);
  TASSERT (String ("") == r.get_data (&strkey).c_str()); // fallback()
}
template<class DataListContainer> static void
data_list_test_ints (DataListContainer &r)
{
  static MyKey intkey;
  TASSERT (-1 == r.get_data (&intkey)); // fallback() == -1
  int dat = r.swap_data (&intkey, 4);
  TASSERT (dat == -1); // former value
  TASSERT (4 == r.get_data (&intkey));
  r.set_data (&intkey, 5);
  TASSERT (5 == r.get_data (&intkey));
  dat = r.swap_data (&intkey, 6);
  TASSERT (5 == dat);
  TASSERT (6 == r.get_data (&intkey));
  dat = r.swap_data (&intkey, 6);
  TASSERT (6 == dat);
  TASSERT (6 == r.get_data (&intkey));
  dat = r.swap_data (&intkey);
  TASSERT (6 == dat);
  TASSERT (-1 == r.get_data (&intkey)); // fallback()
  r.set_data (&intkey, 8);
  TASSERT (8 == r.get_data (&intkey));
  r.delete_data (&intkey);
  TASSERT (-1 == r.get_data (&intkey)); // fallback()
  r.set_data (&intkey, 9);
  TASSERT (9 == r.get_data (&intkey));
}
static void
data_list_test ()
{
  TSTART ("DataList<String>");
  {
    DataListContainer r;
    data_list_test_strings (r);
  }
  TDONE();
  TSTART ("DataList<int>");
  {
    DataListContainer r;
    data_list_test_ints (r);
  }
  TDONE();
  TSTART ("DataList-mixed");
  {
    DataListContainer r;
    data_list_test_strings (r);
    data_list_test_ints (r);
    data_list_test_strings (r);
  }
  TDONE();
}
} // anon
int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);
  data_list_test();
  return 0;
}
