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
#define X_OFFSET(self)          (GXK_SCROLL_CANVAS (self)->x_offset)
#define Y_OFFSET(self)          (GXK_SCROLL_CANVAS (self)->y_offset)


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
  BsePartNoteSeq *pseq = proxy ? bse_part_list_notes_within (proxy, column->num, tick, duration) : NULL;
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

static gboolean
pattern_column_note_key_event (BstPatternColumn       *column,
                               BstPatternView         *pview,
                               GdkWindow              *drawable,
                               PangoLayout            *pango_layout,
                               guint                   tick,
                               guint                   duration,
                               GdkRectangle           *cell_rect,
                               guint                   keyval,
                               GdkModifierType         modifier,
                               BstPatternAction        action,
                               gdouble                 param)
{
  // BstPatternColumnNote *self = (BstPatternColumnNote*) column;
  SfiProxy proxy = pview->proxy;
  BsePartNoteSeq *pseq = proxy ? bse_part_list_notes_within (proxy, column->num, tick, duration) : NULL;
  guint i, iparam = 0.5 + param;
  switch (action)
    {
    case BST_PATTERN_REMOVE_EVENTS:
      for (i = 0; i < pseq->n_pnotes; i++)
        bse_part_delete_event (proxy, pseq->pnotes[i]->id);
      return TRUE;
    case BST_PATTERN_SET_NOTE:
      if (pseq->n_pnotes == 1)
        {
          BsePartNote *pnote = pseq->pnotes[0];
          bse_part_delete_event (proxy, pnote->id);
        }
      if (pseq->n_pnotes <= 1)
        bse_part_insert_note (proxy, column->num, tick, duration, SFI_NOTE_CLAMP (iparam), 0, +1);
      else
        gdk_beep();
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
  1,  /* n_focus_positions */
  sizeof (BstPatternColumnNote),
  NULL, /* init */
  pattern_column_note_create_font_desc,
  pattern_column_note_draw_cell,
  pattern_column_note_width_request,
  pattern_column_note_key_event,
  pattern_column_note_finalize,
};


/* --- vbar cell --- */
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
  GdkGC *draw_gc = STYLE (pview)->dark_gc[GTK_STATE_NORMAL];
  gint dlen, line_width = 0; /* line widths != 0 interfere with dash-settings on some X servers */
  if (self->num < 0)
    {
      guint8 dash[3] = { 2, 2, 0 };
      gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
      dlen = dash[0] + dash[1];
      gdk_gc_set_dashes (draw_gc, (Y_OFFSET (pview) + crect->y + 1) % dlen, dash, 2);
    }
  if (self->num)
    gdk_draw_line (drawable, draw_gc,
                   crect->x + crect->width / 2, crect->y,
                   crect->x + crect->width / 2, crect->y + crect->height - 1);
  if (self->num < 0)
    gdk_gc_set_line_attributes (draw_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
}

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
  return 2 * XTHICKNESS (pview) + 1;
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
  pattern_column_vbar_draw_cell,
  pattern_column_vbar_width_request,
  NULL, /* key_event */
  pattern_column_vbar_finalize,
};


/* --- layout configuration --- */
static void
pattern_column_layouter_apply (GtkWidget *dialog)
{
  BstPatternView *pview = g_object_get_data (dialog, "user_data");
  GtkEntry *entry = gxk_gadget_find (dialog, "layout-entry");
  const gchar *layout = gtk_entry_get_text (entry);
  guint l = bst_pattern_view_set_layout (pview, layout);
  if (l < strlen (layout))
    {
      gtk_editable_select_region (GTK_EDITABLE (entry), l, -1);
      gdk_beep();
    }
}

void
bst_pattern_column_layouter_popup (BstPatternView *pview)
{
  static const gchar *help_text[] = {
    N_("The pattern editor column layout is specified by listing column types "
       "with possible modifiers in display order."), "\n\n",
    N_("COLUMN TYPES:"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("note-1, note-2, ..."), "\n",
    N_("  display notes of the first, second, ... channel"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("offset-1, length-2, velocty-3, ..."), "\n",
    N_("  display offset, length or velocity of notes in the first, second, ... channel"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("control-0, ..., control-63, cc-0, cc-31"), "\n",
    N_("  select various event types (controls, continuous controllers)"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("bar, |"), "\n",
    N_("  display solid vertical bar"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("dbar, :"), "\n",
    N_("  display dotted vertical bar"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("space, _"), "\n",
    N_("  insert vertical space"), "\n\n",
    N_("EVENTS:"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("  balance, volume, cc-8, ..."), "\n\n",
    N_("MODIFIERS:"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("hex2, hex4"), "\n",
    N_("  display 2 (00..FF) or 4 (0000..FFFF) digit hex numbers"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("shex2, shex4"), "\n",
    N_("  display 2 (-FF..+FF) or 4 (-FFFF..+FFFF) digit signed hex numbers"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("dec2, dec3"), "\n",
    N_("  display 2 (00..99) or 3 (000..999) digit decimal numbers"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("sdec2, sdec3"), "\n",
    N_("  display 2 (-99..+99) or 3 (-999..+999) digit signed decimal numbers"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("col1, col2, col3"), "\n",
    N_("  selects one of 3 predefined colors"), "\n",
    N_("EXAMPLE:"), "\n",
    /* !!! LEAVE UNTRANSLATED !!! */
    N_("note-1 | velocity-1=hex2 | note-2 | volume=hex4"), "\n",
  };
  static GtkWidget *dialog = NULL;
  if (!dialog)
    {
      GtkWidget *sctext, *w;
      guint i;
      dialog = gxk_dialog_new_gadget (&dialog, NULL,
                                      GXK_DIALOG_HIDE_ON_DELETE | GXK_DIALOG_MODAL | GXK_DIALOG_POPUP_POS,
                                      _("Pattern Editor Layout"),
                                      "beast", "pattern-editor-layout-box");
      sctext = gxk_scroll_text_create_for (GXK_SCROLL_TEXT_WIDGET_LOOK, gxk_gadget_find (dialog, "sctext-box"));
      gxk_scroll_text_clear (sctext);
      for (i = 0; i < G_N_ELEMENTS (help_text); i++)
        {
          const gchar *tx = help_text[i], *p = tx;
          while (*p == ' ')
            p++;
          if (p > tx)
            gxk_scroll_text_push_indent (sctext);
          gxk_scroll_text_append (sctext, p);
          if (p > tx)
            gxk_scroll_text_pop_indent (sctext);
        }
      g_signal_connect (gxk_gadget_find (dialog, "cancel-button"), "clicked",
                        G_CALLBACK (gxk_toplevel_delete), NULL);
      g_signal_connect_swapped (w = gxk_gadget_find (dialog, "apply-button"), "clicked",
                                G_CALLBACK (pattern_column_layouter_apply), dialog);
      gxk_dialog_set_default (GXK_DIALOG (dialog), w);
    }
  g_object_set_data (dialog, "user_data", pview);
  gxk_widget_showraise (dialog);
}

const gchar*
bst_pattern_layout_parse_column (const gchar      *string,
                                 BstPatternLType  *ltype,
                                 gint             *num,
                                 BstPatternLFlags *flags)
{
  static const struct { const gchar *name; guint with_num, type; } coltypes[] = {
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
  static const struct { const gchar *name; guint flag; } colflags[] = {
    { "hex2",           BST_PATTERN_LFLAG_HEX2 },
    { "hex4",           BST_PATTERN_LFLAG_HEX4 },
    { "shex2",          BST_PATTERN_LFLAG_SIGNED | BST_PATTERN_LFLAG_HEX2 },
    { "shex4",          BST_PATTERN_LFLAG_SIGNED | BST_PATTERN_LFLAG_HEX4 },
    { "dec2",           BST_PATTERN_LFLAG_DEC2 },
    { "dec3",           BST_PATTERN_LFLAG_DEC3 },
    { "sdec2",          BST_PATTERN_LFLAG_SIGNED | BST_PATTERN_LFLAG_DEC2 },
    { "sdec3",          BST_PATTERN_LFLAG_SIGNED | BST_PATTERN_LFLAG_DEC3 },
    { "col1",           BST_PATTERN_LFLAG_COL1 },
    { "col2",           BST_PATTERN_LFLAG_COL2 },
    { "col3",           BST_PATTERN_LFLAG_COL3 },
  };
  guint i;
  *ltype = 0;
  *num = 0;
  *flags = 0;
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
      while (string[0] >= '0' && string[0] <= '9')
        string++;
      if (string == p)
        return string;  /* failed */
      mem = g_memdup (p, string - p);
      *num = g_ascii_strtoull (mem, NULL, 10);
      g_free (mem);
    }
  /* parse flags */
  if (string[0] != '=')
    return string;      /* done without flags */
  do
    {
      string++;
      for (i = 0; i < G_N_ELEMENTS (flags); i++)
        if (strncmp (string, colflags[i].name, strlen (colflags[i].name)) == 0)
          {
            string += strlen (colflags[i].name);
            *flags |= colflags[i].flag;
            break;
          }
    }
  while (string[0] == ',');
  return string;
}

BstPatternColumn*
bst_pattern_column_create (BstPatternLType   ltype,
                           gint              num,
                           BstPatternLFlags  lflags)
{
  BstPatternColumnClass *class = NULL;
  BstPatternColumn *column;
  switch (ltype)
    {
    case BST_PATTERN_LTYPE_CONTROL:
    case BST_PATTERN_LTYPE_SPACE:
      class = &pattern_column_vbar_class;
      num = 0;
      break;
    case BST_PATTERN_LTYPE_BAR:
      class = &pattern_column_vbar_class;
      num = 1;
      break;
    case BST_PATTERN_LTYPE_DBAR:
      class = &pattern_column_vbar_class;
      num = -1;
      break;
    case BST_PATTERN_LTYPE_NOTE:
    case BST_PATTERN_LTYPE_OFFSET:
    case BST_PATTERN_LTYPE_LENGTH:
    case BST_PATTERN_LTYPE_VELOCITY:
    case BST_PATTERN_LTYPE_FINE_TUNE:
      class = &pattern_column_note_class;
      break;
    }
  g_assert (class->instance_size >= sizeof (BstPatternColumn));
  column = g_malloc0 (class->instance_size);
  column->klass = class;
  column->num = num;
  if (class->init)
    class->init (column);
  return column;
}
