// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsteventrollctrl.hh"
#include "bstpianorollctrl.hh"


#define CONTROL_TYPE(erctrl)    ((erctrl)->eroll->control_type)
#define QUANTIZATION(self)      ((self)->quant_rtools->action_id)
#define HAVE_OBJECT             (1 << 31)


/* --- prototypes --- */
static gboolean bst_event_roll_controller_check_action  (BstEventRollController *self,
                                                         gulong                  action_id,
                                                         guint64                 action_stamp);
static void     bst_event_roll_controller_exec_action   (BstEventRollController *self,
                                                         gulong                  action_id);
static void	controller_canvas_drag		        (BstEventRollController	*self,
                                                         BstEventRollDrag	*drag);
static void	controller_vpanel_drag		        (BstEventRollController	*self,
                                                         BstEventRollDrag	*drag);
static void	controller_update_canvas_cursor	        (BstEventRollController *self,
                                                         BstCommonRollTool	 tool);


/* --- variables --- */
static BsePartControlSeq *clipboard_cseq = NULL;

/* --- actions --- */
enum {
  ACTION_NONE           = BST_COMMON_ROLL_TOOL_LAST,
  ACTION_SELECT_ALL,
  ACTION_SELECT_NONE,
  ACTION_SELECT_INVERT,
};

/* --- functions --- */
GxkActionList*
bst_event_roll_controller_select_actions (BstEventRollController *self)
{
  GxkActionList *alist = gxk_action_list_create ();
  static const GxkStockAction actions[] = {
    { N_("All"),                "",     N_("Select all events"),
      ACTION_SELECT_ALL,        BST_STOCK_SELECT_ALL },
    { N_("None"),               "",     N_("Unselect all events"),
      ACTION_SELECT_NONE,       BST_STOCK_SELECT_NONE },
    { N_("Invert"),             "",     N_("Invert the current selection"),
      ACTION_SELECT_INVERT,     BST_STOCK_SELECT_INVERT },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/,
                               (GxkActionCheck) bst_event_roll_controller_check_action,
                               (GxkActionExec) bst_event_roll_controller_exec_action,
                               self);
  return alist;
}

void
bst_event_roll_controller_set_clipboard (BsePartControlSeq *cseq)
{
  if (clipboard_cseq)
    bse_part_control_seq_free (clipboard_cseq);
  clipboard_cseq = cseq && cseq->n_pcontrols ? bse_part_control_seq_copy_shallow (cseq) : NULL;
  if (clipboard_cseq)
    bst_piano_roll_controller_set_clipboard (NULL);
}

BsePartControlSeq*
bst_event_roll_controller_get_clipboard (void)
{
  return clipboard_cseq;
}

static void
controller_reset_canvas_cursor (BstEventRollController *self)
{
  controller_update_canvas_cursor (self, BstCommonRollTool (self->canvas_rtools->action_id));
}

BstEventRollController*
bst_event_roll_controller_new (BstEventRoll   *eroll,
                               GxkActionGroup *quant_rtools,
                               GxkActionGroup *canvas_rtools)
{
  BstEventRollController *self;

  g_return_val_if_fail (BST_IS_EVENT_ROLL (eroll), NULL);
  g_return_val_if_fail (quant_rtools && canvas_rtools, NULL);

  self = g_new0 (BstEventRollController, 1);
  self->eroll = eroll;
  self->ref_count = 1;

  self->ref_count++;
  g_signal_connect_data (eroll, "canvas-drag",
			 G_CALLBACK (controller_canvas_drag),
			 self, (GClosureNotify) bst_event_roll_controller_unref,
			 G_CONNECT_SWAPPED);
  g_signal_connect_data (eroll, "vpanel-drag",
			 G_CALLBACK (controller_vpanel_drag),
			 self, NULL,
			 G_CONNECT_SWAPPED);
  self->quant_rtools = (GxkActionGroup*) g_object_ref (quant_rtools);
  self->canvas_rtools = (GxkActionGroup*) g_object_ref (canvas_rtools);
  /* update from atools */
  g_object_connect (self->canvas_rtools,
                    "swapped_signal::changed", controller_reset_canvas_cursor, self,
                    NULL);
  controller_reset_canvas_cursor (self);

  return self;
}

BstEventRollController*
bst_event_roll_controller_ref (BstEventRollController *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ref_count >= 1, NULL);

  self->ref_count++;

  return self;
}

void
bst_event_roll_controller_unref (BstEventRollController *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count >= 1);

  self->ref_count--;
  if (!self->ref_count)
    {
      gxk_action_group_dispose (self->canvas_rtools);
      g_object_unref (self->canvas_rtools);
      gxk_action_group_dispose (self->quant_rtools);
      g_object_unref (self->quant_rtools);
      g_free (self);
    }
}

static gboolean
bst_event_roll_controller_check_action (BstEventRollController *self,
                                        gulong                  action_id,
                                        guint64                 action_stamp)
{
  switch (action_id)
    {
    case ACTION_SELECT_ALL:
      return TRUE;
    case ACTION_SELECT_NONE:
    case ACTION_SELECT_INVERT:
      return bst_event_roll_controller_has_selection (self, action_stamp);
    }
  return FALSE;
}

static void
bst_event_roll_controller_exec_action (BstEventRollController *self,
                                       gulong                  action_id)
{
  SfiProxy part = self->eroll->proxy;
  switch (action_id)
    {
      BsePartControlSeq *cseq;
      guint i;
    case ACTION_SELECT_ALL:
      bse_part_select_controls (part, 0, self->eroll->max_ticks, CONTROL_TYPE (self));
      break;
    case ACTION_SELECT_NONE:
      bse_part_deselect_controls (part, 0, self->eroll->max_ticks, CONTROL_TYPE (self));
      break;
    case ACTION_SELECT_INVERT:
      cseq = bse_part_list_selected_controls (part, CONTROL_TYPE (self));
      bse_part_select_controls (part, 0, self->eroll->max_ticks, CONTROL_TYPE (self));
      for (i = 0; i < cseq->n_pcontrols; i++)
        {
          BsePartControl *pcontrol = cseq->pcontrols[i];
          bse_part_deselect_event (part, pcontrol->id);
        }
      break;
    }
  gxk_widget_update_actions_downwards (self->eroll);
}

static BstCommonRollTool
event_canvas_button_tool (BstEventRollController *self,
                          guint                   button,
                          guint                   have_object)
{
  switch (self->canvas_rtools->action_id | /* user selected tool */
          (have_object ? HAVE_OBJECT : 0))
    {
    case BST_COMMON_ROLL_TOOL_INSERT: /* background */
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_INSERT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_INSERT | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_RESIZE;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_DELETE: /* background */
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_DELETE;       /* user error */
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_DELETE | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_DELETE;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_ALIGN: /* background */
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_ALIGN;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_ALIGN | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_ALIGN;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_SELECT: /* background */
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_SELECT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_SELECT | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_SELECT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_VSELECT: /* background */
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_SELECT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_VSELECT | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_SELECT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    }
  return BST_COMMON_ROLL_TOOL_NONE;
}

void
bst_event_roll_controller_clear (BstEventRollController *self)
{
  BsePartControlSeq *cseq;
  SfiProxy proxy;
  guint i;

  g_return_if_fail (self != NULL);

  proxy = self->eroll->proxy;
  cseq = bse_part_list_selected_controls (proxy, CONTROL_TYPE (self));
  bse_item_group_undo (proxy, "Clear Selection");
  for (i = 0; i < cseq->n_pcontrols; i++)
    {
      BsePartControl *pctrl = cseq->pcontrols[i];
      bse_part_delete_event (proxy, pctrl->id);
    }
  bse_item_ungroup_undo (proxy);
}

void
bst_event_roll_controller_cut (BstEventRollController *self)
{
  BsePartControlSeq *cseq;
  SfiProxy proxy;
  guint i;

  g_return_if_fail (self != NULL);

  proxy = self->eroll->proxy;
  cseq = bse_part_list_selected_controls (proxy, CONTROL_TYPE (self));
  bse_item_group_undo (proxy, "Cut Selection");
  for (i = 0; i < cseq->n_pcontrols; i++)
    {
      BsePartControl *pctrl = cseq->pcontrols[i];
      bse_part_delete_event (proxy, pctrl->id);
    }
  bst_event_roll_controller_set_clipboard (cseq);
  bse_item_ungroup_undo (proxy);
}

gboolean
bst_event_roll_controller_copy (BstEventRollController *self)
{
  BsePartControlSeq *cseq;
  SfiProxy proxy;

  g_return_val_if_fail (self != NULL, FALSE);

  proxy = self->eroll->proxy;
  cseq = bse_part_list_selected_controls (proxy, CONTROL_TYPE (self));
  bst_event_roll_controller_set_clipboard (cseq);
  return cseq && cseq->n_pcontrols;
}

void
bst_event_roll_controller_paste (BstEventRollController *self)
{
  BsePartControlSeq *cseq;
  SfiProxy proxy;

  g_return_if_fail (self != NULL);

  proxy = self->eroll->proxy;
  cseq = bst_event_roll_controller_get_clipboard ();
  if (cseq)
    {
      guint i, ptick, ctick = self->eroll->max_ticks;
      ptick = 100; // FIXME: bst_event_roll_get_paste_pos (self->eroll, &ptick);
      for (i = 0; i < cseq->n_pcontrols; i++)
	{
	  BsePartControl *pctrl = cseq->pcontrols[i];
	  ctick = MIN (ctick, pctrl->tick);
	}
      bse_item_group_undo (proxy, "Paste Clipboard");
      bse_part_deselect_controls (proxy, 0, self->eroll->max_ticks, CONTROL_TYPE (self));
      for (i = 0; i < cseq->n_pcontrols; i++)
	{
	  BsePartControl *pctrl = cseq->pcontrols[i];
	  guint id = bse_part_insert_control (proxy,
                                              pctrl->tick - ctick + ptick,
                                              pctrl->control_type,
                                              pctrl->value);
          if (id)
            bse_part_select_event (proxy, id);
	}
      bse_item_ungroup_undo (proxy);
    }
}

gboolean
bst_event_roll_controller_clipboard_full (BstEventRollController *self)
{
  BsePartControlSeq *cseq = bst_event_roll_controller_get_clipboard ();
  return cseq && cseq->n_pcontrols;
}

gboolean
bst_event_roll_controller_has_selection (BstEventRollController *self,
                                         guint64                 action_stamp)
{
  if (self->cached_stamp != action_stamp)
    {
      SfiProxy part = self->eroll->proxy;
      if (part)
        {
          self->cached_stamp = action_stamp;
          BsePartControlSeq *cseq = bse_part_list_selected_controls (part, CONTROL_TYPE (self));
          self->cached_n_controls = cseq->n_pcontrols;
        }
      else
        self->cached_n_controls = 0;
    }
  return self->cached_n_controls > 0;
}

guint
bst_event_roll_controller_quantize (BstEventRollController *self,
                                    guint                   fine_tick)
{
  g_return_val_if_fail (self != NULL, fine_tick);

  /* quantize tick */
  if (QUANTIZATION (self) && self->eroll)
    {
      guint quant = self->eroll->ppqn * 4 / QUANTIZATION (self);
      guint qtick = fine_tick / quant;
      qtick *= quant;
      if (fine_tick - qtick > quant / 2 &&
          qtick + quant > fine_tick)
        fine_tick = qtick + quant;
      else
        fine_tick = qtick;
    }
  return fine_tick;
}

static void
controller_update_canvas_cursor (BstEventRollController *self,
                                 BstCommonRollTool      tool)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self->eroll);
  switch (tool)
    {
    case BST_COMMON_ROLL_TOOL_INSERT:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_PENCIL);
      break;
    case BST_COMMON_ROLL_TOOL_RESIZE:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_SB_V_DOUBLE_ARROW);
      break;
    case BST_COMMON_ROLL_TOOL_MOVE:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_FLEUR);
      break;
    case BST_COMMON_ROLL_TOOL_DELETE:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_TARGET);
      break;
    case BST_COMMON_ROLL_TOOL_SELECT:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_CROSSHAIR);
      break;
    default:
      gxk_scroll_canvas_set_canvas_cursor (scc, GXK_DEFAULT_CURSOR);
      break;
    }
}

static void
move_start (BstEventRollController *self,
	    BstEventRollDrag       *drag)
{
  SfiProxy part = self->eroll->proxy;
  if (self->obj_id)	/* got control event to move */
    {
      controller_update_canvas_cursor (self, BST_COMMON_ROLL_TOOL_MOVE);
      gxk_status_set (GXK_STATUS_WAIT, _("Move Control Event"), NULL);
      drag->state = GXK_DRAG_CONTINUE;
      if (bse_part_is_event_selected (part, self->obj_id))
	self->sel_cseq = bse_part_control_seq_copy_shallow (bse_part_list_selected_controls (part, CONTROL_TYPE (self)));
    }
  else
    {
      gxk_status_set (GXK_STATUS_ERROR, _("Move Control Event"), _("No target"));
      drag->state = GXK_DRAG_HANDLED;
    }
}

static void
move_group_motion (BstEventRollController *self,
		   BstEventRollDrag       *drag)
{
  SfiProxy part = self->eroll->proxy;
  gint i, new_tick, delta_tick;

  new_tick = bst_event_roll_controller_quantize (self, drag->current_tick);
  delta_tick = self->obj_tick;
  delta_tick -= new_tick;
  bse_item_group_undo (part, "Move Selection");
  for (i = 0; i < self->sel_cseq->n_pcontrols; i++)
    {
      BsePartControl *pctrl = self->sel_cseq->pcontrols[i];
      gint tick = pctrl->tick;
      bse_part_change_control (part, pctrl->id,
                               MAX (tick - delta_tick, 0),
                               CONTROL_TYPE (self),
                               pctrl->value);
    }
  if (drag->type == GXK_DRAG_DONE)
    {
      bse_part_control_seq_free (self->sel_cseq);
      self->sel_cseq = NULL;
    }
  bse_item_ungroup_undo (part);
}

static void
move_motion (BstEventRollController *self,
	     BstEventRollDrag       *drag)
{
  SfiProxy part = self->eroll->proxy;
  gint new_tick;

  if (self->sel_cseq)
    {
      move_group_motion (self, drag);
      return;
    }

  new_tick = bst_event_roll_controller_quantize (self, drag->current_tick);
  if (new_tick != self->obj_tick)
    {
      BsePartControlSeq *cseq = bse_part_get_controls (part, new_tick, CONTROL_TYPE (self));
      if (!cseq->n_pcontrols)    /* avoid overlap */
        {
          if (bse_part_change_control (part, self->obj_id, new_tick, CONTROL_TYPE (self), self->obj_value) != BSE_ERROR_NONE)
            drag->state = GXK_DRAG_ERROR;
          else
            self->obj_tick = new_tick;
        }
    }
}

static void
move_abort (BstEventRollController *self,
	    BstEventRollDrag       *drag)
{
  if (self->sel_cseq)
    {
      bse_part_control_seq_free (self->sel_cseq);
      self->sel_cseq = NULL;
    }
  gxk_status_set (GXK_STATUS_ERROR, _("Move Control Event"), _("Lost Event"));
}

static void
align_start (BstEventRollController *self,
             BstEventRollDrag       *drag)
{
  bst_event_roll_init_segment (self->eroll, BST_SEGMENT_LINE);
  bst_event_roll_segment_start (self->eroll, drag->start_tick, drag->start_value);
  drag->state = GXK_DRAG_CONTINUE;
  gxk_status_set (GXK_STATUS_WAIT, _("Align Control Events"), NULL);
}

static void
align_motion (BstEventRollController *self,
              BstEventRollDrag       *drag)
{
  bst_event_roll_segment_move_to (self->eroll, drag->current_tick, drag->current_value_raw);
  if (drag->type == GXK_DRAG_DONE)
    {
      SfiProxy part = self->eroll->proxy;
      guint tick, duration, i;
      BsePartControlSeq *cseq;

      bse_item_group_undo (part, "Align Control Events");
      bst_event_roll_segment_tick_range (self->eroll, &tick, &duration);
      cseq = bse_part_list_controls (part, tick, duration, CONTROL_TYPE (self));
      for (i = 0; i < cseq->n_pcontrols; i++)
        {
          BsePartControl *pctrl = cseq->pcontrols[i];
          gdouble v = bst_event_roll_segment_value (self->eroll, pctrl->tick);
          bse_part_change_control (part, pctrl->id, pctrl->tick, CONTROL_TYPE (self), v);
        }
      bst_event_roll_clear_segment (self->eroll);
      bse_item_ungroup_undo (part);
    }
}

static void
align_abort (BstEventRollController *self,
             BstEventRollDrag       *drag)
{
  bst_event_roll_clear_segment (self->eroll);
  gxk_status_set (GXK_STATUS_ERROR, _("Align Control Events"), _("Aborted"));
}

static void
insert_start (BstEventRollController *self,
	      BstEventRollDrag       *drag)
{
  SfiProxy part = self->eroll->proxy;
  BseErrorType error = BSE_ERROR_INVALID_OVERLAP;
  if (!self->obj_id && drag->start_valid)
    {
      guint qtick = bst_event_roll_controller_quantize (self, drag->start_tick);
      self->obj_id = bse_part_insert_control (part, qtick, CONTROL_TYPE (self), drag->current_value);
      if (self->obj_id)
        {
          self->obj_tick = qtick;
          self->obj_value = drag->current_value;
          error = BSE_ERROR_NONE;
        }
      else
        error = BSE_ERROR_NO_TARGET;
    }
  else if (!self->obj_id)
    error = BSE_ERROR_NO_TARGET;
  else /* no insertion */
    self->obj_id = 0;
  bst_status_eprintf (error, _("Insert Control Event"));
  drag->state = GXK_DRAG_HANDLED;
}

static void
resize_start (BstEventRollController *self,
	      BstEventRollDrag       *drag)
{
  if (self->obj_id)	/* got control event for resize */
    {
      controller_update_canvas_cursor (self, BST_COMMON_ROLL_TOOL_RESIZE);
      gxk_status_set (GXK_STATUS_WAIT, _("Resize Control Event"), NULL);
      drag->state = GXK_DRAG_CONTINUE;
    }
  else
    {
      gxk_status_set (GXK_STATUS_ERROR, _("Resize Control Event"), _("No target"));
      drag->state = GXK_DRAG_HANDLED;
    }
}

static void
insert_resize_start (BstEventRollController *self,
                     BstEventRollDrag       *drag)
{
  insert_start (self, drag);
  if (self->obj_id)
    resize_start (self, drag);
}

static void
resize_motion (BstEventRollController *self,
	       BstEventRollDrag       *drag)
{
  SfiProxy part = self->eroll->proxy;

  /* apply new control event size */
  if (drag->current_value != self->obj_value)
    {
      bse_item_group_undo (part, "Resize Control Event");
      self->obj_value = drag->current_value;
      if (bse_part_change_control (part, self->obj_id, self->obj_tick, CONTROL_TYPE (self),
                                   self->obj_value) != BSE_ERROR_NONE)
        drag->state = GXK_DRAG_ERROR;
      bse_item_ungroup_undo (part);
    }
}

static void
resize_abort (BstEventRollController *self,
	      BstEventRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Resize Control Event"), _("Lost Event"));
}

static void
delete_start (BstEventRollController *self,
	      BstEventRollDrag       *drag)
{
  SfiProxy part = self->eroll->proxy;
  if (self->obj_id)	/* got control event to delete */
    {
      BseErrorType error = bse_part_delete_event (part, self->obj_id);
      bst_status_eprintf (error, _("Delete Control Event"));
    }
  else
    gxk_status_set (GXK_STATUS_ERROR, _("Delete Control Event"), _("No target"));
  drag->state = GXK_DRAG_HANDLED;
}

static void
select_start (BstEventRollController *self,
	      BstEventRollDrag       *drag)
{
  drag->start_tick = bst_event_roll_controller_quantize (self, drag->start_tick);
  bst_event_roll_set_view_selection (drag->eroll, drag->start_tick, 0);
  gxk_status_set (GXK_STATUS_WAIT, _("Select Region"), NULL);
  drag->state = GXK_DRAG_CONTINUE;
}

static void
select_motion (BstEventRollController *self,
	       BstEventRollDrag       *drag)
{
  SfiProxy part = self->eroll->proxy;
  guint start_tick = MIN (drag->start_tick, drag->current_tick);
  guint end_tick = MAX (drag->start_tick, drag->current_tick);

  bst_event_roll_set_view_selection (drag->eroll, start_tick, end_tick - start_tick);
  if (drag->type == GXK_DRAG_DONE)
    {
      bse_part_select_controls_exclusive (part, start_tick, end_tick - start_tick, CONTROL_TYPE (self));
      bst_event_roll_set_view_selection (drag->eroll, 0, 0);
    }
}

static void
select_abort (BstEventRollController *self,
	      BstEventRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Select Region"), _("Aborted"));
  bst_event_roll_set_view_selection (drag->eroll, 0, 0);
}

#if 0
static void
generic_abort (BstEventRollController *self,
	       BstEventRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Abortion"), NULL);
}
#endif

typedef void (*DragFunc) (BstEventRollController *,
			  BstEventRollDrag       *);

void
controller_canvas_drag (BstEventRollController *self,
			BstEventRollDrag       *drag)
{
  static struct {
    BstCommonRollTool tool;
    DragFunc start, motion, abort;
  } tool_table[] = {
    { BST_COMMON_ROLL_TOOL_INSERT,     insert_resize_start,	resize_motion,  resize_abort,	},
    { BST_COMMON_ROLL_TOOL_ALIGN,	align_start,	        align_motion,	align_abort,	},
    { BST_COMMON_ROLL_TOOL_RESIZE,	resize_start,	        resize_motion,	resize_abort,	},
    { BST_COMMON_ROLL_TOOL_MOVE,	move_start,	        move_motion,	move_abort,	},
    { BST_COMMON_ROLL_TOOL_DELETE,	delete_start,	        NULL,		NULL,		},
    { BST_COMMON_ROLL_TOOL_SELECT,	select_start,	        select_motion,	select_abort,	},
  };
  guint i;

  // g_printerr ("canvas drag event, tick=%d (valid=%d) value=%f", drag->current_tick, drag->current_valid, drag->current_value);

  if (drag->type == GXK_DRAG_START)
    {
      BstCommonRollTool tool = BST_COMMON_ROLL_TOOL_NONE;
      BsePartControlSeq *cseq;
      gint j, i = drag->start_tick;
      BsePartControl *nearest = NULL;
      gboolean retry_quantized = TRUE;

      /* setup drag data */
    retry_quantized:
      j = i;
      i -= drag->tick_width;
      j += drag->tick_width;
      i = MAX (i, 0);
      cseq = bse_part_list_controls (drag->eroll->proxy, i, j - i + 1, CONTROL_TYPE (self));
      j = SFI_MAXINT;
      for (i = 0; i < cseq->n_pcontrols; i++)
        {
          gint d = MAX (cseq->pcontrols[i]->tick, drag->start_tick) -
                   MIN (cseq->pcontrols[i]->tick, drag->start_tick);
          if (d < j)
            {
              j = d;
              nearest = cseq->pcontrols[i];
            }
        }
      if (!nearest && retry_quantized--)
        {
          i = bst_event_roll_controller_quantize (self, drag->start_tick);
          goto retry_quantized;
        }
      if (nearest)
	{
	  self->obj_id = nearest->id;
	  self->obj_tick = nearest->tick;
	  self->obj_value = nearest->value;
	}
      else
	{
	  self->obj_id = 0;
	  self->obj_tick = 0;
          self->obj_value = 0;
	}
      if (self->sel_cseq)
	g_warning ("leaking old drag selection (%p)", self->sel_cseq);
      self->sel_cseq = NULL;

      /* find drag tool */
      tool = event_canvas_button_tool (self, drag->button, self->obj_id > 0);
      for (i = 0; i < G_N_ELEMENTS (tool_table); i++)
	if (tool_table[i].tool == tool)
	  break;
      self->tool_index = i;
      if (i >= G_N_ELEMENTS (tool_table))
	return;		/* unhandled */
    }
  i = self->tool_index;
  g_return_if_fail (i < G_N_ELEMENTS (tool_table));
  switch (drag->type)
    {
    case GXK_DRAG_START:
      if (tool_table[i].start)
	tool_table[i].start (self, drag);
      break;
    case GXK_DRAG_MOTION:
    case GXK_DRAG_DONE:
      if (tool_table[i].motion)
	tool_table[i].motion (self, drag);
      break;
    case GXK_DRAG_ABORT:
      if (tool_table[i].abort)
	tool_table[i].abort (self, drag);
      break;
    }
  if (drag->type == GXK_DRAG_DONE ||
      drag->type == GXK_DRAG_ABORT)
    controller_reset_canvas_cursor (self);
}

void
controller_vpanel_drag (BstEventRollController *self,
                        BstEventRollDrag       *drag)
{
  // g_printerr ("vpanel drag event, tick=%d (valid=%d) value=%f", drag->current_tick, drag->current_valid, drag->current_value);

  if (drag->type == GXK_DRAG_START ||
      drag->type == GXK_DRAG_MOTION)
    drag->state = GXK_DRAG_CONTINUE;
}
