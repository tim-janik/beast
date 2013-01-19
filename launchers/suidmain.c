// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#include "topconfig.h"  /* holds HAVE_SETEUID etc... */
#include "suidmain.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
static int original_priority = 0;
static int      /* returns 0 for success */
adjust_priority (void)
{
  /* figure current priority */
  errno = 0;
  original_priority = getpriority (PRIO_PROCESS, getpid());
  if (errno != 0)
    {
      /* not really fatal */
      original_priority = 0;
    }
  /* improve priority */
  if (original_priority > -10)
    {
      errno = 0;
      setpriority (PRIO_PROCESS, getpid(), PRIO_MIN);
    }
  else
    {
      /* assume somebody already adjusted our priority to a desired value */
    }
  if (errno != 0)
    return errno;       /* failed */
  return 0;
}
int
main (int    argc,
      char **argv)
{
  const char *executable = NULL;
  int euid = geteuid ();
  int uid = getuid ();
  /* call privileged code */
  int priority_error = adjust_priority (); /* sets original_priority */
  /* drop root privileges if running setuid root as soon as possible */
  if (euid != uid)
    {
#if     HAVE_SETEUID
      seteuid (uid);
#elif   HAVE_SETREUID
      setreuid (-1, uid);
#else
#error platform misses facility to drop privileges
#endif
      /* verify priviledge drop */
      if (geteuid() != uid)
        {
          fprintf (stderr, "%s: failed to drop priviledges: %s\n", argv[0], errno ? strerror (errno) : "Unknown error");
          _exit (255);
        }
    }
  /* non-priviledged code */
  /* make sure we have a program name */
  if (argc < 1)
    return -1;
  /* give notice about errors */
  if (euid == 0 && priority_error)
    fprintf (stderr, "%s: failed to renice process: %s\n", argv[0], strerror (priority_error));
  /* parse -N and -n options */
  int i, dropped_priority = -2147483647;
  for (i = 1; i < argc; i++)
    if (strcmp (argv[i], "-n") == 0 && i + 1 < argc)
      {
        char *endptr = NULL;
        long int nl = strtol (argv[i + 1], &endptr, 10);
        if (endptr == NULL || *endptr == 0)
          dropped_priority = nl;
      }
    else if (strncmp (argv[i], "-n=", 3) == 0)
      {
        char *endptr = NULL;
        long int nl = strtol (argv[i] + 3, &endptr, 10);
        if (endptr == NULL || *endptr == 0)
          dropped_priority = nl;
      }
    else if (strncmp (argv[i], "-n", 2) == 0 && argv[i][3] >= '0' && argv[i][3] <= '9')
      {
        char *endptr = NULL;
        long int nl = strtol (argv[i] + 2, &endptr, 10);
        if (endptr == NULL || *endptr == 0)
          dropped_priority = nl;
      }
    else if (strcmp (argv[i], "-N") == 0)
      dropped_priority = original_priority;
    else if (custom_check_arg_stopper (argv[i]))        /* check for "--" and similar args */
      break;
  /* handle -N and -n options */
  if (dropped_priority != -2147483647)
    setpriority (PRIO_PROCESS, getpid(), dropped_priority);
  /* find executable */
  executable = custom_find_executable (&argc, &argv);
  /* exec */
  argv[0] = (char*) executable;
  execv (executable, argv);
  /* handle execution errors */
  perror (executable);
  return -1;
}
