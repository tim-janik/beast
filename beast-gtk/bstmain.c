/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#include "bstparam.h"
#include "bstpreferences.h"
#include "topconfig.h"
#include "data/beast-images.h"
#include <unistd.h>
#include <string.h>
#include "sfi/toyprof-mem.h"


/* --- prototypes --- */
static void			bst_early_parse_args	(gint        *argc_p,
							 gchar     ***argv_p,
							 SfiRec	     *bseconfig);
static void			bst_print_blurb		(void);
static void			bst_exit_print_version	(void);


/* --- variables --- */
gboolean            bst_developer_hints = FALSE;
gboolean            bst_debug_extensions = FALSE;
gboolean            bst_main_loop_running = TRUE;
static GtkWidget   *beast_splash = NULL;
static gboolean     registration_done = FALSE;
static gboolean     arg_force_xkb = FALSE;
static gboolean     register_core_plugins = TRUE;
static gboolean     register_ladspa_plugins = TRUE;
static gboolean     register_scripts = TRUE;
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
  BstApp *app = NULL;
  SfiRec *bseconfig;
  gchar *string;
  GSource *source;
  guint i;

  /* initialize i18n */
  bindtextdomain (BST_GETTEXT_DOMAIN, BST_PATH_LOCALE);
  bind_textdomain_codeset (BST_GETTEXT_DOMAIN, "UTF-8");
  textdomain (BST_GETTEXT_DOMAIN);
  setlocale (LC_ALL, "");

  /* initialize GLib guts */
  if (0)
    toyprof_init_glib_memtable ("/tmp/beast-leak.debug", 10 /* SIGUSR1 */);
  g_thread_init (NULL);
  g_type_init ();

  /* initialize Sfi guts */
  sfi_init ();
  sfi_log_allow_info ("all");
  // sfi_log_allow_debug ("misc");
  /* ensure SFI can wake us up */
  sfi_thread_set_wakeup ((SfiThreadWakeup) g_main_context_wakeup,
			 g_main_context_default (), NULL);

  /* pre-parse BEAST args */
  bseconfig = sfi_rec_new ();
  bst_early_parse_args (&argc, &argv, bseconfig);

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
  beast_splash = bst_splash_new ("BEAST-Splash", BST_SPLASH_WIDTH, BST_SPLASH_HEIGHT, 15);
  bst_splash_set_title (beast_splash, _("BEAST Startup"));
  gtk_object_set_user_data (GTK_OBJECT (beast_splash), NULL);	/* fix for broken user_data in 2.2 */
  bst_splash_set_text (beast_splash,
		       "<b><big>BEAST</big></b>\n"
		       "<b>The Bedevilled Audio System</b>\n"
		       "<b>Version %s (%s)</b>\n",
		       BST_VERSION, BST_VERSION_HINT);
  bst_splash_update_entity (beast_splash, _("Startup"));
  bst_splash_show_grab (beast_splash);

  /* BEAST initialization
   */
  bst_splash_update_item (beast_splash, _("Objects"));
  _bst_init_utils ();
  _bst_init_params ();
  _bst_gconfig_init ();
  bst_splash_update_item (beast_splash, _("Language"));

  /* GUI patchups
   */
  gtk_rc_parse_string (bst_rc_string);

  /* parse rc file
   */
  if (TRUE)
    {
      gchar *file_name = BST_STRDUP_RC_FILE ();
      bst_splash_update_item (beast_splash, _("RC File"));
      bst_rc_parse (file_name);
      g_free (file_name);
    }

  /* show splash images
   */
  bst_splash_update_item (beast_splash, _("Splash Image"));
  string = g_strconcat (BST_PATH_IMAGES, G_DIR_SEPARATOR_S, BST_SPLASH_IMAGE, NULL);
  anim = gdk_pixbuf_animation_new_from_file (string, NULL);
  g_free (string);
  bst_splash_update ();
  if (anim)
    {
      bst_splash_set_animation (beast_splash, anim);
      g_object_unref (anim);
    }

  /* start BSE core and connect */
  bst_splash_update_item (beast_splash, _("BSE Core"));
  bse_init_async (&argc, &argv, bseconfig);
  sfi_rec_unref (bseconfig);
  sfi_glue_context_push (bse_init_glue_context ("BEAST"));
  source = g_source_simple (G_PRIORITY_HIGH,
			    (GSourcePending) sfi_glue_context_pending,
			    (GSourceDispatch) sfi_glue_context_dispatch,
			    NULL, NULL, NULL);
  g_source_attach (source, NULL);
  g_source_unref (source);

  /* watch registration notifications on server */
  bse_proxy_connect (BSE_SERVER,
		     "signal::registration", server_registration, beast_splash,
		     NULL);

  /* register core plugins */
  if (register_core_plugins)
    {
      bst_splash_update_entity (beast_splash, _("Plugins"));

      /* plugin registration, this is done asyncronously,
       * so we wait until all are done
       */
      registration_done = FALSE;
      bse_server_register_core_plugins (BSE_SERVER);
      while (!registration_done)
	{
	  GDK_THREADS_LEAVE ();
	  g_main_iteration (TRUE);
	  GDK_THREADS_ENTER ();
	  sfi_glue_gc_run ();
	}
    }

  /* register LADSPA plugins */
  if (register_ladspa_plugins)
    {
      bst_splash_update_entity (beast_splash, _("LADSPA Plugins"));

      /* plugin registration, this is done asyncronously,
       * so we wait until all are done
       */
      registration_done = FALSE;
      bse_server_register_ladspa_plugins (BSE_SERVER);
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
      bst_splash_update_entity (beast_splash, "Debugging Hook");
      g_message ("going into sleep mode due to debugging request (pid=%u)", getpid ());
      g_usleep (2147483647);
    }

  /* register BSE scripts */
  if (register_scripts)
    {
      bst_splash_update_entity (beast_splash, _("Scripts"));

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
  bst_splash_update_entity (beast_splash, _("Dialogs"));
  bst_catch_scripts_and_msgs ();
  _bst_init_gadgets ();

  /* open files given on command line
   */
  if (argc > 1)
    bst_splash_update_entity (beast_splash, _("Loading..."));
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
	      wrepo = bse_project_get_wave_repo (project);
	      error = bse_wave_repo_load_file (wrepo, argv[i]);
	      if (!error)
		{
		  app = bst_app_new (project);
		  gxk_idle_show_widget (GTK_WIDGET (app));
		  bse_item_unuse (project);
		  gtk_widget_hide (beast_splash);
		  continue;
		}
	      bse_item_unuse (project);
	    }
	  else
	    {
	      SfiProxy wrepo = bse_project_get_wave_repo (app->project);
	      
	      gxk_status_printf (GXK_STATUS_WAIT, NULL, _("Loading \"%s\""), argv[i]);
	      error = bse_wave_repo_load_file (wrepo, argv[i]);
	      bst_status_eprintf (error, _("Loading \"%s\""), argv[i]);
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
	  gtk_widget_hide (beast_splash);
	}
      bse_item_unuse (project);
      
      if (error)
	bst_status_eprintf (error, _("Loading project \"%s\""), argv[i]);
    }

  /* open default app window
   */
  if (!app)
    {
      SfiProxy project = bse_server_use_new_project (BSE_SERVER, "Untitled.bse");
      
      bse_project_get_wave_repo (project);
      app = bst_app_new (project);
      bse_item_unuse (project);
      gxk_idle_show_widget (GTK_WIDGET (app));
      gtk_widget_hide (beast_splash);
    }
  /* splash screen is definitely hidden here (still grabbing) */

  /* fire up release notes dialog
   */
  if (!BST_RC_VERSION || strcmp (BST_RC_VERSION, BST_VERSION))
    {
      bst_app_trigger_action (app, BST_ACTION_HELP_RELEASE_NOTES);
      bst_gconfig_set_rc_version (BST_VERSION);
    }

  /* release splash grab */
  gtk_widget_hide (beast_splash);
  bst_splash_release_grab (beast_splash);
  /* away into the main loop */
  while (bst_main_loop_running)
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
		      char ***argv_p,
		      SfiRec *bseconfig)
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  gchar *envar;
  guint i, e;
  
  envar = getenv ("BEAST_DEBUG");
  if (envar)
    {
      sfi_log_allow_debug (envar);
      sfi_log_allow_info (envar);
    }
  envar = getenv ("BST_DEBUG");
  if (envar)
    {
      sfi_log_allow_debug (envar);
      sfi_log_allow_info (envar);
    }

  for (i = 1; i < argc; i++)
    {
      if (strncmp (argv[i], "-:", 2) == 0)
	{
	  const gchar *flags = argv[i] + 2;
	  g_printerr ("BEAST(%s): pid = %u\n", BST_VERSION, getpid ());
	  if (strchr (flags, 'N') != NULL)
	    {
	      register_core_plugins = FALSE;
	      register_ladspa_plugins = FALSE;
	      register_scripts = FALSE;
	    }
	  if (strchr (flags, 'p'))
	    register_core_plugins = TRUE;
	  if (strchr (flags, 'P'))
	    register_core_plugins = FALSE;
	  if (strchr (flags, 'l'))
	    register_ladspa_plugins = TRUE;
	  if (strchr (flags, 'L'))
	    register_ladspa_plugins = FALSE;
	  if (strchr (flags, 's'))
	    register_scripts = TRUE;
	  if (strchr (flags, 'S'))
	    register_scripts = FALSE;
	  if (strchr (flags, 'f'))
	    {
	      GLogLevelFlags fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	      fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
	      g_log_set_always_fatal (fatal_mask);
	    }
	  if (strchr (flags, 'd') != NULL)
	    {
              bst_developer_hints = TRUE;
	      bst_debug_extensions = TRUE;
	      g_message ("enabling possibly harmfull debugging extensions due to '-:d'");
	    }
	  sfi_rec_set_bool (bseconfig, "debug-extensions", bst_debug_extensions);
	  argv[i] = NULL;
	}
      else if (strcmp ("--devel", argv[i]) == 0)
	{
	  bst_developer_hints = TRUE;
          argv[i] = NULL;
	}
      else if (strcmp ("--debug-list", argv[i]) == 0)
	{
	  const gchar **keys = _bst_log_scan_keys ();
	  guint i;
	  g_print ("debug keys: all");
	  for (i = 0; keys[i]; i++)
	    g_print (":%s", keys[i]);
	  g_print ("\n");
	  exit (0);
	  argv[i] = NULL;
	}
      else if (strcmp ("--debug", argv[i]) == 0 ||
	       strncmp ("--debug=", argv[i], 8) == 0)
	{
	  gchar *equal = argv[i] + 7;
	  
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
      else if (strcmp ("--force-xkb", argv[i]) == 0)
	{
	  arg_force_xkb = TRUE;
          argv[i] = NULL;
	}
      else if (strcmp ("-h", argv[i]) == 0 ||
	       strcmp ("--help", argv[i]) == 0)
	{
	  bst_print_blurb ();
          argv[i] = NULL;
	  exit (0);
	}
      else if (strcmp ("-v", argv[i]) == 0 ||
	       strcmp ("--version", argv[i]) == 0)
	{
	  bst_exit_print_version ();
	  argv[i] = NULL;
	  exit (0);
	}
      else if (strcmp ("--print-dir", argv[i]) == 0 ||
	       strncmp ("--print-dir=", argv[i], 13) == 0)
	{
	  gchar *arg = argv[i][13 - 1] == '=' ? argv[i] + 13 : (argv[i + 1] ? argv[i + 1] : "");
	  if (strcmp (arg, "docs") == 0)
	    g_print ("%s\n", BST_PATH_DOCS);
	  else if (strcmp (arg, "images") == 0)
	    g_print ("%s\n", BST_PATH_IMAGES);
	  else if (strcmp (arg, "locale") == 0)
	    g_print ("%s\n", BST_PATH_LOCALE);
	  else if (strcmp (arg, "ladspa") == 0)
	    g_print ("%s\n", BSE_PATH_LADSPA);
	  else if (strcmp (arg, "plugins") == 0)
	    g_print ("%s\n", BSE_PATH_PLUGINS);
	  else if (strcmp (arg, "samples") == 0)
	    g_print ("%s\n", bse_server_get_sample_path (BSE_SERVER));
	  else if (strcmp (arg, "effects") == 0)
	    g_print ("%s\n", bse_server_get_effect_path (BSE_SERVER));
	  else if (strcmp (arg, "scripts") == 0)
	    g_print ("%s\n", bse_server_get_script_path (BSE_SERVER));
	  else if (strcmp (arg, "instruments") == 0)
	    g_print ("%s\n", bse_server_get_instrument_path (BSE_SERVER));
	  else
	    {
	      if (arg[0])
                g_message ("no such resource path: %s", arg);
	      g_message ("supported resource paths: docs, images, locale, ladspa, plugins, scripts, effects, instruments, samples");
	    }
	  exit (0);
	}
    }
  gxk_param_set_devel_tips (bst_developer_hints);

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

static void G_GNUC_NORETURN
bst_exit_print_version (void)
{
  const gchar *c;
  /* hack: start BSE, so we can query it for paths, works since we immediately exit() afterwards */
  bse_init_async (NULL, NULL, NULL);
  sfi_glue_context_push (bse_init_glue_context ("BEAST"));
  g_print ("BEAST version %s (%s)\n", BST_VERSION, BST_VERSION_HINT);
  g_print ("Libraries: ");
  g_print ("GLib %u.%u.%u", glib_major_version, glib_minor_version, glib_micro_version);
  g_print (", SFI %u.%u.%u", bse_major_version, bse_minor_version, bse_micro_version);
  g_print (", BSE %u.%u.%u", bse_major_version, bse_minor_version, bse_micro_version);
  g_print (", Ogg/Vorbis");
  c = bse_server_get_mp3_version (BSE_SERVER);
  if (c)
    g_print (", %s", c);
  g_print (", GTK+ %u.%u.%u", gtk_major_version, gtk_minor_version, gtk_micro_version);
  g_print (", GdkPixbuf");
#ifdef BST_WITH_XKB
  g_print (", XKBlib");
#endif
  g_print (", GXK %s", BST_VERSION);
  g_print ("\n");
  g_print ("Compiled for: %s\n", BST_ARCH_NAME);
  g_print ("\n");
  g_print ("Locale Path:     %s\n", BST_PATH_LOCALE);
  g_print ("Doc Path:        %s\n", BST_PATH_DOCS);
  g_print ("Image Path:      %s\n", BST_PATH_IMAGES);
  g_print ("Sample Path:     %s\n", bse_server_get_sample_path (BSE_SERVER));
  g_print ("Script Path:     %s\n", bse_server_get_script_path (BSE_SERVER));
  g_print ("Effect Path:     %s\n", bse_server_get_effect_path (BSE_SERVER));
  g_print ("Instrument Path: %s\n", bse_server_get_instrument_path (BSE_SERVER));
  g_print ("Plugin Path:     %s\n", bse_server_get_plugin_path (BSE_SERVER));
  g_print ("LADSPA Path:     %s:$LADSPA_PATH\n", bse_server_get_ladspa_path (BSE_SERVER));
  g_print ("\n");
  g_print ("BEAST comes with ABSOLUTELY NO WARRANTY.\n");
  g_print ("You may redistribute copies of BEAST under the terms of\n");
  g_print ("the GNU General Public License which can be found in the\n");
  g_print ("BEAST source package. Sources, examples and contact\n");
  g_print ("information are available at http://beast.gtk.org\n");
  exit (0);
}

static void
bst_print_blurb (void)
{
  g_print ("Usage: beast [options] [files...]\n");
#ifdef BST_WITH_XKB
  g_print ("  --force-xkb             force XKB keytable queries\n");
#endif
  g_print ("  --print-dir[=RESOURCE]  print the directory for a specific resource\n");
  g_print ("  --devel                 enrich the GUI with hints usefull for developers,\n");
  g_print ("                          enable unstable plugins and experimental code\n");
  g_print ("  -h, --help              show this help message\n");
  g_print ("  -v, --version           print version and file paths\n");
  g_print ("  --display=DISPLAY       X server for the GUI; see X(1)\n");
  g_print ("Development Options:\n");
  g_print ("  --debug=KEYS            enable certain verbosity stages\n");
  g_print ("  --debug-list            list possible debug keys\n");
  g_print ("  -:[flags]               [flags] can be any of:\n");
  g_print ("                          f - fatal warnings\n");
  g_print ("                          N - disable script and plugin registration\n");
  g_print ("                          p - enable core plugin registration\n");
  g_print ("                          P - disable core plugin registration\n");
  g_print ("                          l - enable LADSPA plugin registration\n");
  g_print ("                          L - disable LADSPA plugin registration\n");
  g_print ("                          s - enable script registration\n");
  g_print ("                          S - disable script registration\n");
  g_print ("                          d - enable debugging extensions (harmfull)\n");
  if (!BST_VERSION_STABLE)
    {
      g_print ("Gtk+ Options:\n");
      g_print ("  --gtk-debug=FLAGS       Gtk+ debugging flags to enable\n");
      g_print ("  --gtk-no-debug=FLAGS    Gtk+ debugging flags to disable\n");
      g_print ("  --gtk-module=MODULE     load additional Gtk+ modules\n");
      g_print ("  --gdk-debug=FLAGS       Gdk debugging flags to enable\n");
      g_print ("  --gdk-no-debug=FLAGS    Gdk debugging flags to disable\n");
      g_print ("  --g-fatal-warnings      make warnings fatal (abort)\n");
      g_print ("  --sync                  do all X calls synchronously\n");
    }
}

void
beast_show_about_box (void)
{
  /* contributor/author names do not get i18n markup. instead, their names are to be
   * written using utf-8 encoding, where non-ascii characters are entered using
   * hexadecimal escape sequences. for instance: "Ville P\xc3\xa4tsi"
   */
  const gchar *contributors[] = {
    /* graphics */
    "Cyria Arweiler",
    "Tuomas Kuosmanen",
    "Jakub Steiner",
    "Ville P\xc3\xa4tsi",
    /* general code and fixes */
    "Sam Hocevar",
    "Alper Ersoy",
    "Ben Collver",
    "Tim Janik",
    "Stefan Westerfeld",
    /* plugins */
    "David A. Bartold",
    //"Tim Janik",
    //"Stefan Westerfeld",
    /* languages */
    "Metin Amiroff",
    "Alexandre Prokoudine",
    "Danilo Segan",
    "Christian Rose",
    "Christian Neumair",
    "Miloslav Trmac",
    "Duarte Loreto",
    "Vincent van Adrighem",
    "Danilo Segan",
    "Xavier Conde Rueda",
    "Kostas Papadimas",
    "Ismael Andres Rubio Rojas",
    NULL
  };
  if (!GTK_WIDGET_VISIBLE (beast_splash))
    {
      bst_splash_set_title (beast_splash, _("BEAST About"));
      bst_splash_update_entity (beast_splash, _("BEAST Version %s"), BST_VERSION);
      bst_splash_update_item (beast_splash, _("Contributions made by:"));
      bst_splash_animate_strings (beast_splash, contributors);
    }
  gxk_widget_showraise (beast_splash);
}
