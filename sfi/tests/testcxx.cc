// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#undef G_LOG_DOMAIN
#define  G_LOG_DOMAIN __FILE__
#include <sfi/sfitests.hh>
#include "../sficxx.hh"
#include <stdio.h>

using namespace Sfi;

struct Bar {
  int i;
};

typedef Sequence<Bar> BarSeq;
typedef Sequence<Int> IntSeq;

typedef struct {
  guint n_elements;
  Int  *elements;
} CIntSeq;

int
main (int   argc,
      char *argv[])
{
  sfi_init_test (&argc, argv);

  TSTART ("Test String");
  TASSERT (sizeof (String) == sizeof (const char*));
  String s1 = "huhu";
  String s2;
  s2 += "huhu";
  TASSERT (strcmp (s1.c_str(), "huhu") == 0);
  TASSERT (s1 == s2);
  TASSERT (s1 + "HAHA" == std::string ("huhuHAHA"));
  s2 += "1";
  TASSERT (s1 != s2);
  String s3 = "huhu1";
  TASSERT (s2 == s3);
  TDONE();
  TSTART ("Test RecordHandle<>");
  TASSERT (sizeof (RecordHandle<Bar>) == sizeof (void*));
  RecordHandle<Bar> b1;
  TASSERT (b1.c_ptr() == NULL);
  TASSERT (b1.is_null());
  TASSERT (!b1);
  RecordHandle<Bar> b2 (INIT_DEFAULT);
  TASSERT (b2->i == 0);
  RecordHandle<Bar> b3 (INIT_EMPTY);
  Bar b;
  b.i = 5;
  RecordHandle<Bar> b4 = b;
  TASSERT (b4->i == 5);
  TASSERT (b2[0].i == 0);
  TDONE();
  TSTART ("Test IntSeq");
  TASSERT (sizeof (IntSeq) == sizeof (void*));
  IntSeq is (9);
  for (guint i = 0; i < 9; i++)
    is[i] = i;
  for (int i = 0; i < 9; i++)
    TASSERT (is[i] == i);
  is.resize (0);
  TASSERT (is.length() == 0);
  is.resize (12);
  TASSERT (is.length() == 12);
  for (guint i = 0; i < 12; i++)
    is[i] = 2147483600 + i;
  for (int i = 0; i < 12; i++)
    TASSERT (is[i] == 2147483600 + i);
  TDONE();
  TSTART ("Test IntSeq in C");
  CIntSeq *cis = *(CIntSeq**) &is;
  TASSERT (cis->n_elements == 12);
  for (int i = 0; i < 12; i++)
    TASSERT (cis->elements[i] == 2147483600 + i);
  TDONE();
  TSTART ("Test BarSeq");
  TASSERT (sizeof (BarSeq) == sizeof (void*));
  BarSeq bs (7);
  TASSERT (bs.length() == 7);
  for (guint i = 0; i < 7; i++)
    bs[i].i = i;
  for (int i = 0; i < 7; i++)
    TASSERT (bs[i].i == i);
  bs.resize (22);
  TASSERT (bs.length() == 22);
  for (guint i = 0; i < 22; i++)
    bs[i].i = 2147483600 + i;
  for (int i = 0; i < 22; i++)
    TASSERT (bs[i].i == 2147483600 + i);
  bs.resize (0);
  TASSERT (bs.length() == 0);
  TDONE();
  return 0;
}
