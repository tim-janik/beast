// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PATTERN_VIEW_H__
#define __BST_PATTERN_VIEW_H__

#include "bstpatterncolumns.hh"

/* --- type macros --- */
#define BST_TYPE_PATTERN_VIEW              (bst_pattern_view_get_type ())
#define BST_PATTERN_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_PATTERN_VIEW, BstPatternView))
#define BST_PATTERN_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_PATTERN_VIEW, BstPatternViewClass))
#define BST_IS_PATTERN_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_PATTERN_VIEW))
#define BST_IS_PATTERN_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_PATTERN_VIEW))
#define BST_PATTERN_VIEW_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_PATTERN_VIEW, BstPatternViewClass))


/* --- typedefs & enums --- */
/* bstpatterncolumns.hh: typedef struct _BstPatternView      BstPatternView; */
typedef struct _BstPatternViewClass BstPatternViewClass;


/* --- structures & typedefs --- */
typedef enum /*< skip >*/
{
  BST_PATTERN_VIEW_MARKER_NONE,
  BST_PATTERN_VIEW_MARKER_FOCUS,
} BstPatternViewMarkerType;
struct BstPatternViewDrag : GxkScrollCanvasDrag {
  uint          start_col = 0;
  uint          start_row = 0;
  uint          start_tick = 0;
  uint          start_duration = 0;
  bool          start_valid = false;
  bool          current_valid = false;
  uint          current_col = 0;
  uint          current_row = 0;
  int           current_tick = 0;
  int           current_duration = 0;
  // convenience:
  BstPatternView *pview = NULL;
};
struct _BstPatternView
{
  GxkScrollCanvas    parent_instance;

  Bse::PartS         part;

  /* vertical layout */
  guint              row_height;
  guint              tpqn;              /* ticks per quarter note */
  guint              tpt;               /* ticks per tact */
  gint               vticks;            /* ticks per row */
  guint              max_ticks;

  /* horizontal layout */
  guint              n_cols;
  BstPatternColumn **cols;
  gchar             *layout_string;

  /* focus cell */
  guint              focus_col;
  guint              focus_row;
  int                n_focus_cols;
  BstPatternColumn **focus_cols;

  /* shading */
  guint16            srow1, srow2;

  /* last drag state */
  guint              start_col;
  guint              start_row;
  guint              start_tick;
  guint              start_duration;
  gboolean           start_valid;
};
struct _BstPatternViewClass
{
  GxkScrollCanvasClass parent_class;

  void          (*drag)                         (BstPatternView     *self,
                                                 BstPatternViewDrag *drag);
  void          (*clicked)                      (BstPatternView     *tview,
                                                 guint               button,
                                                 guint               row,
                                                 guint               tick_position,
                                                 GdkEvent           *event);
};


/* --- prototypes --- */
GType             bst_pattern_view_get_type        (void);
void              bst_pattern_view_set_part        (BstPatternView *self, Bse::PartH part = Bse::PartH());
void              bst_pattern_view_vsetup          (BstPatternView            *self,
                                                    guint                      tpqn,
                                                    guint                      qnpt,
                                                    guint                      max_ticks,
                                                    guint                      vticks);
void              bst_pattern_view_set_shading     (BstPatternView            *self,
                                                    guint                      row1,
                                                    guint                      row2,
                                                    guint                      row3,
                                                    guint                      row4);
void              bst_pattern_view_set_marker      (BstPatternView            *self,
                                                    guint                      mark_index,
                                                    guint                      position,
                                                    BstPatternViewMarkerType   mtype);
void              bst_pattern_view_set_pixmarker   (BstPatternView            *self,
                                                    guint                      mark_index,
                                                    BstPatternViewMarkerType   mtype,
                                                    gint                       x,
                                                    gint                       y,
                                                    gint                       width,
                                                    gint                       height);
void              bst_pattern_view_add_column      (BstPatternView            *self,
                                                    BstPatternLType            ltype,
                                                    gint                       num,
                                                    BstPatternLFlags           lflags);
void              bst_pattern_view_set_focus       (BstPatternView *self, int focus_col, int focus_row);
gint              bst_pattern_view_get_focus_width (BstPatternView            *self);
BstPatternColumn* bst_pattern_view_get_focus_cell  (BstPatternView *self, int *tick, int *duration);
gboolean          bst_pattern_view_dispatch_key    (BstPatternView            *self,
                                                    guint                      keyval,
                                                    GdkModifierType            modifier,
                                                    BstPatternFunction         action,
                                                    gdouble                    param,
                                                    BstPatternFunction        *movement);
gint              bst_pattern_view_get_last_row    (BstPatternView            *self);
const gchar*      bst_pattern_view_get_layout      (BstPatternView            *self);
guint             bst_pattern_view_set_layout      (BstPatternView            *self,
                                                    const gchar               *layout);


#endif /* __BST_PATTERN_VIEW_H__ */
