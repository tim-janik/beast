// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bse.hh>
#include <../beast-gtk/bstoldbseapi.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

using namespace Bse;
using namespace std;

static SfiGlueContext *bse_context = NULL;

static double
gettime ()
{
  timeval tv;
  gettimeofday (&tv, 0);
  return double(tv.tv_sec) + double(tv.tv_usec) * (1.0 / 1000000.0);
}

int
main (int argc, char **argv)
{
  std::set_terminate (__gnu_cxx::__verbose_terminate_handler);
  Bse::init_async (&argc, argv, "Perftest");
  bse_context = Bse::init_glue_context (argv[0], []() { g_main_context_wakeup (g_main_context_default()); });
  sfi_glue_context_push (bse_context);
  printf ("%s: testing remote glue layer calls via legacy C interface:\n", argv[0]);
  const int max_calls = 60000;
  const int runs = 7;
  double t = 1e7;
  for (int r = 0; r < runs; r++)
    {
      double start = gettime ();
      for(int i=0; i < max_calls; i++)
        bse_note_to_freq (BSE_MUSICAL_TUNING_12_TET, 60, 0);
      t = std::min (gettime () - start, t);
    }
  printf ("%f seconds for %d invocations => %f invocations/second, %f µseconds per invocation\n",
          t, max_calls, max_calls / t, t * 1000000. / max_calls);
  sfi_glue_context_pop ();
}

#include <../beast-gtk/bstoldbseapi.cc>
