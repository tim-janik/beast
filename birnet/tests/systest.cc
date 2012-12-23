// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
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
