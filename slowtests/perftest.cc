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
#include "bsecxxapi.h"
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
  g_thread_init (NULL);
  sfi_init ();
  sfi_log_allow_debug ("misc");
  bse_init_async (&argc, &argv, NULL);
  bse_context = bse_init_glue_context (argv[0]);

  sfi_glue_context_push (bse_context);

  const int MAX = 10000;
  double start = gettime ();

  for(int i=0; i<MAX; i++)
    note_to_freq (60, 0);

  double t = gettime () - start;
  printf ("%f seconds for %d invocations => %f invocations/second, %f ms per invocation\n",
          t, MAX, MAX / t, t * 1000.0 / MAX);

  sfi_glue_context_pop ();
}

/* vim:set ts=8 sts=2 sw=2: */
