/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2004 Tim Janik
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
#include "bstpianoroll.h"
#include "bstasciipixbuf.h"
#include "bstskinconfig.h"
#include <string.h>


/* --- defines --- */
/* accessors */
#define	STYLE(self)		(GTK_WIDGET (self)->style)
#define STATE(self)             (GTK_WIDGET (self)->state)
#define XTHICKNESS(self)        (STYLE (self)->xthickness)
#define YTHICKNESS(self)        (STYLE (self)->ythickness)
#define	ALLOCATION(self)	(&GTK_WIDGET (self)->allocation)
#define	N_OCTAVES(self)		(MAX_OCTAVE (self) - MIN_OCTAVE (self) + 1)
#define	MAX_OCTAVE(self)	(SFI_NOTE_OCTAVE ((self)->max_note))
#define	MAX_SEMITONE(self)	(SFI_NOTE_SEMITONE ((self)->max_note))
#define	MIN_OCTAVE(self)	(SFI_NOTE_OCTAVE ((self)->min_note))
#define	MIN_SEMITONE(self)	(SFI_NOTE_SEMITONE ((self)->min_note))
#define X_OFFSET(self)          (GXK_SCROLL_CANVAS (self)->x_offset)
#define Y_OFFSET(self)          (GXK_SCROLL_CANVAS (self)->y_offset)
#define PLAYOUT(self, i)        (GXK_SCROLL_CANVAS (self)->pango_layout[i])
#define PLAYOUT_HPANEL(self)    (PLAYOUT (self, PINDEX_HPANEL))
#define COLOR_GC(self, i)       (GXK_SCROLL_CANVAS (self)->color_gc[i])
#define COLOR_GC_HGRID(self)    (COLOR_GC (self, CINDEX_HGRID))
#define COLOR_GC_VGRID(self)    (COLOR_GC (self, CINDEX_VGRID))
#define COLOR_GC_HBAR(self)     (COLOR_GC (self, CINDEX_HBAR))
#define COLOR_GC_VBAR(self)     (COLOR_GC (self, CINDEX_VBAR))
#define COLOR_GC_MBAR(self)     (COLOR_GC (self, CINDEX_MBAR))
#define CANVAS(self)            (GXK_SCROLL_CANVAS (self)->canvas)
#define HPANEL(self)            (GXK_SCROLL_CANVAS (self)->top_panel)
#define VPANEL(self)            (GXK_SCROLL_CANVAS (self)->left_panel)
/* layout (requisition) */
#define	NOTE_HEIGHT(self)	((gint) ((self)->vzoom * 1.2))		/* factor must be between 1 .. 2 */
#define	OCTAVE_HEIGHT(self)	(14 * (self)->vzoom + 7 * NOTE_HEIGHT (self))	/* coord_to_note() */
#define KEYBOARD_WIDTH(self)    (32 + 2 * XTHICKNESS (self))
#define	KEYBOARD_RATIO(self)	(2.9 / 5.)	/* black/white key ratio */
/* appearance */
#define	KEY_DEFAULT_VPIXELS	(4)
#define	QNOTE_HPIXELS		(30)	/* guideline */


/* --- prototypes --- */
static void	bst_piano_roll_hsetup			(BstPianoRoll		*self,
							 guint			 ppqn,
							 guint			 qnpt,
							 guint			 max_ticks,
							 gfloat			 hzoom);

/* --- static variables --- */
static guint	signal_canvas_drag = 0;
static guint	signal_canvas_clicked = 0;
static guint	signal_piano_drag = 0;
static guint	signal_piano_clicked = 0;


/* --- functions --- */
G_DEFINE_TYPE (BstPianoRoll, bst_piano_roll, GXK_TYPE_SCROLL_CANVAS);

enum {
  PINDEX_HPANEL,
  PINDEX_COUNT
};

static void
piano_roll_class_setup_skin (BstPianoRollClass *class)
{
  static GdkColor colors[] = {
    /* C: */
    { 0, 0x8080, 0x0000, 0x0000 },  /* dark red */
    { 0, 0xa000, 0x0000, 0xa000 },  /* dark magenta */
    /* D: */
    { 0, 0x0000, 0x8000, 0x8000 },  /* dark turquoise */
    { 0, 0x0000, 0xff00, 0xff00 },  /* light turquoise */
    /* E: */
    { 0, 0xff00, 0xff00, 0x0000 },  /* bright yellow */
    /* F: */
    { 0, 0xff00, 0x4000, 0x4000 },  /* light red */
    { 0, 0xff00, 0x8000, 0x0000 },  /* bright orange */
    /* G: */
    { 0, 0xb000, 0x0000, 0x6000 },  /* dark pink */
    { 0, 0xff00, 0x0000, 0x8000 },  /* light pink */
    /* A: */
    { 0, 0x0000, 0x7000, 0x0000 },  /* dark green */
    { 0, 0x4000, 0xff00, 0x4000 },  /* bright green */
    /* B: */
    { 0, 0x4000, 0x4000, 0xff00 },  /* light blue */
    /* drawing colors, index: 12+ */
    { 0, 0x9e00, 0x9a00, 0x9100 },  /* hgrid */
    { 0, 0x9e00, 0x9a00, 0x9100 },  /* vgrid */
    { 0, 0x8000, 0x0000, 0x0000 },  /* hbar */
    { 0, 0x9e00, 0x9a00, 0x9100 },  /* vbar */
    { 0, 0xff00, 0x8000, 0x0000 },  /* mbar */
#define CINDEX_HGRID    12
#define CINDEX_VGRID    13
#define CINDEX_HBAR     14
#define CINDEX_VBAR     15
#define CINDEX_MBAR     16
  };
  GxkScrollCanvasClass *scroll_canvas_class = GXK_SCROLL_CANVAS_CLASS (class);
  scroll_canvas_class->n_pango_layouts = PINDEX_COUNT;
  scroll_canvas_class->n_colors = G_N_ELEMENTS (colors);
  scroll_canvas_class->colors = colors;
  colors[CINDEX_HGRID] = gdk_color_from_rgb (BST_SKIN_CONFIG (piano_hgrid));
  colors[CINDEX_VGRID] = gdk_color_from_rgb (BST_SKIN_CONFIG (piano_vgrid));
  colors[CINDEX_HBAR] = gdk_color_from_rgb (BST_SKIN_CONFIG (piano_hbar));
  colors[CINDEX_VBAR] = gdk_color_from_rgb (BST_SKIN_CONFIG (piano_vbar));
  colors[CINDEX_MBAR] = gdk_color_from_rgb (BST_SKIN_CONFIG (piano_mbar));
  g_free (scroll_canvas_class->image_file_name);
  scroll_canvas_class->image_file_name = BST_SKIN_CONFIG_STRDUP_PATH (piano_image);
  scroll_canvas_class->image_tint = gdk_color_from_rgb (BST_SKIN_CONFIG (piano_color));
  scroll_canvas_class->image_saturation = BST_SKIN_CONFIG (piano_shade) * 0.01;
  gxk_scroll_canvas_class_skin_changed (scroll_canvas_class);
}

static void
bst_piano_roll_init (BstPianoRoll *self)
{
  GxkScrollCanvas *scc = GXK_SCROLL_CANVAS (self);
  
  GTK_WIDGET_SET_FLAGS (self, GTK_CAN_FOCUS);
  
  self->proxy = 0;
  self->vzoom = KEY_DEFAULT_VPIXELS;
  self->ppqn = 384;	/* default Parts (clock ticks) Per Quarter Note */
  self->qnpt = 1;
  self->max_ticks = 1;
  self->hzoom = 1;
  self->draw_qn_grid = TRUE;
  self->draw_qqn_grid = TRUE;
  self->release_closes_toplevel = TRUE;
  self->min_note = SFI_MIN_NOTE;
  self->max_note = SFI_MAX_NOTE;
  gxk_scroll_canvas_set_canvas_cursor (scc, GDK_LEFT_PTR);
  gxk_scroll_canvas_set_left_panel_cursor (scc, GDK_HAND2);
  gxk_scroll_canvas_set_top_panel_cursor (scc, GDK_LEFT_PTR);
  self->scroll_timer = 0;
  self->selection_tick = 0;
  self->selection_duration = 0;
  self->selection_min_note = 0;
  self->selection_max_note = 0;
  bst_piano_roll_hsetup (self, 384, 4, 800 * 384, 1);
  
  bst_ascii_pixbuf_ref ();
}

static void
bst_piano_roll_destroy (GtkObject *object)
{
  BstPianoRoll *self = BST_PIANO_ROLL (object);
  
  bst_piano_roll_set_proxy (self, 0);
  
  GTK_OBJECT_CLASS (bst_piano_roll_parent_class)->destroy (object);
}

static void
bst_piano_roll_dispose (GObject *object)
{
  BstPianoRoll *self = BST_PIANO_ROLL (object);
  
  bst_piano_roll_set_proxy (self, 0);
  
  G_OBJECT_CLASS (bst_piano_roll_parent_class)->dispose (object);
}

static void
bst_piano_roll_finalize (GObject *object)
{
  BstPianoRoll *self = BST_PIANO_ROLL (object);
  
  bst_piano_roll_set_proxy (self, 0);
  
  if (self->scroll_timer)
    {
      g_source_remove (self->scroll_timer);
      self->scroll_timer = 0;
    }
  bst_ascii_pixbuf_unref ();
  
  G_OBJECT_CLASS (bst_piano_roll_parent_class)->finalize (object);
}

gfloat
bst_piano_roll_set_vzoom (BstPianoRoll *self,
			  gfloat        vzoom)
{
  g_return_val_if_fail (BST_IS_PIANO_ROLL (self), 0);
  
  self->vzoom = vzoom; //  * KEY_DEFAULT_VPIXELS;
  self->vzoom = CLAMP (self->vzoom, 1, 16);
  
  gtk_widget_queue_resize (GTK_WIDGET (self));
  
  return self->vzoom;
}

static void
piano_roll_get_layout (GxkScrollCanvas        *scc,
                       GxkScrollCanvasLayout  *layout)
{
  BstPianoRoll *self = BST_PIANO_ROLL (scc);
  PangoRectangle rect = { 0 };
  pango_layout_get_pixel_extents (PLAYOUT_HPANEL (self), NULL, &rect);
  layout->top_panel_height = rect.height;
  layout->left_panel_width = KEYBOARD_WIDTH (self);
  layout->right_panel_width = 0;
  layout->bottom_panel_height = 0;
  layout->max_canvas_height = N_OCTAVES (self) * OCTAVE_HEIGHT (self);
  layout->canvas_height = N_OCTAVES (self) * OCTAVE_HEIGHT (self);
}

static gint
ticks_to_pixels (BstPianoRoll *self,
		 gint	       ticks)
{
  gfloat ppqn = self->ppqn;
  gfloat tpixels = QNOTE_HPIXELS;
  
  /* compute pixel span of a tick range */
  
  tpixels *= self->hzoom / ppqn * (gfloat) ticks;
  if (ticks)
    tpixels = MAX (tpixels, 1);
  return tpixels;
}

static gint
pixels_to_ticks (BstPianoRoll *self,
		 gint	       pixels)
{
  gfloat ppqn = self->ppqn;
  gfloat ticks = 1.0 / (gfloat) QNOTE_HPIXELS;
  
  /* compute tick span of a pixel range */
  
  ticks = ticks * ppqn / self->hzoom * (gfloat) pixels;
  if (pixels > 0)
    ticks = MAX (ticks, 1);
  else
    ticks = 0;
  return ticks;
}

static gint
tick_to_coord (BstPianoRoll *self,
	       gint	     tick)
{
  return ticks_to_pixels (self, tick) - X_OFFSET (self);
}

static gint
coord_to_tick (BstPianoRoll *self,
	       gint	     x,
	       gboolean	     right_bound)
{
  guint tick;
  
  x += X_OFFSET (self);
  tick = pixels_to_ticks (self, x);
  if (right_bound)
    {
      guint tick2 = pixels_to_ticks (self, x + 1);
      
      if (tick2 > tick)
	tick = tick2 - 1;
    }
  return tick;
}

#define	CROSSING_TACT		(1)
#define	CROSSING_QNOTE		(2)
#define	CROSSING_QNOTE_Q	(3)

static guint
coord_check_crossing (BstPianoRoll *self,
		      gint	    x,
		      guint	    crossing)
{
  guint ltick = coord_to_tick (self, x, FALSE);
  guint rtick = coord_to_tick (self, x, TRUE);
  guint lq = 0, rq = 0;
  
  /* catch _at_ tick boundary as well */
  rtick += 1;
  
  switch (crossing)
    {
    case CROSSING_TACT:
      lq = ltick / (self->ppqn * self->qnpt);
      rq = rtick / (self->ppqn * self->qnpt);
      break;
    case CROSSING_QNOTE:
      lq = ltick / self->ppqn;
      rq = rtick / self->ppqn;
      break;
    case CROSSING_QNOTE_Q:
      lq = ltick * 4 / self->ppqn;
      rq = rtick * 4 / self->ppqn;
      break;
    }
  
  return lq != rq;
}

#define	DRAW_NONE	(0)
#define	DRAW_START	(1)
#define	DRAW_MIDDLE	(2)
#define	DRAW_END	(3)

typedef struct {
  gint  octave;
  guint semitone;	/* 0 .. 11    within octave */
  guint key;		/* 0 .. 6     index of white key */
  guint key_frac;	/* 0 .. 4*z-1 fractional pixel index into key */
  guint wstate;		/* DRAW_ START/MIDDLE/END of white key */
  guint bstate;		/* DRAW_ NONE/START/MIDDLE/END of black key */
  guint bmatch : 1;	/* TRUE if on black key (differs from bstate!=NONE) */
  guint ces_fes : 1;	/* TRUE if on non-existant black key below C or F */
  guint valid : 1;	/* FALSE if min/max octave/semitone are exceeded */
  gint  valid_octave;
  guint valid_semitone;
} NoteInfo;

static gint
note_to_pixels (BstPianoRoll *self,
		gint	      note,
		gint	     *height_p,
		gint	     *ces_fes_height_p)
{
  gint octave, ythickness = 1, z = self->vzoom, h = NOTE_HEIGHT (self), semitone = SFI_NOTE_SEMITONE (note);
  gint oheight = OCTAVE_HEIGHT (self), y, zz = z + z, offs = 0, height = h;
  
  switch (semitone)
    {
    case 10:	offs += zz + h;
    case  8:	offs += zz + h;
    case  6:	offs += zz + h + zz + h;
    case  3:	offs += zz + h;
    case  1:	offs += z + h + (zz - h) / 2;
      break;
    case 11:	offs += h + zz;
    case  9:	offs += h + zz;
    case  7:	offs += h + zz;
    case  5:	offs += h + zz;
    case  4:	offs += h + zz;
    case  2:	offs += h + zz;
    case  0:	offs += z;
      break;
    }
  octave = N_OCTAVES (self) - 1 - SFI_NOTE_OCTAVE (note) + MIN_OCTAVE (self);
  y = octave * oheight;
  y += oheight - offs - h;
  
  /* spacing out by a bit looks nicer */
  if (z >= 4)
    {
      height += ythickness;
    }
  
  if (height_p)
    *height_p = height;
  if (ces_fes_height_p)
    *ces_fes_height_p = (semitone == 0 || semitone == 4 || semitone == 5 || semitone == 11) ? z : 0;
  
  return y;
}

static gint
note_to_coord (BstPianoRoll *self,
	       gint	     note,
	       gint	    *height_p,
	       gint	    *ces_fes_height_p)
{
  return note_to_pixels (self, note, height_p, ces_fes_height_p) - Y_OFFSET (self);
}

static gboolean
coord_to_note (BstPianoRoll *self,
	       gint          y,
	       NoteInfo	    *info)
{
  gint ythickness = 1, i, z = self->vzoom, h = NOTE_HEIGHT (self);
  gint end_shift, start_shift, black_shift = 0;
  gint oheight = OCTAVE_HEIGHT (self), kheight = 2 * z + h;
  
  y += Y_OFFSET (self);
  info->octave = y / oheight;
  i = y - info->octave * oheight;
  i = oheight - 1 - i;		/* octave increases with decreasing y */
  info->key = i / kheight;
  info->key_frac = i - info->key * kheight;
  i = info->key_frac;
  info->octave = N_OCTAVES (self) - 1 - info->octave + MIN_OCTAVE (self);
  
  /* figure black notes */
  end_shift = i >= z + h;
  start_shift = i < z; /* + ythickness; */
  info->semitone = 0;
  info->ces_fes = ((info->key == 0 && start_shift) ||
		   (info->key == 2 && end_shift) ||
		   (info->key == 3 && start_shift) ||
		   (info->key == 6 && end_shift));
  switch (info->key)
    {
    case 3:	info->semitone += 5;
    case 0:
      info->semitone += 0 + end_shift;
      black_shift = end_shift;
      break;
    case 5:	info->semitone += 2;
    case 4:	info->semitone += 5;
    case 1:
      info->semitone += 2 + (start_shift ? -1 : end_shift);
      black_shift = start_shift || end_shift;
      break;
    case 6:	info->semitone += 7;
    case 2:
      info->semitone += 4 - start_shift;
      black_shift = start_shift;
      break;
    }
  
  /* pixel layout and note numbers:
   * Iz|h|zIz|h|zIz|h|zIz|h|zIz|h|zIz|h|zIz|h|zI
   * I 0 |#1#|2|#3#|4  I  5|#6#|7|#8#|9|#10|11 I
   * I   |###| |###|   I   |###| |###| |###|   I
   * I   +-+-+ +-+-+   I   +-+-+ +-+-+ +-+-+   I
   * I     I     I     I     I     I     I     I
   * +--0--+--1--+--2--+--3--+--4--+--5--+--6--+
   * i=key_fraction, increases to right --->
   */
  
  /* figure draw states */
  if (i < ythickness)
    info->wstate = DRAW_START;
  else if (i < kheight - ythickness)
    info->wstate = DRAW_MIDDLE;
  else
    info->wstate = DRAW_END;
  if (!black_shift)
    info->bstate = DRAW_NONE;
  else if (i < z - ythickness)
    info->bstate = DRAW_MIDDLE;
  else if (i < z)
    info->bstate = DRAW_START;
  else if (i < z + h + ythickness)
    info->bstate = DRAW_END;
  else
    info->bstate = DRAW_MIDDLE;
  
  /* behaviour fixup, ignore black note borders */
  if (black_shift && info->bstate == DRAW_START)
    {
      info->bmatch = FALSE;
      info->semitone += 1;
    }
  else if (black_shift && info->bstate == DRAW_END)
    {
      info->bmatch = FALSE;
      info->semitone -= 1;
    }
  else
    info->bmatch = TRUE;
  
  /* validate note */
  if (y < 0 ||		/* we calc junk in this case, flag invalidity */
      info->octave > MAX_OCTAVE (self) ||
      (info->octave == MAX_OCTAVE (self) && info->semitone > MAX_SEMITONE (self)))
    {
      info->valid_octave = MAX_OCTAVE (self);
      info->valid_semitone = MAX_SEMITONE (self);
      info->valid = FALSE;
    }
  else if (info->octave < MIN_OCTAVE (self) ||
	   (info->octave == MIN_OCTAVE (self) && info->semitone < MIN_SEMITONE (self)))
    {
      info->valid_octave = MIN_OCTAVE (self);
      info->valid_semitone = MIN_SEMITONE (self);
      info->valid = FALSE;
    }
  else
    {
      info->valid_octave = info->octave;
      info->valid_semitone = info->semitone;
      info->valid = TRUE;
    }
  
  return info->bmatch != 0;
}

static void
bst_piano_roll_overlap_grow_vpanel_area (BstPianoRoll *self,
					 GdkRectangle *area)
{
  /* grow vpanel exposes by surrounding white keys */
  area->y -= OCTAVE_HEIGHT (self) / 7;			/* fudge 1 key upwards */
  area->height += OCTAVE_HEIGHT (self) / 7;             /* compensate for y-=key */
  area->height += OCTAVE_HEIGHT (self) / 7;		/* fudge 1 key downwards */
}

static void
bst_piano_roll_draw_vpanel (GxkScrollCanvas *scc,
                            GdkWindow       *drawable,
                            GdkRectangle    *area)
{
  BstPianoRoll *self = BST_PIANO_ROLL (scc);
  GdkGC *black_gc = STYLE (self)->fg_gc[GTK_STATE_NORMAL];
  GdkGC *dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  GdkGC *light_gc = STYLE (self)->light_gc[GTK_STATE_NORMAL];
  gint y, start_x = 0, white_x = KEYBOARD_WIDTH (self), black_x = white_x * KEYBOARD_RATIO (self);
  gint width, height;
  gdk_window_get_size (drawable, &width, &height);
  bst_piano_roll_overlap_grow_vpanel_area (self, area);
  
  /* draw vertical frame lines */
  gdk_draw_line (drawable, dark_gc, start_x + white_x - 1, area->y, start_x + white_x - 1, area->y + area->height - 1);
  gdk_draw_line (drawable, light_gc, start_x, area->y, start_x, area->y + area->height - 1);

  /* draw horizontal lines */
  for (y = MAX (area->y, 0); y < area->y + area->height; y++)
    {
      gint x = black_x + 1;
      NoteInfo info;
      
      coord_to_note (self, y, &info);
      switch (info.bstate)
	{
	case DRAW_START:
	  gdk_draw_line (drawable, black_gc, start_x + 1, y, start_x + black_x - 1, y);
	  gdk_draw_line (drawable, dark_gc,  start_x + black_x, y, start_x + black_x, y);
	  break;
	case DRAW_MIDDLE:
	  gdk_draw_line (drawable, black_gc, start_x + 1, y, start_x + black_x - 1, y);
	  gdk_draw_line (drawable, dark_gc,  start_x + black_x, y, start_x + black_x, y);
	  break;
	case DRAW_END:
	  gdk_draw_line (drawable, dark_gc, start_x + 1, y, start_x + black_x, y);
	  break;
	default:
	  x = 0;
	}
      switch (info.wstate)
	{
	case DRAW_START:
	  gdk_draw_line (drawable, dark_gc, start_x + x, y, start_x + white_x, y);
	  if (info.semitone == 0)	/* C */
	    {
	      gint pbheight, ypos, ythickness = 1, overlap = 1;
	      gint pbwidth = white_x - black_x + overlap;
	      GdkPixbuf *pixbuf;
              
	      pbheight = OCTAVE_HEIGHT (self) / 7;
	      pbwidth /= 2;
	      ypos = y - pbheight + ythickness;
	      pixbuf = bst_ascii_pixbuf_new ('C', pbwidth, pbheight);
	      gdk_pixbuf_render_to_drawable (pixbuf, drawable, light_gc, 0, 0,
					     start_x + black_x, ypos, -1, -1,
					     GDK_RGB_DITHER_MAX, 0, 0);
	      g_object_unref (pixbuf);
	      if (info.octave < 0)
		{
		  guint indent = pbwidth * 0.5;
		  /* render a minus '-' for negative octaves into the 'C' */
		  pixbuf = bst_ascii_pixbuf_new ('-', pbwidth - indent, pbheight - 1);
		  gdk_pixbuf_render_to_drawable (pixbuf, drawable, light_gc, 0, 0,
						 start_x + black_x + indent + overlap, ypos, -1, -1,
						 GDK_RGB_DITHER_MAX, 0, 0);
		  g_object_unref (pixbuf);
		}
	      pixbuf = bst_ascii_pixbuf_new (ABS (info.octave) + '0', pbwidth, pbheight);
	      gdk_pixbuf_render_to_drawable (pixbuf, drawable, light_gc, 0, 0,
					     start_x + black_x + pbwidth - overlap, ypos, -1, -1,
					     GDK_RGB_DITHER_MAX, 0, 0);
	      g_object_unref (pixbuf);
	    }
	  break;
	case DRAW_MIDDLE:
	  // gdk_draw_line (drawable, white_gc, start_x + x, y, start_x + white_x, y);
	  break;
	case DRAW_END:
	  gdk_draw_line (drawable, light_gc, start_x + x, y, start_x + white_x, y);
	  break;
	}
    }
}

static void
bst_piano_roll_draw_canvas (GxkScrollCanvas *scc,
                            GdkWindow       *drawable,
                            GdkRectangle    *area)
{
  BstPianoRoll *self = BST_PIANO_ROLL (scc);
  GdkGC *light_gc, *dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  gint pass, i, dlen, width, height, line_width = 0; /* line widths != 0 interfere with dash-settings on some X servers */
  BsePartNoteSeq *pseq;
  GXK_SCROLL_CANVAS_CLASS (bst_piano_roll_parent_class)->draw_canvas (scc, drawable, area);
  gdk_window_get_size (drawable, &width, &height);

  /* draw selection */
  if (self->selection_duration)
    {
      gint x1, x2, y1, y2, h;
      
      x1 = tick_to_coord (self, self->selection_tick);
      x2 = tick_to_coord (self, self->selection_tick + self->selection_duration);
      y1 = note_to_coord (self, self->selection_max_note, &h, NULL);
      y2 = note_to_coord (self, self->selection_min_note, &h, NULL);
      y2 += h;
      /* confine to 16bit coordinates for gdk to handle correctly */
      x1 = MAX (x1, 0);
      x2 = MIN (x2, width);
      y1 = MAX (y1, 0);
      y2 = MIN (y2, height);
      gdk_draw_rectangle (drawable, GTK_WIDGET (self)->style->bg_gc[GTK_STATE_SELECTED], TRUE,
			  x1, y1, MAX (x2 - x1, 0), MAX (y2 - y1, 0));
    }

  /* we do multiple passes to draw h/v grid lines for them to properly ovrlay */
  for (pass = 1; pass <= 3; pass++)
    {
      /* draw vertical grid lines */
      for (i = area->x; i < area->x + area->width; i++)
        {
          if (pass == 3 && coord_check_crossing (self, i, CROSSING_TACT))
            {
              GdkGC *draw_gc = COLOR_GC_VBAR (self);
              gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
              gdk_draw_line (drawable, draw_gc, i, area->y, i, area->y + area->height - 1);
            }
          else if (pass == 1 && self->draw_qn_grid && coord_check_crossing (self, i, CROSSING_QNOTE))
            {
              GdkGC *draw_gc = COLOR_GC_VGRID (self);
              guint8 dash[3] = { 2, 2, 0 };
              gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
              dlen = dash[0] + dash[1];
              gdk_gc_set_dashes (draw_gc, (Y_OFFSET (self) + area->y + 1) % dlen, dash, 2);
              gdk_draw_line (drawable, draw_gc, i, area->y, i, area->y + area->height - 1);
              gdk_gc_set_line_attributes (draw_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
            }
          else if (pass == 1 && self->draw_qqn_grid && coord_check_crossing (self, i, CROSSING_QNOTE_Q))
            {
              GdkGC *draw_gc = COLOR_GC_VGRID (self);
              guint8 dash[3] = { 1, 1, 0 };
              gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
              dlen = dash[0] + dash[1];
              gdk_gc_set_dashes (draw_gc, (Y_OFFSET (self) + area->y + 1) % dlen, dash, 2);
              gdk_draw_line (drawable, draw_gc, i, area->y, i, area->y + area->height - 1);
              gdk_gc_set_line_attributes (draw_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
            }
        }
      /* draw horizontal grid lines */
      for (i = area->y; i < area->y + area->height; i++)
        {
          NoteInfo info;
          coord_to_note (self, i, &info);
          if (info.wstate != DRAW_START)
            continue;
          if (pass == 3 && info.semitone == 0)	/* C */
            {
              GdkGC *draw_gc = COLOR_GC_HBAR (self);
              gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
              gdk_draw_line (drawable, draw_gc, area->x, i, area->x + area->width - 1, i);
            }
          else if (pass == 2 && info.semitone == 5) /* F */
            {
              GdkGC *draw_gc = COLOR_GC_MBAR (self);
              guint8 dash[3] = { 2, 2, 0 };
              
              gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
              dlen = dash[0] + dash[1];
              gdk_gc_set_dashes (draw_gc, (X_OFFSET (self) + area->x + 1) % dlen, dash, 2);
              gdk_draw_line (drawable, draw_gc, area->x, i, area->x + area->width - 1, i);
              gdk_gc_set_line_attributes (draw_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
            }
          else if (pass == 1)
            {
              GdkGC *draw_gc = COLOR_GC_HGRID (self);
              guint8 dash[3] = { 1, 1, 0 };
              
              gdk_gc_set_line_attributes (draw_gc, line_width, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
              dlen = dash[0] + dash[1];
              gdk_gc_set_dashes (draw_gc, (X_OFFSET (self) + area->x + 1) % dlen, dash, 2);
              gdk_draw_line (drawable, draw_gc, area->x, i, area->x + area->width - 1, i);
              gdk_gc_set_line_attributes (draw_gc, 0, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
            }
        }
    }

  /* draw notes */
  light_gc = STYLE (self)->light_gc[GTK_STATE_NORMAL];
  dark_gc = STYLE (self)->dark_gc[GTK_STATE_NORMAL];
  pseq = self->proxy ? bse_part_list_notes_crossing (self->proxy,
						     coord_to_tick (self, area->x, FALSE),
						     coord_to_tick (self, area->x + area->width, FALSE)) : NULL;
  for (i = 0; pseq && i < pseq->n_pnotes; i++)
    {
      BsePartNote *pnote = pseq->pnotes[i];
      gint semitone = SFI_NOTE_SEMITONE (pnote->note);
      guint start = pnote->tick, end = start + pnote->duration;
      GdkGC *xdark_gc, *xlight_gc, *xnote_gc;
      gint x1, x2, y1, y2, height;
      gboolean selected = pnote->selected;
      
      selected |= (pnote->tick >= self->selection_tick &&
		   pnote->tick < self->selection_tick + self->selection_duration &&
		   pnote->note >= self->selection_min_note &&
		   pnote->note <= self->selection_max_note);
      if (selected)
	{
	  xdark_gc = STYLE (self)->bg_gc[GTK_STATE_SELECTED];
	  xnote_gc = STYLE (self)->fg_gc[GTK_STATE_SELECTED];
	  xlight_gc = STYLE (self)->bg_gc[GTK_STATE_SELECTED];
	}
      else
	{
	  xdark_gc = STYLE (self)->black_gc;
	  xnote_gc = COLOR_GC (self, semitone);
	  xlight_gc = dark_gc;
	}
      x1 = tick_to_coord (self, start);
      x2 = tick_to_coord (self, end);
      
      y1 = note_to_coord (self, pnote->note, &height, NULL);
      y2 = y1 + height - 1;
      gdk_draw_line (drawable, xdark_gc, x1, y2, x2, y2);
      gdk_draw_line (drawable, xdark_gc, x2, y1, x2, y2);
      gdk_draw_rectangle (drawable, xnote_gc, TRUE, x1, y1, MAX (x2 - x1, 1), MAX (y2 - y1, 1));
      if (y2 - y1 >= 3)	/* work for zoom to micro size */
	{
	  if (xlight_gc)
	    {
	      gdk_draw_line (drawable, xlight_gc, x1, y1, x2, y1);
	      gdk_draw_line (drawable, xlight_gc, x1, y1, x1, y2);
	    }
	}
    }
}

static void
bst_piano_roll_overlap_grow_hpanel_area (BstPianoRoll *self,
					 GdkRectangle *area)
{
  gint i, x = area->x, xbound = x + area->width;
  
  /* grow hpanel exposes by surrounding tacts */
  i = coord_to_tick (self, x, FALSE);
  i /= self->ppqn * self->qnpt;
  if (i > 0)
    i -= 1;		/* fudge 1 tact to the left */
  i *= self->ppqn * self->qnpt;
  x = tick_to_coord (self, i);
  i = coord_to_tick (self, xbound + 1, TRUE);
  i /= self->ppqn * self->qnpt;
  i += 2;		/* fudge 1 tact to the right (+1 for round-off) */
  i *= self->ppqn * self->qnpt;
  xbound = tick_to_coord (self, i);
  
  area->x = x;
  area->width = xbound - area->x;
}

static void
bst_piano_roll_draw_hpanel (GxkScrollCanvas *scc,
                            GdkWindow       *drawable,
                            GdkRectangle    *area)
{
  BstPianoRoll *self = BST_PIANO_ROLL (scc);
  GdkGC *draw_gc = STYLE (self)->fg_gc[STATE (self)];
  PangoRectangle rect = { 0 };
  gchar buffer[64];
  gint i, width, height;
  gdk_window_get_size (drawable, &width, &height);
  bst_piano_roll_overlap_grow_hpanel_area (self, area);
  
  /* draw tact/note numbers */
  gdk_gc_set_clip_rectangle (draw_gc, area);
  for (i = area->x; i < area->x + area->width; i++)
    {
      /* drawing qnote numbers is not of much use if we can't even draw
       * the qnote quarter grid, so we special case draw_qqn_grid here
       */
      if (coord_check_crossing (self, i, CROSSING_TACT))
	{
	  guint next_pixel, tact = coord_to_tick (self, i, TRUE) + 1;
          
	  tact /= (self->ppqn * self->qnpt);
	  next_pixel = tick_to_coord (self, (tact + 1) * (self->ppqn * self->qnpt));
          
	  g_snprintf (buffer, 64, "%u", tact + 1);
          pango_layout_set_text (PLAYOUT_HPANEL (self), buffer, -1);
          pango_layout_get_pixel_extents (PLAYOUT_HPANEL (self), NULL, &rect);

	  /* draw this tact if there's enough space */
	  if (i + rect.width / 2 < (i + next_pixel) / 2)
	    gdk_draw_layout (drawable, draw_gc,
			     i - rect.width / 2, (height - rect.height) / 2,
			     PLAYOUT_HPANEL (self));
	}
      else if (self->draw_qqn_grid && coord_check_crossing (self, i, CROSSING_QNOTE))
	{
          guint next_pixel, tact = coord_to_tick (self, i, TRUE) + 1, qn = tact;

	  tact /= (self->ppqn * self->qnpt);
	  qn /= self->ppqn;
	  next_pixel = tick_to_coord (self, (qn + 1) * self->ppqn);
          qn = qn % self->qnpt + 1;
          if (qn == 1)
            continue;   /* would draw on top of tact number */

	  g_snprintf (buffer, 64, ":%u", qn);
          pango_layout_set_text (PLAYOUT_HPANEL (self), buffer, -1);
          pango_layout_get_pixel_extents (PLAYOUT_HPANEL (self), NULL, &rect);
          
	  /* draw this tact if there's enough space */
	  if (i + rect.width < (i + next_pixel) / 2)		/* don't half width, leave some more space */
	    gdk_draw_layout (drawable, draw_gc,
			     i - rect.width / 2, (height - rect.height) / 2,
                             PLAYOUT_HPANEL (self));
	}
    }
  gdk_gc_set_clip_rectangle (draw_gc, NULL);
}

static void
piano_roll_queue_expose (BstPianoRoll *self,
			 GdkWindow    *window,
			 guint	       note,
			 guint	       tick_start,
			 guint	       tick_end)
{
  gint x1 = tick_to_coord (self, tick_start);
  gint x2 = tick_to_coord (self, tick_end);
  gint height, cfheight, y1 = note_to_coord (self, note, &height, &cfheight);
  GdkRectangle area;
  
  area.x = x1;
  area.width = x2 - x1;
  area.x -= 3;		        /* add fudge */
  area.width += 3 + 3;	        /* add fudge */
  area.y = y1 - cfheight;
  area.height = height + 2 * cfheight;
  area.y -= height / 2;         /* add fudge */
  area.height += height;        /* add fudge */
  if (window == VPANEL (self))
    {
      area.x = 0;
      gdk_window_get_size (VPANEL (self), &area.width, NULL);
    }
  else if (window == HPANEL (self))
    {
      area.y = 0;
      gdk_window_get_size (HPANEL (self), NULL, &area.height);
    }
  gdk_window_invalidate_rect (window, &area, TRUE);
}

static void
piano_roll_update_adjustments (GxkScrollCanvas *scc,
			       gboolean         hadj,
			       gboolean         vadj)
{
  BstPianoRoll *self = BST_PIANO_ROLL (scc);
  
  if (hadj)
    {
      scc->hadjustment->upper = ticks_to_pixels (self, self->max_ticks);
      scc->hadjustment->step_increment = self->ppqn;
      scc->hadjustment->page_increment = self->ppqn * self->qnpt;
      /* FIXME: hack: artificially confine horizontal scroll range, until
       * proper time scale support is implemented
       */
      scc->hadjustment->upper = MIN (scc->hadjustment->upper, 1000000);
    }
  if (vadj)
    {
      scc->vadjustment->upper = OCTAVE_HEIGHT (self) * N_OCTAVES (self);
      scc->vadjustment->step_increment = OCTAVE_HEIGHT (self) / 7;
    }
  GXK_SCROLL_CANVAS_CLASS (bst_piano_roll_parent_class)->update_adjustments (scc, hadj, vadj);
  if (self->init_vpos && self->proxy)
    {
      self->init_vpos = FALSE;
      gtk_adjustment_set_value (scc->vadjustment,
                                (scc->vadjustment->upper -
                                 scc->vadjustment->lower -
                                 scc->vadjustment->page_size) / 2);
    }
}

static void
bst_piano_roll_hsetup (BstPianoRoll *self,
		       guint	     ppqn,
		       guint	     qnpt,
		       guint	     max_ticks,
		       gfloat	     hzoom)
{
  guint old_ppqn = self->ppqn;
  guint old_qnpt = self->qnpt;
  guint old_max_ticks = self->max_ticks;
  gfloat old_hzoom = self->hzoom;
  
  /* here, we setup all things necessary to determine our
   * horizontal layout. we have to avoid resizes at
   * least if just max_ticks changes, since the tick range
   * might need to grow during pointer grabs
   */
  self->ppqn = MAX (ppqn, 1);
  self->qnpt = CLAMP (qnpt, 3, 4);
  self->max_ticks = MAX (max_ticks, 1);
  self->hzoom = CLAMP (hzoom, 0.01, 100);
  
  if (old_ppqn != self->ppqn ||
      old_qnpt != self->qnpt ||
      old_max_ticks != self->max_ticks ||	// FIXME: shouldn't always cause a redraw
      old_hzoom != self->hzoom)
    {
      self->draw_qn_grid = ticks_to_pixels (self, self->ppqn) >= 3;
      self->draw_qqn_grid = ticks_to_pixels (self, self->ppqn / 4) >= 5;
      gtk_widget_queue_draw (GTK_WIDGET (self));
      X_OFFSET (self) = GXK_SCROLL_CANVAS (self)->hadjustment->value;
      gxk_scroll_canvas_update_adjustments (GXK_SCROLL_CANVAS (self), TRUE, FALSE);
    }
}

gfloat
bst_piano_roll_set_hzoom (BstPianoRoll *self,
			  gfloat        hzoom)
{
  g_return_val_if_fail (BST_IS_PIANO_ROLL (self), 0);
  
  bst_piano_roll_hsetup (self, self->ppqn, self->qnpt, self->max_ticks, hzoom);
  
  return self->hzoom;
}

static void
piano_roll_handle_drag (GxkScrollCanvas     *scc,
                        GxkScrollCanvasDrag *scc_drag,
                        GdkEvent            *event)
{
  BstPianoRoll *self = BST_PIANO_ROLL (scc);
  BstPianoRollDrag drag_mem = { 0 }, *drag = &drag_mem;
  gint hdrag = scc_drag->canvas_drag || scc_drag->top_panel_drag;
  gint vdrag = scc_drag->canvas_drag || scc_drag->left_panel_drag;
  /* copy over drag setup */
  memcpy (drag, scc_drag, sizeof (*scc_drag));  /* sizeof (*scc_drag) < sizeof (*drag) */
  drag->proll = self;
  /* calculate widget specific drag data */
  if (hdrag)
    drag->current_tick = coord_to_tick (self, MAX (drag->current_x, 0), FALSE);
  if (vdrag)
    {
      NoteInfo info;
      coord_to_note (self, MAX (drag->current_y, 0), &info);
      drag->current_note = SFI_NOTE_GENERIC (info.valid_octave, info.valid_semitone);
      drag->current_valid = info.valid && !info.ces_fes;
    }
  /* sync start-position fields */
  if (drag->type == GXK_DRAG_START)
    {
      drag->start_tick = self->start_tick = drag->current_tick;
      drag->start_note = self->start_note = drag->current_note;
      drag->start_valid = self->start_valid = drag->current_valid;
    }
  else
    {
      drag->start_tick = self->start_tick;
      drag->start_note = self->start_note;
      drag->start_valid = self->start_valid;
    }
  /* handle drag */
  if (drag->canvas_drag)
    g_signal_emit (self, signal_canvas_drag, 0, drag);
  else if (drag->left_panel_drag)
    g_signal_emit (self, signal_piano_drag, 0, drag);
  /* copy over drag reply */
  scc_drag->state = drag->state;
  /* resort to clicks for unhandled button presses */
  if (drag->type == GXK_DRAG_START && drag->state == GXK_DRAG_UNHANDLED &&
      event && event->type == GDK_BUTTON_PRESS)
    {
      drag->state = GXK_DRAG_HANDLED;
      if (drag->canvas_drag)
        g_signal_emit (self, signal_canvas_clicked, 0, drag->button, drag->start_tick, drag->start_note, event);
      else if (drag->left_panel_drag)
        g_signal_emit (self, signal_piano_clicked, 0, drag->button, drag->start_tick, drag->start_note, event);
    }
}

static void
piano_roll_update (BstPianoRoll *self,
		   guint         tick,
		   guint         duration,
		   gint          min_note,
		   gint          max_note)
{
  gint note;
  
  duration = MAX (duration, 1);
  for (note = min_note; note <= max_note; note++)
    piano_roll_queue_expose (self, CANVAS (self), note, tick, tick + duration - 1);
}

static void
piano_roll_release_proxy (BstPianoRoll *self)
{
  gxk_toplevel_delete (GTK_WIDGET (self));
  bst_piano_roll_set_proxy (self, 0);
}

void
bst_piano_roll_set_proxy (BstPianoRoll *self,
			  SfiProxy      proxy)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));
  if (proxy)
    {
      g_return_if_fail (BSE_IS_ITEM (proxy));
      g_return_if_fail (bse_item_get_project (proxy) != 0);
    }
  
  if (self->proxy)
    {
      bse_proxy_disconnect (self->proxy,
			    "any_signal", piano_roll_release_proxy, self,
			    "any_signal", piano_roll_update, self,
			    NULL);
      bse_item_unuse (self->proxy);
    }
  self->proxy = proxy;
  if (self->proxy)
    {
      self->init_vpos = TRUE;
      bse_item_use (self->proxy);
      bse_proxy_connect (self->proxy,
			 "swapped_signal::release", piano_roll_release_proxy, self,
			 // "swapped_signal::property-notify::uname", piano_roll_update_name, self,
			 "swapped_signal::range-changed", piano_roll_update, self,
			 NULL);
      self->min_note = bse_part_get_min_note (self->proxy);
      self->max_note = bse_part_get_max_note (self->proxy);
      self->max_ticks = bse_part_get_max_tick (self->proxy);
      bst_piano_roll_hsetup (self, self->ppqn, self->qnpt, self->max_ticks, self->hzoom);
    }
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
piano_roll_queue_region (BstPianoRoll *self,
			 guint         tick,
			 guint         duration,
			 gint          min_note,
			 gint          max_note)
{
  if (self->proxy && duration)	/* let the part extend the area by spanning notes if necessary */
    bse_part_queue_notes (self->proxy, tick, duration, min_note, max_note);
  piano_roll_update (self, tick, duration, min_note, max_note);
}

void
bst_piano_roll_set_view_selection (BstPianoRoll *self,
				   guint         tick,
				   guint         duration,
				   gint          min_note,
				   gint          max_note)
{
  g_return_if_fail (BST_IS_PIANO_ROLL (self));
  
  if (min_note > max_note || !duration)	/* invalid selection */
    {
      tick = 0;
      duration = 0;
      min_note = 0;
      max_note = 0;
    }
  
  if (self->selection_duration && duration)
    {
      /* if at least one corner of the old an the new selection
       * matches, it's probably worth updating only diff-regions
       */
      if ((tick == self->selection_tick ||
	   tick + duration == self->selection_tick + self->selection_duration) &&
	  (min_note == self->selection_min_note ||
	   max_note == self->selection_max_note))
	{
	  guint start, end;
	  gint note_min, note_max;
	  /* difference on the left */
	  start = MIN (tick, self->selection_tick);
	  end = MAX (tick, self->selection_tick);
	  if (end != start)
	    piano_roll_queue_region (self, start, end - start,
				     MIN (min_note, self->selection_min_note),
				     MAX (max_note, self->selection_max_note));
	  /* difference on the right */
	  start = MIN (tick + duration, self->selection_tick + self->selection_duration);
	  end = MAX (tick + duration, self->selection_tick + self->selection_duration);
	  if (end != start)
	    piano_roll_queue_region (self, start, end - start,
				     MIN (min_note, self->selection_min_note),
				     MAX (max_note, self->selection_max_note));
	  start = MIN (tick, self->selection_tick);
	  end = MAX (tick + duration, self->selection_tick + self->selection_duration);
	  /* difference on the top */
	  note_max = MAX (max_note, self->selection_max_note);
	  note_min = MIN (max_note, self->selection_max_note);
	  if (note_max != note_min)
	    piano_roll_queue_region (self, start, end - start, note_min, note_max);
	  /* difference on the bottom */
	  note_max = MAX (min_note, self->selection_min_note);
	  note_min = MIN (min_note, self->selection_min_note);
	  if (note_max != note_min)
	    piano_roll_queue_region (self, start, end - start, note_min, note_max);
	}
      else
	{
	  /* simply update new and old selection */
	  piano_roll_queue_region (self, self->selection_tick, self->selection_duration,
				   self->selection_min_note, self->selection_max_note);
	  piano_roll_queue_region (self, tick, duration, min_note, max_note);
	}
    }
  else if (self->selection_duration)
    piano_roll_queue_region (self, self->selection_tick, self->selection_duration,
			     self->selection_min_note, self->selection_max_note);
  else /* duration != 0 */
    piano_roll_queue_region (self, tick, duration, min_note, max_note);
  self->selection_tick = tick;
  self->selection_duration = duration;
  self->selection_min_note = min_note;
  self->selection_max_note = max_note;
}

gint
bst_piano_roll_get_vpanel_width (BstPianoRoll *self)
{
  gint width = 0;
  g_return_val_if_fail (BST_IS_PIANO_ROLL (self), 0);
  if (VPANEL (self))
    gdk_window_get_size (VPANEL (self), &width, NULL);
  else
    width = GXK_SCROLL_CANVAS (self)->layout.left_panel_width;
  return width;
}

void
bst_piano_roll_get_paste_pos (BstPianoRoll *self,
			      guint        *tick_p,
			      gint         *note_p)
{
  guint tick, semitone;
  gint octave;
  
  g_return_if_fail (BST_IS_PIANO_ROLL (self));
  
  if (GTK_WIDGET_DRAWABLE (self))
    {
      NoteInfo info;
      gint x, y, width, height;
      gdk_window_get_pointer (CANVAS (self), &x, &y, NULL);
      gdk_window_get_size (CANVAS (self), &width, &height);
      if (x < 0 || y < 0 || x >= width || y >= height)
	{
	  /* fallback value if the pointer is outside the window */
	  x = width / 3;
	  y = height / 3;
	}
      tick = coord_to_tick (self, MAX (x, 0), FALSE);
      coord_to_note (self, MAX (y, 0), &info);
      semitone = info.valid_semitone;
      octave = info.valid_octave;
    }
  else
    {
      semitone = 6;
      octave = (MIN_OCTAVE (self) + MAX_OCTAVE (self)) / 2;
      tick = 0;
    }
  if (note_p)
    *note_p = SFI_NOTE_MAKE_VALID (SFI_NOTE_GENERIC (octave, semitone));
  if (tick_p)
    *tick_p = tick;
}

static void
bst_piano_roll_class_init (BstPianoRollClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  // GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GxkScrollCanvasClass *scroll_canvas_class = GXK_SCROLL_CANVAS_CLASS (class);
  
  gobject_class->dispose = bst_piano_roll_dispose;
  gobject_class->finalize = bst_piano_roll_finalize;
  
  object_class->destroy = bst_piano_roll_destroy;
  
  scroll_canvas_class->hscrollable = TRUE;
  scroll_canvas_class->vscrollable = TRUE;
  scroll_canvas_class->get_layout = piano_roll_get_layout;
  scroll_canvas_class->draw_canvas = bst_piano_roll_draw_canvas;
  scroll_canvas_class->draw_top_panel = bst_piano_roll_draw_hpanel;
  scroll_canvas_class->draw_left_panel = bst_piano_roll_draw_vpanel;
  scroll_canvas_class->update_adjustments = piano_roll_update_adjustments;
  scroll_canvas_class->handle_drag = piano_roll_handle_drag;

  bst_skin_config_add_notify ((BstSkinConfigNotify) piano_roll_class_setup_skin, class);
  piano_roll_class_setup_skin (class);
  
  class->canvas_clicked = NULL;
  
  signal_canvas_drag = g_signal_new ("canvas-drag", G_OBJECT_CLASS_TYPE (class),
				     G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPianoRollClass, canvas_drag),
				     NULL, NULL,
				     bst_marshal_NONE__POINTER,
				     G_TYPE_NONE, 1, G_TYPE_POINTER);
  signal_canvas_clicked = g_signal_new ("canvas-clicked", G_OBJECT_CLASS_TYPE (class),
					G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPianoRollClass, canvas_clicked),
					NULL, NULL,
					bst_marshal_NONE__UINT_UINT_INT_BOXED,
					G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT,
					GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  signal_piano_drag = g_signal_new ("piano-drag", G_OBJECT_CLASS_TYPE (class),
				    G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPianoRollClass, piano_drag),
				    NULL, NULL,
				    bst_marshal_NONE__POINTER,
				    G_TYPE_NONE, 1, G_TYPE_POINTER);
  signal_piano_clicked = g_signal_new ("piano-clicked", G_OBJECT_CLASS_TYPE (class),
				       G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BstPianoRollClass, piano_clicked),
				       NULL, NULL,
				       bst_marshal_NONE__UINT_INT_BOXED,
				       G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_INT,
				       GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}
