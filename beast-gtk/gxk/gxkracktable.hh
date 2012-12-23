// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_RACK_TABLE_H__
#define __GXK_RACK_TABLE_H__

#include <gxk/gxkutils.hh>

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_RACK_TABLE              (gxk_rack_table_get_type ())
#define GXK_RACK_TABLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_RACK_TABLE, GxkRackTable))
#define GXK_RACK_TABLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_RACK_TABLE, GxkRackTableClass))
#define GXK_IS_RACK_TABLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_RACK_TABLE))
#define GXK_IS_RACK_TABLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_RACK_TABLE))
#define GXK_RACK_TABLE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_RACK_TABLE, GxkRackTableClass))


/* --- structures & typedefs --- */
typedef struct _GxkRackTable      GxkRackTable;
typedef struct _GxkRackTableClass GxkRackTableClass;
typedef struct _GxkRackEditor     GxkRackEditor;
struct _GxkRackTable
{
  GtkTable       parent_object;
  GBitMatrix    *child_map;
  guint          cell_request_width;
  guint          cell_request_height;
  guint          cell_width;
  guint          cell_height;
  GSList        *covers;
  GxkRackEditor *editor;
};
struct _GxkRackTableClass
{
  GtkTableClass parent_class;

  void  (*edit_mode_changed)    (GxkRackTable   *self,
                                 gboolean        edit_mode);
  void  (*child_changed)        (GxkRackTable   *self,
                                 GtkWidget      *child);
};


/* --- prototypes --- */
GType      gxk_rack_table_get_type             (void);
gboolean   gxk_rack_table_get_child_area       (GxkRackTable *self,
                                                GtkWidget    *child,
                                                guint        *col,
                                                guint        *row,
                                                guint        *hspan,
                                                guint        *vspan);
void       gxk_rack_table_redraw_cells         (GxkRackTable *self,
                                                guint         hcell1,
                                                guint         vcell1,
                                                guint         hspan,
                                                guint         vspan);
gboolean   gxk_rack_table_translate            (GxkRackTable *self,
                                                gint          x,
                                                gint          y,
                                                guint        *hcell,
                                                guint        *vcell);
GtkWidget* gxk_rack_table_find_child           (GxkRackTable *self,
                                                gint          x,
                                                gint          y);
void       gxk_rack_table_update_child_map     (GxkRackTable *self);
void       gxk_rack_table_invalidate_child_map (GxkRackTable *self);
void       gxk_rack_table_attach               (GxkRackTable *self,
                                                GtkWidget    *child,
                                                guint         col,
                                                guint         row,
                                                guint         hspan,
                                                guint         vspan);
void       gxk_rack_table_cover_up             (GxkRackTable *self);
void       gxk_rack_table_uncover              (GxkRackTable *self);
                                                

G_END_DECLS

#endif /* __GXK_RACK_TABLE_H__ */
