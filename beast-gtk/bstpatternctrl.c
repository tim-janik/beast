/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstpatternctrl.h"
#include "bstkeybindings.h"
#include <gdk/gdkkeysyms.h>
#include <string.h>


/* --- prototypes --- */
static gboolean pattern_controller_key_press    (BstPatternController   *self,
                                                 GdkEventKey            *event);


/* --- functions --- */
BstPatternController*
bst_pattern_controller_new (BstPatternView         *pview,
                            GxkActionGroup         *quant_rtools)
{
  BstPatternController *self;

  g_return_val_if_fail (BST_IS_PATTERN_VIEW (pview), NULL);

  self = g_new0 (BstPatternController, 1);
  self->pview = pview;
  self->ref_count = 1;

  self->ref_count++;
  g_signal_connect_data (pview, "key-press-event",
                         G_CALLBACK (pattern_controller_key_press),
                         self, (GClosureNotify) bst_pattern_controller_unref,
                         G_CONNECT_SWAPPED);
  self->quant_rtools = quant_rtools ? g_object_ref (quant_rtools) : NULL;

  gxk_scroll_canvas_set_canvas_cursor (GXK_SCROLL_CANVAS (pview), GDK_XTERM);

  return self;
}

BstPatternController*
bst_pattern_controller_ref (BstPatternController   *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ref_count >= 1, NULL);

  self->ref_count++;

  return self;
}

void
bst_pattern_controller_unref (BstPatternController   *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count >= 1);

  self->ref_count--;
  if (!self->ref_count)
    {
      if (self->quant_rtools)
        {
          gxk_action_group_dispose (self->quant_rtools);
          g_object_unref (self->quant_rtools);
        }
      g_free (self);
    }
}

static void
pattern_controller_move_focus (BstPatternController   *self,
                               BstPatternFunction      ftype,
                               guint                  *focus_col_p,
                               guint                  *focus_row_p)
{
  const guint channel_page = 2, row_page = 4;
  const guint note_wrap = TRUE;
  BstPatternView *pview = self->pview;
  gint focus_row = *focus_row_p;
  gint focus_col = *focus_col_p;

  /* movement */
  switch (ftype)
    {
    case BST_PATTERN_MOVE_LEFT:         focus_col--;                                        break;
    case BST_PATTERN_MOVE_RIGHT:        focus_col++;                                        break;
    case BST_PATTERN_MOVE_UP:           focus_row--;                                        break;
    case BST_PATTERN_MOVE_DOWN:         focus_row++;                                        break;
    case BST_PATTERN_PAGE_LEFT:         focus_col -= channel_page;                          break;
    case BST_PATTERN_PAGE_RIGHT:        focus_col += channel_page;                          break;
    case BST_PATTERN_PAGE_UP:           focus_row -= row_page;                              break;
    case BST_PATTERN_PAGE_DOWN:         focus_row += row_page;                              break;
    case BST_PATTERN_JUMP_LEFT:         focus_col = 0;                                      break;
    case BST_PATTERN_JUMP_RIGHT:        focus_col = pview->n_focus_cols - 1;                break;
    case BST_PATTERN_JUMP_TOP:          focus_row = 0;                                      break;
    case BST_PATTERN_JUMP_BOTTOM:       focus_row = bst_pattern_view_get_last_row (pview);  break;
    default: ;
    }
  /* wrapping */
  if (focus_col < 0)
    focus_col = note_wrap ? pview->n_focus_cols - 1 : 0;
  if (focus_col >= pview->n_focus_cols)
    focus_col = note_wrap ? 0 : pview->n_focus_cols - 1;
  if (focus_row < 0)
    focus_row = 0;
  if (focus_row > bst_pattern_view_get_last_row (pview))
    focus_row = bst_pattern_view_get_last_row (pview);
  *focus_row_p = focus_row;
  *focus_col_p = focus_col;
}

static gboolean
pattern_controller_key_press (BstPatternController *self,
                              GdkEventKey          *event)
{
  BstPatternFunction ftype = bst_key_binding_lookup_id (bst_pattern_controller_bindings(),
                                                        event->keyval, event->state);
  if (ftype)
    {
      guint focus_col = self->pview->focus_col;
      guint focus_row = self->pview->focus_row;
      pattern_controller_move_focus (self, ftype, &focus_col, &focus_row);
      bst_pattern_view_set_focus (self->pview, focus_col, focus_row);
      return TRUE;
    }
  return FALSE;
}

BstKeyBinding*
bst_pattern_controller_default_bindings (void)
{
  static BstKeyBindingFunction pcfuncs[] = {
    { BST_PATTERN_MOVE_UP,      "test-func-note",  BST_KEY_BINDING_PARAM_NOTE,   "" },
    { BST_PATTERN_MOVE_UP,      "test-func-oct",   BST_KEY_BINDING_PARAM_OCTAVE, "" },
    { BST_PATTERN_MOVE_UP,      "test-func-perc",  BST_KEY_BINDING_PARAM_PERC,   "" },
    { BST_PATTERN_MOVE_UP,      "test-func-m1p1",  BST_KEY_BINDING_PARAM_m1_p1,  "" },
    { BST_PATTERN_MOVE_UP,      "test-func-0p1",   BST_KEY_BINDING_PARAM_0_p1,   "" },
    { BST_PATTERN_MOVE_UP,      "test-func-m10",   BST_KEY_BINDING_PARAM_m1_0,   "" },
    { BST_PATTERN_MOVE_UP,      "move-up",      0, N_("Move focus cell upwards") },
    { BST_PATTERN_MOVE_LEFT,    "move-left",    0, N_("Move focus cell to the left") },
    { BST_PATTERN_MOVE_RIGHT,   "move-right",   0, N_("Move focus cell to the right") },
    { BST_PATTERN_MOVE_DOWN,    "move-down",    0, N_("Move focus cell downwards") },
    { BST_PATTERN_PAGE_UP,      "page-up",      0, N_("Move focus cell upwards page-wise") },
    { BST_PATTERN_PAGE_LEFT,    "page-left",    0, N_("Move focus cell to the left page-wise") },
    { BST_PATTERN_PAGE_RIGHT,   "page-right",   0, N_("Move focus cell to the right page-wise") },
    { BST_PATTERN_PAGE_DOWN,    "page-down",    0, N_("Move focus cell downwards page-wise") },
    { BST_PATTERN_JUMP_TOP,     "jump-top",     0, N_("Set the focus cell to the topmost position possible") },
    { BST_PATTERN_JUMP_LEFT,    "jump-left",    0, N_("Set the focus cell to the leftmost position possible") },
    { BST_PATTERN_JUMP_RIGHT,   "jump-right",   0, N_("Set the focus cell to the rightmost position possible") },
    { BST_PATTERN_JUMP_BOTTOM,  "jump-bottom",  0, N_("Set the focus cell to the bottommost position possible") },
  };
  static BstKeyBindingItem defkeys[] = {
    { "Up",    "move-up", 0 },
    { "Left",  "move-left", 0 },
    { "Right", "move-right", 0 },
    { "Down",  "move-down", 0 },
  };
  static BstKeyBinding kbinding = {
    "pattern-controller-default-keys",
    G_N_ELEMENTS (pcfuncs), pcfuncs,
  };
  static gboolean initialized = 0;
  if (!initialized)
    {
      BstKeyBindingItemSeq *iseq;
      guint i;
      initialized = TRUE;
      /* translate function blurbs */
      for (i = 0; i < kbinding.n_funcs; i++)
        pcfuncs[i].function_blurb = _(pcfuncs[i].function_blurb);
      /* setup default keys */
      iseq = bst_key_binding_item_seq_new();
      for (i = 0; i < G_N_ELEMENTS (defkeys); i++)
        bst_key_binding_item_seq_append (iseq, &defkeys[i]);
      bst_key_binding_set_item_seq (&kbinding, iseq);
      bst_key_binding_item_seq_free (iseq);
    }
  return &kbinding;
}

BstKeyBinding*
bst_pattern_controller_bindings (void)
{
  static BstKeyBinding kbinding = { "pattern-controller-keys" };
  if (!kbinding.n_funcs)
    {
      BstKeyBinding *defkb = bst_pattern_controller_default_bindings();
      BstKeyBindingItemSeq *iseq;
      /* copy functions */
      kbinding.n_funcs = defkb->n_funcs;
      kbinding.funcs = defkb->funcs;
      /* copy keys */
      iseq = bst_key_binding_get_item_seq (defkb);
      bst_key_binding_set_item_seq (&kbinding, iseq);
      bst_key_binding_item_seq_free (iseq);
    }
  return &kbinding;
}
