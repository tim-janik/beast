// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_TESTS_H__
#define __SFI_TESTS_H__

#include <sfi/sfi.hh>
#include <rapicorn-test.hh>

static void BIRNET_UNUSED
sfi_init_test (int *argcp, char **argv)
{
  sfi_init (argcp, argv, RAPICORN_PRETTY_FILE, Bse::cstrings_to_vector ("rapicorn-test-initialization=1", NULL));
  unsigned int flags = g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK);
  g_log_set_always_fatal ((GLogLevelFlags) (flags | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
}

#define TICK()  TOK()
#define TACK()  TOK()

#ifndef   _
#  define _(x)          (x)
#  define Q_(x)         (x)
#  define N_(x)         (x)
#  define U_(x)         (x)
#endif

/** Macro for test repetitions needed to benchmark in the order of milliseconds.
 * TEST_CALIBRATION() - This macro is used to calculate the number of
 * repetitions needed for execution of a test routine, so that the total
 * duration is long enough to be measured by a timer with millisecond
 * resolution like gettimeofday().
 * Given an upper test duration bound, this macro will return the number
 * of inner loop repetitions needed for benchmarking a piece of code.
 * Estimated run time: the calibration process should take somewhat
 * less than MIN (max_calibration_time, (target_ms * 2 * 7)) milliseconds.
 * @li @c target_ms - expected total test runtime for RUNS * dups * testfunc_time
 * @li @c max_calibration_time - expected upper bound for test calibration runtime
 * In this macro, "dups" is calculated appropriately for max_calibration_time,
 * and scaled upon return to match target_ms accordingly.
 */
#define TEST_CALIBRATION(target_ms, CODE)		({			 	\
  const uint   runs = 7;                                                                \
  const double max_calibration_time = 35.0;                                             \
  double       factor = MAX (1.0, (runs * target_ms * 2) / max_calibration_time);	\
  double       ms, scaled_target_ms = target_ms / factor;                               \
  GTimer       *calibration_timer = false ? g_timer_new() : NULL;                       \
  if (calibration_timer)                                                                \
    g_timer_start (calibration_timer);                                                  \
  GTimer *timer = g_timer_new();                                                        \
  guint   dups = 1;                                                                     \
  /* birnet_thread_yield(); * on some OSes, this can stabelize the loop benches */	\
  do                                                                                    \
    {                                                                                   \
      uint i, j;                                                                        \
      ms = 9e300;                                                                       \
      for (i = 0; i < runs && ms >= scaled_target_ms; i++)                              \
        {                                                                               \
          g_timer_start (timer);                                                        \
          for (j = 0; j < dups; j++)                                                    \
            {                                                                           \
              CODE;                                                                     \
            }                                                                           \
          g_timer_stop (timer);                                                         \
          double current_run_ms = g_timer_elapsed (timer, NULL) * 1000;                 \
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
      g_printerr ("TEST_CALIBRATION: this system can do %d dups in %.6f milliseconds\n",\
                  (guint) (dups * factor), ms * factor);                                \
      g_printerr ("TEST_CALIBRATION: calibration took %.6f milliseconds\n",		\
                  calibration_time_ms); 						\
    }                                                                                   \
  dups = MAX ((uint) (dups * factor), 1);                                               \
  dups;                                                                                 \
})


#endif /* __SFI_TESTS_H__ */
