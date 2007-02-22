/* Birnet
 * Copyright (C) 2007 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
//#define TEST_VERBOSE
#include <birnet/birnettests.h>
#include <poll.h>

namespace {
using namespace Birnet;

static void
test_poll()
{
  TSTART("POLL constants");
  TASSERT (BIRNET_SYSVAL_POLLIN     == POLLIN);
  TASSERT (BIRNET_SYSVAL_POLLPRI    == POLLPRI);
  TASSERT (BIRNET_SYSVAL_POLLOUT    == POLLOUT);
  TASSERT (BIRNET_SYSVAL_POLLRDNORM == POLLRDNORM);
  TASSERT (BIRNET_SYSVAL_POLLRDBAND == POLLRDBAND);
  TASSERT (BIRNET_SYSVAL_POLLWRNORM == POLLWRNORM);
  TASSERT (BIRNET_SYSVAL_POLLWRBAND == POLLWRBAND);
  TASSERT (BIRNET_SYSVAL_POLLERR    == POLLERR);
  TASSERT (BIRNET_SYSVAL_POLLHUP    == POLLHUP);
  TASSERT (BIRNET_SYSVAL_POLLNVAL   == POLLNVAL);
  TDONE();
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);

  test_poll();

  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
