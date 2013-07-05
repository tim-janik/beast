// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gxkutils.hh"
#include "glewidgets.h"
#include "gxkmenubutton.hh"
#include "gxkstock.hh"
#include "gxkcellrendererpopup.hh"
#include "gxkauxwidgets.hh"
#include <string.h>
#include <stdlib.h>


/* --- generated marshallers --- */
#include "gxkmarshal.cc"


/* --- generated type IDs and enums --- */
#include "gxkgentypes.cc"


/* --- prototypes --- */
static void     gxk_traverse_viewable_changed	        (GtkWidget      *widget,
                                                         gpointer        data);
static void     gxk_traverse_attached_hierarchy_changed (GtkWidget      *widget,
                                                         gpointer        data);
static void     gxk_menu_refetch_accel_group            (GtkMenu        *menu);


/* --- variables --- */
static guint signal_viewable_changed = 0;
static guint signal_attached_hierarchy_changed = 0;


/* --- functions --- */
static gboolean
gxk_widget_real_can_activate_accel (GtkWidget *widget, // GTKFIX: #145270, remove this when depending on gtk+ > 2.4.10
                                    guint      signal_id)
{
  /* widgets must be onscreen for accels to take effect */
  return GTK_WIDGET_IS_SENSITIVE (widget) && GTK_WIDGET_DRAWABLE (widget) && gxk_widget_ancestry_viewable (widget);
}

static gboolean
ehook_container_focus_child_set (GSignalInvocationHint *ihint,
                                 guint                  n_param_values,
                                 const GValue          *param_values,
                                 gpointer               data)
{
  GtkContainer *container = (GtkContainer*) g_value_get_object (param_values + 0);
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
gxk_init_utils (void)
{
  /* type registrations */
  gxk_type_register_generated (G_N_ELEMENTS (generated_type_entries), generated_type_entries);

  /* Gtk+ patchups */
  signal_viewable_changed = g_signal_newv ("viewable-changed",
                                           G_TYPE_FROM_CLASS (gtk_type_class (GTK_TYPE_WIDGET)),
                                           G_SIGNAL_RUN_LAST,
                                           g_cclosure_new (G_CALLBACK (gxk_traverse_viewable_changed), NULL, NULL),
                                           NULL, NULL,
                                           gtk_marshal_VOID__VOID,
                                           G_TYPE_NONE, 0, NULL);
  signal_attached_hierarchy_changed = g_signal_newv ("attached-hierarchy-changed",
                                                     G_TYPE_FROM_CLASS (gtk_type_class (GTK_TYPE_WIDGET)),
                                                     G_SIGNAL_RUN_LAST,
                                                     g_cclosure_new (G_CALLBACK (gxk_traverse_attached_hierarchy_changed), NULL, NULL),
                                                     NULL, NULL,
                                                     gtk_marshal_VOID__VOID,
                                                     G_TYPE_NONE, 0, NULL);
  GtkWidgetClass *widget_class = (GtkWidgetClass*) gtk_type_class (GTK_TYPE_WIDGET);
  widget_class->can_activate_accel = gxk_widget_real_can_activate_accel;

  /* patch up scrolling+focus behaviour */
  g_type_class_unref (g_type_class_ref (GTK_TYPE_CONTAINER));   /* create static class */
  g_signal_add_emission_hook (g_signal_lookup ("set-focus-child", GTK_TYPE_CONTAINER), 0,
                              ehook_container_focus_child_set, NULL, NULL);
}

/**
 * @param widget	a valid GtkWidget
 * @return		whether @a widget is visible on screen
 *
 * Checks for @a widget to be effectively visible on screen.
 * This function works around a bug in Gtk+ versions <= 2.4.10,
 * with newer Gtk+ versions, (gdk_window_is_viewable(widget->window) &&
 * GTK_WIDGET_DRAWABLE(widget)) is a suitable replacement.
 */
gboolean
gxk_widget_ancestry_viewable (GtkWidget *widget)
{
  // GTKFIX: gdk_window_is_viewable() is broken up to Gtk+-2.4.10
  if (!widget->window || !gdk_window_is_viewable (widget->window))
    return FALSE;
  while (widget)
    {
      if (!GTK_WIDGET_DRAWABLE (widget) || !gdk_window_is_viewable (widget->window))
        return FALSE;
      widget = widget->parent;
    }
  return TRUE;
}

/**
 * @param n_entries	number of generated types to register
 * @param entries	GxkTypeGenerated type descriptions
 *
 * Register each of the generated type entries with the
 * type system. Currently supported parent types are
 * G_TYPE_ENUM and G_TYPE_FLAGS in which cases the
 * @a type_data member must point to a NULL terminated
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
	  type_id = g_enum_register_static (entries[i].type_name, (const GEnumValue*) entries[i].type_data);
	  break;
	case G_TYPE_FLAGS:
	  type_id = g_flags_register_static (entries[i].type_name, (const GFlagsValue*) entries[i].type_data);
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
 * @param object	a valid GObject
 * @param name	name of the double value to set
 * @param v_double	the actual value
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
    g_object_set_data ((GObject*) object, name, NULL);
  else
    {
      gdouble *dp = g_new0 (gdouble, 1);
      *dp = v_double;
      g_object_set_data_full ((GObject*) object, name, dp, g_free);
    }
}

/**
 * @param object	a valid GObject
 * @param name	name of the double value to retrieve
 * @return		the actual value
 *
 * Convenience variant of g_object_get_data() to retrieve
 * a double instead of a pointer.
 */
gdouble
g_object_get_double (gpointer     object,
		     const gchar *name)
{
  g_return_val_if_fail (G_IS_OBJECT (object), 0);

  double *dp = (double*) g_object_get_data ((GObject*) object, name);
  return dp ? *dp : 0;
}

/**
 * @param object	a valid GObject
 * @param name	name of the long value to set
 * @param v_long	the actual value
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

  g_object_set_data ((GObject*) object, name, (gpointer) v_long);
}

/**
 * @param object	a valid GObject
 * @param name	name of the long value to retrieve
 * @return		the actual value
 *
 * Convenience variant of g_object_get_data() to retrieve
 * a long instead of a pointer.
 */
glong
g_object_get_long (gpointer     object,
		   const gchar *name)
{
  g_return_val_if_fail (G_IS_OBJECT (object), 0);

  return (glong) g_object_get_data ((GObject*) object, name);
}

gchar*
gxk_convert_latin1_to_utf8 (const gchar *string)
{
  if (string)
    {
      const uint l = strlen (string);
      const guchar *s = (const guchar*) string;
      guchar *dest = g_new (guchar, l * 2 + 1), *d = dest;
      while (*s)
	if (*s >= 0xC0)
	  *d++ = 0xC3, *d++ = *s++ - 0x40;
	else if (*s >= 0x80)
	  *d++ = 0xC2, *d++ = *s++;
	else
	  *d++ = *s++;
      *d++ = 0;
      return (char*) dest;
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
 * @param widget	valid GtkWidget
 * RETURNS: TRUE if the widget is viewable, FALSE otherwise
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
 * @param widget	valid GtkWidget
 *
 * A widget should call this function if it changed
 * the mapped state of one of its children (or if it
 * is a toplevel and gets show or hidden) to emit the
 * ::viewable-changed signal on the related sub-tree.
 * GxkDialog properly emits this signal if show or
 * hidden, containers like GtkNotebook need this
 * function be explicitely connected to their ::switch-page
 * signal, in order for their children to get properly
 * notified.
 */
void
gxk_widget_viewable_changed (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_signal_emit (widget, signal_viewable_changed, 0);
}

static void
gxk_traverse_viewable_changed (GtkWidget *widget,
			       gpointer   data)
{
  if (GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget), (GtkCallback) gxk_widget_viewable_changed, NULL);
}

/**
 * @param widget	valid GtkWidget
 *
 * Setting or unsetting a parent on a widget leads to emission
 * of the ::hirarchy-changed signal on the widget and any children
 * it contains. However, popup menus which are merely attached to
 * widgets aren't properly notified upon such hirarchy changes,
 * for this gxk_widget_attached_hierarchy_changed() is provided.
 * On menus which got attached to a widget with
 * gxk_menu_attach_as_popup(), the signal ::attached-hirarchy-changed
 * is emitted if ::hirarchy-changed is emitted on the widget,
 * by calling gxk_widget_attached_hierarchy_changed() on the menu.
 */
void
gxk_widget_attached_hierarchy_changed (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_signal_emit (widget, signal_attached_hierarchy_changed, 0);
}

static void
widget_propagate_hierarchy_changed_to_attached (GtkWidget *widget)
{
  if (GTK_IS_OPTION_MENU (widget))
    {
      GtkOptionMenu *option_menu = GTK_OPTION_MENU (widget);
      if (option_menu->menu)
        gxk_widget_attached_hierarchy_changed (option_menu->menu);
    }
  if (GTK_IS_MENU_ITEM (widget))
    {
      GtkMenuItem *menu_item = GTK_MENU_ITEM (widget);
      if (menu_item->submenu)
        gxk_widget_attached_hierarchy_changed (menu_item->submenu);
    }
  GList *menu_list = (GList*) g_object_get_data ((GObject*) widget, "GxkWidget-popup-menus");
  for (; menu_list; menu_list = menu_list->next)
    gxk_widget_attached_hierarchy_changed ((GtkWidget*) menu_list->data);
}

static void
gxk_traverse_attached_hierarchy_changed (GtkWidget *widget,
                                         gpointer   data)
{
  if (GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget), (GtkCallback) gxk_widget_attached_hierarchy_changed, NULL);
  widget_propagate_hierarchy_changed_to_attached (widget);

  /* special attached_hierarchy_changed hooks */
  if (GTK_IS_MENU (widget))
    gxk_menu_refetch_accel_group (GTK_MENU (widget));
}

static void
gxk_widget_proxy_hierarchy_changed_to_attached (GtkWidget *widget)
{
  if (!gxk_signal_handler_exists (widget, "hierarchy-changed", G_CALLBACK (widget_propagate_hierarchy_changed_to_attached), NULL))
    g_signal_connect_after (widget, "hierarchy-changed", G_CALLBACK (widget_propagate_hierarchy_changed_to_attached), NULL);
  widget_propagate_hierarchy_changed_to_attached (widget);
}

/**
 * @param window	valid GdkWindow*
 * @param cursor	GdkCursorType cursor type
 *
 * Set a window's cursor type. If GXK_DEFAULT_CURSOR is specified
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
      GdkCursor *wc = gdk_cursor_new (GdkCursorType (cursor & ~1));
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
  window = (GdkWindow*) g_slist_pop_head (&expose_windows);
  cwindow = (GdkWindow*) g_slist_pop_head (&cexpose_windows);
  while (window || cwindow)
    {
      if (window)
	gdk_window_process_updates (window, FALSE);
      if (cwindow)
	gdk_window_process_updates (cwindow, TRUE);
      window = (GdkWindow*) g_slist_pop_head (&expose_windows);
      cwindow = (GdkWindow*) g_slist_pop_head (&cexpose_windows);
    }
  expose_handler_id = 0;
  GDK_THREADS_LEAVE ();
  return FALSE;
}

/**
 * @param window	valid GdkWindow
 * @param update_children	whether to also process updates for child windows
 *
 * Cause @a window to be updated asyncronously as soon as possible via
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
 * Convenience variant of gdk_draw_line() to draw a horizontal line.
 */
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

/**
 * Convenience variant of gdk_draw_line() to draw a vertical line.
 */
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
 * @param colormap	valid GdkColormap
 * @param color	valid GdkColor
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
 * @param rgb_value 0xRrGgBb color value
 *
 * Fill out a GdkColor with red, green and blue color values,
 * from Rr, Gg and Bb respectively.
 * GdkColor.pixel will be 0-initialized.
 */
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

/**
 * @param rgb_value 0xAaRrGgBb color value
 *
 * Fill out a GdkColor with red, green and blue color values,
 * from Rr, Gg and Bb respectively. Aa maybe an alpha value
 * and will be ignored. GdkColor.pixel will be 0-initialized.
 */
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

/**
 * @param rgb_value 0xRrGgBbAa color value
 *
 * Fill out a GdkColor with red, green and blue color values,
 * from Rr, Gg and Bb respectively. Aa maybe an alpha value
 * and will be ignored. GdkColor.pixel will be 0-initialized.
 */
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

/* --- Colors --- */
static int
gxk_color_dot_cmp (const void *v1,
                   const void *v2)
{
  const GxkColorDot *c1 = (const GxkColorDot*) v1;
  const GxkColorDot *c2 = (const GxkColorDot*) v2;
  return c1->value < c2->value ? -1 : c1->value > c2->value;
}

GxkColorDots*
gxk_color_dots_new (guint              n_dots,
                    const GxkColorDot *dots)
{
  g_return_val_if_fail (n_dots >= 2, NULL);
  GxkColorDots *cdots = g_new0 (GxkColorDots, 1);
  guint sizeof_color_dot = sizeof (GxkColorDot);
  cdots->n_colors = n_dots;
  cdots->colors = (GxkColorDot*) g_memdup (dots, sizeof_color_dot * n_dots);
  qsort (cdots->colors, cdots->n_colors, sizeof_color_dot, gxk_color_dot_cmp);
  return cdots;
}

guint
gxk_color_dots_interpolate (GxkColorDots   *cdots,
                            double          value,
                            double          saturation)
{
  g_return_val_if_fail (cdots != NULL, 0);
  /* find segment via bisection */
  guint offset = 0, n = cdots->n_colors;
  while (offset + 1 < n)
    {
      guint i = (offset + n) >> 1;
      if (value < cdots->colors[i].value)
        n = i;
      else
        offset = i;
    }
  g_assert (offset == 0 || value >= cdots->colors[offset].value);
  if (value >= cdots->colors[offset].value && offset + 1 < cdots->n_colors)
    {   /* linear interpolation */
      guint c1 = cdots->colors[offset].rgb;
      guint c2 = cdots->colors[offset + 1].rgb;
      double delta = value - cdots->colors[offset].value;       /* >= 0, see assertion above */
      double range = cdots->colors[offset + 1].value -
                     cdots->colors[offset].value;               /* >= 0, ascending sort */
      double d2 = delta / range;                                /* <= 1, due to bisection */
      double d1 = 1.0 - d2;
      guint8 red = saturation * (((c1 >> 16) & 0xff) * d1 + ((c2 >> 16) & 0xff) * d2);
      guint8 green = saturation * (((c1 >> 8) & 0xff) * d1 + ((c2 >> 8) & 0xff) * d2);
      guint8 blue = saturation * ((c1 & 0xff) * d1 + (c2 & 0xff) * d2);
      return (red << 16) | (green << 8) | blue;
    }
  else  /* value is out of range on either boundary */
    {
      guint8 red = saturation * ((cdots->colors[offset].rgb >> 16) & 0xff);
      guint8 green = saturation * ((cdots->colors[offset].rgb >> 8) & 0xff);
      guint8 blue = saturation * (cdots->colors[offset].rgb & 0xff);
      return (red << 16) | (green << 8) | blue;
    }
}

void
gxk_color_dots_destroy (GxkColorDots *cdots)
{
  g_return_if_fail (cdots != NULL);
  g_free (cdots->colors);
  g_free (cdots);
}

/* --- Gtk convenience --- */

/**
 * @param widget	a valid GtkWidget
 *
 * This function is euqivalent to gtk_widget_set_sensitive (@a widget, FALSE);
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
 * @param widget	a valid GtkWidget
 *
 * This function is euqivalent to gtk_widget_set_sensitive (@a widget, TRUE);
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
  GtkWidget **widget_p = (GtkWidget**) data;
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
 * @param widget	a valid GtkWidget
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
 * @param widget	a valid GtkWidget
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
 * @param widget	a valid GtkWindow
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
  GtkWidget *ev = (GtkWidget*) g_object_new (GTK_TYPE_EVENT_BOX, NULL);
  GtkWidget *image = gtk_image_new();
  GtkWidget *label = (GtkWidget*) g_object_new (GTK_TYPE_LABEL, NULL);
  GtkWidget *box = (GtkWidget*) g_object_new (GTK_TYPE_HBOX, "parent", ev, NULL);
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
          GtkIconSize isize = GtkIconSize (0);
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
 * @param notebook	a valid notebook
 * @param child	a valid parent-less widget
 * @param label	notebook page name
 * @param fillexpand	whether the tab label should expand
 *
 * Add a new page containing @a child to @a notebook,
 * naming the page @a label.
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
 * @param notebook	valid GtkNotebook
 * @param page	@a notebook page widget
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
 * @param draw_seperator	enable visible vertical seperator
 * @return		visible vertical space/seperator widget
 *
 * Create a vertical seperator widget. @a draw_seperator indicates
 * whether the seperator should be amount to simple space or not.
 */
GtkWidget*
gxk_vseparator_space_new (gboolean draw_seperator)
{
  GtkWidget *widget = (GtkWidget*) g_object_new (draw_seperator ? GTK_TYPE_VSEPARATOR : GTK_TYPE_ALIGNMENT,
                                    "visible", TRUE,
                                    NULL);
  g_signal_connect (widget, "size-request", G_CALLBACK (vseparator_space_request),
                    GUINT_TO_POINTER (draw_seperator ? 2 + 1 + 2 : 3));
  return widget;
}

/**
 * @param notebook	valid GtkNotebook
 * @return		the widget corresponding to the current page
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
 * @param widget	valid GtkWidget
 * @return		notebook page widget or NULL
 *
 * Find the innermost notebook page widget that contains @a widget.
 */
GtkWidget*
gxk_notebook_descendant_get_page (GtkWidget *widget)
{
  while (widget->parent && !GTK_IS_NOTEBOOK (widget->parent))
    widget = widget->parent;
  return widget->parent ? widget : NULL;
}

/**
 * @param widget	valid GtkWidget
 * @return		notebook page tab widget or NULL
 *
 * Find the innermost notebook page widget that contains @a widget
 * and return its tabulator widget.
 */
GtkWidget*
gxk_notebook_descendant_get_tab (GtkWidget *widget)
{
  widget = gxk_notebook_descendant_get_page (widget);
  return widget ? gtk_notebook_get_tab_label (GTK_NOTEBOOK (widget->parent), widget) : NULL;
}

/**
 * @param box	a valid GtkBox
 * @param pos	position of the requested child
 * @return		a child of @a box or NULL
 *
 * Find the child at position @a pos (0 indicates the first child) of
 * @a box and return it. To retrieve the last xchild of @a box, -1
 * may be passed as @a pos.
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

  GtkBoxChild *child_info = child && pos <= 0 ? (GtkBoxChild*) child->data : NULL;
  return child_info ? child_info->widget : NULL;
}

/**
 * @param widget	a valid widget
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

static gboolean
async_delete_event_handler (gpointer data)
{
  GDK_THREADS_ENTER ();
  GtkWidget *widget = GTK_WIDGET (data);
  if (GTK_IS_WINDOW (widget) && GTK_WIDGET_DRAWABLE (widget))
    {
      GdkEvent event = { GdkEventType (0), };
      event.any.type = GDK_DELETE;
      event.any.window = widget->window;
      event.any.send_event = TRUE;
      if (!gtk_widget_event (widget, &event))
        gtk_widget_destroy (widget);
    }
  g_object_unref (widget);
  GDK_THREADS_LEAVE ();
  return FALSE;
}

/**
 * @param widget	a widget having a toplevel
 *
 * This function is useful to produce the an effect
 * similar to user caused window manager triggered
 * window deletion on the toplevel of @a widget.
 * Note that this function will cause window deletion
 * despite any grabs in effect however.
 */
void
gxk_toplevel_delete (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  widget = gtk_widget_get_toplevel (widget);
  if (GTK_IS_WINDOW (widget) && GTK_WIDGET_DRAWABLE (widget))
    g_idle_add_full (G_PRIORITY_DEFAULT, async_delete_event_handler, g_object_ref (widget), NULL);
}

/**
 * @param widget	a widget having a toplevel
 *
 * Activate the default widget of the toplevel of @a widget.
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
 * @param widget	a widget having a toplevel
 *
 * Hide the toplevel of @a widget.
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
  DescendantSearch *dsearch = (DescendantSearch*) data;
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
 * @param widget	a valid GtkWidget
 *
 * Modify the widget and it's style, so that it is insensitive,
 * but doesn't quite look that way. Useful for inactive title
 * menu items in menus (@a widget should be the menu item's label).
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
 * @param widget	a valid GtkWidget
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
 * @param widget	a valid GtkWidget
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
 * @param widget	a valid GtkWidget
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
 * @param widget	a valid GtkWidget
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
 * @param widget	a valid GtkWidget
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
 * @param widget	a valid GtkWidget
 * @param tooltip	descriptive tooltip
 *
 * As a convenience function, this sets the @a tooltip for @a widget
 * on GXK_TOOLTIPS, if @a widget is supposed to have tooltips
 * according to the system configuration and also sets the latent tooltip.
 */
void
gxk_widget_set_tooltip (gpointer     widget,
                        const gchar *tooltip)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  gxk_widget_set_latent_tooltip ((GtkWidget*) widget, tooltip);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, (GtkWidget*) widget, tooltip, NULL);
}

/**
 * @param widget	a valid GtkWidget
 * @param tooltip	descriptive tooltip
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
  g_object_set_data_full ((GObject*) widget, "gxk-widget-latent-tooltip", g_strdup (tooltip), g_free);
}

/**
 * @param widget	a valid GtkWidget
 * @return      	descriptive tooltip
 *
 * Retrieve the latent tooltip for @a widget. See
 * gxk_widget_set_latent_tooltip() for the purpose of latent
 * tooltips.
 */
const gchar*
gxk_widget_get_latent_tooltip (GtkWidget *widget)
{
  return (const char*) g_object_get_data ((GObject*) widget, "gxk-widget-latent-tooltip");
}

static gboolean
gxk_activate_accel_group (GtkWidget     *widget,
			  GdkEventKey   *event,
			  GtkAccelGroup *accel_group)
{
  GdkModifierType accel_mods = GdkModifierType (event->state);
  guint accel_key = event->keyval;
  gboolean was_handled = FALSE;
  if (gtk_accelerator_valid (accel_key, accel_mods))
    {
      gchar *accel_name = gtk_accelerator_name (accel_key, GdkModifierType (accel_mods & gtk_accelerator_get_default_mod_mask ()));
      GQuark accel_quark = g_quark_from_string (accel_name);
      guint signal_accel_activate = g_signal_lookup ("accel_activate", GTK_TYPE_ACCEL_GROUP);
      g_free (accel_name);
      g_signal_emit (accel_group, signal_accel_activate, accel_quark,
		     widget, accel_key, accel_mods, &was_handled);
    }
  return was_handled;
}

/**
 * @param widget	a valid GtkWidget
 * @param accel_group	a valid GtkAccelGroup
 *
 * Activate accelerators within accel group when @a widget
 * receives key press events. This function isn't pure
 * convenience, as it works around Gtk+ version prior to
 * Gtk+-2.4 not exporting
 * gtk_accel_group_activate(), gtk_accel_group_attach()
 * and gtk_accel_group_detach().
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
			     (GClosureNotify) g_object_unref, GConnectFlags (0));
    }
}

/**
 * @param sgmode	size group mode, one of @c GTK_SIZE_GROUP_NONE,
 *          @c GTK_SIZE_GROUP_HORIZONTAL, @c GTK_SIZE_GROUP_VERTICAL or
 *	    @c GTK_SIZE_GROUP_BOTH
 * @param ...:    NULL terminated list of widgets to group together
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
      GtkWidget *widget = (GtkWidget*) first_widget;
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
 * @param strpath	stringified GtkTreePath
 *
 * Return index[0] of @a strpath. Useful for paths in lists,
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
 *
 * This function is a replacement for gtk_tree_model_get_iter()
 * for Gtk+-2.4. For sort models, gtk_tree_model_get_iter() can
 * erroneously return TRUE which is corrected by calling this
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
 * @param path	valid GtkTreePath
 *
 * Workaround for gtk_tree_path_prev() which corrupts memory
 * if called on empty paths (up to version Gtk+-2.4 at least).
 */
gboolean
gxk_tree_path_prev (GtkTreePath *path)
{
  if (path && gtk_tree_path_get_depth (path) < 1)
    return FALSE;
  return gtk_tree_path_prev (path);
}

/**
 * @param tree_view	valid GtkTreeView
 * @param position	column position (or -1 to append)
 * @param column	valid GtkTreeViewColumn
 * @param cell	valid GtkCellRenderer
 * @param ...:       attribute mappings
 *
 * Appends @a cell to @a column and adds @a column
 * to @a tree_view at the specified @a position.
 * This function takes a NULL-terminated list
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
 * @param tree_view	valid GtkTreeView
 * @param n_cols	number of columns to append
 * @param ...:       column arguments
 *
 * Add @a n_cols new columns with text cells to
 * @a tree_view (a short hand version for multiple
 * calls to gxk_tree_view_add_text_column()).
 * Per column, the caller needs to
 * supply a guint, a string, a gdouble and another
 * string. The Arguments are used as model column
 * number (for the text to be displayed), the column
 * specific flags, the horizontal cell alignment
 * (between 0 and 1) and the column title respectively.
 *
 * The @a column_flags argument is a combination of letters that
 * are able to switch certain characteristics on or off,
 * currently supported are:
 * @li @c F - column is fixed in sizing;
 * @li @c A - column resizes automatically;
 * @li @c G - columns sizing behaviour is grow only;
 * @li @c S - column is sortable;
 * @li @c s - column is unsortable;
 * @li @c O - column is reorderable;
 * @li @c o - column is not reorderable;
 * @li @c R - column is user-resizable;
 * @li @c r - column is not user-resizable;
 * @li @c P - add extra padding between multiple cells of the same column;
 * @li @c p - cancel a previous #P flag;
 * @li @c # - automatically popup dialogs for popup cell renderers.
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
				     NULL, NULL, GConnectFlags (0));
    }
  va_end (var_args);
}

static GtkTreeViewColumn*
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
      cell = (GtkCellRenderer*) g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
                                              "xalign", xalign,
                                              "editable", callback1 != NULL,
                                              NULL);
      prop = "text";
      if (callback1)
	g_signal_connect_data (cell, "edited", G_CALLBACK (callback1), data, NULL, cflags);
      break;
    case 2:	/* popup */
      cell = (GtkCellRenderer*) g_object_new (GXK_TYPE_CELL_RENDERER_POPUP,
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
      cell = (GtkCellRenderer*) g_object_new (GTK_TYPE_CELL_RENDERER_TOGGLE,
                                              /* "radio", radio_indicator, */
                                              "activatable", callback1 != NULL,
                                              NULL);
      prop = "active";
      if (callback1)
	g_signal_connect_data (cell, "toggled", G_CALLBACK (callback1), data, NULL, cflags);
      break;
    }
  gxk_tree_view_add_column (tree_view, -1,
			    tcol = (GtkTreeViewColumn*) g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
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
  return tcol;
}

/**
 * @param tree_view	valid GtkTreeView
 * @param model_column	model column
 * @param column_flags	column flags
 * @param xalign	horizontal text alignment
 * @param title	column title
 * @param tooltip	column tooltip
 * @param edited_callback	notification callback 
 * @param data	data passed in to toggled_callback
 * @param cflags	connection flags
 * @return		a newly added GtkTreeViewColumn
 *
 * Add a new column with text cell to a @a tree_view.
 * The @a model_column indicates the column number
 * of the tree model containing the text to be
 * displayed, the @a column_flags toggle specific
 * column characteristics (see
 * gxk_tree_view_append_text_columns() for details)
 * and @a xalign controls the horizontal cell alignment
 * (between 0 and 1).
 * If non-NULL, @a edited_callback(@a data) is connected
 * with @a cflags (see g_signal_connect_data()) to the
 * "::edited" signal of the text cell and the cell is
 * made editable.
 */
GtkTreeViewColumn*
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
  g_return_val_if_fail (GTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view_add_column (tree_view, model_column, xalign, title, tooltip,
                               edited_callback, NULL, data, cflags,
			       1, "", column_flags);
}

/**
 * @param tree_view	valid GtkTreeView
 * @param model_column	model column
 * @param xalign	horizontal text alignment
 * @param title	column title
 * @param tooltip	column tooltip
 * @param edited_callback	edit notification callback
 * @param popup_callback	popup notification callback
 * @param data	data passed in to toggled_callback
 * @param cflags	connection flags
 * @return		a newly added GtkTreeViewColumn
 *
 * Add a text column with popup facility, similar to
 * gxk_tree_view_add_text_column(). This function takes
 * an additional argument @a popup_callback() which is
 * called when the user clicks on the cells "popup"
 * button.
 */
GtkTreeViewColumn*
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
  g_return_val_if_fail (GTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view_add_column (tree_view, model_column, xalign, title, tooltip,
			       edited_callback, popup_callback, data, cflags,
			       2, "", column_flags);
}

/**
 * @param tree_view	valid GtkTreeView
 * @param model_column	model column
 * @param xalign	horizontal text alignment
 * @param title	column title
 * @param tooltip	column tooltip
 * @param toggled_callback	notification callback
 * @param data	data passed in to toggled_callback
 * @param cflags	connection flags
 * @return		a newly added GtkTreeViewColumn
 *
 * Add a toggle button column, similar
 * to gxk_tree_view_add_text_column(), however
 * the model column is expected to be of type
 * G_TYPE_BOOLEAN, and instead of an @a edited_callback(),
 * this function has a
 * void @a toggled_callback(GtkCellRendererToggle*, const gchar *strpath, @a data)
 * callback which is connected to the "toggled" signal of
 * the new cell.
 */
GtkTreeViewColumn*
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
  g_return_val_if_fail (GTK_IS_TREE_VIEW (tree_view), NULL);

  return tree_view_add_column (tree_view, model_column, xalign, title, tooltip,
			       toggled_callback, NULL, data, cflags,
			       3, "A", column_flags);
}

void
gxk_tree_view_set_editable (GtkTreeView *tview,
                            gboolean     maybe_editable)
{
  GList *clist = gtk_tree_view_get_columns (tview);
  GtkTreeViewColumn *tcol = (GtkTreeViewColumn*) g_list_pop_head (&clist);
  while (tcol)
    {
      GList *rlist = gtk_tree_view_column_get_cell_renderers (tcol);
      GtkCellRenderer *cell = (GtkCellRenderer*) g_list_pop_head (&rlist);
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
          cell = (GtkCellRenderer*) g_list_pop_head (&rlist);
        }
      tcol = (GtkTreeViewColumn*) g_list_pop_head (&clist);
    }
}

static void
fixup_tcolumn_title (GtkWidget   *widget,
		     const gchar *tooltip)
{
  while (!GTK_IS_BUTTON (widget))
    widget = widget->parent;
  if (GTK_IS_BUTTON (widget))
    gxk_widget_set_tooltip (widget, tooltip);
}

/**
 * @param tree_column	valid GtkTreeViewColumn
 * @param title	column title
 * @param tooltip	column tooltip
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
  label = (GtkWidget*) g_object_new (GTK_TYPE_LABEL,
                                     "visible", TRUE,
                                     "label", title,
                                     "xalign", 0.5,
                                     NULL);
  g_signal_connect_data (label, "map",
			 G_CALLBACK (fixup_tcolumn_title), g_strdup (tooltip), (GClosureNotify) g_free,
			 GConnectFlags (0));
  gtk_tree_view_column_set_widget (tree_column, label);
}

/**
 * @param selection	GtkTreeSelection to modify
 * @param str_path	a stringified GtkTreePath
 *
 * Select the row denoted by @a str_path.
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
 * @param selection	GtkTreeSelection to modify
 * @param str_path	a stringified GtkTreePath
 *
 * Unselect the row denoted by @a str_path.
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
 * @param selection	GtkTreeSelection to modify
 * @param ...:       GtkTreePath indices
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
 * @param selection	GtkTreeSelection to modify
 * @param ...:       GtkTreePath indices
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
  GDK_THREADS_ENTER ();
  GtkTreeSelection *selection = (GtkTreeSelection*) g_slist_pop_head (&browse_selection_queue);
  while (selection)
    {
      GtkTreeIter iter;
      gboolean needs_sel;
      g_object_weak_unref (G_OBJECT (selection), browse_selection_weak_notify, selection);
      needs_sel = (gtk_tree_selection_get_mode (selection) == GTK_SELECTION_BROWSE &&
		   !gtk_tree_selection_get_selected (selection, NULL, &iter));
      if (needs_sel)
	{
          /* in the following code, we need to do some extra path copying, because
           * browse_selection_changed() changes the "GxkTreeSelection-last" path
           * upon selection changes.
           */
	  const GtkTreePath *cpath = (GtkTreePath*) g_object_get_data ((GObject*) selection, "GxkTreeSelection-last");
	  g_object_ref (selection);
	  browse_selection_ignore = selection;
	  if (cpath)
	    {
              GtkTreePath *p = gtk_tree_path_copy (cpath);
	      gtk_tree_selection_select_path (selection, p);
              gtk_tree_path_free (p);
	      cpath = (const GtkTreePath*) g_object_get_data ((GObject*) selection, "GxkTreeSelection-last");
	      needs_sel = !gtk_tree_selection_get_selected (selection, NULL, NULL);
            }
          if (needs_sel && cpath)
            {
              GtkTreePath *p = gtk_tree_path_copy (cpath);
              if (gxk_tree_path_prev (p))
                gtk_tree_selection_select_path (selection, p);
              gtk_tree_path_free (p);
              cpath = (const GtkTreePath*) g_object_get_data ((GObject*) selection, "GxkTreeSelection-last");
              needs_sel = !gtk_tree_selection_get_selected (selection, NULL, NULL);
            }
          if (needs_sel && cpath)
            {
              GtkTreePath *p = gtk_tree_path_copy (cpath);
              if (gtk_tree_path_up (p))
                gtk_tree_selection_select_path (selection, p);
              gtk_tree_path_free (p);
              cpath = (const GtkTreePath*) g_object_get_data ((GObject*) selection, "GxkTreeSelection-last");
              needs_sel = !gtk_tree_selection_get_selected (selection, NULL, NULL);
            }
	  if (needs_sel)
	    {
	      GtkTreePath *p = gtk_tree_path_new ();
	      gtk_tree_path_append_index (p, 0);
	      gtk_tree_selection_select_path (selection, p);
              if (gtk_tree_selection_path_is_selected (selection, p))
		{
		  /* GTKFIX: this triggeres an assertion on empty sort models */
		  gtk_tree_view_set_cursor (gtk_tree_selection_get_tree_view (selection),
					    p, NULL, FALSE);
		}
	      gtk_tree_path_free (p);
              /* cpath invalid */
	    }
	  else
            {
              GtkTreePath *p = gtk_tree_path_copy (cpath);
              gtk_tree_view_set_cursor (gtk_tree_selection_get_tree_view (selection),
                                        p, NULL, FALSE);
              gtk_tree_path_free (p);
              /* cpath invalid */
            }
	  browse_selection_ignore = NULL;
	  g_object_unref (selection);
	}
      selection = (GtkTreeSelection*) g_slist_pop_head (&browse_selection_queue);
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
      g_object_set_data_full ((GObject*) selection, "GxkTreeSelection-last", path, GDestroyNotify (gtk_tree_path_free));
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
 * @param selection	GtkTreeSelection to watch
 * @param model	tree model used with @a selection
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
    g_signal_connect_data (selection, "changed", G_CALLBACK (browse_selection_changed), selection, NULL, GConnectFlags (0));
  if (model && !gxk_signal_handler_exists (model, "row-inserted", G_CALLBACK (browse_selection_changed), selection))
    g_signal_connect_object (model, "row-inserted", G_CALLBACK (browse_selection_changed), selection, G_CONNECT_SWAPPED);
  browse_selection_changed (selection);
}

/**
 * @param tree	valid GtkTreeView
 * @param x_p	x position
 * @param y_p	y position
 *
 * Retrieve the position of the bin window (row display area) of
 * a GtkTreeView widget once it's realized.
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
 * @param tree	valid GtkTreeView
 * @param row	row to retrieve area coordinates for
 * @param y_p	y position of @a row
 * @param height_p	height of @a row
 * @param content_area	whether the cell background area or content area is returned
 *
 * Retrieve the position and height of a row of a
 * GtkTreeView widget within its bin window.
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
 * @param tree	valid GtkTreeView
 * @param row	row to focus
 *
 * Force focus to @a row, causes automatic selection of
 * @a row in browse mode.
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
 * @param tree	valid GtkTreeView
 * @param row	row to test
 * @return		whether @a row is selected
 *
 * Check whether @a row in @a tree is selected.
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
 * @param tree	valid GtkTreeView
 * @return		first selected row or -1
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
 * @param tree	valid GtkTreeView
 * @param y	bin window y coordinate
 * @param row_p	row pointed to by @a y
 * @return		whether y lies within the visible area
 *
 * Retrieve the row within which @a y lies. If @a y lies
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
 * @param instance	object instance with signals
 * @param detailed_signal	signal name
 * @param callback	custom callback function
 * @param data	callback data
 * @return		whether callback is connected
 *
 * Find out whether a specific @a callback is connected to a
 * specific signal on an instance, the callback may be blocked.
 * @a detailed_signal may be
 * NULL to act as a wildcard. TRUE is returned if
 * the @a callback is found, FALSE otherwise.
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
					    (detail ? G_SIGNAL_MATCH_DETAIL : GSignalMatchType (0))),
				 signal_id, detail, NULL, (void*) callback, data) != 0)
	return TRUE;
    }
  else if (!detailed_signal)
    {
      if (g_signal_handler_find (instance, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
				 0, 0, NULL, (void*) callback, data) != 0)
	return TRUE;
    }
  else
    g_warning ("%s: signal name \"%s\" is invalid for instance `%p'", G_STRLOC, detailed_signal, instance);
  return FALSE;
}

/**
 * @param instance	object instance with signals
 * @param detailed_signal	signal name
 * @param callback	custom callback function
 * @param data	callback data
 * @return		whether callback is connected
 *
 * Find out whether a specific @a callback is pending
 * (connected and unblocked) for a
 * specific signal on an instance. @a detailed_signal may be
 * NULL to act as a wildcard. TRUE is returned if
 * the @a callback is found, FALSE otherwise.
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
					    (detail ? G_SIGNAL_MATCH_DETAIL : GSignalMatchType (0))),
				 signal_id, detail, NULL, (void*) callback, data) != 0)
	return TRUE;
    }
  else if (!detailed_signal)
    {
      if (g_signal_handler_find (instance, G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA | G_SIGNAL_MATCH_UNBLOCKED,
				 0, 0, NULL, (void*) callback, data) != 0)
	return TRUE;
    }
  else
    g_warning ("%s: signal name \"%s\" is invalid for instance `%p'", G_STRLOC, detailed_signal, instance);
  return FALSE;
}

/* --- Gtk bug fixes --- */
/**
 * @param ecell	valid GtkCellEditable
 * @return		whether editing got aborted
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
 * @param ecell	valid GtkCellEditable
 * @return		returns FALSE
 *
 * Call gtk_cell_editable_editing_done() if necessary and return FALSE.
 * This function is meant to be used to handle "notify::is-focus" signals
 * on GtkCellEditable widgets.
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
 * @param ifactory	valid GtkItemFactory
 * @param path	item factory path
 * @param sensitive	whether menu item should be sensitive
 * @return		menu item according to @a path
 *
 * This function turns the menu item found via
 * gxk_item_factory_get_item() (in-)sensitive
 * according to @a sensitive. Additional checks
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
 * @param ifactory	valid GtkItemFactory
 * @param path	item factory path
 * @return		menu item according to @a path
 *
 * This function strips unescaped underlines ('_') from @a path
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
 * @param ifactory	valid GtkItemFactory
 * @param path	item factory path
 * @return		widget according to @a path
 *
 * This function strips unescaped underlines ('_') from @a path
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
  double *xyscale = (double*) data;
  guint width = requisition->width;
  guint height = requisition->height;

  /* patch up requisition since gtk tends to allocate the viewport with the
   * requested size minus vscrollbar->width or minus hscrollbar->height.
   */
  if (GTK_IS_SCROLLED_WINDOW (widget->parent))
    {
      GtkScrolledWindow *scwin = GTK_SCROLLED_WINDOW (widget->parent);
      GtkRequisition requisition;
      if (xyscale[0] >= 1.0 && scwin->vscrollbar && scwin->vscrollbar_policy != GTK_POLICY_NEVER)
        {
          gtk_widget_size_request (scwin->vscrollbar, &requisition);
          width += requisition.width;
        }
      if (xyscale[1] >= 1.0 && scwin->hscrollbar && scwin->hscrollbar_policy != GTK_POLICY_NEVER)
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
 * @param widget	valid GtkWidget
 * @param xscale	fractional factor for screen width
 * @param yscale	fractional factor for screen height
 *
 * Proxy the size requisition of @a widget through the ::width-request
 * and ::height-request properties. This is useful only for immediate
 * children of a GtkScrolledWindow (e.g. a GtkViewport), to have
 * the GtkScrolledWindow honour the widgets size requisition.
 * If @a xscale or @a yscale is passed as -1, the corresponding dimension
 * ::width-request or ::height-request is left unset.
 * If @a xscale or @a yscale is passed a value between 0 and +1, it is
 * interpreted as a fraction of the screen width or screen height to
 * constrain the corresponding requested dimension.
 */
void
gxk_widget_proxy_requisition (GtkWidget *widget,
                              gdouble    xscale,
                              gdouble    yscale)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  g_signal_handlers_disconnect_by_func (widget, (void*) requisition_to_aux_info, NULL);
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
 * @param scwin	valid GtkScrolledWindow
 *
 * A normal GtkScrolledWindow requests extra space for a horizontal scrollbar
 * if the vertical scroll policy is set to @c GTK_POLICY_NEVER and vice versa,
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
 *
 * @param scwin	valid GtkScrolledWindow
 *
 * Undo the effect of a call to gxk_scrolled_window_spare_space().
 */
void
gxk_scrolled_window_unspare_space (GtkScrolledWindow*scwin)
{
  if (gxk_signal_handler_exists (scwin, "size-request", G_CALLBACK (scrolled_window_size_request_spare_space), NULL))
    g_signal_handlers_disconnect_by_func (scwin, (void*) scrolled_window_size_request_spare_space, NULL);
}

/**
 * @param child	valid GtkWidget
 * @param shadow_type	shadow around the GtkViewport
 * @param xrequest	fractional factor for screen width
 * @param yrequest	fractional factor for screen height
 * @param spare_space	whether to call gxk_scrolled_window_spare_space()
 * @return		the newly created GtkScrolledWindow
 *
 * Create a GtkScrolledWindow with a GtkViewport as parent of @a child.
 * The @a xrequest and @a yrequest arguments are passed on
 * to gxk_widget_proxy_requisition().
 */
GtkWidget*
gxk_scrolled_window_create (GtkWidget        *child,
                            GtkShadowType     shadow_type,
                            gdouble           xrequest,
                            gdouble           yrequest)
{
  GtkWidget *scwin = (GtkWidget*) g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                                   "visible", TRUE,
                                   "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
                                   "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
                                   NULL);
  child = gtk_widget_get_toplevel (child);
  GtkWidget *viewport = (GtkWidget*) g_object_new (GTK_TYPE_VIEWPORT,
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
 * @param widget	valid GtkWidget
 * @param client	valid GtkWidget
 *
 * Request the horizontal size of @a client as height
 * for @a widget.
 */
void
gxk_widget_request_hclient_height (GtkWidget       *widget,
                                   GtkWidget       *client)
{
  g_signal_handlers_disconnect_by_func (widget, (void*) request_hclient_height, client);
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
 * @param widget	valid GtkWidget
 * @param client	valid GtkWidget
 *
 * Request the vertical size of @a client as width
 * for @a widget.
 */
void
gxk_widget_request_vclient_width (GtkWidget       *widget,
                                  GtkWidget       *client)
{
  g_signal_handlers_disconnect_by_func (widget, (void*) request_vclient_width, client);
  g_signal_connect_after (widget, "size_request", G_CALLBACK (request_vclient_width), client);
}

/**
 * @param widget	valid GtkWidget
 * @param ancestor	valid GtkWidget
 * @return		whether @a ancestor is ancestor of @a widget
 *
 * This function checks whether @a widget and @a ancestor are equal,
 * or whether @a ancestor is an ancestor of @a widget, in the same
 * way gtk_widget_is_ancestor() tests it.
 */
gboolean
gxk_widget_has_ancestor (gpointer widget,
                         gpointer ancestor)
{
  GtkWidget *w = (GtkWidget*) widget, *a = (GtkWidget*) ancestor;
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
 * @param menu	valid GtkMenu
 * @param child	an immediate child of @a menu
 *
 * This function replaces gtk_menu_set_active(). The @a child to be
 * set as last selection is passed in as ordinary child pointer
 * and if the menu is attached to an option menu or menu button,
 * the attach widget is updated after the selection changed, due
 * to the emission of ::selection-done.
 */
void
gxk_menu_set_active (GtkMenu         *menu,
                     GtkWidget       *child)
{
  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (GTK_IS_WIDGET (child));

  gint nth = g_list_index (GTK_MENU_SHELL (menu)->children, child);
  if (nth >= 0 && child != menu->old_active_menu_item)
    {
      gtk_menu_set_active (menu, nth);
#if 1
      g_signal_emit_by_name (GTK_MENU_SHELL (menu), "selection-done");
#else
      GtkWidget *awidget = gtk_menu_get_attach_widget (menu);
      if (GTK_IS_OPTION_MENU (awidget))
        gtk_option_menu_set_history (GTK_OPTION_MENU (awidget), nth);
      if (GXK_IS_MENU_BUTTON (awidget))
        gxk_menu_button_update (GXK_MENU_BUTTON (awidget));
#endif
    }
}

/**
 * @param widget	valid GtkWidget
 * @param sensitive	whether @a widget should be sensitive
 * @param active	whether @a widget should be active
 *
 * Regulate a widgets state. The @a sensitive parameter
 * controls sensitivity like gtk_widget_set_sensitive()
 * and @a active controls whether the widget is active
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
        gxk_menu_set_active (GTK_MENU (widget->parent), widget);
      g_object_thaw_notify (G_OBJECT (widget));
    }
}

/**
 * @param widget	valid GtkWidget
 * @return		TRUE if gxk_widget_regulate() uses @a active for @a widget
 *
 * Check whether gxk_widget_regulate() will actually make
 * use of its @a active argument for @a widget. If not,
 * FALSE is returned, and gxk_widget_regulate() is
 * fully equivalent to just gtk_widget_set_sensitive().
 */
gboolean
gxk_widget_regulate_uses_active (GtkWidget *widget)
{
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (widget), "active");
  return GTK_IS_MENU_ITEM (widget) || (pspec && pspec->value_type == G_TYPE_BOOLEAN);
}

/**
 * @param window	valid GtkWindow
 * @return		valid GtkAccelGroup
 *
 * This function hands out an accel group for @a window
 * specifically targeted at holding accelerators of
 * menu items in this window.
 */
GtkAccelGroup*
gxk_window_get_menu_accel_group (GtkWindow *window)
{
  GtkAccelGroup *agroup = (GtkAccelGroup*) g_object_get_data ((GObject*) window, "GxkWindow-menu-accel-group");
  if (!agroup)
    {
      agroup = gtk_accel_group_new ();
      gtk_window_add_accel_group (window, agroup);
      g_object_set_data_full ((GObject*) window, "GxkWindow-menu-accel-group", agroup, g_object_unref);
    }
  return agroup;
}

static GdkGeometry*
window_get_geometry (GtkWindow *window)
{
  GdkGeometry *geometry = (GdkGeometry*) g_object_get_data ((GObject*) window, "gxk-GdkGeometry");
  if (!geometry)
    {
      geometry = g_new0 (GdkGeometry, 1);
      geometry->width_inc = 1;
      geometry->height_inc = 1;
      g_object_set_data_full ((GObject*) window, "gxk-GdkGeometry", geometry, g_free);
    }
  return geometry;
}

void
gxk_window_set_geometry_min_width (GtkWindow       *window,
                                   guint            min_width)
{
  GdkGeometry *geometry = window_get_geometry (window);
  if (geometry->min_width != (int) min_width)
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
  if (geometry->min_height != (int) min_height)
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
  if (geometry->width_inc != (int) width_increment)
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
  if (geometry->height_inc != (int) height_increment)
    {
      geometry->height_inc = height_increment;
      gtk_window_set_geometry_hints (GTK_WINDOW (window), NULL, geometry, GDK_HINT_MIN_SIZE | GDK_HINT_RESIZE_INC);
    }
}

static void
adjust_visibility (GtkWidget  *expander,
                   GParamSpec *pspec,
                   GtkWidget  *widget)
{
  gboolean expanded;
  g_return_if_fail (G_IS_PARAM_SPEC_BOOLEAN (pspec));
  g_object_get (expander, "expanded", &expanded, NULL);
  if (expanded)
    gtk_widget_show (widget);
  else
    gtk_widget_hide (widget);
}

/**
 * @param expander	valid GtkWidget with boolean ::expanded property
 * @param widget	valid GtkWidget
 *
 * Setup signal connections, so that the visibility of @a widget
 * is controlled by the ::expanded property of @a expander.
 */
void
gxk_expander_connect_to_widget (GtkWidget       *expander,
                                GtkWidget       *widget)
{
  if (!gxk_signal_handler_pending (expander, "notify::expanded", G_CALLBACK (adjust_visibility), widget))
    g_signal_connect_object (expander, "notify::expanded", G_CALLBACK (adjust_visibility), widget, GConnectFlags (0));
  g_object_notify ((GObject*) expander, "expanded");
}

/**
 * @param label a GtkLabel
 * @param ...   a list of PangoAttrType and value pairs terminated by -1.
 *
 * Sets Pango attributes on a GtkLabel in a more convenient way than
 * gtk_label_set_attributes().
 *
 * This function is useful if you want to change the font attributes
 * of a GtkLabel. This is an alternative to using PangoMarkup which
 * is slow to parse and akward to handle in an i18n-friendly way.
 *
 * The attributes are set on the complete label, from start to end. If
 * you need to set attributes on part of the label, you will have to
 * use the PangoAttributes API directly.
 *
 * This function is based on gimp_label_set_attributes().
 **/
void
gxk_label_set_attributes (GtkLabel *label,
                          ...)
{
  /* this function is based on gimp_label_set_attributes(), which is
   * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
   */
  g_return_if_fail (GTK_IS_LABEL (label));

  va_list         args;
  va_start (args, label);
  PangoAttrList  *attrs = pango_attr_list_new ();
  PangoAttribute *attr  = NULL;
  do
    {
      PangoAttrType attr_type = (PangoAttrType) va_arg (args, int); // PangoAttrType

      switch (attr_type)
        {
        case PANGO_ATTR_LANGUAGE:
          attr = pango_attr_language_new (va_arg (args, PangoLanguage *));
          break;
        case PANGO_ATTR_FAMILY:
          attr = pango_attr_family_new (va_arg (args, const gchar *));
          break;
        case PANGO_ATTR_STYLE:
          attr = pango_attr_style_new ((PangoStyle) va_arg (args, int)); // PangoStyle
          break;
        case PANGO_ATTR_WEIGHT:
          attr = pango_attr_weight_new ((PangoWeight) va_arg (args, int)); // PangoWeight
          break;
        case PANGO_ATTR_VARIANT:
          attr = pango_attr_variant_new ((PangoVariant) va_arg (args, int)); // PangoVariant
          break;
        case PANGO_ATTR_STRETCH:
          attr = pango_attr_stretch_new ((PangoStretch) va_arg (args, int)); // PangoStretch
          break;
        case PANGO_ATTR_SIZE:
          attr = pango_attr_size_new (va_arg (args, gint));
          break;
        case PANGO_ATTR_FONT_DESC:
          attr = pango_attr_font_desc_new (va_arg (args, const PangoFontDescription *));
          break;
        case PANGO_ATTR_FOREGROUND:
          {
            const PangoColor *color = va_arg (args, const PangoColor *);

            attr = pango_attr_foreground_new (color->red,
                                              color->green,
                                              color->blue);
          }
          break;
        case PANGO_ATTR_BACKGROUND:
          {
            const PangoColor *color = va_arg (args, const PangoColor *);

            attr = pango_attr_background_new (color->red,
                                              color->green,
                                              color->blue);
          }
          break;
        case PANGO_ATTR_UNDERLINE:
          attr = pango_attr_underline_new ((PangoUnderline) va_arg (args, int)); // PangoUnderline
          break;
        case PANGO_ATTR_STRIKETHROUGH:
          attr = pango_attr_underline_new ((PangoUnderline) va_arg (args, gboolean));
          break;
        case PANGO_ATTR_RISE:
          attr = pango_attr_rise_new (va_arg (args, gint));
          break;
        case PANGO_ATTR_SCALE:
          attr = pango_attr_scale_new (va_arg (args, gdouble));
          break;
        default:
          g_warning ("%s: invalid PangoAttribute type %d",
                     G_STRFUNC, attr_type);
        case -1:
        case PANGO_ATTR_INVALID:
          attr = NULL;
          break;
        }
      if (attr)
        {
          attr->start_index = 0;
          attr->end_index   = -1;
          pango_attr_list_insert (attrs, attr);
        }
    }
  while (attr);
  va_end (args);
  gtk_label_set_attributes (label, attrs);
  pango_attr_list_unref (attrs);
}


guint
gxk_container_get_insertion_slot (GtkContainer *container)
{
  guint *slots = (guint*) g_object_steal_data ((GObject*) container, "gxk-container-slots");
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
  g_object_set_data_full ((GObject*) container, "gxk-container-slots", slots, g_free);
  return n_slots;
}

void
gxk_container_slot_reorder_child (GtkContainer    *container,
                                  GtkWidget       *widget,
                                  guint            sloti)
{
  guint *slots = (guint*) g_object_get_data ((GObject*) container, "gxk-container-slots");
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
 * @param window	the window receiving the grab
 * @param owner_events	if TRUE, events will be reported relative to @a window
 * @param event_mask	mask of interesting events
 * @param confine_to	limits the pointer to the specified window
 * @param cursor	cursor to use for the duration of the grab
 * @param time	event time when grab is requested
 * @return		TRUE if pointer and keyboard could successfully be grabbed
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
 * @param window	window pointer was previously grabed on
 *
 * This function releases a pointer and keyboard grab
 * acquired through gxk_grab_pointer_and_keyboard().
 * The @a window is used to release grabs on the correct
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
 * @param menu	valid GtkMenu
 * @return		TRUE if @a menu contains selectable items
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
      GtkWidget *child = (GtkWidget*) list->data;
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
gxk_menu_refetch_accel_group (GtkMenu *menu)
{
  GtkWidget *toplevel = gxk_widget_get_attach_toplevel ((GtkWidget*) menu);
  if (GTK_IS_WINDOW (toplevel))
    gtk_menu_set_accel_group (menu, gxk_window_get_menu_accel_group ((GtkWindow*) toplevel));
}

/**
 * @param menu	valid GtkMenu
 * @param menu_item	valid GtkMenuItem
 *
 * This function is a replacement for
 * gtk_menu_item_set_submenu(). It installs
 * the necessary hooks on the @a menu to automatically
 * update sensitivity of @a menu_item in response
 * to children being deleted or added to the @a menu.
 * The rationale behind this is to avoid empty
 * submenus popups.
 * Also, a propagation mechanism is set up, so @a menu
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

  gxk_widget_proxy_hierarchy_changed_to_attached (GTK_WIDGET (menu_item));

  if (!gxk_signal_handler_exists (menu, "parent-set", G_CALLBACK (submenu_adjust_sensitivity), NULL))
    g_object_connect (menu,
                      "signal_after::parent-set", submenu_adjust_sensitivity, NULL,
                      "signal_after::add", submenu_adjust_sensitivity, NULL,
                      "signal_after::remove", submenu_adjust_sensitivity, NULL,
                      NULL);
  submenu_adjust_sensitivity (menu);
}

/**
 * @param option_menu	valid GtkOptionMenu
 * @param menu	valid GtkMenu
 *
 * This function is a replacement for
 * gtk_option_menu_set_menu(). Similar to
 * gxk_menu_attach_as_submenu(), it sets up
 * a propagation mechanism, so @a menu
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

  gxk_widget_proxy_hierarchy_changed_to_attached (GTK_WIDGET (option_menu));
}

static void
popup_menus_detach (gpointer data)
{
  GList *menu_list = (GList*) data;
  while (menu_list)
    {
      GtkMenu *menu = (GtkMenu*) g_list_pop_head (&menu_list);
      if (gtk_menu_get_attach_widget (menu))
        gtk_menu_detach (menu);
    }
}

static void
popup_menu_detacher (GtkWidget *widget,
                     GtkMenu   *menu)
{
  GList *menu_list = (GList*) g_object_steal_data ((GObject*) widget, "GxkWidget-popup-menus");
  GtkMenuDetachFunc mdfunc;
  if (menu_list)
    {
      menu_list = g_list_remove (menu_list, menu);
      g_object_set_data_full ((GObject*) widget, "GxkWidget-popup-menus", menu_list, popup_menus_detach);
    }
  mdfunc = (GtkMenuDetachFunc) g_object_get_data ((GObject*) menu, "gxk-GtkMenuDetachFunc");
  if (mdfunc)
    mdfunc (widget, menu);
}

/**
 * @param menu	valid GtkMenu
 * @param menu_item	valid GtkMenuItem
 * @param mdfunc	a GtkMenuDetachFunc func as in gtk_menu_attach_to_widget()
 *
 * Variant of gxk_menu_attach_as_popup() which preserves the GtkMenuDetachFunc.
 */
void
gxk_menu_attach_as_popup_with_func (GtkMenu          *menu,
                                    GtkWidget        *widget,
                                    GtkMenuDetachFunc mdfunc)
{
  g_return_if_fail (GTK_IS_MENU (menu));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  GList *menu_list = (GList*) g_object_steal_data ((GObject*) widget, "GxkWidget-popup-menus");
  menu_list = g_list_prepend (menu_list, menu);
  g_object_set_data_full ((GObject*) widget, "GxkWidget-popup-menus", menu_list, popup_menus_detach);
  g_object_set_data ((GObject*) menu, "gxk-GtkMenuDetachFunc", (void*) mdfunc);
  gtk_menu_attach_to_widget (menu, widget, popup_menu_detacher);

  gxk_widget_proxy_hierarchy_changed_to_attached (widget);
}

/**
 * @param menu	valid GtkMenu
 * @param menu_item	valid GtkMenuItem
 *
 * This function is a replacement for gtk_menu_attach_to_widget().
 * Similar to gxk_menu_attach_as_submenu(), it sets up a propagation
 * mechanism, so @a menu and submenus thereof automatically fetch their
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
  gint     x, y, pushed_x, pushed_y;
  guint    pushed_in : 1;
  guint    pushable : 1;
} PopupData;

static gboolean
menu_position_unpushed (GtkMenu   *menu,
                        gint      *x,
                        gint      *y)
{
  /* lift up if too close to edge */
  GtkRequisition requisition;
  gtk_widget_get_child_requisition (GTK_WIDGET (menu), &requisition);
  if (*y + requisition.height > gdk_screen_height())
    {
      *y = MAX (0, gdk_screen_height() - requisition.height);
      return FALSE;
    }
  else
    return TRUE;
}

static void
menu_position_pushed_in (GtkMenu   *menu,
                         gint      *x,
                         gint      *y,
                         GtkWidget *active)
{
  GList *list;
  for (list = GTK_MENU_SHELL (menu)->children; list; list = list->next)
    {
      GtkWidget *child = (GtkWidget*) list->data;
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

static void
menu_position_func (GtkMenu  *menu,
                    gint     *x,
                    gint     *y,
                    gboolean *push_in,
                    gpointer  func_data)
{
  PopupData *pdata = (PopupData*) func_data;
  *push_in = FALSE;
  if (pdata->pushed_in)
    {
      GtkWidget *active = gtk_menu_get_active (menu);
      if (active)
        {
          *x = pdata->pushed_x;
          *y = pdata->pushed_y;
          *push_in = TRUE;
          menu_position_pushed_in (menu, x, y, active);
        }
      else
        {
          *x = pdata->x;
          *y = pdata->y;
          *push_in = FALSE;
          menu_position_unpushed (menu, x, y);
        }
    }
  else
    {
      *x = pdata->x;
      *y = pdata->y;
      *push_in = FALSE;
      gboolean fitscreen = menu_position_unpushed (menu, x, y);
      if (!fitscreen && pdata->pushable)
        {
          GtkWidget *active = gtk_menu_get_active (menu);
          if (active)
            {
              *x = pdata->pushed_x;
              *y = pdata->pushed_y;
              *push_in = TRUE;
              menu_position_pushed_in (menu, x, y, active);
            }
        }
    }
}

void
gxk_menu_popup (GtkMenu *menu,
                gint     x,
                gint     y,
                guint    mouse_button,
                guint32  time)
{
  g_return_if_fail (GTK_IS_MENU (menu));
  PopupData *pdata = g_new0 (PopupData, 1);
  pdata->x = x;
  pdata->y = y;
  gtk_menu_popup (menu, NULL, NULL, menu_position_func, pdata, mouse_button, time);
}

void
gxk_menu_popup_pushable (GtkMenu         *menu,
                         gint             x,
                         gint             y,
                         gint             pushed_x,
                         gint             pushed_y,
                         guint            mouse_button,
                         guint32          time)
{
  g_return_if_fail (GTK_IS_MENU (menu));
  PopupData *pdata = g_new0 (PopupData, 1);
  pdata->x = x;
  pdata->y = y;
  pdata->pushed_x = pushed_x;
  pdata->pushed_y = pushed_y;
  pdata->pushable = TRUE;
  gtk_menu_popup (menu, NULL, NULL, menu_position_func, pdata, mouse_button, time);
}

void
gxk_menu_popup_pushed_in (GtkMenu         *menu,
                          gint             pushed_x,
                          gint             pushed_y,
                          guint            mouse_button,
                          guint32          time)
{
  g_return_if_fail (GTK_IS_MENU (menu));
  PopupData *pdata = g_new0 (PopupData, 1);
  pdata->pushed_x = pushed_x;
  pdata->pushed_y = pushed_y;
  pdata->pushed_in = TRUE;
  gtk_menu_popup (menu, NULL, NULL, menu_position_func, pdata, mouse_button, time);
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
          GtkWidget *child = (GtkWidget*) list->data;
          if (child->name && strcmp (child->name, name) == 0)
            {
              g_list_free (children);
              return child;
            }
        }
      /* none found, search next level */
      for (list = children; list; list = list->next)
        {
          widget = (GtkWidget*) list->data;
          if (GTK_IS_CONTAINER (widget))
            newlist = g_list_concat (gtk_container_get_children (GTK_CONTAINER (widget)), newlist);
          if (GTK_IS_MENU_ITEM (widget))
            {
              GtkMenuItem *mitem = GTK_MENU_ITEM (widget);
              if (mitem->submenu)
                newlist = g_list_prepend (newlist, mitem->submenu);
            }
          GList *extra_children = (GList*) g_object_get_data ((GObject*) widget, "GxkWidget-popup-menus");
          if (extra_children)
            newlist = g_list_concat (g_list_copy (extra_children), newlist);
        }
      g_list_free (children);
      children = newlist;
    }
  return NULL;
}

/**
 * @param toplevel	valid GtkWidget
 * @param name	name of the widget being looked for
 * @return		a widget named @a name or NULL
 *
 * Search for a widget named @a name, child of @a toplevel.
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
 * @param widget	valid GtkWidget
 *
 * This function returns the topmost container widget
 * for @a widget, much like gtk_widget_get_toplevel().
 * The only difference is that for menus, not the immediate
 * parent is returned (the GtkWindow used to display a
 * menu) but the tree walk continues on the menu item
 * using the menu as submenu.
 * For example, for a window containing a menubar with
 * submenus, gtk_widget_get_toplevel() invoked on one
 * of the menu items will return the GtkWindow widgets
 * for the corresponding submenus, while
 * gxk_widget_get_attach_toplevel() will return the
 * actual GtkWindow containing the menubar.
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
 * @param widget	valid GtkWidget
 * @param n_chars	number of characters to request space for
 * @param n_digits	number of digits to request space for
 *
 * This function adds up extra space to the widget size
 * requisition. The space is an approximation of the space
 * required by @a n_chars characters and @a n_digits digits
 * written with the widget's font.
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
 * @param widget	valid GtkWidget
 * @return		custom options set on the widget
 *
 * This function returns the set of custom options
 * currently set on the widget.
 */
const gchar*
gxk_widget_get_options (gpointer widget)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  return (const char*) g_object_get_data ((GObject*) widget, "GxkWidget-options");
}

/**
 * @param widget	valid GtkWidget
 * @param option	option to add to widget
 * @param value	value of @a option (currently just "+" and "-" are supported)
 *
 * Add/set a custom @a option of @a widget to a particular @a value.
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
  options = (const char*) g_object_get_data ((GObject*) widget, "GxkWidget-options");
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
      g_object_set_data_full ((GObject*) widget, "GxkWidget-options", s, g_free);
    }
}

/**
 * @param widget	valid GtkWidget
 * @param option	option to check for
 * @return		whether @a option is set
 *
 * Test whether the custom @a option is set on @a widget.
 */
gboolean
gxk_widget_check_option (gpointer         widget,
                         const gchar     *option)
{
  const gchar *options;
  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  options = (const char*) g_object_get_data ((GObject*) widget, "GxkWidget-options");
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
 * @param fs	valid GtkFileSelection
 * @return		new toplevel VBox of the file selection
 *
 * Fixup various oddities that happened to the Gtk+
 * file selection widget over time. This function
 * corrects container border widths, spacing, button
 * placement and the default and focus widgets.
 * Also, the lifetime of the file selection window
 * is tied to the returned GtkVBox, enabling removal
 * of the GtkVBox from it's parent and thus using the
 * file selection widgets in a custom GtkWindow.
 */
GtkWidget*
gxk_file_selection_heal (GtkFileSelection *fs)
{
  GtkWidget *any, *main_box;

  main_box = gxk_file_selection_split (fs, NULL);

  /* add obligatory button seperator */
  any = (GtkWidget*) g_object_new (GTK_TYPE_HSEPARATOR,
                                   "visible", TRUE,
                                   NULL);
  gtk_box_pack_end (GTK_BOX (main_box), any, FALSE, TRUE, 0);

  return main_box;
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
  guint8 *text = (guint8*) g_malloc (len);
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
  return (char*) text;
}
