/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bstutils.h"
#include "bstcxxutils.h"
#include "bse/bse.h"
#include "bstapp.h"
#include "bstsplash.h"
#include "bstxkb.h"
#include "bstgconfig.h"
#include "bstkeybindings.h"
#include "bstskinconfig.h"
#include "bstmsgabsorb.h"
#include "bstusermessage.h"
#include "bstparam.h"
#include "bstpreferences.h"
#include "topconfig.h"
#include "data/beast-images.h"
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
// #include "sfi/toyprof-mem.h"


/* --- prototypes --- */
static void			bst_early_parse_args	(gint        *argc_p,
							 gchar     ***argv_p);
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
static gboolean     may_auto_update_bse_rc_file = TRUE;


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
  gchar *string;
  GSource *source;
  char debugbool[2] = "0";
  SfiInitValue config[] = {
    { "debug-extensions", debugbool },
    { NULL },
  };
  guint i;

  /* initialize i18n */
  bindtextdomain (BST_GETTEXT_DOMAIN, BST_PATH_LOCALE);
  bind_textdomain_codeset (BST_GETTEXT_DOMAIN, "UTF-8");
  textdomain (BST_GETTEXT_DOMAIN);
  setlocale (LC_ALL, "");

  /* initialize random numbers */
  struct timeval tv;
  gettimeofday (&tv, NULL);
  srand48 (tv.tv_usec + (tv.tv_sec << 16));
  srand (tv.tv_usec + (tv.tv_sec << 16));

  /* initialize GLib guts */
  // toyprof_init_glib_memtable ("/tmp/beast-leak.debug", 10 /* SIGUSR1 */);
  g_thread_init (NULL);
  g_type_init ();

  /* initialize Birnet/Sfi */
  sfi_init (&argc, &argv, _("BEAST"), NULL);  /* application name is user visible */       
  sfi_msg_allow ("misc");
  /* ensure SFI can wake us up */
  sfi_thread_set_name ("Beast GUI");
  sfi_thread_set_wakeup ((BirnetThreadWakeup) g_main_context_wakeup,
			 g_main_context_default (), NULL);

  /* initialize Gtk+ and go into threading mode */
  bst_early_parse_args (&argc, &argv);
  if (bst_debug_extensions)
    debugbool[0] = '1';
  gtk_init (&argc, &argv);
  GDK_THREADS_ENTER ();

  /* initialize Gtk+ Extension Kit */
  gxk_init ();
  /* documentation search paths */
  gxk_text_add_tsm_path (BST_PATH_DOCS);
  gxk_text_add_tsm_path (BST_PATH_IMAGES);
  gxk_text_add_tsm_path (".");

  /* now, we can popup the splash screen */
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

  /* BEAST initialization */
  bst_splash_update_item (beast_splash, _("Initializers"));
  _bst_init_utils ();
  _bst_init_params ();
  _bst_gconfig_init ();
  _bst_skin_config_init ();
  _bst_msg_absorb_config_init ();
  
  /* parse rc file */
  bst_splash_update_item (beast_splash, _("RC Files"));
  bst_preferences_load_rc_files();

  /* show splash images */
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
  bse_init_async (&argc, &argv, "BEAST", config);
  sfi_glue_context_push (bse_init_glue_context ("BEAST"));
  source = g_source_simple (GDK_PRIORITY_EVENTS, // G_PRIORITY_HIGH - 100,
			    (GSourcePending) sfi_glue_context_pending,
			    (GSourceDispatch) sfi_glue_context_dispatch,
			    NULL, NULL, NULL);
  g_source_attach (source, NULL);
  g_source_unref (source);

  /* now that the BSE thread runs, drop scheduling priorities if we have any */
  setpriority (PRIO_PROCESS, getpid(), 0);

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

  /* listen to BseServer notification */
  bst_splash_update_entity (beast_splash, _("Dialogs"));
  bst_message_connect_to_server ();
  _bst_init_radgets ();

  /* install message dialog handler */
  bst_message_handler_install();

  /* open files given on command line */
  if (argc > 1)
    bst_splash_update_entity (beast_splash, _("Loading..."));
  BstApp *app = NULL;
  gboolean merge_with_last = FALSE;
  for (i = 1; i < argc; i++)
    {
      bst_splash_update ();

      /* parse non-file args */
      if (strcmp (argv[i], "--merge") == 0)
        {
          merge_with_last = TRUE;
          continue;
        }

      /* load waves into the last project */
      if (bse_server_can_load (BSE_SERVER, argv[i]))
	{
	  if (app)
	    {
	      SfiProxy wrepo = bse_project_get_wave_repo (app->project);
	      gxk_status_printf (GXK_STATUS_WAIT, NULL, _("Loading \"%s\""), argv[i]);
	      BseErrorType error = bse_wave_repo_load_file (wrepo, argv[i]);
              bst_status_eprintf (error, _("Loading \"%s\""), argv[i]);
              if (error)
                sfi_error (_("Failed to load wave file \"%s\": %s"), argv[i], bse_error_blurb (error));
	    }
          else
	    {
	      SfiProxy project = bse_server_use_new_project (BSE_SERVER, "Untitled.bse");
	      SfiProxy wrepo = bse_project_get_wave_repo (project);
	      BseErrorType error = bse_wave_repo_load_file (wrepo, argv[i]);
	      if (!error)
		{
		  app = bst_app_new (project);
		  gxk_idle_show_widget (GTK_WIDGET (app));
		  bse_item_unuse (project);
		  gtk_widget_hide (beast_splash);
		}
              else
                {
                  bse_item_unuse (project);
                  sfi_error (_("Failed to load wave file \"%s\": %s"), argv[i], bse_error_blurb (error));
                }
	    }
          continue;
	}

      /* load/merge projects */
      if (!app || !merge_with_last)
        {
          SfiProxy project = bse_server_use_new_project (BSE_SERVER, argv[i]);
          BseErrorType error = bst_project_restore_from_file (project, argv[i], TRUE, TRUE);
          if (!error || error == BSE_ERROR_FILE_NOT_FOUND)
            {
              error = 0;
              app = bst_app_new (project);
              gxk_idle_show_widget (GTK_WIDGET (app));
              gtk_widget_hide (beast_splash);
            }
          bse_item_unuse (project);
          if (error)
            sfi_error (_("Failed to load project \"%s\": %s"), argv[i], bse_error_blurb (error));
        }
      else
        {
          BseErrorType error = bst_project_restore_from_file (app->project, argv[i], TRUE, FALSE);
          if (error)
            sfi_error (_("Failed to merge project \"%s\": %s"), argv[i], bse_error_blurb (error));
        }
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
  gboolean update_rc_files = FALSE;
  if (!BST_RC_VERSION || strcmp (BST_RC_VERSION, BST_VERSION))
    {
      const char *release_notes_title =
        "BEAST/BSE Release Notes";
      const char *release_notes_contents =
        "<tag-span-markup>"
        "<tagdef name=\"title\" weight=\"bold\" scale=\"1.5\"/>"
        "<tagdef name=\"hyperlink\" underline=\"single\" foreground=\"#0000ff\"/>"
        "<tagdef name=\"mono\" family=\"monospace\"/>"
        "<span tag=\"title\">BEAST/BSE " BST_VERSION " Release Notes</span>"
        "<newline/><newline/>"
        "The 0.7 development series of Beast focusses on improving usability and ease of music production. "
        "<newline/><newline/>"
        "Feedback is very much appreciated, please take the opportunity and provide your comments "
        "and questions in online forums like the Beast "
        "<span tag=\"hyperlink\"><xlink ref=\"http://beast.gtk.org/wiki:HelpDesk\">Help Desk</xlink></span>, Beast "
        "<span tag=\"hyperlink\"><xlink ref=\"http://bugzilla.gnome.org/buglist.cgi?query=product:beast\">Bugzilla</xlink></span> "
        "or the "
        "<span tag=\"hyperlink\"><xlink ref=\"http://beast.gtk.org/contact\">mailing list</xlink></span>, "
        "all of which can be reached through the links provided by the Help/ menu."
        "<newline/><newline/>"
        "The Beast website also provides much more information like detailed "
        "<span tag=\"hyperlink\"><xlink ref=\"http://beast.gtk.org/news-file\">release NEWS</xlink></span>, "
        "synthesis hints, documentation and a "
        "<span tag=\"hyperlink\"><xlink ref=\"http://beast.gtk.org/sound-browser\">sound archive</xlink></span>. "
        "<newline/>"
        "So either activate 'Help/Beast Website...' or go to:"
        "<newline/>"
        "<span tag=\"mono\">        </span><span tag=\"hyperlink\"><xlink ref=\"http://beast.gtk.org\">http://beast.gtk.org</xlink></span>"
        "</tag-span-markup>";
      GtkWidget *sctext = gxk_scroll_text_create (GXK_SCROLL_TEXT_WRAP | GXK_SCROLL_TEXT_SANS | GXK_SCROLL_TEXT_CENTER, NULL);
      gxk_scroll_text_set_tsm (sctext, release_notes_contents);
      GtkWidget *rndialog = gxk_dialog_new (NULL, NULL, GXK_DIALOG_DELETE_BUTTON, release_notes_title, sctext);
      gxk_dialog_set_sizes (GXK_DIALOG (rndialog), 320, 200, 540, 420);
      gxk_scroll_text_rewind (sctext);
      gxk_idle_show_widget (rndialog);
      update_rc_files = TRUE;
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
  
  /* take down GUI */
  bst_message_handler_uninstall();
  bst_message_dialogs_popdown ();
  
  /* perform necessary cleanup cycles */
  GDK_THREADS_LEAVE ();
  while (g_main_iteration (FALSE))
    {
      GDK_THREADS_ENTER ();
      sfi_glue_gc_run ();
      GDK_THREADS_LEAVE ();
    }
  GDK_THREADS_ENTER ();
  
  /* save BSE configuration */
  if (update_rc_files && !bst_preferences_saved())
    {
      if (may_auto_update_bse_rc_file)
        bse_server_save_preferences (BSE_SERVER);
      /* save BEAST configuration and accelerator map */
      gchar *file_name = BST_STRDUP_RC_FILE ();
      BseErrorType error = bst_rc_dump (file_name);
      if (error)
	g_warning ("failed to save rc-file \"%s\": %s", file_name, bse_error_blurb (error));
      g_free (file_name);
    }
  
  /* perform necessary cleanup cycles
   */
  GDK_THREADS_LEAVE ();
  while (g_main_iteration (FALSE))
    {
      GDK_THREADS_ENTER ();
      sfi_glue_gc_run ();
      GDK_THREADS_LEAVE ();
    }
  birnet_cleanup_force_handlers();

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
  
  envar = getenv ("BST_DEBUG");
  if (envar)
    sfi_msg_allow (envar);
  envar = getenv ("BST_NO_DEBUG");
  if (envar)
    sfi_msg_deny (envar);
  envar = getenv ("BEAST_DEBUG");
  if (envar)
    sfi_msg_allow (envar);
  envar = getenv ("BEAST_NO_DEBUG");
  if (envar)
    sfi_msg_deny (envar);

  gboolean initialize_bse_and_exit = FALSE;
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--") == 0)
        {
          argv[i] = NULL;
          break;
        }
      else if (strncmp (argv[i], "-:", 2) == 0)
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
	      g_message ("enabling possibly harmful debugging extensions due to '-:d'");
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--devel", argv[i]) == 0)
	{
	  bst_developer_hints = TRUE;
          argv[i] = NULL;
	}
      else if (strcmp ("--debug-list", argv[i]) == 0)
	{
	  const BstMsgID *mids = bst_message_list_types (NULL);
	  guint j;
	  g_print ("BEAST debug keys: all");
	  for (j = 0; mids[j].ident; j++)
            if (mids[j].type >= SFI_MSG_DEBUG && mids[j].label && mids[j].label[0])
              g_print (", %s (%s)", mids[j].ident, mids[j].label);
            else if (mids[j].type >= SFI_MSG_DEBUG)
              g_print (", %s", mids[j].ident);
	  g_print ("\n");
	  exit (0);
	  argv[i] = NULL;
	}
      else if (strcmp ("--debug", argv[i]) == 0 ||
	       strncmp ("--debug=", argv[i], 8) == 0)
	{
	  gchar *equal = argv[i] + 7;
	  
	  if (*equal == '=')
            sfi_msg_allow (equal + 1);
	  else if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
	      sfi_msg_allow (argv[i]);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--no-debug", argv[i]) == 0 ||
	       strncmp ("--no-debug=", argv[i], 11) == 0)
	{
	  gchar *equal = argv[i] + 10;
	  
	  if (*equal == '=')
            sfi_msg_deny (equal + 1);
	  else if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
	      sfi_msg_deny (argv[i]);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--force-xkb", argv[i]) == 0)
	{
	  arg_force_xkb = TRUE;
          argv[i] = NULL;
	}
      else if (strcmp (argv[i], "-n") == 0 && i + 1 < argc)
        { /* handled by priviledged launcher */
          argv[i++] = NULL;
          argv[i] = NULL;
        }
      else if (strncmp (argv[i], "-n=", 3) == 0 ||
               (strncmp (argv[i], "-n", 2) == 0 && argv[i][3] >= '0' && argv[i][3] <= '9') ||
               strcmp (argv[i], "-N") == 0)
        { /* handled by priviledged launcher */
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
      else if (strcmp ("--skinrc", argv[i]) == 0 ||
	       strncmp ("--skinrc=", argv[i], 9) == 0)
        {
          gchar *arg = argv[i][9 - 1] == '=' ? argv[i] + 9 : (argv[i + 1] ? argv[i + 1] : "");
          bst_skin_config_set_rcfile (arg);
        }
      else if (strcmp ("--print-dir", argv[i]) == 0 ||
	       strncmp ("--print-dir=", argv[i], 12) == 0)
	{
	  gchar *arg = argv[i][12 - 1] == '=' ? argv[i] + 12 : (argv[i + 1] ? argv[i + 1] : "");
          gchar *freeme = NULL;
	  if (strcmp (arg, "prefix") == 0)
	    g_print ("%s\n", BST_PATH_PREFIX);
	  else if (strcmp (arg, "docs") == 0)
	    g_print ("%s\n", BST_PATH_DOCS);
	  else if (strcmp (arg, "images") == 0)
	    g_print ("%s\n", BST_PATH_IMAGES);
	  else if (strcmp (arg, "locale") == 0)
	    g_print ("%s\n", BST_PATH_LOCALE);
	  else if (strcmp (arg, "skins") == 0)
	    g_print ("%s\n", freeme = BST_STRDUP_SKIN_PATH ());
	  else if (strcmp (arg, "keys") == 0)
	    g_print ("%s\n", BST_PATH_KEYS);
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
	  else if (strcmp (arg, "demo") == 0)
	    g_print ("%s\n", bse_server_get_demo_path (BSE_SERVER));
	  else
	    {
	      if (arg[0])
                g_message ("no such resource path: %s", arg);
	      g_message ("supported resource paths: prefix, docs, images, keys, locale, skins, ladspa, plugins, scripts, effects, instruments, demo, samples");
	    }
          g_free (freeme);
	  exit (0);
	}
      else if (strcmp ("--bse-latency", argv[i]) == 0 ||
               strncmp ("--bse-latency=", argv[i], 14) == 0)
        {
          gchar *equal = argv[i] + 13;
          may_auto_update_bse_rc_file = FALSE;
          if (*equal != '=')
            i++;
          /* leave args for BSE */
        }
      else if (strcmp ("--bse-mixing-freq", argv[i]) == 0 ||
               strncmp ("--bse-mixing-freq=", argv[i], 18) == 0)
        {
          gchar *equal = argv[i] + 17;
          may_auto_update_bse_rc_file = FALSE;
          if (*equal != '=')
            i++;
          /* leave args for BSE */
        }
      else if (strcmp ("--bse-control-freq", argv[i]) == 0 ||
               strncmp ("--bse-control-freq=", argv[i], 19) == 0)
        {
          gchar *equal = argv[i] + 18;
          may_auto_update_bse_rc_file = FALSE;
          if (*equal != '=')
            i++;
          /* leave args for BSE */
        }
      else if (strcmp ("--bse-driver-list", argv[i]) == 0)
        {
          initialize_bse_and_exit = TRUE;
          /* leave args for BSE */
        }
      else if (strcmp ("-p", argv[i]) == 0)
        {
          /* modify args for BSE */
          argv[i] = "--bse-pcm-driver";
        }
      else if (strcmp ("-m", argv[i]) == 0)
        {
          /* modify args for BSE */
          argv[i] = "--bse-midi-driver";
        }
    }
  gxk_param_set_devel_tips (bst_developer_hints);

  e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;

  if (initialize_bse_and_exit)
    {
      bse_init_async (argc_p, argv_p, "BEAST", NULL);
      exit (0);
    }
}

static void G_GNUC_NORETURN
bst_exit_print_version (void)
{
  const gchar *c;
  gchar *freeme = NULL;
  /* hack: start BSE, so we can query it for paths, works since we immediately exit() afterwards */
  bse_init_async (NULL, NULL, "BEAST", NULL);
  sfi_glue_context_push (bse_init_glue_context ("BEAST"));
  g_print ("BEAST version %s (%s)\n", BST_VERSION, BST_VERSION_HINT);
  g_print ("Libraries: ");
  g_print ("GLib %u.%u.%u", glib_major_version, glib_minor_version, glib_micro_version);
  g_print (", SFI %u.%u.%u", bse_major_version, bse_minor_version, bse_micro_version);
  g_print (", BSE %u.%u.%u", bse_major_version, bse_minor_version, bse_micro_version);
  c = bse_server_get_vorbis_version (BSE_SERVER);
  if (c)
    g_print (", %s", c);
  c = bse_server_get_mp3_version (BSE_SERVER);
  if (c)
    g_print (", %s", c);
  g_print (", GTK+ %u.%u.%u", gtk_major_version, gtk_minor_version, gtk_micro_version);
#ifdef BST_WITH_XKB
  g_print (", XKBlib");
#endif
  g_print (", GXK %s", BST_VERSION);
  g_print ("\n");
  g_print ("Compiled for %s %s SSE plugins.\n", BST_ARCH_NAME, BSE_WITH_SSE_FLAGS ? "with" : "without");
  g_print ("Intrinsic code selected according to runtime CPU detection:\n");
  const SfiCPUInfo cpu_info = sfi_cpu_info();
  gchar *cpu_blurb = sfi_cpu_info_string (&cpu_info);
  g_print ("%s", cpu_blurb);
  g_free (cpu_blurb);
  g_print ("\n");
  g_print ("Prefix:          %s\n", BST_PATH_PREFIX);
  g_print ("Doc Path:        %s\n", BST_PATH_DOCS);
  g_print ("Image Path:      %s\n", BST_PATH_IMAGES);
  g_print ("Locale Path:     %s\n", BST_PATH_LOCALE);
  g_print ("Keyrc Path:      %s\n", BST_PATH_KEYS);
  g_print ("Skin Path:       %s\n", freeme = BST_STRDUP_SKIN_PATH());
  g_print ("Sample Path:     %s\n", bse_server_get_sample_path (BSE_SERVER));
  g_print ("Script Path:     %s\n", bse_server_get_script_path (BSE_SERVER));
  g_print ("Effect Path:     %s\n", bse_server_get_effect_path (BSE_SERVER));
  g_print ("Instrument Path: %s\n", bse_server_get_instrument_path (BSE_SERVER));
  g_print ("Demo Path:       %s\n", bse_server_get_demo_path (BSE_SERVER));
  g_print ("Plugin Path:     %s\n", bse_server_get_plugin_path (BSE_SERVER));
  g_print ("LADSPA Path:     %s:$LADSPA_PATH\n", bse_server_get_ladspa_path (BSE_SERVER));
  g_print ("\n");
  g_print ("BEAST comes with ABSOLUTELY NO WARRANTY.\n");
  g_print ("You may redistribute copies of BEAST under the terms of\n");
  g_print ("the GNU Lesser General Public License which can be found in\n");
  g_print ("the BEAST source package. Sources, examples and contact\n");
  g_print ("information are available at http://beast.gtk.org/.\n");
  g_free (freeme);
  exit (0);
}

static void
bst_print_blurb (void)
{
  g_print ("Usage: beast [options] [files...]\n");
  /*        12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
#ifdef BST_WITH_XKB
  g_print ("  --force-xkb             force XKB keytable queries\n");
#endif
  g_print ("  --skinrc[=FILENAME]     Skin resource file name\n");
  g_print ("  --print-dir[=RESOURCE]  Print the directory for a specific resource\n");
  g_print ("  --merge                 Merge the following files into the previous project\n");
  g_print ("  --devel                 Enrich the GUI with hints useful for developers,\n");
  g_print ("                          enable unstable plugins and experimental code\n");
  g_print ("  -h, --help              Show this help message\n");
  g_print ("  -v, --version           Print version and file paths\n");
  g_print ("  -n NICELEVEL            Run with priority NICELEVEL (for suid wrapper beast)\n");
  g_print ("  -N                      Disable renicing\n");
  g_print ("  --display=DISPLAY       X server for the GUI; see X(1)\n");
  g_print ("  --bse-latency=USECONDS  Specify synthesis latency in milliseconds\n");
  g_print ("  --bse-mixing-freq=FREQ  Specify synthesis mixing frequency in Hz \n");
  g_print ("  --bse-control-freq=FREQ Specify control frequency in Hz\n");
  g_print ("  --bse-force-fpu         Disable loading of SSE or similarly optimized plugins\n");
  g_print ("  --bse-pcm-driver DRIVERCONF\n");
  g_print ("  -p DRIVERCONF           Try to use the PCM driver DRIVERCONF, multiple\n");
  g_print ("                          options may be supplied to try a variety of\n");
  g_print ("                          drivers. Uunless -p auto is given, only the\n");
  g_print ("                          drivers listed by -p options are used; each\n");
  g_print ("                          DRIVERCONF consists of a driver name and may be\n");
  g_print ("                          assigned an optional comma seperated list of\n");
  g_print ("                          arguments, e.g.: -p oss=/dev/dsp2,rw\n");
  g_print ("  --bse-midi-driver DRIVERCONF\n");
  g_print ("  -m DRIVERCONF           Try to use the MIDI driver DRIVERCONF, multiple\n");
  g_print ("                          options may be specified similarly to the\n");
  g_print ("                          option handling for --bse-pcm-driver\n");
  g_print ("  --bse-driver-list       List available PCM and MIDI drivers\n");
  g_print ("Development Options:\n");
  g_print ("  --debug=KEYS            Enable specific debugging messages\n");
  g_print ("  --no-debug=KEYS         Disable specific debugging messages\n");
  g_print ("  --debug-list            List possible debug keys\n");
  g_print ("  -:[Flags]               [Flags] can be any combination of:\n");
  g_print ("                          f - fatal warnings\n");
  g_print ("                          N - disable script and plugin registration\n");
  g_print ("                          p - enable core plugin registration\n");
  g_print ("                          P - disable core plugin registration\n");
  g_print ("                          l - enable LADSPA plugin registration\n");
  g_print ("                          L - disable LADSPA plugin registration\n");
  g_print ("                          s - enable script registration\n");
  g_print ("                          S - disable script registration\n");
  g_print ("                          d - enable debugging extensions (harmfull)\n");
  g_print ("Gtk+ Options:\n");
  g_print ("  --gtk-debug=FLAGS       Gtk+ debugging flags to enable\n");
  g_print ("  --gtk-no-debug=FLAGS    Gtk+ debugging flags to disable\n");
  g_print ("  --gtk-module=MODULE     Load additional Gtk+ modules\n");
  g_print ("  --gdk-debug=FLAGS       Gdk debugging flags to enable\n");
  g_print ("  --gdk-no-debug=FLAGS    Gdk debugging flags to disable\n");
  g_print ("  --g-fatal-warnings      Make warnings fatal (abort)\n");
  g_print ("  --sync                  Do all X calls synchronously\n");
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
    //"Artem Popov",
    "Cyria Arweiler",
    "Jakub Steiner",
    "Tuomas Kuosmanen",
    "Ville P\xc3\xa4tsi",
    /* general code and fixes */
    "Alper Ersoy",
    "Ben Collver",
    "Hanno Behrens",
    "Nedko Arnaudov",
    "Rasmus Kaj",
    "Sam Hocevar",
    "Stefan Westerfeld",
    "Sven Herzberg",
    "Tim Janik",
    /* plugins */
    "\xd0\x90\xd1\x80\xd1\x82\xd0\xb5\xd0\xbc \xd0\x9f\xd0\xbe\xd0\xbf\xd0\xbe\xd0\xb2", /* "Artem Popov", */
    "David A. Bartold",
    //"Tim Janik",
    //"Stefan Westerfeld",
    /* translations */
    "Adam Weinberger",
    "Alexandre Prokoudine",
    "Amanpreet Singh Alam",
    //"Artem Popov",
    "Christian Neumair",
    "Christian Rose",
    "Christophe Merlet",
    "Daniel Nylander",
    "Danilo Segan",
    "David Lodge",
    "Dirk Janik",
    "Duarte Loreto",
    "Dulmandakh Sukhbaatar",
    "Francisco Javier F. Serrador",
    "Gareth Owen",
    "Gil Forcada",
    "Hendrik Richter",
    "Hizkuntza Politikarako Sailburuordetza",
    "Iassen Pramatarov",
    // "I\xc3\261aki Larra\xc3\261aga Murgotio",
    "Ismael Andres Rubio Rojas",
    "Jens Seidel",
    "Jorge Gonzalez",
    "Kees van den Broek",
    "Kostas Papadimas",
    "Laurent Dhima",
    "Metin Amiroff",
    "Miloslav Trmac",
    "Moritz Mekelburger",
    "Pawan Chitrakar",
    "Petrecca Michele",
    "Raphael Higino",
    "Robert Sedak",
    "Satoru SATOH",
    "Steve Murphy",
    "Tino Meinen",
    "Vincent van Adrighem",
    "Xavier Conde Rueda",
    "Yelitza Louze",
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
