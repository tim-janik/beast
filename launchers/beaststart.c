/* beaststart - start a beast executable
 *
 * This software is provided "as is"; redistribution and modification
 * is permitted, provided that the following disclaimer is retained.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
#include "topconfig.h"  /* holds HAVE_SETEUID etc... */
#include "suidmain.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const char*
custom_find_executable (int    *argc_p,
                        char ***argv_p)
{
  const char *bindir = BINDIR;
  const char *name = "beast";
  const char *version = BIN_VERSION;
  int l = strlen (bindir) + 1 + strlen (name) + 1 + strlen (version);
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
  strcat (string, "-");
  strcat (string, version);
  return string;
}
