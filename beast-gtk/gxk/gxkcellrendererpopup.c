/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2003 Tim Janik
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
#include "gxkcellrendererpopup.h"
#include "gxkmarshal.h"
#include <gtk/gtkhbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkarrow.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>


#define	ARROW_WIDTH	20

enum {
  PROP_NONE,
  PROP_SHOW_BUTTON
};


/* --- prototypes --- */
static void	gxk_cell_renderer_popup_init		(GxkCellRendererPopup	   *self);
static void	gxk_cell_renderer_popup_class_init	(GxkCellRendererPopupClass *class);
static void	gxk_cell_renderer_popup_get_property	(GObject		*object,
							 guint			 param_id,
							 GValue			*value,
							 GParamSpec		*pspec);
static void	gxk_cell_renderer_popup_set_property	(GObject		*object,
							 guint			 param_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	gxk_cell_renderer_popup_get_size	(GtkCellRenderer	*cell,
							 GtkWidget		*widget,
							 GdkRectangle		*cell_area,
							 gint			*xoffs_p,
							 gint			*yoffs_p,
							 gint			*width_p,
							 gint			*height_p);
static void	gxk_cell_renderer_popup_render		(GtkCellRenderer	*cell,
							 GdkWindow		*window,
							 GtkWidget		*widget,
							 GdkRectangle		*background_area,
							 GdkRectangle		*cell_area,
							 GdkRectangle		*expose_area,
							 GtkCellRendererState	 flags);
static gboolean	gxk_cell_renderer_popup_activate	(GtkCellRenderer	*cell,
							 GdkEvent		*event,
							 GtkWidget		*widget,
							 const gchar		*path,
							 GdkRectangle		*background_area,
							 GdkRectangle		*cell_area,
							 GtkCellRendererState	 flags);
static GtkCellEditable* gxk_cell_renderer_popup_start_editing (GtkCellRenderer      *cell,
							       GdkEvent             *event,
							       GtkWidget            *widget,
							       const gchar          *path,
							       GdkRectangle         *background_area,
							       GdkRectangle         *cell_area,
							       GtkCellRendererState  flags);


/* --- variables --- */
static gpointer parent_class = NULL;
static guint    signal_popup = 0;


/* --- functions --- */
/**
 * gxk_cell_renderer_popup_get_type
 * @RETURNS: %GXK_TYPE_CELL_RENDERER_POPUP
 *
 * #GxkCellRendererPopup is an editable text cell renderer which
 * supports popping up an auxillary window.
 */
GType
gxk_cell_renderer_popup_get_type (void)
{
  static GType type = 0;
  
  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (GxkCellRendererPopupClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gxk_cell_renderer_popup_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (GxkCellRendererPopup),
	0,      /* n_preallocs */
	(GInstanceInitFunc) gxk_cell_renderer_popup_init,
      };
      
      type = g_type_register_static (GTK_TYPE_CELL_RENDERER_TEXT,
				     "GxkCellRendererPopup",
				     &type_info, 0);
    }
  return type;
}

static void
gxk_cell_renderer_popup_init (GxkCellRendererPopup *self)
{
  self->show_button = TRUE;
  // GTK_CELL_RENDERER (self)->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
  // GTK_CELL_RENDERER (self)->xpad = 2;
  // GTK_CELL_RENDERER (self)->ypad = 2;
}

static void
gxk_cell_renderer_popup_class_init (GxkCellRendererPopupClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->get_property = gxk_cell_renderer_popup_get_property;
  object_class->set_property = gxk_cell_renderer_popup_set_property;
  
  cell_class->get_size = gxk_cell_renderer_popup_get_size;
  cell_class->render = gxk_cell_renderer_popup_render;
  cell_class->activate = gxk_cell_renderer_popup_activate;
  cell_class->start_editing = gxk_cell_renderer_popup_start_editing;
  
  g_object_class_install_property (object_class,
				   PROP_SHOW_BUTTON,
				   g_param_spec_boolean ("show_button", "Show Button", "Whether the popup button is visible",
							 TRUE, G_PARAM_READWRITE));
  signal_popup = g_signal_new ("popup",
			       G_OBJECT_CLASS_TYPE (object_class),
			       G_SIGNAL_RUN_LAST,
			       G_STRUCT_OFFSET (GxkCellRendererPopupClass, popup),
			       NULL, NULL,
			       gxk_marshal_VOID__STRING_STRING,
			       G_TYPE_NONE, 2,
			       G_TYPE_STRING,
			       G_TYPE_STRING);
}

static void
gxk_cell_renderer_popup_set_property (GObject      *object,
				      guint         param_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
  GxkCellRendererPopup *self = GXK_CELL_RENDERER_POPUP (object);
  
  switch (param_id)
    {
    case PROP_SHOW_BUTTON:
      self->show_button = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gxk_cell_renderer_popup_get_property (GObject    *object,
				      guint       param_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
  GxkCellRendererPopup *self = GXK_CELL_RENDERER_POPUP (object);
  
  switch (param_id)
    {
    case PROP_SHOW_BUTTON:
      g_value_set_boolean (value, self->show_button);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gxk_cell_renderer_popup_get_size (GtkCellRenderer *cell,
				  GtkWidget       *widget,
				  GdkRectangle    *cell_area,
				  gint            *xoffs_p,
				  gint            *yoffs_p,
				  gint            *width_p,
				  gint            *height_p)
{
  GxkCellRendererPopup *self = GXK_CELL_RENDERER_POPUP (cell);

  GTK_CELL_RENDERER_CLASS (parent_class)->get_size (cell, widget, cell_area, xoffs_p, yoffs_p, width_p, height_p);

  if (self->show_button)
    {
      if (width_p)
	*width_p += ARROW_WIDTH;
      if (xoffs_p)
	*xoffs_p = MAX (*xoffs_p - ARROW_WIDTH / 2, 0);
    }
}

static void
gxk_cell_renderer_popup_render (GtkCellRenderer     *cell,
				GdkWindow           *window,
				GtkWidget           *widget,
				GdkRectangle        *background_area,
				GdkRectangle        *cell_area,
				GdkRectangle        *expose_area,
				GtkCellRendererState flags)
{
  GTK_CELL_RENDERER_CLASS (parent_class)->render (cell, window, widget, background_area, cell_area, expose_area, flags);
}

static gboolean
gxk_cell_renderer_popup_activate (GtkCellRenderer     *cell,
				  GdkEvent            *event,
				  GtkWidget           *widget,
				  const gchar         *path,
				  GdkRectangle        *background_area,
				  GdkRectangle        *cell_area,
				  GtkCellRendererState flags)
{
  return GTK_CELL_RENDERER_CLASS (parent_class)->activate (cell, event, widget, path, background_area, cell_area, flags);
}

void
gxk_cell_renderer_popup_dialog (GxkCellRendererPopup *self,
				GtkWidget            *dialog)
{
  g_return_if_fail (GXK_IS_CELL_RENDERER_POPUP (self));
  if (dialog)
    g_return_if_fail (GTK_IS_WINDOW (dialog));

  if (self->dialog)
    {
      gxk_toplevel_delete (self->dialog);
      g_object_unref (self->dialog);
    }
  self->dialog = dialog;
  if (self->dialog)
    {
      g_object_ref (self->dialog);
      gxk_widget_showraise (self->dialog);
    }
}

static void
gxk_cell_renderer_popup_popdown (GxkCellRendererPopup *self)
{
  gxk_cell_renderer_popup_dialog (self, NULL);
}

static void
gxk_cell_renderer_popup_editing_done (GxkCellRendererPopup *self,
				      GtkEntry             *entry)
{
  const gchar *path = g_object_get_data (self, "gxk-cell-edit-path");
  if (entry->editing_canceled)
    return;
  g_signal_emit_by_name (self, "edited", path, gtk_entry_get_text (entry));
}

static gboolean
gxk_cell_renderer_popup_clicked (GxkCellRendererPopup *self)
{
  const gchar *path = g_object_get_data (self, "gxk-cell-edit-path");
  if (self->dialog && GTK_WIDGET_VISIBLE (self->dialog))
    gxk_cell_renderer_popup_dialog (self, NULL);
  else
    {
      gxk_cell_renderer_popup_dialog (self, NULL);
      g_signal_emit (self, signal_popup, 0, path, NULL);
    }
  return TRUE;	/* we handled the event */
}

static GtkCellEditable*
gxk_cell_renderer_popup_start_editing (GtkCellRenderer      *cell,
				       GdkEvent             *event,
				       GtkWidget            *widget,
				       const gchar          *path,
				       GdkRectangle         *background_area,
				       GdkRectangle         *cell_area,
				       GtkCellRendererState  flags)
{
  GxkCellRendererPopup *self = GXK_CELL_RENDERER_POPUP (cell);
  GtkCellRendererText *tcell = GTK_CELL_RENDERER_TEXT (self);
  GtkWidget *popup_area, *entry, *hbox;
  GtkCellEditable *ecell;
  GxkProxyEditable *eproxy;
  if (tcell->editable == FALSE)
    return NULL;
  entry = g_object_new (GTK_TYPE_ENTRY,
			"has_frame", FALSE,
			"visible", TRUE,
			NULL);
  g_object_connect (entry,
		    "swapped_signal::editing_done", gxk_cell_renderer_popup_editing_done, self,
		    "signal::notify::is-focus", gxk_cell_editable_is_focus_handler, self,
		    "swapped_signal::remove_widget", gxk_cell_renderer_popup_popdown, self,
		    NULL);
  if (tcell->text)
    gtk_entry_set_text (GTK_ENTRY (entry), tcell->text);
  ecell = GTK_CELL_EDITABLE (entry);
  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
  eproxy = g_object_new (GXK_TYPE_PROXY_EDITABLE,
			 "visible", TRUE,
			 "events", GDK_BUTTON_PRESS_MASK,
			 NULL);
  hbox = g_object_new (GTK_TYPE_HBOX,
		       "visible", TRUE,
		       "parent", eproxy,
		       "child", entry,
		       NULL);
#if 0	/* GTKFIX: this exhibits tree view scrolling+resizing bug in gtk+2.2 */
  popup_area = g_object_new (GTK_TYPE_BUTTON, "visible", self->show_button,
			     "can_focus", FALSE, "width_request", ARROW_WIDTH,
			     "child", g_object_new (GTK_TYPE_ARROW, "visible", TRUE,
						    "arrow_type", GTK_ARROW_DOWN, NULL),
			     NULL);
  g_object_connect (popup_area, "swapped_signal::clicked", gxk_cell_renderer_popup_clicked, cell, NULL);
#else
  popup_area = g_object_new (GTK_TYPE_FRAME, "visible", self->show_button,
			     "width_request", ARROW_WIDTH, "shadow_type", GTK_SHADOW_OUT,
			     "child", g_object_new (GTK_TYPE_LABEL, "visible", TRUE,
						    "label", "...", NULL),
			     NULL);
  g_object_connect (eproxy, "swapped_signal::button_press_event", gxk_cell_renderer_popup_clicked, cell, NULL);
#endif
  if (!g_signal_has_handler_pending (self, signal_popup, 0, FALSE))
    gtk_widget_set_sensitive (popup_area, FALSE);
  g_object_connect (eproxy,
		    // "swapped_signal::remove_widget", g_print, "remove_widget\n",
		    // "swapped_signal::editing_done", g_print, "editing_done\n",
		    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), popup_area, FALSE, FALSE, 0);
  gxk_proxy_editable_set_cell_editable (eproxy, ecell);
  g_object_set_data_full (cell, "gxk-cell-edit-path", g_strdup (path), g_free);
  return GTK_CELL_EDITABLE (eproxy);
}


/* --- GxkProxyEditable --- */
static void
gxk_proxy_editable_start_editing (GtkCellEditable *cell_editable,
				  GdkEvent        *event)
{
  GxkProxyEditable *self = GXK_PROXY_EDITABLE (cell_editable);
  if (!self->block_start_editing && self->ecell)
    {
      self->block_start_editing++;
      if (GTK_WIDGET_CAN_FOCUS (self->ecell))
	gtk_widget_grab_focus (GTK_WIDGET (self->ecell));
      gtk_cell_editable_start_editing (self->ecell, event);
      self->block_start_editing--;
    }
}

static void
gxk_proxy_editable_remove_widget (GtkCellEditable *cell_editable)
{
  GxkProxyEditable *self = GXK_PROXY_EDITABLE (cell_editable);
  if (!self->block_remove_widget && self->ecell)
    {
      self->block_remove_widget++;
      gtk_cell_editable_remove_widget (self->ecell);
      self->block_remove_widget--;
    }
}

static void
gxk_proxy_editable_proxy_remove_widget (GtkCellEditable *cell_editable)
{
  GxkProxyEditable *self = GXK_PROXY_EDITABLE (cell_editable);
  if (!self->block_remove_widget)
    {
      self->block_remove_widget++;
      gtk_cell_editable_remove_widget (cell_editable);
      self->block_remove_widget--;
    }
}

static void
gxk_proxy_editable_editing_done (GtkCellEditable *cell_editable)
{
  GxkProxyEditable *self = GXK_PROXY_EDITABLE (cell_editable);
  if (!self->block_editing_done && self->ecell)
    {
      self->block_editing_done++;
      gtk_cell_editable_editing_done (self->ecell);
      self->block_editing_done--;
    }
}

static void
gxk_proxy_editable_proxy_editing_done (GtkCellEditable *cell_editable)
{
  GxkProxyEditable *self = GXK_PROXY_EDITABLE (cell_editable);
  if (!self->block_editing_done)
    {
      self->block_editing_done++;
      gtk_cell_editable_editing_done (cell_editable);
      self->block_editing_done--;
    }
}

/**
 * gxk_proxy_editable_set_cell_editable
 * @self:  valid #GxkProxyEditable
 * @ecell: valid #GtkCellEditable
 *
 * Set the backend @ecell onto which #GtkCellEditable
 * methods issued on @self are to be forwarded.
 * This function may only be called once per #GxkProxyEditable.
 */
void
gxk_proxy_editable_set_cell_editable (GxkProxyEditable *self,
				      GtkCellEditable  *ecell)
{
  g_return_if_fail (GXK_IS_PROXY_EDITABLE (self));
  g_return_if_fail (GTK_IS_CELL_EDITABLE (ecell));
  g_return_if_fail (self->ecell == NULL);

  self->ecell = ecell;
  g_signal_connect_object (ecell, "remove_widget",
			   G_CALLBACK (gxk_proxy_editable_proxy_remove_widget), self,
			   G_CONNECT_SWAPPED);
  g_signal_connect_object (ecell, "editing_done",
			   G_CALLBACK (gxk_proxy_editable_proxy_editing_done), self,
			   G_CONNECT_SWAPPED);
}

static void
gxk_proxy_editable_init_cell_editable (GtkCellEditableIface *iface)
{
  iface->start_editing = gxk_proxy_editable_start_editing;
  iface->remove_widget = gxk_proxy_editable_remove_widget;
  iface->editing_done = gxk_proxy_editable_editing_done;
}

/**
 * gxk_proxy_editable_get_type
 * @RETURNS: %GXK_TYPE_PROXY_EDITABLE
 *
 * #GxkProxyEditable is a #GtkEventBox which proxies
 * the #GtkCellEditable interface onto a backend
 * widget which also implements the #GtkCellEditable
 * interface.
 */
GType
gxk_proxy_editable_get_type (void)
{
  static GType type = 0;
  
  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (GxkProxyEditableClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) NULL,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (GxkProxyEditable),
	0,      /* n_preallocs */
	(GInstanceInitFunc) NULL,
      };
      static const GInterfaceInfo iface_info = {
	(GInterfaceInitFunc) gxk_proxy_editable_init_cell_editable,
	NULL,	/* interface_finalize */
	NULL,	/* interface_data */
      };
      type = g_type_register_static (GTK_TYPE_EVENT_BOX,
				     "GxkProxyEditable",
				     &type_info, 0);
      g_type_add_interface_static (type,
				   GTK_TYPE_CELL_EDITABLE,
				   &iface_info);
    }
  return type;
}
