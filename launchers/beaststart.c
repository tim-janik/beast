// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#include "suidmain.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

const char*
getbeastexepath (int *argc, char ***argv)
{
  const char *procexe = NULL;
#if     defined __linux__
  procexe = "/proc/self/exe";           // Linux
#elif   defined __NetBSD__
  procexe = "/proc/curproc/exe";        // NetBSD
#elif   defined __FreeBSD__ || defined __DragonFly__
  procexe = "/proc/curproc/file";       // FreeBSD
#elif   defined __sun
  procexe = "/proc/self/path/a.out";    // Solaris
#endif
  errno = ENOSYS;
  char *exe = procexe ? realpath (procexe, NULL) : NULL;
  if (exe)
    {
      if (0 == access (exe, X_OK))
        {
          const size_t l = strlen (exe);
          if (l > 3 && strcmp (exe + l - 3, "-rt") == 0)
            return strndup (exe, l - 3); // strips "-rt"
        }
      free (exe);
    }
  return NULL; // reading link failed
}

int
custom_check_arg_stopper (const char *argument)
{
  if (strcmp (argument, "--") == 0)
    return 1;
  return 0;
}
