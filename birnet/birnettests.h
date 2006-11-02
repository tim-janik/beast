/* BirnetTests - Utilities for writing test programs
 * Copyright (C) 2006 Tim Janik
 * Copyright (C) 2006 Stefan Westerfeld
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
#ifdef  __cplusplus
#include <birnet/birnet.hh>
#else
#include <birnet/birnetconfig.h>
#endif

/* this file may be included by C programs */

#include <glib.h>
#include <string.h>

BIRNET_EXTERN_C_BEGIN();

/* === auxillary macros for test programs === */

/* provide dummy i18n function for test programs */
#ifndef	  _
#  define _(x) 		(x)
#  define Q_(x)		(x)
#  define N_(x)		(x)
#  define U_(x)		(x)
#endif

/* --- macros --- */
/* macros used for testing.
 * note that g_print() does fflush(stdout) automatically.
 * however we're using g_printerr() for test messages to also allow testing
 * of programs which generate output on stdout.
 */
typedef void (*BirnetTAbort) (void*);
#ifdef	TEST_VERBOSE
#define TSTART(...)		TSTART_impl (":\n", __VA_ARGS__)	/* test intro */
#define TOK()           	do { g_printerr ("OK.\n"); } while (0)	/* subtest OK */
#define TICK()          	TOK()					/* subtest OK */
#define TACK()          	do { g_printerr ("ACK.\n"); } while (0)	/* alternate OK */
#define	TPRINT(...)		g_printerr (__VA_ARGS__)		/* misc messages */
#define	TASSERT(code)		TASSERT_impl ("FAIL.\n", code, 2)	/* test assertion */
#define	TASSERT_CMP(a,cmp,b)	TASSERT_CMP_impl ("FAIL.\n",a,cmp,b, 2)	/* test assertion */
#define	TCHECK(code)		TASSERT_impl ("FAIL.\n", code, 0)	/* test assertion (silent) */
#define	TCHECK_CMP(a,cmp,b)	TASSERT_CMP_impl ("FAIL.\n",a,cmp,b, 0)	/* test assertion */
#define	TERROR(...)		TERROR_impl ("FAIL.\n", __VA_ARGS__)	/* test error, abort */
#define TABORT_HANDLER(fnc,dat)	TABORT_set (fnc, dat)                   /* set custom abort handler */
#define TDONE()         	do { g_printerr ("DONE.\n");            /* test outro */ \
                                     TABORT_set (NULL, NULL); } while(0)
#else
#define TSTART(...)		TSTART_impl (": [", __VA_ARGS__)	/* test intro */
#define TOK()           	do { g_printerr ("-"); } while (0)	/* subtest OK */
#define TICK()          	TOK()					/* subtest OK */
#define TACK()          	do { g_printerr ("+"); } while (0)	/* alternate OK */
#define	TPRINT(...)		do { g_printerr ("*"); } while (0)	/* skip messages */
#define	TASSERT(code)		TASSERT_impl ("X", code, 1)		/* test assertion */
#define	TASSERT_CMP(a,cmp,b)	TASSERT_CMP_impl ("X",a,cmp,b, 1)	/* test assertion */
#define	TCHECK(code)		TASSERT_impl ("X", code, 0)		/* test assertion (silent) */
#define	TCHECK_CMP(a,cmp,b)	TASSERT_CMP_impl ("X",a,cmp,b, 0)	/* test assertion */
#define	TERROR(...)		TERROR_impl ("X", __VA_ARGS__)		/* test error, abort */
#define TABORT_HANDLER(fnc,dat)	TABORT_set (fnc, dat)                   /* set custom abort handler */
#define TDONE()         	do { g_printerr ("]\n");                /* test outro */ \
                                     TABORT_set (NULL, NULL); } while(0)
#endif

/* --- performance --- */
typedef enum {
  TUNIT_NONE		= 0,
  TUNIT_PSEC 		= 0xc001,	/* pico second */
  TUNIT_NSEC 		= 0xd001,	/* nano second */
  TUNIT_USEC 		= 0xe001,	/* micro second */
  TUNIT_MSEC 		= 0xf001,	/* milli second */
  TUNIT_SECOND 		= 0x0001,	/* SECOND */
  TUNIT_MINUTE 		= 0x1001,
  TUNIT_HOUR 		= 0x2001,
  TUNIT_DAY 		= 0x3001,
  TUNIT_WEEK 		= 0x4001,
  TUNIT_MONTH 		= 0x5001,
  TUNIT_YEAR 		= 0x6001,
  TUNIT_BIT 		= 0x0002,	/* BIT */
  TUNIT_BYTE 		= 0x0003,	/* BYTE */
  TUNIT_KILO_BYTE 	= 0x1003,
  TUNIT_MEGA_BYTE 	= 0x2003,
  TUNIT_GIGA_BYTE 	= 0x3003,
  TUNIT_TERA_BYTE 	= 0x4003,
  TUNIT_STRUCT		= 0x0004,	/* STRUCT */
  TUNIT_OBJECT		= 0x0005,	/* OBJECT */
  TUNIT_SAMPLE		= 0x0006,	/* SAMPLE */
  TUNIT_STREAM		= 0x0007,	/* STREAM */
  TUNIT_FILE		= 0x0008,	/* FILE */
} TUnitType;
#define	TUNIT(unit1,per_unit2)		((TUnitType) (0x00010000 * (uint) TUNIT_ ## per_unit2 | (uint) TUNIT_ ## unit1))

static const char*
treport_unit (uint tunit)
{
  switch (tunit)
    {
    case 0x0000: default: return "";
    case 0x0001: return "Seconds"; case 0x1001: return "Minutes"; case 0x2001: return "Hours";
    case 0xc001: return "pSeconds"; case 0xd001: return "nSeconds"; case 0xe001: return "uSeconds"; case 0xf001: return "mSeconds";
    case 0x3001: return "Days"; case 0x4001: return "Weeks"; case 0x5001: return "Months"; case 0x6001: return "Years";
    case 0x0002: return "Bits";
    case 0x0003: return "Bytes"; case 0x1003: return "KBytes"; case 0x2003: return "MBytes"; case 0x3003: return "GBytes"; case 0x4003: return "TBytes";
    case 0x0004: return "Structs";
    case 0x0005: return "Objects";
    case 0x0006: return "Samples";
    case 0x0007: return "Streams";
    case 0x0008: return "Files";
    };
}
static void treport_generic (const char *perf_name, double amount, TUnitType amount_unit, int bias);

static void BIRNET_UNUSED
treport_title (const char *perf_name)
{
  treport_generic (perf_name, 0, TUNIT_NONE, 0);
}
static void BIRNET_UNUSED	/* larger amount is better */
treport_maximized (const char *perf_name,
		   double      amount,
		   TUnitType   amount_unit)
{
  treport_generic (perf_name, amount, amount_unit, +1);
}
static void BIRNET_UNUSED	/* smaller amount is better */
treport_minimized (const char *perf_name,
		   double      amount,
		   TUnitType   amount_unit)
{
  treport_generic (perf_name, amount, amount_unit, -1);
}
static const char*
treport_cpu_name (const char *new_info)
{
  if (new_info)
    g_dataset_set_data_full ((void*) g_dataset_destroy, "birnet-treport-custom-info", g_strdup (new_info), g_free);
  /* the implementation of this function is a pretty bad hack around not exporting C symbols... */
  return (const char*) g_dataset_get_data ((void*) g_dataset_destroy, "birnet-treport-custom-info");
}
static void	/* smaller amount is better */
treport_generic (const char *perf_name,
		 double      amount,
		 TUnitType   amount_unit,
		 int         bias)
{
  char numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = "";
  g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%+.14g", amount);
  int l = strlen (numbuf);
  char *c = strchr (numbuf, '.');
  int n = c ? c - numbuf : l;
  const char spaces[] = "                                             ";
  uint indent = 9 - MIN (9, n);
  const char *custom_info = treport_cpu_name (NULL);
  g_print ("#TBENCH%s:%s: %28s:%s%s%s %s%c%s\n",
	   bias > 0 ? "=maxi" : bias < 0 ? "=mini" : "=====",
	   custom_info ? custom_info : "",
	   perf_name,
	   &spaces[sizeof (spaces) - 1 - indent], numbuf, &spaces[sizeof (spaces) - 1 - (23 - MIN (23, indent + l))],
	   treport_unit (amount_unit & 0xffff),
	   amount_unit > 0xffff ? '/' : ' ',
	   treport_unit (amount_unit >> 16));
}

/* --- macro details --- */
static void
tabort_handler (bool   set_values,
                void **func_loc,
                void **data_loc)
{
  /* the implementation of this function is a pretty bad hack around not exporting C symbols... */
  if (set_values)
    {
      g_dataset_set_data_full ((void*) g_dataset_destroy, "birnet-tabort-func", *func_loc, NULL);
      g_dataset_set_data_full ((void*) g_dataset_destroy, "birnet-tabort-data", *data_loc, NULL);
    }
  else
    {
      *func_loc = g_dataset_get_data ((void*) g_dataset_destroy, "birnet-tabort-func");
      *data_loc = g_dataset_get_data ((void*) g_dataset_destroy, "birnet-tabort-data");
    }
}

#define TABORT_set(func,data)           do {            \
  void *__tabort_func = (void*) func;                   \
  void *__tabort_data = (void*) data;                   \
  tabort_handler (1, &__tabort_func, &__tabort_data);   \
} while (0)

#define TABORT_call()                   do {            \
  void *__tabort_func = NULL, *__tabort_data = NULL;    \
  tabort_handler (0, &__tabort_func, &__tabort_data);   \
  if (__tabort_func)                                    \
    ((BirnetTAbort) __tabort_func) (__tabort_data);     \
  G_BREAKPOINT();                                       \
} while (0)

#define TSTART_impl(postfix, ...)	do {		\
  char *_test_name_ = g_strdup_printf (__VA_ARGS__);	\
  g_printerr ("%s%s", _test_name_, postfix);		\
  g_free (_test_name_);					\
} while (0)

#define TASSERT_impl(mark, code, show)	do {		\
  if (code) {						\
    if (show >= 2)					\
      g_printerr ("OK - asserted (%s).\n", #code);	\
    else if (show) TOK ();				\
  } else {						\
  g_printerr ("%s", mark);				\
  g_printerr ("\n***ERROR***\n"                         \
              "%s:%u:%s(): assertion failed: %s\n",	\
           __FILE__, __LINE__, __PRETTY_FUNCTION__,	\
           #code);					\
  TABORT_call(); }                                      \
} while (0)

#define TASSERT_CMP_impl(mark, a, cmp, b, show)	do {	\
  double __tassert_va = a; double __tassert_vb = b;	\
  if (a cmp b) {					\
    if (show >= 2)					\
      g_printerr ("OK - asserted: "			\
                  "%s %s %s: %.17g %s %.17g\n",		\
                  #a, #cmp, #b,				\
                  __tassert_va, #cmp, __tassert_vb);	\
    else if (show) TOK ();				\
  } else {						\
  g_printerr ("%s", mark);				\
  g_printerr ("\n***ERROR***\n"                         \
              "%s:%u:%s(): assertion failed: "		\
              "%s %s %s: %.17g %s %.17g\n",		\
              __FILE__, __LINE__, __PRETTY_FUNCTION__,	\
              #a, #cmp, #b, 				\
              __tassert_va, #cmp, __tassert_vb); 	\
  TABORT_call(); }                                      \
} while (0)

#define TERROR_impl(mark, ...)	do {			\
  g_printerr ("%s", mark);				\
  char *_error_msg_ = g_strdup_printf (__VA_ARGS__);	\
  g_printerr ("\n***ERROR***\n"                         \
              "%s:%u:%s(): %s\n",			\
              __FILE__, __LINE__, __PRETTY_FUNCTION__,	\
              _error_msg_);				\
  g_free (_error_msg_);					\
  TABORT_call();                                        \
} while (0)

/* Given an upper test duration bound, this macro will return the number
 * of inner loop repetitions needed for benchmarking a piece of code.
 * Estimated run time: the calibration process should take somewhat
 * less than MIN (max_calibration_time, (target_ms * 2 * 7)) milliseconds.
 */
#define TEST_CALIBRATION(target_ms, CODE)		({			 	\
  const guint   runs = 7;                                                               \
  const gdouble max_calibration_time = 35.0;                                            \
  gdouble       factor = MAX (1.0, (runs * target_ms * 2) / max_calibration_time);	\
  gdouble       ms, scaled_target_ms = target_ms / factor;                              \
  GTimer       *calibration_timer = false ? g_timer_new() : NULL;                       \
  if (calibration_timer)                                                                \
    g_timer_start (calibration_timer);                                                  \
  GTimer *timer = g_timer_new();                                                        \
  guint   dups = 1;                                                                     \
  /* birnet_thread_yield(); * on some OSes, this can stabelize the loop benches */	\
  do                                                                                    \
    {                                                                                   \
      guint i, j;                                                                       \
      ms = 9e300;                                                                       \
      for (i = 0; i < runs && ms >= scaled_target_ms; i++)                              \
        {                                                                               \
          g_timer_start (timer);                                                        \
          for (j = 0; j < dups; j++)                                                    \
            {                                                                           \
              CODE;                                                                     \
            }                                                                           \
          g_timer_stop (timer);                                                         \
          gdouble current_run_ms = g_timer_elapsed (timer, NULL) * 1000;                \
          ms = MIN (current_run_ms, ms);                                                \
        }                                                                               \
      if (ms < scaled_target_ms)                                                        \
        dups *= 2;                                                                      \
    }                                                                                   \
  while (ms < scaled_target_ms);                                                        \
  factor *= (scaled_target_ms / ms);                                                    \
  g_timer_destroy (timer);                                                              \
  if (calibration_timer)                                                                \
    {                                                                                   \
      g_timer_stop (calibration_timer);                                                 \
      double calibration_time_ms = g_timer_elapsed (calibration_timer, NULL) * 1000;    \
      g_timer_destroy (calibration_timer);                                              \
      g_printerr ("TEST_CALIBRATION: your processor can do %d dups in %.6f msecs\n",    \
                  (guint) (dups * factor), ms * factor);                                \
      g_printerr ("TEST_CALIBRATION: calibration took %.6f msecs\n",			\
                  calibration_time_ms); 						\
    }                                                                                   \
  dups = MAX ((guint) (dups * factor), 1);                                              \
  dups;                                                                                 \
})

/* --- C++ test initialization --- */
#ifdef  __cplusplus
namespace Birnet {
static inline void
birnet_init_test (int    *argc,
		  char ***argv)
{
  /* check that NULL is defined to __null in C++ on 64bit */
  BIRNET_ASSERT (sizeof (NULL) == sizeof (void*));
  /* normal initialization */
  BirnetInitValue ivalues[] = {
    { "stand-alone", "true" },
    { "birnet-test-parse-args", "true" },
    { NULL }
  };
  birnet_init (argc, argv, NULL, ivalues);
  unsigned int flags = g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK);
  g_log_set_always_fatal ((GLogLevelFlags) (flags | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
  CPUInfo ci = cpu_info();
  treport_cpu_name (ci.machine);
  if (init_settings().test_perf)
    g_printerr ("PERF: %s\n", g_get_prgname());
  else
    g_printerr ("TEST: %s\n", g_get_prgname());
}
} // Birnet
#endif

BIRNET_EXTERN_C_END();
