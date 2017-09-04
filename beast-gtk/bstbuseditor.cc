// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstbuseditor.hh"
#include "bstparam.hh"
#include "bstitemseqdialog.hh" // FIXME
#include "bstsnifferscope.hh" // FIXME


/* --- prototypes --- */
static void     bus_editor_action_exec           (gpointer                data,
                                                  size_t                  action);
static gboolean bus_editor_action_check          (gpointer                data,
                                                  size_t                  action,
                                                  guint64                 action_stamp);


/* --- bus actions --- */
enum {
  ACTION_ADD_BUS,
  ACTION_DELETE_BUS,
  ACTION_EDIT_BUS
};
static const GxkStockAction bus_editor_actions[] = {
  { N_("Add"),          NULL,   NULL,   ACTION_ADD_BUS,        BST_STOCK_PART },
  { N_("Delete"),       NULL,   NULL,   ACTION_DELETE_BUS,     BST_STOCK_TRASHCAN },
  { N_("Editor"),       NULL,   NULL,   ACTION_EDIT_BUS,       BST_STOCK_PART_EDITOR },
};


/* --- functions --- */
G_DEFINE_TYPE (BstBusEditor, bst_bus_editor, GTK_TYPE_ALIGNMENT);

static void
bst_bus_editor_init (BstBusEditor *self)
{
  /* complete GUI */
  gxk_radget_complete (GTK_WIDGET (self), "beast", "bus-editor", NULL);
  /* create tool actions */
  gxk_widget_publish_actions (self, "bus-editor-actions",
                              G_N_ELEMENTS (bus_editor_actions), bus_editor_actions,
                              NULL, bus_editor_action_check, bus_editor_action_exec);
}

static void
bst_bus_editor_destroy (GtkObject *object)
{
  BstBusEditor *self = BST_BUS_EDITOR (object);
  bst_bus_editor_set_bus (self, 0);
  GTK_OBJECT_CLASS (bst_bus_editor_parent_class)->destroy (object);
}

static void
bst_bus_editor_finalize (GObject *object)
{
  BstBusEditor *self = BST_BUS_EDITOR (object);
  bst_bus_editor_set_bus (self, 0);
  G_OBJECT_CLASS (bst_bus_editor_parent_class)->finalize (object);
}

GtkWidget*
bst_bus_editor_new (SfiProxy bus)
{
  assert_return (BSE_IS_BUS (bus), NULL);
  GtkWidget *widget = (GtkWidget*) g_object_new (BST_TYPE_BUS_EDITOR, NULL);
  BstBusEditor *self = BST_BUS_EDITOR (widget);
  bst_bus_editor_set_bus (self, bus);
  return widget;
}

static void
bus_editor_release_item (SfiProxy      item,
                         BstBusEditor *self)
{
  assert_return (self->item == item);
  bst_bus_editor_set_bus (self, 0);
}

static void
bus_probes_notify (SfiProxy     bus,
                   SfiSeq      *sseq,
                   gpointer     data)
{
  BstBusEditor *self = BST_BUS_EDITOR (data);
  BseProbeSeq *pseq = bse_probe_seq_from_seq (sseq);
  BseProbe *lprobe = NULL, *rprobe = NULL;
  guint i;
  for (i = 0; i < pseq->n_probes && (!lprobe || !rprobe); i++)
    if (pseq->probes[i]->channel_id == 0)
      lprobe = pseq->probes[i];
    else if (pseq->probes[i]->channel_id == 1)
      rprobe = pseq->probes[i];
  if (self->lbeam && lprobe && lprobe->probe_features->probe_energie)
    bst_db_beam_set_value (self->lbeam, lprobe->energie);
  if (self->rbeam && rprobe && rprobe->probe_features->probe_energie)
    bst_db_beam_set_value (self->rbeam, rprobe->energie);
  bst_source_queue_probe_request (self->item, 0, BST_SOURCE_PROBE_ENERGIE, 20.0);
  bst_source_queue_probe_request (self->item, 1, BST_SOURCE_PROBE_ENERGIE, 20.0);
  bse_probe_seq_free (pseq);
}

static GtkWidget*
bus_build_param (BstBusEditor *self,
                 const gchar  *property,
                 const gchar  *area,
                 const gchar  *editor,
                 const gchar  *label)
{
  GParamSpec *pspec = bse_proxy_get_pspec (self->item, property);
  self->params = sfi_ring_prepend (self->params, bst_param_new_proxy (pspec, self->item));
  GtkWidget *ewidget = gxk_param_create_editor ((GxkParam*) self->params->data, editor);
  gxk_radget_add (self, area, ewidget);
  if (label)
    g_object_set (gxk_parent_find_descendant (ewidget, GTK_TYPE_LABEL), "label", label, NULL);
  return ewidget;
}

static gboolean
grab_focus_and_false (GtkWidget *widget)
{
  gtk_widget_grab_focus (widget);
  return FALSE;
}

void
bst_bus_editor_set_bus (BstBusEditor *self,
                        SfiProxy      item)
{
  if (item)
    assert_return (BSE_IS_BUS (item));
  if (self->item)
    {
      bse_proxy_disconnect (self->item,
                            "any_signal::probes", bus_probes_notify, self,
                            NULL);
      bse_proxy_disconnect (self->item,
                            "any-signal", bus_editor_release_item, self,
                            NULL);
      while (self->params)
        gxk_param_destroy ((GxkParam*) sfi_ring_pop_head (&self->params));
    }
  self->item = item;
  if (self->item)
    {
      GParamSpec *pspec;
      SfiRing *ring;
      bse_proxy_connect (self->item,
                         "signal::release", bus_editor_release_item, self,
                         NULL);
      /* create and hook up volume params & scopes */
      pspec = bse_proxy_get_pspec (self->item, "left-volume");
      GxkParam *lvolume = bst_param_new_proxy (pspec, self->item);
      GtkWidget *lspinner = gxk_param_create_editor (lvolume, "spinner");
      pspec = bse_proxy_get_pspec (self->item, "right-volume");
      GxkParam *rvolume = bst_param_new_proxy (pspec, self->item);
      GtkWidget *rspinner = gxk_param_create_editor (rvolume, "spinner");
      BstDBMeter *dbmeter = (BstDBMeter*) gxk_radget_find (self, "db-meter");
      if (dbmeter)
        {
          GtkRange *range = bst_db_meter_get_scale (dbmeter, 0);
          bst_db_scale_hook_up_param (range, lvolume);
          g_signal_connect_object (range, "button-press-event", G_CALLBACK (grab_focus_and_false), lspinner, G_CONNECT_SWAPPED);
          range = bst_db_meter_get_scale (dbmeter, 1);
          bst_db_scale_hook_up_param (range, rvolume);
          g_signal_connect_object (range, "button-press-event", G_CALLBACK (grab_focus_and_false), rspinner, G_CONNECT_SWAPPED);
          self->lbeam = bst_db_meter_get_beam (dbmeter, 0);
          if (self->lbeam)
            bst_db_beam_set_value (self->lbeam, -G_MAXDOUBLE);
          self->rbeam = bst_db_meter_get_beam (dbmeter, 1);
          if (self->rbeam)
            bst_db_beam_set_value (self->rbeam, -G_MAXDOUBLE);
        }
      gxk_radget_add (self, "spinner-box", lspinner);
      gxk_radget_add (self, "spinner-box", rspinner);
      self->params = sfi_ring_prepend (self->params, lvolume);
      self->params = sfi_ring_prepend (self->params, rvolume);
      /* create remaining params */
      bus_build_param (self, "uname", "name-box", NULL, NULL);
      bus_build_param (self, "inputs", "inputs-box", NULL, NULL);
      bus_build_param (self, "mute", "toggle-box", "toggle+label", "M");
      bus_build_param (self, "sync", "toggle-box", "toggle+label", "Y");
      bus_build_param (self, "solo", "toggle-box", "toggle+label", "S");
      bus_build_param (self, "outputs", "outputs-box", NULL, NULL);
      /* update params */
      for (ring = self->params; ring; ring = sfi_ring_walk (ring, self->params))
        gxk_param_update ((GxkParam*) ring->data);
      /* setup scope */
      bse_proxy_connect (self->item,
                         "signal::probes", bus_probes_notify, self,
                         NULL);
      bst_source_queue_probe_request (self->item, 0, BST_SOURCE_PROBE_ENERGIE, 20.0);
      bst_source_queue_probe_request (self->item, 1, BST_SOURCE_PROBE_ENERGIE, 20.0);
    }
}

static void
bus_editor_action_exec (gpointer data,
                        size_t   action)
{
  BstBusEditor *self = BST_BUS_EDITOR (data);
  switch (action)
    {
    case ACTION_ADD_BUS:
      break;
    case ACTION_DELETE_BUS:
      break;
    case ACTION_EDIT_BUS:
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
bus_editor_action_check (gpointer data,
                         size_t   action,
                         guint64  action_stamp)
{
  // BstBusEditor *self = BST_BUS_EDITOR (data);
  switch (action)
    {
    case ACTION_ADD_BUS:
    case ACTION_DELETE_BUS:
    case ACTION_EDIT_BUS:
      return TRUE;
    default:
      return FALSE;
    }
}

static void
bst_bus_editor_class_init (BstBusEditorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);

  gobject_class->finalize = bst_bus_editor_finalize;
  object_class->destroy = bst_bus_editor_destroy;
}
