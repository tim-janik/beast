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
#include "bsessequencer.h"
#include "bsejanitor.h"
#include "bseplugin.h"
#include "bsecategories.h"
#include "bsemidireceiver.h"
#include "gslcommon.h"
#include "gslengine.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>


/* --- prototypes --- */
static void	bse_main_loop		(gpointer	data);
static void	bse_async_parse_args	(gint	       *argc_p,
					 gchar	     ***argv_p,
					 SfiRec        *config);


/* --- variables --- */
GMainContext            *bse_main_context = NULL;
SfiMutex	         bse_main_sequencer_mutex = { 0, };
gboolean	         bse_main_debug_extensions = FALSE;
SfiThread               *bse_main_thread = NULL;
static volatile gboolean bse_initialization_stage = 0;
/* from bse.h */
const guint		 bse_major_version = BSE_MAJOR_VERSION;
const guint		 bse_minor_version = BSE_MINOR_VERSION;
const guint		 bse_micro_version = BSE_MICRO_VERSION;
const guint		 bse_interface_age = BSE_INTERFACE_AGE;
const guint		 bse_binary_age = BSE_BINARY_AGE;
const gchar		*bse_version = BSE_VERSION;


/* --- functions --- */
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
  
  /* this function is running in the user thread and needs to start the main BSE thread */
  
  /* initialize submodules */
  sfi_init ();
  /* paranoid assertions */
  g_assert (G_BYTE_ORDER == G_LITTLE_ENDIAN || G_BYTE_ORDER == G_BIG_ENDIAN);
  /* early argument handling */
  if (argc && argv)
    {
      if (*argc && !g_get_prgname ())
	g_set_prgname (**argv);
      bse_async_parse_args (argc, argv, config);
    }
  
  /* start main BSE thread */
  thread = sfi_thread_run ("BSE-CORE", bse_main_loop, sfi_thread_self ());
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

  /* initialize random numbers */
  {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    srand (tv.tv_sec ^ tv.tv_usec);
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
  /* make sure the server is alive */
  bse_server_get ();
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
      bse_async_parse_args (argc, argv, config);
    }
  
  bse_init_core ();

  /* initialize core plugins */
  if (sfi_rec_get_bool (config, "load-core-plugins"))
    {
      GSList *slist = bse_plugin_dir_list_files (BSE_PATH_PLUGINS);
      while (slist)
        {
          gchar *name = g_slist_pop_head (&slist);
          const char *error = bse_plugin_check_load (name);
          if (error)
            sfi_warn ("%s: %s", name, error);
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
  
  /* notify client about completion */
  bse_initialization_stage++;
  sfi_thread_wakeup (client);

  /* start other threads */
  bse_ssequencer_init_thread ();

  /* and away into the main loop */
  do
    {
      g_main_context_pending (bse_main_context);
      g_main_context_iteration (bse_main_context, TRUE);
    }
  while (!sfi_thread_aborted ());
}

static void
bse_async_parse_args (gint    *argc_p,
		      gchar ***argv_p,
		      SfiRec  *config)
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
    {
      sfi_log_allow_debug (envar);
      sfi_log_allow_debug (envar);
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
	  
	  if (*equal == '=')
	    {
	      sfi_log_allow_debug (equal + 1);
	      sfi_log_allow_info (equal + 1);
	    }
	  else if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
	      sfi_log_allow_debug (argv[i]);
	      sfi_log_allow_info (argv[i]);
	    }
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

  if (config && sfi_rec_get_bool (config, "debug-extensions"))
    bse_main_debug_extensions = TRUE;
}
