// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#include "suidmain.h"
#include "../configure.h" // BEAST_EXEC_POSTFIX
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const char*
custom_find_executable (int    *argc_p,
                        char ***argv_p)
{
  const char *bindir = BINDIR;
  const char *name = "beast";
  const char *postfix = BEAST_EXEC_POSTFIX;
  int l = 1 + strlen (bindir) + 1 + strlen (name) + strlen (postfix);
  char *string = malloc (l);
  if (!string)
    {
      perror ((*argv_p)[0]);
      exit (-1);
    }
  string[0] = 0;
  strcat (string, bindir);
  strcat (string, "/");
  strcat (string, name);
  strcat (string, postfix);
  return string;
}

int
custom_check_arg_stopper (const char *argument)
{
  if (strcmp (argument, "--") == 0)
    return 1;
  return 0;
}
