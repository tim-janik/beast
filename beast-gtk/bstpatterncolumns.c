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
#include "bstpatterncolumns.h"
#include "bstpatternview.h"
#include <string.h>


/* --- defines --- */
/* accessors */
#define STYLE(self)             (GTK_WIDGET (self)->style)
#define STATE(self)             (GTK_WIDGET (self)->state)
#define SELECTED_STATE(self)    (GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_SELECTED : GTK_STATE_INSENSITIVE)
#define ACTIVE_STATE(self)      (GTK_WIDGET_IS_SENSITIVE (self) ? GTK_STATE_ACTIVE : GTK_STATE_INSENSITIVE)
#define XTHICKNESS(self)        (STYLE (self)->xthickness)
#define YTHICKNESS(self)        (STYLE (self)->ythickness)
#define FOCUS_WIDTH(self)       (bst_pattern_view_get_focus_width (self))


/* --- note cell --- */
typedef struct {
  BstPatternColumn column;
  gint co0, co1, co2, co3;      /* char offsets */
  gint cw0, cw1, cw2, cw3;      /* char widths */
} BstPatternColumnNote;

static BstPatternColumn*
pattern_column_note_create (BstPatternColumnClass *class)
{
  BstPatternColumnNote *self = g_new0 (BstPatternColumnNote, 1);
  self->column.klass = class;
  return &self->column;
}

static PangoFontDescription*
pattern_column_note_create_font_desc (BstPatternColumn *self)
{
  PangoFontDescription *fdesc = pango_font_description_new ();
  // pango_font_description_set_family_static (fdesc, "monospace");
  pango_font_description_set_weight (fdesc, PANGO_WEIGHT_BOLD);
  return fdesc;
}

static void
playout_calc_char_extends (PangoLayout *pango_layout,
                           gint        *char_offset,
                           gint        *char_width,
                           const gchar *chars,
                           gboolean     reduce_to_ink_width)
{
  gint mlwidth = 0, miwidth = 0;
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
      *char_width = miwidth;
      *char_offset = (miwidth - mlwidth) / 2;
    }
  else
    {
      *char_width = mlwidth;
      *char_offset = 0;
    }
}

static guint
pattern_column_note_width_request (BstPatternColumn       *column,
                                   BstPatternView         *pview,
                                   GdkWindow              *drawable,
                                   PangoLayout            *pango_layout,
                                   guint                   duration)
{
  BstPatternColumnNote *self = (BstPatternColumnNote*) column;
  gint width;
  playout_calc_char_extends (pango_layout, &self->co0, &self->cw0, "*-# ", TRUE);
  self->co0 += 1;
  self->cw0 += 1 + 1;
  playout_calc_char_extends (pango_layout, &self->co1, &self->cw1, "*-CDEFGABH", TRUE);
  self->cw1 += 1;
  playout_calc_char_extends (pango_layout, &self->co2, &self->cw2, "*-+ ", TRUE);
  self->cw2 += 1;
  playout_calc_char_extends (pango_layout, &self->co3, &self->cw3, "*- 123456", TRUE);
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
  ndesc = bse_server_describe_note (BSE_SERVER, pseq->pnotes[0]->note, pseq->pnotes[0]->fine_tune);
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
                               GdkRectangle           *expose_area)
{
  BstPatternColumnNote *self = (BstPatternColumnNote*) column;
  PangoRectangle prect = { 0 };
  GdkGC *dark_gc = STYLE (pview)->dark_gc[GTK_STATE_NORMAL];
  GdkGC *black_gc = STYLE (pview)->black_gc;
  GdkGC *draw_gc;
  SfiProxy proxy = pview->proxy;
  BsePartNoteSeq *pseq = proxy ? bse_part_list_notes_within (proxy, tick, duration) : NULL;
  gchar ch;
  gint accu, yline;

  if (0)
    gdk_draw_line (drawable, dark_gc,
                   crect->x, crect->y,
                   crect->x + crect->width - 1, crect->y);
  
  accu = crect->x + FOCUS_WIDTH (pview);
  ch = pattern_column_note_char (self, pseq, 0);
  draw_gc = ch == '-' ? dark_gc : black_gc;      /* choose gc dependant on note letter */
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

static void
pattern_column_note_finalize (BstPatternColumn *column)
{
  BstPatternColumnNote *self = (BstPatternColumnNote*) column;
  g_free (self);
}

BstPatternColumnClass*
bst_pattern_column_note_get_class (void)
{
  static BstPatternColumnClass pattern_column_note_class = {
    1,  /* n_positions */
    pattern_column_note_create,
    pattern_column_note_create_font_desc,
    pattern_column_note_draw_cell,
    pattern_column_note_width_request,
    pattern_column_note_finalize,
  };
  return &pattern_column_note_class;
}


/* --- vbar cell --- */
static BstPatternColumn*
pattern_column_vbar_create (BstPatternColumnClass *class)
{
  BstPatternColumn *self = g_new0 (BstPatternColumn, 1);
  self->klass = class;
  return self;
}

static void
pattern_column_vbar_draw_cell (BstPatternColumn       *self,
                               BstPatternView         *pview,
                               GdkWindow              *drawable,
                               PangoLayout            *pango_layout,
                               guint                   tick,
                               guint                   duration,
                               GdkRectangle           *crect,
                               GdkRectangle           *expose_area)
{
  GdkGC *dark_gc = STYLE (pview)->dark_gc[GTK_STATE_NORMAL];
  gdk_draw_line (drawable, dark_gc,
                 crect->x + crect->width / 2, crect->y,
                 crect->x + crect->width / 2, crect->y + crect->height - 1);
}

static guint
pattern_column_vbar_width_request (BstPatternColumn       *self,
                                   BstPatternView         *pview,
                                   GdkWindow              *drawable,
                                   PangoLayout            *pango_layout,
                                   guint                   duration)
{
  return 1; // need no: FOCUS_WIDTH (pview)
}

static void
pattern_column_vbar_finalize (BstPatternColumn *self)
{
  g_free (self);
}

BstPatternColumnClass*
bst_pattern_column_vbar_get_class (void)
{
  static BstPatternColumnClass pattern_column_vbar_class = {
    0,  /* n_positions */
    pattern_column_vbar_create,
    NULL, /* create_font_desc */
    pattern_column_vbar_draw_cell,
    pattern_column_vbar_width_request,
    pattern_column_vbar_finalize,
  };
  return &pattern_column_vbar_class;
}
