// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_SIMPLE_LABEL_H__
#define __GXK_SIMPLE_LABEL_H__
#include <gxk/gxkutils.hh>
G_BEGIN_DECLS
#define GXK_TYPE_SIMPLE_LABEL		  (gxk_simple_label_get_type ())
#define GXK_SIMPLE_LABEL(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GXK_TYPE_SIMPLE_LABEL, GxkSimpleLabel))
#define GXK_SIMPLE_LABEL_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_SIMPLE_LABEL, GxkSimpleLabelClass))
#define GXK_IS_SIMPLE_LABEL(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GXK_TYPE_SIMPLE_LABEL))
#define GXK_IS_SIMPLE_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_SIMPLE_LABEL))
#define GXK_SIMPLE_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GXK_TYPE_SIMPLE_LABEL, GxkSimpleLabelClass))
typedef struct {
  GtkMisc        parent_instance;
  gchar         *label;
  guint8         jtype;
  guint          use_underline : 1;
  guint          auto_cut : 1;
  guint          needs_cutting : 1;
  gchar         *text;
  guint          mnemonic_keyval;
  GtkWidget     *mnemonic_widget;
  GtkWindow     *mnemonic_window;
  PangoAttrList *effective_attrs;
  PangoLayout   *layout;
} GxkSimpleLabel;
typedef GtkMiscClass GxkSimpleLabelClass;
GType   gxk_simple_label_get_type             (void) G_GNUC_CONST;
void    gxk_simple_label_set_mnemonic_widget  (GxkSimpleLabel  *self,
                                               GtkWidget       *widget);
G_END_DECLS
#endif /* __GXK_SIMPLE_LABEL_H__ */
