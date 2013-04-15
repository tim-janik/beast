// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstwaveview.hh"
#include "bstprocedure.hh"
#include "bstwaveeditor.hh"
#include "bstfiledialog.hh"
#include "bstsampleeditor.hh"


/* --- prototypes --- */
static void     wave_view_action_exec           (gpointer                data,
                                                 gulong                  action);
static gboolean wave_view_action_check          (gpointer                data,
                                                 gulong                  action,
                                                 guint64                 action_stamp);


/* --- wave actions --- */
enum {
  ACTION_LOAD_WAVE,
  ACTION_LOAD_WAVE_LIB,
  ACTION_DELETE_WAVE,
  ACTION_EDIT_WAVE,
  ACTION_WAVE_LAST
};
static const GxkStockAction wave_view_actions[] = {
  { N_("Load..."),  NULL,       N_("Load a new wave file from disk"),
    ACTION_LOAD_WAVE,           BST_STOCK_LOAD,	},
  { N_("Lib..."),   NULL,       N_("Load a new wave file from library paths"),
    ACTION_LOAD_WAVE_LIB,	BST_STOCK_LOAD_LIB, },
  { N_("Delete"),   NULL,       N_("Delete the currently selected wave"),
    ACTION_DELETE_WAVE,         BST_STOCK_TRASHCAN, },
  { N_("Editor"),   NULL,       N_("Edit the currently selected wave"),
    ACTION_EDIT_WAVE,	        BST_STOCK_EDIT_TOOL, },
};


/* --- functions --- */
G_DEFINE_TYPE (BstWaveView, bst_wave_view, BST_TYPE_ITEM_VIEW);

static void
bst_wave_view_class_init (BstWaveViewClass *klass)
{
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (klass);

  item_view_class->item_type = "BseWave";
}

static void
bst_wave_view_init (BstWaveView *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  /* complete GUI */
  GxkRadget *radget = gxk_radget_complete (GTK_WIDGET (self), "beast", "wave-view", NULL);
  gxk_widget_publish_actions (self, "wave-view-actions",
                              G_N_ELEMENTS (wave_view_actions), wave_view_actions,
                              NULL, wave_view_action_check, wave_view_action_exec);
  /* setup tree view */
  GtkTreeView *tview = (GtkTreeView*) gxk_radget_find (radget, "tree-view");
  bst_item_view_complete_tree (iview, tview);
  /* prime locals */
  self->editable = TRUE;
}

GtkWidget*
bst_wave_view_new (SfiProxy wrepo)
{
  GtkWidget *wave_view;

  g_return_val_if_fail (BSE_IS_WAVE_REPO (wrepo), NULL);

  wave_view = gtk_widget_new (BST_TYPE_WAVE_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (wave_view), wrepo);

  return wave_view;
}

static void
popup_wave_dialog (BstWaveView *wave_view)
{
  SfiProxy wave = bst_item_view_get_current (BST_ITEM_VIEW (wave_view));
  GtkWidget *weditor, *wdialog;

  weditor = bst_wave_editor_new (wave);
  wdialog = (GtkWidget*) gxk_dialog_new (NULL, GTK_OBJECT (wave_view), GXK_DIALOG_DELETE_BUTTON, NULL, weditor);
  bst_window_sync_title_to_proxy (GXK_DIALOG (wdialog), wave, "%s");
  gtk_widget_show (wdialog);
}

#if 0
static void
popup_wave_dialog (BstWaveView *wave_view)
{
  SfiProxy wave = bst_item_view_get_current (BST_ITEM_VIEW (wave_view));
  SfiProxy esample = bse_wave_use_editable (wave, 0);

  if (esample)
    {
      GtkWidget *wdialog, *editor = bst_sample_editor_new (esample);

      wdialog = gxk_dialog_new (NULL, GTK_OBJECT (wave_view), GXK_DIALOG_DELETE_BUTTON,
				NULL,
				editor);
      bst_window_sync_title_to_proxy (GXK_DIALOG (wdialog), esample, "%s");
      gtk_widget_show (editor);
      bse_item_unuse (esample);
      gtk_widget_show (wdialog);
    }
}
#endif

void
bst_wave_view_set_editable (BstWaveView *self,
                            gboolean     enabled)
{
  BstItemView *iview = BST_ITEM_VIEW (self);

  g_return_if_fail (BST_IS_WAVE_VIEW (self));

  self->editable = enabled != FALSE;
  if (iview->tree)
    gxk_tree_view_set_editable (iview->tree, self->editable);

  gxk_widget_update_actions_downwards (self);
}

static void
wave_view_action_exec (gpointer                data,
                       gulong                  action)
{
  BstWaveView *self = BST_WAVE_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  SfiProxy wrepo = item_view->container;

  switch (action)
    {
      SfiProxy item;
    case ACTION_LOAD_WAVE:
      bst_file_dialog_popup_load_wave (item_view, BST_ITEM_VIEW (self)->container, FALSE);
      break;
    case ACTION_LOAD_WAVE_LIB:
      bst_file_dialog_popup_load_wave (item_view, BST_ITEM_VIEW (self)->container, TRUE);
      break;
    case ACTION_DELETE_WAVE:
      item = bst_item_view_get_current (BST_ITEM_VIEW (self));
      bse_wave_repo_remove_wave (wrepo, item);
      break;
    case ACTION_EDIT_WAVE:
      popup_wave_dialog (self);
      break;
    default:
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
wave_view_action_check (gpointer                data,
                        gulong                  action,
                        guint64                 action_stamp)
{
  BstWaveView *self = BST_WAVE_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  switch (action)
    {
    case ACTION_LOAD_WAVE:
    case ACTION_LOAD_WAVE_LIB:
      return TRUE;
    case ACTION_DELETE_WAVE:
      return bst_item_view_get_current (item_view) != 0;
    case ACTION_EDIT_WAVE:
      return bst_item_view_get_current (item_view) != 0 && self->editable;
    default:
      return FALSE;
    }
}
