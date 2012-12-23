/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2001-2003 Tim Janik
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
#include "gxkrackeditor.hh"
#include "gxkrackitem.hh"


#define CR_WINDOW_THICKNESS(self)     (2)
#define CR_WINDOW_SPAN(self)          (MAX (self->cell_request_width, self->cell_request_height) * 3/4)


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
  self->editor->crb1 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->editor->crb1, self);
  gdk_window_set_background (self->editor->crb1, color);
  self->editor->crb2 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->editor->crb2, self);
  gdk_window_set_background (self->editor->crb2, color);
  self->editor->crb3 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->editor->crb3, self);
  gdk_window_set_background (self->editor->crb3, color);
  self->editor->crb4 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->editor->crb4, self);
  gdk_window_set_background (self->editor->crb4, color);
  attributes.width = CR_WINDOW_SPAN (self);
  attributes.height = CR_WINDOW_SPAN (self);
  attributes.event_mask = GDK_BUTTON_PRESS_MASK;
  self->editor->crq1 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->editor->crq1, self);
  gdk_window_set_background (self->editor->crq1, color);
  self->editor->crq2 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->editor->crq2, self);
  gdk_window_set_background (self->editor->crq2, color);
  self->editor->crq3 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->editor->crq3, self);
  gdk_window_set_background (self->editor->crq3, color);
  self->editor->crq4 = gdk_window_new (pwindow, &attributes, attributes_mask);
  gdk_window_set_user_data (self->editor->crq4, self);
  gdk_window_set_background (self->editor->crq4, color);
}

static void
rack_table_hide_rframe (GxkRackTable   *self)
{
  if (self->editor->rfx >= 0 || self->editor->rfy >= 0)
    {
      gdk_window_hide (self->editor->crq1);
      gdk_window_hide (self->editor->crq2);
      gdk_window_hide (self->editor->crq3);
      gdk_window_hide (self->editor->crq4);
      gdk_window_hide (self->editor->crb1);
      gdk_window_hide (self->editor->crb2);
      gdk_window_hide (self->editor->crb3);
      gdk_window_hide (self->editor->crb4);
      self->editor->rfx = self->editor->rfy = -1;
      self->editor->rfw = self->editor->rfh = 0;
    }
}

static GtkWidget*
rack_table_update_rframe (GxkRackTable   *self,
                          gint            cx,
                          gint            cy)
{
  GtkWidget *child = gxk_rack_table_find_child (self, cx, cy);
  gint x, y, width, height, bx, by, bw, bh;
  if (!GXK_IS_RACK_ITEM (child) || self->editor->drag_child)
    {
      rack_table_hide_rframe (self);
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
  gdk_window_move (self->editor->crq1, x, y);
  gdk_window_move (self->editor->crq2, x + width - CR_WINDOW_SPAN (self), y);
  gdk_window_move (self->editor->crq3, x, y + height - CR_WINDOW_SPAN (self));
  gdk_window_move (self->editor->crq4, x + width - CR_WINDOW_SPAN (self), y + height - CR_WINDOW_SPAN (self));
  bx = x + CR_WINDOW_SPAN (self);
  bw = width - 2 * CR_WINDOW_SPAN (self);
  by = y + height - CR_WINDOW_THICKNESS (self);
  gdk_window_move_resize (self->editor->crb1, bx, y, bw, CR_WINDOW_THICKNESS (self));
  gdk_window_move_resize (self->editor->crb3, bx, by, bw, CR_WINDOW_THICKNESS (self));
  by = y + CR_WINDOW_SPAN (self);
  bh = height - 2 * CR_WINDOW_SPAN (self);
  bx = x + width - CR_WINDOW_THICKNESS (self);
  gdk_window_move_resize (self->editor->crb2, x, by, CR_WINDOW_THICKNESS (self), bh);
  gdk_window_move_resize (self->editor->crb4, bx, by, CR_WINDOW_THICKNESS (self), bh);
  gdk_window_show_unraised (self->editor->crq1);
  gdk_window_show_unraised (self->editor->crq2);
  gdk_window_show_unraised (self->editor->crq3);
  gdk_window_show_unraised (self->editor->crq4);
  gdk_window_show_unraised (self->editor->crb1);
  gdk_window_show_unraised (self->editor->crb2);
  gdk_window_show_unraised (self->editor->crb3);
  gdk_window_show_unraised (self->editor->crb4);
  return child;
}

static void
rack_table_editor_unrealize (GxkRackTable   *self)
{
  gdk_window_destroy (self->editor->iwindow);
  gdk_window_destroy (self->editor->crq1);
  gdk_window_destroy (self->editor->crq2);
  gdk_window_destroy (self->editor->crq3);
  gdk_window_destroy (self->editor->crq4);
  gdk_window_destroy (self->editor->crb1);
  gdk_window_destroy (self->editor->crb2);
  gdk_window_destroy (self->editor->crb3);
  gdk_window_destroy (self->editor->crb4);
}

void
gxk_rack_table_adjust_editor (GxkRackTable *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  gint x, y;
  gdk_window_move_resize (self->editor->iwindow,
                          widget->allocation.x, widget->allocation.y,
                          widget->allocation.width, widget->allocation.height);
  gdk_window_raise (self->editor->crb1);
  gdk_window_raise (self->editor->crb2);
  gdk_window_raise (self->editor->crb3);
  gdk_window_raise (self->editor->crb4);
  gdk_window_raise (self->editor->crq1);
  gdk_window_raise (self->editor->crq2);
  gdk_window_raise (self->editor->crq3);
  gdk_window_raise (self->editor->crq4);
  gdk_window_raise (self->editor->iwindow);
  gdk_window_show (self->editor->iwindow);
  gdk_window_get_pointer (self->editor->iwindow, &x, &y, NULL);
  rack_table_update_rframe (self, x, y);
}

void
gxk_rack_table_unmap_editor (GxkRackTable *self)
{
  rack_table_abort_drag (self, GDK_CURRENT_TIME);
  gdk_window_hide (self->editor->iwindow);
  rack_table_hide_rframe (self);
}

gboolean
gxk_rack_table_handle_button_press (GxkRackTable   *self,
                                    GdkEventButton *event)
{
  GtkWidget *widget = GTK_WIDGET (self);

  if (!self->editor || event->window != self->editor->iwindow)
    return FALSE;

  if (!self->editor->drag_child && (event->button == 1 || event->button == 2))
    {
      gint x = widget->allocation.x + event->x;
      gint y = widget->allocation.y + event->y;
      self->editor->drag_child = rack_table_update_rframe (self, event->x, event->y);
      if (event->button == 1 && GXK_IS_RACK_ITEM (self->editor->drag_child))
        {
          /* guess corner */
          self->editor->drag_corner = 1;        /* top-left */
          if (y > self->editor->drag_child->allocation.y + self->editor->drag_child->allocation.height / 2)
            self->editor->drag_corner += 2;     /* "bottom" bit */
          if (x > self->editor->drag_child->allocation.x + self->editor->drag_child->allocation.width / 2)
            self->editor->drag_corner += 1;     /* "right" bit */
        }
      else
        self->editor->drag_corner = 0;
      if (GXK_IS_RACK_ITEM (self->editor->drag_child) &&
          (event->button == 2 ||
           (event->button == 1 && self->editor->drag_corner)))
        {
          GdkCursor *cursor;
          GdkCursorType cursort;
          switch (self->editor->drag_corner)
            {
            default: /* silence compiler */
            case 0: cursort = GDK_FLEUR; break;
            case 1: cursort = GDK_TOP_LEFT_CORNER; break;
            case 2: cursort = GDK_TOP_RIGHT_CORNER; break;
            case 3: cursort = GDK_BOTTOM_LEFT_CORNER; break;
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
          if (self->editor->drag_corner)
            self->editor->drag_x = self->editor->drag_y = 0;
          else
            {
              self->editor->drag_x = x - self->editor->drag_child->allocation.x;
              self->editor->drag_y = y - self->editor->drag_child->allocation.y;
            }
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
      return TRUE;
    }
  else if (!self->editor->drag_child && event->button == 3)
    {
      GtkWidget *child = gxk_rack_table_find_child (self, event->x, event->y);
      if (GXK_IS_RACK_ITEM (child))
        {
          /* proxy button presses */
          g_signal_emit_by_name (child, "button-press", event);
          return TRUE;
        }
    }
  return FALSE;
}

gboolean
gxk_rack_table_handle_enter_notify (GxkRackTable     *self,
                                    GdkEventCrossing *event)
{
  if (self->editor && self->editor->iwindow == event->window)
    rack_table_update_rframe (self, event->x, event->y);
  return TRUE;
}

static void
rack_table_editor_drag_corner (GxkRackTable   *self,
                               gint            h,
                               gint            v)
{
  GxkRackItem *ritem = GXK_RACK_ITEM (self->editor->drag_child);
  gboolean framed = ritem->empty_frame;
  gint x1 = self->editor->drag_col;
  gint y1 = self->editor->drag_row;
  gint x2 = x1 + self->editor->drag_hspan;
  gint y2 = y1 + self->editor->drag_vspan;
  /* jump-change area according to pointer */
  switch (self->editor->drag_corner)
    {
    case 0:     /* movement */
      if (!gxk_rack_table_check_area (self, framed,
                                      h, v, self->editor->drag_hspan, self->editor->drag_vspan,
                                      self->editor->drag_child))
        {
          x1 = h;
          y1 = v;
          x2 = x1 + self->editor->drag_hspan;
          y2 = y1 + self->editor->drag_vspan;
        }
      break;
    case 1:     /* top-left */
      if (h < x2 && v < y1 &&
          !gxk_rack_table_check_area (self, framed, h, v, x2 - h, y2 - v, self->editor->drag_child))
        {
          x1 = h;
          y1 = v;
        }
      break;
    case 2:     /* top-right */
      if (h > x1 && v < y2 &&
          !gxk_rack_table_check_area (self, framed, x1, v, h - x1, y2 - v, self->editor->drag_child))
        {
          x2 = h;
          y1 = v;
        }
      break;
    case 3:     /* bottom-left */
      if (h < x2 && v > y1 &&
          !gxk_rack_table_check_area (self, framed, h, y1, x2 - h, v - y1, self->editor->drag_child))
        {
          x1 = h;
          y2 = v;
        }
      break;
    case 4:     /* bottom-right */
      if (h > x1 && v > y1 &&
          !gxk_rack_table_check_area (self, framed, x1, y1, h - x1, v - y1, self->editor->drag_child))
        {
          x2 = h;
          y2 = v;
        }
      break;
    }
  /* "grow" area towards pointer */
  if (!self->editor->drag_corner)
    {   /* movement */
      while (h > x1 &&
             gxk_rack_table_check_area (self, framed,
                                        h, y1, self->editor->drag_hspan, self->editor->drag_vspan,
                                        self->editor->drag_child))
        h--;
      while (h < x1 &&
             gxk_rack_table_check_area (self, framed,
                                        h, y1, self->editor->drag_hspan, self->editor->drag_vspan,
                                        self->editor->drag_child))
        h++;
      x1 = h;
      x2 = x1 + self->editor->drag_hspan;
      while (v > y1 &&
             gxk_rack_table_check_area (self, framed,
                                        x1, v, self->editor->drag_hspan, self->editor->drag_vspan,
                                        self->editor->drag_child))
        v--;
      while (v < y1 &&
             gxk_rack_table_check_area (self, framed,
                                        x1, v, self->editor->drag_hspan, self->editor->drag_vspan,
                                        self->editor->drag_child))
        v++;
      y1 = v;
      y2 = y1 + self->editor->drag_vspan;
    }
  switch (self->editor->drag_corner)
    {   /* incremental x1 */
    case 1:     /* top-left */
    case 3:     /* bottom-left */
      if (h > x1)
        {
          h = MIN (h, x2 - 1);
          while (h > x1 &&
                 gxk_rack_table_check_area (self, framed, h, y1, x2 - h, y2 - y1, self->editor->drag_child))
            h--;
        }
      else
        while (h < x1 &&
               gxk_rack_table_check_area (self, framed, h, y1, x2 - h, y2 - y1, self->editor->drag_child))
          h++;
      x1 = h;
    }
  switch (self->editor->drag_corner)
    {   /* incremental x2 */
    case 2:     /* top-right */
    case 4:     /* bottom-right */
      if (h < x2)
        {
          h = MAX (x1 + 1, h);
          while (h < x2 &&
                 gxk_rack_table_check_area (self, framed, x1, y1, h - x1, y2 - y1, self->editor->drag_child))
            h++;
        }
      else
        while (h > x2 &&
               gxk_rack_table_check_area (self, framed, x1, y1, h - x1, y2 - y1, self->editor->drag_child))
          h--;
      x2 = h;
    }
  switch (self->editor->drag_corner)
    {   /* incremental y1 */
    case 1:     /* top-left */
    case 2:     /* top-right */
      if (v > y1)
        {
          v = MIN (v, y2 - 1);
          while (v > y1 &&
                 gxk_rack_table_check_area (self, framed, x1, v, x2 - x1, y2 - v, self->editor->drag_child))
            v--;
        }
      else
        while (v < y1 &&
               gxk_rack_table_check_area (self, framed, x1, v, x2 - x1, y2 - v, self->editor->drag_child))
          v++;
      y1 = v;
    }
  switch (self->editor->drag_corner)
    {   /* incremental y2 */
    case 3:     /* bottom-left */
    case 4:     /* bottom-right */
      if (v < y2)
        {
          v = MAX (y1 + 1, v);
          while (v < y2 &&
                 gxk_rack_table_check_area (self, framed, x1, y1, x2 - x1, v - y1, self->editor->drag_child))
            v++;
        }
      else
        while (v > y2 &&
               gxk_rack_table_check_area (self, framed, x1, y1, x2 - x1, v - y1, self->editor->drag_child))
          v--;
      y2 = v;
    }
  self->editor->drag_col = x1;
  self->editor->drag_row = y1;
  self->editor->drag_hspan = x2 - x1;
  self->editor->drag_vspan = y2 - y1;
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
  GtkWidget *widget = GTK_WIDGET (self);

  if (self->editor->drag_child)
    {
      guint h, v;
      gint x, y;
      /* translate x/y to center of first cell */
      x = event->x - self->editor->drag_x;
      y = event->y - self->editor->drag_y;
      x += self->cell_width / 2;
      y += self->cell_height / 2;
      gxk_rack_table_translate (self, x, y, &h, &v);
      rack_table_editor_drag_corner (self, h, v);
      if (event->is_hint)       /* trigger new events */
        gdk_window_get_pointer (widget->window, NULL, NULL, NULL);
    }
  else
    {
      rack_table_update_rframe (self, event->x, event->y);
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
      self->editor->drag_child = NULL;
      self->editor->drag_corner = 0;
      gxk_rack_table_redraw_cells (self,
                                   self->editor->drag_col,
                                   self->editor->drag_row,
                                   self->editor->drag_hspan,
                                   self->editor->drag_vspan);
    }
}

gboolean
gxk_rack_table_handle_button_release (GxkRackTable   *self,
                                      GdkEventButton *event)
{
  if ((event->button == 1 || event->button == 2) && self->editor->drag_child)
    {
      guint h, v;
      gint x, y;
      /* translate x/y to center of first cell */
      x = event->x - self->editor->drag_x;
      y = event->y - self->editor->drag_y;
      x += self->cell_width / 2;
      y += self->cell_height / 2;
      gxk_rack_table_translate (self, x, y, &h, &v);
      rack_table_editor_drag_corner (self, h, v);
      rack_table_abort_drag (self, event->time);
    }
  return TRUE;
}

void
gxk_rack_table_destroy_editor (GxkRackTable *self)
{
  g_return_if_fail (GXK_IS_RACK_TABLE (self));
  g_return_if_fail (self->editor);
  rack_table_abort_drag (self, GDK_CURRENT_TIME);
  rack_table_editor_unrealize (self);
  g_free (self->editor);
  self->editor = NULL;
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
    }
  else if (self->editor && !enable_editing)
    {
      gxk_rack_table_destroy_editor (self);
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
                           gboolean      framed,
                           guint         col,
                           guint         row,
                           guint         hspan,
                           guint         vspan,
                           GtkWidget    *exempt)
{
  guint ecol, erow, ehspan, evspan, i, j;
  gboolean empty_frame;

  g_return_val_if_fail (GXK_IS_RACK_TABLE (self), FALSE);

  gxk_rack_table_update_child_map (self);
  if (col + hspan > self->child_map->width ||
      row + vspan > self->child_map->height)
    return TRUE;
  if (!exempt || !gxk_rack_table_get_child_area (self, exempt, &ecol, &erow, &ehspan, &evspan))
    exempt = NULL;
  empty_frame = GXK_IS_RACK_ITEM (exempt) && GXK_RACK_ITEM (exempt)->empty_frame;
  for (i = col; i < col + hspan; i++)
    for (j = row; j < row + vspan; j++)
      {
        if (framed &&   /* discard inside-frame checks */
            i > col && i + 1 < col + hspan &&
            j > row && j + 1 < row + vspan)
          continue;
        if (g_bit_matrix_test (self->child_map, i, j))
          {
            if (exempt &&               /* check being inside exempt widget */
                i >= ecol && i < ecol + ehspan &&
                j >= erow && j < erow + evspan)
              {
                if (empty_frame &&      /* check being inside empty frame widget */
                    i > ecol && i + 1 < ecol + ehspan &&
                    j > erow && j + 1 < erow + evspan)
                  return TRUE;          /* non-exempt cell */
                continue;               /* exempt cell */
              }
            return TRUE;
          }
      }
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
