// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_TESTS_H__
#define __SFI_TESTS_H__

#include <sfi/sfi.hh>
#include <birnet/birnettests.h>

/* --- test initialization --- */
static void BIRNET_UNUSED
sfi_init_test (int          *argcp,
	       char       ***argvp,
	       SfiInitValue *nvalues)
{
  SfiInitValue empty_init_value = { NULL, };
  if (!nvalues)
    nvalues = &empty_init_value;
  SfiInitValue jvalues[] = {
    { "stand-alone", "true" },
    { "birnet-test-parse-args", "true" },
    { NULL }
  };
  guint i, j = 0, n = 0;
  while (jvalues[j].value_name)
    j++;
  while (nvalues[n].value_name)
    n++;
  SfiInitValue *iv = g_new (SfiInitValue, j + n + 1);
  for (i = 0; i < j; i++)
    iv[i] = jvalues[i];
  for (i = 0; i < n; i++)
    iv[j + i] = nvalues[i];
  iv[j + n] = empty_init_value;
  sfi_init (argcp, argvp, NULL, iv);
  g_free (iv);
  unsigned int flags = g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK);
  g_log_set_always_fatal ((GLogLevelFlags) (flags | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
  SfiCPUInfo ci = sfi_cpu_info();
  treport_cpu_name (ci.machine);
  g_printerr ("TEST: %s\n", g_get_prgname());
  /* check NULL definition, especially for 64bit */
  BIRNET_ASSERT (sizeof (NULL) == sizeof (void*));
}

#endif /* __SFI_TESTS_H__ */

/* vim:set ts=8 sts=2 sw=2: */
