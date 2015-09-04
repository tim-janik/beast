// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bsemain.hh>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
  bse_init_inprocess (&argc, argv, "bsetool"); // Bse::cstrings_to_vector (NULL)
  // now that the BSE thread runs, drop scheduling priorities if we have any
  setpriority (PRIO_PROCESS, getpid(), 0);
  printf ("bsetool!\n");
  return 0;
}
