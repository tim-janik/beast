// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_ITEM_SEQ_DIALOG_H__
#define __BST_ITEM_SEQ_DIALOG_H__
#include "bstutils.hh"
#include "bstwaveview.hh"
G_BEGIN_DECLS
/* --- Gtk+ type macros --- */
#define BST_TYPE_ITEM_SEQ_DIALOG            (bst_item_seq_dialog_get_type ())
#define BST_ITEM_SEQ_DIALOG(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_ITEM_SEQ_DIALOG, BstItemSeqDialog))
#define BST_ITEM_SEQ_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_ITEM_SEQ_DIALOG, BstItemSeqDialogClass))
#define BST_IS_ITEM_SEQ_DIALOG(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_ITEM_SEQ_DIALOG))
#define BST_IS_ITEM_SEQ_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_ITEM_SEQ_DIALOG))
#define BST_ITEM_SEQ_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BST_TYPE_ITEM_SEQ_DIALOG, BstItemSeqDialogClass))
/* --- structures & typedefs --- */
typedef struct _BstItemSeqDialog          BstItemSeqDialog;
typedef struct _BstItemSeqDialogClass     BstItemSeqDialogClass;
typedef void (*BstItemSeqDialogSelected) (gpointer             data,
                                          BseItemSeq          *iseq,
                                          BstItemSeqDialog    *isdialog);
struct _BstItemSeqDialog
{
  GxkDialog         parent_instance;
  GtkTreeModel     *candidate_store;
  GtkTreeSelection *candidate_sel;
  GtkTreeModel     *item_store; /* proxy store */
  GtkTreeSelection *item_sel;
  GtkWidget     *ok;            /* ok button */
  GtkWindow     *parent_window;
  guint          ignore_activate : 1;
  BstItemSeqDialogSelected selected_callback;
  gpointer                 selected_data;
  GxkFreeFunc              selected_cleanup;
};
struct _BstItemSeqDialogClass
{
  GxkDialogClass parent_class;
};
/* --- prototypes --- */
GType      bst_item_seq_dialog_get_type (void);
GtkWidget* bst_item_seq_dialog_popup    (gpointer                  parent_widget,
                                         SfiProxy                  item,
                                         const gchar              *candidate_label,
                                         const gchar              *candidate_tooltip,
                                         BseItemSeq               *candidates,
                                         const gchar              *item_label,
                                         const gchar              *item_tooltip,
                                         BseItemSeq               *iseq,
                                         BstItemSeqDialogSelected  selected_callback,
                                         gpointer                  selected_data,
                                         GxkFreeFunc               selected_cleanup);
void       bst_item_seq_dialog_set      (BstItemSeqDialog         *self,
                                         BseItemSeq               *candidates,
                                         BseItemSeq               *iseq);
G_END_DECLS
#endif /* __BST_ITEM_SEQ_DIALOG_H__ */
