/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2003 Tim Janik and Stefan Westerfeld
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
#include "gslcommon.h"

#include "gsldatacache.h"
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/time.h>


/* --- variables --- */
volatile guint64     bse_engine_exvar_tick_stamp = 0;   /* initialized to 1 upon gsl_init(), so 0==invalid */
static guint64	     tick_stamp_system_time = 0;
static guint         global_tick_stamp_leaps = 0;


/* --- tick stamps --- */
static BirnetMutex     global_tick_stamp_mutex = { 0, };
/**
 * gsl_tick_stamp
 * @RETURNS: GSL's execution tick stamp as unsigned 64bit integer
 *
 * Retrieve the global GSL tick counter stamp.
 * GSL increments its global tick stamp at certain intervals,
 * by specific amounts (refer to bse_engine_init() for further
 * details). The tick stamp is a non-wrapping, unsigned 64bit
 * integer greater than 0. Threads can schedule sleep interruptions
 * at certain tick stamps with sfi_thread_awake_after() and
 * sfi_thread_awake_before(). Tick stamp updating occours at
 * GSL engine block processing boundaries, so code that can
 * guarantee to not run across those boundaries (for instance
 * BseProcessFunc() functions) may use the macro %GSL_TICK_STAMP
 * to retrieve the current tick in a faster manner (not involving
 * mutex locking). See also bse_module_tick_stamp().
 * This function is MT-safe and may be called from any thread.
 */
guint64
gsl_tick_stamp (void)
{
  guint64 stamp;

  GSL_SPIN_LOCK (&global_tick_stamp_mutex);
  stamp = bse_engine_exvar_tick_stamp;
  GSL_SPIN_UNLOCK (&global_tick_stamp_mutex);

  return stamp;
}

void
_gsl_tick_stamp_set_leap (guint ticks)
{
  GSL_SPIN_LOCK (&global_tick_stamp_mutex);
  global_tick_stamp_leaps = ticks;
  GSL_SPIN_UNLOCK (&global_tick_stamp_mutex);
}

/**
 * gsl_tick_stamp_last
 * @RETURNS: Current tick stamp and system time in micro seconds
 *
 * Get the system time of the last GSL global tick stamp update.
 * This function is MT-safe and may be called from any thread.
 */
GslTickStampUpdate
gsl_tick_stamp_last (void)
{
  GslTickStampUpdate ustamp;

  GSL_SPIN_LOCK (&global_tick_stamp_mutex);
  ustamp.tick_stamp = bse_engine_exvar_tick_stamp;
  ustamp.system_time = tick_stamp_system_time;
  GSL_SPIN_UNLOCK (&global_tick_stamp_mutex);

  return ustamp;
}

void
_gsl_tick_stamp_inc (void)
{
  volatile guint64 newstamp;
  guint64 systime;

  g_return_if_fail (global_tick_stamp_leaps > 0);

  systime = sfi_time_system ();
  newstamp = bse_engine_exvar_tick_stamp + global_tick_stamp_leaps;

  GSL_SPIN_LOCK (&global_tick_stamp_mutex);
  bse_engine_exvar_tick_stamp = newstamp;
  tick_stamp_system_time = systime;
  GSL_SPIN_UNLOCK (&global_tick_stamp_mutex);

  sfi_thread_emit_wakeups (newstamp);
}

/**
 * gsl_thread_awake_before
 * @tick_stamp: tick stamp update to trigger wakeup
 * Wakeup the currently running thread upon the last global tick stamp
 * update (see gsl_tick_stamp()) that happens prior to updating the
 * global tick stamp to @tick_stamp.
 * (If the moment of wakeup has already passed by, the thread is
 * woken up at the next global tick stamp update.)
 */
void
gsl_thread_awake_before (guint64 tick_stamp)
{
  g_return_if_fail (tick_stamp > 0);

  if (tick_stamp > global_tick_stamp_leaps)
    sfi_thread_awake_after (tick_stamp - global_tick_stamp_leaps);
  else
    sfi_thread_awake_after (tick_stamp);
}


/* --- misc --- */
const gchar*
gsl_byte_order_to_string (guint byte_order)
{
  g_return_val_if_fail (byte_order == G_LITTLE_ENDIAN || byte_order == G_BIG_ENDIAN, NULL);

  if (byte_order == G_LITTLE_ENDIAN)
    return "little-endian";
  if (byte_order == G_BIG_ENDIAN)
    return "big-endian";

  return NULL;
}

guint
gsl_byte_order_from_string (const gchar *string)
{
  g_return_val_if_fail (string != NULL, 0);

  while (*string == ' ')
    string++;
  if (strncasecmp (string, "little", 6) == 0)
    return G_LITTLE_ENDIAN;
  if (strncasecmp (string, "big", 3) == 0)
    return G_BIG_ENDIAN;
  return 0;
}

BseErrorType
gsl_file_check (const gchar *file_name,
		const gchar *mode)
{
  if (birnet_file_check (file_name, mode))
    return BSE_ERROR_NONE;
  return gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
}

BseErrorType
gsl_error_from_errno (gint         sys_errno,
		      BseErrorType fallback)
{
  switch (sys_errno)
    {
    case 0:             return BSE_ERROR_NONE;
    case ELOOP:
    case ENAMETOOLONG:
    case ENOENT:        return BSE_ERROR_FILE_NOT_FOUND;
    case EISDIR:        return BSE_ERROR_FILE_IS_DIR;
    case EROFS:
    case EPERM:
    case EACCES:        return BSE_ERROR_PERMS;
#ifdef ENODATA  /* GNU/kFreeBSD lacks this */
    case ENODATA:
#endif
    case ENOMSG:        return BSE_ERROR_FILE_EOF;
    case ENOMEM:	return BSE_ERROR_NO_MEMORY;
    case ENOSPC:	return BSE_ERROR_NO_SPACE;
    case ENFILE:	return BSE_ERROR_NO_FILES;
    case EMFILE:	return BSE_ERROR_MANY_FILES;
    case EFBIG:
    case ESPIPE:
    case EIO:           return BSE_ERROR_IO;
    case EEXIST:        return BSE_ERROR_FILE_EXISTS;
    case ETXTBSY:
    case EBUSY:         return BSE_ERROR_FILE_BUSY;
    case EAGAIN:
    case EINTR:		return BSE_ERROR_TEMP;
    case EFAULT:        return BSE_ERROR_INTERNAL;
    case EBADF:
    case ENOTDIR:
    case ENODEV:
    case EINVAL:
    default:            return fallback;
    }
}

static guint
score_error (BseErrorType error)
{
  /* errors are sorted by increasing descriptiveness */
  static const BseErrorType error_score[] = {
    BSE_ERROR_NONE /* least descriptive, indicates 0-initialized error variable */,
    BSE_ERROR_UNKNOWN, BSE_ERROR_INTERNAL, BSE_ERROR_TEMP,
    BSE_ERROR_IO, BSE_ERROR_FILE_EOF,
    BSE_ERROR_FILE_OPEN_FAILED, BSE_ERROR_FILE_SEEK_FAILED,
    BSE_ERROR_FILE_READ_FAILED, BSE_ERROR_FILE_WRITE_FAILED,
    BSE_ERROR_FILE_NOT_FOUND, BSE_ERROR_WAVE_NOT_FOUND,
  };
  guint i;
  for (i = 0; i < G_N_ELEMENTS (error_score); i++)
    if (error_score[i] == error)
      return i;
  return i;
}

BseErrorType
gsl_error_select (guint           n_errors,
                  BseErrorType    first_error,
                  ...)
{
  BseErrorType *errors = g_new (BseErrorType, MAX (1, n_errors));
  va_list args;
  guint i, e, score;
  /* function used to select a descriptive error in
   * multi-error scenarios
   */
  va_start (args, first_error);
  for (i = 0; i < n_errors; i++)
    {
      if (i)
        first_error = va_arg (args, BseErrorType);
      errors[i] = first_error;
    }
  va_end (args);
  /* grab first error, unless followed by an error with higher score */
  e = errors[0];
  score = score_error (e);
  for (i = 1; i < n_errors; i++)
    {
      guint s = score_error (errors[i]);
      if (s > score)
        {
          score = s;
          e = errors[i];
        }
    }
  g_free (errors);
  return e;
}


/* --- progress notification --- */
GslProgressState
gsl_progress_state (gpointer        data,
                    GslProgressFunc pfunc,
                    guint           precision)
{
  GslProgressState pstate = { 0, -99, };
  pstate.pfunc = pfunc;
  pstate.pdata = data;
  pstate.precision = precision = CLAMP (precision, 0, 9);
  pstate.epsilon = 1;
  while (precision--)
    pstate.epsilon *= 0.1;
  pstate.epsilon *= 0.5;
  return pstate;
}

void
gsl_progress_notify (GslProgressState *pstate,
                     gfloat            pval,
                     const gchar      *detail_format,
                     ...)
{
  gboolean need_update;

  g_return_if_fail (pstate != NULL);

  if (pval >= 0)
    {
      pval = CLAMP (pval, 0, 100);
      need_update = ABS (pval - pstate->pval) > pstate->epsilon;
    }
  else
    {
      pval = -1;
      need_update = TRUE;
    }

  if (need_update && pstate->pfunc)
    {
      gchar *detail = NULL;
      guint l;
      if (detail_format)
        {
          va_list args;
          va_start (args, detail_format);
          detail = g_strdup_vprintf (detail_format, args);
          va_end (args);
        }
      pstate->pval = pval;
      l = pstate->pfunc (pstate->pdata, pstate->pval, detail && detail[0] ? detail : NULL, pstate);
      pstate->wipe_length = MAX (pstate->wipe_length, l);
      g_free (detail);
    }
}

void
gsl_progress_wipe (GslProgressState *pstate)
{
  g_return_if_fail (pstate != NULL);

  if (pstate->wipe_length)
    {
      gchar *wstr = g_malloc (pstate->wipe_length + 1 + 1);
      memset (wstr, ' ', pstate->wipe_length);
      wstr[pstate->wipe_length] = '\r';
      wstr[pstate->wipe_length + 1] = 0;
      g_printerr (wstr);
      g_free (wstr);
      pstate->wipe_length = 0;
    }
}

guint
gsl_progress_printerr (gpointer          message,
                       gfloat            pval,
                       const gchar      *detail,
                       GslProgressState *pstate)
{
  gchar *str, format[128] = "%s%sprocessed %5.1f%% %s%s%s";
  gchar *ppos = strchr (format, '1');
  guint l, prec = pstate->precision;
  ppos[0] = '0' + CLAMP (prec, 0, 9);
  str = g_strdup_printf (format,
                         message ? (gchar*) message : "",
                         message ? ": " : "",
                         pval,
                         detail ? "(" : "",
                         detail ? detail : "",
                         detail ? ")" : "");
  l = strlen (str);
  g_printerr ("%s            \r", str);
  g_free (str);
  return l;
}

/* --- global initialization --- */
void
gsl_init (void)
{
  g_return_if_fail (bse_engine_exvar_tick_stamp == 0);  /* assert single initialization */
  bse_engine_exvar_tick_stamp = 1;

  /* initialize subsystems */
  sfi_mutex_init (&global_tick_stamp_mutex);
  _gsl_init_fd_pool ();
  _gsl_init_data_caches ();
  _gsl_init_loader_gslwave ();
  _gsl_init_loader_aiff ();
  _gsl_init_loader_wav ();
  _gsl_init_loader_oggvorbis ();
  _gsl_init_loader_mad ();
  bse_init_loader_gus_patch ();
}
