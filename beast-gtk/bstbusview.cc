// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstbusview.hh"
#include "bse/internal.hh"

/* --- prototypes --- */
static void     bus_view_action_exec           (gpointer                data,
                                                size_t                  action);
static gboolean bus_view_action_check          (gpointer                data,
                                                size_t                  action,
                                                guint64                 action_stamp);

/* --- bus actions --- */
enum {
  ACTION_ADD_BUS,
  ACTION_DELETE_BUS,
};
static const GxkStockAction bus_view_actions[] = {
  { N_("Add"),          NULL,   NULL,   ACTION_ADD_BUS,        BST_STOCK_NO_ICON },
  { N_("Delete"),       NULL,   NULL,   ACTION_DELETE_BUS,     BST_STOCK_TRASHCAN },
};

/* --- functions --- */
G_DEFINE_TYPE (BstBusView, bst_bus_view, BST_TYPE_ITEM_VIEW);

static void
bst_bus_view_class_init (BstBusViewClass *klass)
{
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (klass);

  item_view_class->item_type = "BseBus";
}

static void
bst_bus_view_init (BstBusView *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  /* complete GUI */
  GxkRadget *radget = gxk_radget_complete (GTK_WIDGET (self), "beast", "bus-view", NULL);
  /* setup tree view */
  GtkTreeView *tview = (GtkTreeView*) gxk_radget_find (radget, "tree-view");
  bst_item_view_complete_tree (iview, tview);
  /* create tool actions */
  gxk_widget_publish_actions (self, "bus-view-actions",
                              G_N_ELEMENTS (bus_view_actions), bus_view_actions,
                              NULL, bus_view_action_check, bus_view_action_exec);
  /* create property editor */
  bst_item_view_build_param_view (iview, (GtkContainer*) gxk_radget_find (radget, "property-area"));
}

GtkWidget*
bst_bus_view_new (SfiProxy song)
{
  GtkWidget *bus_view;

  assert_return (BSE_IS_SONG (song), NULL);

  bus_view = gtk_widget_new (BST_TYPE_BUS_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (bus_view), song);

  return bus_view;
}

static void
bus_view_action_exec (gpointer                data,
                      size_t                  action)
{
  BstBusView *self = BST_BUS_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  Bse::SongH song = Bse::SongH::down_cast (item_view->container);
  Bse::BusH bus;
  switch (action)
    {
    case ACTION_ADD_BUS:
      bus = song.create_bus();
      bst_item_view_select (item_view, bus.proxy_id());
      break;
    case ACTION_DELETE_BUS:
      bus = Bse::BusH::down_cast (bse_server.from_proxy (bst_item_view_get_current (item_view)));
      song.remove_bus (bus);
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
bus_view_action_check (gpointer                data,
                       size_t                  action,
                       guint64                 action_stamp)
{
  BstBusView *self = BST_BUS_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_BUS:
      return TRUE;
    case ACTION_DELETE_BUS:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    default:
      return FALSE;
    }
}
