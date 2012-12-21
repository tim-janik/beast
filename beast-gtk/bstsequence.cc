/* BEAST - Better Audio System
 * Copyright (C) 2002 Tim Janik
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
#include "bstutils.h"   /* for GScanner */
#include "bstsequence.h"

#include <gtk/gtkdrawingarea.h>


/* --- prototypes --- */
static void     bst_sequence_finalize	(GObject		*object);
static gint	darea_configure_event	(BstSequence		*seq,
					 GdkEventConfigure	*event);
static gint	darea_cross_event	(BstSequence		*seq,
					 GdkEventCrossing	*event);
static gint	darea_expose_event	(BstSequence		*seq,
					 GdkEventExpose		*event);
static gint	darea_button_event	(BstSequence		*seq,
					 GdkEventButton		*event);
static gint	darea_motion_event	(BstSequence		*seq,
					 GdkEventMotion		*event);


/* --- varibales --- */
static guint           seq_changed_signal = 0;


/* --- fucntions --- */
G_DEFINE_TYPE (BstSequence, bst_sequence, GTK_TYPE_HBOX);

static void
bst_sequence_class_init (BstSequenceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  
  gobject_class->finalize = bst_sequence_finalize;

  seq_changed_signal = gtk_signal_new ("seq-changed",
				       GTK_RUN_LAST,
				       GTK_CLASS_TYPE (object_class),
				       GTK_SIGNAL_OFFSET (BstSequenceClass, seq_changed),
				       gtk_signal_default_marshaller,
				       GTK_TYPE_NONE, 0);
}

static void
bst_sequence_init (BstSequence *seq)
{
  GtkWidget *frame;
  gtk_widget_show (GTK_WIDGET (seq));

  frame = gtk_widget_new (GTK_TYPE_FRAME,
			  "visible", TRUE,
			  "label", NULL,
			  "shadow", GTK_SHADOW_IN,
			  "border_width", 0,
			  "parent", seq,
			  NULL);

  seq->darea = (GtkWidget*) g_object_new (GTK_TYPE_DRAWING_AREA,
                                          "visible", TRUE,
                                          "height_request", 50,
                                          "parent", frame,
                                          "events", (GDK_EXPOSURE_MASK |
                                                     GDK_ENTER_NOTIFY_MASK |
                                                     GDK_LEAVE_NOTIFY_MASK |
                                                     GDK_BUTTON_PRESS_MASK |
                                                     GDK_BUTTON_RELEASE_MASK |
                                                     GDK_BUTTON1_MOTION_MASK),
                                          NULL);
  g_object_connect (seq->darea,
		    "swapped_signal::destroy", g_nullify_pointer, &seq->darea,
		    "swapped_object_signal::configure_event", darea_configure_event, seq,
		    "swapped_object_signal::expose_event", darea_expose_event, seq,
		    "swapped_object_signal::enter_notify_event", darea_cross_event, seq,
		    "swapped_object_signal::leave_notify_event", darea_cross_event, seq,
		    "swapped_object_signal::button_press_event", darea_button_event, seq,
		    "swapped_object_signal::button_release_event", darea_button_event, seq,
		    "swapped_object_signal::motion_notify_event", darea_motion_event, seq,
		    NULL);

  seq->n_rows = 13;
  seq->sdata = bse_note_sequence_new ();
  bse_note_seq_resize (seq->sdata->notes, 1);
  seq->sdata->offset = SFI_NOTE_C (SFI_KAMMER_OCTAVE);
}

static void
bst_sequence_finalize (GObject *object)
{
  BstSequence *seq = BST_SEQUENCE (object);

  bse_note_sequence_free (seq->sdata);

  /* chain parent class' handler */
  G_OBJECT_CLASS (bst_sequence_parent_class)->finalize (object);
}

void
bst_sequence_set_seq (BstSequence     *seq,
		      BseNoteSequence *sdata)
{
  g_return_if_fail (BST_IS_SEQUENCE (seq));

  bse_note_sequence_free (seq->sdata);
  if (sdata)
    seq->sdata = bse_note_sequence_copy_shallow (sdata);
  else
    {
      seq->sdata = bse_note_sequence_new ();
      bse_note_seq_resize (seq->sdata->notes, 1);
      seq->sdata->offset = SFI_NOTE_C (SFI_KAMMER_OCTAVE);
    }
  gtk_widget_queue_draw (seq->darea);
}

static gint
darea_configure_event (BstSequence       *seq,
		       GdkEventConfigure *event)
{
  GtkWidget *widget = seq->darea;

  gdk_window_set_background (widget->window, &widget->style->base[GTK_WIDGET_STATE (widget)]);

  return TRUE;
}

static gint
darea_cross_event (BstSequence      *seq,
		   GdkEventCrossing *event)
{
  GtkWidget *widget = seq->darea;

  if (event->type == GDK_ENTER_NOTIFY)
    seq->entered = TRUE;
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (seq->entered)
	seq->entered = FALSE;
    }

  gtk_widget_queue_draw (widget);

  return TRUE;
}

static gint
darea_expose_event (BstSequence    *seq,
		    GdkEventExpose *event)
{
  GtkWidget *widget = seq->darea;
  BseNoteSequence *sdata = seq->sdata;
  GdkDrawable *drawable = widget->window;
  GdkGC *fg_gc = widget->style->black_gc;
  GdkGC *bg_gc = widget->style->base_gc[GTK_WIDGET_STATE (widget)];
  GdkGC *hl_gc = widget->style->bg_gc[GTK_STATE_SELECTED];
  gint width, height, maxx, maxy;
  gfloat nwidth, row_height;
  gint i, j;

  gdk_window_get_size (widget->window, &width, &height);
  maxx = width - 1;
  maxy = height - 1;

  /* clear background
   */
  gdk_draw_rectangle (drawable, bg_gc,
		      TRUE,
		      0,
		      0,
		      width,
		      height);

  /* draw rectangles */
  row_height = maxy / (gfloat) seq->n_rows;
  nwidth = maxx / (gfloat) sdata->notes->n_notes;
  for (i = 0; i < sdata->notes->n_notes; i++)
    for (j = 0; j < seq->n_rows; j++)
      {
	gboolean ncheck = sdata->notes->notes[i] == (seq->n_rows - 1 - j) + sdata->offset;
	
	if (ncheck)
	  gdk_draw_rectangle (drawable, hl_gc, TRUE,
			      i * nwidth + 0.5, j * row_height + 0.5,
			      nwidth, row_height);
	gdk_draw_rectangle (drawable, fg_gc, FALSE,
			    i * nwidth + 0.5, j * row_height + 0.5,
			    nwidth, row_height);
      }
  
  return TRUE;
}

static gint
darea_button_event (BstSequence    *seq,
		    GdkEventButton *event)
{	
  GtkWidget *widget = seq->darea;
  BseNoteSequence *sdata = seq->sdata;
  gboolean changed = FALSE;

  if (event->type == GDK_BUTTON_PRESS)
    {
      if (event->button == 1)
	{
	  gint width, height, maxx, maxy;
	  gfloat nwidth, row_height;
	  gint dx, dy;
	  
	  gdk_window_get_size (widget->window, &width, &height);
	  maxx = width - 1;
	  maxy = height - 1;
	  row_height = maxy / (gfloat) seq->n_rows;
	  nwidth = maxx / (gfloat) sdata->notes->n_notes;
	  
	  dx = event->x / nwidth;
	  dy = event->y / row_height;
	  dy = seq->n_rows - 1 - CLAMP (dy, 0, seq->n_rows - 1);
	  if (dx >= 0 && dx < sdata->notes->n_notes &&
	      sdata->notes->notes[dx] != dy + sdata->offset)
	    {
	      sdata->notes->notes[dx] = dy + sdata->offset;
	      changed = TRUE;
	    }
	}
      else if (event->button == 2)
	{
	  guint i;

	  for (i = 0; i < sdata->notes->n_notes; i++)
	    sdata->notes->notes[i] = sdata->offset;
	  changed = TRUE;
	}
    }
  if (changed)
    {
      g_signal_emit (seq, seq_changed_signal, 0);
      gtk_widget_queue_draw (widget);
    }
  
  return TRUE;
}

static gint
darea_motion_event (BstSequence    *seq,
		    GdkEventMotion *event)
{
  GtkWidget *widget = seq->darea;
  BseNoteSequence *sdata = seq->sdata;
  gboolean changed = FALSE;

  if (event->type == GDK_MOTION_NOTIFY && !event->is_hint)
    {
      gint width, height, maxx, maxy;
      gfloat nwidth, row_height;
      gint dx, dy;

      gdk_window_get_size (widget->window, &width, &height);
      maxx = width - 1;
      maxy = height - 1;
      row_height = maxy / (gfloat) seq->n_rows;
      nwidth = maxx / (gfloat) sdata->notes->n_notes;

      dx = event->x / nwidth;
      dy = event->y / row_height;
      dy = seq->n_rows - 1 - CLAMP (dy, 0, seq->n_rows - 1);
      if (dx >= 0 && dx < sdata->notes->n_notes &&
	  sdata->notes->notes[dx] != dy + sdata->offset)
	{
	  sdata->notes->notes[dx] = dy + sdata->offset;
	  changed = TRUE;
	}
    }
  if (changed)
    {
      g_signal_emit (seq, seq_changed_signal, 0);
      gtk_widget_queue_draw (widget);
    }
  
  return TRUE;
}
