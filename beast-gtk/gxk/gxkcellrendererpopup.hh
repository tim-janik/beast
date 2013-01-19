// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_CELL_RENDERER_POPUP_H__
#define __GXK_CELL_RENDERER_POPUP_H__
#include <gxk/gxkutils.hh>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkeventbox.h>
G_BEGIN_DECLS
/* --- type macros --- */
#define GXK_TYPE_CELL_RENDERER_POPUP              (gxk_cell_renderer_popup_get_type ())
#define GXK_CELL_RENDERER_POPUP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_CELL_RENDERER_POPUP, GxkCellRendererPopup))
#define GXK_CELL_RENDERER_POPUP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_CELL_RENDERER_POPUP, GxkCellRendererPopupClass))
#define GXK_IS_CELL_RENDERER_POPUP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_CELL_RENDERER_POPUP))
#define GXK_IS_CELL_RENDERER_POPUP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_CELL_RENDERER_POPUP))
#define GXK_CELL_RENDERER_POPUP_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_CELL_RENDERER_POPUP, GxkCellRendererPopupClass))
/* --- structures & typedefs --- */
typedef struct {
  GtkCellRendererText parent_instace;
  GtkWidget          *dialog, *entry;
  guint               text_editing : 1;
  guint               popup_editing : 1;
  guint               auto_popup : 1;
} GxkCellRendererPopup;
typedef struct {
  GtkCellRendererTextClass parent_class;
  void (*popup) (GxkCellRendererPopup *cell,
		 const gchar          *path,
		 const gchar          *text);
} GxkCellRendererPopupClass;
/* --- functions --- */
GType	gxk_cell_renderer_popup_get_type	(void);
void	gxk_cell_renderer_popup_dialog		(GxkCellRendererPopup	*popup,
						 GtkWidget		*dialog);
void    gxk_cell_renderer_popup_change          (GxkCellRendererPopup   *popup,
                                                 const gchar            *text,
                                                 gboolean                preserve_popup,
                                                 gboolean                keep_editing);
/* --- type macros --- */
#define GXK_TYPE_PROXY_EDITABLE              (gxk_proxy_editable_get_type ())
#define GXK_PROXY_EDITABLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_PROXY_EDITABLE, GxkProxyEditable))
#define GXK_PROXY_EDITABLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_PROXY_EDITABLE, GxkProxyEditableClass))
#define GXK_IS_PROXY_EDITABLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_PROXY_EDITABLE))
#define GXK_IS_PROXY_EDITABLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_PROXY_EDITABLE))
#define GXK_PROXY_EDITABLE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_PROXY_EDITABLE, GxkProxyEditableClass))
/* --- structures & typedefs --- */
typedef struct {
  GtkEventBox	   parent_instace;
  GtkCellEditable *ecell;
  guint8	   block_start_editing;
  guint8	   block_remove_widget;
  guint8	   block_editing_done;
} GxkProxyEditable;
typedef struct {
  GtkEventBoxClass parent_class;
} GxkProxyEditableClass;
/* --- functions --- */
GType	gxk_proxy_editable_get_type		(void);
void	gxk_proxy_editable_set_cell_editable	(GxkProxyEditable	*self,
						 GtkCellEditable	*ecell);
G_END_DECLS
#endif	/* __GXK_CELL_RENDERER_POPUP_H__ */
