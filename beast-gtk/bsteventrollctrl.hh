// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_EVENT_ROLL_CONTROLLER_H__
#define __BST_EVENT_ROLL_CONTROLLER_H__
#include "bsteventroll.hh"
G_BEGIN_DECLS
typedef struct {
  /* misc data */
  guint		     ref_count;
  BstEventRoll	    *eroll;
  /* drag data */
  guint		     obj_id, obj_tick;
  gfloat             obj_value;
  BsePartControlSeq *sel_cseq;
  BstSegment        *segment;
  /* tool data */
  guint		     tool_index;
  /* tool selections */
  GxkActionGroup  *quant_rtools;
  GxkActionGroup  *canvas_rtools;
  /* action cache */
  guint64          cached_stamp;
  guint            cached_n_controls;
} BstEventRollController;
/* --- API --- */
BstEventRollController*	bst_event_roll_controller_new		 (BstEventRoll		        *eroll,
                                                                  GxkActionGroup                *quant_rtools,
                                                                  GxkActionGroup                *canvas_rtools);
BstEventRollController*	bst_event_roll_controller_ref		 (BstEventRollController	*self);
void			bst_event_roll_controller_unref		 (BstEventRollController	*self);
guint                   bst_event_roll_controller_quantize       (BstEventRollController        *self,
                                                                  guint                          fine_tick);
GxkActionList*          bst_event_roll_controller_select_actions (BstEventRollController        *self);
void			bst_event_roll_controller_set_clipboard  (BsePartControlSeq	        *cseq);
BsePartControlSeq*	bst_event_roll_controller_get_clipboard	 (void);
void			bst_event_roll_controller_clear		 (BstEventRollController	*self);
void			bst_event_roll_controller_cut		 (BstEventRollController	*self);
gboolean		bst_event_roll_controller_copy		 (BstEventRollController	*self);
void			bst_event_roll_controller_paste		 (BstEventRollController        *self);
gboolean                bst_event_roll_controller_clipboard_full (BstEventRollController        *self);
gboolean                bst_event_roll_controller_has_selection  (BstEventRollController        *self,
                                                                  guint64                        action_stamp);
G_END_DECLS
#endif /* __BST_EVENT_ROLL_CONTROLLER_H__ */
