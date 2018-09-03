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
  new (&self->source) Bse::SourceS();
  new (&self->lmonitor) Bse::SignalMonitorS();
  new (&self->rmonitor) Bse::SignalMonitorS();
  self->mon_handler = 0;
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
  using namespace Bse;
  Bst::remove_handler (&self->mon_handler);
  self->source.~SourceS();
  self->lmonitor.~SignalMonitorS();
  self->rmonitor.~SignalMonitorS();
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

static GxkParam *
get_property_param (BstBusEditor *self,
                    const gchar  *property)
{
  Bse::BusH bus = Bse::BusH::down_cast (bse_server.from_proxy (self->item));
  for (auto cxxpspec : Bse::introspection_fields_to_param_list (bus.__aida_aux_data__()))
    {
      std::string pname = cxxpspec->name;
      for (auto& c : pname)
        if (c == '-')
          c = '_';

      if (property == pname)
        return bst_param_new_property (cxxpspec, bus);
    }

  return nullptr;
}

static GtkWidget*
bus_build_param (BstBusEditor *self,
                 const gchar  *property,
                 const gchar  *area,
                 const gchar  *editor,
                 const gchar  *label)
{
  GxkParam *gxk_param = get_property_param (self, property); /* aida property? */
  if (!gxk_param)
    {
      /* proxy property */
      auto pspec = bse_proxy_get_pspec (self->item, property);
      gxk_param = bst_param_new_proxy (pspec, self->item);
    }

  self->params = sfi_ring_prepend (self->params, gxk_param);
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
      Bst::remove_handler (&self->mon_handler);
      self->lmonitor = NULL;
      self->rmonitor = NULL;
      Bse::SourceH source = Bse::SourceH::down_cast (bse_server.from_proxy (self->item));
      bse_proxy_disconnect (self->item,
                            "any-signal", bus_editor_release_item, self,
                            NULL);
      self->source = NULL;
      while (self->params)
        gxk_param_destroy ((GxkParam*) sfi_ring_pop_head (&self->params));
    }
  self->item = item;
  if (self->item)
    {
      self->source = Bse::SourceH::down_cast (bse_server.from_proxy (self->item));
      SfiRing *ring;
      bse_proxy_connect (self->item,
                         "signal::release", bus_editor_release_item, self,
                         NULL);
      /* create and hook up volume params & scopes */
      GxkParam *lvolume = get_property_param (self, "left_volume");
      GtkWidget *lspinner = gxk_param_create_editor (lvolume, "spinner");
      GxkParam *rvolume = get_property_param (self, "right_volume");
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
      Bse::ProbeFeatures features;
      features.probe_energy = true;
      self->lmonitor = self->source.create_signal_monitor (0);
      self->rmonitor = self->source.create_signal_monitor (1);
      self->lmonitor.set_probe_features (features);
      self->rmonitor.set_probe_features (features);
      Bst::MonitorFieldU lfields = Bst::monitor_fields_from_shm (self->lmonitor.get_shm_id(), self->lmonitor.get_shm_offset());
      Bst::MonitorFieldU rfields = Bst::monitor_fields_from_shm (self->rmonitor.get_shm_id(), self->rmonitor.get_shm_offset());
      auto framecb = [self, lfields, rfields] () {
        bst_db_beam_set_value (self->lbeam, lfields.f32 (Bse::MonitorField::F32_DB_SPL));
        bst_db_beam_set_value (self->rbeam, rfields.f32 (Bse::MonitorField::F32_DB_SPL));
        if (0)
          printerr ("BstBusEditor: (%x.%x/%x %x.%x/%x) ldb=%f rdb=%f\n",
                    self->lmonitor.get_shm_id(), self->lmonitor.get_shm_offset(),
                    lfields.f64 (Bse::MonitorField::F64_GENERATION),
                    self->rmonitor.get_shm_id(), self->rmonitor.get_shm_offset(),
                    rfields.f64 (Bse::MonitorField::F64_GENERATION),
                    lfields.f32 (Bse::MonitorField::F32_DB_SPL),
                    rfields.f32 (Bse::MonitorField::F32_DB_SPL));
      };
      self->mon_handler = Bst::add_frame_handler (framecb);
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
