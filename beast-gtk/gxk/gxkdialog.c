/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include	"gxkdialog.h"

#include	"gxkstock.h"
#include	"gxkstatusbar.h"
#include	<gdk/gdkkeysyms.h>
#include	<string.h>

#define	DEFAULT_TITLE	"Auxillary Dialog"


/* --- properties --- */
enum {
  PROP_0,
  PROP_POINTER,
  PROP_ALIVE_OBJECT,
  PROP_FLAGS,
  PROP_TITLE
};


/* --- prototypes --- */
static void	gxk_dialog_class_init		(GxkDialogClass	  *class);
static void	gxk_dialog_init			(GxkDialog	  *dialog);
static void	gxk_dialog_destroy		(GtkObject	  *object);
static void	gxk_dialog_finalize		(GObject	  *object);
static void	gxk_dialog_show			(GtkWidget	  *widget);
static void	gxk_dialog_hide			(GtkWidget	  *widget);
static gboolean gxk_dialog_key_press_event	(GtkWidget	  *widget,
						 GdkEventKey	  *event);
static gboolean gxk_dialog_delete_event		(GtkWidget	  *widget,
						 GdkEventAny	  *event);
static gboolean	gxk_dialog_enter_notify_event	(GtkWidget	  *widget,
						 GdkEventCrossing *event);
static void	gxk_dialog_set_property		(GObject	  *object,
						 guint		   prop_id,
						 const GValue	  *value,
						 GParamSpec	  *pspec);
static void	gxk_dialog_get_property		(GObject	  *object,
						 guint		   prop_id,
						 GValue		  *value,
						 GParamSpec	  *pspec);


/* --- variables --- */
static gpointer		 parent_class = NULL;
static GSList		*enter_stack = NULL;


/* --- functions --- */
GtkType
gxk_dialog_get_type (void)
{
  static GtkType dialog_type = 0;

  if (!dialog_type)
    {
      GtkTypeInfo dialog_info =
      {
	"GxkDialog",
	sizeof (GxkDialog),
	sizeof (GxkDialogClass),
	(GtkClassInitFunc) gxk_dialog_class_init,
	(GtkObjectInitFunc) gxk_dialog_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      dialog_type = gtk_type_unique (GTK_TYPE_WINDOW, &dialog_info);
    }

  return dialog_type;
}

static void
gxk_dialog_class_init (GxkDialogClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = gxk_dialog_finalize;
  gobject_class->set_property = gxk_dialog_set_property;
  gobject_class->get_property = gxk_dialog_get_property;
  
  object_class->destroy = gxk_dialog_destroy;

  widget_class->show = gxk_dialog_show;
  widget_class->hide = gxk_dialog_hide;
  widget_class->key_press_event = gxk_dialog_key_press_event;
  widget_class->delete_event = gxk_dialog_delete_event;
  widget_class->enter_notify_event = gxk_dialog_enter_notify_event;

  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_POINTER,
				   g_param_spec_pointer ("pointer", NULL, NULL,
							 G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_ALIVE_OBJECT,
				   g_param_spec_object ("alive_object", NULL, NULL,
							GTK_TYPE_OBJECT,
							G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_FLAGS,
				   g_param_spec_flags ("flags", NULL, NULL,
						       GXK_TYPE_DIALOG_FLAGS, 0,
						       G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (object_class),
				   PROP_TITLE,
				   g_param_spec_string ("title", NULL, NULL,
							NULL, G_PARAM_READWRITE));
}

static void
gxk_dialog_init (GxkDialog *dialog)
{
  GtkWindow *window = GTK_WINDOW (dialog);

  dialog->flags = 0;
  dialog->pointer_loc = NULL;
  dialog->alive_object = NULL;
  gxk_dialog_set_title (dialog, DEFAULT_TITLE);

  /* main box */
  dialog->mbox = g_object_new (GTK_TYPE_VBOX,
			       "visible", TRUE,
			       "homogeneous", FALSE,
			       "spacing", 0,
			       "parent", window,
			       NULL);
  g_signal_connect_swapped (dialog->mbox, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->mbox);

  /* user vbox */
  dialog->vbox = g_object_new (GTK_TYPE_VBOX,
			       "visible", TRUE,
			       "homogeneous", FALSE,
			       "spacing", 0,
			       "parent", dialog->mbox,
			       NULL);
  g_signal_connect_swapped (dialog->vbox, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->vbox);

  /* status bar */
  dialog->status_bar = gxk_status_bar_create ();
  g_signal_connect_swapped (dialog->status_bar, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->status_bar);
  gtk_box_pack_end (GTK_BOX (dialog->mbox), dialog->status_bar, FALSE, FALSE, 0);

  /* button box */
  dialog->hbox = g_object_new (GTK_TYPE_HBOX,
			       "visible", FALSE,
			       "homogeneous", TRUE,
			       "spacing", GXK_INNER_PADDING,
			       "border_width", GXK_INNER_PADDING,
			       NULL);
  g_signal_connect_swapped (dialog->hbox, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->hbox);
  gtk_box_pack_end (GTK_BOX (dialog->mbox), dialog->hbox, FALSE, TRUE, GXK_INNER_PADDING);

  /* separator */
  dialog->sep = g_object_new (GTK_TYPE_HSEPARATOR,
			      "visible", FALSE,
			      NULL);
  g_signal_connect_swapped (dialog->sep, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->sep);
  gtk_box_pack_end (GTK_BOX (dialog->mbox), dialog->sep, FALSE, FALSE, GXK_INNER_PADDING);
}

static void
gxk_dialog_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  GxkDialog *dialog = GXK_DIALOG (object);

  switch (prop_id)
    {
      const gchar *cstring;
      gchar *string;
      guint old_flags;
    case PROP_ALIVE_OBJECT:
      if (dialog->alive_object)
	g_signal_handlers_disconnect_by_func (dialog->alive_object, gtk_widget_destroy, dialog);
      dialog->alive_object = g_value_get_object (value);
      if (dialog->alive_object && G_OBJECT (dialog->alive_object)->ref_count)
	g_signal_connect_swapped (dialog->alive_object, "destroy", G_CALLBACK (gtk_widget_destroy), dialog);
      else
	dialog->alive_object = NULL;
      break;
    case PROP_POINTER:
      dialog->pointer_loc = g_value_get_pointer (value);
      break;
    case PROP_TITLE:
      cstring = g_value_get_string (value);
      if (!cstring)
	cstring = "";
      if (!GTK_WIDGET_VISIBLE (dialog))
	{
	  if (GTK_WIDGET_REALIZED (dialog))
	    gtk_widget_unrealize (GTK_WIDGET (dialog));
	  gtk_window_set_role (GTK_WINDOW (dialog), cstring);
	}
      string = g_strconcat (cstring, ": BEAST", NULL);
      g_object_set (dialog, "GtkWindow::title", string, NULL);
      g_free (string);
      break;
    case PROP_FLAGS:
      old_flags = dialog->flags;
      dialog->flags = g_value_get_flags (value);
      if (dialog->status_bar)
	{
	  if (dialog->flags & GXK_DIALOG_STATUS_SHELL)
	    gtk_widget_show (dialog->status_bar);
	  else
	    gtk_widget_hide (dialog->status_bar);
	}
      gtk_window_set_modal (GTK_WINDOW (dialog), dialog->flags & GXK_DIALOG_MODAL);
      /* some flags can't be unset */
      if (!(old_flags & GXK_DIALOG_DELETE_BUTTON) &&
	  (dialog->flags & GXK_DIALOG_DELETE_BUTTON))
	{
	  /* we synthesize a delete event instead of hiding/destroying
	   * directly, because derived classes may override delete_event
	   */
	  gxk_dialog_default_action (dialog, GTK_STOCK_CLOSE, gxk_toplevel_delete, NULL);
	}
      else if (old_flags & GXK_DIALOG_DELETE_BUTTON)
	dialog->flags |= GXK_DIALOG_DELETE_BUTTON;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gxk_dialog_get_property (GObject     *object,
			 guint        prop_id,
			 GValue      *value,
			 GParamSpec  *pspec)
{
  GxkDialog *dialog = GXK_DIALOG (object);

  switch (prop_id)
    {
    case PROP_FLAGS:
      g_value_set_flags (value, dialog->flags);
      break;
    case PROP_ALIVE_OBJECT:
      g_value_set_object (value, dialog->alive_object);
      break;
    case PROP_POINTER:
      g_value_set_pointer (value, dialog->pointer_loc);
      break;
    case PROP_TITLE:
      g_value_set_string (value, GTK_WINDOW (dialog)->title);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gxk_dialog_destroy (GtkObject *object)
{
  GxkDialog *dialog = GXK_DIALOG (object);

  enter_stack = g_slist_remove (enter_stack, dialog);

  if (dialog->pointer_loc)
    *dialog->pointer_loc = NULL;

  g_object_set (dialog,
		"alive_object", NULL,
		"pointer", NULL,
		NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gxk_dialog_finalize (GObject *object)
{
  // GxkDialog *dialog = GXK_DIALOG (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gxk_dialog_new
 * @pointer_loc:  pointer to nullify upon dialog destruction
 * @alive_object: object which upon destruction, takes the dialog with it
 * @flags:        dialog flags
 * @title:        window title for the dialog
 * @child:        child to pack into the dialog
 *
 * Create a new configurable dialog. Possible values for the
 * flags are:
 * %GXK_DIALOG_HIDE_ON_DELETE: only hide and not desroy the
 * dialog upon window manager delete events;
 * %GXK_DIALOG_STATUS_SHELL: the dialog has a status bar and
 * acts as a shell window for primary application data;
 * %GXK_DIALOG_MODAL: the dialog is modal while visible;
 * %GXK_DIALOG_POPUP_POS: popup the dialog below mouse pointer;
 * %GXK_DIALOG_DELETE_BUTTON: add a "Close" button to the dialog
 * (not recommended for GXK_DIALOG_STATUS_SHELL dialogs, which
 * usually have menus).
 */
gpointer
gxk_dialog_new (gpointer       pointer_loc,
		GtkObject     *alive_object,
		GxkDialogFlags flags,
		const gchar   *title,
		GtkWidget     *child)
{
  GxkDialog *dialog;

  dialog = g_object_new (GXK_TYPE_DIALOG,
			 "pointer", pointer_loc,
			 "alive_object", alive_object,
			 "flags", flags,
			 "title", title ? title : DEFAULT_TITLE,
			 NULL);
  gxk_dialog_set_title (dialog, title);
  gxk_dialog_set_child (dialog, child);

  return dialog;
}

/**
 * gxk_dialog_set_title
 * @dialog: valid GxkDialog
 * @title: dialog window manager title
 *
 * Change the dialog's window manager title and role.
 */
void
gxk_dialog_set_title (GxkDialog   *dialog,
		      const gchar *title)
{
  g_return_if_fail (GXK_IS_DIALOG (dialog));

  g_object_set (dialog, "title", title, NULL);
}

/**
 * gxk_dialog_add_flags
 * @dialog: valid GxkDialog
 * @flags: additional flags to set on the dialog.
 *
 * Alter dialog flags, see gxk_dialog_new().
 */
void
gxk_dialog_add_flags (GxkDialog     *dialog,
		      GxkDialogFlags flags)
{
  gint f;

  g_return_if_fail (GXK_IS_DIALOG (dialog));

  f = dialog->flags;
  f |= flags;
  g_object_set (dialog, "flags", f, NULL);
}

/**
 * gxk_dialog_clear_flags
 * @dialog: valid GxkDialog
 * @flags: flags to unset on the dialog.
 *
 * Alter dialog flags, see gxk_dialog_new().
 */
void
gxk_dialog_clear_flags (GxkDialog     *dialog,
			GxkDialogFlags flags)
{
  gint f;

  g_return_if_fail (GXK_IS_DIALOG (dialog));

  f = dialog->flags;
  f &= ~flags;
  g_object_set (dialog, "flags", f, NULL);
}

/**
 * gxk_dialog_get_child
 * @dialog: valid GxkDialog
 *
 * Retrieve the primary child of the dialog.
 */
GtkWidget*
gxk_dialog_get_child (GxkDialog *dialog)
{
  GtkBoxChild *child;
  GtkBox *box;

  g_return_val_if_fail (GXK_IS_DIALOG (dialog), NULL);

  /* return the single child that was passed to gxk_dialog_new() if any */
  box = dialog->vbox ? GTK_BOX (dialog->vbox) : NULL;
  child = box && box->children ? box->children->data : NULL;

  return child ? child->widget : NULL;
}

/**
 * gxk_dialog_set_child
 * @dialog: valid GxkDialog
 * @child:  new child
 *
 * Change the dialog's primary child to @child.
 * Destroys the old child if any.
 */
void
gxk_dialog_set_child (GxkDialog *dialog,
		      GtkWidget *child)
{
  g_return_if_fail (GXK_IS_DIALOG (dialog));

  gtk_container_foreach (GTK_CONTAINER (dialog->vbox), (GtkCallback) gtk_widget_destroy, NULL);
  if (child)
    gtk_container_add (GTK_CONTAINER (dialog->vbox), child);
}

static void
gxk_dialog_show (GtkWidget *widget)
{
  GxkDialog *dialog = GXK_DIALOG (widget);
  
  if (dialog->flags & GXK_DIALOG_POPUP_POS)
    g_object_set (dialog, "window_position", GTK_WIN_POS_MOUSE, NULL);
  else
    g_object_set (dialog, "window_position", GTK_WIN_POS_NONE, NULL);
  
  gtk_window_set_focus (GTK_WINDOW (dialog), NULL);
  if (dialog->default_widget)
    gtk_widget_grab_default (dialog->default_widget);

  if (dialog->status_bar &&
      gxk_dialog_get_status_window () == NULL &&
      !g_slist_find (enter_stack, dialog))
    enter_stack = g_slist_prepend (enter_stack, dialog);

  GTK_WIDGET_CLASS (parent_class)->show (widget);
}

static void
gxk_dialog_hide (GtkWidget *widget)
{
  // GxkDialog *dialog = GXK_DIALOG (widget);
  
  GTK_WIDGET_CLASS (parent_class)->hide (widget);
}

static gboolean
gxk_dialog_key_press_event (GtkWidget   *widget,
			    GdkEventKey *event)
{
  GxkDialog *dialog = GXK_DIALOG (widget);

  /* decide whether we close the window upon Escape:
   * - we provide Escape as a short cut for the GXK_DIALOG_DELETE_BUTTON
   * - we offer Escape for normal dialogs, i.e. non status-shell ones.
   */
  if (event->keyval == GDK_Escape &&
      ((dialog->flags & GXK_DIALOG_DELETE_BUTTON) ||
       !(dialog->flags & GXK_DIALOG_STATUS_SHELL)))
    {
      /* trigger delete event */
      gxk_toplevel_delete (widget);
      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
}

static gboolean
gxk_dialog_delete_event (GtkWidget   *widget,
			 GdkEventAny *event)
{
  GxkDialog *dialog = GXK_DIALOG (widget);

  if (dialog->flags & GXK_DIALOG_HIDE_ON_DELETE)
    gtk_widget_hide (GTK_WIDGET (dialog));

  if (dialog->flags & GXK_DIALOG_HIDE_ON_DELETE)
    return TRUE;
  else
    return FALSE;
}

static gboolean
gxk_dialog_enter_notify_event (GtkWidget        *widget,
			       GdkEventCrossing *event)
{
  GxkDialog *dialog = GXK_DIALOG (widget);
  GtkWidget *event_widget = gtk_get_event_widget ((GdkEvent*) event);

  if (event_widget == widget && event->detail != GDK_NOTIFY_INFERIOR)
    {
      enter_stack = g_slist_remove (enter_stack, dialog);
      enter_stack = g_slist_prepend (enter_stack, dialog);
    }
  return FALSE;
}

/**
 * gxk_dialog_get_status_window
 * @RETURNS: a valid GxkDialog or %NULL
 *
 * Retrieve the most recently entered GxkDialog if any.
 */
GxkDialog*
gxk_dialog_get_status_window (void)
{
  GSList *slist;

  for (slist = enter_stack; slist; slist = slist->next)
    {
      GxkDialog *dialog = GXK_DIALOG (slist->data);

      if (dialog->status_bar && GTK_WIDGET_DRAWABLE (dialog->status_bar))
	return dialog;
    }
  return NULL;
}

/**
 * gxk_dialog_remove_actions
 * @dialog: valid GxkDialog
 *
 * Remove all action buttons setup for this dialog.
 */
void
gxk_dialog_remove_actions (GxkDialog *dialog)
{
  g_return_if_fail (GXK_IS_DIALOG (dialog));

  if (dialog->hbox)
    gtk_container_foreach (GTK_CONTAINER (dialog->hbox), (GtkCallback) gtk_widget_destroy, NULL);
  dialog->flags &= ~GXK_DIALOG_DELETE_BUTTON;
}

/**
 * gxk_dialog_action*
 * @dialog:   valid GxkDialog
 * @action:   button label or stock ID
 * @callback: callback function for button activation
 * @data:     callback data
 *
 * Add a new (stock) button to a dialog.
 */
GtkWidget*
gxk_dialog_action_multi (GxkDialog          *dialog,
			 const gchar        *action,
			 gpointer            callback,
			 gpointer            data,
			 const gchar        *icon_stock_id,
			 GxkDialogMultiFlags multi_mode)
{
  GtkWidget *button = NULL;

  g_return_val_if_fail (GXK_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (action != NULL, NULL);

  if (dialog->sep)
    gtk_widget_show (dialog->sep);
  if (dialog->hbox)
    {
      GtkWidget *alignment, *hbox, *image = icon_stock_id ? gxk_stock_image (icon_stock_id, GXK_SIZE_BUTTON) : NULL;

      if (!image)
	image = gxk_stock_image (action, GXK_SIZE_BUTTON);

      /* catch installation of a Close button */
      if (strcmp (action, GTK_STOCK_CLOSE) == 0)
	dialog->flags |= GXK_DIALOG_DELETE_BUTTON;

      /* setup button */
      button = g_object_new (GTK_TYPE_BUTTON,
			     "can_default", TRUE,
			     "parent", dialog->hbox,
			     NULL);
      if (callback)
	g_signal_connect_data (button, "clicked",
			       callback, data, NULL,
			       (multi_mode & GXK_DIALOG_MULTI_SWAPPED) ? G_CONNECT_SWAPPED : 0);

      /* setup button contents */
      alignment = gtk_alignment_new (0.5, 0.5, 0.1, 0.1);
      gtk_container_add (GTK_CONTAINER (button), alignment);
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_container_add (GTK_CONTAINER (alignment), hbox);
      if (image)
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (hbox),
			  g_object_new (GTK_TYPE_LABEL,
					"label", gxk_stock_action (action),
					"use_underline", TRUE,
					NULL),
			  FALSE, TRUE, 0);
      gtk_widget_show_all (button);

      gtk_widget_show (dialog->hbox);

      if (multi_mode & GXK_DIALOG_MULTI_DEFAULT)
	{
	  if (dialog->default_widget)
	    g_signal_handlers_disconnect_by_func (dialog->default_widget, g_nullify_pointer, &dialog->default_widget);
	  dialog->default_widget = button;
	  g_signal_connect_swapped (dialog->default_widget, "destroy", G_CALLBACK (g_nullify_pointer), &dialog->default_widget);
	}
    }
  return button;
}
