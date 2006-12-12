/*
 * Copyright (C) 2003,2004 Stefan Westerfeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsecxxapi.hh"
#include <bse/bse.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

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

  const int max_calls = 10000;
  double start = gettime ();

  for(int i=0; i < max_calls; i++)
    note_to_freq (MUSICAL_TUNING_EQUAL_TEMPERAMENT, 60, 0);

  double t = gettime () - start;
  printf ("%f seconds for %d invocations => %f invocations/second, %f milli seconds per invocation\n",
          t, max_calls, max_calls / t, t * 1000 / max_calls);

  sfi_glue_context_pop ();
}

/* vim:set ts=8 sts=2 sw=2: */
