/* Birnet
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <birnet/birnet.h>

BIRNET_EXTERN_C_BEGIN();

/* === auxillary macros for test programs === */

/* provide dummy i18n function for test programs */
#ifndef	  _
#  define _(x) 		(x)
#  define Q_(x)		(x)
#  define N_(x)		(x)
#  define U_(x)		(x)
#endif

/* macros used for testing.
 * note that g_print() does fflush(stdout) automatically.
 * however we're using g_printerr() for test messages to also allow testing
 * of programs which generate output on stdout.
 */
#ifdef	TEST_VERBOSE
#define TSTART(what)	do { g_printerr ("%s:\n", what); } while (0)	/* test intro */
#define TOK()           do { g_printerr ("OK.\n"); } while (0)		/* subtest OK */
#define TICK()          TOK()						/* subtest OK */
#define TACK()          do { g_printerr ("ACK.\n"); } while (0)		/* alternate OK */
#define TFAIL()         do { g_printerr ("FAIL.\n"); } while (0)	/* allowed failure */
#define	TPRINT		g_printerr					/* misc messages */
#define TDONE()         do { g_printerr ("DONE.\n"); } while (0)	/* test outro */
#else
#define TSTART(what)	do { g_printerr ("%s [", what); } while (0)	/* test intro */
#define TOK()           do { g_printerr ("-"); } while (0)		/* subtest OK */
#define TICK()          TOK()						/* subtest OK */
#define TACK()          do { g_printerr ("+"); } while (0)		/* alternate OK */
#define TFAIL()         do { g_printerr ("X"); } while (0)		/* allowed failure */
#define	TPRINT(...)	do { g_printerr ("*"); } while (0)		/* skip messages */
#define TDONE()         do { g_printerr ("]\n"); } while (0)		/* test outro */
#endif

/* test assertion */
#define TASSERT(code)	do { 				\
  if (code) TOK (); else {				\
  TFAIL();						\
  g_error ("%s:%u: %s: assertion failed: %s",		\
           __FILE__, __LINE__, __PRETTY_FUNCTION__,	\
           #code); }					\
} while (0)

/* convenience initialization functions for tests */
extern inline void
birnet_init_test (int    *argc,
		  char ***argv)
{
  birnet_init (argc, argv, NULL);
  unsigned int flags = g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK);
  g_log_set_always_fatal ((GLogLevelFlags) (flags | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
  g_printerr ("TESTING: %s\n", g_get_prgname());
}

BIRNET_EXTERN_C_END();
