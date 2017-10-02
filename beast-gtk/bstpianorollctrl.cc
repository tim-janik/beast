// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstpianorollctrl.hh"
#include "bsteventrollctrl.hh"


#define NOTE_LENGTH(self)       ((self)->note_rtools->action_id)
#define QUANTIZATION(self)      ((self)->quant_rtools->action_id)
#define HAVE_OBJECT             (unsigned (1) << 31)


/* --- prototypes --- */
static gboolean bst_piano_roll_controller_check_action  (BstPianoRollController *self,
                                                         gulong                  action_id,
                                                         guint64                 action_stamp);
static void     bst_piano_roll_controller_exec_action   (BstPianoRollController *self,
                                                         gulong                  action_id);
static void	controller_canvas_drag		        (BstPianoRollController	*self,
                                                         BstPianoRollDrag	*drag);
static void	controller_piano_drag		        (BstPianoRollController	*self,
                                                         BstPianoRollDrag	*drag);
static void	controller_update_canvas_cursor	        (BstPianoRollController *self,
                                                         BstCommonRollTool	 tool);


/* --- variables --- */
static Bse::PartNoteSeq *clipboard_pseq = NULL;

/* --- actions --- */
enum {
  ACTION_NONE           = BST_COMMON_ROLL_TOOL_LAST,
  ACTION_SELECT_ALL,
  ACTION_SELECT_NONE,
  ACTION_SELECT_INVERT,
};

/* --- functions --- */
GxkActionList*
bst_piano_roll_controller_select_actions (BstPianoRollController *self)
{
  GxkActionList *alist = gxk_action_list_create ();
  static const GxkStockAction actions[] = {
    { N_("All"),                "<ctrl>A",              N_("Select all notes"),
      ACTION_SELECT_ALL,        BST_STOCK_SELECT_ALL },
    { N_("None"),               "<shift><ctrl>A",       N_("Unselect all notes"),
      ACTION_SELECT_NONE,       BST_STOCK_SELECT_NONE },
    { N_("Invert"),             "<ctrl>I",              N_("Invert the current selection"),
      ACTION_SELECT_INVERT,     BST_STOCK_SELECT_INVERT },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/,
                               (GxkActionCheck) bst_piano_roll_controller_check_action,
                               (GxkActionExec) bst_piano_roll_controller_exec_action,
                               self);
  return alist;
}

GxkActionList*
bst_piano_roll_controller_canvas_actions (BstPianoRollController *self)
{
  GxkActionList *alist = gxk_action_list_create_grouped (self->canvas_rtools);
  static const GxkStockAction actions[] = {
    { N_("Insert"),           "I",    N_("Insert/resize/move notes (mouse button 1 and 2)"),
      BST_COMMON_ROLL_TOOL_INSERT,   BST_STOCK_PART_TOOL },
    { N_("Delete"),           "D",    N_("Delete note (mouse button 1)"),
      BST_COMMON_ROLL_TOOL_DELETE,   BST_STOCK_TRASHCAN },
    { N_("Align Events"),     "A",    N_("Draw a line to align events to"),
      BST_COMMON_ROLL_TOOL_ALIGN,    BST_STOCK_EVENT_CONTROL },
    { N_("Select"),           "S",    N_("Rectangle select notes"),
      BST_COMMON_ROLL_TOOL_SELECT,   BST_STOCK_RECT_SELECT },
    { N_("Vertical Select"),  "V",    N_("Select tick range vertically"),
      BST_COMMON_ROLL_TOOL_VSELECT,  BST_STOCK_VERT_SELECT },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/, NULL /*acheck*/, NULL /*aexec*/, NULL);
  return alist;
}

GxkActionList*
bst_piano_roll_controller_note_actions (BstPianoRollController *self)
{
  GxkActionList *alist = gxk_action_list_create_grouped (self->note_rtools);
  static const GxkStockAction actions[] = {
    { N_("1\\/1"),              "1",    N_("Insert whole notes"),
      1,                        BST_STOCK_NOTE_1, },
    { N_("1\\/2"),              "2",    N_("Insert half notes"),
      2,                        BST_STOCK_NOTE_2, },
    { N_("1\\/4"),              "4",    N_("Insert quarter notes"),
      4,                        BST_STOCK_NOTE_4, },
    { N_("1\\/8"),              "8",    N_("Insert eighths note"),
      8,                        BST_STOCK_NOTE_8, },
    { N_("1\\/16"),             "6",    N_("Insert sixteenth note"),
      16,                       BST_STOCK_NOTE_16, },
    { N_("1\\/32"),             "3",    N_("Insert thirty-second note"),
      32,                       BST_STOCK_NOTE_32, },
    { N_("1\\/64"),             "5",    N_("Insert sixty-fourth note"),
      64,                       BST_STOCK_NOTE_64, },
    { N_("1\\/128"),            "7",    N_("Insert hundred twenty-eighth note"),
      128,                      BST_STOCK_NOTE_128, },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/, NULL /*acheck*/, NULL /*aexec*/, NULL);
  return alist;
}

GxkActionList*
bst_piano_roll_controller_quant_actions (BstPianoRollController *self)
{
  GxkActionList *alist = gxk_action_list_create_grouped (self->quant_rtools);
  static const GxkStockAction actions[] = {
      { N_("Q: Tact"),          "<ctrl>T",      N_("Quantize to tact boundaries"),
        BST_QUANTIZE_TACT,      BST_STOCK_QTACT, },
      { N_("Q: None"),          "<ctrl>0",      N_("No quantization selected"),
        BST_QUANTIZE_NONE,      BST_STOCK_QNOTE_NONE, },
      { N_("Q: 1\\/1"),         "<ctrl>1",      N_("Quantize to whole note boundaries"),
        BST_QUANTIZE_NOTE_1,    BST_STOCK_QNOTE_1, },
      { N_("Q: 1\\/2"),         "<ctrl>2",      N_("Quantize to half note boundaries"),
        BST_QUANTIZE_NOTE_2,    BST_STOCK_QNOTE_2, },
      { N_("Q: 1\\/4"),         "<ctrl>4",      N_("Quantize to quarter note boundaries"),
        BST_QUANTIZE_NOTE_4,    BST_STOCK_QNOTE_4, },
      { N_("Q: 1\\/8"),         "<ctrl>8",      N_("Quantize to eighths note boundaries"),
        BST_QUANTIZE_NOTE_8,    BST_STOCK_QNOTE_8, },
      { N_("Q: 1\\/16"),        "<ctrl>6",      N_("Quantize to sixteenth note boundaries"),
        BST_QUANTIZE_NOTE_16,   BST_STOCK_QNOTE_16, },
      { N_("Q: 1\\/32"),        "<ctrl>3",      N_("Quantize to thirty-second note boundaries"),
        BST_QUANTIZE_NOTE_32,   BST_STOCK_QNOTE_32, },
      { N_("Q: 1\\/64"),        "<ctrl>5",      N_("Quantize to sixty-fourth note boundaries"),
        BST_QUANTIZE_NOTE_64,   BST_STOCK_QNOTE_64, },
      { N_("Q: 1\\/128"),       "<ctrl>7",      N_("Quantize to hundred twenty-eighth note boundaries"),
        BST_QUANTIZE_NOTE_128,  BST_STOCK_QNOTE_128, },
  };
  gxk_action_list_add_actions (alist, G_N_ELEMENTS (actions), actions,
                               NULL /*i18n_domain*/, NULL /*acheck*/, NULL /*aexec*/, NULL);
  return alist;
}

void
bst_piano_roll_controller_set_clipboard (const Bse::PartNoteSeq *pseq)
{
  if (clipboard_pseq)
    delete clipboard_pseq;
  clipboard_pseq = pseq->size() > 0 ? new Bse::PartNoteSeq (*pseq) : NULL;
  if (clipboard_pseq)
    bst_event_roll_controller_set_clipboard (Bse::PartControlSeq());
}

Bse::PartNoteSeq*
bst_piano_roll_controller_get_clipboard (void)
{
  return clipboard_pseq;
}

static void
controller_reset_canvas_cursor (BstPianoRollController *self)
{
  controller_update_canvas_cursor (self, BstCommonRollTool (self->canvas_rtools->action_id));
}

BstPianoRollController*
bst_piano_roll_controller_new (BstPianoRoll *proll)
{
  BstPianoRollController *self;

  assert_return (BST_IS_PIANO_ROLL (proll), NULL);

  self = g_new0 (BstPianoRollController, 1);
  self->proll = proll;
  self->ref_count = 1;

  self->ref_count++;
  g_signal_connect_data (proll, "canvas-drag",
			 G_CALLBACK (controller_canvas_drag),
			 self, (GClosureNotify) bst_piano_roll_controller_unref,
			 G_CONNECT_SWAPPED);
  g_signal_connect_data (proll, "piano-drag",
			 G_CALLBACK (controller_piano_drag),
			 self, NULL,
			 G_CONNECT_SWAPPED);
  /* canvas tools */
  self->canvas_rtools = gxk_action_group_new ();
  gxk_action_group_select (self->canvas_rtools, BST_COMMON_ROLL_TOOL_INSERT);
  /* note length selection */
  self->note_rtools = gxk_action_group_new ();
  gxk_action_group_select (self->note_rtools, 4);
  /* quantization selection */
  self->quant_rtools = gxk_action_group_new ();
  gxk_action_group_select (self->quant_rtools, BST_QUANTIZE_NOTE_8);
  /* update from action group */
  g_signal_connect_swapped (self->canvas_rtools, "changed",
                            G_CALLBACK (controller_reset_canvas_cursor), self);
  controller_reset_canvas_cursor (self);
  return self;
}

BstPianoRollController*
bst_piano_roll_controller_ref (BstPianoRollController *self)
{
  assert_return (self != NULL, NULL);
  assert_return (self->ref_count >= 1, NULL);

  self->ref_count++;

  return self;
}

void
bst_piano_roll_controller_unref (BstPianoRollController *self)
{
  assert_return (self != NULL);
  assert_return (self->ref_count >= 1);

  self->ref_count--;
  if (!self->ref_count)
    {
      gxk_action_group_dispose (self->canvas_rtools);
      g_object_unref (self->canvas_rtools);
      gxk_action_group_dispose (self->note_rtools);
      g_object_unref (self->note_rtools);
      gxk_action_group_dispose (self->quant_rtools);
      g_object_unref (self->quant_rtools);
      g_free (self);
    }
}

static gboolean
bst_piano_roll_controller_check_action (BstPianoRollController *self,
                                        gulong                  action_id,
                                        guint64                 action_stamp)
{
  switch (action_id)
    {
    case ACTION_SELECT_ALL:
      return TRUE;
    case ACTION_SELECT_NONE:
    case ACTION_SELECT_INVERT:
      return bst_piano_roll_controller_has_selection (self, action_stamp);
    }
  return FALSE;
}

static void
bst_piano_roll_controller_exec_action (BstPianoRollController *self,
                                       gulong                  action_id)
{
  Bse::PartH part = self->proll->part;
  Bse::PartNoteSeq pseq;
  switch (action_id)
    {
    case ACTION_SELECT_ALL:
      part.select_notes (0, self->proll->max_ticks, self->proll->min_note, self->proll->max_note);
      break;
    case ACTION_SELECT_NONE:
      part.deselect_notes (0, self->proll->max_ticks, self->proll->min_note, self->proll->max_note);
      break;
    case ACTION_SELECT_INVERT:
      pseq = part.list_selected_notes();
      part.select_notes (0, self->proll->max_ticks, self->proll->min_note, self->proll->max_note);
      for (size_t i = 0; i < pseq.size(); i++)
        {
          const Bse::PartNote *pnote = &pseq[i];
          part.deselect_event (pnote->id);
        }
      break;
    }
  gxk_widget_update_actions_downwards (self->proll);
}

static BstCommonRollTool
piano_canvas_button_tool (BstPianoRollController *self,
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
      case 1:  return BST_COMMON_ROLL_TOOL_VSELECT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;         /* user error */
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    case BST_COMMON_ROLL_TOOL_VSELECT | HAVE_OBJECT:
      switch (button) {
      case 1:  return BST_COMMON_ROLL_TOOL_VSELECT;
      case 2:  return BST_COMMON_ROLL_TOOL_MOVE;
      default: return BST_COMMON_ROLL_TOOL_NONE;
      }
    }
  return BST_COMMON_ROLL_TOOL_NONE;
}

void
bst_piano_roll_controller_clear (BstPianoRollController *self)
{
  assert_return (self != NULL);
  Bse::PartH part = self->proll->part;
  Bse::PartNoteSeq pseq = part.list_selected_notes();
  part.group_undo ("Clear Selection");
  for (size_t i = 0; i < pseq.size(); i++)
    {
      const Bse::PartNote *pnote = &pseq[i];
      part.delete_event (pnote->id);
    }
  part.ungroup_undo();
}

void
bst_piano_roll_controller_cut (BstPianoRollController *self)
{
  assert_return (self != NULL);

  Bse::PartH part = self->proll->part;
  Bse::PartNoteSeq pseq = part.list_selected_notes();
  part.group_undo ("Cut Selection");
  for (size_t i = 0; i < pseq.size(); i++)
    {
      const Bse::PartNote *pnote = &pseq[i];
      part.delete_event (pnote->id);
    }
  bst_piano_roll_controller_set_clipboard (&pseq);
  part.ungroup_undo();
}

gboolean
bst_piano_roll_controller_copy (BstPianoRollController *self)
{
  assert_return (self != NULL, FALSE);

  Bse::PartH part = self->proll->part;
  Bse::PartNoteSeq pseq = part.list_selected_notes();
  bst_piano_roll_controller_set_clipboard (&pseq);
  return pseq.size() > 0;
}

void
bst_piano_roll_controller_paste (BstPianoRollController *self)
{
  assert_return (self != NULL);

  Bse::PartH part = self->proll->part;
  Bse::PartNoteSeq *pseq = bst_piano_roll_controller_get_clipboard ();
  if (pseq)
    {
      guint i, paste_tick, ctick = self->proll->max_ticks;
      gint cnote = 0;
      gint paste_note;
      part.group_undo ("Paste Clipboard");
      part.deselect_notes (0, self->proll->max_ticks, self->proll->min_note, self->proll->max_note);
      bst_piano_roll_get_paste_pos (self->proll, &paste_tick, &paste_note);
      paste_tick = bst_piano_roll_controller_quantize (self, paste_tick);
      for (i = 0; i < pseq->size(); i++)
	{
          const Bse::PartNote *pnote = &(*pseq)[i];
	  ctick = MIN (ctick, uint (pnote->tick));
	  cnote = MAX (cnote, pnote->note);
	}
      cnote = paste_note - cnote;
      for (i = 0; i < pseq->size(); i++)
	{
          const Bse::PartNote *pnote = &(*pseq)[i];
	  guint id;
	  gint note;
	  note = pnote->note + cnote;
	  if (note >= 0)
	    {
	      id = part.insert_note_auto (pnote->tick - ctick + paste_tick, pnote->duration, note, pnote->fine_tune, pnote->velocity);
              part.select_event (id);
            }
	}
      part.ungroup_undo();
    }
}

gboolean
bst_piano_roll_controller_clipboard_full (BstPianoRollController *self)
{
  Bse::PartNoteSeq *pseq = bst_piano_roll_controller_get_clipboard ();
  return pseq && pseq->size() > 0;
}

gboolean
bst_piano_roll_controller_has_selection (BstPianoRollController *self,
                                         guint64                 action_stamp)
{
  if (self->cached_stamp != action_stamp)
    {
      Bse::PartH part = self->proll->part;
      if (part)
        {
          self->cached_stamp = action_stamp;
          Bse::PartNoteSeq pseq = part.list_selected_notes();
          self->cached_n_notes = pseq.size();
        }
      else
        self->cached_n_notes = 0;
    }
  return self->cached_n_notes > 0;
}

guint
bst_piano_roll_controller_quantize (BstPianoRollController *self,
                                    guint                   fine_tick)
{
  assert_return (self != NULL, fine_tick);

  Bse::PartH part = self->proll->part;
  Bse::SongTiming timing = part.get_timing (fine_tick);
  uint quant, tick, qtick;
  if (QUANTIZATION (self) == BST_QUANTIZE_NONE)
    quant = 1;
  else if (QUANTIZATION (self) == BST_QUANTIZE_TACT)
    quant = timing.tpt;
  else
    quant = timing.tpqn * 4 / QUANTIZATION (self);
  tick = fine_tick - timing.tick;
  qtick = tick / quant;
  qtick *= quant;
  if (tick - qtick > quant / 2)
    qtick += quant;
  tick = timing.tick + qtick;
  return tick;
}

static void
controller_update_canvas_cursor (BstPianoRollController *self,
                                 BstCommonRollTool      tool)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self->proll);
  switch (tool)
    {
    case BST_COMMON_ROLL_TOOL_INSERT:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_PENCIL);
      break;
    case BST_COMMON_ROLL_TOOL_RESIZE:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_SB_H_DOUBLE_ARROW);
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
    case BST_COMMON_ROLL_TOOL_VSELECT:
      gxk_scroll_canvas_set_canvas_cursor (scc, GDK_LEFT_SIDE);
      break;
    default:
      gxk_scroll_canvas_set_canvas_cursor (scc, GXK_DEFAULT_CURSOR);
      break;
    }
}

static gboolean
check_hoverlap (Bse::PartH part, uint tick, uint duration, int note, uint except_tick, uint except_duration)
{
  if (duration)
    {
      Bse::PartNoteSeq pseq = part.check_overlap (tick, duration, note);

      if (pseq.size() == 0)
	return false;   // no overlap
      if (pseq.size() > 1)
	return true;    // definitly overlaps
      const Bse::PartNote *pnote = &pseq[0];
      if (uint (pnote->tick) == except_tick &&
	  uint (pnote->duration) == except_duration)
	return false;   // overlaps with exception
    }
  return true;
}

static void
move_start (BstPianoRollController *self,
	    BstPianoRollDrag       *drag)
{
  Bse::PartH part = self->proll->part;
  if (self->obj_id)	/* got note to move */
    {
      self->xoffset = drag->start_tick - self->obj_tick;	/* drag offset */
      controller_update_canvas_cursor (self, BST_COMMON_ROLL_TOOL_MOVE);
      gxk_status_set (GXK_STATUS_WAIT, _("Move Note"), NULL);
      drag->state = GXK_DRAG_CONTINUE;
      if (part.is_event_selected (self->obj_id))
        self->sel_pseq = part.list_selected_notes();
    }
  else
    {
      gxk_status_set (GXK_STATUS_ERROR, _("Move Note"), _("No target"));
      drag->state = GXK_DRAG_HANDLED;
    }
}

static void
move_group_motion (BstPianoRollController *self, BstPianoRollDrag *drag)
{
  Bse::PartH part = self->proll->part;
  int new_tick, old_note, new_note, delta_tick, delta_note;

  new_tick = MAX (drag->current_tick, self->xoffset) - self->xoffset;
  new_tick = bst_piano_roll_controller_quantize (self, new_tick);
  old_note = self->obj_note;
  new_note = drag->current_note;
  delta_tick = self->obj_tick;
  delta_note = old_note;
  delta_tick -= new_tick;
  delta_note -= new_note;
  part.group_undo ("Move Selection");
  for (size_t i = 0; i < self->sel_pseq.size(); i++)
    {
      const Bse::PartNote *pnote = &self->sel_pseq[i];
      gint tick = pnote->tick;
      gint note = pnote->note;
      note -= delta_note;
      part.change_note (pnote->id, MAX (tick - delta_tick, 0), pnote->duration,
                        SFI_NOTE_CLAMP (note), pnote->fine_tune, pnote->velocity);
    }
  if (drag->type == GXK_DRAG_DONE)
    self->sel_pseq.clear();
  part.ungroup_undo();
}

static void
move_motion (BstPianoRollController *self,
	     BstPianoRollDrag       *drag)
{
  Bse::PartH part = self->proll->part;
  bool note_changed;

  if (self->sel_pseq.size() > 0)
    {
      move_group_motion (self, drag);
      return;
    }

  int new_tick = MAX (drag->current_tick, self->xoffset) - self->xoffset;
  new_tick = bst_piano_roll_controller_quantize (self, new_tick);
  note_changed = self->obj_note != drag->current_note;
  if ((uint (new_tick) != self->obj_tick || note_changed) &&
      !check_hoverlap (part, new_tick, self->obj_duration, drag->current_note,
		       self->obj_tick, note_changed ? 0 : self->obj_duration))
    {
      part.group_undo ("Move Note");
      if (part.delete_event (self->obj_id) != Bse::Error::NONE)
        drag->state = GXK_DRAG_ERROR;
      else
	{
	  self->obj_id = part.insert_note_auto (new_tick, self->obj_duration,
                                                drag->current_note, self->obj_fine_tune, self->obj_velocity);
	  self->obj_tick = new_tick;
	  self->obj_note = drag->current_note;
	  if (!self->obj_id)
	    drag->state = GXK_DRAG_ERROR;
	}
      part.ungroup_undo();
    }
}

static void
move_abort (BstPianoRollController *self,
	    BstPianoRollDrag       *drag)
{
  self->sel_pseq.clear();
  gxk_status_set (GXK_STATUS_ERROR, _("Move Note"), _("Lost Note"));
}

static void
resize_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  if (self->obj_id)	/* got note for resize */
    {
      guint bound = self->obj_tick + self->obj_duration + 1;

      /* set the fix-point (either note start or note end) */
      if (drag->start_tick - self->obj_tick <= bound - drag->start_tick)
	self->tick_bound = bound;
      else
	self->tick_bound = self->obj_tick;
      controller_update_canvas_cursor (self, BST_COMMON_ROLL_TOOL_RESIZE);
      gxk_status_set (GXK_STATUS_WAIT, _("Resize Note"), NULL);
      drag->state = GXK_DRAG_CONTINUE;
    }
  else
    {
      gxk_status_set (GXK_STATUS_ERROR, _("Resize Note"), _("No target"));
      drag->state = GXK_DRAG_HANDLED;
    }
}

static void
resize_motion (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  Bse::PartH part = self->proll->part;
  guint new_bound, new_tick, new_duration;

  /* calc new note around fix-point */
  new_tick = bst_piano_roll_controller_quantize (self, drag->current_tick);
  new_bound = MAX (new_tick, self->tick_bound);
  new_tick = MIN (new_tick, self->tick_bound);
  new_duration = new_bound - new_tick;
  new_duration = MAX (new_duration, 1) - 1;

  /* apply new note size */
  if ((self->obj_tick != new_tick || new_duration != self->obj_duration) &&
      !check_hoverlap (part, new_tick, new_duration, self->obj_note,
		       self->obj_tick, self->obj_duration))
    {
      part.group_undo ("Resize Note");
      if (self->obj_id)
	{
	  Bse::Error error = part.delete_event (self->obj_id);
	  if (error != 0)
	    drag->state = GXK_DRAG_ERROR;
	  self->obj_id = 0;
	}
      if (new_duration && drag->state != GXK_DRAG_ERROR)
	{
	  self->obj_id = part.insert_note_auto (new_tick, new_duration,
                                                self->obj_note, self->obj_fine_tune, self->obj_velocity);
	  self->obj_tick = new_tick;
	  self->obj_duration = new_duration;
	  if (!self->obj_id)
	    drag->state = GXK_DRAG_ERROR;
	}
      part.ungroup_undo();
    }
}

static void
resize_abort (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Resize Note"), _("Lost Note"));
}

static void
delete_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  Bse::PartH part = self->proll->part;
  if (self->obj_id)	/* got note to delete */
    {
      Bse::Error error = part.delete_event (self->obj_id);
      bst_status_eprintf (error, _("Delete Note"));
    }
  else
    gxk_status_set (GXK_STATUS_ERROR, _("Delete Note"), _("No target"));
  drag->state = GXK_DRAG_HANDLED;
}

static void
insert_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  Bse::PartH part = self->proll->part;
  Bse::Error error = Bse::Error::NO_TARGET;
  if (drag->start_valid)
    {
      guint qtick = bst_piano_roll_controller_quantize (self, drag->start_tick);
      guint duration = drag->proll->ppqn * 4 / NOTE_LENGTH (self);
      if (check_hoverlap (part, qtick, duration, drag->start_note, 0, 0))
	error = Bse::Error::INVALID_OVERLAP;
      else
	{
	  part.insert_note_auto (qtick, duration, drag->start_note, 0, 1.0);
	  error = Bse::Error::NONE;
	}
    }
  bst_status_eprintf (error, _("Insert Note"));
  drag->state = GXK_DRAG_HANDLED;
}

static void
select_start (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  drag->start_tick = bst_piano_roll_controller_quantize (self, drag->start_tick);
  bst_piano_roll_set_view_selection (drag->proll, drag->start_tick, 0, 0, 0);
  gxk_status_set (GXK_STATUS_WAIT, _("Select Region"), NULL);
  drag->state = GXK_DRAG_CONTINUE;
}

static void
select_motion (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  Bse::PartH part = self->proll->part;
  uint start_tick = MIN (drag->start_tick, drag->current_tick);
  uint end_tick = MAX (drag->start_tick, drag->current_tick);
  int min_note = MIN (drag->start_note, drag->current_note);
  int max_note = MAX (drag->start_note, drag->current_note);

  bst_piano_roll_set_view_selection (drag->proll, start_tick, end_tick - start_tick, min_note, max_note);
  if (drag->type == GXK_DRAG_DONE)
    {
      part.select_notes_exclusive (start_tick, end_tick - start_tick, min_note, max_note);
      bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
    }
}

static void
select_abort (BstPianoRollController *self,
	      BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Select Region"), _("Aborted"));
  bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
}

static void
vselect_start (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  drag->start_tick = bst_piano_roll_controller_quantize (self, drag->start_tick);
  bst_piano_roll_set_view_selection (drag->proll, drag->start_tick, 0, drag->proll->min_note, drag->proll->max_note);
  gxk_status_set (GXK_STATUS_WAIT, _("Vertical Select"), NULL);
  drag->state = GXK_DRAG_CONTINUE;
}

static void
vselect_motion (BstPianoRollController *self,
		BstPianoRollDrag       *drag)
{
  Bse::PartH part = self->proll->part;
  uint start_tick = MIN (drag->start_tick, drag->current_tick);
  uint end_tick = MAX (drag->start_tick, drag->current_tick);

  bst_piano_roll_set_view_selection (drag->proll, start_tick, end_tick - start_tick,
				     drag->proll->min_note, drag->proll->max_note);
  if (drag->type == GXK_DRAG_DONE)
    {
      part.select_notes_exclusive (start_tick, end_tick - start_tick,
                                   drag->proll->min_note, drag->proll->max_note);
      bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
    }
}

static void
vselect_abort (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Vertical Region"), _("Aborted"));
  bst_piano_roll_set_view_selection (drag->proll, 0, 0, 0, 0);
}

#if 0
static void
generic_abort (BstPianoRollController *self,
	       BstPianoRollDrag       *drag)
{
  gxk_status_set (GXK_STATUS_ERROR, _("Abortion"), NULL);
}
#endif

typedef void (*DragFunc) (BstPianoRollController *,
			  BstPianoRollDrag       *);

void
controller_canvas_drag (BstPianoRollController *self,
			BstPianoRollDrag       *drag)
{
  static struct {
    BstCommonRollTool tool;
    DragFunc start, motion, abort;
  } tool_table[] = {
    { BST_COMMON_ROLL_TOOL_INSERT,	insert_start,	NULL,		NULL,		},
    { BST_COMMON_ROLL_TOOL_ALIGN,	insert_start,	NULL,		NULL,		},
    { BST_COMMON_ROLL_TOOL_RESIZE,	resize_start,	resize_motion,	resize_abort,	},
    { BST_COMMON_ROLL_TOOL_MOVE,	move_start,	move_motion,	move_abort,	},
    { BST_COMMON_ROLL_TOOL_DELETE,	delete_start,	NULL,		NULL,		},
    { BST_COMMON_ROLL_TOOL_SELECT,	select_start,	select_motion,	select_abort,	},
    { BST_COMMON_ROLL_TOOL_VSELECT,	vselect_start,	vselect_motion,	vselect_abort,	},
  };
  guint i;

  if (drag->type == GXK_DRAG_START)
    {
      BstCommonRollTool tool = BST_COMMON_ROLL_TOOL_NONE;

      /* setup drag data */
      Bse::PartH part = drag->proll->part;
      Bse::PartNoteSeq pseq = part.get_notes (drag->start_tick, drag->start_note);
      if (pseq.size() > 0)
	{
	  const Bse::PartNote *pnote = &pseq[0];
	  self->obj_id = pnote->id;
	  self->obj_tick = pnote->tick;
	  self->obj_duration = pnote->duration;
	  self->obj_note = pnote->note;
	  self->obj_fine_tune = pnote->fine_tune;
	  self->obj_velocity = pnote->velocity;
	}
      else
	{
	  self->obj_id = 0;
	  self->obj_tick = 0;
	  self->obj_duration = 0;
	  self->obj_note = 0;
	  self->obj_fine_tune = 0;
	  self->obj_velocity = 0;
	}
      if (self->sel_pseq.size())
	Bse::warning ("leaking old drag selection (%zu)", self->sel_pseq.size());
      self->sel_pseq.clear();
      self->xoffset = 0;
      self->tick_bound = 0;

      /* find drag tool */
      tool = piano_canvas_button_tool (self, drag->button, self->obj_id > 0);
      for (i = 0; i < G_N_ELEMENTS (tool_table); i++)
	if (tool_table[i].tool == tool)
	  break;
      self->tool_index = i;
      if (i >= G_N_ELEMENTS (tool_table))
	return;		/* unhandled */
    }
  i = self->tool_index;
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
controller_piano_drag (BstPianoRollController *self,
		       BstPianoRollDrag       *drag)
{
  Bse::PartH part = self->proll->part;
  Bse::SongH song = Bse::SongH::down_cast (part.get_parent());
  Bse::ProjectH project;
  Bse::TrackH track;
  if (song)
    {
      project = Bse::ProjectH::down_cast (song.get_parent());
      track = song.find_track_for_part (part);
    }

  // printerr ("piano drag event, note=%d (valid=%d)", drag->current_note, drag->current_valid);

  if (project && track &&
      (drag->type == GXK_DRAG_START ||
       (drag->type == GXK_DRAG_MOTION &&
        self->obj_note != drag->current_note)))
    {
      project.auto_deactivate (5 * 1000);
      Bse::Error error = project.activate();
      self->obj_note = drag->current_note;
      if (error == Bse::Error::NONE)
        song.synthesize_note (track, 384 * 4, self->obj_note, 0, 1.0);
      bst_status_eprintf (error, _("Play note"));
      drag->state = GXK_DRAG_CONTINUE;
    }

  if (drag->type == GXK_DRAG_START ||
      drag->type == GXK_DRAG_MOTION)
    drag->state = GXK_DRAG_CONTINUE;
}
