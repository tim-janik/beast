/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2006 Tim Janik
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
#include "gxkdialog.h"
#include "gxkradget.h"
#include "gxkstock.h"
#include "gxkstatusbar.h"
#include <gdk/gdkkeysyms.h>
#include <string.h>

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


/* --- variables --- */
static GSList		*enter_stack = NULL;


/* --- functions --- */
G_DEFINE_TYPE (GxkDialog, gxk_dialog, GTK_TYPE_WINDOW);

static void
gxk_dialog_init (GxkDialog *self)
{
  GtkWindow *window = GTK_WINDOW (self);
  
  self->flags = 0;
  self->pointer_loc = NULL;
  self->alive_object = NULL;
  gtk_window_set_role (window, G_OBJECT_TYPE_NAME (self));
  gxk_dialog_set_title (self, DEFAULT_TITLE);
  
  /* main box */
  self->mbox = g_object_new (GTK_TYPE_VBOX,
                             "visible", TRUE,
                             "homogeneous", FALSE,
                             "spacing", 0,
                             "parent", window,
                             NULL);
  gxk_nullify_in_object (self, &self->mbox);
  
  /* user vbox */
  self->vbox = g_object_new (GTK_TYPE_VBOX,
                             "visible", TRUE,
                             "homogeneous", FALSE,
                             "spacing", 0,
                             "parent", self->mbox,
                             NULL);
  gxk_nullify_in_object (self, &self->vbox);
  
  /* status bar */
  self->status_bar = gxk_status_bar_create ();
  gxk_nullify_in_object (self, &self->status_bar);
  gtk_box_pack_end (GTK_BOX (self->mbox), self->status_bar, FALSE, FALSE, 0);
  
  /* button box */
  self->hbox = g_object_new (GTK_TYPE_HBOX,
                             "visible", FALSE,
                             "homogeneous", TRUE,
                             "spacing", GXK_BUTTON_PADDING,
                             "border_width", GXK_OUTER_BORDER,
                             NULL);
  gxk_nullify_in_object (self, &self->hbox);
  gtk_box_pack_end (GTK_BOX (self->mbox), self->hbox, FALSE, TRUE, 0);
  
  /* separator */
  self->sep = g_object_new (GTK_TYPE_HSEPARATOR,
                            "visible", FALSE,
                            NULL);
  gxk_nullify_in_object (self, &self->sep);
  gtk_box_pack_end (GTK_BOX (self->mbox), self->sep, FALSE, FALSE, 0);
}

static void
gxk_dialog_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  GxkDialog *self = GXK_DIALOG (object);
  
  switch (prop_id)
    {
      GtkWindow *window;
      const gchar *cstring, *astring;
      gchar *string;
      guint old_flags, added_flags, removed_flags;
    case PROP_ALIVE_OBJECT:
      if (self->alive_object)
	g_signal_handlers_disconnect_by_func (self->alive_object, gtk_widget_destroy, self);
      self->alive_object = g_value_get_object (value);
      if (self->alive_object && G_OBJECT (self->alive_object)->ref_count)
	g_signal_connect_swapped (self->alive_object, "destroy", G_CALLBACK (gtk_widget_destroy), self);
      else
	self->alive_object = NULL;
      break;
    case PROP_POINTER:
      self->pointer_loc = g_value_get_pointer (value);
      break;
    case PROP_TITLE:
      cstring = g_value_get_string (value);
      if (!cstring)
	cstring = "";
      astring = g_get_application_name();
      string = g_strconcat (cstring, astring ? " - " : NULL, astring, NULL);
      g_object_set (self, "GtkWindow::title", string, NULL);
      g_free (string);
      break;
    case PROP_FLAGS:
      old_flags = self->flags;
      self->flags = g_value_get_flags (value);
      added_flags = self->flags & ~old_flags;
      removed_flags = old_flags & ~self->flags;
      window = GTK_WINDOW (self);
      if (added_flags & GXK_DIALOG_WINDOW_GROUP)
	{
	  GtkWindowGroup *wgroup = gtk_window_group_new ();
	  if (window->group)
	    gtk_window_group_remove_window (window->group, window);
          gtk_window_group_add_window (wgroup, window); // GTKFIX: Gtk+-2.2.1 window groups seem to be broken wrt menus, but we're using Gtk+-2.4 now */
	  g_object_unref (wgroup);
	}
      else if (removed_flags & GXK_DIALOG_WINDOW_GROUP)
	{
	  if (window->group)
	    gtk_window_group_remove_window (window->group, window);
	}
      if (added_flags & GXK_DIALOG_STATUS_BAR)
	{
	  if (self->status_bar)
	    gtk_widget_show (self->status_bar);
	}
      else if (removed_flags & GXK_DIALOG_STATUS_BAR)
	{
	  if (self->status_bar)
	    gtk_widget_hide (self->status_bar);
	}
      gtk_window_set_modal (GTK_WINDOW (self), self->flags & GXK_DIALOG_MODAL);
      if (added_flags & GXK_DIALOG_DELETE_BUTTON)
	{
	  /* we synthesize a delete event instead of hiding/destroying
	   * directly, because derived classes may override delete_event
	   */
	  gxk_dialog_default_action (self, GTK_STOCK_CLOSE, gxk_toplevel_delete, NULL);
	}
      else if (removed_flags & GXK_DIALOG_DELETE_BUTTON)
	self->flags |= GXK_DIALOG_DELETE_BUTTON;        /* some flags can't be unset */
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

  GTK_OBJECT_CLASS (gxk_dialog_parent_class)->destroy (object);
}

static void
gxk_dialog_finalize (GObject *object)
{
  GxkDialog *dialog = GXK_DIALOG (object);

  if (dialog->child)
    g_object_unref (dialog->child);
  dialog->child = NULL;

  G_OBJECT_CLASS (gxk_dialog_parent_class)->finalize (object);
}

/**
 * @param pointer_loc	pointer to nullify upon dialog destruction
 * @param alive_object	object which upon destruction, takes the dialog with it
 * @param flags	dialog flags
 * @param title	window title for the dialog
 * @param child	child to pack into the dialog
 *
 * Create a new configurable dialog. Possible values for the
 * flags are:
 * @itemize
 * @item GXK_DIALOG_HIDE_ON_DELETE - only hide and not destroy the
 * dialog upon window manager delete events;
 * @item GXK_DIALOG_IGNORE_ESCAPE - prevents delete event generation on Escape key presses;
 * @item GXK_DIALOG_DELETE_BUTTON - add a "Close" button to the dialog;
 * @item GXK_DIALOG_STATUS_BAR - add a status bar widget to the dialog;
 * @item GXK_DIALOG_WINDOW_GROUP - open up an extra window group for the dialog;
 * @item GXK_DIALOG_MODAL - the dialog is modal while visible;
 * @item GXK_DIALOG_POPUP_POS - popup the dialog below mouse pointer;
 * @item GXK_DIALOG_PRESERVE_STATE - prevents unrealization of the dialog upon hiding,
 * which preserves properties like the window size.
 * @done
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

gpointer
gxk_dialog_new_radget (gpointer        pointer_loc,
                       GtkObject      *alive_object,
                       GxkDialogFlags  flags,
                       const gchar    *title,
                       const gchar    *domain_name,
                       const gchar    *radget_name)
{
  GxkRadget *radget = gxk_radget_create (domain_name, radget_name, NULL);
  if (radget)
    return gxk_dialog_new (pointer_loc, alive_object, flags, title, radget);
  else
    return NULL;
}

static void
gxk_dialog_set_min_size (GxkDialog      *self,
                         gint            min_width,
                         gint            min_height)
{
  GdkGeometry geometry = { 0, };
  geometry.min_width = min_width;
  geometry.min_height = min_height;
  if (g_object_get_long (self, "GxkDialog-min-width") != geometry.min_width ||
      g_object_get_long (self, "GxkDialog-min-height") != geometry.min_height)
    {
      g_object_set_long (self, "GxkDialog-min-width", geometry.min_width);
      g_object_set_long (self, "GxkDialog-min-height", geometry.min_height);
      gtk_window_set_geometry_hints (GTK_WINDOW (self), NULL, &geometry, GDK_HINT_MIN_SIZE);
    }
}

/**
 * @param dialog	valid GxkDialog
 * @param min_width	minimum dialog width or -1
 * @param min_height	minimum dialog height or -1
 * @param default_width	default dialog width or -1
 * @param default_height	default dialog height or -1
 *
 * Set the dialog's minimum and default sizes, constrained to not exceed the
 * screen dimensions.
 */
void
gxk_dialog_set_sizes (GxkDialog      *dialog,
                      gint            min_width,
                      gint            min_height,
                      gint            default_width,
                      gint            default_height)
{
  gint swidth = gdk_screen_width();
  gint sheight = gdk_screen_height();
  min_width = CLAMP (min_width, -1, swidth);
  min_height = CLAMP (min_height, -1, sheight);
  if (default_width > 0)
    default_width = MIN (default_width, swidth * 0.95);
  if (default_height > 0)
    default_height = MIN (default_height, sheight * 0.95);
  if (min_width > 0 && default_width > 0)
    min_width = MIN (min_width, default_width);
  if (min_height > 0 && default_height > 0)
    min_height = MIN (min_height, default_height);
  gxk_dialog_set_min_size (dialog, min_width, min_height);
  gtk_window_set_default_size (GTK_WINDOW (dialog), default_width, default_height);
}

/**
 * @param dialog	valid GxkDialog
 * @param title	dialog window manager title
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
 * @param dialog	valid GxkDialog
 * @param flags	additional flags to set on the dialog.
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
 * @param dialog	valid GxkDialog
 * @param flags	flags to unset on the dialog.
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
 * @param dialog	valid GxkDialog
 *
 * Retrieve the primary child of the dialog.
 */
GtkWidget*
gxk_dialog_get_child (GxkDialog *dialog)
{
  g_return_val_if_fail (GXK_IS_DIALOG (dialog), NULL);
  /* return the single child that was passed to gxk_dialog_new() if any */
  return dialog->child;
}

/**
 * @param dialog	valid GxkDialog
 * @param child	new child
 *
 * Change the dialog's primary child to @a child.
 * Destroys the old child if any.
 */
void
gxk_dialog_set_child (GxkDialog *dialog,
		      GtkWidget *child)
{
  g_return_if_fail (GXK_IS_DIALOG (dialog));

  gtk_container_foreach (GTK_CONTAINER (dialog->vbox), (GtkCallback) gtk_widget_destroy, NULL);
  if (dialog->child)
    g_object_unref (dialog->child);
  dialog->child = child;
  if (dialog->child)
    {
      g_object_ref (dialog->child);
      gtk_container_add (GTK_CONTAINER (dialog->vbox), gtk_widget_get_toplevel (child));
    }
}

/**
 * @param dialog	valid GxkDialog
 * @param focus_widget	valid GtkWidget
 *
 * A GxkDialog will automatically unset the focus
 * everytime it is shown, unless @a focus_widget is
 * a valid widget that can be focused each time.
 */
void
gxk_dialog_set_focus (GxkDialog *self,
		      GtkWidget *focus_widget)
{
  g_return_if_fail (GXK_IS_DIALOG (self));

  if (self->focus_widget)
    g_signal_handlers_disconnect_by_func (self->focus_widget, g_nullify_pointer, &self->focus_widget);
  self->focus_widget = focus_widget;
  g_signal_connect_swapped (self->focus_widget, "destroy", G_CALLBACK (g_nullify_pointer), &self->focus_widget);
}

/**
 * @param dialog	 valid GxkDialog
 * @param default_widget valid GtkWidget
 *
 * This function is similar to gxk_dialog_set_focus(),
 * it just affects the widget taking the default
 * activation.
 */
void
gxk_dialog_set_default (GxkDialog *self,
			GtkWidget *default_widget)
{
  g_return_if_fail (GXK_IS_DIALOG (self));

  if (self->default_widget)
    g_signal_handlers_disconnect_by_func (self->default_widget, g_nullify_pointer, &self->default_widget);
  self->default_widget = default_widget;
  g_signal_connect_swapped (self->default_widget, "destroy", G_CALLBACK (g_nullify_pointer), &self->default_widget);
  if (!self->focus_widget && default_widget)
    gxk_dialog_set_focus (self, default_widget);
}

static void
gxk_dialog_show (GtkWidget *widget)
{
  GxkDialog *self = GXK_DIALOG (widget);
  
  if (self->flags & GXK_DIALOG_POPUP_POS)
    g_object_set (self, "window_position", GTK_WIN_POS_MOUSE, NULL);
  else
    g_object_set (self, "window_position", GTK_WIN_POS_NONE, NULL);

  if (self->focus_widget && GTK_WIDGET_CAN_FOCUS (self->focus_widget) &&
      gtk_widget_get_toplevel (self->focus_widget) == widget)
    gtk_window_set_focus (GTK_WINDOW (self), self->focus_widget);
  else
    gtk_window_set_focus (GTK_WINDOW (self), NULL);
  if (self->default_widget)
    gtk_widget_grab_default (self->default_widget);

  if (self->status_bar &&
      gxk_dialog_get_status_window () == NULL &&
      !g_slist_find (enter_stack, self))
    enter_stack = g_slist_prepend (enter_stack, self);

  GTK_WIDGET_CLASS (gxk_dialog_parent_class)->show (widget);

  gxk_widget_viewable_changed (widget);

  /* GTKFIX: gtk doesn't take away focus from a widget on a hidden notebook page when realizing the window */
  if (self->focus_widget && !gxk_widget_viewable (self->focus_widget))
    gtk_window_set_focus (GTK_WINDOW (self), NULL);
}

static void
gxk_dialog_hide (GtkWidget *widget)
{
  GxkDialog *self = GXK_DIALOG (widget);
  
  GTK_WIDGET_CLASS (gxk_dialog_parent_class)->hide (widget);

  gxk_widget_viewable_changed (widget);

  if (!(self->flags & GXK_DIALOG_PRESERVE_STATE))
    gxk_idle_unrealize_widget (widget);
}

static gboolean
gxk_dialog_key_press_event (GtkWidget   *widget,
			    GdkEventKey *event)
{
  GxkDialog *dialog = GXK_DIALOG (widget);
  GtkWindow *window = GTK_WINDOW (widget);
  gboolean handled = FALSE;

  /* decide whether we close the window upon Escape:
   * - we provide Escape as a short cut for the GXK_DIALOG_DELETE_BUTTON
   * - we offer Escape for normal dialogs, i.e. non status-shell ones.
   */
  if (event->keyval == GDK_Escape &&
      ((dialog->flags & GXK_DIALOG_DELETE_BUTTON) ||
       !(dialog->flags & GXK_DIALOG_IGNORE_ESCAPE)))
    {
      /* trigger delete event */
      gxk_toplevel_delete (widget);
      return TRUE;
    }

  /* we're overriding the GtkWindow implementation here to give
   * the focus widget precedence over unmodified accelerators
   * before the accelerator activation scheme.
   */

  /* invoke control/alt accelerators */
  if (!handled && event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))
    handled = gtk_window_activate_key (window, event);
  /* invoke focus widget handlers */
  if (!handled)
    handled = gtk_window_propagate_key_event (window, event);
  /* invoke non-(control/alt) accelerators */
  if (!handled && !(event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
    handled = gtk_window_activate_key (window, event);
  /* chain up, bypassing gtk_window_key_press(), to invoke binding set */
  if (!handled)
    handled = GTK_WIDGET_CLASS (g_type_class_peek (g_type_parent (GTK_TYPE_WINDOW)))->key_press_event (widget, event);

  return handled;
}

static gboolean
gxk_dialog_delete_event (GtkWidget   *widget,
			 GdkEventAny *event)
{
  GxkDialog *dialog = GXK_DIALOG (widget);

  if (dialog->flags & GXK_DIALOG_HIDE_ON_DELETE)
    {
      gtk_widget_hide (GTK_WIDGET (dialog));
      return TRUE;
    }
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
 * @return	a valid GxkDialog or NULL
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
 * @param dialog valid GxkDialog
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
 * @param dialog	valid GxkDialog
 * @param action	button label or stock ID
 * @param callback	callback function for button activation
 * @param data	callback data
 *
 * Add a new (stock) button to a dialog.
 */
GtkWidget*
gxk_dialog_action_multi (GxkDialog          *self,
			 const gchar        *action,
			 gpointer            callback,
			 gpointer            data,
			 const gchar        *icon_stock_id,
			 GxkDialogMultiFlags multi_mode)
{
  GtkWidget *button = NULL;

  g_return_val_if_fail (GXK_IS_DIALOG (self), NULL);
  g_return_val_if_fail (action != NULL, NULL);

  if (self->sep)
    gtk_widget_show (self->sep);
  if (self->hbox)
    {
      GtkWidget *alignment, *hbox, *image = icon_stock_id ? gxk_stock_image (icon_stock_id, GXK_ICON_SIZE_BUTTON) : NULL;

      if (!image)
	image = gxk_stock_image (action, GXK_ICON_SIZE_BUTTON);

      /* catch installation of a Close button */
      if (strcmp (action, GTK_STOCK_CLOSE) == 0)
        self->flags |= GXK_DIALOG_DELETE_BUTTON;

      /* setup button */
      button = g_object_new (GTK_TYPE_BUTTON,
			     "can_default", TRUE,
			     "parent", self->hbox,
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
					"label", gxk_stock_item (action),
					"use_underline", TRUE,
					NULL),
			  FALSE, TRUE, 0);
      gtk_widget_show_all (button);

      gtk_widget_show (self->hbox);

      if (multi_mode & GXK_DIALOG_MULTI_DEFAULT)
	gxk_dialog_set_default (self, button);
    }
  return button;
}

static void
gxk_dialog_class_init (GxkDialogClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

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
