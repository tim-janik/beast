// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsecxxapi.hh"
#include <bse/bse.hh>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
static SfiGlueContext *bse_context = NULL;
using namespace Bse;
using namespace std;
double
gettime ()
{
  timeval tv;
  gettimeofday (&tv, 0);
  return double(tv.tv_sec) + double(tv.tv_usec) * (1.0 / 1000000.0);
}
int main(int argc, char **argv)
{
  std::set_terminate (__gnu_cxx::__verbose_terminate_handler);
  bse_init_async (&argc, &argv, "Perftest", NULL);
  sfi_msg_allow ("misc");
  bse_context = bse_init_glue_context (argv[0]);
  sfi_glue_context_push (bse_context);
  printf ("%s: testing remote glue layer calls via C++ interface:\n", argv[0]);
  const int max_calls = 30000;
  const int runs = 7;
  double t = 1e7;
  for (int r = 0; r < runs; r++)
    {
      double start = gettime ();
      for(int i=0; i < max_calls; i++)
        note_to_freq (MUSICAL_TUNING_12_TET, 60, 0);
      t = std::min (gettime () - start, t);
    }
  printf ("%f seconds for %d invocations => %f invocations/second, %f milli seconds per invocation\n",
          t, max_calls, max_calls / t, t * 1000 / max_calls);
  sfi_glue_context_pop ();
}
/* vim:set ts=8 sts=2 sw=2: */
