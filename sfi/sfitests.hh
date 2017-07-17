// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_TESTS_H__
#define __SFI_TESTS_H__

#include <sfi/sfi.hh>
#include <rapicorn-test.hh>

static void RAPICORN_UNUSED
sfi_init_test (int *argcp, char **argv)
{
  sfi_init (argcp, argv, Bse::cstrings_to_vector ("rapicorn-test-initialization=1", NULL));
  unsigned int flags = g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK);
  g_log_set_always_fatal ((GLogLevelFlags) (flags | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
}

#endif /* __SFI_TESTS_H__ */
