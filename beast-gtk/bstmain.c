/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstutils.h"
#include "bse/bse.h"
#include "bstapp.h"
#include "bstsplash.h"
#include "bstxkb.h"
#include "bstgconfig.h"
#include "bstusermessage.h"
#include "bstpreferences.h"
#include "../PKG_config.h"
#include "images/beast-images.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "sfi/toyprof-mem.h"


/* --- prototypes --- */
static void			bst_early_parse_args	(gint        *argc_p,
							 gchar     ***argv_p);
static void			bst_print_blurb		(FILE	     *fout,
							 gboolean     print_help);


/* --- variables --- */
BstDebugFlags       bst_debug_flags = 0;
gboolean            beast_main_loop = TRUE;
gboolean            bst_dvl_hints = FALSE;
gboolean            registration_done = FALSE;
static GDebugKey    bst_debug_keys[] = { /* keep in sync with bstdefs.h */
  { "keytable",		BST_DEBUG_KEYTABLE, },
  { "samples",		BST_DEBUG_SAMPLES, },
};
static const guint  bst_n_debug_keys = sizeof (bst_debug_keys) / sizeof (bst_debug_keys[0]);
static gboolean     arg_force_xkb = FALSE;
static gboolean     bst_load_plugins = TRUE;
static const gchar *bst_rc_string =
( "style'BstTooltips-style'"
  "{"
  "bg[NORMAL]={.94,.88,0.}"
  "}"
  "widget'gtk-tooltips'style'BstTooltips-style'"
  "\n");


/* --- functions --- */
static void
server_registration (SfiProxy     server,
		     SfiChoice    rchoice,
		     const gchar *what,
		     const gchar *error,
		     gpointer     data)
{
  BseRegistrationType rtype = bse_registration_type_from_choice (rchoice);

  if (rtype == BSE_REGISTER_DONE)
    registration_done = TRUE;
  else
    {
      gchar *base = strrchr (what, '/');
      bst_splash_update_item (data, "%s", base ? base + 1 : what);
      if (error && error[0])
	g_message ("failed to register \"%s\": %s", what, error);
    }
}

int
main (int   argc,
      char *argv[])
{
  GdkPixbufAnimation *anim;
  GtkWidget *splash;
  BstApp *app = NULL;
  gchar *string;
  GSource *source;
  guint i;

  /* GLib's thread and object systems
   */
  if (toyprof_mem_table && 0)
    g_mem_set_vtable (toyprof_mem_table);
  g_thread_init (NULL);
  g_type_init ();

  /* pre-parse BEAST args
   */
  bst_early_parse_args (&argc, &argv);
  g_message ("BEAST: pid = %u", getpid ());

  /* initialize Gtk+ and go into threading mode
   */
  gtk_init (&argc, &argv);
  g_set_prgname ("BEAST");	/* overriding Gdk's program name */
  GDK_THREADS_ENTER ();

  /* initialize Gtk+ Extension Kit
   */
  gxk_init ();
  /* documentation search paths */
  gxk_text_add_tsm_path (BST_PATH_DOCS);
  gxk_text_add_tsm_path (BST_PATH_IMAGES);
  gxk_text_add_tsm_path (".");

  /* now, we can popup the splash screen
   */
  splash = bst_splash_new ("BEAST Startup", BST_SPLASH_WIDTH, BST_SPLASH_HEIGHT, 15);
  bst_splash_set_text (splash,
		       "<b><big>BEAST</big></b>\n"
		       "<b>The Bedevilled Audio System</b>\n"
		       "<b>Version %s (%s)</b>\n",
		       BST_VERSION, BST_VERSION_HINT);
  bst_splash_update_entity (splash, "Startup");
  bst_splash_show_now (splash);

  /* initialize Sfi types
   */
  sfi_init ();

  /* ensure SFI can wake us up */
  sfi_thread_set_wakeup ((SfiThreadWakeup) g_main_context_wakeup,
			 g_main_context_default (), NULL);

  /* BEAST initialization
   */
  bst_splash_update_item (splash, "Objects");
  _bst_init_utils ();
  _bst_init_params ();
  _bst_gconfig_init ();

  /* GUI patchups
   */
  gtk_rc_parse_string (bst_rc_string);
  g_type_name (bst_free_radio_button_get_type ());	/* urg, GCC_CONST */

  /* parse rc file
   */
  if (TRUE)
    {
      gchar *file_name = BST_STRDUP_RC_FILE ();
      bst_splash_update_item (splash, "RC File");
      bst_rc_parse (file_name);
      g_free (file_name);
    }

  /* show splash images
   */
  bst_splash_update_item (splash, "Splash Image");
  string = g_strconcat (BST_PATH_IMAGES, G_DIR_SEPARATOR_S, BST_SPLASH_IMAGE, NULL);
  anim = gdk_pixbuf_animation_new_from_file (string, NULL);
  g_free (string);
  bst_splash_update ();
  if (anim)
    {
      bst_splash_set_animation (splash, anim);
      g_object_unref (anim);
    }

  /* start BSE core and connect */
  bst_splash_update_item (splash, "BSE Core");
  bse_init_async (&argc, &argv, NULL);
  sfi_glue_context_push (bse_init_glue_context ("BEAST"));
  source = g_source_simple (G_PRIORITY_HIGH,
			    (GSourcePending) sfi_glue_context_pending,
			    (GSourceDispatch) sfi_glue_context_dispatch,
			    NULL, NULL, NULL);
  g_source_attach (source, NULL);
  g_source_unref (source);

  /* watch registration notifications on server */
  bse_proxy_connect (BSE_SERVER,
		     "signal::registration", server_registration, splash,
		     NULL);

  /* register dynamic types and modules (plugins) */
  if (bst_load_plugins)
    {
      bst_splash_update_entity (splash, "Plugins");

      /* plugin registration, this is done asyncronously,
       * so we wait until all are done
       */
      registration_done = FALSE;
      bse_server_register_plugins (BSE_SERVER);
      while (!registration_done)
	{
	  GDK_THREADS_LEAVE ();
	  g_main_iteration (TRUE);
	  GDK_THREADS_ENTER ();
	  sfi_glue_gc_run ();
	}
    }

  /* debugging hook */
  string = g_getenv ("BEAST_SLEEP4GDB");
  if (string && atoi (string) > 0)
    {
      bst_splash_update_entity (splash, "Debugging Hook");
      g_message ("going into sleep mode due to debugging request (pid=%u)", getpid ());
      g_usleep (2147483647);
    }

  /* register BSE scripts */
  if (bst_load_plugins)
    {
      bst_splash_update_entity (splash, "Scripts");

      /* script registration, this is done asyncronously,
       * so we wait until all are done
       */
      registration_done = FALSE;
      bse_server_register_scripts (BSE_SERVER);
      while (!registration_done)
	{
	  GDK_THREADS_LEAVE ();
	  g_main_iteration (TRUE);
	  GDK_THREADS_ENTER ();
	  sfi_glue_gc_run ();
	}
    }

  /* listen to BseServer notification
   */
  bst_splash_update_entity (splash, "Dialogs");
  bst_catch_scripts_and_msgs ();

  /* grab events on the splash to keep the user away
   * from application windows during loading.
   * we hide the splash as soon as a real app window appears.
   */
  gtk_grab_add (splash);

  /* open files given on command line
   */
  if (argc > 1)
    bst_splash_update_entity (splash, "Loading...");
  for (i = 1; i < argc; i++)
    {
      SfiProxy project, wrepo;
      BseErrorType error;

      bst_splash_update ();

      /* load waves into the last project */
      if (bse_server_can_load (BSE_SERVER, argv[i]))
	{
	  if (!app)
	    {
	      project = bse_server_use_new_project (BSE_SERVER, "Untitled.bse");
	      wrepo = bse_project_ensure_wave_repo (project);
	      error = bse_wave_repo_load_file (wrepo, argv[i]);
	      if (!error)
		{
		  app = bst_app_new (project);
		  gxk_idle_show_widget (GTK_WIDGET (app));
		  bse_item_unuse (project);
		  gtk_widget_hide (splash);
		  continue;
		}
	      bse_item_unuse (project);
	    }
	  else
	    {
	      SfiProxy wrepo = bse_project_ensure_wave_repo (app->project);
	      
	      gxk_status_printf (GXK_STATUS_WAIT, NULL, "Loading \"%s\"", argv[i]);
	      error = bse_wave_repo_load_file (wrepo, argv[i]);
	      bst_status_eprintf (error, "Loading \"%s\"", argv[i]);
	      if (!error)
		continue;
	    }
	}
      project = bse_server_use_new_project (BSE_SERVER, argv[i]);
      error = bse_project_restore_from_file (project, argv[i]);
      
      if (!error)
	{
	  app = bst_app_new (project);
	  gxk_idle_show_widget (GTK_WIDGET (app));
	  gtk_widget_hide (splash);
	}
      bse_item_unuse (project);
      
      if (error)
	bst_status_eprintf (error, "Loading project \"%s\"", argv[i]);
    }

  /* open default app window
   */
  if (!app)
    {
      SfiProxy project = bse_server_use_new_project (BSE_SERVER, "Untitled.bse");
      
      bse_project_ensure_wave_repo (project);
      app = bst_app_new (project);
      bse_item_unuse (project);
      gxk_idle_show_widget (GTK_WIDGET (app));
      gtk_widget_hide (splash);
    }
  /* splash screen is definitely hidden here (still grabbing) */

  /* fire up release notes dialog
   */
  if (!BST_RC_VERSION || strcmp (BST_RC_VERSION, BST_VERSION))
    {
      bst_app_operate (app, BST_OP_HELP_RELEASE_NOTES);
      bst_gconfig_set_rc_version (BST_VERSION);
    }

  /* destroy splash to release grab */
  gtk_widget_destroy (splash);
  /* away into the main loop */
  while (beast_main_loop)
    {
      sfi_glue_gc_run ();
      GDK_THREADS_LEAVE ();
      g_main_iteration (TRUE);
      GDK_THREADS_ENTER ();
    }
  
  /* stop everything playing
   */
  // bse_heart_reset_all_attach ();
  
  /* take down GUI leftovers
   */
  bst_user_messages_kill ();
  
  /* perform necessary cleanup cycles
   */
  GDK_THREADS_LEAVE ();
  while (g_main_iteration (FALSE))
    {
      GDK_THREADS_ENTER ();
      sfi_glue_gc_run ();
      GDK_THREADS_LEAVE ();
    }
  GDK_THREADS_ENTER ();
  
  /* save rc file
   */
  if (TRUE)
    {
      gchar *file_name = BST_STRDUP_RC_FILE ();
      BseErrorType error = bst_rc_dump (file_name);
      bse_server_save_preferences (BSE_SERVER);
      if (error)
	g_warning ("failed to save rc-file \"%s\": %s", file_name, bse_error_blurb (error));
      g_free (file_name);
    }
  
  /* remove pcm devices
   */
  // bse_heart_unregister_all_devices ();
  
  /* perform necessary cleanup cycles
   */
  GDK_THREADS_LEAVE ();
  while (g_main_iteration (FALSE))
    {
      GDK_THREADS_ENTER ();
      sfi_glue_gc_run ();
      GDK_THREADS_LEAVE ();
    }

  bse_object_debug_leaks ();
  
  return 0;
}

static void
bst_early_parse_args (int    *argc_p,
		      char ***argv_p)
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  gchar *envar;
  guint i, e;
  
  envar = getenv ("BEAST_DEBUG");
  if (envar)
    bst_debug_flags |= g_parse_debug_string (envar, bst_debug_keys, bst_n_debug_keys);
  envar = getenv ("BST_DEBUG");
  if (envar)
    bst_debug_flags |= g_parse_debug_string (envar, bst_debug_keys, bst_n_debug_keys);
  envar = getenv ("BEAST_NO_DEBUG");
  if (envar)
    bst_debug_flags &= ~g_parse_debug_string (envar, bst_debug_keys, bst_n_debug_keys);
  envar = getenv ("BST_NO_DEBUG");
  if (envar)
    bst_debug_flags &= ~g_parse_debug_string (envar, bst_debug_keys, bst_n_debug_keys);
  
  for (i = 1; i < argc; i++)
    {
      if (strncmp (argv[i], "-!", 2) == 0)
	{
	  // bse_developer_extensions = TRUE;
	  if (strchr (argv[i], 'p'))
	    bst_load_plugins = TRUE;
	  else
	    bst_load_plugins = FALSE;
	  if (strchr (argv[i], 'g'))
	    {
	      GLogLevelFlags fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	      fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
	      g_log_set_always_fatal (fatal_mask);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--beast-debug", argv[i]) == 0 ||
	  strncmp ("--beast-debug=", argv[i], 14) == 0)
	{
	  gchar *equal = argv[i] + 13;
	  
	  if (*equal == '=')
	    bst_debug_flags |= g_parse_debug_string (equal + 1, bst_debug_keys, bst_n_debug_keys);
	  else if (i + 1 < argc)
	    {
	      bst_debug_flags |= g_parse_debug_string (argv[i + 1],
						       bst_debug_keys,
						       bst_n_debug_keys);
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bst-debug", argv[i]) == 0 ||
	       strncmp ("--bst-debug=", argv[i], 12) == 0)
	{
	  gchar *equal = argv[i] + 11;
	  
	  if (*equal == '=')
	    bst_debug_flags |= g_parse_debug_string (equal + 1, bst_debug_keys, bst_n_debug_keys);
	  else if (i + 1 < argc)
	    {
	      bst_debug_flags |= g_parse_debug_string (argv[i + 1],
						       bst_debug_keys,
						       bst_n_debug_keys);
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--beast-no-debug", argv[i]) == 0 ||
	       strncmp ("--beast-no-debug=", argv[i], 17) == 0)
	{
	  gchar *equal = argv[i] + 16;
	  
	  if (*equal == '=')
	    bst_debug_flags &= ~g_parse_debug_string (equal + 1, bst_debug_keys, bst_n_debug_keys);
	  else if (i + 1 < argc)
	    {
	      bst_debug_flags &= ~g_parse_debug_string (argv[i + 1],
							bst_debug_keys,
							bst_n_debug_keys);
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bst-no-debug", argv[i]) == 0 ||
	       strncmp ("--bst-no-debug=", argv[i], 15) == 0)
	{
	  gchar *equal = argv[i] + 14;
	  
	  if (*equal == '=')
	    bst_debug_flags &= ~g_parse_debug_string (equal + 1, bst_debug_keys, bst_n_debug_keys);
	  else if (i + 1 < argc)
	    {
	      bst_debug_flags &= ~g_parse_debug_string (argv[i + 1],
							bst_debug_keys,
							bst_n_debug_keys);
	      argv[i] = NULL;
	      i += 1;
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--hints", argv[i]) == 0)
	{
	  bst_dvl_hints = TRUE;
          argv[i] = NULL;
	}
      else if (strcmp ("--force-xkb", argv[i]) == 0)
	{
	  arg_force_xkb = TRUE;
          argv[i] = NULL;
	}
      else if (strcmp ("-h", argv[i]) == 0 ||
	       strcmp ("--help", argv[i]) == 0)
	{
	  bst_print_blurb (stdout, TRUE);
          argv[i] = NULL;
	  exit (0);
	}
      else if (strcmp ("-v", argv[i]) == 0 ||
	       strcmp ("--version", argv[i]) == 0)
	{
	  bst_print_blurb (stdout, FALSE);
	  argv[i] = NULL;
	  exit (0);
	}
      else if (strcmp ("--print-path", argv[i]) == 0 ||
	       strncmp ("--print-path=", argv[i], 13) == 0)
	{
	  gchar *arg = argv[i][13 - 1] == '=' ? argv[i] + 13 : (argv[i + 1] ? argv[i + 1] : "");
	  if (strcmp (arg, "docs") == 0)
	    fprintf (stdout, "%s\n", BST_PATH_DOCS);
	  else if (strcmp (arg, "images") == 0)
	    fprintf (stdout, "%s\n", BST_PATH_IMAGES);
	  else if (strcmp (arg, "plugins") == 0)
	    fprintf (stdout, "%s\n", BSE_PATH_PLUGINS);
	  else if (strcmp (arg, "scripts") == 0)
	    fprintf (stdout, "%s\n", BSW_PATH_SCRIPTS);
	  else if (strcmp (arg, "samples") == 0)
	    fprintf (stdout, "%s\n", BST_PATH_DATA_SAMPLES);
	  else
	    g_message ("no such resource path: %s", arg);
	  exit (0);
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
}

static void
bst_print_blurb (FILE    *fout,
		 gboolean print_help)
{
  if (!print_help)
    {
      fprintf (fout, "BEAST version %s (%s)\n", BST_VERSION, BST_VERSION_HINT);
      fprintf (fout, "Libraries: ");
      fprintf (fout, "BSE %u.%u.%u", bse_major_version, bse_minor_version, bse_micro_version);
      fprintf (fout, ", GTK+ %u.%u.%u", gtk_major_version, gtk_minor_version, gtk_micro_version);
      fprintf (fout, ", GLib %u.%u.%u", glib_major_version, glib_minor_version, glib_micro_version);
#ifdef BST_WITH_GDK_PIXBUF
      fprintf (fout, ", GdkPixbuf");
#endif
#ifdef BST_WITH_XKB
      fprintf (fout, ", XKBlib");
#endif
      fprintf (fout, "\n");
      fprintf (fout, "Compiled for: %s\n", BST_ARCH_NAME);
      fprintf (fout, "\n");
      fprintf (fout, "Doc Path:    %s\n", BST_PATH_DOCS);
      fprintf (fout, "Image Path:  %s\n", BST_PATH_IMAGES);
      fprintf (fout, "Plugin Path: %s\n", BSE_PATH_PLUGINS);
      fprintf (fout, "Script Path: %s\n", BSW_PATH_SCRIPTS);
      fprintf (fout, "Sample Path: %s\n", BST_PATH_DATA_SAMPLES);
      fprintf (fout, "\n");
      fprintf (fout, "BEAST comes with ABSOLUTELY NO WARRANTY.\n");
      fprintf (fout, "You may redistribute copies of BEAST under the terms of\n");
      fprintf (fout, "the GNU General Public License which can be found in the\n");
      fprintf (fout, "BEAST source package. Sources, examples and contact\n");
      fprintf (fout, "information are available at http://beast.gtk.org\n");
    }
  else
    {
      fprintf (fout, "Usage: beast [options] [files...]\n");
      fprintf (fout, "  --hints                         enrich the GUI with hints usefull for (script) developers\n");
      fprintf (fout, "  --force-xkb                     force XKB keytable queries\n");
      fprintf (fout, "  --beast-debug=keys              enable certain BEAST debug stages\n");
      fprintf (fout, "  --beast-no-debug=keys           disable certain BEAST debug stages\n");
      fprintf (fout, "  --bse-debug=keys                enable certain BSE debug stages\n");
      fprintf (fout, "  --bse-no-debug=keys             disable certain BSE debug stages\n");
      fprintf (fout, "  --print-path=resource           print the file path for a specific resource\n");
      fprintf (fout, "  -h, --help                      show this help message\n");
      fprintf (fout, "  -v, --version                   print version and file paths\n");
      fprintf (fout, "  --display=DISPLAY               X server for the GUI; see X(1)\n");
      if (!BST_VERSION_STABLE)
        {
          fprintf (fout, "  --gtk-debug=FLAGS               Gtk+ debugging flags to enable\n");
          fprintf (fout, "  --gtk-no-debug=FLAGS            Gtk+ debugging flags to disable\n");
          fprintf (fout, "  --gtk-module=MODULE             load additional Gtk+ modules\n");
          fprintf (fout, "  --gdk-debug=FLAGS               Gdk debugging flags to enable\n");
          fprintf (fout, "  --gdk-no-debug=FLAGS            Gdk debugging flags to disable\n");
          fprintf (fout, "  --g-fatal-warnings              make warnings fatal (abort)\n");
          fprintf (fout, "  --sync                          do all X calls synchronously\n");
        }
    }
}
