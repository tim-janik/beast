/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2003 Tim Janik
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
#include "bsemain.h"
#include "topconfig.h"
#include "bseserver.h"
#include "bsesequencer.h"
#include "bsejanitor.h"
#include "bseplugin.h"
#include "bsecategories.h"
#include "bsemidireceiver.h"
#include "gslcommon.h"
#include "bsepcmdevice.h"
#include "bsemididevice.h"
#include "bseengine.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


/* --- prototypes --- */
static void	bse_main_loop		(gpointer	data);
static void	bse_async_parse_args	(gint	       *argc_p,
					 gchar	     ***argv_p,
                                         BseMainArgs   *margs,
                                         SfiRec        *config);


/* --- variables --- */
/* from bse.h */
const guint		 bse_major_version = BSE_MAJOR_VERSION;
const guint		 bse_minor_version = BSE_MINOR_VERSION;
const guint		 bse_micro_version = BSE_MICRO_VERSION;
const guint		 bse_interface_age = BSE_INTERFACE_AGE;
const guint		 bse_binary_age = BSE_BINARY_AGE;
const gchar		*bse_version = BSE_VERSION;
GMainContext            *bse_main_context = NULL;
SfiMutex	         bse_main_sequencer_mutex = { 0, };
gboolean	         bse_main_debug_extensions = FALSE;
SfiThread               *bse_main_thread = NULL;
static volatile gboolean bse_initialization_stage = 0;
static gboolean          textdomain_setup = FALSE;
static BseMainArgs       default_main_args = {
  .path_binaries = BSE_PATH_BINARIES,
};
BseMainArgs             *bse_main_args = &default_main_args;


/* --- functions --- */
const gchar*
bse_gettext (const gchar *text)
{
  g_assert (textdomain_setup == TRUE);
  return dgettext (BSE_GETTEXT_DOMAIN, text);
}

void
bse_init_async (gint    *argc,
		gchar ***argv,
		SfiRec  *config)
{
  SfiThread *thread;

  if (bse_initialization_stage != 0)
    g_error ("%s() may only be called once", "bse_init_async");
  bse_initialization_stage++;
  if (bse_initialization_stage != 1)
    g_error ("%s() may only be called once", "bse_init_async");

  bindtextdomain (BSE_GETTEXT_DOMAIN, BST_PATH_LOCALE);
  bind_textdomain_codeset (BSE_GETTEXT_DOMAIN, "UTF-8");
  textdomain_setup = TRUE;

  /* this function is running in the user program and needs to start the main BSE thread */
  
  /* initialize submodules */
  sfi_init ();
  /* paranoid assertions */
  g_assert (G_BYTE_ORDER == G_LITTLE_ENDIAN || G_BYTE_ORDER == G_BIG_ENDIAN);

  /* handle argument early*/
  SfiRec *unref_me = NULL;
  if (!config)
    config = unref_me = sfi_rec_new();
  if (argc && argv)
    {
      if (*argc && !g_get_prgname ())
	g_set_prgname (**argv);
      bse_async_parse_args (argc, argv, bse_main_args, config);
    }
  
  /* start main BSE thread */
  thread = sfi_thread_run ("BSE Core", bse_main_loop, sfi_thread_self ());
  if (!thread)
    g_error ("failed to start seperate thread for BSE core");

  /* wait for initialization completion of the core thread */
  while (bse_initialization_stage < 2)
    sfi_thread_sleep (-1);
  /* cleanup */
  if (unref_me)
    sfi_rec_unref (unref_me);
}

gchar*
bse_check_version (guint required_major,
		   guint required_minor,
		   guint required_micro)
{
  if (required_major > BSE_MAJOR_VERSION)
    return "BSE version too old (major mismatch)";
  if (required_major < BSE_MAJOR_VERSION)
    return "BSE version too new (major mismatch)";
  if (required_minor > BSE_MINOR_VERSION)
    return "BSE version too old (minor mismatch)";
  if (required_minor < BSE_MINOR_VERSION)
    return "BSE version too new (minor mismatch)";
  if (required_micro < BSE_MICRO_VERSION - BSE_BINARY_AGE)
    return "BSE version too new (micro mismatch)";
  if (required_micro > BSE_MICRO_VERSION)
    return "BSE version too old (micro mismatch)";
  return NULL;
}

typedef struct {
  SfiGlueContext *context;
  const gchar *client;
  SfiThread *thread;
} AsyncData;

static gboolean
async_create_context (gpointer data)
{
  AsyncData *adata = data;
  SfiComPort *port1, *port2;

  sfi_com_port_create_linked ("Client", adata->thread, &port1,
			      "Server", sfi_thread_self (), &port2);
  adata->context = sfi_glue_encoder_context (port1);
  bse_janitor_new (port2);

  /* wakeup client */
  sfi_thread_wakeup (adata->thread);

  return FALSE; /* single-shot */
}

SfiGlueContext*
bse_init_glue_context (const gchar *client)
{
  AsyncData adata = { 0, };
  GSource *source;

  g_return_val_if_fail (client != NULL, NULL);

  /* function runs in user threads and queues handler in BSE thread to create context */

  if (bse_initialization_stage < 2)
    g_error ("%s() called without prior %s()",
	     "bse_init_glue_context",
	     "bse_init_async");

  /* queue handler to create context */
  source = g_idle_source_new ();
  g_source_set_priority (source, G_PRIORITY_HIGH);
  adata.client = client;
  adata.thread = sfi_thread_self ();
  g_source_set_callback (source, async_create_context, &adata, NULL);
  g_source_attach (source, bse_main_context);
  g_source_unref (source);
  /* wake up BSE thread */
  g_main_context_wakeup (bse_main_context);

  /* wait til context creation */
  do
    sfi_thread_sleep (-1);
  while (!adata.context);

  return adata.context;
}

static void
bse_init_core (void)
{
  /* global threading things */
  sfi_mutex_init (&bse_main_sequencer_mutex);
  bse_main_context = g_main_context_new ();
  sfi_thread_set_wakeup ((SfiThreadWakeup) g_main_context_wakeup,
			 bse_main_context, NULL);
  sfi_log_set_thread_handler (bse_log_handler);

  /* initialize random numbers */
  {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    srand (tv.tv_usec + (tv.tv_sec << 16));
  }
  
  /* initialize basic components */
  bse_globals_init ();
  _bse_init_categories ();
  bse_type_init ();
  bse_cxx_init ();
  bse_cxx_checks ();
  
  /* FIXME: global spawn dir is evil */
  {
    gchar *dir = g_get_current_dir ();
    sfi_com_set_spawn_dir (dir);
    g_free (dir);
  }
  
  /* initialize GSL components */
  {
    static const GslConfigValue gslconfig[] = {
      { "wave_chunk_padding",		BSE_MAX_BLOCK_PADDING, },
      { "wave_chunk_big_pad",		256, },
      { "dcache_block_size",		4000, },
      { "dcache_cache_memory",		10 * 1024 * 1024, },
      { "midi_kammer_note",		BSE_KAMMER_NOTE, },
      { "kammer_freq",			BSE_KAMMER_FREQUENCY_f, },
      { NULL, },
    };
    gsl_init (gslconfig);
  }
  
  /* remaining BSE components */
  _bse_midi_init ();
  bse_plugin_init_builtins ();
  /* initialize C wrappers around C++ generated types */
  _bse_init_c_wrappers ();

  /* make sure the server is alive */
  bse_server_get ();

  /* load drivers early */
  if (bse_main_args->load_drivers_early)
    {
      SfiRing *ring = bse_plugin_path_list_files (TRUE, FALSE);
      while (ring)
        {
          gchar *name = sfi_ring_pop_head (&ring);
          const char *error = bse_plugin_check_load (name);
          if (error)
            sfi_diag ("while loading \"%s\": %s", name, error);
          g_free (name);
        }
    }

  /* dump device list */
  if (bse_main_args->dump_driver_list)
    {
      g_printerr (_("\nAvailable PCM drivers:\n"));
      bse_device_dump_list (BSE_TYPE_PCM_DEVICE, "  ", TRUE, NULL, NULL);
      g_printerr (_("\nAvailable MIDI drivers:\n"));
      bse_device_dump_list (BSE_TYPE_MIDI_DEVICE, "  ", TRUE, NULL, NULL);
    }
}

void
bse_init_intern (gint    *argc,
		 gchar ***argv,
		 SfiRec  *config)
{
  SfiRec *unref_me = NULL;
  if (bse_initialization_stage != 0)
    g_error ("%s() may only be called once", "bse_init_intern");
  bse_initialization_stage++;
  if (bse_initialization_stage != 1)
    g_error ("%s() may only be called once", "bse_init_intern");

  bindtextdomain (BSE_GETTEXT_DOMAIN, BST_PATH_LOCALE);
  bind_textdomain_codeset (BSE_GETTEXT_DOMAIN, "UTF-8");
  textdomain_setup = TRUE;
  
  /* initialize submodules */
  sfi_init ();
  if (!config)
    config = unref_me = sfi_rec_new();
  /* paranoid assertions */
  g_assert (G_BYTE_ORDER == G_LITTLE_ENDIAN || G_BYTE_ORDER == G_BIG_ENDIAN);
  /* early argument handling */
  if (argc && argv)
    {
      if (*argc && !g_get_prgname ())
	g_set_prgname (**argv);
      bse_async_parse_args (argc, argv, bse_main_args, config);
    }
  
  bse_init_core ();

  /* initialize core plugins */
  if (sfi_rec_get_bool (config, "load-core-plugins"))
    {
      SfiRing *ring = bse_plugin_path_list_files (!bse_main_args->load_drivers_early, TRUE);
      while (ring)
        {
          gchar *name = sfi_ring_pop_head (&ring);
          const char *error = bse_plugin_check_load (name);
          if (error)
            sfi_diag ("while loading \"%s\": %s", name, error);
          g_free (name);
        }
    }
  if (unref_me)
    sfi_rec_unref (unref_me);
}

static void
bse_main_loop (gpointer data)
{
  SfiThread *client = data;

  bse_main_thread = sfi_thread_self ();

  bse_init_core ();

  /* start other threads */
  bse_sequencer_init_thread ();

  /* notify client about completion */
  bse_initialization_stage++;   /* =2 */
  sfi_thread_wakeup (client);

  /* and away into the main loop */
  do
    {
      g_main_context_pending (bse_main_context);
      g_main_context_iteration (bse_main_context, TRUE);
    }
  while (!sfi_thread_aborted ());
}

guint
bse_main_getpid (void)
{
  if (bse_initialization_stage >= 2)
    return sfi_thread_self_pid ();
  else
    return 0;
}

static gboolean
core_thread_send_message (gpointer data)
{
  BseUserMsg *umsg = data;
  bse_server_user_message (bse_server_get(),
                           umsg->log_domain,
                           umsg->msg_type,
                           umsg->config_blurb,
                           umsg->message,
                           umsg->pid,
                           umsg->process);
  bse_user_msg_free (umsg);
  return FALSE;
}

/**
 * bse_log_handler
 * BSE log handler, suitable for sfi_log_set_thread_handler().
 * This function is MT-safe and may be called from any thread.
 */
void
bse_log_handler (SfiLogMessage *message)
{
  if (!message->message)
    return;
  BseUserMsg *umsg = bse_user_msg_new();
  g_free (umsg->log_domain);
  umsg->log_domain = g_strdup (message->log_domain);
  switch (message->level)
    {
    case SFI_LOG_ERROR:   umsg->msg_type = BSE_USER_MSG_ERROR;   break;
    case SFI_LOG_WARNING: umsg->msg_type = BSE_USER_MSG_WARNING; break;
    case SFI_LOG_INFO:    umsg->msg_type = BSE_USER_MSG_INFO;    break;
    default:              umsg->msg_type = BSE_USER_MSG_MISC;    break;
    }
  g_free (umsg->config_blurb);
  umsg->config_blurb = g_strdup (message->config_blurb);
  g_free (umsg->message);
  umsg->message = g_strdup (message->message);
  umsg->pid = sfi_thread_get_pid (NULL);
  g_free (umsg->process);
  umsg->process = g_strdup (sfi_thread_get_name (NULL));
  bse_idle_now (core_thread_send_message, umsg);
}

static void
bse_async_parse_args (gint        *argc_p,
		      gchar     ***argv_p,
                      BseMainArgs *margs,
		      SfiRec      *config)
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  gchar *envar;
  guint i, e;
  
  /* this function is called before the main BSE thread is started,
   * so we can't use any BSE functions yet.
   */

  envar = getenv ("BSE_DEBUG");
  if (envar)
    sfi_debug_allow (envar);
  envar = getenv ("BSE_NO_DEBUG");
  if (envar)
    sfi_debug_deny (envar);

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
	{
	  GLogLevelFlags fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
	  g_log_set_always_fatal (fatal_mask);
	  
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-debug", argv[i]) == 0 ||
	       strncmp ("--bse-debug=", argv[i], 12) == 0)
	{
	  gchar *equal = argv[i] + 11;
	  if (*equal == '=')
            sfi_debug_allow (equal + 1);
	  else if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
	      sfi_debug_allow (argv[i]);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-no-debug", argv[i]) == 0 ||
	       strncmp ("--bse-no-debug=", argv[i], 15) == 0)
	{
	  gchar *equal = argv[i] + 14;
	  if (*equal == '=')
            sfi_debug_deny (equal + 1);
	  else if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
	      sfi_debug_deny (argv[i]);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-latency", argv[i]) == 0 ||
	       strncmp ("--bse-latency=", argv[i], 14) == 0)
	{
	  gchar *equal = argv[i] + 13;
	  if (*equal == '=')
            margs->latency = g_ascii_strtoull (equal + 1, NULL, 10);
	  else if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
              margs->latency = g_ascii_strtoull (argv[i], NULL, 10);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-mixing-freq", argv[i]) == 0 ||
	       strncmp ("--bse-mixing-freq=", argv[i], 18) == 0)
	{
	  gchar *equal = argv[i] + 17;
	  if (*equal == '=')
            margs->mixing_freq = g_ascii_strtoull (equal + 1, NULL, 10);
	  else if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
              margs->mixing_freq = g_ascii_strtoull (argv[i], NULL, 10);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-control-freq", argv[i]) == 0 ||
	       strncmp ("--bse-control-freq=", argv[i], 19) == 0)
	{
	  gchar *equal = argv[i] + 18;
	  if (*equal == '=')
            margs->control_freq = g_ascii_strtoull (equal + 1, NULL, 10);
	  else if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
              margs->control_freq = g_ascii_strtoull (argv[i], NULL, 10);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-driver-list", argv[i]) == 0)
	{
          margs->load_drivers_early = TRUE;
          margs->dump_driver_list = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-pcm-driver", argv[i]) == 0)
	{
          if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
              margs->pcm_drivers = sfi_ring_append (margs->pcm_drivers, argv[i]);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-midi-driver", argv[i]) == 0)
	{
          if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
              margs->midi_drivers = sfi_ring_append (margs->midi_drivers, argv[i]);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-override-plugin-globs", argv[i]) == 0 && i + 1 < argc)
	{
          argv[i++] = NULL;
          margs->override_plugin_globs = argv[i];
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-override-script-path", argv[i]) == 0 && i + 1 < argc)
	{
          argv[i++] = NULL;
          margs->override_script_path = argv[i];
	  argv[i] = NULL;
	}
#if 0
      else if (strcmp ("--bse-override-binaries-path", argv[i]) == 0 && i + 1 < argc)
	{
          argv[i++] = NULL;
          margs->path_binaries = argv[i];
	  argv[i] = NULL;
	}
#endif
    }

  e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;

  if (sfi_rec_get_bool (config, "debug-extensions"))
    bse_main_debug_extensions = TRUE;
}
