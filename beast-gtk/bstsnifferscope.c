/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstsnifferscope.h"


/* --- functions --- */
G_DEFINE_TYPE (BstSnifferScope, bst_sniffer_scope, GTK_TYPE_WIDGET);

static void
bst_sniffer_scope_init (BstSnifferScope *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  GTK_WIDGET_SET_FLAGS (self, GTK_NO_WINDOW);
  gtk_widget_set_double_buffered (widget, FALSE);
  gtk_widget_show (widget);
}

GtkWidget*
bst_sniffer_scope_new (void)
{
  BstSnifferScope *self = g_object_new (BST_TYPE_SNIFFER_SCOPE, NULL);
  return GTK_WIDGET (self);
}

static void
bst_sniffer_scope_destroy (GtkObject *object)
{
  BstSnifferScope *self = BST_SNIFFER_SCOPE (object);

  bst_sniffer_scope_set_sniffer (self, 0);

  GTK_OBJECT_CLASS (bst_sniffer_scope_parent_class)->destroy (object);
}

static void
bst_sniffer_scope_finalize (GObject *object)
{
  BstSnifferScope *self = BST_SNIFFER_SCOPE (object);

  bst_sniffer_scope_set_sniffer (self, 0);

  G_OBJECT_CLASS (bst_sniffer_scope_parent_class)->finalize (object);
}

static void
bst_sniffer_scope_size_request (GtkWidget      *widget,
                                GtkRequisition *requisition)
{
  // BstSnifferScope *self = BST_SNIFFER_SCOPE (widget);

  requisition->width = 20;
  requisition->height = 20;
}

static void
bst_sniffer_scope_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  // BstSnifferScope *self = BST_SNIFFER_SCOPE (widget);
  GTK_WIDGET_CLASS (bst_sniffer_scope_parent_class)->size_allocate (widget, allocation);
}

static gboolean
bst_sniffer_scope_expose (GtkWidget      *widget,
                          GdkEventExpose *event)
{
  // BstSnifferScope *self = BST_SNIFFER_SCOPE (widget);
  GdkWindow *window = event->window;
  GdkRectangle area = event->area;
  // GdkGC *dark_gc = widget->style->dark_gc[widget->state];
  // GtkAllocation *allocation = &widget->allocation;
  if (window != widget->window)
    return FALSE;

  /* with gtk_widget_set_double_buffered (self, FALSE) in init and
   * with gdk_window_begin_paint_region()/gdk_window_end_paint()
   * around our redraw functions, we can decide on our own on what
   * windows we want double buffering.
   */
  // gdk_window_clear_area (widget->window, area.x, area.y, area.width, area.height);
  gdk_window_begin_paint_rect (event->window, &area);
  // bst_sniffer_scope_draw_window (self, area.x, area.y, area.x + area.width, area.y + area.height);
  gdk_window_end_paint (event->window);
  
  return FALSE;
}

static void
bst_sniffer_scope_class_init (BstSnifferScopeClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  gobject_class->finalize = bst_sniffer_scope_finalize;

  object_class->destroy = bst_sniffer_scope_destroy;

  widget_class->size_request = bst_sniffer_scope_size_request;
  widget_class->size_allocate = bst_sniffer_scope_size_allocate;
  widget_class->expose_event = bst_sniffer_scope_expose;
}

static void
scope_probes_notify (SfiProxy     proxy,
                     SfiSeq      *sseq,
                     gpointer     data)
{
  BstSnifferScope *self = BST_SNIFFER_SCOPE (data);
  if (GTK_WIDGET_DRAWABLE (self))
    {
      BseProbeSeq *pseq = bse_probe_seq_from_seq (sseq);
      GtkWidget *widget = GTK_WIDGET (self);
      BseProbe *lprobe = NULL, *rprobe = NULL;
      guint i;
      for (i = 0; i < pseq->n_probes && (!lprobe || !rprobe); i++)
        if (pseq->probes[i]->channel_id == 0)
          lprobe = pseq->probes[i];
        else if (pseq->probes[i]->channel_id == 1)
          rprobe = pseq->probes[i];
      if (lprobe && lprobe->sample_data && lprobe->sample_data->n_values >= widget->allocation.width / 2 &&
          rprobe && rprobe->sample_data && rprobe->sample_data->n_values >= widget->allocation.width / 2)
        {
          GdkWindow *window = widget->window;
          GtkAllocation *allocation = &widget->allocation;
          GdkGC *fg_gc = widget->style->dark_gc[widget->state];
          GdkGC *bg_gc = widget->style->white_gc;
          guint i;
          for (i = 0; i < widget->allocation.width / 2; i++)
            {
              gfloat lv = CLAMP (lprobe->sample_data->values[i], -1, +1) * 0.5 + 0.5;
              gfloat rv = CLAMP (rprobe->sample_data->values[i], -1, +1) * 0.5 + 0.5;
              gdk_draw_line (window, bg_gc,
                             allocation->x + i,
                             allocation->y,
                             allocation->x + i,
                             allocation->y + allocation->height * (1.0 - lv));
              gdk_draw_line (window, fg_gc,
                             allocation->x + i,
                             allocation->y + allocation->height * (1.0 - lv),
                             allocation->x + i,
                             allocation->y + allocation->height);
              gdk_draw_line (window, bg_gc,
                             allocation->x + i + allocation->width / 2,
                             allocation->y,
                             allocation->x + i + allocation->width / 2,
                             allocation->y + allocation->height * (1.0 - rv));
              gdk_draw_line (window, fg_gc,
                             allocation->x + i + allocation->width / 2,
                             allocation->y + allocation->height * (1.0 - rv),
                             allocation->x + i + allocation->width / 2,
                             allocation->y + allocation->height);
            }
        }
      bse_probe_seq_free (pseq);
    }
  bse_source_queue_probe_request (self->proxy, 0, 1, 1, 1, 1);
  bse_source_queue_probe_request (self->proxy, 1, 1, 1, 1, 1);
}

void
bst_sniffer_scope_set_sniffer (BstSnifferScope *self,
                               SfiProxy         proxy)
{
  if (proxy)
    g_return_if_fail (BSE_IS_SOURCE (proxy));
  if (!bse_source_has_outputs (proxy))
    proxy = 0;
  if (self->proxy)
    {
      bse_proxy_disconnect (self->proxy,
                            "any_signal::probes", scope_probes_notify, self,
                            NULL);
    }
  self->proxy = proxy;
  if (self->proxy)
    {
      bse_proxy_connect (self->proxy,
                         "signal::probes", scope_probes_notify, self,
                         NULL);
      bse_source_queue_probe_request (self->proxy, 0, 1, 1, 1, 1);
      bse_source_queue_probe_request (self->proxy, 1, 1, 1, 1, 1);
    }
}

static BseProbeRequestSeq *probe_request_seq = NULL;
static gboolean
source_probe_idle_request (gpointer data)
{
  GDK_THREADS_ENTER ();
  BseProbeRequestSeq *prs = probe_request_seq;
  probe_request_seq = NULL;
  if (prs)
    {
      bse_source_mass_request (prs);
      bse_probe_request_seq_free (prs);
    }
  GDK_THREADS_LEAVE ();
  return FALSE;
}

void
bse_source_queue_probe_request (SfiProxy            source,
                                guint               ochannel_id,
                                gboolean            probe_range,
                                gboolean            probe_energie,
                                gboolean            probe_samples,
                                gboolean            probe_fft)
{
  BseProbeFeatures features = { 0, };
  features.probe_range = probe_range;
  features.probe_energie = probe_energie;
  features.probe_samples = probe_samples;
  features.probe_fft = probe_fft;
  BseProbeRequest request = { 0, };
  request.source = source;
  request.channel_id = ochannel_id;
  request.probe_features = &features;
  if (!probe_request_seq)
    {
      g_idle_add_full (G_PRIORITY_HIGH, source_probe_idle_request, NULL, NULL);
      probe_request_seq = bse_probe_request_seq_new();
    }
  bse_probe_request_seq_append (probe_request_seq, &request);
}
