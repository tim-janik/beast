// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstsnifferscope.hh"
#include <string.h>


#define WIDGET(self)    (GTK_WIDGET (self))
#define STATE(self)     (WIDGET (self)->state)
#define STYLE(self)     (WIDGET (self)->style)
#define BASE_GC(self)   (STYLE (self)->base_gc[STATE (self)])

G_DEFINE_TYPE (BstSnifferScope, bst_sniffer_scope, GTK_TYPE_WIDGET);

/* --- functions --- */
static void
bst_sniffer_scope_init (BstSnifferScope *self)
{
  new (&self->source) Bse::SourceH();
  new (&self->lmonitor) Bse::SignalMonitorH();
  new (&self->rmonitor) Bse::SignalMonitorH();
  self->mon_handler = 0;
  GtkWidget *widget = GTK_WIDGET (self);

  GTK_WIDGET_SET_FLAGS (self, GTK_NO_WINDOW);
  gtk_widget_show (widget);
}

GtkWidget*
bst_sniffer_scope_new (void)
{
  BstSnifferScope *self = (BstSnifferScope*) g_object_new (BST_TYPE_SNIFFER_SCOPE, NULL);
  return GTK_WIDGET (self);
}

static void
bst_sniffer_scope_destroy (GtkObject *object)
{
  BstSnifferScope *self = BST_SNIFFER_SCOPE (object);

  bst_sniffer_scope_set_sniffer (self);

  GTK_OBJECT_CLASS (bst_sniffer_scope_parent_class)->destroy (object);
}

static void
bst_sniffer_scope_finalize (GObject *object)
{
  BstSnifferScope *self = BST_SNIFFER_SCOPE (object);

  bst_sniffer_scope_set_sniffer (self);
  g_free (self->lvalues);
  g_free (self->rvalues);
  G_OBJECT_CLASS (bst_sniffer_scope_parent_class)->finalize (object);
  using namespace Bse;
  Bst::remove_handler (&self->mon_handler);
  self->source.~SourceH();
  self->lmonitor.~SignalMonitorH();
  self->rmonitor.~SignalMonitorH();
}

static void
bst_sniffer_scope_size_request (GtkWidget      *widget,
                                GtkRequisition *requisition)
{
  // BstSnifferScope *self = BST_SNIFFER_SCOPE (widget);

  requisition->width = 30 + 4;
  requisition->height = 20;
}

static void
bst_sniffer_scope_realize (GtkWidget *widget)
{
  BstSnifferScope *self = BST_SNIFFER_SCOPE (widget);
  GTK_WIDGET_CLASS (bst_sniffer_scope_parent_class)->realize (widget);
  self->oshoot_gc = gdk_gc_new (widget->window);
  GdkColor color = gdk_color_from_rgb (0xff0000);
  gdk_gc_set_rgb_fg_color (self->oshoot_gc, &color);
}

static void
bst_sniffer_scope_unrealize (GtkWidget *widget)
{
  BstSnifferScope *self = BST_SNIFFER_SCOPE (widget);
  g_object_unref (self->oshoot_gc);
  self->oshoot_gc = NULL;
  GTK_WIDGET_CLASS (bst_sniffer_scope_parent_class)->unrealize (widget);
}

static void
bst_sniffer_scope_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  BstSnifferScope *self = BST_SNIFFER_SCOPE (widget);
  GTK_WIDGET_CLASS (bst_sniffer_scope_parent_class)->size_allocate (widget, allocation);
  self->n_values = MAX (widget->allocation.width - 4, 2) / 2;
  self->lvalues = g_renew (float, self->lvalues, self->n_values);
  memset (self->lvalues, 0, self->n_values * sizeof (self->lvalues[0]));
  self->rvalues = g_renew (float, self->rvalues, self->n_values);
  memset (self->rvalues, 0, self->n_values * sizeof (self->rvalues[0]));
}

static void
sniffer_scope_lregion (BstSnifferScope *self,
                       gint            *x,
                       gint            *width)
{
  GtkWidget *widget = GTK_WIDGET (self);
  *width = self->n_values;
  *x = widget->allocation.x + 1;
}

static void
sniffer_scope_rregion (BstSnifferScope *self,
                       gint            *x,
                       gint            *width)
{
  GtkWidget *widget = GTK_WIDGET (self);
  *width = self->n_values;
  *x = widget->allocation.x + 1 + *width + 1 + (widget->allocation.width & 1) + 1;
}

static void
sniffer_scope_draw_bar (BstSnifferScope *self,
                        guint            x,
                        double           value)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GdkWindow *window = widget->window;
  GtkAllocation *allocation = &widget->allocation;
  GdkGC *fg_gc = widget->style->fg_gc[widget->state];
  GdkGC *bg_gc = BASE_GC (self);
  GdkGC *xg_gc = self->oshoot_gc;
  gint y = allocation->y + 1, height = MAX (allocation->height - 2, 1);
  gint top = CLAMP (value * height, 0, height);
  gdk_draw_vline (window, bg_gc,
                  x, y, height - top);
  gdk_draw_vline (window, value > 1.0 ? xg_gc : fg_gc,
                  x, y + height - top, top);
}

static gboolean
bst_sniffer_scope_expose (GtkWidget      *widget,
                          GdkEventExpose *event)
{
  BstSnifferScope *self = BST_SNIFFER_SCOPE (widget);
  GdkWindow *window = event->window;
  GtkAllocation *allocation = &widget->allocation;
  if (window != widget->window)
    return FALSE;

  /* draw left and right channel */
  gint i, xl, xr, width;
  sniffer_scope_lregion (self, &xl, &width);
  sniffer_scope_rregion (self, &xr, &width);
  for (i = 0; i < int (self->n_values); i++)
    {
      sniffer_scope_draw_bar (self, xl + width - 1 - i, self->lvalues[i]);
      sniffer_scope_draw_bar (self, xr + i, self->rvalues[i]);
    }
  /* draw outline and center */
  GdkGC *dark_gc = widget->style->dark_gc[widget->state];
  gdk_draw_hline (window, dark_gc, allocation->x, allocation->y + allocation->height - 1, allocation->width);
  GdkGC *light_gc = widget->style->light_gc[widget->state];
  gint y = allocation->y + 1, height = allocation->height - 2, x, bgcol = -1;
  gdk_draw_vline (window, light_gc, allocation->x + width + 0, y, height);
  gdk_draw_vline (window, dark_gc,  allocation->x + width + 1, y, height);
  gdk_draw_vline (window, light_gc, allocation->x + width + 2, y, height);
  /* draw outline */
  x = allocation->x;
  gdk_draw_vline (window, light_gc, x, y, height);
  x += 1 + width;
  gdk_draw_vline (window, dark_gc, x, y, height);
  x += 1;
  if (widget->allocation.width & 1)
    {
      bgcol = x;
      x += 1;
    }
  gdk_draw_vline (window, light_gc, x, y, height);
  x += 1 + width;
  gdk_draw_vline (window, dark_gc, x, y, height);
  gdk_draw_hline (window, light_gc, allocation->x, allocation->y, allocation->width);
  if (bgcol >= 0)       /* paint above hline outline */
    gdk_draw_vline (window, widget->style->bg_gc[GTK_STATE_NORMAL], bgcol, allocation->y, allocation->height);

  return FALSE;
}

static void
sniffer_scope_shift (BstSnifferScope *self)
{
  if (self->n_values)
    {
      memmove (self->lvalues + 1, self->lvalues, (self->n_values - 1) * sizeof (self->lvalues[0]));
      memmove (self->rvalues + 1, self->rvalues, (self->n_values - 1) * sizeof (self->rvalues[0]));
      if (GTK_WIDGET_DRAWABLE (self))
        {
          GtkWidget *widget = GTK_WIDGET (self);
          gint x, width;
          sniffer_scope_lregion (self, &x, &width);
          gdk_window_copy_area (widget->window, BASE_GC (self),
                                /* destination coords: */
                                x, widget->allocation.y + 1,
                                /* source rectangle: */
                                widget->window,
                                x + 1, widget->allocation.y + 1,
                                width - 1, widget->allocation.height - 2);
          sniffer_scope_rregion (self, &x, &width);
          gdk_window_copy_area (widget->window, BASE_GC (self),
                                /* destination coords: */
                                x + 1, widget->allocation.y + 1,
                                /* source rectangle: */
                                widget->window,
                                x, widget->allocation.y + 1,
                                width - 1, widget->allocation.height - 2);
        }
    }
}

static void
scope_draw_range (BstSnifferScope *self, float lmin, float lmax, float rmin, float rmax)
{
  if (GTK_WIDGET_DRAWABLE (self))
    {
      sniffer_scope_shift (self);
      self->lvalues[0] = MAX (ABS (lmin), ABS (lmax));
      self->rvalues[0] = MAX (ABS (rmin), ABS (rmax));
      int x, width;
      sniffer_scope_lregion (self, &x, &width);
      sniffer_scope_draw_bar (self, x + width - 1, self->lvalues[0]);
      sniffer_scope_rregion (self, &x, &width);
      sniffer_scope_draw_bar (self, x, self->rvalues[0]);
    }
}

static void
bst_sniffer_scope_class_init (BstSnifferScopeClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->finalize = bst_sniffer_scope_finalize;

  object_class->destroy = bst_sniffer_scope_destroy;

  widget_class->size_request = bst_sniffer_scope_size_request;
  widget_class->size_allocate = bst_sniffer_scope_size_allocate;
  widget_class->realize = bst_sniffer_scope_realize;
  widget_class->expose_event = bst_sniffer_scope_expose;
  widget_class->unrealize = bst_sniffer_scope_unrealize;
}

static Bse::MonitorFields*
monitor_fields_from_shm (int64 shm_id, uint32 shm_offset)
{
  Bse::SharedMemory sm = bse_server.get_shared_memory (shm_id);
  assert_return (sm.shm_id == shm_id, NULL);
  assert_return (sm.shm_creator == Bse::this_thread_getpid(), NULL);
  char *shm_start = (char*) sm.shm_start; // allowed if sm.shm_creator matches our pid
  assert_return (shm_start != NULL, NULL);
  assert_return (shm_offset + sizeof (Bse::MonitorFields) <= size_t (sm.shm_length), NULL);
  return (Bse::MonitorFields*) (shm_start + shm_offset);
}

void
bst_sniffer_scope_set_sniffer (BstSnifferScope *self, Bse::SourceH source)
{
  if (source and !source.has_outputs())
    source = Bse::SourceH();
  return_unless (source != self->source);
  Bst::remove_handler (&self->mon_handler);
  self->lmonitor = Bse::SignalMonitorH();
  self->rmonitor = Bse::SignalMonitorH();
  self->source = source;
  if (self->source)
    {
      bst_source_queue_probe_request (self->source.proxy_id(), 0, BST_SOURCE_PROBE_RANGE, 20.0);
      bst_source_queue_probe_request (self->source.proxy_id(), 1, BST_SOURCE_PROBE_RANGE, 20.0);
      if (self->source.n_ochannels() >= 2)
        {
          self->lmonitor = self->source.create_signal_monitor (0);
          self->rmonitor = self->source.create_signal_monitor (1);
          Bse::ProbeFeatures features;
          features.probe_range = true;
          self->lmonitor.set_probe_features (features);
          self->rmonitor.set_probe_features (features);
          Bse::MonitorFields *lfields = monitor_fields_from_shm (self->lmonitor.get_shm_id(), self->lmonitor.get_shm_offset());
          Bse::MonitorFields *rfields = monitor_fields_from_shm (self->rmonitor.get_shm_id(), self->rmonitor.get_shm_offset());
          auto framecb = [self, lfields, rfields] () {
            scope_draw_range (self, lfields->min, lfields->max, rfields->min, rfields->max);
            if (0)
              printerr ("SnifferScope: (%x.%x/%x %x.%x/%x) %f,%f %f,%f\n",
                        self->lmonitor.get_shm_id(), self->lmonitor.get_shm_offset(), lfields->gen,
                        self->rmonitor.get_shm_id(), self->rmonitor.get_shm_offset(), rfields->gen,
                        lfields->min, lfields->max, rfields->min, rfields->max);
          };
          self->mon_handler = Bst::add_frame_handler (framecb);
        }
      static_assert (sizeof (Bse::MonitorFields) == sizeof (double) * 4 + sizeof (int64));
    }
}

static Bse::ProbeRequestSeq probe_request_seq;

static gboolean
source_probe_idle_request (gpointer data)
{
  GDK_THREADS_ENTER ();
  Bse::ProbeRequestSeq prs;
  prs.swap (probe_request_seq);
  if (prs.size())
    Bse::SourceH::down_cast (prs[0].source).request_probes (prs);
  GDK_THREADS_LEAVE ();
  return FALSE;
}

void
bst_source_queue_probe_request (SfiProxy              source,
                                guint                 ochannel_id,
                                BstSourceProbeFeature pfeature,
                                gfloat                frequency)
{
  Bse::ProbeFeatures features;
  features.probe_range = 0 != (pfeature & BST_SOURCE_PROBE_RANGE);
  features.probe_energie = 0 != (pfeature & BST_SOURCE_PROBE_ENERGIE);
  features.probe_samples = 0 != (pfeature & BST_SOURCE_PROBE_SAMPLES);
  features.probe_fft = 0 != (pfeature & BST_SOURCE_PROBE_FFT);
  Bse::ProbeRequest request;
  request.source = Bse::SourceH::down_cast (bse_server.from_proxy (source));
  request.channel = ochannel_id;
  request.probe_features = features;
  request.frequency = frequency;
  if (BST_GCONFIG (slow_scopes) && !(features.probe_samples || features.probe_fft))
    request.frequency = MIN (request.frequency, 4);
  if (probe_request_seq.size() == 0)
    g_idle_add_full (G_PRIORITY_LOW + 5, source_probe_idle_request, NULL, NULL);  // GTK_PRIORITY_REDRAW + 5
  probe_request_seq.push_back (std::move (request));
}
