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
#include "gxkracktable.h"
#include "gxkrackeditor.h"
#include "gxkrackitem.h"

typedef enum
{
  EXPOSE_AREA,
  FOCUS_AREA,
  DRAW_ACTIVE,
  DRAW_INACTIVE,
} AreaAction;


#define CR_WINDOW_THICKNESS     (2)
#define CR_WINDOW_SPAN          (8)


/* --- prototypes --- */
static void             gxk_rack_table_class_init       (GxkRackTableClass      *klass);
static void             gxk_rack_table_init             (GxkRackTable           *self);
static void             gxk_rack_table_destroy          (GtkObject              *object);
static void             gxk_rack_table_finalize         (GObject                *object);
static void             gxk_rack_table_style_set        (GtkWidget              *widget,
                                                         GtkStyle               *previous_style);
static void             gxk_rack_table_size_request     (GtkWidget              *widget,
                                                         GtkRequisition         *requisition);
static void             gxk_rack_table_size_allocate    (GtkWidget              *widget,
                                                         GtkAllocation          *allocation);
static void             gxk_rack_table_unrealize        (GtkWidget              *widget);
static void             gxk_rack_table_map              (GtkWidget              *widget);
static void             gxk_rack_table_unmap            (GtkWidget              *widget);
static gint             gxk_rack_table_expose           (GtkWidget              *widget,
                                                         GdkEventExpose         *event);
static void             gxk_rack_table_add              (GtkContainer           *container,
                                                         GtkWidget              *child);
static void             gxk_rack_table_remove           (GtkContainer           *container,
                                                         GtkWidget              *child);
static GtkWidget*       gxk_rack_table_find_cell_child  (GxkRackTable           *self,
                                                         guint                   hcell,
                                                         guint                   vcell);
static void             gxk_rack_table_draw_area        (GxkRackTable           *self,
                                                         AreaAction              action,
                                                         guint                   hcell1,
                                                         guint                   vcell1,
                                                         guint                   hspan,
                                                         guint                   vspan);


/* --- static variables --- */
static gpointer parent_class = NULL;
static guint    signal_edit_mode_changed = 0;


/* --- functions --- */
GType
gxk_rack_table_get_type (void)
{
  static GType object_type = 0;
  if (!object_type)
    {
      static const GTypeInfo object_info = {
        sizeof (GxkRackTableClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gxk_rack_table_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (GxkRackTable),
        0,      /* n_preallocs */
        (GInstanceInitFunc) gxk_rack_table_init,
      };
      object_type = g_type_register_static (GTK_TYPE_TABLE,
                                            "GxkRackTable",
                                            &object_info, 0);
    }
  return object_type;
}

static void
gxk_rack_table_class_init (GxkRackTableClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->finalize = gxk_rack_table_finalize;
  
  object_class->destroy = gxk_rack_table_destroy;
  
  widget_class->style_set = gxk_rack_table_style_set;
  widget_class->size_request = gxk_rack_table_size_request;
  widget_class->size_allocate = gxk_rack_table_size_allocate;
  widget_class->unrealize = gxk_rack_table_unrealize;
  widget_class->map = gxk_rack_table_map;
  widget_class->unmap = gxk_rack_table_unmap;
  widget_class->button_press_event = (gpointer) gxk_rack_table_handle_button_press;
  widget_class->motion_notify_event = (gpointer) gxk_rack_table_handle_motion_notify;
  widget_class->enter_notify_event = (gpointer) gxk_rack_table_handle_enter_notify;
  widget_class->leave_notify_event = (gpointer) gxk_rack_table_handle_leave_notify;
  widget_class->button_release_event = (gpointer) gxk_rack_table_handle_button_release;
  widget_class->expose_event = gxk_rack_table_expose;
  
  container_class->add = gxk_rack_table_add;
  container_class->remove = gxk_rack_table_remove;
  
  signal_edit_mode_changed = g_signal_new ("edit-mode-changed",
                                           G_OBJECT_CLASS_TYPE (class),
                                           G_SIGNAL_RUN_FIRST,
                                           G_STRUCT_OFFSET (GxkRackTableClass, edit_mode_changed),
                                           NULL, NULL,
                                           gxk_marshal_NONE__BOOLEAN,
                                           G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
gxk_rack_table_init (GxkRackTable *self)
{
  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (self), TRUE);
  gtk_container_set_reallocate_redraws (GTK_CONTAINER (self), TRUE);
  g_object_set (self,
                "visible", TRUE,
                "homogeneous", TRUE,
                "column_spacing", 0,
                "row_spacing", 0,
                NULL);
  self->child_map = NULL;
  self->cell_request_width = 12;
  self->cell_request_height = 12;
  self->cell_width = 0;
  self->cell_height = 0;
  self->editor = NULL;
}

static void
gxk_rack_table_destroy (GtkObject *object)
{
  GxkRackTable *self = GXK_RACK_TABLE (object);
  
  if (self->editor)
    gxk_rack_table_destroy_editor (self);
  gxk_rack_table_uncover (self);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gxk_rack_table_finalize (GObject *object)
{
  GxkRackTable *self = GXK_RACK_TABLE (object);

  while (self->covers)
    g_object_unref (g_slist_pop_head (&self->covers));

  g_bit_matrix_free (self->child_map);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gxk_rack_table_style_set (GtkWidget *widget,
                          GtkStyle  *previous_style)
{
  GxkRackTable *self = GXK_RACK_TABLE (widget);
  GdkFont *font = gtk_style_get_font (widget->style);
  guint i, x = 0;
  
  for (i = 0; i < 256; i++)
    {
      guint width = gdk_char_width (font, i);
      
      x = MAX (x, width);
    }
  self->cell_request_width = x;
  self->cell_request_height = font->ascent + font->descent;
  self->cell_request_width = MAX (self->cell_request_width, 8);
  self->cell_request_height = MAX (self->cell_request_height, 8);
  if (1)
    {
      self->cell_request_width = self->cell_request_height = MAX (self->cell_request_width, self->cell_request_height);
    }
}

static void
gxk_rack_table_size_request (GtkWidget      *widget,
                             GtkRequisition *requisition)
{
  GxkRackTable *self = GXK_RACK_TABLE (widget);
  GtkTable *table = GTK_TABLE (self);
  GList *list;
  guint i, j;
  
  gxk_rack_table_invalidate_child_map (self);   /* we get here when re-attaching children */
  for (list = table->children; list; list = list->next)
    {
      GtkTableChild *child = list->data;
      
      if (GTK_WIDGET_VISIBLE (child->widget))
        gtk_widget_size_request (child->widget, NULL);
    }
  
  requisition->width = GTK_CONTAINER (self)->border_width * 2;
  requisition->height = GTK_CONTAINER (self)->border_width * 2;
  for (i = 0; i < table->ncols; i++)
    {
      table->cols[i].requisition = self->cell_request_width + table->cols[i].spacing;
      requisition->width += table->cols[i].requisition;
    }
  for (j = 0; j < table->nrows; j++)
    {
      table->rows[j].requisition = self->cell_request_height + table->rows[j].spacing;
      requisition->height += table->rows[j].requisition;
    }
}

static void
gxk_rack_table_size_allocate (GtkWidget     *widget,
                              GtkAllocation *allocation)
{
  GxkRackTable *self = GXK_RACK_TABLE (widget);
  GtkTable *table = GTK_TABLE (self);
  
  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
  
  gxk_rack_table_invalidate_child_map (self);
  
  self->cell_width = table->cols[0].allocation;
  self->cell_height = table->rows[0].allocation;

  if (self->editor)
    gxk_rack_table_adjust_editor (self);
}

static void
gxk_rack_table_unrealize (GtkWidget *widget)
{
  GxkRackTable *self = GXK_RACK_TABLE (widget);
  if (self->editor)
    gxk_rack_table_destroy_editor (self);
  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gxk_rack_table_map (GtkWidget *widget)
{
  GxkRackTable *self = GXK_RACK_TABLE (widget);
  GTK_WIDGET_CLASS (parent_class)->map (widget);
  if (self->editor)
    gxk_rack_table_adjust_editor (self);
}

static void
gxk_rack_table_unmap (GtkWidget *widget)
{
  GxkRackTable *self = GXK_RACK_TABLE (widget);
  if (self->editor)
    gxk_rack_table_unmap_editor (self);
  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gxk_rack_table_draw_area (GxkRackTable *self,
                          AreaAction    action,
                          guint         hcell1,
                          guint         vcell1,
                          guint         hspan,
                          guint         vspan)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkTable *table = GTK_TABLE (self);
  guint i, x, y, width, height, hcell2 = hcell1 + hspan, vcell2 = vcell1 + vspan;
  
  g_return_if_fail (hspan > 0 && hcell2 <= table->ncols);
  g_return_if_fail (vspan > 0 && vcell2 <= table->nrows);
  
  x = GTK_CONTAINER (widget)->border_width + widget->allocation.x;
  width = 0;
  for (i = 0; i < hcell2; i++)
    {
      guint bound = table->cols[i].allocation + table->cols[i].spacing;
      
      if (i < hcell1)
        x += bound;
      else
        width += bound;
    }
  y = GTK_CONTAINER (widget)->border_width + widget->allocation.y;
  height = 0;
  for (i = 0; i < vcell2; i++)
    {
      guint bound = table->rows[i].allocation + table->rows[i].spacing;
      
      if (i < vcell1)
        y += bound;
      else
        height += bound;
    }
  
  switch (action)
    {
      GdkGC *bg_gc, *dark_gc, *light_gc;
      guint r;
    case EXPOSE_AREA:
      gtk_widget_queue_draw_area (widget, x, y, width, height);
      break;
    case DRAW_INACTIVE:
    case DRAW_ACTIVE:
      if (action == DRAW_INACTIVE)
        {
          bg_gc = widget->style->bg_gc[GTK_STATE_ACTIVE];
          light_gc = widget->style->light_gc[widget->state];
          dark_gc = widget->style->dark_gc[widget->state];
        }
      else
        {
          bg_gc = widget->style->bg_gc[GTK_STATE_PRELIGHT];
          dark_gc = widget->style->light_gc[widget->state];
          light_gc = widget->style->dark_gc[widget->state];
        }
      r = MIN (width, height);
      if (width > height)
        x += (width - height) / 2;
      if (height > width)
        y += (height - width) / 2;
      gdk_draw_arc (widget->window,
                    bg_gc,
                    TRUE,
                    x + r / 4, y + r / 4, r / 2, r / 2,
                    0. * 64, 360. * 64);
      gdk_draw_arc (widget->window,
                    light_gc,
                    FALSE,
                    x + r / 4, y + r / 4, r / 2, r / 2,
                    225. * 64, 180. * 64);
      gdk_draw_arc (widget->window,
                    dark_gc,
                    FALSE,
                    x + r / 4, y + r / 4, r / 2, r / 2,
                    45. * 64, 180. * 64);
      break;
    case FOCUS_AREA:
      gdk_draw_rectangle (widget->window,
                          widget->style->black_gc,
                          FALSE,
                          x, y, width, height);
      break;
    }
}

void
gxk_rack_table_redraw_cells (GxkRackTable     *self,
                             guint             hcell1,
                             guint             vcell1,
                             guint             hspan,
                             guint             vspan)
{
  gxk_rack_table_draw_area (self, EXPOSE_AREA, hcell1, vcell1, hspan, vspan);
}

static gint
gxk_rack_table_expose (GtkWidget      *widget,
                       GdkEventExpose *event)
{
  GxkRackTable *self = GXK_RACK_TABLE (widget);
  GtkTable *table = GTK_TABLE (self);
  
  if (self->editor && event->window == widget->window)
    {
      guint x = GTK_CONTAINER (self)->border_width + GTK_WIDGET (self)->allocation.x, bx = 0;
      guint i, j;
      
      gxk_rack_table_update_child_map (self);
      for (i = 0; i < table->ncols; i++)
        {
          guint y = GTK_CONTAINER (self)->border_width + GTK_WIDGET (self)->allocation.y, by = 0;
          
          bx = table->cols[i].allocation + table->cols[i].spacing;
          if (x > event->area.x + event->area.width || x + bx < event->area.x)
            {
              x += bx;
              gxk_rack_table_draw_area (self, FOCUS_AREA, i, 0, 1, table->nrows);
              continue;
            }
          for (j = 0; j < table->nrows; j++)
            {
              by = table->rows[j].allocation + table->rows[j].spacing;
              if (y > event->area.y + event->area.height || y + by < event->area.y)
                {
                  y += by;
                  gxk_rack_table_draw_area (self, FOCUS_AREA, i, j, 1, 1);
                  continue;
                }
              if (g_bit_matrix_test (self->child_map, i, j))
                continue;
              if (self->editor->drag_child &&
                  i >= self->editor->drag_col &&
                  i < self->editor->drag_col + self->editor->drag_hspan &&
                  j >= self->editor->drag_row &&
                  j < self->editor->drag_row + self->editor->drag_vspan)
                gxk_rack_table_draw_area (self, DRAW_ACTIVE, i, j, 1, 1);
              else
                gxk_rack_table_draw_area (self, DRAW_INACTIVE, i, j, 1, 1);
              y += by;
            }
          x += bx;
        }
    }
  
  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
  
  return FALSE;
}

void
gxk_rack_table_attach (GxkRackTable *self,
                       GtkWidget    *child,
                       guint         col,
                       guint         row,
                       guint         hspan,
                       guint         vspan)
{
  GtkTable *table = GTK_TABLE (self);
  GxkRackItem *ritem = GXK_IS_RACK_ITEM (child) ? GXK_RACK_ITEM (child) : NULL;
  /* preset area from rack item config */
  if (ritem)
    gxk_rack_item_set_area (ritem, col, row, hspan, vspan);
  /* attach to parent */
  gtk_table_attach (table, child,
                    col, col + hspan,
                    row, row + vspan,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                    0, 0);
  gxk_rack_table_invalidate_child_map (self);
}

static void
gxk_rack_table_add (GtkContainer *container,
                    GtkWidget    *child)
{
  GxkRackTable *self = GXK_RACK_TABLE (container);
  GtkTable *table = GTK_TABLE (self);
  GxkRackItem *ritem = GXK_IS_RACK_ITEM (child) ? GXK_RACK_ITEM (child) : NULL;
  gint col = -1, row = -1, hspan = 0, vspan = 0;
  /* preset area from rack item config */
  if (ritem)
    {
      col = ritem->col;
      row = ritem->row;
      hspan = ritem->hspan;
      vspan = ritem->vspan;
    }
  /* find initial column/row */
  if (col < 0 || row < 0)
    {
      guint r, c;
      for (r = row < 0 ? 0 : row; r < (row < 0 ? table->nrows : row + 1); r++)
        for (c = col < 0 ? 0 : col; c < (col < 0 ? table->ncols : col + 1); c++)
          if (gxk_rack_table_check_area (self,
                                         GXK_IS_RACK_ITEM (child) &&
                                         GXK_RACK_ITEM (child)->empty_frame,
                                         c, r, MAX (hspan, 1), MAX (vspan, 1), NULL))
            goto break_break;
    break_break:
      row = CLAMP (row, 0, table->nrows - 1);
      col = CLAMP (col, 0, table->ncols - 1);
      if (ritem)
        gxk_rack_item_set_area (ritem, col, row, hspan, vspan);
    }
  /* attach to parent */
  gtk_table_attach (table, child,
                    col, col + MAX (hspan, 1),
                    row, row + MAX (vspan, 1),
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                    0, 0);
  gxk_rack_table_invalidate_child_map (self);
  /* auto-expand horizontally or vertically if not preset */
  if (hspan < 1 || vspan < 1)
    {
      GtkRequisition requisition;
      gtk_widget_size_request (child, &requisition);
      if (hspan < 1)
        {
          hspan = requisition.width / self->cell_request_width;
          hspan = MAX (hspan, 1);
          if (hspan > 1 && !ritem)
            gtk_container_child_set (GTK_CONTAINER (self), child, "right-attach", col + hspan, NULL);
        }
      if (vspan < 1)
        {
          vspan = requisition.height / self->cell_request_height;
          vspan = MAX (vspan, 1);
          if (vspan > 1 && !ritem)
            gtk_container_child_set (GTK_CONTAINER (self), child, "bottom-attach", row + vspan, NULL);
        }
      if (ritem)
        gxk_rack_item_set_area (ritem, col, row, hspan, vspan);
    }
  if (GXK_IS_RACK_ITEM (child))
    gxk_rack_item_gui_changed (GXK_RACK_ITEM (child));
}

static void
gxk_rack_table_remove (GtkContainer *container,
                       GtkWidget    *child)
{
  GxkRackTable *self = GXK_RACK_TABLE (container);
  
  gxk_rack_table_invalidate_child_map (self);
  
  /* chain parent class' handler */
  GTK_CONTAINER_CLASS (parent_class)->remove (container, child);
}

gboolean
gxk_rack_table_translate (GxkRackTable          *self,
                          gint                   x,
                          gint                   y,
                          guint                 *hcell,
                          guint                 *vcell)
{
  GtkTable *table = GTK_TABLE (self);
  guint i;

  /* translate widget relative coordinates */
  x -= GTK_CONTAINER (self)->border_width;
  *hcell = 0;
  for (i = 0; i < table->ncols; i++)
    {
      gint bound = table->cols[i].allocation + table->cols[i].spacing;
      if (x < bound)
        {
          *hcell = i;
          break;
        }
      x -= bound;
    }
  if (i >= table->ncols)
    *hcell = table->ncols;
  
  y -= GTK_CONTAINER (self)->border_width;
  *vcell = 0;
  for (i = 0; i < table->nrows; i++)
    {
      gint bound = table->rows[i].allocation + table->rows[i].spacing;
      if (y < bound)
        {
          *vcell = i;
          break;
        }
      y -= bound;
    }
  if (i >= table->nrows)
    *vcell = table->nrows;
  
  return x >= 0 && *hcell < table->ncols && y >= 0 && *vcell < table->nrows;
}

GtkWidget*
gxk_rack_table_find_child (GxkRackTable *self,
                           gint          x,
                           gint          y)
{
  GtkTable *table = GTK_TABLE (self);
  GList *list;
  gint h, v;
  if (gxk_rack_table_translate (self, x, y, &h, &v))
    {
      for (list = table->children; list; list = list->next)
        {
          GtkTableChild *child = list->data;
          if (h >= child->left_attach && h < child->right_attach &&
              v >= child->top_attach && v < child->bottom_attach)
            {
              if (GXK_IS_RACK_ITEM (child->widget) &&       /* exempt inner areas of frames */
                  GXK_RACK_ITEM (child->widget)->empty_frame &&
                  h >= child->left_attach + 1 &&
                  h + 1 < child->right_attach &&
                  v >= child->top_attach + 1 &&
                  v + 1 < child->bottom_attach)
                continue;
              return child->widget;
            }
        }
    }
  return NULL;
}

gboolean
gxk_rack_table_get_child_area (GxkRackTable *self,
                               GtkWidget    *child_widget,
                               guint        *col,
                               guint        *row,
                               guint        *hspan,
                               guint        *vspan)
{
  GtkTable *table = GTK_TABLE (self);
  GList *list;

  for (list = table->children; list; list = list->next)
    {
      GtkTableChild *child = list->data;
      if (child->widget == child_widget)
        {
          if (col)
            *col = child->left_attach;
          if (row)
            *row = child->top_attach;
          if (hspan)
            *hspan = child->right_attach - child->left_attach;
          if (vspan)
            *vspan = child->bottom_attach - child->top_attach;
          return TRUE;
        }
    }
  return FALSE;
}

static GtkWidget*
gxk_rack_table_find_cell_child (GxkRackTable *self,
                                guint         hcell,
                                guint         vcell)
{
  GtkTable *table = GTK_TABLE (self);
  GList *list;
  
  for (list = table->children; list; list = list->next)
    {
      GtkTableChild *child = list->data;
      
      if (hcell >= child->left_attach && hcell < child->right_attach &&
          vcell >= child->top_attach && vcell < child->bottom_attach)
        {
          if (GXK_IS_RACK_ITEM (child->widget) &&
              GXK_RACK_ITEM (child->widget)->empty_frame &&
              hcell > child->left_attach && hcell + 1 < child->right_attach &&
              vcell > child->top_attach && vcell + 1 < child->bottom_attach)
            continue;
          return child->widget;
        }
    }
  
  return NULL;
}

void
gxk_rack_table_invalidate_child_map (GxkRackTable *self)
{
  g_bit_matrix_free (self->child_map);
  self->child_map = NULL;
}

void
gxk_rack_table_update_child_map (GxkRackTable *self)
{
  if (!self->child_map)
    {
      GtkTable *table = GTK_TABLE (self);
      guint i, j;
      self->child_map = g_bit_matrix_new (table->ncols, table->nrows);
      for (j = 0; j < table->nrows; j++)
        for (i = 0; i < table->ncols; i++)
          g_bit_matrix_change (self->child_map, i, j,
                               gxk_rack_table_find_cell_child (self, i, j) != NULL);
    }
}

#include "gxkrackcovers.c"

void
gxk_rack_table_cover_up (GxkRackTable *self)
{
  g_return_if_fail (GXK_IS_RACK_TABLE (self));
  if (!self->covers)
    self->covers = rack_cover_add_plates (self);
}

void
gxk_rack_table_uncover (GxkRackTable *self)
{
  g_return_if_fail (GXK_IS_RACK_TABLE (self));
  while (self->covers)
    {
      GtkWidget *widget = g_slist_pop_head (&self->covers);
      gtk_widget_destroy (widget);
      g_object_unref (widget);
    }
}
