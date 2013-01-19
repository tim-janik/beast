// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_RACK_EDITOR_H__
#define __GXK_RACK_EDITOR_H__
#include <gxk/gxkracktable.hh>
G_BEGIN_DECLS
struct _GxkRackEditor {
  GdkWindow     *iwindow;
  gint           rfx, rfy, rfw, rfh;
  /* child resizing windows */
  GdkWindow     *crq1, *crq2, *crq3, *crq4;     /* corner windows */
  GdkWindow     *crb1, *crb2, *crb3, *crb4;     /* bar windows: 1=upper, 2=right, 3=bottom 4=left */
  /* dragging */
  guint          grabbing : 1;
  GtkWidget     *drag_child;
  /* corners: 1=top-left, 2=top-right, 3=bottom-left, 4=bottom-right */
  gint           drag_corner, drag_x, drag_y;
  gint           drag_col, drag_row, drag_hspan, drag_vspan;
};
/* --- prototypes --- */
void     gxk_rack_table_set_edit_mode         (GxkRackTable     *self,
                                               gboolean          enable_editing);
gboolean gxk_rack_table_check_cell            (GxkRackTable     *self,
                                               guint             col,
                                               guint             row);
gboolean gxk_rack_table_check_area            (GxkRackTable     *self,
                                               gboolean          framed,
                                               guint             col,
                                               guint             row,
                                               guint             hspan,
                                               guint             vspan,
                                               GtkWidget        *exempt);
gboolean gxk_rack_table_expand_rect           (GxkRackTable     *self,
                                               guint             col,
                                               guint             row,
                                               guint            *hspan,
                                               guint            *vspan);
/* --- implementation details --- */
void     gxk_rack_table_destroy_editor        (GxkRackTable     *self);
gboolean gxk_rack_table_handle_enter_notify   (GxkRackTable     *self,
                                               GdkEventCrossing *event);
gboolean gxk_rack_table_handle_button_press   (GxkRackTable     *self,
                                               GdkEventButton   *event);
gboolean gxk_rack_table_handle_motion_notify  (GxkRackTable     *self,
                                               GdkEventMotion   *event);
gboolean gxk_rack_table_handle_button_release (GxkRackTable     *self,
                                               GdkEventButton   *event);
gboolean gxk_rack_table_handle_leave_notify   (GxkRackTable     *self,
                                               GdkEventCrossing *event);
void     gxk_rack_table_adjust_editor         (GxkRackTable     *self);
void     gxk_rack_table_unmap_editor          (GxkRackTable     *self);
G_END_DECLS
#endif /* __GXK_RACK_EDITOR_H__ */
