// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
//#define TEST_VERBOSE
#include <birnet/birnettests.h>
using namespace Birnet;
#if BIRNET_CHECK_VERSION (2147483647, 2147483647, 2147483647) || !BIRNET_CHECK_VERSION (0, 0, 1)      
#error BIRNET_CHECK_VERSION() apparently broken
#endif
static void
test_cpu_info (void)
{
  TSTART ("CpuInfo");
  TOK();
  const BirnetCPUInfo cpi = cpu_info ();
  TASSERT (cpi.machine != NULL);
  String cps = cpu_info_string (cpi);
  TASSERT (cps.size() != 0);
  TPRINT ("%s", cps.c_str());
  TOK();
  TDONE();
}
static void
test_paths()
{
  TSTART ("Path handling");
  String p, s;
  s = Path::join ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f");
#if BIRNET_DIR_SEPARATOR == '/'
  p = "0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f";
#else
  p = "0\\1\\2\\3\\4\\5\\6\\7\\8\\9\\a\\b\\c\\d\\e\\f";
#endif
  // g_printerr ("%s == %s\n", s.c_str(), p.c_str());
  TASSERT (s == p);
  bool b = Path::isabs (p);
  TASSERT (b == false);
#if BIRNET_DIR_SEPARATOR == '/'
  s = Path::join (BIRNET_DIR_SEPARATOR_S, s);
#else
  s = Path::join ("C:\\", s);
#endif
  b = Path::isabs (s);
  TASSERT (b == true);
  s = Path::skip_root (s);
  TASSERT (s == p);
  TASSERT (Path::basename ("simple") == "simple");
  TASSERT (Path::basename ("skipthis" BIRNET_DIR_SEPARATOR_S "file") == "file");
  TASSERT (Path::basename (BIRNET_DIR_SEPARATOR_S "skipthis" BIRNET_DIR_SEPARATOR_S "file") == "file");
  TASSERT (Path::dirname ("file") == ".");
  TASSERT (Path::dirname ("dir" BIRNET_DIR_SEPARATOR_S) == "dir");
  TASSERT (Path::dirname ("dir" BIRNET_DIR_SEPARATOR_S "file") == "dir");
  TDONE();
}
static void
test_zintern()
{
  static const unsigned char TEST_DATA[] = "x\332K\312,\312K-\321\255\312\314+I-\312S(I-.QHI,I\4\0v\317\11V";
  TSTART ("ZIntern");
  uint8 *data = zintern_decompress (24, TEST_DATA, sizeof (TEST_DATA) / sizeof (TEST_DATA[0]));
  TASSERT (String ((char*) data) == "birnet-zintern test data");
  zintern_free (data);
  TOK();
  TDONE();
}
static void
test_files (const char *argv0)
{
  TSTART ("FileChecks");
  TASSERT (Path::equals ("/bin", "/../bin") == TRUE);
  TASSERT (Path::equals ("/bin", "/sbin") == FALSE);
  TASSERT (Path::check (argv0, "e") == TRUE);
  TASSERT (Path::check (argv0, "r") == TRUE);
  // TASSERT (Path::check (argv0, "w") == TRUE); // fails on kfreebsd
  TASSERT (Path::check (argv0, "x") == TRUE);
  TASSERT (Path::check (argv0, "d") == FALSE);
  TASSERT (Path::check (argv0, "l") == FALSE);
  TASSERT (Path::check (argv0, "c") == FALSE);
  TASSERT (Path::check (argv0, "b") == FALSE);
  TASSERT (Path::check (argv0, "p") == FALSE);
  TASSERT (Path::check (argv0, "s") == FALSE);
  TDONE();
}
static void
test_messaging ()
{
  TSTART ("Message Types");
  TASSERT (Msg::NONE    == Msg::lookup_type ("none"));
  TASSERT (Msg::ALWAYS  == Msg::lookup_type ("always"));
  TASSERT (Msg::ERROR   == Msg::lookup_type ("error"));
  TASSERT (Msg::WARNING == Msg::lookup_type ("warning"));
  TASSERT (Msg::SCRIPT  == Msg::lookup_type ("script"));
  TASSERT (Msg::INFO    == Msg::lookup_type ("info"));
  TASSERT (Msg::DIAG    == Msg::lookup_type ("diag"));
  TASSERT (Msg::DEBUG   == Msg::lookup_type ("debug"));
  TASSERT (Msg::check (Msg::NONE) == false);
  TASSERT (Msg::check (Msg::ALWAYS) == true);
  Msg::enable (Msg::NONE);
  Msg::disable (Msg::ALWAYS);
  TASSERT (Msg::check (Msg::NONE) == false);
  TASSERT (Msg::check (Msg::ALWAYS) == true);
  TASSERT (Msg::check (Msg::INFO) == true);
  Msg::disable (Msg::INFO);
  TASSERT (Msg::check (Msg::INFO) == false);
  Msg::enable (Msg::INFO);
  TASSERT (Msg::check (Msg::INFO) == true);
  TDONE();
  Msg::display (Msg::WARNING,
                Msg::Title ("Warning Title"),
                Msg::Text1 ("Primary warning message."),
                Msg::Text2 ("Secondary warning message."),
                Msg::Text2 ("Continuation of secondary warning message."),
                Msg::Text3 ("Message details: 1, 2, 3."),
                Msg::Text3 ("And more message details: a, b, c."),
                Msg::Check ("Show this message again."));
}
static void
test_virtual_typeid()
{
  struct TypeA : public virtual VirtualTypeid {};
  struct TypeB : public virtual VirtualTypeid {};
  TSTART ("VirtualTypeid");
  TypeA a;
  TypeB b;
  TOK();
  TASSERT (a.typeid_name() != b.typeid_name());
  TASSERT (strstr (a.typeid_pretty_name().c_str(), "TypeA") != NULL);
  TASSERT (strstr (b.typeid_pretty_name().c_str(), "TypeB") != NULL);
  TDONE();
}
int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);
  test_cpu_info();
  test_paths();
  test_zintern();
  test_files (argv[0]);
  test_messaging();
  test_virtual_typeid();
  return 0;
}
/* vim:set ts=8 sts=2 sw=2: */
