// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstpartview.hh"
#include "bstpartdialog.hh"
#include "bstpianoroll.hh"
#include "bse/internal.hh"

/* --- prototypes --- */
static void     part_view_action_exec           (gpointer                data,
                                                 size_t                  action);
static gboolean part_view_action_check          (gpointer                data,
                                                 size_t                  action,
                                                 guint64                 action_stamp);

/* --- part actions --- */
enum {
  ACTION_ADD_PART,
  ACTION_DELETE_PART,
  ACTION_EDIT_PART
};
static const GxkStockAction part_view_actions[] = {
  { N_("Add"),          NULL,   NULL,   ACTION_ADD_PART,        BST_STOCK_PART },
  { N_("Delete"),       NULL,   NULL,   ACTION_DELETE_PART,     BST_STOCK_TRASHCAN },
  { N_("Editor"),       NULL,   NULL,   ACTION_EDIT_PART,       BST_STOCK_PART_EDITOR },
};


/* --- functions --- */
G_DEFINE_TYPE (BstPartView, bst_part_view, BST_TYPE_ITEM_VIEW);

static void
bst_part_view_class_init (BstPartViewClass *klass)
{
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (klass);

  item_view_class->item_type = "BsePart";
}

static void
bst_part_view_init (BstPartView *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  /* complete GUI */
  GxkRadget *radget = gxk_radget_complete (GTK_WIDGET (self), "beast", "part-view", NULL);
  /* setup tree view */
  GtkTreeView *tview = (GtkTreeView*) gxk_radget_find (radget, "tree-view");
  bst_item_view_complete_tree (iview, tview);
  /* create tool actions */
  gxk_widget_publish_actions (self, "part-view-actions",
                              G_N_ELEMENTS (part_view_actions), part_view_actions,
                              NULL, part_view_action_check, part_view_action_exec);
  /* create property editor */
  bst_item_view_build_param_view (iview, (GtkContainer*) gxk_radget_find (radget, "property-area"));
}

static void
popup_part_dialog (BstPartView *part_view)
{
  GtkWidget *pdialog;

  Bse::PartH part = bst_item_view_get_current_part (BST_ITEM_VIEW (part_view));
  pdialog = (GtkWidget*) g_object_new (BST_TYPE_PART_DIALOG, NULL);

  bst_part_dialog_set_part (BST_PART_DIALOG (pdialog), part);
  g_signal_connect_object (part_view, "destroy", G_CALLBACK (gtk_widget_destroy), pdialog, G_CONNECT_SWAPPED);
  gtk_widget_show (pdialog);
}

GtkWidget*
bst_part_view_new (SfiProxy song)
{
  GtkWidget *part_view;

  assert_return (BSE_IS_SONG (song), NULL);

  part_view = gtk_widget_new (BST_TYPE_PART_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (part_view), song);

  return part_view;
}

static void
part_view_action_exec (gpointer                data,
                       size_t                  action)
{
  BstPartView *self = BST_PART_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  Bse::SongH song = Bse::SongH::__cast__ (item_view->container);
  Bse::PartH part;
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_PART:
      part = song.create_part();
      bst_item_view_select (item_view, part.proxy_id());
      break;
    case ACTION_DELETE_PART:
      item = bst_item_view_get_current (item_view);
      part = Bse::PartH::__cast__ (bse_server.from_proxy (item));
      song.remove_part (part);
      break;
    case ACTION_EDIT_PART:
      popup_part_dialog (self);
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
part_view_action_check (gpointer                data,
                        size_t                  action,
                        guint64                 action_stamp)
{
  BstPartView *self = BST_PART_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_PART:
      return TRUE;
    case ACTION_DELETE_PART:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    case ACTION_EDIT_PART:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    default:
      return FALSE;
    }
}
