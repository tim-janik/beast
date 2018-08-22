// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_TRACK_ROLL_H__
#define __BST_TRACK_ROLL_H__

#include "bstutils.hh"

/* --- type macros --- */
#define BST_TYPE_TRACK_ROLL              (bst_track_roll_get_type ())
#define BST_TRACK_ROLL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_TRACK_ROLL, BstTrackRoll))
#define BST_TRACK_ROLL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_TRACK_ROLL, BstTrackRollClass))
#define BST_IS_TRACK_ROLL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_TRACK_ROLL))
#define BST_IS_TRACK_ROLL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_TRACK_ROLL))
#define BST_TRACK_ROLL_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_TRACK_ROLL, BstTrackRollClass))


/* --- typedefs & enums --- */
typedef struct _BstTrackRoll              BstTrackRoll;
typedef struct _BstTrackRollClass         BstTrackRollClass;
typedef Bse::TrackH (*BstTrackRollTrackFunc) (void *proxy_data, int row);


/* --- structures & typedefs --- */
typedef enum    /*< skip >*/
{
  BST_TRACK_ROLL_MARKER_NONE,
  BST_TRACK_ROLL_MARKER_POS,
  BST_TRACK_ROLL_MARKER_LOOP,
  BST_TRACK_ROLL_MARKER_SELECT
} BstTrackRollMarkerType;
struct BstTrackRollDrag {
  GXK_SCROLL_CANVAS_DRAG_FIELDS;
  uint          start_row;
  Bse::TrackH   start_track;
  uint          start_tick;
  bool          start_valid;
  uint          current_row;
  Bse::TrackH   current_track;
  uint          current_tick;
  bool          current_valid;
  // convenience:
  BstTrackRoll *troll;
  BstTrackRollDrag() : start_row (~uint (0)), start_tick (0), start_valid (0),
                       current_row (~uint (0)), current_tick (0), current_valid (0), troll (NULL) {}
};
struct _BstTrackRoll
{
  GxkScrollCanvas   parent_instance;

  Bse::SongS        song;
  GtkTreeView      *tree;
  guint             n_scopes;   /* does not always reflect number of rows */
  GtkWidget       **scopes;
  guint             scope_update;

  /* horizontal layout */
  guint          tpt;           /* ticks (parts) per tact */
  guint          max_ticks;
  gdouble        hzoom;
  guint          draw_tact_grid : 1;

  guint          prelight_row;
  guint          hpanel_height;

  /* editable popup */
  GtkCellEditable *ecell;
  guint            ecell_row;
  guint            ecell_tick;
  guint            ecell_duration;

  /* size queries */
  gint                  area_offset;

  /* BseTrack retrieval */
  gpointer              proxy_data;
  BstTrackRollTrackFunc get_track;

  /* last drag state */
  uint          start_row;
  Bse::TrackH   start_track;
  uint          start_tick;
  bool          start_valid;
};
struct _BstTrackRollClass
{
  GxkScrollCanvasClass parent_class;

  void          (*select_row)                   (BstTrackRoll     *troll,
                                                 gint              row);
  void          (*drag)                         (BstTrackRoll     *self,
                                                 BstTrackRollDrag *drag);
  void          (*clicked)                      (BstTrackRoll     *troll,
                                                 guint             button,
                                                 guint             row,
                                                 guint             tick_position,
                                                 GdkEvent         *event);
  void          (*stop_edit)                    (BstTrackRoll     *self,
                                                 gboolean          canceled,
                                                 GtkCellEditable  *ecell);
};


/* --- prototypes --- */
GType   bst_track_roll_get_type            (void);
void    bst_track_roll_setup               (BstTrackRoll *troll, GtkTreeView *tree, Bse::SongH song);
gdouble bst_track_roll_set_hzoom           (BstTrackRoll           *troll,
                                            gdouble                 hzoom);
void    bst_track_roll_set_track_callback  (BstTrackRoll           *self,
                                            gpointer                data,
                                            BstTrackRollTrackFunc   get_track);
void    bst_track_roll_check_update_scopes (BstTrackRoll           *self);
void    bst_track_roll_reselect            (BstTrackRoll           *self);
void    bst_track_roll_queue_row_change    (BstTrackRoll *self, int row);
void    bst_track_roll_set_prelight_row    (BstTrackRoll           *self,
                                            guint                   row);
void    bst_track_roll_start_edit          (BstTrackRoll           *self,
                                            guint                   row,
                                            guint                   tick,
                                            guint                   duration,
                                            GtkCellEditable        *ecell);
void    bst_track_roll_stop_edit           (BstTrackRoll           *self);
void    bst_track_roll_abort_edit          (BstTrackRoll           *self);
void    bst_track_roll_set_marker          (BstTrackRoll           *self,
                                            guint                   mark_index,
                                            guint                   position,
                                            BstTrackRollMarkerType  mtype);


#endif /* __BST_TRACK_ROLL_H__ */
