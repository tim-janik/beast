/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2001-2003 Tim Janik
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
#include "gxkrackeditor.h"
#include "gxkrackitem.h"


#define CR_WINDOW_THICKNESS(self)     (2)
#define CR_WINDOW_SPAN(self)          (MAX (self->cell_request_width, self->cell_request_height))


/* --- prototypes --- */
static void rack_table_abort_drag    (GxkRackTable *self,
                                      guint32       etime);


/* --- functions --- */
static void
rack_table_realize_rframe (GxkRackTable *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GdkWindow *pwindow = gtk_widget_get_parent_window (GTK_WIDGET (self));
  GdkColor *color = &GTK_WIDGET (self)->style->black;
  GdkWindowAttr attributes = { 0, };
  gint attributes_mask;

  attributes_mask = GDK_WA_X | GDK_WA_Y;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK |
                          GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                          GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK;
  self->editor->iwindow = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->editor->iwindow, self);

  attributes_mask = 0;
  attributes.width = CR_WINDOW_THICKNESS (self);
  attributes.height = CR_WINDOW_THICKNESS (self);
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = GDK_EXPOSURE_MASK;
  self->crb1 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->crb1, self);
  gdk_window_set_background (self->crb1, color);
  self->crb2 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->crb2, self);
  gdk_window_set_background (self->crb2, color);
  self->crb3 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->crb3, self);
  gdk_window_set_background (self->crb3, color);
  self->crb4 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->crb4, self);
  gdk_window_set_background (self->crb4, color);
  attributes.width = CR_WINDOW_SPAN (self);
  attributes.height = CR_WINDOW_SPAN (self);
  attributes.event_mask = GDK_BUTTON_PRESS_MASK;
  self->crq1 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->crq1, self);
  gdk_window_set_background (self->crq1, color);
  self->crq2 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->crq2, self);
  gdk_window_set_background (self->crq2, color);
  self->crq3 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->crq3, self);
  gdk_window_set_background (self->crq3, color);
  self->crq4 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->crq4, self);
  gdk_window_set_background (self->crq4, color);
}

static void
rack_table_hide_rframe (GxkRackTable   *self)
{
  if (self->editor->rfx >= 0 || self->editor->rfy >= 0)
    {
      gdk_window_hide (self->crq1);
      gdk_window_hide (self->crq2);
      gdk_window_hide (self->crq3);
      gdk_window_hide (self->crq4);
      gdk_window_hide (self->crb1);
      gdk_window_hide (self->crb2);
      gdk_window_hide (self->crb3);
      gdk_window_hide (self->crb4);
      self->editor->rfx = self->editor->rfy = -1;
      self->editor->rfw = self->editor->rfh = 0;
    }
}

static inline void
adjust_corner (GxkRackTable *self,
               GdkWindow    *window,
               gint          corner,
               gint          cx,
               gint          cy,
               gint         *cornerp,
               gint          x,
               gint          y)
{
  gdk_window_move (window, x, y);
  if (cornerp && cx >= x && cy >= y &&
      cx < x + CR_WINDOW_SPAN (self) &&
      cy < y + CR_WINDOW_SPAN (self))
    *cornerp = corner;
}

static GtkWidget*
rack_table_update_rframe (GxkRackTable   *self,
                          gint           *cornerp,
                          gint            cx,
                          gint            cy)
{
  GtkWidget *child = gxk_rack_table_find_child (self, cx, cy);
  gint x, y, width, height, bx, by, bw, bh, corner = 0;
  if (!GXK_IS_RACK_ITEM (child) || self->editor->drag_child)
    {
      rack_table_hide_rframe (self);
      if (cornerp)
        *cornerp = 0;
      return NULL;
    }
  x = child->allocation.x;
  y = child->allocation.y;
  width = child->allocation.width;
  height = child->allocation.height;
  self->editor->rfx = x;
  self->editor->rfy = y;
  self->editor->rfw = width;
  self->editor->rfh = height;
  adjust_corner (self, self->crq1, 1, cx, cy, &corner, x, y);
  adjust_corner (self, self->crq2, 2, cx, cy, &corner, x + width - CR_WINDOW_SPAN (self), y);
  adjust_corner (self, self->crq3, 3, cx, cy, &corner, x, y + height - CR_WINDOW_SPAN (self));
  adjust_corner (self, self->crq4, 4, cx, cy, &corner, x + width - CR_WINDOW_SPAN (self), y + height - CR_WINDOW_SPAN (self));
  bx = x + CR_WINDOW_SPAN (self);
  bw = width - 2 * CR_WINDOW_SPAN (self);
  by = y + height - CR_WINDOW_THICKNESS (self);
  gdk_window_move_resize (self->crb1, bx, y, bw, CR_WINDOW_THICKNESS (self));
  gdk_window_move_resize (self->crb3, bx, by, bw, CR_WINDOW_THICKNESS (self));
  by = y + CR_WINDOW_SPAN (self);
  bh = height - 2 * CR_WINDOW_SPAN (self);
  bx = x + width - CR_WINDOW_THICKNESS (self);
  gdk_window_move_resize (self->crb2, x, by, CR_WINDOW_THICKNESS (self), bh);
  gdk_window_move_resize (self->crb4, bx, by, CR_WINDOW_THICKNESS (self), bh);
  gdk_window_show_unraised (self->crq1);
  gdk_window_show_unraised (self->crq2);
  gdk_window_show_unraised (self->crq3);
  gdk_window_show_unraised (self->crq4);
  gdk_window_show_unraised (self->crb1);
  gdk_window_show_unraised (self->crb2);
  gdk_window_show_unraised (self->crb3);
  gdk_window_show_unraised (self->crb4);
  if (cornerp)
    *cornerp = corner;
  return child;
}

static void
rack_table_editor_unrealize (GxkRackTable   *self)
{
  gdk_window_destroy (self->editor->iwindow);
  gdk_window_destroy (self->crq1);
  gdk_window_destroy (self->crq2);
  gdk_window_destroy (self->crq3);
  gdk_window_destroy (self->crq4);
  gdk_window_destroy (self->crb1);
  gdk_window_destroy (self->crb2);
  gdk_window_destroy (self->crb3);
  gdk_window_destroy (self->crb4);
}

void
gxk_rack_table_adjust_editor (GxkRackTable *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  gint x, y;
  gdk_window_move_resize (self->editor->iwindow,
                          widget->allocation.x, widget->allocation.y,
                          widget->allocation.width, widget->allocation.height);
  gdk_window_raise (self->crb1);
  gdk_window_raise (self->crb2);
  gdk_window_raise (self->crb3);
  gdk_window_raise (self->crb4);
  gdk_window_raise (self->crq1);
  gdk_window_raise (self->crq2);
  gdk_window_raise (self->crq3);
  gdk_window_raise (self->crq4);
  gdk_window_raise (self->editor->iwindow);
  gdk_window_show (self->editor->iwindow);
  gdk_window_get_pointer (self->editor->iwindow, &x, &y, NULL);
  rack_table_update_rframe (self, NULL, x, y);
}

void
gxk_rack_table_unmap_editor (GxkRackTable *self)
{
  rack_table_abort_drag (self, GDK_CURRENT_TIME);
  gdk_window_hide (self->editor->iwindow);
  rack_table_hide_rframe (self);
}

static void
widget_reparent (GtkWidget *child,
                 GtkWidget *parent)
{
  g_object_ref (child);
  gtk_container_remove (GTK_CONTAINER (child->parent), child);
  gtk_container_add (GTK_CONTAINER (parent), child);
  g_object_unref (child);
}

gboolean
gxk_rack_table_handle_button_press (GxkRackTable   *self,
                                    GdkEventButton *event)
{
  GtkTable *table = GTK_TABLE (self);
  GtkWidget *widget = GTK_WIDGET (self);

  if (!self->editor || event->window != self->editor->iwindow)
    return FALSE;

  if (!self->editor->drag_child && event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
      gint x = widget->allocation.x + event->x;
      gint y = widget->allocation.y + event->y;
      self->editor->drag_child = rack_table_update_rframe (self, &self->editor->drag_corner, x, y);
      if (GXK_IS_RACK_ITEM (self->editor->drag_child) && self->editor->drag_corner)
        {
          GdkCursor *cursor;
          gint cursort;
          switch (self->editor->drag_corner)
            {
            case 1: cursort = GDK_TOP_LEFT_CORNER; break;
            case 2: cursort = GDK_TOP_RIGHT_CORNER; break;
            case 3: cursort = GDK_BOTTOM_LEFT_CORNER; break;
            default: /* silence compiler */
            case 4: cursort = GDK_BOTTOM_RIGHT_CORNER; break;
            }
          cursor = gdk_cursor_new (cursort);
          self->editor->grabbing = gxk_grab_pointer_and_keyboard (self->editor->iwindow, FALSE,
                                                                  GDK_BUTTON_RELEASE_MASK |
                                                                  GDK_POINTER_MOTION_MASK |
                                                                  GDK_POINTER_MOTION_HINT_MASK,
                                                                  NULL, cursor, event->time);
          gdk_cursor_destroy (cursor);
        }
      if (!self->editor->grabbing)
        self->editor->drag_child = NULL;
      else
        {
          GxkRackItem *ritem = GXK_RACK_ITEM (self->editor->drag_child);
          self->editor->drag_col = ritem->col;
          self->editor->drag_row = ritem->row;
          self->editor->drag_hspan = ritem->hspan;
          self->editor->drag_vspan = ritem->vspan;
          gxk_rack_table_redraw_cells (self,
                                       self->editor->drag_col,
                                       self->editor->drag_row,
                                       self->editor->drag_hspan,
                                       self->editor->drag_vspan);
        }
    }
  else if (!self->editor->drag_child && event->type == GDK_BUTTON_PRESS && event->button == 2)
    {
      gint h, v;
      self->editor->drag_corner = 0;
      self->editor->drag_child = rack_table_update_rframe (self, NULL,
                                                           widget->allocation.x + event->x,
                                                           widget->allocation.y + event->y);
      self->editor->grabbing = FALSE;
      if (GXK_IS_RACK_ITEM (self->editor->drag_child))
        {
          GdkCursor *cursor = gdk_cursor_new (GDK_FLEUR);
          gtk_widget_realize (self->editor->drag_window);
          rack_table_hide_rframe (self);
          self->editor->grabbing = gxk_grab_pointer_and_keyboard (self->editor->iwindow, FALSE,
                                                                  GDK_BUTTON_RELEASE_MASK |
                                                                  GDK_POINTER_MOTION_MASK |
                                                                  GDK_POINTER_MOTION_HINT_MASK,
                                                                  NULL, cursor, event->time);
          gdk_cursor_destroy (cursor);
        }
      if (!self->editor->grabbing)
        self->editor->drag_child = NULL;
      else
        {
          GxkRackItem *ritem = GXK_RACK_ITEM (self->editor->drag_child);
          GtkWidget *alignment = GTK_BIN (self->editor->drag_window)->child;
          GtkRequisition requisition;
          self->editor->drag_col = ritem->col;
          self->editor->drag_row = ritem->row;
          self->editor->drag_hspan = ritem->hspan;
          self->editor->drag_vspan = ritem->vspan;
          self->xofs = event->x - GTK_CONTAINER (self)->border_width;
          self->yofs = event->y - GTK_CONTAINER (self)->border_width;
          for (h = 0; h < self->editor->drag_col; h++)
            self->xofs -= table->cols[h].allocation + table->cols[h].spacing;
          for (h = 0; h < self->editor->drag_row; h++)
            self->yofs -= table->rows[h].allocation + table->rows[h].spacing;
          gtk_widget_set_size_request (alignment, self->editor->drag_child->allocation.width,
                                       self->editor->drag_child->allocation.height);
          widget_reparent (self->editor->drag_child, alignment);
          self->drag_col = self->editor->drag_col;
          self->drag_row = self->editor->drag_row;
          gtk_widget_size_request (self->editor->drag_window, &requisition);
          h = event->x_root - self->xofs;
          v = event->y_root - self->yofs;
          gdk_window_move (self->editor->drag_window->window, h, v);
          gtk_window_move (GTK_WINDOW (self->editor->drag_window), h, v);
          gtk_window_resize (GTK_WINDOW (self->editor->drag_window), requisition.width, requisition.height);
          gtk_widget_show (self->editor->drag_window);
          gxk_rack_table_redraw_cells (self,
                                       self->editor->drag_col,
                                       self->editor->drag_row,
                                       self->editor->drag_hspan,
                                       self->editor->drag_vspan);
        }
    }
  else if (!self->editor->drag_child && event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
      GtkWidget *child = gxk_rack_table_find_child (self, widget->allocation.x + event->x, widget->allocation.y + event->y);
      if (GXK_IS_RACK_ITEM (child))
        {
          /* proxy button presses */
          g_signal_emit_by_name (child, "button-press", event);
        }
    }
  
  return TRUE;
}

gboolean
gxk_rack_table_handle_enter_notify (GxkRackTable     *self,
                                    GdkEventCrossing *event)
{
  GtkWidget *widget = GTK_WIDGET (self);
  if (self->editor && self->editor->iwindow == event->window)
    rack_table_update_rframe (self, NULL, widget->allocation.x + event->x, widget->allocation.y + event->y);
  return TRUE;
}

static void
rack_table_editor_drag_corner (GxkRackTable   *self,
                               gint            h,
                               gint            v)
{
  GxkRackItem *ritem = GXK_RACK_ITEM (self->editor->drag_child);
  switch (self->editor->drag_corner)
    {
    case 1: /* top-left */
      self->editor->drag_col = h;
      self->editor->drag_row = v;
      break;
    case 2: /* top-right */
      self->editor->drag_hspan = h - self->editor->drag_col;
      self->editor->drag_row = v;
      break;
    case 3: /* bottom-left */
      self->editor->drag_col = h;
      self->editor->drag_vspan = v - self->editor->drag_row;
      break;
    case 4: /* bottom-right */
      self->editor->drag_hspan = h - self->editor->drag_col;
      self->editor->drag_vspan = v - self->editor->drag_row;
      break;
    }
  if (gxk_rack_item_set_area (ritem,
                              self->editor->drag_col,
                              self->editor->drag_row,
                              self->editor->drag_hspan,
                              self->editor->drag_vspan))
    gxk_rack_table_redraw_cells (self,
                                 self->editor->drag_col,
                                 self->editor->drag_row,
                                 self->editor->drag_hspan,
                                 self->editor->drag_vspan);
}

gboolean
gxk_rack_table_handle_motion_notify (GxkRackTable   *self,
                                     GdkEventMotion *event)
{
  GtkTable *table = GTK_TABLE (self);
  GtkWidget *widget = GTK_WIDGET (self);

  if (self->editor->drag_child && self->editor->drag_corner)
    {
      guint h, v;
      gint x, y;
      /* translate x/y to center of first cell */
      x = event->x + self->cell_width / 2;
      y = event->y + self->cell_height / 2;
      if (gxk_rack_table_iwindow_translate (self, x, y, &h, &v))
        rack_table_editor_drag_corner (self, h, v);
      if (event->is_hint)       /* trigger new events */
        gdk_window_get_pointer (widget->window, NULL, NULL, NULL);
    }
  else if (self->editor->drag_child)
    {
      guint h, v;
      gint x, y;
      
      gtk_window_move (GTK_WINDOW (self->editor->drag_window), event->x_root - self->xofs, event->y_root - self->yofs);
      
      if (event->is_hint)       /* trigger new events */
        gdk_window_get_pointer (widget->window, NULL, NULL, NULL);
      
      /* translate x/y to center of first cell */
      x = event->x - self->xofs;
      y = event->y - self->yofs;
      x += self->cell_width / 2;
      y += self->cell_height / 2;

      /* reset tentative position if drag position is covered/exceeds boundaries */
      if (!gxk_rack_table_iwindow_translate (self, x, y, &h, &v) ||
          h >= table->ncols || v >= table->nrows ||
          h + self->editor->drag_hspan > table->ncols ||
          v + self->editor->drag_vspan > table->nrows ||
          gxk_rack_table_check_area (self, h, v, self->editor->drag_hspan, self->editor->drag_vspan))
        {
          h = self->drag_col;
          v = self->drag_row;
        }

      /* update tentative position */
      if (h != self->editor->drag_col || v != self->editor->drag_row)
        {
          gxk_rack_table_redraw_cells (self,
                                       self->editor->drag_col,
                                       self->editor->drag_row,
                                       self->editor->drag_hspan,
                                       self->editor->drag_vspan);
          self->editor->drag_col = h;
          self->editor->drag_row = v;
          gxk_rack_table_redraw_cells (self,
                                       self->editor->drag_col,
                                       self->editor->drag_row,
                                       self->editor->drag_hspan,
                                       self->editor->drag_vspan);
        }
    }
  else
    {
      rack_table_update_rframe (self, NULL, widget->allocation.x + event->x, widget->allocation.y + event->y);
      if (event->is_hint)       /* trigger new events */
        gdk_window_get_pointer (widget->window, NULL, NULL, NULL);
    }

  return TRUE;
}

gboolean
gxk_rack_table_handle_leave_notify (GxkRackTable     *self,
                                    GdkEventCrossing *event)
{
  if (self->editor && self->editor->iwindow == event->window)
    rack_table_hide_rframe (self);
  return TRUE;
}

static void
rack_table_abort_drag (GxkRackTable *self,
                       guint32       etime)
{
  if (self->editor->drag_child)
    {
      if (self->editor->grabbing)
        {
          gxk_ungrab_pointer_and_keyboard (self->editor->iwindow, etime);
          self->editor->grabbing = FALSE;
        }
      gtk_widget_hide (self->editor->drag_window);
      if (!self->editor->drag_corner)
        {
          gxk_rack_item_set_area (GXK_RACK_ITEM (self->editor->drag_child),
                                  self->editor->drag_col,
                                  self->editor->drag_row,
                                  self->editor->drag_hspan,
                                  self->editor->drag_vspan);
          widget_reparent (self->editor->drag_child, GTK_WIDGET (self));
        }
      self->editor->drag_child = NULL;
      self->editor->drag_corner = 0;
    }
}

gboolean
gxk_rack_table_handle_button_release (GxkRackTable   *self,
                                      GdkEventButton *event)
{
  if (event->button == 1 && self->editor->drag_child && self->editor->drag_corner)
    {
      guint h, v;
      gint x, y;
      /* translate x/y to center of first cell */
      x = event->x + self->cell_width / 2;
      y = event->y + self->cell_height / 2;
      if (gxk_rack_table_iwindow_translate (self, x, y, &h, &v))
        rack_table_editor_drag_corner (self, h, v);
      rack_table_abort_drag (self, event->time);
    }
  else if (event->button  == 2 && self->editor->drag_child)
    rack_table_abort_drag (self, event->time);
  return TRUE;
}

void
gxk_rack_table_set_edit_mode (GxkRackTable *self,
                              gboolean      enable_editing)
{
  g_return_if_fail (GXK_IS_RACK_TABLE (self));

  enable_editing = enable_editing && GTK_WIDGET_DRAWABLE (self);
  if (!self->editor && enable_editing)
    {
      self->editor = g_new0 (GxkRackEditor, 1);
      self->editor->rfx = self->editor->rfy = -1;
      rack_table_realize_rframe (self);
      gxk_rack_table_adjust_editor (self);
      self->editor->drag_window = g_object_new (GTK_TYPE_WINDOW, "type", GTK_WINDOW_POPUP,
                                                "child", g_object_new (GTK_TYPE_ALIGNMENT, "visible", 1, NULL),
                                                NULL);
    }
  else if (self->editor && !enable_editing)
    {
      rack_table_abort_drag (self, GDK_CURRENT_TIME);
      rack_table_editor_unrealize (self);
      gtk_widget_destroy (self->editor->drag_window);
      g_free (self->editor);
      self->editor = NULL;
    }
  else
    return;
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_signal_emit_by_name (self, "edit-mode-changed", self->editor != NULL);
}

gboolean
gxk_rack_table_check_cell (GxkRackTable *self,
                           guint         col,
                           guint         row)
{
  g_return_val_if_fail (GXK_IS_RACK_TABLE (self), FALSE);

  gxk_rack_table_update_child_map (self);
  return g_bit_matrix_test (self->child_map, col, row);
}

gboolean
gxk_rack_table_check_area (GxkRackTable *self,
                           guint         col,
                           guint         row,
                           guint         hspan,
                           guint         vspan)
{
  guint i, j;
  
  g_return_val_if_fail (GXK_IS_RACK_TABLE (self), FALSE);

  gxk_rack_table_update_child_map (self);
  for (i = 0; i < hspan; i++)
    for (j = 0; j < vspan; j++)
      if (g_bit_matrix_test (self->child_map, col + i, row + j))
        return TRUE;
  return FALSE;
}

gboolean
gxk_rack_table_expand_rect (GxkRackTable *self,
                            guint         col,
                            guint         row,
                            guint        *hspan,
                            guint        *vspan)
{
  GtkTable *table;
  guint i, j, f;
  
  g_return_val_if_fail (GXK_IS_RACK_TABLE (self), FALSE);
  
  gxk_rack_table_update_child_map (self);
  table = GTK_TABLE (self);
  if (col >= table->ncols || row >= table->nrows ||
      g_bit_matrix_peek (self->child_map, col, row))
    return FALSE;

  /* we start two expansion passes, first horizontal, then
   * vertical. the greater resulting area is returned.
   */

  /* expand horizontally */
  for (i = 1; col + i < table->ncols; i++)
    if (g_bit_matrix_peek (self->child_map, col + i, row))
      break;
  for (j = 1; row + j < table->nrows; j++)
    for (f = 0; f < i; f++)
      if (g_bit_matrix_peek (self->child_map, col + f, row + j))
        goto last_row_break;
 last_row_break:
  *hspan = i;
  *vspan = j;
  
  /* expand vertically */
  for (j = j; row + j < table->nrows; j++)
    if (g_bit_matrix_peek (self->child_map, col, row + j))
      break;
  if (j == *vspan)
    return TRUE;
  for (i = 1; col + i < table->ncols; i++)
    for (f = 0; f < j; f++)
      if (g_bit_matrix_peek (self->child_map, col + i, row + f))
        goto last_col_break;
 last_col_break:
  if (i * j >= *hspan * *vspan)
    {
      *hspan = i;
      *vspan = j;
    }
  return TRUE;
}
