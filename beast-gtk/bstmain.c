/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include        "bstdefs.h"

#include        "bstapp.h"
#include        "bstsamplerepo.h"
#include        "bstprocedure.h"
#include	"bstxkb.h"
#include	"bstkeytables.h"
#include	"bstmenus.h"
#include	"bstgconfig.h"
#include	"bstpreferences.h"
#include	"../PKG_config.h"




#define	PROGRAM	"BEAST"
#define	TITLE	"Beast"
#define	VERSION	"Pre-Alpha"


/* --- prototypes --- */
static void			bst_parse_args		(gint        *argc_p,
							 gchar     ***argv_p);
static BstKeyTablePatch*	bst_key_table_from_xkb	(const gchar *display);


/* --- variables --- */
static guint        args_changed_signal_id = 0;
BstDebugFlags       bst_debug_flags = 0;
static GDebugKey       bst_debug_keys[] = { /* keep in sync with bstdefs.h */
  { "keytable",		BST_DEBUG_KEYTABLE, },
  { "samples",		BST_DEBUG_SAMPLES, },
};
static const guint bst_n_debug_keys = sizeof (bst_debug_keys) / sizeof (bst_debug_keys[0]);
static const gchar *bst_rc_string =
( "style'BstTooltips-style'"
  "{"
  "bg[NORMAL]={.94,.88,0.}"
  "}"
  "widget'gtk-tooltips'style'BstTooltips-style'"
  "\n");


/* --- functions --- */
int
main (int   argc,
      char *argv[])
{
  BsePcmDevice *pdev = NULL;
  BstApp *app = NULL;
  guint i;
  
  g_message ("BEAST: pid = %u", getpid ());
  
  /* initialize BSE and preferences
   */
  bse_init (&argc, &argv);
  bst_globals_init ();
  
  /* initialize GUI libraries and pre-parse args
   */
  gtk_init (&argc, &argv);
  gtk_rc_parse_string (bst_rc_string);
  gdk_rgb_init ();
  gnome_type_init ();
  // gnome_init (PROGRAM, VERSION, argc, argv);
  bst_free_radio_button_get_type ();
  bst_parse_args (&argc, &argv);
  
  /* parse rc file */
  if (1)
    {
      BseGConfig *gconf;
      BseErrorType error;
      gchar *file_name;

      file_name = BST_STRDUP_RC_FILE ();
      gconf = bse_object_new (BST_TYPE_GCONFIG, NULL);
      bse_gconfig_revert (gconf);
      error = bst_rc_parse (file_name, gconf);
      if (error != BSE_ERROR_FILE_NOT_FOUND)
	{
	  bse_gconfig_apply (gconf);
	  if (error)
	    g_warning ("error parsing rc-file \"%s\": %s", file_name, bse_error_blurb (error));
	}
      bse_object_unref (BSE_OBJECT (gconf));
      g_free (file_name);
    }

  /* setup default keytable for pattern editor class
   */
  bst_key_table_install_patch (bst_key_table_from_xkb (gdk_get_display ()));


  /* check load BSE plugins to register types
   */
  if (1)
    {
      GList *free_list, *list;
      
      free_list = bse_plugin_dir_list_files (BSE_PATH_PLUGINS);
      for (list = free_list; list; list = list->next)
	{
	  gchar *error, *string = list->data;
	  
	  g_message ("loading plugin \"%s\"...", string);
	  error = bse_plugin_check_load (string);
	  if (error)
	    g_warning ("error encountered loading plugin \"%s\": %s", string, error);
	  g_free (string);
	}
      g_list_free (free_list);
      if (!free_list)
	g_warning ("strange, can't find any plugins, please check %s", BSE_PATH_PLUGINS);
    }
  
  
  /* hackery rulez!
   */
  args_changed_signal_id =
    gtk_object_class_user_signal_new (gtk_type_class (GTK_TYPE_OBJECT),
				      "args_changed",
				      GTK_RUN_ACTION,
				      gtk_signal_default_marshaller,
				      GTK_TYPE_NONE, 0);
  
  
  /* setup PCM Devices
   */
  if (!pdev && BSE_TYPE_ID (BsePcmDeviceAlsa) && !BST_DISABLE_ALSA)
    {
      BseErrorType error;

      pdev = (BsePcmDevice*) bse_object_new (BSE_TYPE_ID (BsePcmDeviceAlsa), NULL); /* FIXME: TYPE_ID */
      error = bse_pcm_device_update_caps (pdev);
      if (error && error != BSE_ERROR_DEVICE_BUSY)
	{
	  bse_object_unref (BSE_OBJECT (pdev));
	  pdev = NULL;
	}
    }
  if (!pdev && BSE_TYPE_ID (BsePcmDeviceOSS))
    {
      BseErrorType error;

      pdev = (BsePcmDevice*) bse_object_new (BSE_TYPE_ID (BsePcmDeviceOSS), NULL); /* FIXME: TYPE_ID */
      error = bse_pcm_device_update_caps (pdev);
      if (error && error != BSE_ERROR_DEVICE_BUSY)
	{
	  bse_object_unref (BSE_OBJECT (pdev));
	  pdev = NULL;
	}
    }
  if (!pdev)
    g_error ("No PCM device driver known");
  bse_heart_register_device ("Master", pdev);
  bse_object_unref (BSE_OBJECT (pdev));
  bse_heart_set_default_odevice ("Master");
  bse_heart_set_default_idevice ("Master");


  /* register sample repositories
   */
  bst_sample_repo_init ();
  
  
  /* open files given on command line
   */
  for (i = 1; i < argc; i++)
    {
      BseStorage *storage = bse_storage_new ();
      BseErrorType error;
      
      error = bse_storage_input_file (storage, argv[i]);
      
      if (!error)
	{
	  BseProject *project = bse_project_new (argv[i]);
	  
	  bse_storage_set_path_resolver (storage, bse_project_path_resolver, project);
	  error = bse_project_restore (project, storage);
	  if (!error)
	    {
	      app = bst_app_new (project);
	      gtk_idle_show_widget (GTK_WIDGET (app));
	    }
	  bse_object_unref (BSE_OBJECT (project));
	}
      bse_storage_destroy (storage);
      if (error)
	g_warning ("failed to load project `%s': %s", /* FIXME */
		   argv[i],
		   bse_error_blurb (error));
    }
  
  /* open default app window
   */
  if (!app)
    {
      BseProject *project = bse_project_new ("Untitled.bse");

      app = bst_app_new (project);
      bse_object_unref (BSE_OBJECT (project));
      /* bst_app_operate (app, BST_OP_PROJECT_NEW_SONG); */
      gtk_idle_show_widget (GTK_WIDGET (app));
    }
  
  
  
  /* and away into the main loop
   */
  gtk_main ();

  /* stop everything playing */
  bse_heart_reset_all_attach ();

  /* FXME: wrt cleanup cycles */

  /* perform necessary cleanup cycles */
  while (g_main_iteration (FALSE))
    ;

  /* save rc file */
  if (1)
    {
      BseGConfig *gconf;
      BseErrorType error;
      gchar *file_name;

      gconf = bse_object_new (BST_TYPE_GCONFIG, NULL);
      bse_gconfig_revert (gconf);
      file_name = BST_STRDUP_RC_FILE ();
      error = bst_rc_dump (file_name, gconf);
      bse_object_unref (BSE_OBJECT (gconf));
      if (error)
	g_warning ("error saving rc-file \"%s\": %s", file_name, bse_error_blurb (error));
      g_free (file_name);
    }

  /* remove pcm devices */
  bse_heart_unregister_all_devices ();

  /* perform necessary cleanup cycles */
  while (g_main_iteration (FALSE))
    ;
  
  return 0;
}

static void
bst_parse_args (int    *argc_p,
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
      if (strcmp ("--beast-debug", argv[i]) == 0 ||
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

static BstKeyTablePatch*
bst_key_table_from_xkb_symbol (const gchar *xkb_symbol)
{
  gchar *encoding, *layout, *model, *variant;
  GSList *slist, *name_list = NULL;
  BstKeyTablePatch *patch = NULL;
  
  bst_xkb_parse_symbol (xkb_symbol, &encoding, &layout, &model, &variant);
  BST_IF_DEBUG (KEYTABLE)
    g_message ("keytable %s: encoding(%s) layout(%s) model(%s) variant(%s)",
	       xkb_symbol, encoding, layout, model, variant);
  
  /* strip number of keys (if present) */
  if (layout)
    {
      gchar *n, *l = layout;
      
      while (*l && (*l < '0' || *l > '9'))
	l++;
      n = l;
      while (*n >= '0' && *n <= '9')
	n++;
      *n = 0;
      n = layout;
      layout = *l ? g_strdup (l) : NULL;
      g_free (n);
    }
  
  /* list guesses */
  if (encoding)
    {
      name_list = g_slist_prepend (name_list, g_strdup (encoding));
      if (layout)
	name_list = g_slist_prepend (name_list,
				     g_strdup_printf ("%s-%s",
						      encoding,
						      layout));
    }
  if (model)
    {
      name_list = g_slist_prepend (name_list, g_strdup (model));
      if (layout)
	name_list = g_slist_prepend (name_list,
				     g_strdup_printf ("%s-%s",
						      model,
						      layout));
    }
  g_free (encoding);
  g_free (layout);
  g_free (model);
  g_free (variant);
  
  for (slist = name_list; slist; slist = slist->next)
    {
      gchar *name = slist->data;
      
      if (!patch)
	{
	  patch = bst_key_table_patch_find (name);
	  BST_IF_DEBUG (KEYTABLE)
	    g_message ("Guessing keytable, %s \"%s\"",
		       patch ? "found" : "failed to get",
		       name);
	}
      else
	BST_IF_DEBUG (KEYTABLE)
	  g_message ("Guessing keytable, discarding \"%s\"", name);
      g_free (name);
    }
  g_slist_free (name_list);
  
  return patch;
}

static BstKeyTablePatch*
bst_key_table_from_xkb (const gchar *display)
{
  BstKeyTablePatch *patch = NULL;

  if (!BST_XKB_FORCE_QUERY && BST_XKB_SYMBOL)
    patch = bst_key_table_patch_find (BST_XKB_SYMBOL);

  if (!patch && !BST_XKB_FORCE_QUERY && BST_XKB_SYMBOL)
    {
      BST_IF_DEBUG (KEYTABLE)
	g_message ("Failed to find keytable \"%s\"", BST_XKB_SYMBOL);
    }

  if (!patch)
    {
      gchar *xkb_symbol = NULL;

      BST_IF_DEBUG (KEYTABLE)
	g_message ("Querying keytable layout from X-Server...");

      if (bst_xkb_open (display, TRUE))
	{
	  xkb_symbol = g_strdup (bst_xkb_get_symbol (TRUE));
	  if (!xkb_symbol)
	    xkb_symbol = g_strdup (bst_xkb_get_symbol (FALSE));
	  bst_xkb_close ();
	}

      patch = bst_key_table_from_xkb_symbol (xkb_symbol);
      g_free (xkb_symbol);
    }

  if (patch)
    {
      BST_IF_DEBUG (KEYTABLE)
	g_message ("Using keytable \"%s\"", patch->identifier);
    }
  else
    {
      gchar *name = BST_DFL_KEYTABLE;

      BST_IF_DEBUG (KEYTABLE)
	g_message ("Guessing keytable failed, reverting to \"%s\"", name);
      patch = bst_key_table_patch_find (name);
    }

  bst_globals_set_xkb_symbol (patch->identifier);
  
  return patch;
}

BseIcon*
bst_icon_from_stock (BstIconId _id) /* static icons, no reference counting needed */
{
#include "./icons/noicon.c"
#include "./icons/mouse_tool.c"
#include "./icons/properties.c"
#include "./icons/trash.c"
#include "./icons/close.c"
#include "./icons/no_ilink.c"
#include "./icons/no_olink.c"
  static const BsePixdata pixdatas[] = {
    /* BST_ICON_NONE */
    { 0, 0, 0, NULL, },
    /* BST_ICON_NOICON */
    { NOICON_PIXDATA_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NOICON_PIXDATA_WIDTH, NOICON_PIXDATA_HEIGHT,
      NOICON_PIXDATA_RLE_PIXEL_DATA, },
    /* BST_ICON_MOUSE_TOOL */
    { MOUSE_TOOL_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      MOUSE_TOOL_IMAGE_WIDTH, MOUSE_TOOL_IMAGE_HEIGHT,
      MOUSE_TOOL_IMAGE_RLE_PIXEL_DATA, },
    /* BST_ICON_PROPERTIES */
    { PROPERTIES_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      PROPERTIES_IMAGE_WIDTH, PROPERTIES_IMAGE_HEIGHT,
      PROPERTIES_IMAGE_RLE_PIXEL_DATA, },
    /* BST_ICON_TRASH */
    { TRASH_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      TRASH_IMAGE_WIDTH, TRASH_IMAGE_HEIGHT,
      TRASH_IMAGE_RLE_PIXEL_DATA, },
    /* BST_ICON_CLOSE */
    { CLOSE_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      CLOSE_IMAGE_WIDTH, CLOSE_IMAGE_HEIGHT,
      CLOSE_IMAGE_RLE_PIXEL_DATA, },
    /* BST_ICON_NO_ILINK */
    { NO_ILINK_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NO_ILINK_IMAGE_WIDTH, NO_ILINK_IMAGE_HEIGHT,
      NO_ILINK_IMAGE_RLE_PIXEL_DATA, },
    /* BST_ICON_NO_OLINK */
    { NO_OLINK_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NO_OLINK_IMAGE_WIDTH, NO_OLINK_IMAGE_HEIGHT,
      NO_OLINK_IMAGE_RLE_PIXEL_DATA, },
  };
  static const guint n_stock_icons = sizeof (pixdatas) / sizeof (pixdatas[0]);
  static BseIcon *icons[sizeof (pixdatas) / sizeof (pixdatas[0])] = { NULL, };
  guint icon_id = _id;
  
  g_assert (n_stock_icons == BST_ICON_LAST);
  g_return_val_if_fail (icon_id < n_stock_icons, NULL);
  
  if (!icons[icon_id])
    {
      if (!pixdatas[icon_id].encoded_pix_data)
	return NULL;

      icons[icon_id] = bse_icon_from_pixdata (pixdatas + icon_id); /* static reference */
      bse_icon_static_ref (icons[icon_id]);
    }

  return icons[icon_id];
}

/* read bstdefs.h on this */
void
bst_update_can_operate (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  /* FIXME, we need to queue a high prioritized idle here as
   * this function can be called multiple times in a row
   */
  
  /* figure toplevel app, and update it
   */
  widget = gtk_widget_get_ancestor (widget, BST_TYPE_APP);
  g_return_if_fail (BST_IS_APP (widget));
  
  bst_app_update_can_operate (BST_APP (widget));
}

/* read bstdefs.h on this */
extern void
bst_object_set (gpointer     object,
		const gchar *first_arg_name,
		...)
{
  va_list args;
  
  g_return_if_fail (GTK_IS_OBJECT (object));
  
  gtk_object_ref (object);
  
  va_start (args, first_arg_name);
  
  if (GNOME_IS_CANVAS_ITEM (object))
    gnome_canvas_item_set_valist (object, first_arg_name, args);
  else
    {
      GSList *arg_list = NULL;
      GSList *info_list = NULL;
      gchar *error;
      
      error = gtk_object_args_collect (GTK_OBJECT_TYPE (object),
				       &arg_list,
				       &info_list,
				       first_arg_name,
				       args);
      
      if (error)
	{
	  g_warning ("bst_object_set(): %s", error);
	  g_free (error);
	}
      else if (arg_list)
	{
	  GSList *arg;
	  GSList *info;
	  
	  for (arg = arg_list, info = info_list; arg; arg = arg->next, info = info->next)
	    gtk_object_arg_set (object, arg->data, info->data);
	  
	  gtk_args_collect_cleanup (arg_list, info_list);
	}
    }
  va_end (args);
  
  BST_OBJECT_ARGS_CHANGED (object);
  
  gtk_object_unref (object);
}

static gint
subwindow_button_press_event (GtkWidget      *window,
			      GdkEventButton *event)
{
  gboolean handled = FALSE;

  if (event->button == 3 && event->window == window->window)
    {
      handled = TRUE;
      if (bst_choice_modal (gtk_object_get_data (GTK_OBJECT (window), "subwindow-choice"),
			    event->button,
			    event->time) == 1)
	gtk_widget_hide (window);
    }

  return handled;
}

static gint
subwindow_delete_event (GtkWidget *window)
{
  gtk_widget_hide (window);

  return TRUE;
}

static GtkWidget *subwindow_choice = NULL;

GtkWidget*
bst_subwindow_new (GtkObject        *alive_host,
		   GtkWidget       **ssubwindow_p,
		   GtkWidget        *child,
		   BstSubWindowFlags flags)
{
  GtkWidget *window;

  g_return_val_if_fail (GTK_IS_WIDGET (child), NULL);
  g_return_val_if_fail (child->parent == NULL, NULL);
  if (alive_host)
    g_return_val_if_fail (GTK_IS_OBJECT (alive_host), NULL);
  if (ssubwindow_p)
    g_return_val_if_fail (ssubwindow_p != NULL, NULL);

  if (!subwindow_choice)
    {
      subwindow_choice = bst_choice_createv (BST_CHOICE (1, "Close", CLOSE),
					     BST_CHOICE_END);
      gtk_widget_set (subwindow_choice,
		      "object_signal::destroy", bse_nullify_pointer, &subwindow_choice,
		      NULL);
    }
  else
    gtk_widget_ref (subwindow_choice);

  window = gtk_widget_new (GTK_TYPE_WINDOW,
			   "auto_shrink", FALSE,
			   "allow_shrink", FALSE,
			   "allow_grow", TRUE,
			   "signal::delete_event", subwindow_delete_event, NULL,
			   "signal::button_press_event", subwindow_button_press_event, NULL,
			   "events", GDK_BUTTON_PRESS_MASK,
			   "child", child,
			   ssubwindow_p ? "object_signal::destroy" : NULL, bse_nullify_pointer, ssubwindow_p,
			   NULL);
  if (flags & BST_SUB_DESTROY_ON_HIDE)
    gtk_signal_connect_after (GTK_OBJECT (window),
			      "hide",
			      GTK_SIGNAL_FUNC (gtk_widget_destroy),
			      NULL);
  gtk_widget_ref (child);
  gtk_object_set_data_full (GTK_OBJECT (window), "subwindow-child", child, (GDestroyNotify) gtk_widget_unref);
  gtk_object_set_data_full (GTK_OBJECT (window), "subwindow-choice", subwindow_choice, (GDestroyNotify) gtk_widget_unref);
  if (alive_host)
    gtk_signal_connect_object_while_alive (alive_host,
					   "destroy",
					   GTK_SIGNAL_FUNC (gtk_widget_destroy),
					   GTK_OBJECT (window));
  else
    gtk_quit_add_destroy (1, GTK_OBJECT (window));

  return window;
}

GtkWidget*
bst_subwindow_get_child (GtkWidget *subwindow)
{
  g_return_val_if_fail (GTK_IS_WINDOW (subwindow), NULL);

  return gtk_object_get_data (GTK_OBJECT (subwindow), "subwindow-child");
}

void
gtk_widget_showraise (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_show (widget);
  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_raise (widget->window);
}

void
gtk_toplevel_hide (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  widget = gtk_widget_get_toplevel (widget);
  gtk_widget_hide (widget);
}

GnomeCanvasItem*
gnome_canvas_typed_item_at (GnomeCanvas *canvas,
			    GtkType      item_type,
			    gdouble      world_x,
			    gdouble      world_y)
{
  GnomeCanvasItem *item;

  g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);

  item = gnome_canvas_get_item_at (canvas, world_x, world_y);
  while (item && !gtk_type_is_a (GTK_OBJECT_TYPE (item), item_type))
    item = item->parent;

  return item && gtk_type_is_a (GTK_OBJECT_TYPE (item), item_type) ? item : NULL;
}

/* read bstdefs.h on this */
GnomeCanvasPoints*
gnome_canvas_points_new0 (guint num_points)
{
  GnomeCanvasPoints *points;
  guint i;
  
  g_return_val_if_fail (num_points > 1, NULL);
  
  points = gnome_canvas_points_new (num_points);
  for (i = 0; i < num_points; i++)
    {
      points->coords[i] = 0;
      points->coords[i + num_points] = 0;
    }
  
  return points;
}

GnomeCanvasPoints*
gnome_canvas_points_newv (guint num_points,
			  ...)
{
  GnomeCanvasPoints *points;
  guint i;
  va_list args;
  
  g_return_val_if_fail (num_points > 1, NULL);
  
  va_start (args, num_points);
  points = gnome_canvas_points_new (num_points);
  for (i = 0; i < num_points * 2; i++)
    points->coords[i] = va_arg (args, gdouble);
  va_end (args);
  
  return points;
}

static void
item_request_update_recurse (GnomeCanvasItem *item)
{
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));
  
  gnome_canvas_item_request_update (item);
  
  if (GNOME_IS_CANVAS_GROUP (item))
    {
      GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (item);
      GList *list;
      
      for (list = group->item_list; list; list = list->next)
	item_request_update_recurse (list->data);
    }
}

void
gnome_canvas_request_full_update (GnomeCanvas *canvas)
{
  g_return_if_fail (GNOME_IS_CANVAS (canvas));
  
  item_request_update_recurse (canvas->root);
}

guint
gnome_canvas_item_get_stacking (GnomeCanvasItem *item)
{
  g_return_val_if_fail (GNOME_IS_CANVAS_ITEM (item), 0);
  
  if (item->parent)
    {
      GnomeCanvasGroup *parent = GNOME_CANVAS_GROUP (item->parent);
      GList *list;
      guint pos = 0;
      
      for (list = parent->item_list; list; list = list->next)
	{
	  if (list->data == item)
	    return pos;
	  pos++;
	}
    }
  
  return 0;
}

extern void
gnome_canvas_item_keep_between (GnomeCanvasItem *between,
				GnomeCanvasItem *item1,
				GnomeCanvasItem *item2)
{
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (between));
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (item1));
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (item2));
  
  if (between->parent && item1->parent == between->parent && item2->parent == between->parent)
    {
      guint n, i, z;
      
      n = gnome_canvas_item_get_stacking (item1);
      i = gnome_canvas_item_get_stacking (item2);
      z = gnome_canvas_item_get_stacking (between);
      n = (n + i + (z > MIN (n, i))) / 2;
      if (z < n)
	gnome_canvas_item_raise (between, n - z);
      else if (n < z)
	gnome_canvas_item_lower (between, z - n);
    }
  else
    g_warning ("gnome_canvas_item_keep_between() called for non-siblings");
}

extern void
gnome_canvas_item_keep_above (GnomeCanvasItem *above,
			      GnomeCanvasItem *item1,
			      GnomeCanvasItem *item2)
{
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (above));
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (item1));
  g_return_if_fail (GNOME_IS_CANVAS_ITEM (item2));
  
  if (above->parent && item1->parent == above->parent && item2->parent == above->parent)
    {
      guint n, i, z;
      
      n = gnome_canvas_item_get_stacking (item1);
      i = gnome_canvas_item_get_stacking (item2);
      z = gnome_canvas_item_get_stacking (above);
      n = MAX (n, i) + 1;
      if (z < n)
	gnome_canvas_item_raise (above, n - z);
      else if (n < z)
	gnome_canvas_item_lower (above, z - n);
    }
  else
    g_warning ("gnome_canvas_item_keep_above() called for non-siblings");
}
