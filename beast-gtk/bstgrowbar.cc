/* BEAST - Better Audio System
 * Copyright (C) 2004 Tim Janik
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
#include <sfi/glib-extra.h>
#include "bstgrowbar.h"
#include "bstdefs.h"
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkhscrollbar.h>
#include <gtk/gtkvscrollbar.h>
#include <gxk/gxkutils.h>


/* --- prototypes --- */
static gboolean draw_grow_sign   (GtkWidget *widget, GdkEventExpose *event, BstGrowBar *self);
static gboolean draw_shrink_sign (GtkWidget *widget, GdkEventExpose *event, BstGrowBar *self);
static void     grow_range       (GtkWidget *widget, BstGrowBar *self);
static void     shrink_range     (GtkWidget *widget, BstGrowBar *self);

/* --- BstGrowBar --- */
G_DEFINE_DATA_TYPE (BstGrowBar, bst_grow_bar, GTK_TYPE_ALIGNMENT);

static void
bst_grow_bar_init (BstGrowBar      *self,
                   BstGrowBarClass *klass)
{
  g_object_set (self,
                "visible", 1,
                "xscale", 1.0,
                "yscale", 1.0,
                NULL);
  self->max_upper = G_MAXINT;
  GtkBox *box = (GtkBox*) g_object_new (klass->is_horizontal ? GTK_TYPE_HBOX : GTK_TYPE_VBOX,
                              "visible", 1,
                              "parent", self,
                              NULL);
  self->growb = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
                                           "visible", 1,
                                           "can-focus", 0,
                                           NULL);
  g_object_ref (self->growb);
  g_signal_connect (self->growb, "clicked", G_CALLBACK (grow_range), self);
  GtkWidget *sign = (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT,
                                  "visible", 1,
                                  "parent", self->growb,
                                  NULL);
  g_signal_connect_after (sign, "expose-event", G_CALLBACK (draw_grow_sign), self);
  self->shrinkb = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
                                             "visible", 1,
                                             "can-focus", 0,
                                             NULL);
  g_object_ref (self->shrinkb);
  g_signal_connect (self->shrinkb, "clicked", G_CALLBACK (shrink_range), self);
  sign = (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT,
                                    "visible", 1,
                                    "parent", self->shrinkb,
                                    NULL);
  g_signal_connect_after (sign, "expose-event", G_CALLBACK (draw_shrink_sign), self);
  gtk_box_pack_start (box, self->shrinkb, FALSE, TRUE, 0);
  self->range = (GtkRange*) g_object_new (klass->is_horizontal ? GTK_TYPE_HSCROLLBAR : GTK_TYPE_VSCROLLBAR,
                                          "visible", 1,
                                          "parent", box,
                                          NULL);
  g_object_ref (self->range);
  gtk_box_pack_start (box, self->growb, FALSE, TRUE, 0);
  if (klass->is_horizontal)
    {
      gxk_widget_request_vclient_width (self->growb, GTK_WIDGET (self->range));
      gxk_widget_request_vclient_width (self->shrinkb, GTK_WIDGET (self->range));
    }
  else
    {
      gxk_widget_request_hclient_height (self->growb, GTK_WIDGET (self->range));
      gxk_widget_request_hclient_height (self->shrinkb, GTK_WIDGET (self->range));
    }
  bst_grow_bar_set_tooltips (self,
                             _("Shrink the scrollable area"),
                             NULL,
                             _("Grow the scrollable area"));
}

static void
bst_grow_bar_finalize (GObject *object)
{
  BstGrowBar *self = BST_GROW_BAR (object);
  g_object_unref (self->shrinkb);
  g_object_unref (self->range);
  g_object_unref (self->growb);
  /* chain parent class handler */
  G_OBJECT_CLASS (bst_grow_bar_parent_class)->finalize (object);
}

static void
bst_grow_bar_class_init (BstGrowBarClass *klass,
                         gpointer         class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  // GtkBarClass *bar_class = GTK_BAR_CLASS (klass);

  gobject_class->finalize = bst_grow_bar_finalize;
}

static gboolean
draw_grow_sign (GtkWidget      *widget,
                GdkEventExpose *event,
                BstGrowBar     *self)
{
  gint l = MAX (1, widget->allocation.width / 5);
  gint p = MAX ((widget->allocation.width - l) / 2, 0);
  gdk_draw_rectangle (widget->window, widget->style->fg_gc[widget->state], TRUE,
                      widget->allocation.x + p, widget->allocation.y, l, widget->allocation.height);
  l = MAX (1, widget->allocation.height / 5);
  p = MAX ((widget->allocation.height - l) / 2, 0);
  gdk_draw_rectangle (widget->window, widget->style->black_gc, TRUE,
                      widget->allocation.x, widget->allocation.y + p, widget->allocation.width, l);
  return FALSE;
}

static gboolean
draw_shrink_sign (GtkWidget      *widget,
                  GdkEventExpose *event,
                  BstGrowBar     *self)
{
  gint l = MAX (1, widget->allocation.height / 5);
  gint p = MAX ((widget->allocation.height - l) / 2, 0);
  gdk_draw_rectangle (widget->window, widget->style->black_gc, TRUE,
                      widget->allocation.x, widget->allocation.y + p, widget->allocation.width, l);
  return FALSE;
}

static void
grow_range (GtkWidget  *widget,
            BstGrowBar *self)
{
  GtkAdjustment *adj = gtk_range_get_adjustment (self->range);
  double u = adj->upper * (4 / 3.);
  u = MIN (u, self->max_upper);
  if (u > adj->upper)
    {
      adj->upper = u;
      gtk_adjustment_changed (adj);
    }
}

static void
shrink_range (GtkWidget  *widget,
              BstGrowBar *self)
{
  GtkAdjustment *adj = gtk_range_get_adjustment (self->range);
  double u = adj->upper * (3 / 4.);
  u = MAX (u, adj->lower + adj->page_size);
  if (u < adj->upper)
    {
      adj->upper = u;
      gtk_adjustment_changed (adj);
      /* adj->upper may be > u here, so clamp *after* ::changed */
      double v = adj->value;
      adj->value = MIN (adj->value, adj->upper - adj->page_size);
      if (v != adj->value)
        gtk_adjustment_value_changed (adj);
    }
}

void
bst_grow_bar_set_adjustment (BstGrowBar     *self,
                             GtkAdjustment  *adj)
{
  gtk_range_set_adjustment (self->range, adj);
}

GtkAdjustment*
bst_grow_bar_get_adjustment (BstGrowBar *self)
{
  return gtk_range_get_adjustment (self->range);
}

void
bst_grow_bar_set_tooltips (BstGrowBar     *self,
                           const gchar    *shrink_tip,
                           const gchar    *scroll_tip,
                           const gchar    *grow_tip)
{
  gxk_widget_set_tooltip (self->shrinkb, shrink_tip);
  gxk_widget_set_tooltip (GTK_WIDGET (self->range), scroll_tip);
  gxk_widget_set_tooltip (self->growb, grow_tip);
}

/* --- BstHGrowBar --- */
G_DEFINE_TYPE (BstHGrowBar, bst_hgrow_bar, BST_TYPE_GROW_BAR);

static void
bst_hgrow_bar_init (BstHGrowBar *self)
{
}

static void
bst_hgrow_bar_class_init (BstHGrowBarClass *klass)
{
  BstGrowBarClass *grow_bar_class = BST_GROW_BAR_CLASS (klass);
  grow_bar_class->is_horizontal = TRUE;
}


/* --- BstVGrowBar --- */
G_DEFINE_TYPE (BstVGrowBar, bst_vgrow_bar, BST_TYPE_GROW_BAR);

static void
bst_vgrow_bar_init (BstVGrowBar *self)
{
}

static void
bst_vgrow_bar_class_init (BstVGrowBarClass *klass)
{
  BstGrowBarClass *grow_bar_class = BST_GROW_BAR_CLASS (klass);
  grow_bar_class->is_horizontal = FALSE;
}
