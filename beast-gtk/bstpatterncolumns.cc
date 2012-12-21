/* BEAST - Better Audio System
 * Copyright (C) 2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bstpatterncolumns.h"
#include "bstpatternview.h"
#include <string.h>
#include <math.h>


/* --- defines --- */
/* checks */
#define ISSPACE(c)      ((c) == ' ' || ((c) >= '\t' && (c) <= '\r'))
#define ISDIGIT(c)      ((c) >= '0' && (c) <= '9')
#define ISHEXDIGIT(c)   (ISDIGIT (c) || ((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
/* accessors */
#define STYLE(self)             (GTK_WIDGET (self)->style)
#define STATE(self)             (GTK_WIDGET (self)->state)
#define SELECTED_STATE(self)    (GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_SELECTED : GTK_STATE_INSENSITIVE)
#define ACTIVE_STATE(self)      (GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_ACTIVE : GTK_STATE_INSENSITIVE)
#define XTHICKNESS(self)        (STYLE (self)->xthickness)
#define YTHICKNESS(self)        (STYLE (self)->ythickness)
#define FOCUS_WIDTH(self)       (bst_pattern_view_get_focus_width (self))
#define X_OFFSET(self)          (GXK_SCROLL_CANVAS (self)->x_offset)
#define Y_OFFSET(self)          (GXK_SCROLL_CANVAS (self)->y_offset)


/* --- functions --- */
static void
playout_calc_char_extents (PangoLayout *pango_layout,
                           gint        *char_offset, /* to center ink-rect within logical-rect */
                           gint        *char_width,
                           const gchar *chars,
                           gboolean     reduce_to_ink_width)
{
  int mlwidth = 0, miwidth = 0;
  while (chars[0])
    {
      PangoRectangle irect = { 0, }, lrect = { 0 };
      pango_layout_set_text (pango_layout, chars++, 1);
      pango_layout_get_pixel_extents (pango_layout, &irect, &lrect);
      miwidth = MAX (miwidth, irect.width);
      mlwidth = MAX (mlwidth, lrect.width);
    }
  if (reduce_to_ink_width)
    {
      if (char_width)
        *char_width = miwidth;
      if (char_offset)
        *char_offset = (miwidth - mlwidth) / 2;
    }
  else
    {
      if (char_width)
        *char_width = mlwidth;
      if (char_offset)
        *char_offset = 0;
    }
}


/* --- note cell --- */
typedef struct {
  BstPatternColumn column;
  gint co0, co1, co2, co3;      /* char offsets */
  gint cw0, cw1, cw2, cw3;      /* char widths */
} BstPatternColumnNote;

static PangoFontDescription*
pattern_column_note_create_font_desc (BstPatternColumn *self)
{
  PangoFontDescription *fdesc = pango_font_description_new ();
  pango_font_description_set_family_static (fdesc, "monospace");
  pango_font_description_set_weight (fdesc, PANGO_WEIGHT_BOLD);
  return fdesc;
}

static guint
pattern_column_note_width_request (BstPatternColumn       *column,
                                   BstPatternView         *pview,
                                   GdkWindow              *drawable,
                                   PangoLayout            *pango_layout,
                                   guint                   duration)
{
  BstPatternColumnNote *self = (BstPatternColumnNote*) column;
  int width;
  playout_calc_char_extents (pango_layout, &self->co0, &self->cw0, "*-# ", TRUE);
  self->co0 += 1;
  self->cw0 += 1 + 1;
  playout_calc_char_extents (pango_layout, &self->co1, &self->cw1, "*-CDEFGABH", TRUE);
  self->cw1 += 1;
  playout_calc_char_extents (pango_layout, &self->co2, &self->cw2, "*-+ ", TRUE);
  self->cw2 += 1;
  playout_calc_char_extents (pango_layout, &self->co3, &self->cw3, "*- 123456", TRUE);
  width = self->cw0 + self->cw1 + self->cw2 + self->cw3;
  return FOCUS_WIDTH (pview) + width + XTHICKNESS (pview) + FOCUS_WIDTH (pview);
}

static char
pattern_column_note_char (BstPatternColumnNote *self,
                          BsePartNoteSeq       *pseq,
                          gint                  pos)
{
  BseNoteDescription *ndesc;
  if (!pseq || !pseq->n_pnotes)
    return '-';
  else if (pseq->n_pnotes > 1)
    return '*';
  /* pseq->n_pnotes == 1 */
  ndesc = bse_note_describe (BSE_MUSICAL_TUNING_12_TET, /* tuning is irrelevant if we ignore ->freq */
                             pseq->pnotes[0]->note, pseq->pnotes[0]->fine_tune);
  switch (pos)
    {
    case 0:     return ndesc->upshift ? '#' : ' ';
    case 1:     return ndesc->letter;
    case 2:     return ndesc->octave <= 0 ? '-' : '+';
    case 3:     return ABS (ndesc->octave) + '0';
    default:    return '-';     /* not reached */
    }
}

static void
pattern_column_note_draw_cell (BstPatternColumn       *column,
                               BstPatternView         *pview,
                               GdkWindow              *drawable,
                               PangoLayout            *pango_layout,
                               guint                   tick,
                               guint                   duration,
                               GdkRectangle           *crect,
                               GdkRectangle           *expose_area,
                               GdkGC                  *gcs[BST_PATTERN_COLUMN_GC_LAST])
{
  BstPatternColumnNote *self = (BstPatternColumnNote*) column;
  PangoRectangle prect = { 0 };
  GdkGC *inactive_gc = gcs[BST_PATTERN_COLUMN_GC_TEXT0];
  GdkGC *text_gc = gcs[BST_PATTERN_COLUMN_GC_TEXT1];
  GdkGC *draw_gc;
  SfiProxy proxy = pview->proxy;
  BsePartNoteSeq *pseq = proxy ? bse_part_list_notes_within (proxy, column->num, tick, duration) : NULL;
  gchar ch;
  gint accu, yline;

  accu = crect->x + FOCUS_WIDTH (pview);
  ch = pattern_column_note_char (self, pseq, 0);
  draw_gc = ch == '-' ? inactive_gc : text_gc;      /* choose gc dependant on note letter */
  pango_layout_set_text (pango_layout, &ch, 1);
  pango_layout_get_pixel_extents (pango_layout, NULL, &prect);
  yline = crect->y + (crect->height - prect.height) / 2;
  gdk_draw_layout (drawable, draw_gc, accu + self->co0 + (self->cw0 - prect.width) / 2, yline, pango_layout);

  accu += self->cw0;
  ch = pattern_column_note_char (self, pseq, 1);
  pango_layout_set_text (pango_layout, &ch, 1);
  pango_layout_get_pixel_extents (pango_layout, NULL, &prect);
  gdk_draw_layout (drawable, draw_gc, accu + self->co1 + (self->cw1 - prect.width) / 2, yline, pango_layout);

  accu += self->cw1;
  ch = pattern_column_note_char (self, pseq, 2);
  pango_layout_set_text (pango_layout, &ch, 1);
  pango_layout_get_pixel_extents (pango_layout, NULL, &prect);
  gdk_draw_layout (drawable, draw_gc, accu + self->co2 + (self->cw2 - prect.width) / 2, yline, pango_layout);

  accu += self->cw2;
  ch = pattern_column_note_char (self, pseq, 3);
  pango_layout_set_text (pango_layout, &ch, 1);
  pango_layout_get_pixel_extents (pango_layout, NULL, &prect);
  gdk_draw_layout (drawable, draw_gc, accu + self->co3 + (self->cw3 - prect.width) / 2, yline, pango_layout);
}

static gboolean
pattern_column_note_key_event (BstPatternColumn       *column,
                               BstPatternView         *pview,
                               GdkWindow              *drawable,
                               PangoLayout            *pango_layout,
                               guint                   tick,
                               guint                   duration,
                               GdkRectangle           *cell_rect,
                               gint                    focus_pos,
                               guint                   keyval,
                               GdkModifierType         modifier,
                               BstPatternFunction      action,
                               gdouble                 param,
                               BstPatternFunction     *movement)
{
  // BstPatternColumnNote *self = (BstPatternColumnNote*) column;
  SfiProxy proxy = pview->proxy;
  BsePartNoteSeq *pseq = proxy ? bse_part_list_notes_within (proxy, column->num, tick, duration) : NULL;
  guint i, iparam = 0.5 + param;
  switch (action)
    {
    case BST_PATTERN_REMOVE_EVENTS:
      bse_item_group_undo (proxy, "Remove Events");
      for (i = 0; i < pseq->n_pnotes; i++)
        bse_part_delete_event (proxy, pseq->pnotes[i]->id);
      bse_item_ungroup_undo (proxy);
      return TRUE;
    case BST_PATTERN_SET_NOTE:
      bse_item_group_undo (proxy, "Set Note");
      if (pseq->n_pnotes == 1)
        {
          BsePartNote *pnote = pseq->pnotes[0];
          bse_part_change_note (proxy, pnote->id, pnote->tick, pnote->duration,
                                SFI_NOTE_CLAMP (iparam), pnote->fine_tune, pnote->velocity);
        }
      else if (pseq->n_pnotes <= 1)
        bse_part_insert_note (proxy, column->num, tick, duration, SFI_NOTE_CLAMP (iparam), 0, +1);
      else
        bst_gui_error_bell (pview);
      bse_item_ungroup_undo (proxy);
      return TRUE;
    default: ;
    }
  return FALSE;
}

static void
pattern_column_note_finalize (BstPatternColumn *column)
{
  BstPatternColumnNote *self = (BstPatternColumnNote*) column;
  g_free (self);
}

static BstPatternColumnClass pattern_column_note_class = {
  1,    /* n_focus_positions */
  sizeof (BstPatternColumnNote),
  NULL, /* init */
  pattern_column_note_create_font_desc,
  pattern_column_note_width_request,
  pattern_column_note_draw_cell,
  NULL, /* get_focus_pos */
  1,    /* collision_group */
  pattern_column_note_key_event,
  pattern_column_note_finalize,
};


/* --- event cell --- */
typedef struct {
  BstPatternColumn column;
  gint co;      /* character offset to translate ink-rect to logical-rect */
  gint cw;      /* character width */
} BstPatternColumnEvent;

static void
pattern_column_event_init (BstPatternColumn *column)
{
  gboolean is_signed  = (column->lflags & BST_PATTERN_LFLAG_SIGNED) != 0;
  gint n_digits = 1 + (column->lflags & BST_PATTERN_LFLAG_DIGIT_MASK);
  column->n_focus_positions = is_signed + n_digits;
}

static PangoFontDescription*
pattern_column_event_create_font_desc (BstPatternColumn *self)
{
  PangoFontDescription *fdesc = pango_font_description_new ();
  pango_font_description_set_family_static (fdesc, "monospace");
  pango_font_description_set_weight (fdesc, PANGO_WEIGHT_BOLD);
  return fdesc;
}

static guint
pattern_column_event_width_request (BstPatternColumn       *column,
                                    BstPatternView         *pview,
                                    GdkWindow              *drawable,
                                    PangoLayout            *pango_layout,
                                    guint                   duration)
{
  BstPatternColumnEvent *self = (BstPatternColumnEvent*) column;
  gboolean is_signed  = (column->lflags & BST_PATTERN_LFLAG_SIGNED) != 0;
  gint width, n_digits = 1 + (column->lflags & BST_PATTERN_LFLAG_DIGIT_MASK);
  playout_calc_char_extents (pango_layout, &self->co, &self->cw, "0123456789", TRUE);
  width = 1 + (is_signed + n_digits) * (self->cw + 1);
  return FOCUS_WIDTH (pview) + width + FOCUS_WIDTH (pview);
}

static guint
control_get_max (guint num_type,
                 guint n_digits)
{
  if (num_type == BST_PATTERN_LFLAG_DEC)
    return pow (10, n_digits) - 1;
  else /* if (num_type == BST_PATTERN_LFLAG_HEX) */
    return pow (16, n_digits) - 1;
}

static guint
control_get_digit_increment (guint num_type,
                             guint nth_digits)
{
  g_assert (nth_digits > 0);
  if (num_type == BST_PATTERN_LFLAG_DEC)
    return pow (10, nth_digits - 1);
  else /* if (num_type == BST_PATTERN_LFLAG_HEX) */
    return pow (16, nth_digits - 1);
}

static guint
pattern_column_event_to_string (BstPatternColumn *column,
                                gchar             buffer[64],
                                BsePartControl   *pctrl,
                                gchar             placeholder,
                                int              *ivalue_p)
{
  gboolean is_signed  = (column->lflags & BST_PATTERN_LFLAG_SIGNED) != 0;
  guint num_type = column->lflags & BST_PATTERN_LFLAG_NUM_MASK;
  guint n_digits = MIN (9, 1 + (column->lflags & BST_PATTERN_LFLAG_DIGIT_MASK));
  if (placeholder)
    {
      memset (buffer, placeholder, 64);
      buffer[n_digits + is_signed] = 0;
      if (ivalue_p)
        *ivalue_p = 0;
    }
  else
    {
      const gchar *format;
      gchar *p = buffer;
      gint ival = control_get_max (num_type, n_digits) * pctrl->value;
      if (num_type == BST_PATTERN_LFLAG_DEC)
        {
          static const char *formats[] = { "%1u", "%2u", "%3u", "%4u", "%5u", "%6u", "%7u", "%8u", "%9u" };
          format = formats[n_digits - 1];
        }
      else /* if (num_type == BST_PATTERN_LFLAG_HEX) */
        {
          static const char *formats[] = { "%01x", "%02x", "%03x", "%04x", "%05x", "%06x", "%07x", "%08x", "%09x" };
          format = formats[n_digits - 1];
        }
      if (is_signed)
        *p++ = ival == 0 ? ' ' : ival > 0 ? '+' : '-';
      g_snprintf (p, 63, format, ABS (ival));
      if (ivalue_p)
        *ivalue_p = ival;
    }
  return is_signed + n_digits;
}

static gfloat
pattern_column_event_value_from_int (BstPatternColumn *column,
                                     gint              ivalue)
{
  guint num_type = column->lflags & BST_PATTERN_LFLAG_NUM_MASK;
  guint n_digits = MIN (9, 1 + (column->lflags & BST_PATTERN_LFLAG_DIGIT_MASK));
  double value = ivalue;
  value /= control_get_max (num_type, n_digits);
  /* to avoid rounding artefacts, we need to fudge values away from zero.
   * since our effective input precision is 16bit (0xffff) and pristine
   * float precision is 23 bit, we fudge around the 20th bit.
   */
  value += value < 0 ? -0.000001 : +0.000001;
  return CLAMP (value, -1, +1);
}

static gint
pattern_column_event_ivalue_from_string (BstPatternColumn *column,
                                         const gchar      *string)
{
  guint num_type = column->lflags & BST_PATTERN_LFLAG_NUM_MASK;
  guint n_digits = MIN (9, 1 + (column->lflags & BST_PATTERN_LFLAG_DIGIT_MASK));
  gboolean is_hex = num_type == BST_PATTERN_LFLAG_HEX;
  gchar *stripped = g_strdup (string), *s = stripped;
  gint ival, i, vmax = control_get_max (num_type, n_digits);
  for (i = 0; string[i]; i++)
    if (!ISSPACE (string[i]))
      *s++ = string[i];
  *s = 0;
  ival = g_ascii_strtoull (stripped, NULL, is_hex ? 16 : 10);
  g_free (stripped);
  return CLAMP (ival, -vmax, vmax);
}

static gfloat
pattern_column_event_value_from_string (BstPatternColumn *column,
                                        const gchar      *string)
{
  gint ival = pattern_column_event_ivalue_from_string (column, string);
  return pattern_column_event_value_from_int (column, ival);
}

static BseMidiSignalType
pattern_column_control_type (BstPatternColumn *column, bool *isnote_p)
{
  BseMidiSignalType control_type;
  bool isnote = true;
  if (column->ltype == BST_PATTERN_LTYPE_VELOCITY)
    control_type = BSE_MIDI_SIGNAL_VELOCITY;
  else if (column->ltype == BST_PATTERN_LTYPE_FINE_TUNE)
    control_type = BSE_MIDI_SIGNAL_FINE_TUNE;
  else
    {
      control_type = BseMidiSignalType (BSE_MIDI_SIGNAL_CONTINUOUS_0 + column->num);
      isnote = false;
    }
  if (isnote_p)
    *isnote_p = isnote;
  return control_type;
}

static BsePartControl*
pattern_column_event_lookup (BstPatternColumn   *column,
                             BstPatternView     *pview,
                             guint               tick,
                             guint               duration,
                             BsePartControlSeq **cseq_p,
                             gchar              *placeholder_p)
{
  BsePartControl *pctrl = NULL;
  BsePartControlSeq *cseq;
  guint control_type = pattern_column_control_type (column, NULL);
  cseq = bse_part_get_channel_controls (pview->proxy, column->num, tick, duration, BseMidiSignalType (control_type));
  if ((!cseq || cseq->n_pcontrols < 1) && placeholder_p)
    *placeholder_p = '-';
  else if (cseq && cseq->n_pcontrols == 1)
    {
      pctrl = cseq->pcontrols[0];
      if (placeholder_p)
        *placeholder_p = 0;
    }
  else if (cseq && cseq->n_pcontrols > 1 && placeholder_p)
    *placeholder_p = '*';
  if (cseq_p)
    *cseq_p = cseq;
  return pctrl;
}

static void
pattern_column_event_draw_cell (BstPatternColumn       *column,
                                BstPatternView         *pview,
                                GdkWindow              *drawable,
                                PangoLayout            *pango_layout,
                                guint                   tick,
                                guint                   duration,
                                GdkRectangle           *crect,
                                GdkRectangle           *expose_area,
                                GdkGC                  *gcs[BST_PATTERN_COLUMN_GC_LAST])
{
  BstPatternColumnEvent *self = (BstPatternColumnEvent*) column;
  GdkGC *inactive_gc = gcs[BST_PATTERN_COLUMN_GC_TEXT0];
  GdkGC *text_gc = gcs[BST_PATTERN_COLUMN_GC_TEXT1];
  gchar placeholder = 0;
  BsePartControl *pctrl = pattern_column_event_lookup (column, pview, tick, duration, NULL, &placeholder);
  gchar buffer[64] = { 0, };
  int n = pattern_column_event_to_string (column, buffer, pctrl, placeholder, NULL);
  GdkGC *draw_gc = placeholder == '-' ? inactive_gc : text_gc;
  int yline, accu = crect->x + FOCUS_WIDTH (pview) + 1;

  for (int i = 0; i < n; i++)
    {
      PangoRectangle irect, prect;
      pango_layout_set_text (pango_layout, buffer + i, 1);
      pango_layout_get_pixel_extents (pango_layout, &irect, &prect);
      yline = crect->y + (crect->height - prect.height) / 2;
      gdk_draw_layout (drawable, draw_gc, accu + self->co + (self->cw - prect.width) / 2, yline, pango_layout);
      accu += self->cw + 1;
    }
}

static void
pattern_column_event_get_focus_pos (BstPatternColumn       *column,
                                    BstPatternView         *pview,
                                    GdkWindow              *drawable,
                                    PangoLayout            *pango_layout,
                                    guint                   tick,
                                    guint                   duration,
                                    GdkRectangle           *crect,
                                    gint                    focus_pos,
                                    gint                   *pos_x,
                                    gint                   *pos_width)
{
  BstPatternColumnEvent *self = (BstPatternColumnEvent*) column;
  gint i, accu, width = self->cw + 1;
  accu = FOCUS_WIDTH (pview) + 1;
  for (i = 0; i < focus_pos; i++)
    accu += width;
  *pos_x = accu;
  *pos_width = width;
}

static gboolean
pattern_column_event_key_event (BstPatternColumn       *column,
                                BstPatternView         *pview,
                                GdkWindow              *drawable,
                                PangoLayout            *pango_layout,
                                guint                   tick,
                                guint                   duration,
                                GdkRectangle           *cell_rect,
                                gint                    focus_pos,
                                guint                   keyval,
                                GdkModifierType         modifier,
                                BstPatternFunction      action,
                                gdouble                 param,
                                BstPatternFunction     *movement)
{
  guint num_type = column->lflags & BST_PATTERN_LFLAG_NUM_MASK;
  guint n_digits = MIN (9, 1 + (column->lflags & BST_PATTERN_LFLAG_DIGIT_MASK));
  gboolean is_signed = (column->lflags & BST_PATTERN_LFLAG_SIGNED) != 0;
  SfiProxy proxy = pview->proxy;
  gchar placeholder = 0;
  BsePartControlSeq *cseq;
  BsePartControl *pctrl = pattern_column_event_lookup (column, pview, tick, duration, &cseq, &placeholder);
  gchar buffer[64] = { 0, };
  int ivalue, handled = FALSE;
  pattern_column_event_to_string (column, buffer, pctrl, placeholder, &ivalue);
  if (action == BST_PATTERN_REMOVE_EVENTS)
    {
      guint i;
      bse_item_group_undo (proxy, "Remove Events");
      for (i = 0; i < (cseq ? cseq->n_pcontrols : 0); i++)
        bse_part_delete_event (proxy, cseq->pcontrols[i]->id);
      bse_item_ungroup_undo (proxy);
      handled = TRUE;
    }
  else if (action == BST_PATTERN_SET_DIGIT &&   /* insertions */
           (!cseq || cseq->n_pcontrols == 0))
    {
      guint digit = column->n_focus_positions - focus_pos;
      gint dmax = control_get_max (num_type, 1);
      bool isnote;
      BseMidiSignalType control_type = pattern_column_control_type (column, &isnote);
      gfloat value;
      ivalue = MIN (param, dmax) * control_get_digit_increment (num_type, digit);
      value = pattern_column_event_value_from_int (column, ivalue);
      if (!isnote && (!is_signed || focus_pos > 0))
        bse_part_insert_control (proxy, tick, control_type, value);
      else
        bst_gui_error_bell (pview);
    }
  else if (pctrl && focus_pos < (int) strlen (buffer) && !(modifier & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
    {
      gfloat value = 0;
      gchar *newstr = g_strdup (buffer);
      switch (action)
        {
        case BST_PATTERN_SET_DIGIT:
          if (!is_signed || focus_pos > 0)
            {
              guint digit = column->n_focus_positions - focus_pos;
              gint dmax = control_get_max (num_type, 1);
              if (ISHEXDIGIT (newstr[focus_pos]))
                {
                  newstr[focus_pos] = '0';
                  ivalue = pattern_column_event_ivalue_from_string (column, newstr);
                }
              ivalue += MIN (param, dmax) * control_get_digit_increment (num_type, digit);
              value = pattern_column_event_value_from_int (column, ivalue);
            }
          handled = TRUE;
          break;
        case BST_PATTERN_NUMERIC_CHANGE:
          if (is_signed && focus_pos == 0)
            {
              newstr[focus_pos] = param < 0 ? '-' : '+';
              value = pattern_column_event_value_from_string (column, newstr);
            }
          else
            {
              guint digit = column->n_focus_positions - focus_pos;
              gint newvalue = ivalue + param * control_get_digit_increment (num_type, digit);
              gint dmax = control_get_max (num_type, n_digits);
              if (ABS (newvalue) <= dmax && (is_signed || newvalue >= 0))
                ivalue = newvalue;
              value = pattern_column_event_value_from_int (column, ivalue);
            }
          handled = TRUE;
          break;
        default: ;
        }
      if (handled)
        bse_part_change_control (proxy, pctrl->id, pctrl->tick, pctrl->control_type, value);
      g_free (newstr);
    }
  return handled;
}

static void
pattern_column_event_finalize (BstPatternColumn *column)
{
  BstPatternColumnEvent *self = (BstPatternColumnEvent*) column;
  g_free (self);
}

static BstPatternColumnClass pattern_column_event_class = {
  1,    /* n_focus_positions */
  sizeof (BstPatternColumnEvent),
  pattern_column_event_init,
  pattern_column_event_create_font_desc,
  pattern_column_event_width_request,
  pattern_column_event_draw_cell,
  pattern_column_event_get_focus_pos,
  2,    /* collision_group */
  pattern_column_event_key_event,
  pattern_column_event_finalize,
};


/* --- vbar cell --- */
static guint
pattern_column_vbar_width_request (BstPatternColumn       *self,
                                   BstPatternView         *pview,
                                   GdkWindow              *drawable,
                                   PangoLayout            *pango_layout,
                                   guint                   duration)
{
  if (self->num)        /* vbar */
    return 1 + 1 + 1;
  /* space */
  return 1 + 1; // 2 * XTHICKNESS (pview) + 1;
}

static void
pattern_column_vbar_draw_cell (BstPatternColumn       *self,
                               BstPatternView         *pview,
                               GdkWindow              *drawable,
                               PangoLayout            *pango_layout,
                               guint                   tick,
                               guint                   duration,
                               GdkRectangle           *crect,
                               GdkRectangle           *expose_area,
                               GdkGC                  *gcs[BST_PATTERN_COLUMN_GC_LAST])
{
  GdkGC *draw_gc = gcs[BST_PATTERN_COLUMN_GC_VBAR];
  gint dlen, line_width = 0; /* line widths != 0 interfere with dash-settings on some X servers */
  if (self->num < 0)
    {
      guint8 dash[3] = { 2, 2, 0 };
      gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
      dlen = dash[0] + dash[1];
      gdk_gc_set_dashes (draw_gc, (Y_OFFSET (pview) + crect->y + 1) % dlen, (gint8*) dash, 2);
    }
  if (self->num)
    gdk_draw_line (drawable, draw_gc,
                   crect->x + crect->width / 2, crect->y,
                   crect->x + crect->width / 2, crect->y + crect->height - 1);
  if (self->num < 0)
    gdk_gc_set_line_attributes (draw_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
}

static void
pattern_column_vbar_finalize (BstPatternColumn *self)
{
  g_free (self);
}

static BstPatternColumnClass pattern_column_vbar_class = {
  0,  /* n_focus_positions */
  sizeof (BstPatternColumn),
  NULL, /* init */
  NULL, /* create_font_desc */
  pattern_column_vbar_width_request,
  pattern_column_vbar_draw_cell,
  NULL, /* get_focus_pos */
  0,    /* collision_group */
  NULL, /* key_event */
  pattern_column_vbar_finalize,
};


/* --- layout configuration --- */
static void
pattern_column_layouter_apply (GtkWidget *dialog)
{
  BstPatternView *pview = (BstPatternView*) g_object_get_data ((GObject*) dialog, "user_data");
  GtkEntry *entry = (GtkEntry*) gxk_radget_find (dialog, "layout-entry");
  const gchar *layout = gtk_entry_get_text (entry);
  guint l = bst_pattern_view_set_layout (pview, layout);
  if (l < strlen (layout))
    {
      gtk_editable_select_region (GTK_EDITABLE (entry), l, -1);
      bst_gui_error_bell (dialog);
    }
}

#define __(str)         str /* leave string unstranslated */

void
bst_pattern_column_layouter_popup (BstPatternView *pview)
{
  static const gchar *help_text[] = {
    N_("The pattern editor column layout is specified by listing column types "
       "with possible modifiers in display order."), "\n\n",
    N_("COLUMN TYPES:"), "\n",
    __("note-1, note-2, ..."), "\n",
    N_("  display notes of the first, second, ... channel"), "\n",
    __("offset-1, length-2, velocity-3, ..."), "\n",
    N_("  display offset, length or velocity of notes in the first, second, ... channel"), "\n",
    __("control-0, ..., control-63, cc-0, cc-31"), "\n",
    N_("  select various event types (controls, continuous controllers)"), "\n",
    __("bar, |"), "\n",
    N_("  display solid vertical bar"), "\n",
    __("dbar, :"), "\n",
    N_("  display dotted vertical bar"), "\n",
    __("space, _"), "\n",
    N_("  insert vertical space"), "\n\n",
    N_("EVENTS:"), "\n",
    __("  balance, volume, cc-8, ..."), "\n\n",
    N_("MODIFIERS:"), "\n",
    __("hex2, hex4"), "\n",
    N_("  display 2 (00..FF) or 4 (0000..FFFF) digit hex numbers"), "\n",
    __("shex2, shex4"), "\n",
    N_("  display 2 (-FF..+FF) or 4 (-FFFF..+FFFF) digit signed hex numbers"), "\n",
    __("dec2, dec3"), "\n",
    N_("  display 2 (00..99) or 3 (000..999) digit decimal numbers"), "\n",
    __("sdec2, sdec3"), "\n",
    N_("  display 2 (-99..+99) or 3 (-999..+999) digit signed decimal numbers"), "\n",
    __("col1, col2, col3"), "\n",
    N_("  selects one of 3 predefined colors"), "\n",
    __("lfold, rfold"), "\n",
    N_("  allow folding the column into left/right neighbour"), "\n",
    N_("EXAMPLE:"), "\n",
    __("note-1 | velocity-1=hex2 | note-2 | volume=hex4"), "\n",
  };
  GtkWidget *dialog = (GtkWidget*) g_object_get_data ((GObject*) pview, "BstPattern-layouter");
  GtkEntry *entry;
  if (!dialog)
    {
      GtkWidget *sctext, *w;
      guint i;
      dialog = (GtkWidget*) gxk_dialog_new_radget (NULL, GTK_OBJECT (pview),
                                                   GXK_DIALOG_HIDE_ON_DELETE | GXK_DIALOG_POPUP_POS, // GXK_DIALOG_MODAL
                                                   _("Pattern Editor Layout"),
                                                   "beast", "pattern-editor-layout-box");
      sctext = (GtkWidget*) gxk_scroll_text_create_for (GXK_SCROLL_TEXT_WIDGET_LOOK, (GtkWidget*)  gxk_radget_find (dialog, "sctext-box"));
      gxk_scroll_text_clear (sctext);
      for (i = 0; i < G_N_ELEMENTS (help_text); i++)
        {
          const gchar *hx = help_text[i], *p = hx;
          while (*p == ' ')
            p++;
          if (p > hx)
            gxk_scroll_text_push_indent (sctext);
          const char *tx = _(hx);
          while (*tx == ' ')
            tx++;
          gxk_scroll_text_append (sctext, tx);
          if (p > hx)
            gxk_scroll_text_pop_indent (sctext);
        }
      g_signal_connect (gxk_radget_find (dialog, "cancel-button"), "clicked",
                        G_CALLBACK (gxk_toplevel_delete), NULL);
      w = (GtkWidget*)  gxk_radget_find (dialog, "apply-button");
      g_signal_connect_swapped (w, "clicked", G_CALLBACK (pattern_column_layouter_apply), dialog);
      gxk_dialog_set_default (GXK_DIALOG (dialog), w);
      g_object_set_data ((GObject*) dialog, "user_data", pview);
      g_object_set_data ((GObject*) pview, "BstPattern-layouter", dialog);
    }
  entry = (GtkEntry*) gxk_radget_find (dialog, "layout-entry");
  gtk_entry_set_text (entry, bst_pattern_view_get_layout (pview));
  gxk_widget_showraise (dialog);
}

const gchar*
bst_pattern_layout_parse_column (const gchar      *string,
                                 BstPatternLType  *ltype,
                                 gint             *num,
                                 BstPatternLFlags *flags)
{
  static const struct { const gchar *name; uint with_num; BstPatternLType type; } coltypes[] = {
    { "note-", 1,       BST_PATTERN_LTYPE_NOTE },
    { "offset-", 1,     BST_PATTERN_LTYPE_OFFSET },
    { "length-", 1,     BST_PATTERN_LTYPE_LENGTH },
    { "velocity-", 1,   BST_PATTERN_LTYPE_VELOCITY },
    { "fine-tune-", 1,  BST_PATTERN_LTYPE_FINE_TUNE },
    { "control-", 1,    BST_PATTERN_LTYPE_CONTROL },
    { "space", 0,       BST_PATTERN_LTYPE_SPACE },
    { "_", 0,           BST_PATTERN_LTYPE_SPACE },
    { "bar", 0,         BST_PATTERN_LTYPE_BAR },
    { "|", 0,           BST_PATTERN_LTYPE_BAR },
    { "dbar", 0,        BST_PATTERN_LTYPE_DBAR },
    { ":", 0,           BST_PATTERN_LTYPE_DBAR },
  };
  static const struct { const gchar *name; BstPatternLFlags flag, mask; } colflags[] = {
#define NUM_MASK        (BST_PATTERN_LFLAG_NUM_MASK | BST_PATTERN_LFLAG_SIGNED)
    { "hex1",           BST_PATTERN_LFLAG_HEX | BST_PATTERN_LFLAG_DIGIT_1, NUM_MASK },
    { "hex2",           BST_PATTERN_LFLAG_HEX | BST_PATTERN_LFLAG_DIGIT_2, NUM_MASK },
    { "hex3",           BST_PATTERN_LFLAG_HEX | BST_PATTERN_LFLAG_DIGIT_3, NUM_MASK },
    { "hex4",           BST_PATTERN_LFLAG_HEX | BST_PATTERN_LFLAG_DIGIT_4, NUM_MASK },
    { "shex1",          BST_PATTERN_LFLAG_HEX | BST_PATTERN_LFLAG_DIGIT_1 | BST_PATTERN_LFLAG_SIGNED, NUM_MASK },
    { "shex2",          BST_PATTERN_LFLAG_HEX | BST_PATTERN_LFLAG_DIGIT_2 | BST_PATTERN_LFLAG_SIGNED, NUM_MASK },
    { "shex3",          BST_PATTERN_LFLAG_HEX | BST_PATTERN_LFLAG_DIGIT_3 | BST_PATTERN_LFLAG_SIGNED, NUM_MASK },
    { "shex4",          BST_PATTERN_LFLAG_HEX | BST_PATTERN_LFLAG_DIGIT_4 | BST_PATTERN_LFLAG_SIGNED, NUM_MASK },
    { "dec1",           BST_PATTERN_LFLAG_DEC | BST_PATTERN_LFLAG_DIGIT_1, NUM_MASK },
    { "dec2",           BST_PATTERN_LFLAG_DEC | BST_PATTERN_LFLAG_DIGIT_2, NUM_MASK },
    { "dec3",           BST_PATTERN_LFLAG_DEC | BST_PATTERN_LFLAG_DIGIT_3, NUM_MASK },
    { "dec4",           BST_PATTERN_LFLAG_DEC | BST_PATTERN_LFLAG_DIGIT_4, NUM_MASK },
    { "sdec1",          BST_PATTERN_LFLAG_DEC | BST_PATTERN_LFLAG_DIGIT_1 | BST_PATTERN_LFLAG_SIGNED, NUM_MASK },
    { "sdec2",          BST_PATTERN_LFLAG_DEC | BST_PATTERN_LFLAG_DIGIT_2 | BST_PATTERN_LFLAG_SIGNED, NUM_MASK },
    { "sdec3",          BST_PATTERN_LFLAG_DEC | BST_PATTERN_LFLAG_DIGIT_3 | BST_PATTERN_LFLAG_SIGNED, NUM_MASK },
    { "sdec4",          BST_PATTERN_LFLAG_DEC | BST_PATTERN_LFLAG_DIGIT_4 | BST_PATTERN_LFLAG_SIGNED, NUM_MASK },
    { "col1",           BST_PATTERN_LFLAG_COL1, BST_PATTERN_LFLAG_COL_MASK },
    { "col2",           BST_PATTERN_LFLAG_COL2, BST_PATTERN_LFLAG_COL_MASK },
    { "col3",           BST_PATTERN_LFLAG_COL3, BST_PATTERN_LFLAG_COL_MASK },
    { "col4",           BST_PATTERN_LFLAG_COL4, BST_PATTERN_LFLAG_COL_MASK },
    { "lfold",          BST_PATTERN_LFLAG_LFOLD, BST_PATTERN_LFLAG_LFOLD },
    { "rfold",          BST_PATTERN_LFLAG_RFOLD, BST_PATTERN_LFLAG_RFOLD },
  };
  uint i, fmask = 0;
  *ltype = BST_PATTERN_LTYPE_SPACE;
  *num = 0;
  *flags = BstPatternLFlags (0);
  /* parse type */
  for (i = 0; i < G_N_ELEMENTS (coltypes); i++)
    if (strncmp (string, coltypes[i].name, strlen (coltypes[i].name)) == 0)
      {
        string += strlen (coltypes[i].name);
        *ltype = coltypes[i].type;
        break;
      }
  if (i >= G_N_ELEMENTS (coltypes))
    return string;      /* failed */
  /* parse number */
  if (coltypes[i].with_num)
    {
      const gchar *p = string;
      gchar *mem;
      while (ISDIGIT (string[0]))
        string++;
      if (string == p)
        return string;  /* failed */
      mem = (char*) g_memdup (p, string - p);
      *num = g_ascii_strtoull (mem, NULL, 10);
      g_free (mem);
    }
  /* parse flags */
  if (string[0] == '=')
    do
      {
        string++;
        for (i = 0; i < G_N_ELEMENTS (colflags); i++)
          if (strncmp (string, colflags[i].name, strlen (colflags[i].name)) == 0)
            {
              string += strlen (colflags[i].name);
              *flags &= ~colflags[i].mask;
              *flags |= colflags[i].flag;
              fmask |= colflags[i].mask;
              break;
            }
      }
    while (string[0] == ',');
  /* adjust defaults */
  if (!(fmask & (BST_PATTERN_LFLAG_NUM_MASK | BST_PATTERN_LFLAG_DIGIT_MASK)))
    *flags |= BST_PATTERN_LFLAG_SIGNED | BST_PATTERN_LFLAG_HEX | BST_PATTERN_LFLAG_DIGIT_2;
  return string;
}

BstPatternColumn*
bst_pattern_column_create (BstPatternLType   ltype,
                           gint              num,
                           BstPatternLFlags  lflags)
{
  BstPatternColumnClass *klass = NULL;
  BstPatternColumn *column;
  switch (ltype)
    {
    case BST_PATTERN_LTYPE_OFFSET:
    case BST_PATTERN_LTYPE_LENGTH:
    case BST_PATTERN_LTYPE_SPACE:
      klass = &pattern_column_vbar_class;
      num = 0;
      break;
    case BST_PATTERN_LTYPE_BAR:
      klass = &pattern_column_vbar_class;
      num = 1;
      break;
    case BST_PATTERN_LTYPE_DBAR:
      klass = &pattern_column_vbar_class;
      num = -1;
      break;
    case BST_PATTERN_LTYPE_NOTE:
      klass = &pattern_column_note_class;
      break;
    case BST_PATTERN_LTYPE_VELOCITY:
    case BST_PATTERN_LTYPE_FINE_TUNE:
    case BST_PATTERN_LTYPE_CONTROL:
      klass = &pattern_column_event_class;
      break;
    }
  g_assert (klass->instance_size >= sizeof (BstPatternColumn));
  column = (BstPatternColumn*) g_malloc0 (klass->instance_size);
  column->klass = klass;
  column->num = num;
  column->ltype = ltype;
  column->lflags = lflags;
  column->n_focus_positions = klass->n_focus_positions;
  if (klass->init)
    klass->init (column);
  return column;
}

gboolean
bst_pattern_column_has_notes (BstPatternColumn *column)
{
  return column->klass == &pattern_column_note_class;
}
