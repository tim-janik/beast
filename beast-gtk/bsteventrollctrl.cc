// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsteventrollctrl.hh"
#include "bstpianorollctrl.hh"
#include "sfi/private.hh"

#define CONTROL_TYPE(erctrl)    ((erctrl)->eroll->control_type)
#define QUANTIZATION(self)      ((self)->quant_rtools->action_id)
#define HAVE_OBJECT             (unsigned (1) << 31)

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
static Bse::PartControlSeq *clipboard_cseq = NULL;

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
bst_event_roll_controller_set_clipboard (const Bse::PartControlSeq &cseq)
{
  if (clipboard_cseq)
    delete clipboard_cseq;
  clipboard_cseq = cseq.size() ? new Bse::PartControlSeq (cseq) : NULL;
  if (clipboard_cseq)
    bst_piano_roll_controller_set_clipboard (NULL);
}

Bse::PartControlSeq*
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

  assert_return (BST_IS_EVENT_ROLL (eroll), NULL);
  assert_return (quant_rtools && canvas_rtools, NULL);

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
  assert_return (self != NULL, NULL);
  assert_return (self->ref_count >= 1, NULL);

  self->ref_count++;

  return self;
}

void
bst_event_roll_controller_unref (BstEventRollController *self)
{
  assert_return (self != NULL);
  assert_return (self->ref_count >= 1);

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
      return true;
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
  Bse::PartH part = self->eroll->part;
  Bse::PartControlSeq cseq;
  switch (action_id)
    {
      guint i;
    case ACTION_SELECT_ALL:
      part.select_controls (0, self->eroll->max_ticks, CONTROL_TYPE (self));
      break;
    case ACTION_SELECT_NONE:
      part.deselect_controls (0, self->eroll->max_ticks, CONTROL_TYPE (self));
      break;
    case ACTION_SELECT_INVERT:
      cseq = part.list_selected_controls (CONTROL_TYPE (self));
      part.select_controls (0, self->eroll->max_ticks, CONTROL_TYPE (self));
      for (i = 0; i < cseq.size(); i++)
        {
          const Bse::PartControl &pcontrol = cseq[i];
          part.deselect_event (pcontrol.id);
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
  GdkEvent *event = gtk_get_current_event ();
  if (bst_mouse_button_move (event))
    return BST_COMMON_ROLL_TOOL_MOVE;
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
  assert_return (self != NULL);

  Bse::PartH part = self->eroll->part;
  const Bse::PartControlSeq &cseq = part.list_selected_controls (CONTROL_TYPE (self));
  part.group_undo ("Clear Selection");
  for (size_t i = 0; i < cseq.size(); i++)
    {
      const Bse::PartControl &pctrl = cseq[i];
      part.delete_event (pctrl.id);
    }
  part.ungroup_undo();
}

void
bst_event_roll_controller_cut (BstEventRollController *self)
{
  assert_return (self != NULL);

  Bse::PartH part = self->eroll->part;
  const Bse::PartControlSeq &cseq = part.list_selected_controls (CONTROL_TYPE (self));
  part.group_undo ("Cut Selection");
  for (size_t i = 0; i < cseq.size(); i++)
    {
      const Bse::PartControl &pctrl = cseq[i];
      part.delete_event (pctrl.id);
    }
  bst_event_roll_controller_set_clipboard (cseq);
  part.ungroup_undo();
}

gboolean
bst_event_roll_controller_copy (BstEventRollController *self)
{
  assert_return (self != NULL, FALSE);

  Bse::PartH part = self->eroll->part;
  const Bse::PartControlSeq &cseq = part.list_selected_controls (CONTROL_TYPE (self));
  bst_event_roll_controller_set_clipboard (cseq);
  return cseq.size() > 0;
}

void
bst_event_roll_controller_paste (BstEventRollController *self)
{
  assert_return (self != NULL);

  Bse::PartH part = self->eroll->part;
  const Bse::PartControlSeq *cseq = bst_event_roll_controller_get_clipboard ();
  if (!cseq || cseq->size() <= 0)
    return;

  uint ptick, ctick = self->eroll->max_ticks;
  ptick = 100; // FIXME: bst_event_roll_get_paste_pos (self->eroll, &ptick);
  for (size_t i = 0; i < cseq->size(); i++)
    {
      const Bse::PartControl &pctrl = (*cseq)[i];
      ctick = MIN (ctick, uint (pctrl.tick));
    }
  part.group_undo ("Paste Clipboard");
  part.deselect_controls (0, self->eroll->max_ticks, CONTROL_TYPE (self));
  for (size_t i = 0; i < cseq->size(); i++)
    {
      const Bse::PartControl &pctrl = (*cseq)[i];
      uint id = part.insert_control (pctrl.tick - ctick + ptick, pctrl.control_type, pctrl.value);
      if (id)
        part.select_event (id);
    }
  part.ungroup_undo();
}

gboolean
bst_event_roll_controller_clipboard_full (BstEventRollController *self)
{
  const Bse::PartControlSeq *cseq = bst_event_roll_controller_get_clipboard ();
  return cseq && cseq->size();
}

gboolean
bst_event_roll_controller_has_selection (BstEventRollController *self,
                                         guint64                 action_stamp)
{
  if (self->cached_stamp != action_stamp)
    {
      Bse::PartH part = self->eroll->part;
      if (part)
        {
          self->cached_stamp = action_stamp;
          const Bse::PartControlSeq &cseq = part.list_selected_controls (CONTROL_TYPE (self));
          self->cached_n_controls = cseq.size();
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
  assert_return (self != NULL, fine_tick);

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
  Bse::PartH part = self->eroll->part;
  if (self->obj_id)	/* got control event to move */
    {
      controller_update_canvas_cursor (self, BST_COMMON_ROLL_TOOL_MOVE);
      gxk_status_set (GXK_STATUS_WAIT, _("Move Control Event"), NULL);
      drag->state = GXK_DRAG_CONTINUE;
      if (part.is_event_selected (self->obj_id))
	self->sel_cseq = part.list_selected_controls (CONTROL_TYPE (self));
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
  Bse::PartH part = self->eroll->part;
  gint new_tick, delta_tick;

  new_tick = bst_event_roll_controller_quantize (self, drag->current_tick);
  delta_tick = self->obj_tick;
  delta_tick -= new_tick;
  part.group_undo ("Move Selection");
  for (size_t i = 0; i < self->sel_cseq.size(); i++)
    {
      const Bse::PartControl &pctrl = self->sel_cseq[i];
      gint tick = pctrl.tick;
      part.change_control (pctrl.id, MAX (tick - delta_tick, 0), CONTROL_TYPE (self), pctrl.value);
    }
  if (drag->type == GXK_DRAG_DONE)
    self->sel_cseq.clear();
  part.ungroup_undo();
}

static void
move_motion (BstEventRollController *self,
	     BstEventRollDrag       *drag)
{
  Bse::PartH part = self->eroll->part;

  if (self->sel_cseq.size())
    {
      move_group_motion (self, drag);
      return;
    }

  uint new_tick = bst_event_roll_controller_quantize (self, drag->current_tick);
  if (new_tick != self->obj_tick)
    {
      const Bse::PartControlSeq &cseq = part.get_controls (new_tick, CONTROL_TYPE (self));
      if (!cseq.size()) // avoid overlap
        {
          if (part.change_control (self->obj_id, new_tick, CONTROL_TYPE (self), self->obj_value) != Bse::Error::NONE)
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
  self->sel_cseq.clear();
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
      Bse::PartH part = self->eroll->part;
      guint tick, duration, i;

      part.group_undo ("Align Control Events");
      bst_event_roll_segment_tick_range (self->eroll, &tick, &duration);
      const Bse::PartControlSeq &cseq = part.list_controls (tick, duration, CONTROL_TYPE (self));
      for (i = 0; i < cseq.size(); i++)
        {
          const Bse::PartControl &pctrl = cseq[i];
          gdouble v = bst_event_roll_segment_value (self->eroll, pctrl.tick);
          part.change_control (pctrl.id, pctrl.tick, CONTROL_TYPE (self), v);
        }
      bst_event_roll_clear_segment (self->eroll);
      part.ungroup_undo();
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
  Bse::PartH part = self->eroll->part;
  Bse::Error error = Bse::Error::INVALID_OVERLAP;
  if (!self->obj_id && drag->start_valid)
    {
      guint qtick = bst_event_roll_controller_quantize (self, drag->start_tick);
      self->obj_id = part.insert_control (qtick, CONTROL_TYPE (self), drag->current_value);
      if (self->obj_id)
        {
          self->obj_tick = qtick;
          self->obj_value = drag->current_value;
          error = Bse::Error::NONE;
        }
      else
        error = Bse::Error::NO_TARGET;
    }
  else if (!self->obj_id)
    error = Bse::Error::NO_TARGET;
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
  Bse::PartH part = self->eroll->part;
  /* apply new control event size */
  if (drag->current_value != self->obj_value)
    {
      part.group_undo ("Resize Control Event");
      self->obj_value = drag->current_value;
      if (part.change_control (self->obj_id, self->obj_tick, CONTROL_TYPE (self), self->obj_value) != Bse::Error::NONE)
        drag->state = GXK_DRAG_ERROR;
      part.ungroup_undo();
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
  Bse::PartH part = self->eroll->part;
  if (self->obj_id)	/* got control event to delete */
    {
      Bse::Error error = part.delete_event (self->obj_id);
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
  Bse::PartH part = self->eroll->part;
  guint start_tick = MIN (drag->start_tick, drag->current_tick);
  guint end_tick = MAX (drag->start_tick, drag->current_tick);

  bst_event_roll_set_view_selection (drag->eroll, start_tick, end_tick - start_tick);
  if (drag->type == GXK_DRAG_DONE)
    {
      part.select_controls_exclusive (start_tick, end_tick - start_tick, CONTROL_TYPE (self));
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

  // printerr ("canvas drag event, tick=%d (valid=%d) value=%f", drag->current_tick, drag->current_valid, drag->current_value);

  if (drag->type == GXK_DRAG_START)
    {
      Bse::PartH part = self->eroll->part;
      BstCommonRollTool tool = BST_COMMON_ROLL_TOOL_NONE;
      ssize_t j, i = drag->start_tick;
      const Bse::PartControl *nearest = NULL;
      gboolean retry_quantized = TRUE;

      /* setup drag data */
    retry_quantized:
      j = i;
      i -= drag->tick_width;
      j += drag->tick_width;
      i = MAX (i, 0);
      const Bse::PartControlSeq &cseq = part.list_controls (i, j - i + 1, CONTROL_TYPE (self));
      j = SFI_MAXINT;
      for (i = 0; i < (ssize_t) cseq.size(); i++)
        {
          int d = MAX (cseq[i].tick, (int) drag->start_tick) - MIN (cseq[i].tick, (int) drag->start_tick);
          if (d < j)
            {
              j = d;
              nearest = &cseq[i];
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
      if (self->sel_cseq.size())
	Bse::warning ("leaking old drag selection (%lu)", self->sel_cseq.size());
      self->sel_cseq.clear();

      /* find drag tool */
      tool = event_canvas_button_tool (self, drag->button, self->obj_id > 0);
      for (i = 0; size_t (i) < G_N_ELEMENTS (tool_table); i++)
	if (tool_table[i].tool == tool)
	  break;
      self->tool_index = i;
      if (size_t (i) >= G_N_ELEMENTS (tool_table))
	return;		/* unhandled */
    }
  size_t i = self->tool_index;
  assert_return (i < G_N_ELEMENTS (tool_table));
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
  // printerr ("vpanel drag event, tick=%d (valid=%d) value=%f", drag->current_tick, drag->current_valid, drag->current_value);

  if (drag->type == GXK_DRAG_START ||
      drag->type == GXK_DRAG_MOTION)
    drag->state = GXK_DRAG_CONTINUE;
}
