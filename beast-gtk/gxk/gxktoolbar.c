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
#include "gxktoolbar.h"

#include "gxkstock.h"


/* --- prototypes --- */
static void	gxk_toolbar_class_init		(GxkToolbarClass	*class);
static void	gxk_toolbar_init		(GxkToolbar		*self);
static void	gxk_toolbar_finalize		(GObject		*object);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
GType
gxk_toolbar_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (GxkToolbarClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gxk_toolbar_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (GxkToolbar),
	0,      /* n_preallocs */
	(GInstanceInitFunc) gxk_toolbar_init,
      };

      type = g_type_register_static (GTK_TYPE_FRAME,
				     "GxkToolbar",
				     &type_info, 0);
    }

  return type;
}

static void
gxk_toolbar_class_init (GxkToolbarClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = gxk_toolbar_finalize;
}

static void
gxk_toolbar_init (GxkToolbar *self)
{
  g_object_set (self,
		"shadow_type", GTK_SHADOW_OUT,
		NULL);
  self->relief_style = GTK_RELIEF_NONE;
  self->icon_size = GXK_SIZE_TOOLBAR;
  self->icon_width = gxk_size_width (self->icon_size);
  self->icon_height = gxk_size_height (self->icon_size);
  self->size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  self->box = g_object_new (GTK_TYPE_HBOX,
			    "visible", TRUE,
			    "parent", self,
			    NULL);
  g_object_ref (self->box);
}

static void
gxk_toolbar_finalize (GObject *object)
{
  GxkToolbar *self = GXK_TOOLBAR (object);

  g_object_unref (self->size_group);
  self->size_group = NULL;
  g_object_unref (self->box);
  self->box = NULL;

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gxk_toolbar_new
 * @nullify_pointer: location of a pointer to nullify or %NULL
 * @RETURNS:         newly created toolbar
 *
 * Create a new toolbar and setup handlers to nullify @nullify_pointer
 * once the toolbar is destructed.
 */
GxkToolbar*
gxk_toolbar_new (gpointer  nullify_pointer)
{
  GtkWidget *toolbar;

  toolbar = g_object_new (GXK_TYPE_TOOLBAR,
			  "visible", TRUE,
			  NULL);
  if (nullify_pointer)
    g_object_connect (toolbar,
		      "swapped_signal::destroy", g_nullify_pointer, nullify_pointer,
		      NULL);

  return GXK_TOOLBAR (toolbar);
}

static void
update_child (GxkToolbar *self,
	      GtkWidget  *child)
{
  gpointer relief_data = g_object_get_data (G_OBJECT (child), "gxk-toolbar-relief");
  gpointer size_data = g_object_get_data (G_OBJECT (child), "gxk-toolbar-size");
  if (relief_data)
    g_object_set (relief_data,
		  "relief", self->relief_style,
		  NULL);
  if (size_data)
    g_object_set (size_data,
		  "width_request", self->icon_width,
		  "height_request", self->icon_height,
		  NULL);
}

static gboolean
button_event_filter (GtkWidget *widget,
		     GdkEvent  *event)
{
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
    case GDK_MOTION_NOTIFY:
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
      return TRUE;
    default:
      return FALSE;
    }
}

static void
button_event_window_lower (GtkButton *button)
{
  gdk_window_lower (button->event_window);
}

/**
 * gxk_toolbar_append
 * @self:       a toolbar as returned from gxk_toolbar_new()
 * @child_type: child type
 * @name:       name of the child wiget (underscores indicate hotkeys)
 * @tooltip:    tooltip to be displayed with this child
 * @icon:       image widget for buttons or a custom widget
 * @RETURNS:    newly created toolbar child
 *
 * Append a new child to the toolbar, named @name, with tooltip
 * @tooltip and containing @icon. @icon is usually an image widget,
 * unless %GXK_TOOLBAR_WIDGET or one of its variants was specified
 * as child type. The possible child types are:
 * %GXK_TOOLBAR_SPACE - add a vertical space;
 * %GXK_TOOLBAR_SEPARATOR - add a vertical seperator;
 * %GXK_TOOLBAR_BUTTON - add a button widget;
 * %GXK_TOOLBAR_TRUNC_BUTTON - like %GXK_TOOLBAR_BUTTON, but clip the
 * @name label if it width exceeds the button size;
 * %GXK_TOOLBAR_EXTRA_BUTTON - like %GXK_TOOLBAR_BUTTON, but leave the
 * button's size alone, it is not made homogeneous with other toolbar children.
 * %GXK_TOOLBAR_TOGGLE - add a toggle button widget;
 * %GXK_TOOLBAR_TRUNC_TOGGLE - like %GXK_TOOLBAR_TRUNC_BUTTON for toggle buttons;
 * %GXK_TOOLBAR_EXTRA_TOGGLE - like %GXK_TOOLBAR_EXTRA_BUTTON for toggle buttons;
 * %GXK_TOOLBAR_WIDGET - treat @icon as a custom widget, and don't put it into
 * a button, toggle button or other activatable container;
 * GXK_TOOLBAR_TRUNC_WIDGET - like %GXK_TOOLBAR_TRUNC_BUTTON for custom widgets;
 * GXK_TOOLBAR_EXTRA_WIDGET - like %GXK_TOOLBAR_EXTRA_BUTTON for custom widgets;
 * GXK_TOOLBAR_FILL_WIDGET - like %GXK_TOOLBAR_EXTRA_WIDGET but extra unused space
 * of the toolbar is given to this child.
 */
GtkWidget*
gxk_toolbar_append (GxkToolbar     *self,
		    GxkToolbarChild child_type,
		    const gchar    *name,
		    const gchar    *tooltip,
		    GtkWidget      *icon)
{
  GtkWidget *child = NULL;
  gpointer relief_data = NULL, size_data = NULL;
  guint padding = 0;
  gboolean expand_fill = FALSE;

  g_return_val_if_fail (GXK_IS_TOOLBAR (self), NULL);

  switch (child_type)
    {
      GtkWidget *any, *vbox;
      GType widget_type;
      gboolean extra, filtered, trunc;
    case GXK_TOOLBAR_SPACE:
      g_return_val_if_fail (name == NULL && tooltip == NULL && icon == NULL, NULL);
      child = g_object_new (GTK_TYPE_ALIGNMENT,
			    "visible", TRUE,
			    "width_request", 10,
			    NULL);
      break;
    case GXK_TOOLBAR_SEPARATOR:
      g_return_val_if_fail (name == NULL && tooltip == NULL && icon == NULL, NULL);
      child = g_object_new (GTK_TYPE_ALIGNMENT,
			    "visible", TRUE,
			    "yalign", 0.5,
			    "yscale", 0.4,
			    NULL);
      any = g_object_new (GTK_TYPE_VSEPARATOR,
			  "visible", TRUE,
			  "parent", child,
			  NULL);
      padding = 2;
      break;
    case GXK_TOOLBAR_BUTTON:
    case GXK_TOOLBAR_TRUNC_BUTTON:
    case GXK_TOOLBAR_EXTRA_BUTTON:
    case GXK_TOOLBAR_WIDGET:
    case GXK_TOOLBAR_TRUNC_WIDGET:
    case GXK_TOOLBAR_EXTRA_WIDGET:
    case GXK_TOOLBAR_FILL_WIDGET:
    case GXK_TOOLBAR_TOGGLE:
    case GXK_TOOLBAR_TRUNC_TOGGLE:
    case GXK_TOOLBAR_EXTRA_TOGGLE:
      if (child_type == GXK_TOOLBAR_TOGGLE ||
	  child_type == GXK_TOOLBAR_TRUNC_TOGGLE ||
	  child_type == GXK_TOOLBAR_EXTRA_TOGGLE)
	widget_type = GTK_TYPE_TOGGLE_BUTTON;
      else
	widget_type = GTK_TYPE_BUTTON;
      filtered = (child_type == GXK_TOOLBAR_WIDGET ||
		  child_type == GXK_TOOLBAR_TRUNC_WIDGET ||
		  child_type == GXK_TOOLBAR_EXTRA_WIDGET ||
		  child_type == GXK_TOOLBAR_FILL_WIDGET);
      trunc = (child_type == GXK_TOOLBAR_TRUNC_BUTTON ||
	       child_type == GXK_TOOLBAR_TRUNC_TOGGLE ||
	       child_type == GXK_TOOLBAR_TRUNC_WIDGET);
      extra = (child_type == GXK_TOOLBAR_EXTRA_BUTTON ||
	       child_type == GXK_TOOLBAR_EXTRA_TOGGLE ||
	       child_type == GXK_TOOLBAR_EXTRA_WIDGET ||
	       child_type == GXK_TOOLBAR_FILL_WIDGET);
      expand_fill = child_type == GXK_TOOLBAR_FILL_WIDGET;
      child = g_object_new (widget_type,
			    "visible", TRUE,
			    "can_focus", FALSE,
			    NULL);
      if (filtered)
	{
	  g_object_connect (child,
			    "signal::event", button_event_filter, NULL,
			    "signal_after::map", button_event_window_lower, NULL,
			    NULL);
	  g_object_set (child,
			"relief", GTK_RELIEF_NONE,
			NULL);
	}
      else
	relief_data = child;
      vbox = g_object_new (GTK_TYPE_VBOX,
			   "visible", TRUE,
			   "parent", child,
			   NULL);
      any = g_object_new (GTK_TYPE_ALIGNMENT,
			  "visible", TRUE,
			  "xalign", 0.5,
			  "yalign", 0.5,
			  "yscale", 1.0,
			  "xscale", 1.0,
			  icon ? "child" : NULL, icon,
			  NULL);
      if (filtered && icon)
	gtk_tooltips_set_tip (GXK_TOOLTIPS, icon, tooltip, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), any, TRUE, TRUE, 0);
      size_data = filtered ? NULL : icon;
      if (name)
	{
	  GtkWidget *label = g_object_new (GTK_TYPE_LABEL,
					   "visible", TRUE,
					   "label", name,
					   "use_underline", TRUE,
					   trunc ? "xalign" : NULL, 0.0,
					   NULL);
	  g_object_set_data (G_OBJECT (child), "gxk-toolbar-label", label);
	  if (trunc)
	    gtk_box_pack_end (GTK_BOX (vbox),
			      g_object_new (GTK_TYPE_ALIGNMENT,
					    "visible", TRUE,
					    "child", label,
					    "xalign", 0.5,
					    "yalign", 1.0,
					    "xscale", 0.0,
					    "yscale", 0.0,
					    "width_request", 1,
					    NULL),
			      FALSE, FALSE, 0);
	  else
	    gtk_box_pack_end (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	}
      if (!extra)
	gtk_size_group_add_widget (self->size_group, child);
      break;
    }
  g_object_set_data (G_OBJECT (child), "gxk-toolbar-size", size_data);
  g_object_set_data (G_OBJECT (child), "gxk-toolbar-relief", relief_data);
  update_child (self, child);
  gtk_box_pack_start (GTK_BOX (self->box), child, expand_fill, expand_fill, padding);
  gtk_tooltips_set_tip (GXK_TOOLTIPS, child, tooltip, NULL);

  return child;
}

/**
 * gxk_toolbar_append_stock
 * @self:       a toolbar as returned from gxk_toolbar_new()
 * @child_type: child type
 * @name:       name of the child wiget (underscores indicate hotkeys)
 * @tooltip:    tooltip to be displayed with this child
 * @stock_id:   stock name
 * @RETURNS:    newly created toolbar child
 *
 * Like gxk_toolbar_append() but create the icon widget from @stock_id.
 */
GtkWidget*
gxk_toolbar_append_stock (GxkToolbar     *self,
			  GxkToolbarChild child_type,
			  const gchar    *name,
			  const gchar    *tooltip,
			  const gchar    *stock_id)
{
  g_return_val_if_fail (GXK_IS_TOOLBAR (self), NULL);

  return gxk_toolbar_append (self, child_type, name, tooltip,
			     gxk_stock_image (stock_id, self->icon_size));
}

typedef struct {
  GtkWidget           *alignment;
  GtkWidget           *menu;
  GxkToolbarChoiceFunc choice_func;
  gpointer             data;
  GDestroyNotify       data_free;
} GxkToolbarChoiceData;

static void
free_choice_data (gpointer data)
{
  GxkToolbarChoiceData *cdata = data;

  if (cdata->data_free)
    cdata->data_free (cdata->data);
}

static void
menu_position_func (GtkMenu  *menu,
		    gint     *x_p,
		    gint     *y_p,
		    gboolean *push_in_p,
		    gpointer  user_data)
{
  GtkWidget *widget = user_data;
  gint x, y;

  gdk_window_get_origin (widget->window, &x, &y);
  if (GTK_WIDGET_NO_WINDOW (widget))
    {
      *x_p = x + widget->allocation.x;
      *y_p = y + widget->allocation.y;
    }
  else
    {
      *x_p = x;
      *y_p = y;
    }
  *push_in_p = FALSE;
}

static void
button_menu_popup (GtkWidget *button)
{
  GxkToolbarChoiceData *cdata = g_object_get_data (G_OBJECT (button), "gxk-toolbar-choice-data");
  GdkEvent *event = gtk_get_current_event ();

  if (event)
    {
      GtkWidget *icon = GTK_BIN (cdata->alignment)->child;
      GtkWidget *parent = icon ? g_object_get_data (G_OBJECT (icon), "gxk-toolbar-parent") : NULL;

      /* fix alignment size across removing child */
      g_object_set (cdata->alignment,
		    "width_request", cdata->alignment->requisition.width,
		    "height_request", cdata->alignment->requisition.height,
		    NULL);
      if (parent)
	{
	  g_object_ref (icon);
	  gtk_container_remove (GTK_CONTAINER (cdata->alignment), icon);
	  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (parent), icon);
	  g_object_unref (icon);
	  /* remember last setting */
	  g_object_set_data (G_OBJECT (cdata->alignment), "gxk-toolbar-last-icon", icon);
	}
      gtk_menu_popup (GTK_MENU (cdata->menu), NULL, NULL, menu_position_func, button, 0, gdk_event_get_time (event));
    }
}

static void
button_menu_popdown (GtkWidget *button)
{
  GxkToolbarChoiceData *cdata = g_object_get_data (G_OBJECT (button), "gxk-toolbar-choice-data");

  if (!GTK_BIN (cdata->alignment)->child)
    {
      /* probably aborted, restore last setting */
      GtkWidget *icon = g_object_get_data (G_OBJECT (cdata->alignment), "gxk-toolbar-last-icon");
      if (icon)
	{
	  g_object_ref (icon);
	  if (icon->parent)
	    gtk_container_remove (GTK_CONTAINER (icon->parent), icon);
	  gtk_container_add (GTK_CONTAINER (cdata->alignment), icon);
	  g_object_unref (icon);
	}
    }
  g_object_set_data (G_OBJECT (cdata->alignment), "gxk-toolbar-last-icon", NULL);
  /* restore alignment size */
  g_object_set (cdata->alignment,
		"width_request", -1,
		"height_request", -1,
		NULL);
}

/**
 * gxk_toolbar_append_choice
 * @self:        a toolbar as returned from gxk_toolbar_new()
 * @child_type:  child type (one for the button variants)
 * @choice_func: callback notified if the choice changes
 * @data:        extra data argument to the callback
 * @data_free:   callback to destroy @data
 * @RETURNS:     newly created toolbar choice child
 *
 * Create and append a new toolbar child which offers
 * a choice menu of various options to the user.
 */
GtkWidget*
gxk_toolbar_append_choice (GxkToolbar          *self,
			   GxkToolbarChild      child_type,
			   GxkToolbarChoiceFunc choice_func,
			   gpointer             data,
			   GDestroyNotify       data_free)
{
  GxkToolbarChoiceData *cdata;
  GtkWidget *button;

  g_return_val_if_fail (GXK_IS_TOOLBAR (self), NULL);
  g_return_val_if_fail (choice_func != NULL, NULL);
  g_return_val_if_fail (child_type >= GXK_TOOLBAR_BUTTON && child_type <= GXK_TOOLBAR_EXTRA_BUTTON, NULL);

  cdata = g_new0 (GxkToolbarChoiceData, 1);
  cdata->choice_func = choice_func;
  cdata->data = data;
  cdata->data_free = data_free;
  cdata->alignment = g_object_new (GTK_TYPE_ALIGNMENT,
				   "visible", TRUE,
				   NULL);
  button = gxk_toolbar_append (self, child_type,
			       "drop-down", NULL, cdata->alignment);
  cdata->menu = g_object_new (GTK_TYPE_MENU, NULL);
  g_object_connect (cdata->menu,
		    "swapped_object_signal::selection-done", button_menu_popdown, button,
		    NULL);
  g_object_set_data_full (G_OBJECT (button), "gxk-toolbar-choice-data", cdata, free_choice_data);
  g_object_connect (button,
		    "signal::clicked", button_menu_popup, NULL,
		    NULL);

  return button;
}

static void
menu_item_select (GtkWidget *item,
		  GtkWidget *button)
{
  GxkToolbarChoiceData *cdata = g_object_get_data (G_OBJECT (button), "gxk-toolbar-choice-data");
  GtkWidget *label = g_object_get_data (G_OBJECT (button), "gxk-toolbar-label");
  GtkWidget *icon = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (item));
  gpointer choice = g_object_get_data (G_OBJECT (item), "gxk-toolbar-choice");

  if (cdata->alignment && icon)
    {
      if (GTK_BIN (cdata->alignment)->child)
	{
	  GtkWidget *old_icon = GTK_BIN (cdata->alignment)->child;
	  GtkWidget *old_parent = g_object_get_data (G_OBJECT (old_icon), "gxk-toolbar-parent");
	  /* remove old child */
	  g_object_ref (old_icon);
	  gtk_container_remove (GTK_CONTAINER (old_icon->parent), old_icon);
	  if (old_parent)
	    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (old_parent), old_icon);
	  g_object_unref (old_icon);
	}
      g_object_ref (icon);
      gtk_container_remove (GTK_CONTAINER (icon->parent), icon);
      gtk_container_add (GTK_CONTAINER (cdata->alignment), icon);
      g_object_unref (icon);
    }
  if (label)
    gtk_label_set (GTK_LABEL (label), g_object_get_data (G_OBJECT (item), "gxk-toolbar-name"));
  gtk_tooltips_set_tip (GXK_TOOLTIPS, button, g_object_get_data (G_OBJECT (item), "gxk-toolbar-tooltip"), NULL);

  if (cdata->alignment)
    g_object_set (cdata->alignment,
		  "width_request", -1,
		  "height_request", -1,
		  NULL);
  cdata->choice_func (cdata->data, GPOINTER_TO_UINT (choice));
}

static GtkWidget*
toolbar_choice_add (GtkWidget            *widget,
		    GxkToolbarChoiceData *cdata,
		    const gchar          *name,
		    const gchar          *tooltip,
		    GtkWidget            *icon)
{
  GtkWidget *item;

  item = gtk_image_menu_item_new_with_label (name);
  if (icon)
    g_object_set_data (G_OBJECT (icon), "gxk-toolbar-parent", item);
  g_object_set (item,
		"visible", TRUE,
		"image", icon,
		"parent", cdata->menu,
		NULL);
  g_object_connect (item,
		    "object_signal::activate", menu_item_select, widget,
		    NULL);
  g_object_set_data_full (G_OBJECT (item), "gxk-toolbar-tooltip", g_strdup (tooltip), g_free);
  g_object_set_data_full (G_OBJECT (item), "gxk-toolbar-name", g_strdup (name), g_free);
  return item;
}

/**
 * gxk_toolbar_choice_add
 * @widget:      a toolbar child as returned from gxk_toolbar_append_choice()
 * @name:        name of the choice child wiget
 * @tooltip:     tooltip to be displayed with this choice child
 * @icon:        image widget for this choice
 * @choice:      unique integer id to identify this choice
 * @RETURNS:     newly created toolbar choice child
 *
 * Add a choice variant to a toolbar choice child.
 */
void
gxk_toolbar_choice_add (GtkWidget   *widget,
			const gchar *name,
			const gchar *tooltip,
			GtkWidget   *icon,
			guint        choice)
{
  GxkToolbarChoiceData *choice_data;
  GtkWidget *item;
  gboolean need_selection;

  g_return_if_fail (GTK_IS_BUTTON (widget));
  choice_data = g_object_get_data (G_OBJECT (widget), "gxk-toolbar-choice-data");
  g_return_if_fail (choice_data != NULL);

  need_selection = GTK_MENU_SHELL (choice_data->menu)->children == NULL;
  item = toolbar_choice_add (widget, choice_data, name, tooltip, icon);
  g_object_set_data (G_OBJECT (item), "gxk-toolbar-choice", GUINT_TO_POINTER (choice));
  if (need_selection)
    menu_item_select (item, widget);
}

/**
 * gxk_toolbar_choice_set
 * @widget:      a toolbar child as returned from gxk_toolbar_append_choice()
 * @name:        name of the choice child wiget
 * @tooltip:     tooltip to be displayed with this choice child
 * @icon:        image widget for this choice
 * @choice:      unique integer id to identify this choice
 * @RETURNS:     newly created toolbar choice child
 *
 * Add a choice variant to a toolbar choice child and make
 * this variant the currently selected one.
 */
void
gxk_toolbar_choice_set (GtkWidget   *widget,
			const gchar *name,
			const gchar *tooltip,
			GtkWidget   *icon,
			guint        choice)
{
  GxkToolbarChoiceData *choice_data;
  GtkWidget *item;

  g_return_if_fail (GTK_IS_BUTTON (widget));
  choice_data = g_object_get_data (G_OBJECT (widget), "gxk-toolbar-choice-data");
  g_return_if_fail (choice_data != NULL);

  item = toolbar_choice_add (widget, choice_data, name, tooltip, icon);
  g_object_set_data (G_OBJECT (item), "gxk-toolbar-choice", GUINT_TO_POINTER (choice));
  menu_item_select (item, widget);
}
