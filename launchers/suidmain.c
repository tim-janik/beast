/* suidmain - suid wrapper to acquire capabilities and drop root suid
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
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static int      /* returns 0 for success */
adjust_priority (void)
{
  int current_priority;

  /* figure current priority */
  errno = 0;
  current_priority = getpriority (PRIO_PROCESS, getpid());
  if (errno != 0)
    {
      /* not really fatal */
      current_priority = 0;
    }

  /* improve priority */
  if (current_priority > -10)
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
  int priority_error = adjust_priority ();

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
    }

  /* make sure we have a program name */
  if (argc < 1)
    return -1;

  /* give notice about errors */
  if (euid == 0 && priority_error)
    fprintf (stderr, "%s: failed to renice process: %s\n", argv[0], strerror (priority_error));

  /* find executable */
  executable = custom_find_executable (&argc, &argv);

  /* exec */
  argv[0] = executable;
  execv (executable, argv);
  /* handle execution errors */
  perror (executable);
  return -1;
}
