// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_TRACK_ROLL_CONTROLLER_H__
#define __BST_TRACK_ROLL_CONTROLLER_H__


#include "bsttrackroll.hh"

typedef struct _BstTrackRollUtil BstTrackRollUtil;
typedef struct {
  /* misc data */
  guint		    ref_count;
  BstTrackRoll	   *troll;
  SfiProxy	    song;
  guint		    note_length;
  /* drag data */
  Bse::TrackH       obj_track;
  Bse::PartH        obj_part;
  int		    obj_tick, obj_duration;
  guint		    xoffset;
  guint		    tick_bound;
  /* tool data */
  BstTrackRollUtil *current_tool;
  guint		    skip_deletion : 1;
  /* tool selections */
  GxkActionGroup   *canvas_rtools;
  GxkActionGroup   *hpanel_rtools;
  GxkActionGroup   *quant_rtools;
} BstTrackRollController;


/* --- API --- */
BstTrackRollController*	bst_track_roll_controller_new		(BstTrackRoll		*troll);
BstTrackRollController*	bst_track_roll_controller_ref		(BstTrackRollController	*self);
void			bst_track_roll_controller_unref		(BstTrackRollController	*self);
void		bst_track_roll_controller_set_song		(BstTrackRollController	*self,
								 SfiProxy		 song);
void		bst_track_roll_controller_set_quantization	(BstTrackRollController *self,
								 BstQuantizationType     quantization);
guint		bst_track_roll_controller_quantize		(BstTrackRollController *self,
								 guint                   fine_tick);
GxkActionList*  bst_track_roll_controller_canvas_actions        (BstTrackRollController *self);
GxkActionList*  bst_track_roll_controller_hpanel_actions        (BstTrackRollController *self);
GxkActionList*  bst_track_roll_controller_quant_actions         (BstTrackRollController *self);


#endif /* __BST_TRACK_ROLL_CONTROLLER_H__ */
