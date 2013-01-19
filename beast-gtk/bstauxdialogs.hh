// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_AUX_DIALOGS_H__
#define __BST_AUX_DIALOGS_H__
#include "bstutils.hh"
G_BEGIN_DECLS
/* --- list popup dialog --- */
typedef void (*BstListPopupHandler)  (GtkWidget              *dialog,
                                      gchar                 **strings,
                                      gpointer                user_data);
GtkWidget*     bst_list_popup_new    (const gchar            *title,
                                      GtkWidget              *transient_parent,
                                      BstListPopupHandler     handler,
                                      gpointer                data,
                                      GDestroyNotify          destroy);
void           bst_list_popup_add    (GtkWidget              *widget,
                                      const gchar            *string);
typedef void (*BstColorPopupHandler) (GtkWidget              *dialog,
                                      GdkColor               *color,
                                      gpointer                user_data);
GtkWidget*     bst_color_popup_new   (const gchar            *title,
                                      GtkWidget              *transient_parent,
                                      GdkColor                color,
                                      BstColorPopupHandler    handler,
                                      gpointer                data,
                                      GDestroyNotify          destroy);
gboolean       bst_key_combo_valid   (guint                   keyval,
                                      GdkModifierType         modifiers);
gboolean       bst_key_combo_popup   (const gchar            *function,
                                      guint                  *keyval,
                                      GdkModifierType        *modifier);
G_END_DECLS
#endif /* __BST_AUX_DIALOGS_H__ */
