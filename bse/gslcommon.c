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
volatile guint64     gsl_externvar_tick_stamp = 0;
static guint64	     tick_stamp_system_time = 0;
static guint         global_tick_stamp_leaps = 0;


/* --- tick stamps --- */
static SfiMutex     global_tick_stamp_mutex = { 0, };
/**
 * gsl_tick_stamp
 * @RETURNS: GSL's execution tick stamp as unsigned 64bit integer
 *
 * Retrieve the global GSL tick counter stamp.
 * GSL increments its global tick stamp at certain intervals,
 * by specific amounts (refer to gsl_engine_init() for further
 * details). The tick stamp is a non-wrapping, unsigned 64bit
 * integer greater than 0. Threads can schedule sleep interruptions
 * at certain tick stamps with sfi_thread_awake_after() and
 * sfi_thread_awake_before(). Tick stamp updating occours at
 * GSL engine block processing boundaries, so code that can
 * guarantee to not run across those boundaries (for instance
 * GslProcessFunc() functions) may use the macro %GSL_TICK_STAMP
 * to retrieve the current tick in a faster manner (not involving
 * mutex locking). See also gsl_module_tick_stamp().
 * This function is MT-safe and may be called from any thread.
 */
guint64
gsl_tick_stamp (void)
{
  guint64 stamp;

  GSL_SPIN_LOCK (&global_tick_stamp_mutex);
  stamp = gsl_externvar_tick_stamp;
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
  ustamp.tick_stamp = gsl_externvar_tick_stamp;
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
  newstamp = gsl_externvar_tick_stamp + global_tick_stamp_leaps;

  GSL_SPIN_LOCK (&global_tick_stamp_mutex);
  gsl_externvar_tick_stamp = newstamp;
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
gsl_strerror (GslErrorType error)
{
  switch (error)
    {
    case GSL_ERROR_NONE:		return "Everything went well";
    case GSL_ERROR_INTERNAL:		return "Internal error (please report)";
    case GSL_ERROR_UNKNOWN:		return "Unknown error";
    case GSL_ERROR_IO:			return "Input/output error";
    case GSL_ERROR_PERMS:		return "Insufficient permission";
    case GSL_ERROR_BUSY:		return "Device or resource busy";
    case GSL_ERROR_EXISTS:		return "File exists already";
    case GSL_ERROR_EOF:			return "File empty or premature EOF";
    case GSL_ERROR_NOT_FOUND:		return "No such file (or directory)";
    case GSL_ERROR_IS_DIR:		return "Is a directory";
    case GSL_ERROR_OPEN_FAILED:		return "Open failed";
    case GSL_ERROR_SEEK_FAILED:		return "Seek failed";
    case GSL_ERROR_READ_FAILED:		return "Read failed";
    case GSL_ERROR_WRITE_FAILED:	return "Write failed";
    case GSL_ERROR_MANY_FILES:		return "Too many open files";
    case GSL_ERROR_NO_FILES:		return "Too many open files in system";
    case GSL_ERROR_NO_SPACE:		return "No space left on device";
    case GSL_ERROR_NO_MEMORY:		return "Out of memory";
    case GSL_ERROR_NO_HEADER:		return "Failed to detect (start of) header";
    case GSL_ERROR_NO_SEEK_INFO:	return "Failed to retrieve seek information";
    case GSL_ERROR_NO_DATA:		return "No data available";
    case GSL_ERROR_DATA_CORRUPT:        return "Data corrupt";
    case GSL_ERROR_FORMAT_INVALID:	return "Invalid format";
    case GSL_ERROR_FORMAT_UNKNOWN:	return "Unknown format";
    case GSL_ERROR_TEMP:		return "Temporary error";
    case GSL_ERROR_WAVE_NOT_FOUND:	return "No such wave";
    case GSL_ERROR_CODEC_FAILURE:	return "CODEC failure";
    default:				return NULL;
    }
}

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

GslErrorType
gsl_check_file (const gchar *file_name,
		const gchar *mode)
{
  guint access_mask = 0;
  guint check_file, check_dir, check_link;
  
  if (strchr (mode, 'r'))	/* readable */
    access_mask |= R_OK;
  if (strchr (mode, 'w'))	/* writable */
    access_mask |= W_OK;
  if (strchr (mode, 'x'))	/* executable */
    access_mask |= X_OK;

  if (access_mask && access (file_name, access_mask) < 0)
    goto have_errno;
  
  check_file = strchr (mode, 'f') != NULL;	/* open as file */
  check_dir  = strchr (mode, 'd') != NULL;	/* open as directory */
  check_link = strchr (mode, 'l') != NULL;	/* open as link */

  if (check_file || check_dir || check_link)
    {
      struct stat st;
      
      if (check_link)
	{
	  if (lstat (file_name, &st) < 0)
	    goto have_errno;
	}
      else if (stat (file_name, &st) < 0)
	goto have_errno;

      if (check_file && S_ISDIR (st.st_mode))
        return GSL_ERROR_IS_DIR;

      if ((check_file && !S_ISREG (st.st_mode)) ||
	  (check_dir && !S_ISDIR (st.st_mode)) ||
	  (check_link && !S_ISLNK (st.st_mode)))
	return GSL_ERROR_OPEN_FAILED;
    }

  return GSL_ERROR_NONE;
  
 have_errno:
  return gsl_error_from_errno (errno, GSL_ERROR_OPEN_FAILED);
}

GslErrorType
gsl_error_from_errno (gint         sys_errno,
		      GslErrorType fallback)
{
  switch (sys_errno)
    {
    case ELOOP:
    case ENAMETOOLONG:
    case ENOTDIR:
    case ENOENT:        return GSL_ERROR_NOT_FOUND;
    case EISDIR:        return GSL_ERROR_IS_DIR;
    case EROFS:
    case EPERM:
    case EACCES:        return GSL_ERROR_PERMS;
    case ENOMEM:	return GSL_ERROR_NO_MEMORY;
    case ENOSPC:	return GSL_ERROR_NO_SPACE;
    case ENFILE:	return GSL_ERROR_NO_FILES;
    case EMFILE:	return GSL_ERROR_MANY_FILES;
    case EFBIG:
    case ESPIPE:
    case EIO:           return GSL_ERROR_IO;
    case EEXIST:        return GSL_ERROR_EXISTS;
    case ETXTBSY:
    case EBUSY:         return GSL_ERROR_BUSY;
    case EAGAIN:
    case EINTR:		return GSL_ERROR_TEMP;
    case EFAULT:
    case EBADF:         return GSL_ERROR_INTERNAL;
    case EINVAL:
    default:            return fallback;
    }
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
  guint prec = pstate->precision;
  ppos[0] = '0' + CLAMP (prec, 0, 9);
  str = g_strdup_printf (format,
                         message ? (gchar*) message : "",
                         message ? ": " : "",
                         pval,
                         detail ? "(" : "",
                         detail ? detail : "",
                         detail ? ")" : "");
  guint l = strlen (str);
  g_printerr ("%s            \r", str);
  g_free (str);
  return l;
}

/* --- global initialization --- */
static guint
get_n_processors (void)
{
#ifdef _SC_NPROCESSORS_ONLN
  {
    gint n = sysconf (_SC_NPROCESSORS_ONLN);

    if (n > 0)
      return n;
  }
#endif
  return 1;
}

static const GslConfig *gsl_config = NULL;

const GslConfig*
gsl_get_config (void)
{
  return gsl_config;
}

#define	ROUND(dblval)	((GslLong) ((dblval) + .5))

void
gsl_init (const GslConfigValue values[])
{
  struct timeval tv;
  const GslConfigValue *config = values;
  static GslConfig pconfig = {	/* DEFAULTS */
    1,				/* n_processors */
    2,				/* wave_chunk_padding */
    4,				/* wave_chunk_big_pad */
    512,			/* dcache_block_size */
    1024 * 1024,		/* dcache_cache_memory */
    69,				/* midi_kammer_note */
    440,			/* kammer_freq */
  };

  sfi_init ();	/* ease transition */

  g_return_if_fail (gsl_config == NULL);	/* assert single initialization */

  gsl_externvar_tick_stamp = 1;

  /* configure permanent config record */
  if (config)
    while (config->value_name)
      {
	if (strcmp ("wave_chunk_padding", config->value_name) == 0)
	  pconfig.wave_chunk_padding = ROUND (config->value);
	else if (strcmp ("wave_chunk_big_pad", config->value_name) == 0)
	  pconfig.wave_chunk_big_pad = ROUND (config->value);
	else if (strcmp ("dcache_cache_memory", config->value_name) == 0)
	  pconfig.dcache_cache_memory = ROUND (config->value);
	else if (strcmp ("dcache_block_size", config->value_name) == 0)
	  pconfig.dcache_block_size = ROUND (config->value);
	else if (strcmp ("midi_kammer_note", config->value_name) == 0)
	  pconfig.midi_kammer_note = ROUND (config->value);
	else if (strcmp ("kammer_freq", config->value_name) == 0)
	  pconfig.kammer_freq = config->value;
	config++;
      }
  
  /* constrain (user) config */
  pconfig.wave_chunk_padding = MAX (1, pconfig.wave_chunk_padding);
  pconfig.wave_chunk_big_pad = MAX (2 * pconfig.wave_chunk_padding, pconfig.wave_chunk_big_pad);
  pconfig.dcache_block_size = MAX (2 * pconfig.wave_chunk_big_pad + sizeof (GslDataType), pconfig.dcache_block_size);
  pconfig.dcache_block_size = sfi_alloc_upper_power2 (pconfig.dcache_block_size - 1);
  /* pconfig.dcache_cache_memory = sfi_alloc_upper_power2 (pconfig.dcache_cache_memory); */

  /* non-configurable config updates */
  pconfig.n_processors = get_n_processors ();

  /* export GSL configuration */
  gsl_config = &pconfig;

  /* initialize random numbers */
  gettimeofday (&tv, NULL);
  srand (tv.tv_sec ^ tv.tv_usec);

  /* initialize subsystems */
  sfi_mutex_init (&global_tick_stamp_mutex);
  _gsl_init_signal ();
  _gsl_init_fd_pool ();
  _gsl_init_data_caches ();
  _gsl_init_engine_utils ();
  _gsl_init_loader_gslwave ();
  _gsl_init_loader_wav ();
  _gsl_init_loader_oggvorbis ();
  _gsl_init_loader_mad ();
}
