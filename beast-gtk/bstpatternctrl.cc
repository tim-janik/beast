// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstpatternctrl.hh"
#include "bstkeybindings.hh"
#include <gdk/gdkkeysyms.h>
#include <string.h>


/* --- prototypes --- */
static gboolean pattern_controller_key_press    (BstPatternController   *self,
                                                 GdkEventKey            *event);


/* --- functions --- */
static void
pattern_controller_vraster_notify (gpointer             notify_data,
                                   GxkParam            *param)
{
  using namespace Bst; // for Bst::NOTE_LENGTH_*
  BstPatternController *self = (BstPatternController*) notify_data;
  BstPatternView *pview = self->pview;
  static const struct { int value, ticks; } choices[] = {
    { int64 (NoteLength::NOTE_LENGTH_1),        1536 }, /* 4 * 384 */
    { int64 (NoteLength::NOTE_LENGTH_2),         768 },
    { int64 (NoteLength::NOTE_LENGTH_4),         384 },
    { int64 (NoteLength::NOTE_LENGTH_8),         192 },
    { int64 (NoteLength::NOTE_LENGTH_16),         96 },
    { int64 (NoteLength::NOTE_LENGTH_32),         48 },
    { int64 (NoteLength::NOTE_LENGTH_64),         24 },
    { int64 (NoteLength::NOTE_LENGTH_128),        12 },
    { int64 (NoteLength::NOTE_LENGTH_1P),       2304 }, /* 4 * 384 * 3 / 2 */
    { int64 (NoteLength::NOTE_LENGTH_2P),       1152 },
    { int64 (NoteLength::NOTE_LENGTH_4P),        576 },
    { int64 (NoteLength::NOTE_LENGTH_8P),        288 },
    { int64 (NoteLength::NOTE_LENGTH_16P),       144 },
    { int64 (NoteLength::NOTE_LENGTH_32P),        72 },
    { int64 (NoteLength::NOTE_LENGTH_64P),        36 },
    { int64 (NoteLength::NOTE_LENGTH_128P),       18 },
    { int64 (NoteLength::NOTE_LENGTH_1T),       1024 }, /* 4 * 384 * 2 / 3 */
    { int64 (NoteLength::NOTE_LENGTH_2T),        512 },
    { int64 (NoteLength::NOTE_LENGTH_4T),        256 },
    { int64 (NoteLength::NOTE_LENGTH_8T),        128 },
    { int64 (NoteLength::NOTE_LENGTH_16T),        64 },
    { int64 (NoteLength::NOTE_LENGTH_32T),        32 },
    { int64 (NoteLength::NOTE_LENGTH_64T),        16 },
    { int64 (NoteLength::NOTE_LENGTH_128T),        8 },
  };
  int vraster = 384;
  const int64 vsval = int64 (Aida::enum_value_from_string<NoteLength> (sfi_value_get_choice (&self->vraster->value)));
  for (uint i = 0; i < G_N_ELEMENTS (choices); i++)
    if (choices[i].value == vsval)
      {
        vraster = choices[i].ticks;
        break;
      }
  bst_pattern_view_vsetup (pview, pview->tpqn, pview->tpt / pview->tpqn,
                           pview->max_ticks, vraster);
}

static void
pattern_controller_row_shading_notify (gpointer  notify_data,
                                       GxkParam *param)
{
  using namespace Bst; // for Bst::ROW_SHADING_*
  BstPatternController *self = (BstPatternController*) notify_data;
  BstPatternView *pview = self->pview;
  static const struct { int value, r1, r2, r3, r4; } choices[] = {
    { int64 (RowShading::ROW_SHADING_NONE),      0, 0, 0, 0 },
    { int64 (RowShading::ROW_SHADING_2),         2 },
    { int64 (RowShading::ROW_SHADING_4),         4 },
    { int64 (RowShading::ROW_SHADING_8),         8 },
    { int64 (RowShading::ROW_SHADING_16),       16 },
    { int64 (RowShading::ROW_SHADING_2_4),       4, 2 },
    { int64 (RowShading::ROW_SHADING_4_8),       8, 4 },
    { int64 (RowShading::ROW_SHADING_4_12),     12, 4 },
    { int64 (RowShading::ROW_SHADING_4_16),     16, 4 },
    { int64 (RowShading::ROW_SHADING_8_16),     16, 8 },
    { int64 (RowShading::ROW_SHADING_3),         3 },
    { int64 (RowShading::ROW_SHADING_6),         6 },
    { int64 (RowShading::ROW_SHADING_12),       12 },
    { int64 (RowShading::ROW_SHADING_3_6),       6, 3 },
    { int64 (RowShading::ROW_SHADING_3_12),     12, 3 },
    { int64 (RowShading::ROW_SHADING_6_12),     12, 6 },
  };
  int r1 = 0, r2 = 0, r3 = 0, r4 = 0;
  const int64 vsval = int64 (Aida::enum_value_from_string<RowShading> (sfi_value_get_choice (&self->row_shading->value)));
  for (uint i = 0; i < G_N_ELEMENTS (choices); i++)
    if (choices[i].value == vsval)
      {
        r1 = choices[i].r1;
        r2 = choices[i].r2;
        r3 = choices[i].r3;
        r4 = choices[i].r4;
        break;
      }
  bst_pattern_view_set_shading (pview, r1, r2, r3, r4);
}

BstPatternController*
bst_pattern_controller_new (BstPatternView         *pview,
                            GxkActionGroup         *quant_rtools)
{
  BstPatternController *self;

  assert_return (BST_IS_PATTERN_VIEW (pview), NULL);

  self = g_new0 (BstPatternController, 1);
  self->vraster = gxk_param_new_value (sfi_pspec_choice ("vertical-raster", _("VZoom"),
                                                         _("The tick/note length per line"),
                                                         "note-length-4",
                                                         Bse::choice_values_from_enum<Bst::NoteLength>(),
                                                         SFI_PARAM_STANDARD),
                                       pattern_controller_vraster_notify, self);
  self->steps = gxk_param_new_value (sfi_pspec_int ("steps", _("Steps"),
                                                    _("The number of cells to move across each time "
                                                      "an event or note was edited"),
                                                    1, 0, 384, 4, SFI_PARAM_STANDARD),
                                     NULL, NULL);
  self->step_dir = gxk_param_new_value (sfi_pspec_choice ("step_dir", _("Direction"),
                                                          _("The direction of cell movement each time "
                                                            "an event or note was edited"),
                                                          "down", Bse::choice_values_from_enum<Bst::Direction>(), SFI_PARAM_STANDARD),
                                        NULL, NULL);
  self->hwrap = gxk_param_new_value (sfi_pspec_bool ("hwrap", _("HWrap"),
                                                     _("Toggle whether horizontal movement of the focus cell will "
                                                       "wrap around edges"), FALSE, SFI_PARAM_STANDARD),
                                     NULL, NULL);
  self->base_octave = gxk_param_new_value (sfi_pspec_int ("base_octave", _("Base Octave"),
                                                          _("Controls the octave relative to which notes are entered"),
                                                          1, -4, +6, 1, SFI_PARAM_STANDARD),
                                           NULL, NULL);
  self->row_shading = gxk_param_new_value (sfi_pspec_choice ("row-shading", _("Row Shading"),
                                                             _("Adjust the number of rows between each shaded row"),
                                                             "row-shading-4-16",
                                                             Bse::choice_values_from_enum<Bst::RowShading>(),
                                                             SFI_PARAM_STANDARD),
                                           pattern_controller_row_shading_notify, self);
  self->pview = pview;
  self->ref_count = 1;

  self->ref_count++;
  g_signal_connect_data (pview, "key-press-event",
                         G_CALLBACK (pattern_controller_key_press),
                         self, (GClosureNotify) bst_pattern_controller_unref,
                         G_CONNECT_SWAPPED);
  self->quant_rtools = quant_rtools ? (GxkActionGroup*) g_object_ref (quant_rtools) : NULL;
  pattern_controller_vraster_notify (self, NULL);
  pattern_controller_row_shading_notify (self, NULL);

  gxk_scroll_canvas_set_canvas_cursor (GXK_SCROLL_CANVAS (pview), GDK_XTERM);
  return self;
}

BstPatternController*
bst_pattern_controller_ref (BstPatternController   *self)
{
  assert_return (self != NULL, NULL);
  assert_return (self->ref_count >= 1, NULL);

  self->ref_count++;

  return self;
}

void
bst_pattern_controller_unref (BstPatternController   *self)
{
  assert_return (self != NULL);
  assert_return (self->ref_count >= 1);

  self->ref_count--;
  if (!self->ref_count)
    {
      if (self->quant_rtools)
        {
          gxk_action_group_dispose (self->quant_rtools);
          g_object_unref (self->quant_rtools);
        }
      gxk_param_destroy (self->vraster);
      gxk_param_destroy (self->steps);
      gxk_param_destroy (self->step_dir);
      gxk_param_destroy (self->hwrap);
      gxk_param_destroy (self->base_octave);
      g_free (self);
    }
}

static gboolean
pattern_controller_key_press (BstPatternController *self,
                              GdkEventKey          *event)
{
  BstPatternView *pview = self->pview;
  BstPatternColumn *col = bst_pattern_view_get_focus_cell (pview, NULL, NULL);
  BstPatternFunction movement, ftype = BstPatternFunction (0);
  gdouble param = 0;
  gboolean handled;
  gint base_octave, note;
  GdkModifierType state = GdkModifierType (event->state);
  uint g = col->klass->collision_group;
  if (!ftype && g)
    ftype = (BstPatternFunction) bst_key_binding_lookup_id (bst_pattern_controller_piano_keys(), event->keyval, state, g, &param);
  if (!ftype)
    ftype = (BstPatternFunction) bst_key_binding_lookup_id (bst_pattern_controller_piano_keys(), event->keyval, state, 0, &param);
  if (!ftype && g)
    ftype = (BstPatternFunction) bst_key_binding_lookup_id (bst_pattern_controller_generic_keys(), event->keyval, state, g, &param);
  if (!ftype)
    ftype = (BstPatternFunction) bst_key_binding_lookup_id (bst_pattern_controller_generic_keys(), event->keyval, state, 0, &param);
  state = GdkModifierType (event->state) & ~GDK_SHIFT_MASK;
  if (!ftype && g)
    ftype = (BstPatternFunction) bst_key_binding_lookup_id (bst_pattern_controller_piano_keys(), event->keyval, state, g, &param);
  if (!ftype)
    ftype = (BstPatternFunction) bst_key_binding_lookup_id (bst_pattern_controller_piano_keys(), event->keyval, state, 0, &param);
  if (!ftype && g)
    ftype = (BstPatternFunction) bst_key_binding_lookup_id (bst_pattern_controller_generic_keys(), event->keyval, state, g, &param);
  if (!ftype)
    ftype = (BstPatternFunction) bst_key_binding_lookup_id (bst_pattern_controller_generic_keys(), event->keyval, state, 0, &param);
  switch (ftype & BST_PATTERN_MASK_ACTION)
    {
    case BST_PATTERN_SET_NOTE:
      base_octave = sfi_value_get_int (&self->base_octave->value);
      note = param + 0.5;
      note += base_octave * 12;
      while (note < SFI_MIN_NOTE)
        note += 12;
      while (note > SFI_MAX_NOTE)
        note -= 12;
      param = note;
      break;
    default: ;
    }
  switch (ftype & BST_PATTERN_MASK_CONTROLS)
    {
    case BST_PATTERN_CHANGE_BASE_OCTAVE:
      base_octave = sfi_value_get_int (&self->base_octave->value);
      base_octave += param;
      sfi_value_set_int (&self->base_octave->value, base_octave);
      gxk_param_update (self->base_octave);     /* we poked the params value */
      break;
    case BST_PATTERN_SET_BASE_OCTAVE:
      base_octave = param;
      sfi_value_set_int (&self->base_octave->value, base_octave);
      gxk_param_update (self->base_octave);     /* we poked the params value */
      break;
    default: ;
    }
  movement = ftype & BST_PATTERN_MASK_MOVEMENT;
  handled = bst_pattern_view_dispatch_key (pview, event->keyval, GdkModifierType (event->state), ftype & BST_PATTERN_MASK_ACTION, param, &movement);
  if (movement == BST_PATTERN_MOVE_NEXT &&      /* if the standard step-next movement */
      (event->state & GDK_SHIFT_MASK) &&        /* is blocked by shift */
      (movement != ftype || handled))           /* and this is not purely "next" */
    movement = BstPatternFunction (0);          /* honour the block */
  if (movement)
    {
      const guint channel_page = 2, row_page = 4;
      gboolean hwrap = sfi_value_get_bool (&self->hwrap->value);
      gint focus_col = pview->focus_col;
      gint focus_row = pview->focus_row;
      /* movement */
      switch (movement)
        {
        case BST_PATTERN_MOVE_LEFT:     focus_col--;                                            break;
        case BST_PATTERN_MOVE_RIGHT:    focus_col++;                                            break;
        case BST_PATTERN_MOVE_UP:       focus_row--;                                            break;
        case BST_PATTERN_MOVE_DOWN:     focus_row++;                                            break;
        case BST_PATTERN_PAGE_LEFT:     focus_col -= channel_page;                              break;
        case BST_PATTERN_PAGE_RIGHT:    focus_col += channel_page;                              break;
        case BST_PATTERN_PAGE_UP:       focus_row -= row_page;                                  break;
        case BST_PATTERN_PAGE_DOWN:     focus_row += row_page;                                  break;
        case BST_PATTERN_JUMP_LEFT:     focus_col = 0;                                          break;
        case BST_PATTERN_JUMP_RIGHT:    focus_col = pview->n_focus_cols - 1;                    break;
        case BST_PATTERN_JUMP_TOP:      focus_row = 0;                                          break;
        case BST_PATTERN_JUMP_BOTTOM:   focus_row = bst_pattern_view_get_last_row (pview);      break;
        case BST_PATTERN_MOVE_NEXT:
          {
            Bst::Direction d = Aida::enum_value_from_string<Bst::Direction> (sfi_value_get_choice (&self->step_dir->value));
            if (d == Bst::Direction::DIR_LEFT || d == Bst::Direction::DIR_RIGHT)
              focus_col += (d == Bst::Direction::DIR_LEFT ? -1 : +1) * g_value_get_int (&self->steps->value);
            else /* UP/DOWN */
              focus_row += (d == Bst::Direction::DIR_UP ? -1 : +1) * g_value_get_int (&self->steps->value);
          }
          break;
        case BST_PATTERN_SET_STEP_WIDTH:
          g_value_set_int (&self->steps->value, param);
          gxk_param_update (self->steps);       /* we poked the params value */
          break;
        default: ;
        }
      /* wrapping */
      if (focus_col < 0)
        focus_col = hwrap ? pview->n_focus_cols - 1 : 0;
      if (focus_col >= pview->n_focus_cols)
        focus_col = hwrap ? 0 : pview->n_focus_cols - 1;
      if (focus_row < 0)
        focus_row = 0;
      if (focus_row > bst_pattern_view_get_last_row (pview))
        focus_row = bst_pattern_view_get_last_row (pview);
      /* update focus */
      bst_pattern_view_set_focus (self->pview, focus_col, focus_row);
      handled = TRUE;
    }
  return handled;
}

static const BstKeyBindingFunction*
pattern_controller_get_functions (gboolean want_piano,
                                  guint   *n_p)
{
  static BstKeyBindingFunction generic_funcs[] = {
    /* movement */
    { BST_PATTERN_MOVE_NEXT,    "next",         BST_KEY_BINDING_PARAM_NONE, N_("Move focus to the next cell (up/left/right/down according to configuration)") },
    { BST_PATTERN_MOVE_UP,      "move-up",      BST_KEY_BINDING_PARAM_NONE, N_("Move focus cell upwards") },
    { BST_PATTERN_MOVE_LEFT,    "move-left",    BST_KEY_BINDING_PARAM_NONE, N_("Move focus cell to the left") },
    { BST_PATTERN_MOVE_RIGHT,   "move-right",   BST_KEY_BINDING_PARAM_NONE, N_("Move focus cell to the right") },
    { BST_PATTERN_MOVE_DOWN,    "move-down",    BST_KEY_BINDING_PARAM_NONE, N_("Move focus cell downwards") },
    { BST_PATTERN_PAGE_UP,      "page-up",      BST_KEY_BINDING_PARAM_NONE, N_("Move focus cell upwards page-wise") },
    { BST_PATTERN_PAGE_LEFT,    "page-left",    BST_KEY_BINDING_PARAM_NONE, N_("Move focus cell to the left page-wise") },
    { BST_PATTERN_PAGE_RIGHT,   "page-right",   BST_KEY_BINDING_PARAM_NONE, N_("Move focus cell to the right page-wise") },
    { BST_PATTERN_PAGE_DOWN,    "page-down",    BST_KEY_BINDING_PARAM_NONE, N_("Move focus cell downwards page-wise") },
    { BST_PATTERN_JUMP_TOP,     "jump-top",     BST_KEY_BINDING_PARAM_NONE, N_("Set the focus cell to the topmost position possible") },
    { BST_PATTERN_JUMP_LEFT,    "jump-left",    BST_KEY_BINDING_PARAM_NONE, N_("Set the focus cell to the leftmost position possible") },
    { BST_PATTERN_JUMP_RIGHT,   "jump-right",   BST_KEY_BINDING_PARAM_NONE, N_("Set the focus cell to the rightmost position possible") },
    { BST_PATTERN_JUMP_BOTTOM,  "jump-bottom",  BST_KEY_BINDING_PARAM_NONE, N_("Set the focus cell to the bottommost position possible") },
    { BST_PATTERN_SET_STEP_WIDTH,
      "set-step-width",                         BST_KEY_BINDING_PARAM_SHORT,
      N_("Set the number of steps to make when moving to the next cell") },
    /* base octave */
    { BST_PATTERN_SET_BASE_OCTAVE | BST_PATTERN_MOVE_NEXT,
      "set-base-octave",                        BST_KEY_BINDING_PARAM_SHORT,
      N_("Set the base octave") },
    { BST_PATTERN_CHANGE_BASE_OCTAVE,
      "change-base-octave",                     BST_KEY_BINDING_PARAM_SHORT,
      N_("Change the base octave by a given amount") },
    /* numeric changes */
    { BST_PATTERN_NUMERIC_CHANGE | BST_PATTERN_MOVE_NEXT,
      "numeric-change",                         BST_KEY_BINDING_PARAM_SHORT,
      N_("Change the numeric focus cell contents (e.g. octave) by a given amount") },
    /* octaves */
    { BST_PATTERN_SET_OCTAVE | BST_PATTERN_MOVE_NEXT,
      "set-octave",                             BST_KEY_BINDING_PARAM_SHORT,
      N_("Set the focus cell octave") },
    /* events */
    { BST_PATTERN_REMOVE_EVENTS | BST_PATTERN_MOVE_NEXT,
      "remove-events",                          BST_KEY_BINDING_PARAM_NONE,
      N_("Remove any events in the focus cell"), 0 },
    /* notes */
    { BST_PATTERN_SET_NOTE | BST_PATTERN_MOVE_NEXT,
      "set-note",                               BST_KEY_BINDING_PARAM_NOTE,
      N_("Set the focus cell note"), 1 },
    /* digits */
    { BST_PATTERN_SET_DIGIT | BST_PATTERN_MOVE_NEXT,
      "set-digit",                              BST_KEY_BINDING_PARAM_USHORT,
      N_("Sets the value of the focus digit"), 2 },
  };
  static BstKeyBindingFunction piano_funcs[] = {
    /* events */
    { BST_PATTERN_REMOVE_EVENTS | BST_PATTERN_MOVE_NEXT,
      "remove-events",                          BST_KEY_BINDING_PARAM_NONE,
      N_("Remove any events in the focus cell"), 0 },
    /* notes */
    { BST_PATTERN_SET_NOTE | BST_PATTERN_MOVE_NEXT,
      "set-note",                               BST_KEY_BINDING_PARAM_NOTE,
      N_("Set the focus cell note"), 1 },
    /* digits */
    { BST_PATTERN_SET_DIGIT | BST_PATTERN_MOVE_NEXT,
      "set-digit",                              BST_KEY_BINDING_PARAM_USHORT,
      N_("Sets the value of the focus digit"), 2 },
  };
  static gboolean initialized = FALSE;
  if (!initialized)
    {
      guint i;
      initialized = TRUE;
      for (i = 0; i < G_N_ELEMENTS (generic_funcs); i++)
        generic_funcs[i].function_blurb = _(generic_funcs[i].function_blurb);
      for (i = 0; i < G_N_ELEMENTS (piano_funcs); i++)
        piano_funcs[i].function_blurb = _(piano_funcs[i].function_blurb);
    }
  if (want_piano)
    {
      *n_p = G_N_ELEMENTS (piano_funcs);
      return piano_funcs;
    }
  else
    {
      *n_p = G_N_ELEMENTS (generic_funcs);
      return generic_funcs;
    }
}

struct StaticKeyBindingItem {
  const char *key_name;
  const char *func_name;
  double      func_param;
};
static Bst::KeyBindingItem
key_binding_item_from_static (const StaticKeyBindingItem &skb)
{
  Bst::KeyBindingItem kbi;
  kbi.key_name = skb.key_name;
  kbi.func_name = skb.func_name;
  kbi.func_param = skb.func_param;
  return kbi;
}

BstKeyBinding*
bst_pattern_controller_default_generic_keys (void)
{
  static StaticKeyBindingItem dflt_keys[] = {
    /* move in cells */
    {                   "Up",                   "move-up",      	0 },
    {                   "KP_Up",                "move-up",      	0 },
    {                   "Left",                 "move-left",    	0 },
    {                   "KP_Left",              "move-left",    	0 },
    {                   "Right",                "move-right",   	0 },
    {                   "KP_Right",             "move-right",   	0 },
    {                   "Down",                 "move-down",    	0 },
    {                   "KP_Down",              "move-down",    	0 },
    {                   "KP_Begin"/*5*/,        "move-down",    	0 },
#if 0
    /* move according to configuration */
    {                   "Return",               "next",    	        0 },
    {                   "KP_Enter",             "next",    	        0 },
#endif
    /* move in pages */
    {                   "Page_Up",              "page-up",      	0 },
    {                   "KP_Page_Up",           "page-up",      	0 },
    { "<Control>"       "Left",                 "page-left",    	0 },
    { "<Control>"       "KP_Left",              "page-left",    	0 },
    { "<Control>"       "Right",                "page-right",   	0 },
    { "<Control>"       "KP_Right",             "page-right",   	0 },
    {                   "Page_Down",            "page-down",    	0 },
    {                   "KP_Page_Down",         "page-down",    	0 },
    /* jump to boundary */
    { "<Control>"       "Page_Up",              "jump-top",     	0 },
    { "<Control>"       "KP_Page_Up",           "jump-top",     	0 },
    {                   "Home",                 "jump-left",    	0 },
    {                   "KP_Home",              "jump-left",    	0 },
    {                   "End",                  "jump-right",   	0 },
    {                   "KP_End",               "jump-right",   	0 },
    { "<Control>"       "Page_Down",            "jump-bottom",  	0 },
    { "<Control>"       "KP_Page_Down",         "jump-bottom",  	0 },
#if 0
    /* part movement */
    {                   "Tab",                  "next-part",            0 },
    {                   "KP_Tab",               "next-part",            0 },
    { "<Shift>"         "Tab",                  "prev-part",            0 },
    { "<Shift>"         "KP_Tab",               "prev-part",            0 },
    {                   "ISO_Left_Tab",         "prev-part",            0 },
#endif
    /* set step width for movement */
    { "<Control>"       "0",                    "set-step-width",       0 },
    { "<Control>"       "1",                    "set-step-width",       1 },
    { "<Control>"       "2",                    "set-step-width",       2 },
    { "<Control>"       "3",                    "set-step-width",       3 },
    { "<Control>"       "4",                    "set-step-width",       4 },
    { "<Control>"       "5",                    "set-step-width",       5 },
    { "<Control>"       "6",                    "set-step-width",       6 },
    { "<Control>"       "7",                    "set-step-width",       7 },
    { "<Control>"       "8",                    "set-step-width",       8 },
    { "<Control>"       "9",                    "set-step-width",       9 },
    /* enter numeric values */
    {                   "0",                    "set-digit",            0 },
    {                   "1",                    "set-digit",            1 },
    {                   "2",                    "set-digit",            2 },
    {                   "3",                    "set-digit",            3 },
    {                   "4",                    "set-digit",            4 },
    {                   "5",                    "set-digit",            5 },
    {                   "6",                    "set-digit",            6 },
    {                   "7",                    "set-digit",            7 },
    {                   "8",                    "set-digit",            8 },
    {                   "9",                    "set-digit",            9 },
    {                   "a",                    "set-digit",           10 },
    {                   "b",                    "set-digit",           11 },
    {                   "c",                    "set-digit",           12 },
    {                   "d",                    "set-digit",           13 },
    {                   "e",                    "set-digit",           14 },
    {                   "f",                    "set-digit",           15 },
    /* remove notes/events */
    {                   "space",                "remove-events",        0 },
    {                   "KP_Space",             "remove-events",        0 },
    /* increment cell octave with shift or move */
    {                   "plus",                 "numeric-change",       +1 },
    {                   "KP_Add",               "numeric-change",       +1 },
    {                   "asterisk",             "numeric-change",       +1 },
    {                   "KP_Multiply",          "numeric-change",       +1 },
    /* decrement cell octave with shift or move */
    {                   "minus",                "numeric-change",       -1 },
    {                   "underscore",           "numeric-change",       -1 },
    {                   "KP_Divide",            "numeric-change",       -1 },
    {                   "KP_Subtract",          "numeric-change",       -1 },
    /* increment base octave */
    { "<Control>"       "plus",                 "change-base-octave",      +1 },
    { "<Control>"       "KP_Add",               "change-base-octave",      +1 },
    { "<Control>"       "asterisk",             "change-base-octave",      +1 },
    { "<Control>"       "KP_Multiply",          "change-base-octave",      +1 },
    /* decrement base octave */
    { "<Control>"       "minus",                "change-base-octave",      -1 },
    { "<Control>"       "underscore",           "change-base-octave",      -1 },
    { "<Control>"       "KP_Divide",            "change-base-octave",      -1 },
    { "<Control>"       "KP_Subtract",          "change-base-octave",      -1 },
    // keypad: "KP_Insert"/*0*/, "KP_Delete"/*,*/
  };
  static BstKeyBinding kbinding = { (char*) "pattern-controller-default-generic-keys", };
  if (!kbinding.n_funcs)
    {
      kbinding.funcs = pattern_controller_get_functions (FALSE, &kbinding.n_funcs);
      // setup default keys
      Bst::KeyBindingItemSeq iseq;
      for (size_t i = 0; i < G_N_ELEMENTS (dflt_keys); i++)
        iseq.push_back (key_binding_item_from_static (dflt_keys[i]));
      bst_key_binding_set_it3m_seq (&kbinding, iseq);
    }
  return &kbinding;
}

BstKeyBinding*
bst_pattern_controller_default_piano_keys (void)
{
  static StaticKeyBindingItem dflt_keys[] = {
    /* events */
    {                   "space",                "remove-events",        0 },
  };
  static BstKeyBinding kbinding = { (char*) "pattern-controller-default-piano-keys", };
  if (!kbinding.n_funcs)
    {
      kbinding.funcs = pattern_controller_get_functions (TRUE, &kbinding.n_funcs);
      Bst::KeyBindingItemSeq iseq;
      // setup default keys
      for (size_t i = 0; i < G_N_ELEMENTS (dflt_keys); i++)
        iseq.push_back (key_binding_item_from_static (dflt_keys[i]));
      bst_key_binding_set_it3m_seq (&kbinding, iseq);
    }
  return &kbinding;
}

BstKeyBinding*
bst_pattern_controller_generic_keys (void)
{
  static BstKeyBinding kbinding = { (char*) "pattern-controller-generic-keys", };
  if (!kbinding.n_funcs)
    {
      BstKeyBinding *dflt_kbinding = bst_pattern_controller_default_generic_keys();
      kbinding.funcs = pattern_controller_get_functions (FALSE, &kbinding.n_funcs);
      // copy keys
      Bst::KeyBindingItemSeq *iseq = bst_key_binding_get_new_it3m_seq (dflt_kbinding);
      bst_key_binding_set_it3m_seq (&kbinding, *iseq);
      delete iseq;
    }
  return &kbinding;
}

BstKeyBinding*
bst_pattern_controller_piano_keys (void)
{
  static BstKeyBinding kbinding = { (char*) "pattern-controller-piano-keys", };
  if (!kbinding.n_funcs)
    {
      BstKeyBinding *dflt_kbinding = bst_pattern_controller_default_piano_keys();
      kbinding.funcs = pattern_controller_get_functions (TRUE, &kbinding.n_funcs);
      // copy keys
      Bst::KeyBindingItemSeq *iseq = bst_key_binding_get_new_it3m_seq (dflt_kbinding);
      bst_key_binding_set_it3m_seq (&kbinding, *iseq);
      delete iseq;
    }
  return &kbinding;
}
