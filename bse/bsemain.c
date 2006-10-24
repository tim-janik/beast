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
#include "bsemathsignal.h"
#include "gsldatacache.h"
#include "bsepcmdevice.h"
#include "bsemididevice.h"
#include "bseengine.h"
#include "bseblockutils.hh" /* bse_block_impl_name() */
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sfi/sfitests.h> /* sfti_test_init() */

/* --- prototypes --- */
static void	bse_main_loop		(gpointer	 data);
static void	bse_async_parse_args	(gint	        *argc_p,
					 gchar	      ***argv_p,
                                         BseMainArgs    *margs,
                                         SfiInitValue    values[]);


/* --- variables --- */
/* from bse.h */
const guint		 bse_major_version = BSE_MAJOR_VERSION;
const guint		 bse_minor_version = BSE_MINOR_VERSION;
const guint		 bse_micro_version = BSE_MICRO_VERSION;
const guint		 bse_interface_age = BSE_INTERFACE_AGE;
const guint		 bse_binary_age = BSE_BINARY_AGE;
const gchar		*bse_version = BSE_VERSION;
GMainContext            *bse_main_context = NULL;
BirnetMutex	         bse_main_sequencer_mutex = { 0, };
BirnetThread            *bse_main_thread = NULL;
static volatile gboolean bse_initialization_stage = 0;
static gboolean          textdomain_setup = FALSE;
static BseMainArgs       default_main_args = {
  .path_binaries        = BSE_PATH_BINARIES,
  .n_processors         = 1,
  .wave_chunk_padding   = 64,
  .wave_chunk_big_pad   = 256,
  .dcache_block_size    = 4000,
  .dcache_cache_memory  = 10 * 1024 * 1024,
  .midi_kammer_note     = BSE_KAMMER_NOTE,      /* 69 */
  .kammer_freq          = BSE_KAMMER_FREQUENCY, /* 440Hz, historically 435Hz */
  .force_fpu            = false,
  .allow_randomization  = true,
};
BseMainArgs             *bse_main_args = NULL;


/* --- functions --- */
void
bse_init_textdomain_only (void)
{
  bindtextdomain (BSE_GETTEXT_DOMAIN, BST_PATH_LOCALE);
  bind_textdomain_codeset (BSE_GETTEXT_DOMAIN, "UTF-8");
  textdomain_setup = TRUE;
}

const gchar*
bse_gettext (const gchar *text)
{
  g_assert (textdomain_setup == TRUE);
  return dgettext (BSE_GETTEXT_DOMAIN, text);
}

void
bse_init_async (gint           *argc,
		gchar        ***argv,
                const char     *app_name,
                SfiInitValue    values[])
{
  BirnetThread *thread;

  bse_init_textdomain_only();

  if (bse_initialization_stage != 0)
    g_error ("%s() may only be called once", "bse_init_async");
  bse_initialization_stage++;
  if (bse_initialization_stage != 1)
    g_error ("%s() may only be called once", "bse_init_async");

  /* this function is running in the user program and needs to start the main BSE thread */
  
  /* paranoid assertions */
  g_assert (G_BYTE_ORDER == G_LITTLE_ENDIAN || G_BYTE_ORDER == G_BIG_ENDIAN);

  /* initialize submodules */
  sfi_init (argc, argv, app_name, values);
  bse_main_args = &default_main_args;
  bse_main_args->birnet = sfi_init_settings();

  /* handle argument early*/
  if (argc && argv)
    {
      if (*argc && !g_get_prgname ())
	g_set_prgname (**argv);
      bse_async_parse_args (argc, argv, bse_main_args, values);
    }
  
  /* start main BSE thread */
  thread = sfi_thread_run ("BSE Core", bse_main_loop, sfi_thread_self ());
  if (!thread)
    g_error ("failed to start seperate thread for BSE core");

  /* wait for initialization completion of the core thread */
  while (bse_initialization_stage < 2)
    sfi_thread_sleep (-1);
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
  BirnetThread *thread;
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
  sfi_thread_set_wakeup ((BirnetThreadWakeup) g_main_context_wakeup,
			 bse_main_context, NULL);
  sfi_msg_set_thread_handler (bse_msg_handler);

  /* initialize basic components */
  bse_globals_init ();
  _bse_init_signal();
  _bse_init_categories ();
  bse_type_init ();
  bse_cxx_init ();
  
  /* FIXME: global spawn dir is evil */
  {
    gchar *dir = g_get_current_dir ();
    sfi_com_set_spawn_dir (dir);
    g_free (dir);
  }
  
  /* initialize GSL components */
  gsl_init ();
  
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

static gboolean single_thread_registration_done = FALSE;

static void
server_registration (SfiProxy            server,
                     BseRegistrationType rtype,
                     const gchar        *what,
                     const gchar        *error,
                     gpointer            data)
{
  // BseRegistrationType rtype = bse_registration_type_from_choice (rchoice);
  if (rtype == BSE_REGISTER_DONE)
    single_thread_registration_done = TRUE;
  else
    {
      //gchar *base = strrchr (what, '/');
      //g_message (data, "%s", base ? base + 1 : what);
      if (error && error[0])
        sfi_diag ("failed to register \"%s\": %s", what, error);
    }
}

static void
bse_init_intern (gint           *argc,
		 gchar        ***argv,
                 const char     *app_name,
		 SfiInitValue    values[],
                 bool            as_test)
{
  bse_init_textdomain_only();

  if (bse_initialization_stage != 0)
    g_error ("%s() may only be called once", "bse_init_intern");
  bse_initialization_stage++;
  if (bse_initialization_stage != 1)
    g_error ("%s() may only be called once", "bse_init_intern");

  /* paranoid assertions */
  g_assert (G_BYTE_ORDER == G_LITTLE_ENDIAN || G_BYTE_ORDER == G_BIG_ENDIAN);
  
  /* initialize submodules */
  if (as_test)
    sfi_init_test (argc, argv, values);
  else
    sfi_init (argc, argv, app_name, values);
  bse_main_args = &default_main_args;
  bse_main_args->birnet = sfi_init_settings();
  
  /* early argument handling */
  if (argc && argv)
    {
      if (*argc && !g_get_prgname ())
	g_set_prgname (**argv);
      bse_async_parse_args (argc, argv, bse_main_args, values);
    }
  
  bse_init_core ();

  /* initialize core plugins & scripts */
  if (bse_main_args->load_core_plugins || bse_main_args->load_core_scripts)
      g_object_connect (bse_server_get(), "signal::registration", server_registration, NULL, NULL);
  if (bse_main_args->load_core_plugins)
    {
      g_object_connect (bse_server_get(), "signal::registration", server_registration, NULL, NULL);
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
  if (bse_main_args->load_core_scripts)
    {
      BseErrorType error = bse_item_exec (bse_server_get(), "register-scripts", NULL);
      if (error)
        sfi_diag ("during script registration: %s", bse_error_blurb (error));
      while (!single_thread_registration_done)
        {
          g_main_context_iteration (bse_main_context, TRUE);
          // sfi_glue_gc_run ();
        }
    }
  if (as_test)
    {
      SfiCPUInfo ci = sfi_cpu_info();
      char *cname = g_strdup_printf ("%s+%s", ci.machine, bse_block_impl_name());
      treport_cpu_name (cname);
      g_free (cname);
    }
  // sfi_glue_gc_run ();
}

void
bse_init_inprocess (gint           *argc,
                    gchar        ***argv,
                    const char     *app_name,
                    SfiInitValue    values[])
{
  bse_init_intern (argc, argv, app_name, values, false);
}

void
bse_init_test (gint           *argc,
               gchar        ***argv,
               SfiInitValue    values[])
{
  bse_init_intern (argc, argv, NULL, values, true);
}

static void
bse_main_loop (gpointer data)
{
  BirnetThread *client = data;

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
core_thread_send_message_async (gpointer data)
{
  BseMessage *umsg = data;
  bse_server_send_message (bse_server_get(), umsg);
  bse_message_free (umsg);
  return FALSE;
}

/**
 * BSE log handler, suitable for sfi_msg_set_thread_handler().
 * This function is MT-safe and may be called from any thread.
 */
void
bse_msg_handler (const SfiMessage *lmsg)
{
  /* this functions is called from multiple threads */
  if (!lmsg->primary && !lmsg->secondary)
    return;
  BseMessage *umsg = bse_message_new();
  g_free (umsg->log_domain);
  umsg->log_domain = g_strdup (lmsg->log_domain);
  g_static_assert (BSE_MSG_NONE    == SFI_MSG_NONE);
  g_static_assert (BSE_MSG_FATAL   == SFI_MSG_FATAL);
  g_static_assert (BSE_MSG_ERROR   == SFI_MSG_ERROR);
  g_static_assert (BSE_MSG_WARNING == SFI_MSG_WARNING);
  g_static_assert (BSE_MSG_SCRIPT  == SFI_MSG_SCRIPT);
  g_static_assert (BSE_MSG_INFO    == SFI_MSG_INFO);
  g_static_assert (BSE_MSG_DIAG    == SFI_MSG_DIAG);
  g_static_assert (BSE_MSG_DEBUG   == SFI_MSG_DEBUG);
  umsg->type = lmsg->type;
  g_free (umsg->config_check);
  umsg->config_check = g_strdup (lmsg->config_check);
  g_free (umsg->title);
  umsg->title = g_strdup (lmsg->title);
  g_free (umsg->primary);
  umsg->primary = g_strdup (lmsg->primary);
  g_free (umsg->secondary);
  umsg->secondary = g_strdup (lmsg->secondary);
  g_free (umsg->details);
  umsg->details = g_strdup (lmsg->details);
  umsg->janitor = NULL;
  g_free (umsg->process);
  umsg->process = g_strdup (sfi_thread_get_name (NULL));
  umsg->pid = sfi_thread_get_pid (NULL);
  /* queue an idle handler in the BSE Core thread */
  bse_idle_next (core_thread_send_message_async, umsg);
}

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

static void
bse_async_parse_args (gint           *argc_p,
		      gchar        ***argv_p,
                      BseMainArgs    *margs,
                      SfiInitValue    values[])
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  
  /* this function is called before the main BSE thread is started,
   * so we can't use any BSE functions yet.
   */

  gchar *envar = getenv ("BSE_DEBUG");
  if (envar)
    sfi_msg_allow (envar);
  envar = getenv ("BSE_NO_DEBUG");
  if (envar)
    sfi_msg_deny (envar);

  guint i;
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
            sfi_msg_allow (equal + 1);
	  else if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
	      sfi_msg_allow (argv[i]);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-no-debug", argv[i]) == 0 ||
	       strncmp ("--bse-no-debug=", argv[i], 15) == 0)
	{
	  gchar *equal = argv[i] + 14;
	  if (*equal == '=')
            sfi_msg_deny (equal + 1);
	  else if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
	      sfi_msg_deny (argv[i]);
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
      else if (strcmp ("--bse-rcfile", argv[i]) == 0 && i + 1 < argc)
	{
          argv[i++] = NULL;
          g_free ((char*) margs->bse_rcfile);
          margs->bse_rcfile = g_strdup (argv[i]);
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
      else if (strcmp ("--bse-force-fpu", argv[i]) == 0)
	{
          margs->force_fpu = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-disable-randomization", argv[i]) == 0)
	{
          margs->allow_randomization = FALSE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-enable-randomization", argv[i]) == 0)
	{
          margs->allow_randomization = TRUE;
	  argv[i] = NULL;
	}
    }

  if (!margs->bse_rcfile)
    margs->bse_rcfile = g_strconcat (g_get_home_dir (), "/.bserc", NULL);

  guint e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;

  if (values)
    {
      SfiInitValue *value = values;
      while (value->value_name)
        {
          if (strcmp (value->value_name, "debug-extensions") == 0)
            margs->debug_extensions |= sfi_init_value_bool (value);
          else if (strcmp (value->value_name, "force-fpu") == 0)
            margs->force_fpu |= sfi_init_value_bool (value);
          else if (strcmp (value->value_name, "allow-randomization") == 0)
            margs->allow_randomization |= sfi_init_value_bool (value);
          else if (strcmp (value->value_name, "load-core-plugins") == 0)
            margs->load_core_plugins |= sfi_init_value_bool (value);
          else if (strcmp (value->value_name, "load-core-scripts") == 0)
            margs->load_core_scripts |= sfi_init_value_bool (value);
          else if (strcmp ("wave-chunk-padding", value->value_name) == 0)
            margs->wave_chunk_padding = sfi_init_value_int (value);
          else if (strcmp ("wave-chunk-big-pad", value->value_name) == 0)
            margs->wave_chunk_big_pad = sfi_init_value_int (value);
          else if (strcmp ("dcache-cache-memory", value->value_name) == 0)
            margs->dcache_cache_memory = sfi_init_value_int (value);
          else if (strcmp ("dcache-block-size", value->value_name) == 0)
            margs->dcache_block_size = sfi_init_value_int (value);
          else if (strcmp ("midi-kammer-note", value->value_name) == 0)
            margs->midi_kammer_note = sfi_init_value_int (value);
          else if (strcmp ("kammer-freq", value->value_name) == 0)
            margs->kammer_freq = sfi_init_value_double (value);
          value++;
        }
    }

  /* constrain (user) config */
  margs->wave_chunk_padding = MAX (1, margs->wave_chunk_padding);
  margs->wave_chunk_big_pad = MAX (2 * margs->wave_chunk_padding, margs->wave_chunk_big_pad);
  margs->dcache_block_size = MAX (2 * margs->wave_chunk_big_pad + sizeof (((GslDataCacheNode*) NULL)->data[0]), margs->dcache_block_size);
  margs->dcache_block_size = sfi_alloc_upper_power2 (margs->dcache_block_size - 1);
  /* margs->dcache_cache_memory = sfi_alloc_upper_power2 (margs->dcache_cache_memory); */

  /* non-configurable config updates */
  margs->n_processors = get_n_processors ();
}
