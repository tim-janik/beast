/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2003 Tim Janik
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
#include "gxkutils.h"
#include "gxkcellrendererpopup.h"
#include <string.h>


/* --- generated marshallers --- */
#include "gxkmarshal.c"


/* --- generated type IDs and enums --- */
#include "gxkgentypes.c"


/* --- prototypes --- */
static void     gxk_traverse_viewable_changed	(GtkWidget      *widget,
						 gpointer        data);


/* --- variables --- */
static gulong viewable_changed_id = 0;


/* --- functions --- */
void
_gxk_init_utils (void)
{
  /* type registrations */
  gxk_type_register_generated (G_N_ELEMENTS (generated_type_entries), generated_type_entries);

  /* Gtk+ patchups */
  viewable_changed_id = g_signal_newv ("viewable-changed",
				       G_TYPE_FROM_CLASS (gtk_type_class (GTK_TYPE_WIDGET)),
				       G_SIGNAL_RUN_LAST,
				       g_cclosure_new (G_CALLBACK (gxk_traverse_viewable_changed), NULL, NULL),
				       NULL, NULL,
				       gtk_marshal_VOID__VOID,
				       G_TYPE_NONE, 0, NULL);
}

/**
 * gxk_type_register_generated
 * @n_entries: number of generated types to register
 * @entries:   GxkTypeGenerated type descriptions
 *
 * Register each of the generated type entries with the
 * type system. Currently supported parent types are
 * %G_TYPE_ENUM and %G_TYPE_FLAGS in which cases the
 * @type_data member must point to a %NULL terminated
 * array of GEnumValue or GFlagValue structures. No
 * extra copying is performed, so the passed in
 * structures have to persist throughout runtime.
 */
void
gxk_type_register_generated (guint                   n_entries,
			     const GxkTypeGenerated *entries)
{
  guint i;

  for (i = 0; i < n_entries; i++)
    {
      GType type_id;
      switch (entries[i].parent)
	{
	case G_TYPE_ENUM:
	  type_id = g_enum_register_static (entries[i].type_name, entries[i].type_data);
	  break;
	case G_TYPE_FLAGS:
	  type_id = g_flags_register_static (entries[i].type_name, entries[i].type_data);
	  break;
	default:
	  g_warning ("%s: unsupported parent type `%s'", G_STRLOC, g_type_name (entries[i].parent));
	  type_id = 0;
	  break;
	}
      *entries[i].type_id = type_id;
    }
}

/**
 * g_object_set_double
 * @object: a valid GObject
 * @name:   name of the double value to set
 * @v_double: the actual value
 *
 * Convenience variant of g_object_set_data() to set
 * a double instead of a pointer.
 */
void
g_object_set_double (gpointer     object,
		     const gchar *name,
		     gdouble      v_double)
{
  gdouble zero = 0;

  g_return_if_fail (G_IS_OBJECT (object));

  if (memcmp (&v_double, &zero, sizeof (zero)) == 0)
    g_object_set_data (object, name, NULL);
  else
    {
      gdouble *dp = g_new0 (gdouble, 1);
      *dp = v_double;
      g_object_set_data_full (object, name, dp, g_free);
    }
}

/**
 * g_object_get_double
 * @object:  a valid GObject
 * @name:    name of the double value to retrieve
 * @RETURNS: the actual value
 *
 * Convenience variant of g_object_get_data() to retrieve
 * a double instead of a pointer.
 */
gdouble
g_object_get_double (gpointer     object,
		     const gchar *name)
{
  gdouble *dp;

  g_return_val_if_fail (G_IS_OBJECT (object), 0);
  
  dp = g_object_get_data (object, name);
  return dp ? *dp : 0;
}

/**
 * g_object_set_long
 * @object: a valid GObject
 * @name:   name of the long value to set
 * @v_long: the actual value
 *
 * Convenience variant of g_object_set_data() to set
 * a long instead of a pointer.
 */
void
g_object_set_long (gpointer     object,
		   const gchar *name,
		   glong        v_long)
{
  g_return_if_fail (G_IS_OBJECT (object));

  g_object_set_data (object, name, (gpointer) v_long);
}

/**
 * g_object_get_long
 * @object:  a valid GObject
 * @name:    name of the long value to retrieve
 * @RETURNS: the actual value
 *
 * Convenience variant of g_object_get_data() to retrieve
 * a long instead of a pointer.
 */
glong
g_object_get_long (gpointer     object,
		   const gchar *name)
{
  g_return_val_if_fail (G_IS_OBJECT (object), 0);

  return (glong) g_object_get_data (object, name);
}

gchar*
gxk_convert_latin1_to_utf8 (const gchar *string)
{
  if (string)
    {
      const guchar *s = string;
      guint l = strlen (s);
      guchar *dest = g_new (guchar, l * 2 + 1), *d = dest;
      while (*s)
	if (*s >= 0xC0)
	  *d++ = 0xC3, *d++ = *s++ - 0x40;
	else if (*s >= 0x80)
	  *d++ = 0xC2, *d++ = *s++;
	else
	  *d++ = *s++;
      *d++ = 0;
      return dest;
    }
  return NULL;
}

gchar*
gxk_filename_to_utf8 (const gchar *filename)
{
  if (filename)
    {
      gchar *s = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);
      if (!s)
	s = gxk_convert_latin1_to_utf8 (filename);
      return s;
    }
  return NULL;
}

/* --- Gtk+ Utilities --- */
/**
 * gxk_widget_viewable_changed
 * @widget: valid #GtkWidget
 *
 * A widget should call this function if it changed
 * the mapped state of one of its children (or if it
 * is a toplevel and gets show or hidden) to emit the
 * ::viewable-changed signal on the related sub-tree.
 * #GxkDialog properly emits this signal if show or
 * hidden, containers like #GtkNotebook need this
 * function be explicitely connected to their ::switch-page
 * signal, in order for their children to get properly
 * notified.
 */
void
gxk_widget_viewable_changed (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_signal_emit (widget, viewable_changed_id, 0);
}

static void
gxk_traverse_viewable_changed (GtkWidget *widget,
			       gpointer   data)
{
  if (GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget), (GtkCallback) gxk_widget_viewable_changed, NULL);
}

/**
 * gxk_widget_viewable
 * @widget: valid #GtkWidget
 * RETURNS: %TRUE if the widget is viewable, %FALSE otherwise
 *
 * Check whether a widget is viewable by verifying the mapped
 * state of the widget and all its parent widgets.
 */
gboolean
gxk_widget_viewable (GtkWidget *widget)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  while (widget)
    {
      if (!GTK_WIDGET_MAPPED (widget))
	return FALSE;
      widget = widget->parent;
    }
  return TRUE;
}

/**
 * gxk_window_set_cursor_type
 * @window: valid #GdkWindow*
 * @cursor: #GdkCursorType cursor type
 *
 * Set a window's cursor type. If %GXK_DEFAULT_CURSOR is specified
 * the window's cursor will be inherited from it's parent.
 */
void
gxk_window_set_cursor_type (GdkWindow    *window,
			    GdkCursorType cursor)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (cursor >= GDK_LAST_CURSOR || cursor < 0)
    gdk_window_set_cursor (window, NULL);
  else
    {
      GdkCursor *wc = gdk_cursor_new (cursor & ~1);
      gdk_window_set_cursor (window, wc);
      gdk_cursor_unref (wc);
    }
}

static guint   expose_handler_id = 0;
static GSList *expose_windows = NULL;
static GSList *cexpose_windows = NULL;

static gboolean
expose_handler (gpointer data)
{
  GdkWindow *window, *cwindow;
  GDK_THREADS_ENTER ();
  window = g_slist_pop_head (&expose_windows);
  cwindow = g_slist_pop_head (&cexpose_windows);
  while (window || cwindow)
    {
      if (window)
	gdk_window_process_updates (window, FALSE);
      if (cwindow)
	gdk_window_process_updates (cwindow, TRUE);
      window = g_slist_pop_head (&expose_windows);
      cwindow = g_slist_pop_head (&cexpose_windows);
    }
  expose_handler_id = 0;
  GDK_THREADS_LEAVE ();
  return FALSE;
}

/**
 * gxk_window_process_next
 * @window:          valid #GdkWindow
 * @update_children: whether to also process updates for child windows
 *
 * Cause @window to be updated asyncronously as soon as possible via
 * gdk_window_process_updates().
 */
void
gxk_window_process_next (GdkWindow *window,
			 gboolean   update_children)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (update_children)
    cexpose_windows = g_slist_append_uniq (cexpose_windows, window);
  else
    expose_windows = g_slist_append_uniq (expose_windows, window);
  if (!expose_handler_id)
    expose_handler_id = g_idle_add_full (G_PRIORITY_DEFAULT, expose_handler, NULL, NULL);
}

/**
 * gxk_color_alloc
 * @colormap: valid #GdkColormap
 * @color:    valid #GdkColor
 *
 * Allocate a color like gdk_color_alloc() with automated
 * error checking.
 */
void
gxk_color_alloc (GdkColormap *colormap,
		 GdkColor    *color)
{
  if (!gdk_color_alloc (colormap, color))
    g_message ("failed to allocate color: { %d, %d, %d }", color->red, color->green, color->blue);
}

/**
 * gxk_widget_make_insensitive
 * @widget: a valid GtkWidget
 *
 * This function is euqivalent to gtk_widget_set_sensitive (@widget, #FALSE);
 * It exists as a convenient signal connection callback.
 */
void
gxk_widget_make_insensitive (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (GTK_WIDGET_IS_SENSITIVE (widget))
    gtk_widget_set_sensitive (widget, FALSE);
}

/**
 * gxk_widget_make_sensitive
 * @widget: a valid GtkWidget
 *
 * This function is euqivalent to gtk_widget_set_sensitive (@widget, #TRUE);
 * It exists as a convenient signal connection callback.
 */
void
gxk_widget_make_sensitive (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (!GTK_WIDGET_IS_SENSITIVE (widget))
    gtk_widget_set_sensitive (widget, TRUE);
}

static gint
idle_shower (GtkWidget **widget_p)
{
  GDK_THREADS_ENTER ();

  if (GTK_IS_WIDGET (*widget_p))
    {
      gtk_signal_disconnect_by_func (GTK_OBJECT (*widget_p),
				     GTK_SIGNAL_FUNC (gtk_widget_destroyed),
				     widget_p);
      gtk_widget_show (*widget_p);
    }
  g_free (widget_p);

  GDK_THREADS_LEAVE ();

  return FALSE;
}

/**
 * gxk_idle_show_widget
 * @widget: a valid #GtkWidget
 *
 * Defer showing this widget until the next idle handler
 * is run. This is usefull if other things are pending
 * which need to be processed first, for instance
 * hiding other toplevels.
 */
void
gxk_idle_show_widget (GtkWidget *widget)
{
  GtkWidget **widget_p;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  widget_p = g_new (GtkWidget*, 1);

  *widget_p = widget;
  gtk_signal_connect (GTK_OBJECT (widget),
		      "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed),
		      widget_p);
  gtk_idle_add_priority (GTK_PRIORITY_RESIZE - 1, (GtkFunction) idle_shower, widget_p);
}

static gint
idle_unrealizer (GtkWidget **widget_p)
{
  GDK_THREADS_ENTER ();

  if (GTK_IS_WINDOW (*widget_p))
    {
      gtk_signal_disconnect_by_func (GTK_OBJECT (*widget_p),
				     GTK_SIGNAL_FUNC (gtk_widget_destroyed),
				     widget_p);
      if (!GTK_WIDGET_VISIBLE (*widget_p) &&
          GTK_WIDGET_REALIZED (*widget_p))
        gtk_widget_unrealize (*widget_p);
    }
  g_free (widget_p);

  GDK_THREADS_LEAVE ();

  return FALSE;
}

/**
 * gxk_idle_unrealize_widget
 * @widget: a valid #GtkWindow
 *
 * Defer unrealizing this widget until the next idle handler
 * is run. This is usefull if other things are pending
 * which need to be processed first, e.g. completing a running
 * signal emission.
 */
void
gxk_idle_unrealize_widget (GtkWidget *widget)
{
  GtkWidget **widget_p;

  g_return_if_fail (GTK_IS_WINDOW (widget));

  widget_p = g_new (GtkWidget*, 1);

  *widget_p = widget;
  gtk_signal_connect (GTK_OBJECT (widget),
		      "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed),
		      widget_p);
  gtk_idle_add_priority (GTK_PRIORITY_RESIZE - 1, (GtkFunction) idle_unrealizer, widget_p);
}

void
gxk_notebook_add_page (GtkNotebook *notebook,
		       GtkWidget   *child,
		       const gchar *tab_text,
		       gboolean     expand_fill)
{
  gtk_container_add (GTK_CONTAINER (notebook), child);
  gtk_notebook_set_tab_label_text (notebook, child, tab_text);
  gtk_notebook_set_menu_label_text (notebook, child, tab_text);
  gtk_notebook_set_tab_label_packing (notebook, child, expand_fill, expand_fill, GTK_PACK_START);
}

/**
 * gxk_notebook_set_current_page_widget
 * @notebook: valid #GtkNotebook
 * @page:     @notebook page widget
 *
 * Set the current notebook page from a page widget, instead
 * of a page number.
 */
void
gxk_notebook_set_current_page_widget (GtkNotebook *notebook,
				      GtkWidget   *page)
{
  gint num = gtk_notebook_page_num (notebook, page);
  if (num >= 0)
    gtk_notebook_set_current_page (notebook, num);
}

/**
 * gtk_notebook_current_widget
 * @notebook: valid #GtkNotebook
 * @RETURNS:  the widget corresponding to the current page
 *
 * This function is essentially a shortcut for
 * gtk_notebook_get_current_page() and gtk_notebook_get_nth_page().
 */
GtkWidget*
gtk_notebook_current_widget (GtkNotebook *notebook)
{
  return gtk_notebook_get_nth_page (notebook, gtk_notebook_get_current_page (notebook));
}

/**
 * gxk_widget_showraise
 * @widget: a valid widget
 *
 * Show the widget. If the widget is a toplevel,
 * also raise its window to top.
 */
void
gxk_widget_showraise (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_show (widget);
  if (GTK_WIDGET_REALIZED (widget) && !widget->parent)
    gdk_window_raise (widget->window);
}

/**
 * gxk_toplevel_delete
 * @widget: a widget having a toplevel
 *
 * This function is usefull to produce the exact same effect
 * as if the user caused window manager triggered window
 * deletion on the toplevel of @widget.
 */
void
gxk_toplevel_delete (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  widget = gtk_widget_get_toplevel (widget);
  if (GTK_IS_WINDOW (widget) && GTK_WIDGET_DRAWABLE (widget))
    {
      GdkEvent event = { 0, };

      event.any.type = GDK_DELETE;
      event.any.window = widget->window;
      event.any.send_event = TRUE;
      gdk_event_put (&event);
    }
}

/**
 * gxk_toplevel_activate_default
 * @widget: a widget having a toplevel
 *
 * Activate the default widget of the toplevel of @widget.
 */
void
gxk_toplevel_activate_default (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  widget = gtk_widget_get_toplevel (widget);
  if (GTK_IS_WINDOW (widget))
    gtk_window_activate_default (GTK_WINDOW (widget));
}

/**
 * gxk_toplevel_hide
 * @widget: a widget having a toplevel
 *
 * Hide the toplevel of @widget.
 */
void
gxk_toplevel_hide (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  widget = gtk_widget_get_toplevel (widget);
  gtk_widget_hide (widget);
}

static void
style_modify_fg_as_sensitive (GtkWidget *widget)
{
  GtkRcStyle *rc_style = gtk_rc_style_new ();

  rc_style->color_flags[GTK_STATE_INSENSITIVE] = GTK_RC_FG;
  rc_style->fg[GTK_STATE_INSENSITIVE].red = widget->style->fg[GTK_STATE_NORMAL].red;
  rc_style->fg[GTK_STATE_INSENSITIVE].green = widget->style->fg[GTK_STATE_NORMAL].green;
  rc_style->fg[GTK_STATE_INSENSITIVE].blue = widget->style->fg[GTK_STATE_NORMAL].blue;
  gtk_widget_modify_style (widget, rc_style);
}

/**
 * gxk_widget_modify_as_title
 * @widget: a valid GtkWidget
 *
 * Modify the widget and it's style, so that it is insensitive,
 * but doesn't quite look that way. Usefull for inactive title
 * menu items in menus.
 */
void
gxk_widget_modify_as_title (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_set_sensitive (widget, FALSE);
  if (GTK_WIDGET_REALIZED (widget))
    style_modify_fg_as_sensitive (widget);
  if (!gxk_signal_handler_pending (widget, "realize", G_CALLBACK (style_modify_fg_as_sensitive), NULL))
    g_signal_connect_after (widget, "realize", G_CALLBACK (style_modify_fg_as_sensitive), NULL);
  if (!gxk_signal_handler_pending (widget, "realize", G_CALLBACK (gxk_widget_make_insensitive), NULL))
    g_signal_connect_after (widget, "realize", G_CALLBACK (gxk_widget_make_insensitive), NULL);
}

static void
style_modify_bg_as_base (GtkWidget *widget)
{
  GtkRcStyle *rc_style = gtk_widget_get_modifier_style (widget);

  rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BG;
  rc_style->bg[GTK_STATE_NORMAL].red = widget->style->base[GTK_STATE_NORMAL].red;
  rc_style->bg[GTK_STATE_NORMAL].green = widget->style->base[GTK_STATE_NORMAL].green;
  rc_style->bg[GTK_STATE_NORMAL].blue = widget->style->base[GTK_STATE_NORMAL].blue;
  gtk_widget_modify_style (widget, rc_style);
}

/**
 * gxk_widget_modify_bg_as_base
 * @widget: a valid GtkWidget
 *
 * Modify the widget's background to look like the background
 * of a text or list widget (usually white). This is usefull
 * if a hbox or similar widget is used to "simulate" a list,
 * text, or similar widget.
 */
void
gxk_widget_modify_bg_as_base (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (GTK_WIDGET_REALIZED (widget))
    style_modify_bg_as_base (widget);
  if (!gxk_signal_handler_pending (widget, "realize", G_CALLBACK (style_modify_bg_as_base), NULL))
    g_signal_connect_after (widget, "realize", G_CALLBACK (style_modify_bg_as_base), NULL);
}

static void
style_modify_base_as_bg (GtkWidget *widget)
{
  GtkRcStyle *rc_style = gtk_rc_style_new ();

  rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BASE;
  rc_style->base[GTK_STATE_NORMAL].red = widget->style->bg[GTK_STATE_NORMAL].red;
  rc_style->base[GTK_STATE_NORMAL].green = widget->style->bg[GTK_STATE_NORMAL].green;
  rc_style->base[GTK_STATE_NORMAL].blue = widget->style->bg[GTK_STATE_NORMAL].blue;
  gtk_widget_modify_style (widget, rc_style);
}

/**
 * gxk_widget_modify_base_as_bg
 * @widget: a valid GtkWidget
 *
 * Modify the widget's base background (used by list and
 * text widgets) to look like an ordinary widget background.
 * This is usefull if a list, text or similar widget shouldn't
 * stand out as such, e.g. when the GtkTextView widget displaying
 * a long non-editable text should look similar to a GtkLabel.
 */
void
gxk_widget_modify_base_as_bg (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (GTK_WIDGET_REALIZED (widget))
    style_modify_base_as_bg (widget);
  if (!gxk_signal_handler_pending (widget, "realize", G_CALLBACK (style_modify_base_as_bg), NULL))
    g_signal_connect_after (widget, "realize", G_CALLBACK (style_modify_base_as_bg), NULL);
}

static gboolean
expose_bg_clear (GtkWidget      *widget,
		 GdkEventExpose *event)
{
  gtk_paint_flat_box (widget->style, widget->window, GTK_STATE_NORMAL,
		      GTK_SHADOW_NONE, &event->area, widget, "base", 0, 0, -1, -1);

  return FALSE;
}

/**
 * gxk_widget_force_bg_clear
 * @widget: a valid GtkWidget
 *
 * Enforce drawing of a widget's background.
 * Some widgets do not explicitely draw their background,
 * but simply draw themsleves on top of their parent's
 * background. This function forces the widget into
 * drawing its background according to its style settings.
 */
void
gxk_widget_force_bg_clear (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_set_redraw_on_allocate (widget, TRUE);
  if (!gxk_signal_handler_pending (widget, "expose_event", G_CALLBACK (expose_bg_clear), NULL))
    g_signal_connect (widget, "expose_event", G_CALLBACK (expose_bg_clear), NULL);
}

static gboolean
gxk_activate_accel_group (GtkWidget     *widget,
			  GdkEventKey   *event,
			  GtkAccelGroup *accel_group)
{
  GdkModifierType accel_mods = event->state;
  guint accel_key = event->keyval;
  gboolean was_handled = FALSE;
  if (gtk_accelerator_valid (accel_key, accel_mods))
    {
      gchar *accel_name = gtk_accelerator_name (accel_key, (accel_mods & gtk_accelerator_get_default_mod_mask ()));
      GQuark accel_quark = g_quark_from_string (accel_name);
      guint signal_accel_activate = g_signal_lookup ("accel_activate", GTK_TYPE_ACCEL_GROUP);
      g_free (accel_name);
      g_signal_emit (accel_group, signal_accel_activate, accel_quark,
		     widget, accel_key, accel_mods, &was_handled);
    }
  return was_handled;
}

/**
 * gxk_widget_activate_accel_group
 * @widget:      a valid #GtkWidget
 * @accel_group: a valid #GtkAccelGroup
 *
 * Activate accelerators within accel group when @widget
 * receives key press events. This function isn't pure
 * convenience, as it works around Gtk 2.2 not exporting
 * _gtk_accel_group_activate(), _gtk_accel_group_attach()
 * or _gtk_accel_group_detach().
 */
void
gxk_widget_activate_accel_group (GtkWidget     *widget,
				 GtkAccelGroup *accel_group)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (accel_group)
    {
      g_return_if_fail (GTK_IS_ACCEL_GROUP (accel_group));

      gtk_accel_group_lock (accel_group);
      g_signal_connect_data (widget, "key_press_event",
			     G_CALLBACK (gxk_activate_accel_group),
			     g_object_ref (accel_group),
			     (GClosureNotify) g_object_unref, 0);
    }
}

/**
 * gxk_size_group
 * @sgmode: size group mode, one of %GTK_SIZE_GROUP_NONE,
 *          %GTK_SIZE_GROUP_HORIZONTAL, %GTK_SIZE_GROUP_VERTICAL or
 *	    %GTK_SIZE_GROUP_BOTH
 * @...:    %NULL terminated list of widgets to group together
 *
 * Group horizontal and/or vertical resizing behaviour of
 * widgets. See gtk_size_group_set_mode() on the effect of
 * the various grouping modes.
 */
void
gxk_size_group (GtkSizeGroupMode sgmode,
		gpointer         first_widget,
		...)
{
  if (first_widget)
    {
      GtkWidget *widget = first_widget;
      GtkSizeGroup *sgroup = gtk_size_group_new (sgmode);
      va_list args;

      va_start (args, first_widget);
      while (widget)
	{
	  gtk_size_group_add_widget (sgroup, widget);
	  widget = va_arg (args, GtkWidget*);
	}
      va_end (args);
      g_object_unref (sgroup);
    }
}

/**
 * gxk_tree_spath_index0
 * @strpath: stringified #GtkTreePath
 *
 * Return index[0] of @strpath. Usefull for paths in lists,
 * where index[0] usually corresponds to the nth row.
 */
gint
gxk_tree_spath_index0 (const gchar *strpath)
{
  GtkTreePath *path = strpath ? gtk_tree_path_new_from_string (strpath) : NULL;
  gint row = -1;
  if (path)
    {
      if (gtk_tree_path_get_depth (path) > 0)
	row = gtk_tree_path_get_indices (path)[0];
      gtk_tree_path_free (path);
    }
  return row;
}

/**
 * gxk_tree_path_prev
 * @path: valid #GtkTreePath
 *
 * Workaround for gtk_tree_path_prev() which corrupts memory
 * if called on empty paths (up to version 2.2 at least).
 */
gboolean
gxk_tree_path_prev (GtkTreePath *path)
{
  if (path && gtk_tree_path_get_depth (path) < 1)
    return FALSE;
  return gtk_tree_path_prev (path);
}

/**
 * gxk_tree_view_add_column
 * @tree_view: valid #GtkTreeView
 * @position:  column position (or -1 to append)
 * @column:    valid #GtkTreeViewColumn
 * @cell:      valid #GtkCellRenderer
 * @...:       attribute mappings
 *
 * Appends @cell to @column and adds @column
 * to @tree_view at the specified @position.
 * This function takes a %NULL-terminated list
 * of attribute mappings consisting of a string
 * and a guint, mapping cell attributes to
 * model columns as documented in
 * gtk_tree_view_column_add_attribute().
 */
guint
gxk_tree_view_add_column (GtkTreeView       *tree_view,
			  gint               position,
			  GtkTreeViewColumn *column,
			  GtkCellRenderer   *cell,
			  const gchar       *attrib_name,
			  ...)
{
  guint n_cols;
  va_list var_args;

  g_return_val_if_fail (GTK_IS_TREE_VIEW (tree_view), 0);
  g_return_val_if_fail (GTK_IS_TREE_VIEW_COLUMN (column), 0);
  g_return_val_if_fail (column->tree_view == NULL, 0);
  g_return_val_if_fail (GTK_IS_CELL_RENDERER (cell), 0);

  g_object_ref (column);
  g_object_ref (cell);
  gtk_object_sink (GTK_OBJECT (column));
  gtk_object_sink (GTK_OBJECT (cell));
  gtk_tree_view_column_pack_start (column, cell, TRUE);

  va_start (var_args, attrib_name);
  while (attrib_name)
    {
      guint col = va_arg (var_args, guint);

      gtk_tree_view_column_add_attribute (column, cell, attrib_name, col);
      attrib_name = va_arg (var_args, const gchar*);
    }
  va_end (var_args);

  n_cols = gtk_tree_view_insert_column (tree_view, column, position);

  g_object_unref (column);
  g_object_unref (cell);

  return n_cols;
}

/**
 * gxk_tree_view_append_text_columns
 * @tree_view: valid #GtkTreeView
 * @n_cols:    number of columns to append
 * @...:       column arguments
 *
 * Add @n_cols new columns with text cells to
 * @tree_view (a short hand version for multiple
 * calls to gxk_tree_view_add_text_column()).
 * Per column, the caller needs to
 * supply a #guint, a string, a #gdouble and another
 * string. The Arguments are used as model column
 * number (for the text to be displayed), the column
 * specific flags, the horizontal cell alignment
 * (between 0 and 1) and the column title respectively.
 *
 * The @column_flags argument is a combination of letters that
 * are able to switch certain characteristics on or of,
 * currently supported are:
 * @* %F - column is fixed in sizing;
 * @* %A - column resizes automatically;
 * @* %G - columns sizing behaviour is grow only;
 * @* %S - column is sortable;
 * @* %s - column is unsortable;
 * @* %O - column is reorderable;
 * @* %o - column is not reorderable;
 * @* %R - column is user-resizable;
 * @* %r - column is not user-resizable;
 * @* %P - add extra padding between multiple cells of the same column;
 * @* %p - cancel a previous %P flag;
 * @* %# - automatically popup dialogs for popup cell renderers.
 */
void
gxk_tree_view_append_text_columns (GtkTreeView *tree_view,
				   guint        n_cols,
				   ...)
{
  va_list var_args;

  g_return_if_fail (GTK_IS_TREE_VIEW (tree_view));

  va_start (var_args, n_cols);
  while (n_cols--)
    {
      guint col = va_arg (var_args, guint);
      gchar *column_flags = va_arg (var_args, gchar*);
      gfloat xalign = va_arg (var_args, gdouble);
      gchar *title = va_arg (var_args, gchar*);

      gxk_tree_view_add_text_column (tree_view, col, column_flags,
				     xalign, title, NULL,
				     NULL, NULL, 0);
    }
  va_end (var_args);
}

static void
tree_view_add_column (GtkTreeView  *tree_view,
		      guint	     model_column,
		      gdouble       xalign,
		      const gchar  *title,
		      const gchar  *tooltip,
		      gpointer      callback1,
		      gpointer      callback2,
		      gpointer      data,
		      GConnectFlags cflags,
		      guint         column_type,
		      const gchar  *dcolumn_flags,
		      const gchar  *ucolumn_flags)
{
  GtkCellRenderer *cell = NULL;
  const gchar *prop = NULL;
  GtkTreeViewColumn *tcol;
  GtkTreeViewColumnSizing sizing = GTK_TREE_VIEW_COLUMN_GROW_ONLY;
  gboolean reorderable = FALSE, resizable = TRUE, sortable = FALSE, auto_popup = FALSE;
  gchar *p, *column_flags = g_strconcat (" ", dcolumn_flags, ucolumn_flags, NULL);
  guint fixed_width = 0, padding = 0;

  for (p = column_flags; *p; p++)
    switch (*p)
      {
      case 'F':	/* column is fixed in sizing */
	sizing = GTK_TREE_VIEW_COLUMN_FIXED;
	fixed_width = 120;
	break;
      case 'A':	/* autosizing columns */
	sizing = GTK_TREE_VIEW_COLUMN_AUTOSIZE;
	break;
      case 'G':	/* grow-only columns */
	sizing = GTK_TREE_VIEW_COLUMN_GROW_ONLY;
	break;
      case 'S':	/* sortable columns */
	sortable = TRUE;
	break;
      case 's':	/* unsortable columns */
	sortable = FALSE;
	break;
      case 'O':	/* reorderable columns */
	reorderable = TRUE;
	break;
      case 'o':	/* non-reorderable columns */
	reorderable = TRUE;
	break;
      case 'R':	/* resizable columns */
	resizable = TRUE;
	break;
      case 'r':	/* non-resizable columns */
	resizable = FALSE;
	break;
      case 'P':	/* add cell padding */
	padding += 3;
	break;
      case 'p':	/* add cell padding */
	padding = MAX (padding, 3) - 3;
	break;
      case '#':	/* auto popup popup cell renderers */
	auto_popup = TRUE;
	break;
      }

  switch (column_type)
    {
    case 1:	/* text */
      /* GTKFIX: we use a popup cell renderer for text cells to work around focus problems in gtk */
      cell = g_object_new (GXK_TYPE_CELL_RENDERER_POPUP,
			   "show_button", FALSE,
			   "xalign", xalign,
			   "editable", callback1 != NULL,
			   NULL);
      prop = "text";
      if (callback1)
	g_signal_connect_data (cell, "edited", G_CALLBACK (callback1), data, NULL, cflags);
      break;
    case 2:	/* popup */
      cell = g_object_new (GXK_TYPE_CELL_RENDERER_POPUP,
			   "xalign", xalign,
			   "editable", callback1 != NULL,
                           "auto_popup", auto_popup,
			   NULL);
      prop = "text";
      if (callback1)
	g_signal_connect_data (cell, "edited", G_CALLBACK (callback1), data, NULL, cflags);
      if (callback2)
	g_signal_connect_data (cell, "popup", G_CALLBACK (callback2), data, NULL, cflags);
      break;
    case 3:
      cell = g_object_new (GTK_TYPE_CELL_RENDERER_TOGGLE,
			   /* "radio", radio_indicator, */
			   "activatable", callback1 != NULL,
			   NULL);
      prop = "active";
      if (callback1)
	g_signal_connect_data (cell, "toggled", G_CALLBACK (callback1), data, NULL, cflags);
      break;
    }
  gxk_tree_view_add_column (tree_view, -1,
			    tcol = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
						 "title", title,
						 "sizing", sizing,
						 "resizable", resizable,
						 "reorderable", reorderable,
						 NULL),
			    cell,
			    prop, model_column,
			    NULL);
  if (fixed_width)
    gtk_tree_view_column_set_fixed_width (tcol, fixed_width);
  if (padding)
    gtk_tree_view_column_set_spacing (tcol, padding);
  if (sortable && GTK_IS_TREE_SORTABLE (gtk_tree_view_get_model (tree_view)))
    gtk_tree_view_column_set_sort_column_id (tcol, model_column);
  if (tooltip)
    gxk_tree_view_column_set_tip_title (tcol, title, tooltip);
  gtk_tree_view_column_set_alignment (tcol, xalign);
  g_free (column_flags);
}

/**
 * gxk_tree_view_add_text_column
 * @tree_view:        valid #GtkTreeView
 * @model_column:     model column
 * @column_flags:     column flags
 * @xalign:	      horizontal text alignment
 * @title:            column title
 * @tooltip:          column tooltip
 * @edited_callback:  notification callback 
 * @data:             data passed in to toggled_callback
 * @cflags:           connection flags
 *
 * Add a new column with text cell to a @tree_view.
 * The @model_column indicates the column number
 * of the tree model containing the text to be
 * displayed, the @column_flags toggle specific
 * column characteristics (see
 * gxk_tree_view_append_text_columns() for details)
 * and @xalign controls the horizontal cell alignment
 * (between 0 and 1).
 * If non-%NULL, @edited_callback(@data) is connected
 * with @cflags (see g_signal_connect_data()) to the
 * "::edited" signal of the text cell and the cell is
 * made editable.
 */
void
gxk_tree_view_add_text_column (GtkTreeView  *tree_view,
                               guint         model_column,
			       const gchar  *column_flags,
                               gdouble       xalign,
                               const gchar  *title,
                               const gchar  *tooltip,
                               gpointer      edited_callback,
                               gpointer      data,
                               GConnectFlags cflags)
{
  g_return_if_fail (GTK_IS_TREE_VIEW (tree_view));

  return tree_view_add_column (tree_view, model_column, xalign, title, tooltip,
                               edited_callback, NULL, data, cflags,
			       1, "", column_flags);
}

/**
 * gxk_tree_view_add_popup_column
 * @tree_view:        valid #GtkTreeView
 * @model_column:     model column
 * @xalign:	      horizontal text alignment
 * @title:            column title
 * @tooltip:          column tooltip
 * @edited_callback:  edit notification callback 
 * @popup_callback:   popup notification callback 
 * @data:             data passed in to toggled_callback
 * @cflags:           connection flags
 *
 * Add a text column with popup facility, similar to
 * gxk_tree_view_add_text_column(). This function takes
 * an additional argument @popup_callback() which is
 * called when the user clicks on the cells "popup"
 * button.
 */
void
gxk_tree_view_add_popup_column (GtkTreeView  *tree_view,
				guint	      model_column,
				const gchar  *column_flags,
				gdouble       xalign,
				const gchar  *title,
				const gchar  *tooltip,
				gpointer      edited_callback,
				gpointer      popup_callback,
				gpointer      data,
				GConnectFlags cflags)
{
  g_return_if_fail (GTK_IS_TREE_VIEW (tree_view));

  return tree_view_add_column (tree_view, model_column, xalign, title, tooltip,
			       edited_callback, popup_callback, data, cflags,
			       2, "", column_flags);
}

/**
 * gxk_tree_view_add_toggle_column
 * @tree_view:        valid #GtkTreeView
 * @model_column:     model column
 * @xalign:	      horizontal text alignment
 * @title:            column title
 * @tooltip:          column tooltip
 * @toggled_callback: notification callback 
 * @data:             data passed in to toggled_callback
 * @cflags:           connection flags
 *
 * Add a toggle button column, similar
 * to gxk_tree_view_add_text_column(), however
 * the model column is expected to be of type
 * %G_TYPE_BOOLEAN, and instead of an @edited_callback(),
 * this function has a @toggled_callback(@data) callback
 * which is connected to the "toggled" signal of
 * the new cell.
 */
void
gxk_tree_view_add_toggle_column (GtkTreeView  *tree_view,
				 guint         model_column,
				 const gchar  *column_flags,
				 gdouble       xalign,
				 const gchar  *title,
				 const gchar  *tooltip,
				 gpointer      toggled_callback,
				 gpointer      data,
				 GConnectFlags cflags)
{
  g_return_if_fail (GTK_IS_TREE_VIEW (tree_view));

  return tree_view_add_column (tree_view, model_column, xalign, title, tooltip,
			       toggled_callback, NULL, data, cflags,
			       3, "A", column_flags);
}

void
gxk_tree_view_set_editable (GtkTreeView *tview,
                            gboolean     maybe_editable)
{
  GList *clist = gtk_tree_view_get_columns (tview);
  GtkTreeViewColumn *tcol = g_list_pop_head (&clist);
  while (tcol)
    {
      GList *rlist = gtk_tree_view_column_get_cell_renderers (tcol);
      GtkCellRenderer *cell = g_list_pop_head (&rlist);
      while (cell)
        {
          GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (cell), "editable");
          if (pspec && g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (pspec), G_TYPE_BOOLEAN))
            {
              gboolean is_editable, was_editable = g_object_get_long (cell, "editable");
              g_object_get (cell, "editable", &is_editable, NULL);
              if (maybe_editable && was_editable && !is_editable)
                g_object_set (cell, "editable", TRUE, NULL);
              else if (is_editable)
                {
                  if (!was_editable)
                    g_object_set_long (cell, "editable", TRUE);
                  if (!maybe_editable)
                    g_object_set (cell, "editable", FALSE, NULL);
                }
            }
          cell = g_list_pop_head (&rlist);
        }
      tcol = g_list_pop_head (&clist);
    }
}

static void
fixup_tcolumn_title (GtkWidget   *widget,
		     const gchar *tooltip)
{
  while (!GTK_IS_BUTTON (widget))
    widget = widget->parent;
  if (GTK_IS_BUTTON (widget))
    gtk_tooltips_set_tip (GXK_TOOLTIPS, widget, tooltip, NULL);
}

/**
 * gxk_tree_view_column_set_tip_title
 * @tree_column: valid #GtkTreeViewColumn
 * @title:       column title
 * @tooltip:     column tooltip
 *
 * Set a tree view column title and its tooltip.
 * This is a bug workaroud for missing tree view
 * column API to set tooltips, so the column title
 * or column title widget shouldn't be changed
 * after calling this function.
 */
void
gxk_tree_view_column_set_tip_title (GtkTreeViewColumn   *tree_column,
				    const gchar         *title,
				    const gchar		*tooltip)
{
  GtkWidget *label;

  g_return_if_fail (GTK_IS_TREE_VIEW_COLUMN (tree_column));

  gtk_tree_view_column_set_title (tree_column, title);
  label = g_object_new (GTK_TYPE_LABEL,
			"visible", TRUE,
			"label", title,
			"xalign", 0.5,
			NULL);
  g_signal_connect_data (label, "map",
			 G_CALLBACK (fixup_tcolumn_title), g_strdup (tooltip), (GClosureNotify) g_free,
			 0);
  gtk_tree_view_column_set_widget (tree_column, label);
}

/**
 * gxk_tree_selection_select_spath
 * @selection: GtkTreeSelection to modify
 * @str_path:  a stringified GtkTreePath
 *
 * Select the row denoted by @str_path.
 */
void
gxk_tree_selection_select_spath (GtkTreeSelection *selection,
				 const gchar      *str_path)
{
  GtkTreePath *path;

  g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (str_path != NULL);

  path = gtk_tree_path_new_from_string (str_path);
  gtk_tree_selection_select_path (selection, path);
  gtk_tree_path_free (path);
}

/**
 * gxk_tree_selection_unselect_spath
 * @selection: GtkTreeSelection to modify
 * @str_path:  a stringified GtkTreePath
 *
 * Unselect the row denoted by @str_path.
 */
void
gxk_tree_selection_unselect_spath (GtkTreeSelection *selection,
				   const gchar      *str_path)
{
  GtkTreePath *path;

  g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
  g_return_if_fail (str_path != NULL);

  path = gtk_tree_path_new_from_string (str_path);
  gtk_tree_selection_unselect_path (selection, path);
  gtk_tree_path_free (path);
}

/**
 * gxk_tree_selection_select_ipath
 * @selection: GtkTreeSelection to modify
 * @...:       GtkTreePath indices
 *
 * Select the row denoted by the path to be constructed from the
 * -1 terminated indices.
 */
void
gxk_tree_selection_select_ipath (GtkTreeSelection *selection,
				 gint              first_index,
				 ...)
{
  GtkTreePath *path;
  va_list args;
  gint i;

  g_return_if_fail (GTK_IS_TREE_SELECTION (selection));

  path = gtk_tree_path_new ();
  i = first_index;
  va_start (args, first_index);
  while (i != -1)
    {
      gtk_tree_path_append_index (path, i);
      i = va_arg (args, gint);
    }
  va_end (args);
  gtk_tree_selection_select_path (selection, path);
  gtk_tree_path_free (path);
}

/**
 * gxk_tree_selection_unselect_ipath
 * @selection: GtkTreeSelection to modify
 * @...:       GtkTreePath indices
 *
 * Select the row denoted by the path to be constructed from the
 * -1 terminated indices.
 */
void
gxk_tree_selection_unselect_ipath (GtkTreeSelection *selection,
				   gint              first_index,
				   ...)
{
  GtkTreePath *path;
  va_list args;
  gint i;

  g_return_if_fail (GTK_IS_TREE_SELECTION (selection));

  path = gtk_tree_path_new ();
  i = first_index;
  va_start (args, first_index);
  while (i != -1)
    {
      gtk_tree_path_append_index (path, i);
      i = va_arg (args, gint);
    }
  va_end (args);
  gtk_tree_selection_unselect_path (selection, path);
  gtk_tree_path_free (path);
}

static GSList           *browse_selection_queue = NULL;
static guint             browse_selection_handler_id = 0;
static GtkTreeSelection *browse_selection_ignore = NULL;

static void
browse_selection_weak_notify (gpointer data,
			      GObject *object)
{
  browse_selection_queue = g_slist_remove (browse_selection_queue, object);
}

static gboolean
browse_selection_handler (gpointer data)
{
  GtkTreeSelection *selection;
  GDK_THREADS_ENTER ();
  selection = g_slist_pop_head (&browse_selection_queue);
  while (selection)
    {
      GtkTreeIter iter;
      gboolean needs_sel;
      g_object_weak_unref (G_OBJECT (selection), browse_selection_weak_notify, selection);
      needs_sel = (gtk_tree_selection_get_mode (selection) == GTK_SELECTION_BROWSE &&
		   !gtk_tree_selection_get_selected (selection, NULL, &iter));
      if (needs_sel)
	{
	  GtkTreePath *path = g_object_get_data (selection, "GxkTreeSelection-last");
	  g_object_ref (selection);
	  browse_selection_ignore = selection;
	  if (path)
	    {
	      gtk_tree_selection_select_path (selection, path);
	      path = g_object_get_data (selection, "GxkTreeSelection-last");
	      needs_sel = !gtk_tree_selection_get_selected (selection, NULL, NULL);
	      if (needs_sel && path && gxk_tree_path_prev (path))
		{
		  gtk_tree_selection_select_path (selection, path);
		  path = g_object_get_data (selection, "GxkTreeSelection-last");
		  needs_sel = !gtk_tree_selection_get_selected (selection, NULL, NULL);
		}
	      if (needs_sel && path && gtk_tree_path_up (path))
		{
		  gtk_tree_selection_select_path (selection, path);
		  path = g_object_get_data (selection, "GxkTreeSelection-last");
		  needs_sel = !gtk_tree_selection_get_selected (selection, NULL, NULL);
		}
	    }
	  if (needs_sel)
	    {
	      path = gtk_tree_path_new ();
	      gtk_tree_path_append_index (path, 0);
	      gtk_tree_selection_select_path (selection, path);
	      if (gtk_tree_selection_path_is_selected (selection, path))
		{
		  /* GTKFIX: this triggeres an assertion on empty sort models */
		  gtk_tree_view_set_cursor (gtk_tree_selection_get_tree_view (selection),
					    path, NULL, FALSE);
		}
	      gtk_tree_path_free (path);
	    }
	  else
	    gtk_tree_view_set_cursor (gtk_tree_selection_get_tree_view (selection),
				      path, NULL, FALSE);
	  browse_selection_ignore = NULL;
	  g_object_unref (selection);
	}
      selection = g_slist_pop_head (&browse_selection_queue);
    }
  browse_selection_handler_id = 0;
  GDK_THREADS_LEAVE ();
  return FALSE;
}

static void
browse_selection_changed (GtkTreeSelection *selection)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
      g_object_set_data_full (selection, "GxkTreeSelection-last", path, gtk_tree_path_free);
    }
  if (browse_selection_ignore != selection &&
      !g_slist_find (browse_selection_queue, selection))
    {
      g_object_weak_ref (G_OBJECT (selection), browse_selection_weak_notify, selection);
      browse_selection_queue = g_slist_prepend (browse_selection_queue, selection);
      if (browse_selection_handler_id == 0)
	browse_selection_handler_id = g_idle_add_full (G_PRIORITY_DEFAULT, browse_selection_handler, NULL, NULL);
    }
}

/**
 * gxk_tree_selection_force_browse
 * @selection: GtkTreeSelection to watch
 * @model:     tree model used with @selection
 *
 * Watch deletion and insertions into empty trees to
 * ensure valid selections across these events.
 */
void
gxk_tree_selection_force_browse (GtkTreeSelection *selection,
				 GtkTreeModel     *model)
{
  g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
  if (model)
    g_return_if_fail (GTK_IS_TREE_MODEL (model));

  if (!gxk_signal_handler_pending (selection, "changed", G_CALLBACK (browse_selection_changed), selection))
    g_signal_connect_data (selection, "changed", G_CALLBACK (browse_selection_changed), selection, NULL, 0);
  if (model && !gxk_signal_handler_pending (model, "row-inserted", G_CALLBACK (browse_selection_changed), selection))
    g_signal_connect_object (model, "row-inserted", G_CALLBACK (browse_selection_changed), selection, G_CONNECT_SWAPPED);
  browse_selection_changed (selection);
}

/**
 * gxk_tree_view_get_bin_window_pos
 * @tree: valid #GtkTreeView
 * @x_p:  x position
 * @y_p:  y position
 *
 * Retrieve the position of the bin window (row display area) of
 * a #GtkTreeView widget once it's realized.
 */
void
gxk_tree_view_get_bin_window_pos (GtkTreeView *tree,
				  gint        *x_p,
				  gint        *y_p)
{
  GdkWindow *window;
  gint ax = 0, ay = 0;

  g_return_if_fail (GTK_IS_TREE_VIEW (tree));

  window = gtk_tree_view_get_bin_window (tree);
  if (window)
    {
      GdkWindow *ancestor = GTK_WIDGET (tree)->window;
      /* accumulate offsets up to widget->window */
      while (window != ancestor)
	{
	  gint dx, dy;
	  gdk_window_get_position (window, &dx, &dy);
	  ax += dx;
	  ay += dy;
	  window = gdk_window_get_parent (window);
	}
    }
  if (x_p)
    *x_p = ax;
  if (y_p)
    *y_p = ay;
}

/**
 * gxk_tree_view_get_row_area
 * @tree:     valid #GtkTreeView
 * @row:      row to retrieve area coordinates for
 * @y_p:      y position of @row
 * @height_p: height of @row
 *
 * Retrieve the position and height of a row of a
 * #GtkTreeView widget within its bin window.
 */
gboolean
gxk_tree_view_get_row_area (GtkTreeView *tree,
                            gint         row,
                            gint        *y_p,
                            gint        *height_p)
{
  GdkRectangle rect;

  g_return_val_if_fail (GTK_IS_TREE_VIEW (tree), FALSE);

  if (row < 0)
    {
      rect.y = 0;
      rect.width = 0;
    }
  else
    {
      GtkTreePath *path = gtk_tree_path_new ();
      gtk_tree_path_append_index (path, row);
      gtk_tree_view_get_background_area (tree, path, NULL, &rect);
      gtk_tree_path_free (path);
    }
  if (y_p)
    *y_p = rect.y;
  if (height_p)
    *height_p = rect.height;
  if (rect.height != 0)
    {
      GtkTreeModel *model = gtk_tree_view_get_model (tree);
      GtkTreeIter iter;
      if (!gtk_tree_model_get_iter_first (model, &iter))
	return FALSE;	/* empty tree */
      return TRUE;	/* valid row */
    }
  return FALSE;		/* no row */
}

/**
 * gxk_tree_view_focus_row
 * @tree:     valid #GtkTreeView
 * @row:      row to focus
 *
 * Force focus to @row, causes automatic selection of
 * @row in browse mode.
 */
void
gxk_tree_view_focus_row (GtkTreeView *tree,
			 gint         row)
{
  GtkTreePath *path;

  g_return_if_fail (GTK_IS_TREE_VIEW (tree));

  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, row);
  gtk_tree_view_set_cursor (tree, path, NULL, FALSE);
  gtk_tree_path_free (path);
}

/**
 * gxk_tree_view_is_row_selected
 * @tree:    valid #GtkTreeView
 * @row:     row to test
 * @RETURNS: whether @row is selected
 *
 * Check whether @row in @tree is selected.
 */
gboolean
gxk_tree_view_is_row_selected (GtkTreeView *tree,
			       gint         row)
{
  g_return_val_if_fail (GTK_IS_TREE_VIEW (tree), FALSE);

  if (row >= 0)
    {
      GtkTreePath *path = gtk_tree_path_new ();
      gboolean selected;
      gtk_tree_path_append_index (path, row);
      selected = gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (tree),
						      path);
      gtk_tree_path_free (path);
      return selected;
    }
  return FALSE;
}

/**
 * gxk_tree_view_get_selected_row
 * @tree:    valid #GtkTreeView
 * @RETURNS: first selected row or -1
 *
 * Retrieve the selected row in browse mode (for other
 * selection modes, return the first selected row if any).
 */
gint
gxk_tree_view_get_selected_row (GtkTreeView *tree)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint row = -1;

  g_return_val_if_fail (GTK_IS_TREE_VIEW (tree), -1);

  if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (tree),
				       &model, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
      if (gtk_tree_path_get_depth (path) > 0)
	row = gtk_tree_path_get_indices (path)[0];
      gtk_tree_path_free (path);
    }
  return row;
}

/**
 * gxk_tree_view_get_row_from_coord
 * @tree:    valid #GtkTreeView
 * @y:       bin window y coordinate
 * @row_p:   row pointed to by @y
 * @RETURNS: whether y lies within the visible area
 *
 * Retrieve the row within which @y lies. If @y lies
 * outside the visible area, the row is clamped to
 * visible rows.
 */
gboolean
gxk_tree_view_get_row_from_coord (GtkTreeView *tree,
				  gint         y,
				  gint        *row_p)
{
  GtkTreePath *path = NULL;
  gint row = -1, outside = FALSE;

  g_return_val_if_fail (GTK_IS_TREE_VIEW (tree), FALSE);

  if (!gtk_tree_view_get_path_at_pos (tree, GTK_WIDGET (tree)->allocation.width / 2,
				      y, &path, NULL, NULL, NULL))
    {
      GdkRectangle rect;
      /* clamp row to visible area */
      gtk_tree_view_get_visible_rect (tree, &rect); /* rect.y is buffer relative */
      if (y <= 0)	/* above visible area */
	gtk_tree_view_get_path_at_pos (tree, GTK_WIDGET (tree)->allocation.width / 2,
				       0, &path, NULL, NULL, NULL);
      else if (!gtk_tree_view_get_path_at_pos (tree, GTK_WIDGET (tree)->allocation.width / 2,
					       MAX (rect.height, 1) - 1, &path,
					       NULL, NULL, NULL))
	{
	  GtkTreePath *last_path = NULL;
	  gint offs = 0;
	  /* no row at end of visible area, find last row */
	  while (offs < rect.height)
	    {
	      gint i = (offs + rect.height) >> 1;
	      if (gtk_tree_view_get_path_at_pos (tree, GTK_WIDGET (tree)->allocation.width / 2,
						 i, &path, NULL, NULL, NULL))
		{
		  if (last_path)
		    gtk_tree_path_free (last_path);
		  last_path = path;
		  /* search in lower half */
		  offs = i + 1;
		}
	      else /* search in upper half */
		rect.height = i;
	    }
	  path = last_path;
	}
      outside = TRUE;
    }
  if (path)
    {
      if (gtk_tree_path_get_depth (path) > 0)
	row = gtk_tree_path_get_indices (path)[0];
      gtk_tree_path_free (path);
    }
  if (row_p)
    *row_p = row;
  return row >= 0 && !outside;
}

/**
 * gxk_notebook_append
 * @notebook: a valid notebook
 * @child:    a valid parent-less widget
 * @label:    notebook page name
 *
 * Add a new page containing @child to @notebook,
 * naming the page @label.
 */
void
gxk_notebook_append (GtkNotebook *notebook,
		     GtkWidget   *child,
		     const gchar *label)
{
  g_return_if_fail (GTK_IS_NOTEBOOK (notebook));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (label != NULL);

  gtk_notebook_append_page (notebook, child, g_object_new (GTK_TYPE_LABEL,
							   "visible", TRUE,
							   "label", label,
							   NULL));
}

/**
 * gxk_signal_handler_pending
 * @instance:        object instance with signals
 * @detailed_signal: signal name
 * @callback:        custom callback function
 * @data:            callback data
 * @RETURNS:         whether callback is connected
 *
 * Find out whether a specific @callback is pending for a
 * specific signal on an instance. @detailed_signal may be
 * %NULL to act as a wildcard. %TRUE is returned if
 * the @callback is found, %FALSE otherwise.
 */
gboolean
gxk_signal_handler_pending (gpointer     instance,
			    const gchar *detailed_signal,
			    GCallback    callback,
			    gpointer     data)
{
  guint signal_id;
  GQuark detail = 0;

  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE (instance), FALSE);
  g_return_val_if_fail (detailed_signal != NULL, FALSE);
  g_return_val_if_fail (callback != NULL, FALSE);

  if (detailed_signal && g_signal_parse_name (detailed_signal, G_TYPE_FROM_INSTANCE (instance),
					      &signal_id, &detail, FALSE))
    {
      if (g_signal_handler_find (instance, (G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA |
					    (detail ? G_SIGNAL_MATCH_DETAIL : 0)),
				 signal_id, detail, NULL, callback, data) != 0)
	return TRUE;
    }
  else if (!detailed_signal)
    {
      if (g_signal_handler_find (instance, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
				 0, 0, NULL, callback, data) != 0)
	return TRUE;
    }
  else
    g_warning ("%s: signal name \"%s\" is invalid for instance `%p'", G_STRLOC, detailed_signal, instance);
  return FALSE;
}


/* --- derivation convenience --- */
typedef struct {
  gpointer *parent_class_p;
  /* GObject */
  gpointer finalize, dispose;
  /* GtkObject */
  gpointer destroy;
} TypeMethods;

static void
gxk_generic_type_class_init (gpointer class,
			     gpointer class_data)
{
  TypeMethods *tm = class_data;
  if (tm)
    *tm->parent_class_p = g_type_class_peek_parent (class);
  if (tm && G_IS_OBJECT_CLASS (class))
    {
      GObjectClass *oclass = G_OBJECT_CLASS (class);
      if (tm->finalize)
	oclass->finalize = tm->finalize;
      if (tm->dispose)
	oclass->dispose = tm->dispose;
    }
  if (tm && GTK_IS_OBJECT_CLASS (class))
    {
      GtkObjectClass *oclass = GTK_OBJECT_CLASS (class);
      if (tm->destroy)
	oclass->destroy = tm->destroy;
    }
}

/**
 * gxk_object_derive
 *
 * Derive a new object type, giving a list of
 * methods which are implemented for this new object type.
 */
GType
gxk_object_derive (GType          parent_type,
		   const gchar   *name,
		   gpointer      *parent_class_p,
		   guint          instance_size,
		   guint          class_size,
		   GxkMethodType  mtype,
		   ...)
{
  GType type = g_type_from_name (name);
  if (!type)
    {
      GTypeInfo type_info = {
	0,	/* class_size */
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gxk_generic_type_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	0,	/* instance_size */
	0,      /* n_preallocs */
	NULL,	/* instance_init */
      };
      TypeMethods tm = { NULL, };
      gpointer initfunc = NULL;
      gboolean need_cd = FALSE;
      va_list args;
      g_return_val_if_fail (G_TYPE_IS_OBJECT (parent_type), 0);
      type_info.class_size = class_size;
      type_info.instance_size = instance_size;
      va_start (args, mtype);
      while (mtype)
	{
	  gpointer func = va_arg (args, gpointer);
	  switch (mtype)
	    {
	    case GXK_METHOD_INIT:		initfunc = func;	break;
	    case GXK_METHOD_FINALIZE:		tm.finalize = func;	break;
	    case GXK_METHOD_DISPOSE:		tm.dispose = func;	break;
	    case GXK_METHOD_DESTROY:		tm.destroy = func;	break;
	    default:	g_error ("invalid method type: %d", mtype);
	    }
	  need_cd |= mtype != GXK_METHOD_INIT;
	  mtype = va_arg (args, GxkMethodType);
	}
      va_end (args);
      type_info.instance_init = initfunc;
      if (need_cd || parent_class_p)
	{
	  g_return_val_if_fail (parent_class_p != NULL, 0);
	  tm.parent_class_p = parent_class_p;
	  type_info.class_data = g_memdup (&tm, sizeof (TypeMethods));
	}
      type = g_type_register_static (parent_type, name, &type_info, 0);
    }
  return type;
}


/* --- Gtk bug fixes --- */
/**
 * gxk_cell_editable_canceled
 * @ecell:   valid #GtkCellEditable
 * @RETURNS: whether editing got aborted
 *
 * Return whether editing was canceled (currently supported
 * by GtkEntry and triggered by pressing Escape during editing).
 */
gboolean
gxk_cell_editable_canceled (GtkCellEditable *ecell)
{
  g_return_val_if_fail (GTK_IS_CELL_EDITABLE (ecell), FALSE);

  if (GTK_IS_ENTRY (ecell))
    return GTK_ENTRY (ecell)->editing_canceled;
  return FALSE;
}

/**
 * gxk_cell_editable_is_focus_handler
 * @ecell:   valid #GtkCellEditable
 * @RETURNS: returns %FALSE
 *
 * Call gtk_cell_editable_editing_done() if necessary and return %FALSE.
 * This function is meant to be used to handle "notify::is-focus" signals
 * on #GtkCellEditable widgets.
 */
void
gxk_cell_editable_is_focus_handler (GtkCellEditable *ecell)
{
  g_return_if_fail (GTK_IS_CELL_EDITABLE (ecell));

  if (!gtk_widget_is_focus (GTK_WIDGET (ecell)))
    {
      gtk_cell_editable_editing_done (ecell);
      gtk_cell_editable_remove_widget (ecell);
    }
}

static gchar*
path_fix_uline (const gchar *str)
{
  gchar *path = g_strdup (str);
  gchar *p = path, *q = path;
  if (!p)
    return NULL;
  while (*p)
    {
      if (*p == '_')
	{
	  if (p[1] == '_')
	    {
	      p++;
	      *q++ = '_';
	    }
	}
      else
	*q++ = *p;
      p++;
    }
  *q = 0;
  return path;
}

/**
 * gxk_item_factory_get_item
 * @ifactory: valid #GtkItemFactory
 * @path:     item factory path
 * @RETURNS:  menu item according to @path
 *
 * This function strips unescaped underlines ('_') from @path
 * and then calls gtk_item_factory_get_item().
 */
GtkWidget*
gxk_item_factory_get_item (GtkItemFactory *ifactory,
			   const gchar    *path)
{
  gchar *p = path_fix_uline (path);
  GtkWidget *widget = gtk_item_factory_get_item (ifactory, p);
  g_free (p);
  return widget;
}

/**
 * gxk_item_factory_get_widget
 * @ifactory: valid #GtkItemFactory
 * @path:     item factory path
 * @RETURNS:  widget according to @path
 *
 * This function strips unescaped underlines ('_') from @path
 * and then calls gtk_item_factory_get_widget().
 */
GtkWidget*
gxk_item_factory_get_widget (GtkItemFactory *ifactory,
			     const gchar    *path)
{
  gchar *p = path_fix_uline (path);
  GtkWidget *widget = gtk_item_factory_get_widget (ifactory, p);
  g_free (p);
  return widget;
}

static void
requisition_to_aux_info (GtkWidget      *widget,
			 GtkRequisition *requisition)
{
  guint width = requisition->width;
  guint height = requisition->height;

  /* we don't want the requisition to get too big */
  width = MIN (width, gdk_screen_width () / 2);
  height = MIN (height, gdk_screen_height () / 2);

  gtk_widget_set_size_request (widget, width, height);
}

/**
 * gxk_widget_proxy_requisition
 * @widget: valid #GtkWidget
 *
 * Proxy the size requisition of @widget through the
 * ::width_request and ::height_request properties.
 * This is usefull only for immediate children of a
 * #GtkScrolledWindow, to have the #GtkScrolledWindow
 * honour the widgets size requisition.
 */
void
gxk_widget_proxy_requisition (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_signal_handlers_disconnect_by_func (widget, requisition_to_aux_info, NULL);
  g_signal_connect_after (widget, "size_request", G_CALLBACK (requisition_to_aux_info), NULL);
}

GtkWidget*
gxk_file_selection_split (GtkFileSelection *fs,
			  GtkWidget       **bbox_p)
{
  GtkWidget *main_vbox;
  GtkWidget *hbox;

  g_return_val_if_fail (GTK_IS_FILE_SELECTION (fs), NULL);

  /* nuke GUI junk */
  gtk_file_selection_hide_fileop_buttons (fs);

  /* fix spacing and borders */
  gtk_container_set_border_width (GTK_CONTAINER (fs), 0);
  gtk_widget_ref (fs->main_vbox);
  gtk_container_remove (GTK_CONTAINER (fs), fs->main_vbox);
  g_object_set (fs->main_vbox,
		"spacing", 0,
		"border_width", 5,
		NULL);
  /* repack into new parent box */
  main_vbox = gtk_widget_new (GTK_TYPE_VBOX,
			      "homogeneous", FALSE,
			      "spacing", 0,
			      "border_width", 0,
			      "parent", fs,
			      "visible", TRUE,
			      NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), fs->main_vbox, TRUE, TRUE, 0);
  gtk_widget_unref (fs->main_vbox);

  /* fixup focus and default widgets */
  gtk_widget_grab_default (fs->ok_button);
  gtk_widget_grab_focus (fs->selection_entry);

  /* use an ordinary HBox as button container */
  gtk_widget_hide (fs->ok_button->parent);
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "homogeneous", TRUE,
			 "spacing", 3,
			 "border_width", 5,
			 "visible", TRUE,
			 NULL);
  if (bbox_p)
    *bbox_p = hbox;
  else
    gtk_box_pack_end (GTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_reparent (fs->ok_button, hbox);
  gtk_widget_reparent (fs->cancel_button, hbox);

  /* fix the action_area packing so we can customize children */
  gtk_box_set_child_packing (GTK_BOX (fs->action_area->parent),
			     fs->action_area,
			     FALSE, TRUE,
			     5, GTK_PACK_START);

  /* fs life cycle */
  g_signal_connect_object (main_vbox, "destroy", G_CALLBACK (gtk_widget_destroy), fs, G_CONNECT_SWAPPED);

  return main_vbox;
}

/**
 * gxk_file_selection_heal
 * @fs:      valid #GtkFileSelection
 * @RETURNS: new toplevel VBox of the file selection
 *
 * Fixup various oddities that happened to the Gtk+
 * file selection widget over time. This function
 * corrects container border widths, spacing, button
 * placement and the default and focus widgets.
 * Also, the lifetime of the file selection window
 * is tied to the returned #GtkVBox, enabling removal
 * of the #GtkVBox from it's parent and thus using the
 * file selection widgets in a custom #GtkWindow.
 */
GtkWidget*
gxk_file_selection_heal (GtkFileSelection *fs)
{
  GtkWidget *any, *main_box;

  main_box = gxk_file_selection_split (fs, NULL);

  /* add obligatory button seperator */
  any = g_object_new (GTK_TYPE_HSEPARATOR,
		      "visible", TRUE,
		      NULL);
  gtk_box_pack_end (GTK_BOX (main_box), any, FALSE, TRUE, 0);

  return main_box;
}
