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
#include "glewidgets.h"
#include "gxkmenubutton.h"
#include "gxkstock.h"
#include "gxkcellrendererpopup.h"
#include "gxkauxwidgets.h"
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
#if     GTK_CHECK_VERSION (2, 4, 0)

#if 0
static void
widget_print_ancestry (GtkWidget *widget)
{
  g_print ("%s: drawable=%d window-viewable=%d ancestry-mapped=%d\n",
           g_type_name (G_OBJECT_TYPE (widget)), GTK_WIDGET_DRAWABLE (widget),
           gdk_window_is_viewable (widget->window),
           widget_ancestry_mapped (widget));
  if (widget->parent)
    widget_print_ancestry (widget->parent);
}
#endif

static gboolean
gxk_widget_real_can_activate_accel (GtkWidget *widget, // GTKFIX: #145270, FIXME: remove this
                                    guint      signal_id)
{
  // g_print ("%s: accelerator-check:\n",  g_type_name (G_OBJECT_TYPE (widget))), widget_print_ancestry(widget);
  /* widgets must be onscreen for accels to take effect */
  return GTK_WIDGET_IS_SENSITIVE (widget) && GTK_WIDGET_DRAWABLE (widget) && gxk_widget_ancestry_viewable (widget);
}
#endif

static gboolean
ehook_container_focus_child_set (GSignalInvocationHint *ihint,
                                 guint                  n_param_values,
                                 const GValue          *param_values,
                                 gpointer               data)
{
  GtkContainer *container = g_value_get_object (param_values + 0);
  if (!container->focus_child ||
      !GTK_WIDGET_DRAWABLE (container->focus_child) ||
      gtk_container_get_focus_hadjustment (container) ||
      gtk_container_get_focus_vadjustment (container))
    return TRUE;
  /* check for scrolled window ancestor */
  GtkWidget *widget = container->focus_child->parent;
  while (widget && !GTK_IS_VIEWPORT (widget))
    widget = widget->parent;
  if (widget)   /* viewport */
    {
      GtkAdjustment *hadj = gtk_viewport_get_hadjustment (GTK_VIEWPORT (widget));
      GtkAdjustment *vadj = gtk_viewport_get_vadjustment (GTK_VIEWPORT (widget));
      if (hadj || vadj)
        {
          GtkWidget *focus_child = container->focus_child;
          while (GTK_IS_CONTAINER (focus_child) && GTK_CONTAINER (focus_child)->focus_child)
            focus_child = GTK_CONTAINER (focus_child)->focus_child;
          gint x, y;
          gtk_widget_translate_coordinates (focus_child, GTK_BIN (widget)->child, 0, 0, &x, &y);
          if (hadj)
            gtk_adjustment_clamp_page (hadj, x, x + focus_child->allocation.width);
          if (vadj)
            gtk_adjustment_clamp_page (vadj, y, y + focus_child->allocation.height);
        }
    }
  return TRUE;
}

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
#if     GTK_CHECK_VERSION (2, 4, 0)
  GtkWidgetClass *widget_class = gtk_type_class (GTK_TYPE_WIDGET);
  widget_class->can_activate_accel = gxk_widget_real_can_activate_accel;
#endif

  /* patch up scrolling+focus behaviour */
  g_type_class_unref (g_type_class_ref (GTK_TYPE_CONTAINER));   /* create static class */
  g_signal_add_emission_hook (g_signal_lookup ("set-focus-child", GTK_TYPE_CONTAINER), 0,
                              ehook_container_focus_child_set, NULL, NULL);
}

/**
 * gxk_widget_ancestry_viewable
 * @widget:  a valid %GtkWidget
 * @RETURNS: whether @widget is visible on screen
 *
 * Checks for @widget to be effectively visible on screen.
 * Note that this is not the same as gdk_window_is_viewable()
 * for the widget's window or GTK_WIDGET_DRAWABLE(), as
 * those may return true for children of notebooks, eventboxes
 * or handleboxes although the widget is not visible on screen.
 */
gboolean
gxk_widget_ancestry_viewable (GtkWidget *widget)
{
  if (!widget->window || !gdk_window_is_viewable (widget->window))
    return FALSE;
  while (widget)
    {
      if (!GTK_WIDGET_DRAWABLE (widget))        /* visible && mapped */
        return FALSE;
      widget = widget->parent;
    }
  return TRUE;
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

const gchar*
gxk_factory_path_get_leaf (const gchar *path)
{
  const gchar *last = NULL, *p = path;
  if (!path)
    return NULL;
  while (*p)
    {
      if (*p == '\\' && p[1])
        p += 1;
      else if (*p == '/')
        last = p + 1;
      p++;
    }
  return last ? last : path;
}

gchar*
gxk_factory_path_unescape_uline (const gchar *path)
{
  gchar *str = g_strdup (path), *d = str, *p = d;
  while (*p)
    {
      if (*p == '_')
        {
          p++;
          if (*p == '_')
            *d++ = *p++;
        }
      *d++ = *p++;
    }
  *d = 0;
  return str;
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

GdkColor
gdk_color_from_rgb (guint rgb_value)
{
  GdkColor c = { 0, };
  c.red = (rgb_value >> 16) & 0xff;
  c.green = (rgb_value >> 8) & 0xff;
  c.blue = rgb_value & 0xff;
  c.red *= 0x0101;
  c.green *= 0x0101;
  c.blue *= 0x0101;
  return c;
}

GdkColor
gdk_color_from_argb (guint rgb_value)
{
  GdkColor c = { 0, };
  c.red = (rgb_value >> 16) & 0xff;
  c.green = (rgb_value >> 8) & 0xff;
  c.blue = rgb_value & 0xff;
  c.red *= 0x0101;
  c.green *= 0x0101;
  c.blue *= 0x0101;
  return c;
}

GdkColor
gdk_color_from_rgba (guint rgb_value)
{
  GdkColor c = { 0, };
  c.red = (rgb_value >> 24) & 0xff;
  c.green = (rgb_value >> 16) & 0xff;
  c.blue = (rgb_value >> 8) & 0xff;
  c.red *= 0x0101;
  c.green *= 0x0101;
  c.blue *= 0x0101;
  return c;
}

void
gdk_draw_hline (GdkDrawable            *drawable,
                GdkGC                  *gc,
                gint                    x,
                gint                    y,
                gint                    width)
{
  if (width > 0)
    gdk_draw_line (drawable, gc, x, y, x + width - 1, y);
}

void
gdk_draw_vline (GdkDrawable            *drawable,
                GdkGC                  *gc,
                gint                    x,
                gint                    y,
                gint                    height)
{
  if (height > 0)
    gdk_draw_line (drawable, gc, x, y, x, y + height - 1);
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

static gboolean
idle_showraiser (gpointer data)
{
  GtkWidget **widget_p = data;
  GDK_THREADS_ENTER ();
  if (GTK_IS_WIDGET (*widget_p))
    {
      GtkWidget *widget = *widget_p;
      gtk_signal_disconnect_by_func (GTK_OBJECT (widget),
				     GTK_SIGNAL_FUNC (gtk_widget_destroyed),
				     widget_p);
      gtk_widget_show (widget);
      if (GTK_WIDGET_REALIZED (widget) && !widget->parent)
        gdk_window_raise (widget->window);
    }
  g_free (widget_p);
  GDK_THREADS_LEAVE ();
  return FALSE;
}

/**
 * gxk_idle_showraise
 * @widget: a valid #GtkWidget
 *
 * Defers showing and raising this widget like gxk_widget_showraise()
 * until the next idle handler is run. This is useful if other things
 * are pending which need to be processed first, for instance hiding
 * other toplevels or constructing remaining parts of the widget hierarchy.
 */
void
gxk_idle_showraise (GtkWidget *widget)
{
  GtkWidget **widget_p;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  widget_p = g_new (GtkWidget*, 1);

  *widget_p = widget;
  g_object_connect (widget, "signal::destroy", gtk_widget_destroyed, widget_p, NULL);
  gtk_idle_add_priority (GTK_PRIORITY_RESIZE - 1, idle_showraiser, widget_p);
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
      if (GTK_WIDGET_DRAWABLE (*widget_p) && GTK_IS_WINDOW (*widget_p))
        gdk_window_raise ((*widget_p)->window);
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
 * is run. This is useful if other things are pending
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
 * is run. This is useful if other things are pending
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

GtkWidget*
gxk_notebook_create_tabulator (const gchar *label_text,
                               const gchar *stock_image,
                               const gchar *tooltip)
{
  GtkWidget *ev = g_object_new (GTK_TYPE_EVENT_BOX, NULL);
  GtkWidget *image = gtk_image_new();
  GtkWidget *label = g_object_new (GTK_TYPE_LABEL, NULL);
  GtkWidget *box = g_object_new (GTK_TYPE_HBOX, "parent", ev, NULL);
  gtk_box_pack_start (GTK_BOX (box), image, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, TRUE, 0);
  gtk_widget_show_all (ev);
  gxk_notebook_change_tabulator (ev, label_text, stock_image, tooltip);
  return ev;
}

void
gxk_notebook_change_tabulator (GtkWidget        *tabulator,
                               const gchar      *label_text,
                               const gchar      *stock_image,
                               const gchar      *tooltip)
{
  if (GTK_IS_EVENT_BOX (tabulator))
    {
      GtkWidget *image = gxk_parent_find_descendant (tabulator, GTK_TYPE_IMAGE);
      GtkWidget *label = gxk_parent_find_descendant (tabulator, GTK_TYPE_LABEL);
      if (image && label)
        {
          if (label_text && strcmp (gtk_label_get_text (GTK_LABEL (label)), label_text) != 0)
            gtk_label_set_text (GTK_LABEL (label), label_text);
          if (label_text)
            gtk_widget_show (label);
          else
            gtk_widget_hide (label);
          gchar *ostock = NULL;
          GtkIconSize isize = 0;
          gtk_image_get_stock (GTK_IMAGE (image), &ostock, &isize);
          if (stock_image && (!ostock || strcmp (ostock, stock_image) != 0 || isize != GXK_ICON_SIZE_TABULATOR))
            gtk_image_set_from_stock (GTK_IMAGE (image), stock_image, GXK_ICON_SIZE_TABULATOR);
          if (stock_image)
            gtk_widget_show (image);
          else
            gtk_widget_hide (image);
          gxk_widget_set_tooltip (tabulator, tooltip);
        }
    }
}


/**
 * gxk_notebook_append
 * @notebook:   a valid notebook
 * @child:      a valid parent-less widget
 * @label:      notebook page name
 * @fillexpand: whether the tab label should expand
 *
 * Add a new page containing @child to @notebook,
 * naming the page @label.
 */
void
gxk_notebook_append (GtkNotebook *notebook,
                     GtkWidget   *child,
                     const gchar *tab_text,
                     gboolean     fillexpand)
{
  gtk_container_add (GTK_CONTAINER (notebook), child);
  gtk_notebook_set_tab_label_text (notebook, child, tab_text);
  gtk_notebook_set_menu_label_text (notebook, child, tab_text);
  gtk_notebook_set_tab_label_packing (notebook, child, fillexpand, fillexpand, GTK_PACK_START);
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

static void
vseparator_space_request (GtkWidget      *widget,
                          GtkRequisition *requisition,
                          gpointer        data)
{
  guint i = GPOINTER_TO_INT (data);
  requisition->width = i * widget->style->xthickness;
}

/**
 * gxk_vseparator_space_new
 * @draw_seperator: enable visible vertical seperator
 * @RETURNS:        visible vertical space/seperator widget
 *
 * Create a vertical seperator widget. @draw_seperator indicates
 * whether the seperator should be amount to simple space or not.
 */
GtkWidget*
gxk_vseparator_space_new (gboolean draw_seperator)
{
  GtkWidget *widget = g_object_new (draw_seperator ? GTK_TYPE_VSEPARATOR : GTK_TYPE_ALIGNMENT,
                                    "visible", TRUE,
                                    NULL);
  g_signal_connect (widget, "size-request", G_CALLBACK (vseparator_space_request),
                    GUINT_TO_POINTER (draw_seperator ? 2 + 1 + 2 : 3));
  return widget;
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
 * gxk_notebook_descendant_get_page
 * @widget:  valid #GtkWidget
 * @RETURNS: notebook page widget or %NULL
 *
 * Find the innermost notebook page widget that contains @widget.
 */
GtkWidget*
gxk_notebook_descendant_get_page (GtkWidget *widget)
{
  while (widget->parent && !GTK_IS_NOTEBOOK (widget->parent))
    widget = widget->parent;
  return widget->parent ? widget : NULL;
}

/**
 * gxk_notebook_descendant_get_tab
 * @widget:  valid #GtkWidget
 * @RETURNS: notebook page tab widget or %NULL
 *
 * Find the innermost notebook page widget that contains @widget
 * and return its tabulator widget.
 */
GtkWidget*
gxk_notebook_descendant_get_tab (GtkWidget *widget)
{
  widget = gxk_notebook_descendant_get_page (widget);
  return widget ? gtk_notebook_get_tab_label (GTK_NOTEBOOK (widget->parent), widget) : NULL;
}

/**
 * gtk_box_get_nth_child
 * @box:     a valid GtkBox
 * @pos:     position of the requested child
 * @RETURNS: a child of @box or %NULL
 *
 * Find the child at position @pos (0 indicates the first child) of
 * @box and return it. To retrieve the last xchild of @box, -1
 * may be passed as @pos.
 */
GtkWidget*
gtk_box_get_nth_child (GtkBox *box,
                       gint    pos)
{
  g_return_val_if_fail (GTK_IS_BOX (box), NULL);

  GList *child = box->children;

  if (child)
    while (child->next && pos--)
      child = child->next;

  GtkBoxChild *child_info = child && pos <= 0 ? child->data : NULL;
  return child_info ? child_info->widget : NULL;
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
 * This function is useful to produce the exact same effect
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

typedef struct {
  GtkWidget *child;
  GType      type;
} DescendantSearch;

static void
find_descendant_callback (GtkWidget *child,
                          gpointer   data)
{
  DescendantSearch *dsearch = data;
  if (!dsearch->child)
    {
      if (g_type_is_a (G_OBJECT_TYPE (child), dsearch->type))
        dsearch->child = child;
      else if (GTK_IS_CONTAINER (child))
        gtk_container_foreach (GTK_CONTAINER (child), find_descendant_callback, dsearch);
    }
}

GtkWidget*
gxk_parent_find_descendant (GtkWidget        *parent,
                            GType             descendant_type)
{
  DescendantSearch dsearch = { NULL, };
  dsearch.type = descendant_type;
  if (GTK_IS_CONTAINER (parent))
    gtk_container_foreach (GTK_CONTAINER (parent), find_descendant_callback, &dsearch);
  return dsearch.child;
}

enum {
  STYLE_MODIFY_FG_AS_SENSITIVE,
  STYLE_MODIFY_BASE_AS_BG,
  STYLE_MODIFY_BG_AS_BASE,
  STYLE_MODIFY_NORMAL_BG_AS_BASE,
  STYLE_MODIFY_BG_AS_ACTIVE,
};

static void
widget_modify_style (GtkWidget *widget)
{
  guint style_modify = g_object_get_int (widget, "gxk-style-modify-type");
  switch (style_modify)
    {
      GtkRcStyle *rc_style;
      guint i;
    case STYLE_MODIFY_FG_AS_SENSITIVE:
      rc_style = gtk_rc_style_new ();
      rc_style->color_flags[GTK_STATE_INSENSITIVE] = GTK_RC_FG;
      rc_style->fg[GTK_STATE_INSENSITIVE].red = widget->style->fg[GTK_STATE_NORMAL].red;
      rc_style->fg[GTK_STATE_INSENSITIVE].green = widget->style->fg[GTK_STATE_NORMAL].green;
      rc_style->fg[GTK_STATE_INSENSITIVE].blue = widget->style->fg[GTK_STATE_NORMAL].blue;
      gtk_widget_modify_style (widget, rc_style);
      gtk_rc_style_unref (rc_style);
      break;
    case STYLE_MODIFY_BASE_AS_BG:
      rc_style = gtk_rc_style_new ();
      rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BASE;
      rc_style->base[GTK_STATE_NORMAL].red = widget->style->bg[GTK_STATE_NORMAL].red;
      rc_style->base[GTK_STATE_NORMAL].green = widget->style->bg[GTK_STATE_NORMAL].green;
      rc_style->base[GTK_STATE_NORMAL].blue = widget->style->bg[GTK_STATE_NORMAL].blue;
      gtk_widget_modify_style (widget, rc_style);
      break;
    case STYLE_MODIFY_BG_AS_BASE:
      rc_style = gtk_widget_get_modifier_style (widget);
      for (i = GTK_STATE_NORMAL; i <= GTK_STATE_INSENSITIVE; i++)
        {
          rc_style->color_flags[i] = GTK_RC_BG;
          rc_style->bg[i].red = widget->style->base[i].red;
          rc_style->bg[i].green = widget->style->base[i].green;
          rc_style->bg[i].blue = widget->style->base[i].blue;
        }
      gtk_widget_modify_style (widget, rc_style);
      break;
    case STYLE_MODIFY_NORMAL_BG_AS_BASE:
      rc_style = gtk_widget_get_modifier_style (widget);
      for (i = GTK_STATE_NORMAL; i <= GTK_STATE_INSENSITIVE; i++)
        if (i == GTK_STATE_NORMAL || i == GTK_STATE_INSENSITIVE)
          {
            rc_style->color_flags[i] = GTK_RC_BG;
            rc_style->bg[i].red = widget->style->base[i].red;
            rc_style->bg[i].green = widget->style->base[i].green;
            rc_style->bg[i].blue = widget->style->base[i].blue;
          }
      gtk_widget_modify_style (widget, rc_style);
      break;
    case STYLE_MODIFY_BG_AS_ACTIVE:
      rc_style = gtk_widget_get_modifier_style (widget);
      for (i = GTK_STATE_NORMAL; i <= GTK_STATE_INSENSITIVE; i++)
        if (i == GTK_STATE_NORMAL)
          {
            rc_style->color_flags[i] = GTK_RC_BG;
            rc_style->bg[i].red = widget->style->bg[GTK_STATE_ACTIVE].red;
            rc_style->bg[i].green = widget->style->bg[GTK_STATE_ACTIVE].green;
            rc_style->bg[i].blue = widget->style->bg[GTK_STATE_ACTIVE].blue;
          }
      gtk_widget_modify_style (widget, rc_style);
      break;
    }
}

/**
 * gxk_widget_modify_as_title
 * @widget: a valid GtkWidget
 *
 * Modify the widget and it's style, so that it is insensitive,
 * but doesn't quite look that way. Useful for inactive title
 * menu items in menus (@widget should be the menu item's label).
 */
void
gxk_widget_modify_as_title (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  
  if (!gxk_signal_handler_exists (widget, "realize", G_CALLBACK (widget_modify_style), NULL))
    {
      g_object_set_int (widget, "gxk-style-modify-type", STYLE_MODIFY_FG_AS_SENSITIVE);
      g_signal_connect_after (widget, "realize", G_CALLBACK (widget_modify_style), NULL);
      if (GTK_WIDGET_REALIZED (widget))
        widget_modify_style (widget);
      gtk_widget_set_sensitive (widget, FALSE);
      if (!gxk_signal_handler_exists (widget, "realize", G_CALLBACK (gxk_widget_make_insensitive), NULL))
        g_signal_connect_after (widget, "realize", G_CALLBACK (gxk_widget_make_insensitive), NULL);
    }
}

/**
 * gxk_widget_modify_bg_as_base
 * @widget: a valid GtkWidget
 *
 * Modify the widget's background to look like the background
 * of a text or list widget (usually white). This is useful
 * if a hbox or similar widget is used to "simulate" a list
 * or text widget.
 */
void
gxk_widget_modify_bg_as_base (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (!gxk_signal_handler_exists (widget, "realize", G_CALLBACK (widget_modify_style), NULL))
    {
      g_object_set_int (widget, "gxk-style-modify-type", STYLE_MODIFY_BG_AS_BASE);
      g_signal_connect_after (widget, "realize", G_CALLBACK (widget_modify_style), NULL);
      if (GTK_WIDGET_REALIZED (widget))
        widget_modify_style (widget);
    }
}

/**
 * gxk_widget_modify_normal_bg_as_base
 * @widget: a valid GtkWidget
 *
 * Modify the widget's background like gxk_widget_modify_bg_as_base()
 * does, as long as the widget isn't activated or selected.
 */
void
gxk_widget_modify_normal_bg_as_base (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (!gxk_signal_handler_exists (widget, "realize", G_CALLBACK (widget_modify_style), NULL))
    {
      g_object_set_int (widget, "gxk-style-modify-type", STYLE_MODIFY_NORMAL_BG_AS_BASE);
      g_signal_connect_after (widget, "realize", G_CALLBACK (widget_modify_style), NULL);
      if (GTK_WIDGET_REALIZED (widget))
        widget_modify_style (widget);
    }
}

/**
 * gxk_widget_modify_base_as_bg
 * @widget: a valid GtkWidget
 *
 * Modify the widget's base background (used by list and
 * text widgets) to look like an ordinary widget background.
 * This is useful if a list, text or similar widget shouldn't
 * stand out as such, e.g. when the GtkTextView widget displaying
 * a long non-editable text should look similar to a GtkLabel.
 */
void
gxk_widget_modify_base_as_bg (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (!gxk_signal_handler_exists (widget, "realize", G_CALLBACK (widget_modify_style), NULL))
    {
      g_object_set_int (widget, "gxk-style-modify-type", STYLE_MODIFY_BASE_AS_BG);
      g_signal_connect_after (widget, "realize", G_CALLBACK (widget_modify_style), NULL);
      if (GTK_WIDGET_REALIZED (widget))
        widget_modify_style (widget);
    }
}

/**
 * gxk_widget_modify_bg_as_active
 * @widget: a valid GtkWidget
 *
 * Modify the widget's background to look like the background
 * of depressed button.
 */
void
gxk_widget_modify_bg_as_active (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (!gxk_signal_handler_exists (widget, "realize", G_CALLBACK (widget_modify_style), NULL))
    {
      g_object_set_int (widget, "gxk-style-modify-type", STYLE_MODIFY_BG_AS_ACTIVE);
      g_signal_connect_after (widget, "realize", G_CALLBACK (widget_modify_style), NULL);
      if (GTK_WIDGET_REALIZED (widget))
        widget_modify_style (widget);
    }
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
  if (!gxk_signal_handler_exists (widget, "expose_event", G_CALLBACK (expose_bg_clear), NULL))
    g_signal_connect (widget, "expose_event", G_CALLBACK (expose_bg_clear), NULL);
}

/**
 * gxk_widget_set_tooltip
 * @widget:  a valid GtkWidget
 * @tooltip: descriptive tooltip
 *
 * As a convenience function, this sets the @tooltip for @widget
 * on %GXK_TOOLTIPS, if @widget is supposed to have tooltips
 * according to the system configuration and also sets the latent tooltip.
 */
void
gxk_widget_set_tooltip (GtkWidget   *widget,
                        const gchar *tooltip)
{
  gxk_widget_set_latent_tooltip (widget, tooltip);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, widget, tooltip, NULL);
}

/**
 * gxk_widget_set_latent_tooltip
 * @widget:  a valid GtkWidget
 * @tooltip: descriptive tooltip
 *
 * Set the latent tooltip for this widget. A latent tooltip
 * will not be shown on mouse over for this widget. Instead
 * it can be querried by other widgets via
 * gxk_widget_get_latent_tooltip() to be shown when appropriate.
 * For instance, GxkMenuButton shows the latent tooltip of its
 * currently selected menu item.
 */
void
gxk_widget_set_latent_tooltip (GtkWidget   *widget,
                               const gchar *tooltip)
{
  if (tooltip && !tooltip[0])
    tooltip = NULL;
  g_object_set_data_full (widget, "gxk-widget-latent-tooltip", g_strdup (tooltip), g_free);
}

/**
 * gxk_widget_get_latent_tooltip
 * @widget:  a valid GtkWidget
 * @RETUSN: descriptive tooltip
 *
 * Retrieve the latent tooltip for @widget. See
 * gxk_widget_set_latent_tooltip() for the purpose of latent
 * tooltips.
 */
const gchar*
gxk_widget_get_latent_tooltip (GtkWidget *widget)
{
  return g_object_get_data (widget, "gxk-widget-latent-tooltip");
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
 * Return index[0] of @strpath. Useful for paths in lists,
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
 * gxk_tree_model_get_iter
 *
 * This function is a replacement for gtk_tree_model_get_iter()
 * for Gtk+-2.4. For sort models, gtk_tree_model_get_iter() can
 * erroneously return %TRUE which is corrected by calling this
 * function instead.
 */
gboolean
gxk_tree_model_get_iter (GtkTreeModel          *tree_model,
                         GtkTreeIter           *iter,
                         GtkTreePath           *path)
{
  if (!gtk_tree_model_get_iter (tree_model, iter, path))
    return FALSE;
  while (GTK_IS_TREE_MODEL_SORT (tree_model))
    {
      GtkTreeIter dummy;
      tree_model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (tree_model));
      if (!gtk_tree_model_get_iter (tree_model, &dummy, path))
        return FALSE;
    }
  return TRUE;
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
      cell = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
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
                           "auto-popup", auto_popup,
			   "editable", callback1 || callback2,
                           "text-editing", callback1 != NULL,
                           "popup-editing", callback2 != NULL,
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

void
gxk_tree_view_select_index (GtkTreeView           *tview,
                            guint                  index)
{
  GtkTreeSelection *tsel;
  GtkTreePath *path;
  g_return_if_fail (GTK_IS_TREE_VIEW (tview));
  tsel = gtk_tree_view_get_selection (tview);
  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, index);
  gtk_tree_selection_select_path (tsel, path);
  if (gtk_tree_selection_path_is_selected (tsel, path))
    {
      /* GTKFIX: this can trigger an assertion on empty sort models */
      gtk_tree_view_set_cursor (tview, path, NULL, FALSE);
    }
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
  if (!gxk_signal_handler_exists (selection, "changed", G_CALLBACK (browse_selection_changed), selection))
    g_signal_connect_data (selection, "changed", G_CALLBACK (browse_selection_changed), selection, NULL, 0);
  if (model && !gxk_signal_handler_exists (model, "row-inserted", G_CALLBACK (browse_selection_changed), selection))
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
 * @tree:         valid #GtkTreeView
 * @row:          row to retrieve area coordinates for
 * @y_p:          y position of @row
 * @height_p:     height of @row
 * @content_area: whether the cell background area or content area is returned
 *
 * Retrieve the position and height of a row of a
 * #GtkTreeView widget within its bin window.
 */
gboolean
gxk_tree_view_get_row_area (GtkTreeView *tree,
                            gint         row,
                            gint        *y_p,
                            gint        *height_p,
                            gboolean     content_area)
{
  GdkRectangle rect = { 0, 0, 0, 0 };

  g_return_val_if_fail (GTK_IS_TREE_VIEW (tree), FALSE);

  if (row >= 0)
    {
      GtkTreePath *path = gtk_tree_path_new ();
      gtk_tree_path_append_index (path, row);
      if (content_area)
        gtk_tree_view_get_cell_area (tree, path, NULL, &rect);
      else
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
 * gxk_signal_handler_exists
 * @instance:        object instance with signals
 * @detailed_signal: signal name
 * @callback:        custom callback function
 * @data:            callback data
 * @RETURNS:         whether callback is connected
 *
 * Find out whether a specific @callback is connected to a
 * specific signal on an instance, the callback may be blocked.
 * @detailed_signal may be
 * %NULL to act as a wildcard. %TRUE is returned if
 * the @callback is found, %FALSE otherwise.
 */
gboolean
gxk_signal_handler_exists (gpointer     instance,
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

/**
 * gxk_signal_handler_pending
 * @instance:        object instance with signals
 * @detailed_signal: signal name
 * @callback:        custom callback function
 * @data:            callback data
 * @RETURNS:         whether callback is connected
 *
 * Find out whether a specific @callback is pending
 * (connected and unblocked) for a
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
      if (g_signal_handler_find (instance, (G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA | G_SIGNAL_MATCH_UNBLOCKED |
					    (detail ? G_SIGNAL_MATCH_DETAIL : 0)),
				 signal_id, detail, NULL, callback, data) != 0)
	return TRUE;
    }
  else if (!detailed_signal)
    {
      if (g_signal_handler_find (instance, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA | G_SIGNAL_MATCH_UNBLOCKED,
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
 * gxk_item_factory_sensitize
 * @ifactory:  valid #GtkItemFactory
 * @path:      item factory path
 * @sensitive: whether menu item should be sensitive
 * @RETURNS:   menu item according to @path
 *
 * This function turns the menu item found via
 * gxk_item_factory_get_item() (in-)sensitive
 * according to @sensitive. Additional checks
 * are performed before making a menu item sensitive
 * to avoid showing e.g. empty submenus.
 */
GtkWidget*
gxk_item_factory_sensitize (GtkItemFactory  *ifactory,
                            const gchar     *path,
                            gboolean         sensitive)
{
  GtkWidget *item = gxk_item_factory_get_item (ifactory, path);
  if (item)
    {
      if (GTK_IS_MENU_ITEM (item) && sensitive)
        {
          GtkWidget *menu = gtk_menu_item_get_submenu ((GtkMenuItem*) item);
          if (menu && GTK_IS_MENU (menu))
            {
              GList *list = gtk_container_get_children (GTK_CONTAINER (menu));
              if (list)
                g_list_free (list);
              else
                sensitive = FALSE;
            }
        }
      gtk_widget_set_sensitive (item, sensitive);
    }
  return item;
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
			 GtkRequisition *requisition,
                         gpointer        data)
{
  double *xyscale = data;
  guint width = requisition->width;
  guint height = requisition->height;
  
  /* patch up requisition since gtk tends to allocate the viewport with the
   * requested size minus vscrollbar->width or minus hscrollbar->height.
   */
  if (GTK_IS_SCROLLED_WINDOW (widget->parent))
    {
      GtkScrolledWindow *scwin = GTK_SCROLLED_WINDOW (widget->parent);
      GtkRequisition requisition;
      if (scwin->vscrollbar && scwin->vscrollbar_policy != GTK_POLICY_NEVER)
        {
          gtk_widget_size_request (scwin->vscrollbar, &requisition);
          width += requisition.width;
        }
      if (scwin->hscrollbar && scwin->hscrollbar_policy != GTK_POLICY_NEVER)
        {
          gtk_widget_size_request (scwin->hscrollbar, &requisition);
          height += requisition.height;
        }
    }
  
  /* we constrain the requisition to a fraction of the screen size */
  width = MIN (width, gdk_screen_width () * xyscale[0]);
  height = MIN (height, gdk_screen_height () * xyscale[1]);

  gtk_widget_set_size_request (widget,
                               xyscale[0] < 0 ? -1 : width,
                               xyscale[1] < 0 ? -1 : height);
}

/**
 * gxk_widget_proxy_requisition
 * @widget: valid #GtkWidget
 * @xscale: fractional factor for screen width
 * @yscale: fractional factor for screen height
 *
 * Proxy the size requisition of @widget through the ::width-request
 * and ::height-request properties. This is useful only for immediate
 * children of a #GtkScrolledWindow (e.g. a #GtkViewport), to have
 * the #GtkScrolledWindow honour the widgets size requisition.
 * If @xscale or @yscale is passed as -1, the corresponding dimension
 * ::width-request or ::height-request is left unset.
 * If @xscale or @yscale is passed a value between 0 and +1, it is
 * interpreted as a fraction of the screen width or screen height to
 * constrain the corresponding requested dimension.
 */
void
gxk_widget_proxy_requisition (GtkWidget *widget,
                              gdouble    xscale,
                              gdouble    yscale)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_signal_handlers_disconnect_by_func (widget, requisition_to_aux_info, NULL);
  double xyscale[2] = { xscale, yscale };
  if (xscale >= 0 || yscale >= 0)
    g_signal_connect_data (widget, "size-request", G_CALLBACK (requisition_to_aux_info),
                           g_memdup (xyscale, sizeof (xyscale)), (GClosureNotify) g_free, G_CONNECT_AFTER);
}

static void
scrolled_window_size_request_spare_space (GtkScrolledWindow *scwin,
                                          GtkRequisition    *requisition)
{
  /* we fixup the additional sizes that scrolled windows add
   * to the child requisition if the h/v scroll policy is NEVER.
   */
  if (GTK_BIN (scwin)->child &&
      (scwin->hscrollbar_policy == GTK_POLICY_NEVER ||
       scwin->vscrollbar_policy == GTK_POLICY_NEVER))
    {
      GtkRequisition child_requisition = { 0, 0 };
      gtk_widget_size_request (GTK_BIN (scwin)->child, &child_requisition);
      if (scwin->hscrollbar_policy == GTK_POLICY_NEVER && !scwin->vscrollbar_visible)
        requisition->width = child_requisition.width;
      if (scwin->vscrollbar_policy == GTK_POLICY_NEVER && !scwin->hscrollbar_visible)
        requisition->height = child_requisition.height;
    }
}

/**
 * gxk_scrolled_window_spare_space
 * @scwin:  valid #GtkScrolledWindow
 * A normal #GtkScrolledWindow requests extra space for a horizontal scrollbar
 * if the vertical scroll policy is set to %GTK_POLICY_NEVER and vice versa,
 * regardless of whether the scrollbars have to be shown or not.
 * This function patches up this behaviour to spare the extra requested space
 * from the outer scrolled window requisition if possible (that is, if the
 * corresponding scrollbar is not currently visible).
 */
void
gxk_scrolled_window_spare_space (GtkScrolledWindow*scwin)
{
  if (!gxk_signal_handler_exists (scwin, "size-request", G_CALLBACK (scrolled_window_size_request_spare_space), NULL))
    g_signal_connect (scwin, "size-request", G_CALLBACK (scrolled_window_size_request_spare_space), NULL);
}

/**
 * gxk_scrolled_window_unspare_space
 * @scwin:  valid #GtkScrolledWindow
 * Undo the effect of a call to gxk_scrolled_window_spare_space().
 */
void
gxk_scrolled_window_unspare_space (GtkScrolledWindow*scwin)
{
  if (gxk_signal_handler_exists (scwin, "size-request", G_CALLBACK (scrolled_window_size_request_spare_space), NULL))
    g_signal_handlers_disconnect_by_func (scwin, G_CALLBACK (scrolled_window_size_request_spare_space), NULL);
}

/**
 * gxk_scrolled_window_create
 * @child:       valid #GtkWidget
 * @shadow_type: shadow around the #GtkViewport
 * @xrequest:    fractional factor for screen width
 * @yrequest:    fractional factor for screen height
 * @spare_space: whether to call gxk_scrolled_window_spare_space()
 * @RETURNS:     the newly created #GtkScrolledWindow
 *
 * Create a #GtkScrolledWindow with a #GtkViewport as parent of @child.
 * The @xrequest and @yrequest arguments are passed on
 * to gxk_widget_proxy_requisition().
 */
GtkWidget*
gxk_scrolled_window_create (GtkWidget        *child,
                            GtkShadowType     shadow_type,
                            gdouble           xrequest,
                            gdouble           yrequest,
                            gboolean          spare_space)
{
  GtkWidget *scwin = g_object_new (GXK_TYPE_SCROLLED_WINDOW,
                                   "visible", TRUE,
                                   "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
                                   "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
                                   "spare-space", spare_space,
                                   NULL);
  child = gtk_widget_get_toplevel (child);
  GtkWidget *viewport = g_object_new (GTK_TYPE_VIEWPORT,
                                      "visible", TRUE,
                                      "shadow_type", shadow_type,
                                      "hadjustment", gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scwin)),
                                      "vadjustment", gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scwin)),
                                      "parent", scwin,
                                      "child", child,
                                      NULL);
  gxk_widget_proxy_requisition (viewport, xrequest, yrequest);
  return scwin;
}

static void
request_hclient_height (GtkWidget      *widget,
                        GtkRequisition *requisition,
                        GtkWidget      *client)
{
  GtkRequisition client_requisition;
  gtk_widget_size_request (client, &client_requisition);
  requisition->height = client_requisition.width;
}

/**
 * gxk_widget_request_hclient_height
 * @widget: valid #GtkWidget
 * @client: valid #GtkWidget
 *
 * Request the horizontal size of @client as height
 * for @widget.
 */
void
gxk_widget_request_hclient_height (GtkWidget       *widget,
                                   GtkWidget       *client)
{
  g_signal_handlers_disconnect_by_func (widget, request_hclient_height, client);
  g_signal_connect_after (widget, "size_request", G_CALLBACK (request_hclient_height), client);
}

static void
request_vclient_width (GtkWidget      *widget,
                       GtkRequisition *requisition,
                       GtkWidget      *client)
{
  GtkRequisition client_requisition;
  gtk_widget_size_request (client, &client_requisition);
  requisition->width = client_requisition.height;
}

/**
 * gxk_widget_request_vclient_width
 * @widget: valid #GtkWidget
 * @client: valid #GtkWidget
 *
 * Request the vertical size of @client as width
 * for @widget.
 */
void
gxk_widget_request_vclient_width (GtkWidget       *widget,
                                  GtkWidget       *client)
{
  g_signal_handlers_disconnect_by_func (widget, request_vclient_width, client);
  g_signal_connect_after (widget, "size_request", G_CALLBACK (request_vclient_width), client);
}

/**
 * gxk_widget_has_ancestor
 * @widget:   valid #GtkWidget
 * @ancestor: valid #GtkWidget
 * @RETURNS:  whether @ancestor is ancestor of @widget
 *
 * This function checks whether @widget and @ancestor are equal,
 * or whether @ancestor is an ancestor of @widget, in the same
 * way gtk_widget_is_ancestor() tests it.
 */
gboolean
gxk_widget_has_ancestor (gpointer widget,
                         gpointer ancestor)
{
  GtkWidget *w = widget, *a = ancestor;
  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (GTK_IS_WIDGET (ancestor), FALSE);

  while (w)
    {
      if (w == a)
        return TRUE;
      w = w->parent;
    }
  return FALSE;
}

/**
 * gxk_widget_regulate
 * @widget:    valid #GtkWidget
 * @sensitive: whether @widget should be sensitive
 * @active:    whether @widget should be active
 *
 * Regulate a widgets state. The @sensitive parameter
 * controls sensitivity like gtk_widget_set_sensitive()
 * and @active controls whether the widget is active
 * like gtk_toggle_button_set_active() or
 * gtk_check_menu_item_set_active().
 * For menu items, the menu item is also made the
 * active widget in its parent menu, possibly affecting
 * option menus.
 */
void
gxk_widget_regulate (GtkWidget      *widget,
                     gboolean        sensitive,
                     gboolean        active)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  if (((GObject*) widget)->ref_count > 0)
    {
      GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (widget), "active");
      g_object_freeze_notify (G_OBJECT (widget));
      gtk_widget_set_sensitive (widget, sensitive);
      if (pspec && pspec->value_type == G_TYPE_BOOLEAN)
        {
          GValue value = { 0, };
          g_value_init (&value, G_TYPE_BOOLEAN);
          g_value_set_boolean (&value, active);
          g_object_set_property ((GObject*) widget, "active", &value);
          g_value_unset (&value);
        }
      if (active && GTK_IS_MENU_ITEM (widget) && widget->parent && GTK_IS_MENU (widget->parent))
        {
          GtkMenu *menu = GTK_MENU (widget->parent);
          guint nth = g_list_index (GTK_MENU_SHELL (menu)->children, widget);
          GtkWidget *awidget = gtk_menu_get_attach_widget (menu);
          gtk_menu_set_active (menu, nth);
          if (GTK_IS_OPTION_MENU (awidget))
            gtk_option_menu_set_history (GTK_OPTION_MENU (awidget), nth);
          if (GXK_IS_MENU_BUTTON (awidget))
            gxk_menu_button_update (GXK_MENU_BUTTON (awidget));
        }
      g_object_thaw_notify (G_OBJECT (widget));
    }
}

/**
 * gxk_widget_regulate_uses_active
 * @widget:  valid #GtkWidget
 * @RETURNS: %TRUE if gxk_widget_regulate() uses @active for @widget
 *
 * Check whether gxk_widget_regulate() will actually make
 * use of its @active argument for @widget. If not,
 * %FALSE is returned, and gxk_widget_regulate() is
 * fully equivalent to just gtk_widget_set_sensitive().
 */
gboolean
gxk_widget_regulate_uses_active (GtkWidget *widget)
{
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (widget), "active");
  return GTK_IS_MENU_ITEM (widget) || (pspec && pspec->value_type == G_TYPE_BOOLEAN);
}

/**
 * gxk_window_get_menu_accel_group
 * @window:  valid #GtkWindow
 * @RETURNS: valid #GtkAccelGroup
 *
 * This function hands out an accel group for @window
 * specifically targeted at holding accelerators of
 * menu items in this window.
 */
GtkAccelGroup*
gxk_window_get_menu_accel_group (GtkWindow *window)
{
  GtkAccelGroup *agroup = g_object_get_data (window, "GxkWindow-menu-accel-group");
  if (!agroup)
    {
      agroup = gtk_accel_group_new ();
      gtk_window_add_accel_group (window, agroup);
      g_object_set_data_full (window, "GxkWindow-menu-accel-group", agroup, g_object_unref);
    }
  return agroup;
}

static GdkGeometry*
window_get_geometry (GtkWindow *window)
{
  GdkGeometry *geometry = g_object_get_data (window, "gxk-GdkGeometry");
  if (!geometry)
    {
      geometry = g_new0 (GdkGeometry, 1);
      geometry->width_inc = 1;
      geometry->height_inc = 1;
      g_object_set_data_full (window, "gxk-GdkGeometry", geometry, g_free);
    }
  return geometry;
}

void
gxk_window_set_geometry_min_width (GtkWindow       *window,
                                   guint            min_width)
{
  GdkGeometry *geometry = window_get_geometry (window);
  if (geometry->min_width != min_width)
    {
      geometry->min_width = min_width;
      gtk_window_set_geometry_hints (GTK_WINDOW (window), NULL, geometry, GDK_HINT_MIN_SIZE | GDK_HINT_RESIZE_INC);
    }
}

void
gxk_window_set_geometry_min_height (GtkWindow       *window,
                                    guint            min_height)
{
  GdkGeometry *geometry = window_get_geometry (window);
  if (geometry->min_height != min_height)
    {
      geometry->min_height = min_height;
      gtk_window_set_geometry_hints (GTK_WINDOW (window), NULL, geometry, GDK_HINT_MIN_SIZE | GDK_HINT_RESIZE_INC);
    }
}

void
gxk_window_set_geometry_width_inc (GtkWindow       *window,
                                   guint            width_increment)
{
  GdkGeometry *geometry = window_get_geometry (window);
  if (geometry->width_inc != width_increment)
    {
      geometry->width_inc = width_increment;
      gtk_window_set_geometry_hints (GTK_WINDOW (window), NULL, geometry, GDK_HINT_MIN_SIZE | GDK_HINT_RESIZE_INC);
    }
}

void
gxk_window_set_geometry_height_inc (GtkWindow       *window,
                                    guint            height_increment)
{
  GdkGeometry *geometry = window_get_geometry (window);
  if (geometry->height_inc != height_increment)
    {
      geometry->height_inc = height_increment;
      gtk_window_set_geometry_hints (GTK_WINDOW (window), NULL, geometry, GDK_HINT_MIN_SIZE | GDK_HINT_RESIZE_INC);
    }
}

guint
gxk_container_get_insertion_slot (GtkContainer *container)
{
  guint *slots = g_object_steal_data (container, "gxk-container-slots");
  guint n_slots = slots ? slots[0] : 0;
  guint n_children = 0;
  if (GTK_IS_CONTAINER (container))
    {
      GList *children = gtk_container_get_children (container);
      n_children = g_list_length (children);
      g_list_free (children);
    }
  n_slots++;
  slots = g_renew (guint, slots, 1 + n_slots);
  slots[0] = n_slots;
  slots[n_slots] = n_children;
  g_object_set_data_full (container, "gxk-container-slots", slots, g_free);
  return n_slots;
}

void
gxk_container_slot_reorder_child (GtkContainer    *container,
                                  GtkWidget       *widget,
                                  guint            sloti)
{
  guint *slots = g_object_get_data (container, "gxk-container-slots");
  guint n_slots = slots ? slots[0] : 0;
  if (sloti && sloti <= n_slots)
    {
      guint i;
      if (GTK_IS_MENU (container))
        gtk_menu_reorder_child ((GtkMenu*) container, widget, slots[sloti]);
      else if (GTK_IS_BOX (container))
        gtk_box_reorder_child ((GtkBox*) container, widget, slots[sloti]);
      else if (GTK_IS_WRAP_BOX (container))
        gtk_wrap_box_reorder_child ((GtkWrapBox*) container, widget, slots[sloti]);
      else
        return;
      for (i = sloti; i <= n_slots; i++)
        slots[i]++;
    }
}

/**
 * gxk_grab_pointer_and_keyboard
 * @window:       the window receiving the grab
 * @owner_events: if %TRUE, events will be reported relative to @window
 * @event_mask:   mask of interesting events
 * @confine_to:   limits the pointer to the specified window
 * @cursor:       cursor to use for the duration of the grab
 * @time:         event time when grab is requested
 * @RETURNS:      %TRUE if pointer and keyboard could successfully be grabbed
 *
 * This function grabs the pointer and keyboard simultaneously.
 * This is recommended over plain pointer grabs, to reduce the
 * risk of other applications (for instance the window manager)
 * aborting the current grab and leaving the application in an
 * invalid state.
 */
gboolean
gxk_grab_pointer_and_keyboard (GdkWindow    *window,
                               gboolean      owner_events,
                               GdkEventMask  event_mask,
                               GdkWindow    *confine_to,
                               GdkCursor    *cursor,
                               guint32       time)
{
  if (gdk_pointer_grab (window, owner_events, event_mask, confine_to, cursor, time) == GDK_GRAB_SUCCESS)
    {
      if (gdk_keyboard_grab (window, TRUE, time) == GDK_GRAB_SUCCESS)
        return TRUE;
      else
        gdk_display_pointer_ungrab (gdk_drawable_get_display (window), time);
    }
  return FALSE;
}

/**
 * gxk_ungrab_pointer_and_keyboard
 * @window: window pointer was previously grabed on
 *
 * This function releases a pointer and keyboard grab
 * acquired through gxk_grab_pointer_and_keyboard().
 * The @window is used to release grabs on the correct
 * display, see gdk_display_pointer_ungrab() and
 * gdk_display_keyboard_ungrab() on this.
 */
void
gxk_ungrab_pointer_and_keyboard (GdkWindow *window,
                                 guint32    time)
{
  GdkDisplay *display = window ? gdk_drawable_get_display (window) : gdk_display_get_default();
  gdk_display_pointer_ungrab (display, time);
  gdk_display_keyboard_ungrab (display, time);
}

/**
 * gxk_menu_check_sensitive
 * @menu:    valid #GtkMenu
 * @RETURNS: TRUE if @menu contains selectable items
 *
 * This function tests whether a menu contains
 * selectable menu items. It can be used to determine
 * sensitivity for menu items containing submenus.
 */
gboolean
gxk_menu_check_sensitive (GtkMenu *menu)
{
  GtkMenuShell *shell = GTK_MENU_SHELL (menu);
  GList *list;
  for (list = shell->children; list; list = list->next)
    {
      GtkWidget *child = list->data;
      if (GTK_WIDGET_VISIBLE (child) &&
          GTK_WIDGET_IS_SENSITIVE (child) &&
          !GTK_IS_TEAROFF_MENU_ITEM (child))
        return TRUE;
    }
  return FALSE;
}

static void
submenu_adjust_sensitivity (GtkMenu *menu)
{
  GtkWidget *widget = gtk_menu_get_attach_widget (menu);
  if (GTK_IS_MENU_ITEM (widget))
    {
      gboolean sensitive = gxk_menu_check_sensitive (menu);
      if (sensitive != GTK_WIDGET_SENSITIVE (widget))
        {
          GtkWidget *parent = widget->parent;
          gtk_widget_set_sensitive (widget, sensitive);
          /* push change along to parent if necessary */
          if (parent && gxk_signal_handler_pending (parent, "parent-set", G_CALLBACK (submenu_adjust_sensitivity), NULL))
            submenu_adjust_sensitivity (GTK_MENU (parent));
        }
    }
}

static void
menu_refetch_accel_group (GtkMenu *menu)
{
  GtkWidget *toplevel = gxk_widget_get_attach_toplevel ((GtkWidget*) menu);
  if (GTK_IS_WINDOW (toplevel))
    gtk_menu_set_accel_group (menu, gxk_window_get_menu_accel_group ((GtkWindow*) toplevel));
}

static void     menu_propagate_hierarchy_changed (GtkMenu *menu);

static void
menu_item_propagate_hierarchy_changed (GtkMenuItem *menu_item)
{
  GtkWidget *menu = menu_item->submenu;
  if (menu)
    menu_propagate_hierarchy_changed (GTK_MENU (menu));
}

/**
 * gxk_menu_attach_as_submenu
 * @menu:      valid #GtkMenu
 * @menu_item: valid #GtkMenuItem
 *
 * This function is a replacement for
 * gtk_menu_item_set_submenu(). It installs
 * the necessary hooks on the @menu to automatically
 * update sensitivity of @menu_item in response
 * to children being deleted or added to the @menu.
 * The rationale behind this is to avoid empty menus
 * being presented to the user.
 * Also, a propagation mechanism is set up, so @menu
 * and submenus thereof automatically fetch their
 * accelerator groups via gxk_window_get_menu_accel_group()
 * from the toplevel window.
 */
void
gxk_menu_attach_as_submenu (GtkMenu     *menu,
                            GtkMenuItem *menu_item)
{
  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (GTK_IS_MENU_ITEM (menu_item));

  gtk_menu_item_set_submenu (menu_item, GTK_WIDGET (menu));

  if (!gxk_signal_handler_exists (menu_item, "hierarchy-changed", G_CALLBACK (menu_item_propagate_hierarchy_changed), NULL))
    g_signal_connect_after (menu_item, "hierarchy-changed", G_CALLBACK (menu_item_propagate_hierarchy_changed), NULL);
  menu_item_propagate_hierarchy_changed (menu_item);

  if (!gxk_signal_handler_exists (menu, "parent-set", G_CALLBACK (submenu_adjust_sensitivity), NULL))
    g_object_connect (menu,
                      "signal_after::parent-set", submenu_adjust_sensitivity, NULL,
                      "signal_after::add", submenu_adjust_sensitivity, NULL,
                      "signal_after::remove", submenu_adjust_sensitivity, NULL,
                      NULL);
  submenu_adjust_sensitivity (menu);
}

static void
option_menu_propagate_hierarchy_changed (GtkOptionMenu *option_menu)
{
  GtkWidget *menu = option_menu->menu;
  if (menu)
    menu_propagate_hierarchy_changed (GTK_MENU (menu));
}

/**
 * gxk_option_menu_set_menu
 * @option_menu: valid #GtkOptionMenu
 * @menu:        valid #GtkMenu
 *
 * This function is a replacement for
 * gtk_option_menu_set_menu(). Similar to
 * gxk_menu_attach_as_submenu(), it sets up
 * a propagation mechanism, so @menu
 * and submenus thereof automatically fetch their
 * accelerator groups via gxk_window_get_menu_accel_group()
 * from the toplevel window.
 */
void
gxk_option_menu_set_menu (GtkOptionMenu *option_menu,
                          GtkMenu       *menu)
{
  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));
  g_return_if_fail (GTK_IS_MENU (menu));

  gtk_option_menu_set_menu (option_menu, GTK_WIDGET (menu));

  if (!gxk_signal_handler_exists (option_menu, "hierarchy-changed", G_CALLBACK (option_menu_propagate_hierarchy_changed), NULL))
    g_signal_connect_after (option_menu, "hierarchy-changed", G_CALLBACK (option_menu_propagate_hierarchy_changed), NULL);
  option_menu_propagate_hierarchy_changed (option_menu);
}

static void
menu_propagate_hierarchy_changed (GtkMenu *menu)
{
  GList *list;
  for (list = GTK_MENU_SHELL (menu)->children; list; list = list->next)
    if (gxk_signal_handler_pending (list->data, "hierarchy-changed",
                                    G_CALLBACK (menu_item_propagate_hierarchy_changed), NULL))
      menu_item_propagate_hierarchy_changed (list->data);
  menu_refetch_accel_group (menu);
}

static void
menu_widget_propagate_hierarchy_changed (GtkWidget *widget)
{
  GList *menu_list = g_object_get_data (widget, "GxkWidget-popup-menus");
  for (; menu_list; menu_list = menu_list->next)
    menu_propagate_hierarchy_changed (menu_list->data);
}

static void
popup_menus_detach (gpointer data)
{
  GSList *menu_list = data;
  while (menu_list)
    {
      GtkMenu *menu = g_slist_pop_head (&menu_list);
      if (gtk_menu_get_attach_widget (menu))
        gtk_menu_detach (menu);
    }
}

static void
popup_menu_detacher (GtkWidget *widget,
                     GtkMenu   *menu)
{
  GList *menu_list = g_object_steal_data (widget, "GxkWidget-popup-menus");
  GtkMenuDetachFunc mdfunc;
  if (menu_list)
    {
      menu_list = g_list_remove (menu_list, menu);
      g_object_set_data_full (widget, "GxkWidget-popup-menus", menu_list, popup_menus_detach);
    }
  mdfunc = g_object_get_data (menu, "gxk-GtkMenuDetachFunc");
  if (mdfunc)
    mdfunc (widget, menu);
}

/**
 * gxk_menu_attach_as_popup_with_func
 * @menu:      valid #GtkMenu
 * @menu_item: valid #GtkMenuItem
 * @mdfunc:    a #GtkMenuDetachFunc func as in gtk_menu_attach_to_widget()
 *
 * Variant of gxk_menu_attach_as_popup() which preserves the #GtkMenuDetachFunc.
 */
void
gxk_menu_attach_as_popup_with_func (GtkMenu          *menu,
                                    GtkWidget        *widget,
                                    GtkMenuDetachFunc mdfunc)
{
  GList *menu_list;
  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  menu_list = g_object_steal_data (widget, "GxkWidget-popup-menus");
  menu_list = g_list_prepend (menu_list, menu);
  g_object_set_data_full (widget, "GxkWidget-popup-menus", menu_list, popup_menus_detach);
  g_object_set_data (menu, "gxk-GtkMenuDetachFunc", mdfunc);
  gtk_menu_attach_to_widget (menu, widget, popup_menu_detacher);

  if (!gxk_signal_handler_exists (widget, "hierarchy-changed", G_CALLBACK (menu_widget_propagate_hierarchy_changed), NULL))
    g_signal_connect_after (widget, "hierarchy-changed", G_CALLBACK (menu_widget_propagate_hierarchy_changed), NULL);
  menu_widget_propagate_hierarchy_changed (widget);
}

/**
 * gxk_menu_attach_as_popup
 * @menu:      valid #GtkMenu
 * @menu_item: valid #GtkMenuItem
 *
 * This function is a replacement for gtk_menu_attach_to_widget().
 * Similar to gxk_menu_attach_as_submenu(), it sets up a propagation
 * mechanism, so @menu and submenus thereof automatically fetch their
 * accelerator groups via gxk_window_get_menu_accel_group() from the
 * toplevel window. In addition, this function allowes
 * gxk_widget_find_level_ordered() to also consider popup menus
 * in its search.
 */
void
gxk_menu_attach_as_popup (GtkMenu         *menu,
                          GtkWidget       *widget)
{
  gxk_menu_attach_as_popup_with_func (menu, widget, NULL);
}

typedef struct {
  gint     x, y;
  gboolean push_in;
} PopupData;

static void
menu_position_func (GtkMenu  *menu,
                    gint     *x,
                    gint     *y,
                    gboolean *push_in,
                    gpointer  func_data)
{
  PopupData *pdata = func_data;
  GtkWidget *active = gtk_menu_get_active (menu);
  *x = pdata->x;
  *y = pdata->y;
  *push_in = pdata->push_in && active;
  if (*push_in)
    {
      GList *list;
      for (list = GTK_MENU_SHELL (menu)->children; list; list = list->next)
        {
          GtkWidget *child = list->data;
          if (active == child)
            break;
          else if (GTK_WIDGET_VISIBLE (child))
            {
              GtkRequisition requisition;
              gtk_widget_get_child_requisition (child, &requisition);
              *y -= requisition.height;
            }
        }
    }
}

void
gxk_menu_popup (GtkMenu *menu,
                gint     x,
                gint     y,
                gboolean push_in,
                guint    mouse_button,
                guint32  time)
{
  PopupData *pdata = g_new0 (PopupData, 1);
  g_return_if_fail (GTK_IS_MENU (menu));
  pdata->x = x;
  pdata->y = y;
  pdata->push_in = push_in;
  gtk_menu_popup (menu, NULL, NULL,
                  menu_position_func, pdata,
                  mouse_button, time);
}

static GtkWidget*
widget_find_level_ordered (GtkWidget   *widget,
                           const gchar *name)
{
  GList *children = g_list_prepend (NULL, widget);
  while (children)
    {
      GList *list, *newlist = NULL;
      for (list = children; list; list = list->next)
        {
          GtkWidget *child = list->data;
          if (child->name && strcmp (child->name, name) == 0)
            {
              g_list_free (children);
              return child;
            }
        }
      /* none found, search next level */
      for (list = children; list; list = list->next)
        {
          GList *extra_children;
          widget = list->data;
          if (GTK_IS_CONTAINER (widget))
            newlist = g_list_concat (gtk_container_get_children (GTK_CONTAINER (widget)), newlist);
          if (GTK_IS_MENU_ITEM (widget))
            {
              GtkMenuItem *mitem = GTK_MENU_ITEM (widget);
              if (mitem->submenu)
                newlist = g_list_prepend (newlist, mitem->submenu);
            }
          extra_children = g_object_get_data (widget, "GxkWidget-popup-menus");
          if (extra_children)
            newlist = g_list_concat (g_list_copy (extra_children), newlist);
        }
      g_list_free (children);
      children = newlist;
    }
  return NULL;
}

/**
 * gxk_widget_find_level_ordered
 * @toplevel: valid #GtkWidget
 * @name:     name of the widget being looked for
 * @RETURNS:  a widget named @name or %NULL
 *
 * Search for a widget named @name, child of @toplevel.
 * The search is ordered by looking at all children of
 * a container before increasing depth.
 * This function also considers submenus in menu items
 * "children", as well as popup menus attached via
 * gxk_menu_attach_as_popup() (not ones attached via
 * gtk_menu_attach_to_widget() though, since Gtk+ doesn't
 * store/export the neccessary information).
 */
GtkWidget*
gxk_widget_find_level_ordered (GtkWidget   *toplevel,
                               const gchar *name)
{
  g_return_val_if_fail (GTK_IS_WIDGET (toplevel), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return widget_find_level_ordered (toplevel, name);
}

/**
 * gxk_widget_get_attach_toplevel
 * @widget: valid #GtkWidget
 *
 * This function returns the topmost container widget
 * for @widget, much like gtk_widget_get_toplevel().
 * The only difference is that for menus, not the immediate
 * parent is returned (the #GtkWindow used to display a
 * menu) but the tree walk continues on the menu item
 * using the menu as submenu.
 * For example, for a window containing a menubar with
 * submenus, gtk_widget_get_toplevel() invoked on one
 * of the menu items will return the #GtkWindow widgets
 * for the corresponding submenus, while
 * gxk_widget_get_attach_toplevel() will return the
 * actual #GtkWindow containing the menubar.
 */
GtkWidget*
gxk_widget_get_attach_toplevel (GtkWidget *widget)
{
  GtkWidget *parent;
  do {
    parent = widget->parent;
    if (GTK_IS_MENU (widget))
      parent = gtk_menu_get_attach_widget ((GtkMenu*) widget);
    widget = parent ? parent : widget;
  } while (parent);
  return widget;
}

static void
widget_add_font_requisition (GtkWidget      *widget,
                             GtkRequisition *requisition)
{
  PangoContext *context = gtk_widget_get_pango_context (widget);
  PangoFontMetrics *metrics = pango_context_get_metrics (context, widget->style->font_desc,
                                                         pango_context_get_language (context));
  gdouble digit_width = pango_font_metrics_get_approximate_digit_width (metrics);
  gdouble char_width = pango_font_metrics_get_approximate_char_width (metrics);
  gdouble points = digit_width * g_object_get_long (widget, "GxkWidget-font-digits") +
                   char_width * g_object_get_long (widget, "GxkWidget-font-chars");
  requisition->width += PANGO_PIXELS (points);
  pango_font_metrics_unref (metrics);
}

/**
 * gxk_widget_add_font_requisition
 * @widget:   valid #GtkWidget
 * @n_chars:  number of characters to request space for
 * @n_digits: number of digits to request space for
 *
 * This function adds up extra space to the widget size
 * requisition. The space is an approximation of the space
 * required by @n_chars characters and @n_digits digits
 * written with the widgets font.
 */
void
gxk_widget_add_font_requisition (GtkWidget       *widget,
                                 guint            n_chars,
                                 guint            n_digits)
{
  g_object_set_long (widget, "GxkWidget-font-chars", n_chars);
  g_object_set_long (widget, "GxkWidget-font-digits", n_digits);
  if ((n_chars || n_digits) &&
      !gxk_signal_handler_exists (widget, "size-request", G_CALLBACK (widget_add_font_requisition), NULL))
    g_signal_connect_after (widget, "size-request", G_CALLBACK (widget_add_font_requisition), NULL);
  gtk_widget_queue_resize (widget);
}

/**
 * gxk_widget_get_options
 * @widget:   valid #GtkWidget
 * @RETURNS:  custom options set on the widget
 *
 * This function returns the set of custom options
 * currently set on the widget.
 */
const gchar*
gxk_widget_get_options (gpointer widget)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  return g_object_get_data (widget, "GxkWidget-options");
}

/**
 * gxk_widget_add_option
 * @widget:   valid #GtkWidget
 * @option:   option to add to widget
 * @value:    value of @option (currently just "+" and "-" are supported)
 *
 * Add/set a custom @option of @widget to a particular @value.
 * Custom options on widgets are used to attach extra information
 * to widgets which may be useful to otherwise disjoint code
 * portions. The actual options are implemented by means of
 * g_option_concat(), g_option_check() and g_option_get().
 */
void
gxk_widget_add_option (gpointer         widget,
                       const gchar     *option,
                       const gchar     *value)
{
  const gchar *options;
  guint append = 0;
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (option != NULL && !strchr (option, ':'));
  g_return_if_fail (value == NULL || !strcmp (value, "-") || !strcmp (value, "+"));
  options = g_object_get_data (widget, "GxkWidget-options");
  if (!options)
    options = "";
  if (value && strcmp (value, "-") == 0 &&
      g_option_check (options, option))
    append = 2;
  else if ((!value || strcmp (value, "+") == 0) &&
           !g_option_check (options, option))
    append = 1;
  if (append)
    {
      guint l = strlen (options);
      gchar *s = g_strconcat (options,
                              options[l] == ':' ? "" : ":",
                              option, /* append >= 1 */
                              append >= 2 ? value : "",
                              NULL);
      g_object_set_data_full (widget, "GxkWidget-options", s, g_free);
    }
}

/**
 * gxk_widget_check_option
 * @widget:   valid #GtkWidget
 * @option:   option to check for
 * @RETURNS:  whether @option is set
 *
 * Test whether the custom @option is set on @widget.
 */
gboolean
gxk_widget_check_option (gpointer         widget,
                         const gchar     *option)
{
  const gchar *options;
  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  options = g_object_get_data (widget, "GxkWidget-options");
  return g_option_check (options, option);
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

/* --- www browser support --- */
gboolean
gxk_show_url (const gchar *url)
{
  static struct { const gchar *prg, *arg1, *prefix, *postfix; gboolean disabled; } www_browsers[] = {
    /* program */               /* arg1 */      /* prefix+URL+postfix */
    { "sensible-browser",       NULL,           "", "" },
    { "x-www-browser",          NULL,           "", "" },
    { "gnome-moz-remote",       "--newwin"      "", "" },
    { "kfmclient",              "openURL",      "", "" },
    { "mozilla",                NULL,           "", "" },
  };
  guint i;
  for (i = 0; i < G_N_ELEMENTS (www_browsers); i++)
    if (!www_browsers[i].disabled)
      {
        gchar *args[128] = { 0, };
        guint n = 0;
        args[n++] = (char*) www_browsers[i].prg;
        if (www_browsers[i].arg1)
          args[n++] = (char*) www_browsers[i].arg1;
        gchar *string = g_strconcat (www_browsers[i].prefix, url, www_browsers[i].postfix, NULL);
        args[n] = string;
        GError *error = NULL;
        gboolean success = g_spawn_async (NULL, /* cwd */
                                          args,
                                          NULL, /* envp */
                                          G_SPAWN_SEARCH_PATH,
                                          NULL, /* child_setup() */
                                          NULL, /* user_data */
                                          NULL, /* child_pid */
                                          &error);
        g_free (string);
        g_printerr ("show \"%s\": %s: %s\n", url, args[0], error ? error->message : "Ok");
        g_clear_error (&error);
        if (success)
          return TRUE;
        www_browsers[i].disabled = TRUE;
      }
  return FALSE;
}


/* --- zlib support --- */
#include <zlib.h>
gchar*
gxk_zfile_uncompress (guint                uncompressed_size,
                      const unsigned char *cdata,
		      guint                cdata_size)
{
  uLongf dlen = uncompressed_size;
  guint len = dlen + 1;
  guint8 *text = g_malloc (len);
  gint result;
  const gchar *err;

  result = uncompress (text, &dlen, cdata, cdata_size);
  switch (result)
    {
    case Z_OK:
      if (dlen == uncompressed_size)
	{
	  err = NULL;
	  break;
	}
      /* fall through */
    case Z_DATA_ERROR:
      err = "internal data corruption";
      break;
    case Z_MEM_ERROR:
      err = "out of memory";
      break;
    case Z_BUF_ERROR:
      err = "insufficient buffer size";
      break;
    default:
      err = "unknown error";
      break;
    }
  if (err)
    g_error ("while decompressing (%p, %u): %s", cdata, cdata_size, err);

  text[dlen] = 0;
  return text;
}
