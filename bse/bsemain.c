/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#define	_XOPEN_SOURCE	600	/* for full pthread facilities */
#include	"bsemain.h"

#include	"bseplugin.h"
#include	"gslcommon.h"
#include	"gslengine.h"
#include	<string.h>
#include	<stdlib.h>


/* --- prototypes --- */
static void	call_gsl_init (const GslConfigValue values[]);


/* --- variables --- */
static gboolean        bse_is_initialized = FALSE;
static GslMutex	       sequencer_mutex;
BseDebugFlags          bse_debug_flags = 0;


/* --- functions --- */
gboolean
bse_initialized (void)
{
  return bse_is_initialized;
}

static void
bse_parse_args (gint    *argc_p,
		gchar ***argv_p)
{
  extern GFlagsValue *bse_debug_key_flag_values;	/* bseenums.c feature */
  extern guint        bse_debug_key_n_flag_values;	/* bseenums.c feature */
  static const GDebugKey op_debug_keys[] = {
    { "engine",	GSL_ENGINE_DEBUG_ENGINE },
    { "jobs",	GSL_ENGINE_DEBUG_JOBS },
    { "sched",	GSL_ENGINE_DEBUG_SCHED },
    { "master",	GSL_ENGINE_DEBUG_MASTER },
    { "slave",	GSL_ENGINE_DEBUG_SLAVE },
  };
  static const guint op_n_debug_keys = sizeof (op_debug_keys) / sizeof (op_debug_keys[0]);
  GDebugKey *debug_keys;
  guint n_debug_keys;
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  gchar *envar;
  guint i, e;

  debug_keys = g_new (GDebugKey, bse_debug_key_n_flag_values);
  for (i = 0; i < bse_debug_key_n_flag_values && bse_debug_key_flag_values[i].value_nick; i++)
    {
      debug_keys[i].key = bse_debug_key_flag_values[i].value_nick;
      debug_keys[i].value = bse_debug_key_flag_values[i].value;
    }
  n_debug_keys = i;
  
  envar = getenv ("BSE_DEBUG");
  if (envar)
    {
      guint op_lvl;

      bse_debug_flags |= g_parse_debug_string (envar, debug_keys, n_debug_keys);
      op_lvl = g_parse_debug_string (envar, op_debug_keys, op_n_debug_keys);
      gsl_engine_debug_enable (op_lvl);
    }
  envar = getenv ("BSE_NO_DEBUG");
  if (envar)
    {
      guint op_lvl;
      
      bse_debug_flags &= ~g_parse_debug_string (envar, debug_keys, n_debug_keys);
      op_lvl = g_parse_debug_string (envar, op_debug_keys, op_n_debug_keys);
      gsl_engine_debug_disable (op_lvl);
    }

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
	{
	  GLogLevelFlags fatal_mask;
	  
	  fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
	  g_log_set_always_fatal (fatal_mask);
	  
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-debug", argv[i]) == 0 ||
	       strncmp ("--bse-debug=", argv[i], 12) == 0)
	{
	  gchar *equal = argv[i] + 11;
	  guint op_lvl = 0;
	  
	  if (*equal == '=')
	    {
	      bse_debug_flags |= g_parse_debug_string (equal + 1, debug_keys, n_debug_keys);
	      op_lvl = g_parse_debug_string (equal + 1, op_debug_keys, op_n_debug_keys);
	    }
	  else if (i + 1 < argc)
	    {
	      bse_debug_flags |= g_parse_debug_string (argv[i + 1],
						       debug_keys,
						       n_debug_keys);
	      op_lvl = g_parse_debug_string (argv[i + 1], op_debug_keys, op_n_debug_keys);
	      argv[i] = NULL;
	      i += 1;
	    }
	  gsl_engine_debug_enable (op_lvl);
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-no-debug", argv[i]) == 0 ||
	       strncmp ("--bse-no-debug=", argv[i], 15) == 0)
	{
	  gchar *equal = argv[i] + 14;
	  guint op_lvl = 0;
	  
	  if (*equal == '=')
	    {
	      bse_debug_flags &= ~g_parse_debug_string (equal + 1, debug_keys, n_debug_keys);
	      op_lvl = g_parse_debug_string (equal + 1, op_debug_keys, op_n_debug_keys);
	    }
	  else if (i + 1 < argc)
	    {
	      bse_debug_flags &= ~g_parse_debug_string (argv[i + 1],
							debug_keys,
							n_debug_keys);
	      op_lvl = g_parse_debug_string (argv[i + 1], op_debug_keys, op_n_debug_keys);
	      argv[i] = NULL;
	      i += 1;
	    }
	  gsl_engine_debug_disable (op_lvl);
	  argv[i] = NULL;
	}
    }
  
  e = 0;
  for (i = 1; i < argc; i++)
    {
      if (e)
	{
	  if (argv[i])
	    {
	      argv[e++] = argv[i];
	      argv[i] = NULL;
	    }
	}
      else if (!argv[i])
	e = i;
    }
  if (e)
    *argc_p = e;

  g_free (debug_keys);
}

static void
dummy_nop (gpointer data)
{
}

static BseLockFuncs        bse_lock_funcs = { NULL, dummy_nop, dummy_nop };

void
bse_init (int	             *argc_p,
	  char	           ***argv_p,
	  const BseLockFuncs *lock_funcs)
{
  g_return_if_fail (bse_is_initialized == FALSE);

  bse_is_initialized = TRUE;
  if (lock_funcs)
    bse_lock_funcs = *lock_funcs;
  
  g_assert (BSE_BYTE_ORDER == BSE_LITTLE_ENDIAN || BSE_BYTE_ORDER == BSE_BIG_ENDIAN);

  gsl_mutex_init (&sequencer_mutex);

  if (argc_p && argv_p)
    {
      g_set_prgname (**argv_p);
      bse_parse_args (argc_p, argv_p);
    }
  
  bse_globals_init ();
  
  bse_type_init ();

  bse_plugins_init ();

  {
    static const GslConfigValue gslconfig[] = {
      { "wave_chunk_padding",		BSE_MAX_BLOCK_PADDING, },
      { "wave_chunk_big_pad",		256, },
      { "dcache_block_size",		8192, },
      { "dcache_cache_memory",		10 * 1024 * 1024, },
      // { "midi_kammer_note",		57, },
      { "kammer_freq",			BSE_KAMMER_FREQ, },
    };

    call_gsl_init (gslconfig);
  }
}

void
bse_main_global_lock (void)
{
  bse_lock_funcs.lock (bse_lock_funcs.lock_data);
}

void
bse_main_global_unlock (void)
{
  bse_lock_funcs.unlock (bse_lock_funcs.lock_data);
}

void
bse_main_sequencer_lock (void)
{
  GSL_SYNC_LOCK (&sequencer_mutex);
}

void
bse_main_sequencer_unlock (void)
{
  GSL_SYNC_UNLOCK (&sequencer_mutex);
}

#if	(GSL_HAVE_MUTEXATTR_SETTYPE > 0)
#include <pthread.h>
static void
pth_mutex_init (GslMutex *mutex)
{
  /* need NULL attribute here, which is the fast mutex on glibc
   * and cannot be chosen through the pthread_mutexattr_settype()
   */
  pthread_mutex_init ((pthread_mutex_t*) mutex, NULL);
}
static void
pth_rec_mutex_init (GslRecMutex *mutex)
{
  pthread_mutexattr_t attr;

  pthread_mutexattr_init (&attr);
  pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init ((pthread_mutex_t*) mutex, &attr);
  pthread_mutexattr_destroy (&attr);
}
static void
pth_rec_cond_init (GslCond *cond)
{
  pthread_cond_init ((pthread_cond_t*) cond, NULL);
}
static void
pth_rec_cond_wait_timed (GslCond  *cond,
			 GslMutex *mutex,
			 gulong    abs_secs,
			 gulong    abs_usecs)
{
  struct timespec abstime;

  abstime.tv_sec = abs_secs;
  abstime.tv_nsec = abs_usecs * 1000;
  pthread_cond_timedwait ((pthread_cond_t*) cond, (pthread_mutex_t*) mutex, &abstime);
}
#endif	/* GSL_HAVE_MUTEXATTR_SETTYPE */


static void
call_gsl_init (const GslConfigValue values[])
{
#if     (GSL_HAVE_MUTEXATTR_SETTYPE > 0)
  static GslMutexTable pth_mutex_table = {
    pth_mutex_init,
    (void (*) (GslMutex*)) pthread_mutex_lock,
    (int  (*) (GslMutex*)) pthread_mutex_trylock,
    (void (*) (GslMutex*)) pthread_mutex_unlock,
    (void (*) (GslMutex*)) pthread_mutex_destroy,
    pth_rec_mutex_init,
    (void (*) (GslRecMutex*)) pthread_mutex_lock,
    (int  (*) (GslRecMutex*)) pthread_mutex_trylock,
    (void (*) (GslRecMutex*)) pthread_mutex_unlock,
    (void (*) (GslRecMutex*)) pthread_mutex_destroy,
    pth_rec_cond_init,
    (void (*)            (GslCond*)) pthread_cond_signal,
    (void (*)            (GslCond*)) pthread_cond_broadcast,
    (void (*) (GslCond*, GslMutex*)) pthread_cond_wait,
    pth_rec_cond_wait_timed,
    (void (*)            (GslCond*)) pthread_cond_destroy,
  }, *mtable = &pth_mutex_table;
#else  /* !GSL_HAVE_MUTEXATTR_SETTYPE */
  static GslMutexTable *mtable = NULL;
#endif /* !GSL_HAVE_MUTEXATTR_SETTYPE */
  
  gsl_init (values, mtable);
  if (mtable)
    g_message ("using Unix98 pthreads directly for mutexes and conditions\n");
}
