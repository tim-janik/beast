// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_RACK_ITEM_H__
#define __GXK_RACK_ITEM_H__
#include <gxk/gxkracktable.hh>
G_BEGIN_DECLS
/* --- type macros --- */
#define GXK_TYPE_RACK_ITEM              (gxk_rack_item_get_type ())
#define GXK_RACK_ITEM(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_RACK_ITEM, GxkRackItem))
#define GXK_RACK_ITEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_RACK_ITEM, GxkRackItemClass))
#define GXK_IS_RACK_ITEM(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_RACK_ITEM))
#define GXK_IS_RACK_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_RACK_ITEM))
#define GXK_RACK_ITEM_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_RACK_ITEM, GxkRackItemClass))
/* --- structures & typedefs --- */
typedef struct  _GxkRackItem            GxkRackItem;
typedef struct  _GxkRackItemClass       GxkRackItemClass;
struct _GxkRackItem
{
  GtkFrame         parent_instance;
  /* maintained by GxkRackTable */
  gint             col, row, hspan, vspan;
  guint            empty_frame : 1;
};
struct _GxkRackItemClass
{
  GtkFrameClass parent_class;
  void          (*button_press) (GxkRackItem    *item,
                                 GdkEventButton *event);
};
/* --- prototypes --- */
GtkType         gxk_rack_item_get_type          (void);
void            gxk_rack_item_gui_changed       (GxkRackItem    *self);
gboolean        gxk_rack_item_set_area          (GxkRackItem    *self,
                                                 gint            col,
                                                 gint            row,
                                                 gint            hspan,
                                                 gint            vspan);
G_END_DECLS
#endif /* __GXK_RACK_ITEM_H__ */
