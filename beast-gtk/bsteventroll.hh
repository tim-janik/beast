// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_EVENT_ROLL_H__
#define __BST_EVENT_ROLL_H__
#include	"bstsegment.hh"
G_BEGIN_DECLS
/* --- type macros --- */
#define BST_TYPE_EVENT_ROLL              (bst_event_roll_get_type ())
#define BST_EVENT_ROLL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_EVENT_ROLL, BstEventRoll))
#define BST_EVENT_ROLL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_EVENT_ROLL, BstEventRollClass))
#define BST_IS_EVENT_ROLL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_EVENT_ROLL))
#define BST_IS_EVENT_ROLL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_EVENT_ROLL))
#define BST_EVENT_ROLL_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_EVENT_ROLL, BstEventRollClass))
/* --- typedefs & enums --- */
typedef struct _BstEventRoll        BstEventRoll;
typedef struct _BstEventRollClass   BstEventRollClass;
/* --- structures & typedefs --- */
typedef struct {
  GXK_SCROLL_CANVAS_DRAG_FIELDS;
  gint          tick_width;
  guint	        start_tick;
  gfloat        start_value;
  guint		start_valid : 1;
  guint         current_tick;
  gfloat        current_value;          /* between -1 and +1 if valid */
  gfloat        current_value_raw;
  guint		current_valid : 1;	/* value out of range */
  /* convenience: */
  BstEventRoll *eroll;
} BstEventRollDrag;
struct _BstEventRoll
{
  GxkScrollCanvas parent_instance;
  SfiProxy	 proxy;
  BseMidiSignalType control_type;
  GtkWidget     *child;
  /* horizontal layout */
  guint		 ppqn;		/* parts per quarter note */
  guint		 qnpt;		/* quarter notes per tact */
  guint		 max_ticks;	/* in ticks */
  gfloat	 hzoom;
  guint		 draw_qn_grid : 1;
  guint		 draw_qqn_grid : 1;
  /* drag data */
  guint		start_valid : 1;
  guint	        start_tick;
  gfloat        start_value;
  /* vpanel width sync */
  gint         (*fetch_vpanel_width) (gpointer data);
  gpointer       fetch_vpanel_width_data;
  /* line drawing */
  BstSegment     segment;
  /* selection rectangle */
  guint		 selection_tick;
  guint		 selection_duration;
  gint		 selection_min_note;
  gint		 selection_max_note;
};
struct _BstEventRollClass
{
  GxkScrollCanvasClass parent_class;
  void		(*canvas_drag)			(BstEventRoll	  *self,
						 BstEventRollDrag *drag);
  void		(*canvas_clicked)		(BstEventRoll	  *eroll,
						 guint		   button,
						 guint		   tick_position,
						 gfloat            value,
						 GdkEvent	  *event);
  void		(*vpanel_drag)			(BstEventRoll	  *self,
						 BstEventRollDrag *drag);
  void		(*vpanel_clicked)		(BstEventRoll	  *eroll,
						 guint		   button,
						 gfloat            value,
						 GdkEvent	  *event);
};
/* --- prototypes --- */
GType       bst_event_roll_get_type              (void);
void        bst_event_roll_set_proxy             (BstEventRoll   *self,
                                                  SfiProxy        proxy);
gfloat      bst_event_roll_set_hzoom             (BstEventRoll   *self,
                                                  gfloat          hzoom);
void        bst_event_roll_set_view_selection    (BstEventRoll   *self,
                                                  guint           tick,
                                                  guint           duration);
void        bst_event_roll_set_vpanel_width_hook (BstEventRoll   *self,
                                                  gint          (*fetch_vpanel_width) (gpointer data),
                                                  gpointer        data);
void        bst_event_roll_set_control_type      (BstEventRoll *self, BseMidiSignalType control_type);
void        bst_event_roll_init_segment          (BstEventRoll   *self,
                                                  BstSegmentType  type);
void        bst_event_roll_segment_start         (BstEventRoll   *self,
                                                  guint           tick,
                                                  gfloat          value);
void        bst_event_roll_segment_move_to       (BstEventRoll   *self,
                                                  guint           tick,
                                                  gfloat          value);
void        bst_event_roll_segment_tick_range    (BstEventRoll   *self,
                                                  guint          *tick,
                                                  guint          *duration);
gdouble     bst_event_roll_segment_value         (BstEventRoll   *self,
                                                  guint           tick);
void        bst_event_roll_clear_segment         (BstEventRoll   *self);
G_END_DECLS
#endif /* __BST_EVENT_ROLL_H__ */
