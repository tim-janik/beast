/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-1999, 2000-2002 Tim Janik
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
#include	"bstwaveeditor.h"

#include	"bstplayback.h"
#include	"bstprocedure.h"
#include	"bstmenus.h"
#include	<bse/gsldatacache.h>	// FIXME
#include	<bse/gslwavechunk.h>	// FIXME
#include	<bse/gsldatahandle.h>	// FIXME
#include	<gdk/gdkkeysyms.h>



enum {
  SCROLL_NONE  = 0,
  SCROLL_BOTH  = 3,
  SCROLL_LEFT  = 1,
  SCROLL_RIGHT = 2,
};

/* --- wave chunk list model --- */
enum {
  COL_OSC_FREQ,
  COL_MIX_FREQ,
  COL_LOOP,
  COL_WAVE_NAME,
  COL_FILE_NAME,
  N_COLS
};


/* --- prototypes --- */
static void	bst_wave_editor_class_init	(BstWaveEditorClass	*klass);
static void	bst_wave_editor_init		(BstWaveEditor		*wave_editor);
static void	bst_wave_editor_destroy		(GtkObject		*object);
static void	bst_wave_editor_finalize	(GObject		*object);
static void	tree_row_activated		(BstWaveEditor		*self,
						 GtkTreePath		*path,
						 GtkTreeViewColumn	*column,
						 GtkTreeView		*tree_view);
static void	tree_selection_changed		(BstWaveEditor		*self,
						 GtkTreeSelection	*tsel);
static void	adjustments_changed		(BstWaveEditor		*self,
						 GtkAdjustment		*adjustment);
static void	playpos_changed			(BstWaveEditor		*self,
						 GtkAdjustment		*adjustment);
static void	change_draw_mode		(BstWaveEditor		*self,
						 GtkOptionMenu		*omenu);
static void	change_scroll_mode		(BstWaveEditor		*self,
						 GtkOptionMenu		*omenu);
static void	play_back_button_clicked	(BstWaveEditor		*self);
static void	wave_chunk_fill_value		(BstWaveEditor *self,
						 guint          column,
						 guint          row,
						 GValue        *value);
static void	wave_editor_set_n_qsamplers	(BstWaveEditor		*self,
						 guint			 n_qsamplers);


/* --- static variables --- */
static gpointer		   parent_class = NULL;


/* --- functions --- */
GtkType
bst_wave_editor_get_type (void)
{
  static GtkType wave_editor_type = 0;
  
  if (!wave_editor_type)
    {
      GtkTypeInfo wave_editor_info =
      {
	"BstWaveEditor",
	sizeof (BstWaveEditor),
	sizeof (BstWaveEditorClass),
	(GtkClassInitFunc) bst_wave_editor_class_init,
	(GtkObjectInitFunc) bst_wave_editor_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      wave_editor_type = gtk_type_unique (GTK_TYPE_VBOX, &wave_editor_info);
    }
  
  return wave_editor_type;
}

static void
bst_wave_editor_class_init (BstWaveEditorClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  // GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = bst_wave_editor_finalize;

  object_class->destroy = bst_wave_editor_destroy;
}

static void
bst_wave_editor_init (BstWaveEditor *self)
{
  GtkTreeSelection *tsel;
  GtkWidget *any, *paned;
  gpointer gmask;
  
  /* setup main container */
  self->main_vbox = GTK_WIDGET (self);

  /* wave chunk list model */
  self->chunk_wrapper = gxk_list_wrapper_new (N_COLS,
					      G_TYPE_STRING,  /* COL_OSC_FREQ */
					      G_TYPE_STRING,  /* COL_MIX_FREQ */
					      G_TYPE_STRING,  /* COL_LOOP */
					      G_TYPE_STRING,  /* COL_WAVE_NAME */
					      G_TYPE_STRING   /* COL_FILE_NAME */
					      );
  g_signal_connect_object (self->chunk_wrapper, "fill-value",
			   G_CALLBACK (wave_chunk_fill_value),
			   self, G_CONNECT_SWAPPED);

  /* chunk list / qsampler paned container */
  paned = g_object_new (GTK_TYPE_VPANED,
			"visible", TRUE,
			"parent", self->main_vbox,
			NULL);

  /* chunk list widgets */
  any = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
		      "visible", TRUE,
		      "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
		      "vscrollbar_policy", GTK_POLICY_ALWAYS,
		      "height_request", 120,
		      "border_width", 5,
		      "shadow_type", GTK_SHADOW_IN,
		      NULL);
  gtk_paned_pack1 (GTK_PANED (paned), any, TRUE, TRUE);
  self->tree = g_object_new (GTK_TYPE_TREE_VIEW,
			     "visible", TRUE,
			     "can_focus", TRUE, /* FALSE, */
			     "model", self->chunk_wrapper,
			     "border_width", 10,
			     "parent", any,
			     NULL);
  gxk_nullify_on_destroy (self->tree, &self->tree);
  g_object_connect (self->tree, "swapped_object_signal::row_activated", tree_row_activated, self, NULL);
  tsel = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->tree));
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_SINGLE);
  g_object_connect (tsel, "swapped_object_signal::changed", tree_selection_changed, self, NULL);

  /* add columns to chunk list */
  gxk_tree_view_append_text_columns (GTK_TREE_VIEW (self->tree), N_COLS,
				     COL_OSC_FREQ, "", 0.0, "OscFreq",
				     COL_MIX_FREQ, "", 0.0, "MixFreq",
				     COL_LOOP, "", 0.0, "Loop",
				     COL_WAVE_NAME, "", 0.0, "WaveName",
				     COL_FILE_NAME, "", 0.0, "FileName");

  /* qsampler container */
  self->qsampler_parent = g_object_new (GTK_TYPE_VBOX,
					"visible", TRUE,
					"spacing", 1,
					"border_width", 0,
					NULL);
  gxk_nullify_on_destroy (self->qsampler_parent, &self->qsampler_parent);
  gtk_paned_pack2 (GTK_PANED (paned), self->qsampler_parent, TRUE, TRUE);

  /* GUI mask container */
  self->gmask_parent = bst_gmask_container_create (5, TRUE);
  gxk_nullify_on_destroy (self->gmask_parent, &self->gmask_parent);
  gtk_box_pack_start (GTK_BOX (self->main_vbox), self->gmask_parent, FALSE, TRUE, 0);

  /* qsampler (horizontal) zoom */
  self->zoom_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (100, 1e-16, 1e+16, 0.1, 10, 0));
  g_object_connect (self->zoom_adjustment, "swapped_signal_after::value_changed", adjustments_changed, self, NULL);
  gxk_nullify_on_destroy (self->zoom_adjustment, &self->zoom_adjustment);
  any = g_object_new (GTK_TYPE_SPIN_BUTTON,
		      "adjustment", self->zoom_adjustment,
		      "digits", 5,
		      "visible", TRUE,
		      NULL);
  gmask = bst_gmask_quick (self->gmask_parent, 0, "Zoom:", any, NULL);

  /* qsampler vscale */
  self->vscale_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (100, 1e-16, 1e+16, 1, 10, 0));
  g_object_connect (self->vscale_adjustment, "swapped_signal_after::value_changed", adjustments_changed, self, NULL);
  gxk_nullify_on_destroy (self->vscale_adjustment, &self->vscale_adjustment);
  any = g_object_new (GTK_TYPE_SPIN_BUTTON,
		      "adjustment", self->vscale_adjustment,
		      "digits", 5,
		      "visible", TRUE,
		      NULL);
  gmask = bst_gmask_quick (self->gmask_parent, 1, "VScale:", any, NULL);

  /* qsampler draw type choice */
  any = g_object_new (GTK_TYPE_OPTION_MENU, "visible", TRUE, NULL);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (any),
			    bst_choice_menu_createv ("<BEAST-WaveEditor>/QSamplerDrawType",
						     BST_CHOICE (BST_QSAMPLER_DRAW_CRANGE, "Shape Range", NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_ZERO_SHAPE, "Shape Average", NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_MINIMUM_SHAPE, "Shape Minimum", NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_MAXIMUM_SHAPE, "Shape Maximum", NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_CSHAPE, "Sketch Range", NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_MIDDLE_LINE, "Sketch Average", NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_MINIMUM_LINE, "Sketch Minimum", NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_MAXIMUM_LINE, "Sketch Maximum", NONE),
						     BST_CHOICE_END));
  g_object_connect (any, "swapped_signal::changed", change_draw_mode, self, NULL);
  gtk_option_menu_set_history (GTK_OPTION_MENU (any), 0);
  self->draw_mode = BST_QSAMPLER_DRAW_CRANGE;
  gmask = bst_gmask_quick (self->gmask_parent, 2, NULL, any, NULL);

  /* playback handle */
  self->phandle = bst_play_back_handle_new ();
  self->playback_length = 0;

  /* preview auto scroll choice */
  any = g_object_new (GTK_TYPE_OPTION_MENU, "visible", TRUE, NULL);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (any),
			    bst_choice_menu_createv ("<BEAST-WaveEditor>/PreviewScrollType",
						     BST_CHOICE (SCROLL_NONE, "Scroll None", NONE),
						     BST_CHOICE (SCROLL_BOTH, "Scroll Both", NONE),
						     BST_CHOICE (SCROLL_LEFT, "Scroll Left", NONE),
						     BST_CHOICE (SCROLL_RIGHT, "Scroll Right", NONE),
						     BST_CHOICE_END));
  g_object_connect (any, "swapped_signal::changed", change_scroll_mode, self, NULL);
  gtk_option_menu_set_history (GTK_OPTION_MENU (any), 1);
  self->auto_scroll_mode = SCROLL_BOTH;
  gmask = bst_gmask_quick (self->gmask_parent, 1, NULL, any, NULL);

  /* preview buttons */
  any = g_object_new (GTK_TYPE_HBOX,
		      "visible", TRUE,
		      NULL);
  self->preview_on = gxk_stock_button_child (BST_STOCK_PREVIEW_AUDIO, "Start _Preview");
  self->preview_off = gxk_stock_button_child (BST_STOCK_PREVIEW_NO_AUDIO, "Stop _Preview");
  gtk_container_add (GTK_CONTAINER (any), self->preview_on);
  gtk_container_add (GTK_CONTAINER (any), self->preview_off);
  gxk_nullify_on_destroy (self->preview_on, &self->preview_on);
  gxk_nullify_on_destroy (self->preview_off, &self->preview_off);
  any = g_object_new (GTK_TYPE_BUTTON,
		      "visible", TRUE,
		      "child", any,
		      NULL);
  g_object_connect (any, "swapped_signal::clicked", play_back_button_clicked, self, NULL);
  gtk_widget_show (self->preview_on);
  gtk_widget_hide (self->preview_off);
  gmask = bst_gmask_quick (self->gmask_parent, 2, NULL, any, NULL);
}

static void
bst_wave_editor_destroy (GtkObject *object)
{
  BstWaveEditor *self = BST_WAVE_EDITOR (object);

  bst_wave_editor_set_esample (self, 0);
  if (self->phandle)
    {
      bst_play_back_handle_destroy (self->phandle);
      self->phandle = NULL;
    }
  bst_wave_editor_set_wave (self, 0);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_wave_editor_finalize (GObject *object)
{
  BstWaveEditor *self = BST_WAVE_EDITOR (object);

  g_return_if_fail (self->qsamplers == NULL);

  g_object_unref (self->chunk_wrapper);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
bst_wave_editor_set_wave (BstWaveEditor *self,
			  SfiProxy	 wave)
{
  g_return_if_fail (BST_IS_WAVE_EDITOR (self));

  if (wave != self->wave)
    {
      if (self->wave)
	{
	  bse_item_unuse (self->wave);
	  gxk_list_wrapper_notify_clear (self->chunk_wrapper);
	}
      bst_wave_editor_set_esample (self, 0);
      if (BSE_IS_WAVE (wave))
	self->wave = wave;
      else
	self->wave = 0;
      if (self->wave)
	{
	  bse_item_use (self->wave);
	  /* add wave's chunks to list */
	  gxk_list_wrapper_notify_prepend (self->chunk_wrapper, bse_wave_n_wave_chunks (self->wave));
	  /* setup (initial) list selection */
	  gxk_tree_selection_select_spath (gtk_tree_view_get_selection (GTK_TREE_VIEW (self->tree)), "0");
	}
    }
}

static void
play_back_wchunk_off (BstWaveEditor *self)
{
  guint i;
  if (self->playback_marker)
    g_source_remove (self->playback_marker);
  self->playback_marker = 0;
  bst_play_back_handle_stop (self->phandle);
  bst_play_back_handle_set (self->phandle, 0, 440);
  if (self->preview_off)
    gtk_widget_hide (self->preview_off);
  if (self->preview_on)
    gtk_widget_show (self->preview_on);
  for (i = 0; i < self->n_qsamplers; i++)
    bst_qsampler_set_mark (self->qsamplers[i], 3, 0, 0);
}

static void
update_play_back_marks (gpointer data,
			SfiNum   tick_stamp,
			guint    pcm_pos)
{
  BstWaveEditor *self = data;

  /* initial notify */
  if (!self->tick_stamp || tick_stamp <= self->tick_stamp || pcm_pos <= self->pcm_pos)
    {
      self->tick_stamp = tick_stamp;
      self->pcm_pos = pcm_pos;
      self->pcm_per_tick = 0;		/* be conservative */
      bst_play_back_handle_time_pcm_notify (self->phandle, 40); /* request quick update */
    }
  else /* compute slope */
    {
      SfiNum tdelta = tick_stamp;
      SfiNum pdelta = pcm_pos;
      gdouble slope;
      tdelta -= self->tick_stamp;
      pdelta -= self->pcm_pos;
      slope = pdelta;
      slope /= (gdouble) tdelta;
      self->pcm_per_tick = slope;
      self->tick_stamp = tick_stamp;
      self->pcm_pos = pcm_pos;
      /* updates may slow down now */
      bst_play_back_handle_time_pcm_notify (self->phandle, 500);
    }

  if (pcm_pos > self->playback_length)
    play_back_wchunk_off (self);	/* stop looping */
}

guint64       gsl_engine_tick_stamp_from_systime (guint64       systime); // FIXME

static gboolean
playback_marker (gpointer data)
{
  BstWaveEditor *self;

  GDK_THREADS_ENTER ();
  self = BST_WAVE_EDITOR (data);
  if (self->tick_stamp)
    {
      SfiNum tick_now = gsl_engine_tick_stamp_from_systime (sfi_time_system ()); // FIXME!!!
      guint pcm_pos = self->pcm_pos + (tick_now - self->tick_stamp) * self->pcm_per_tick;
      self->ignore_playpos = TRUE;
      gtk_adjustment_set_value (GTK_RANGE (self->qsampler_playpos)->adjustment,
				pcm_pos * 100.0 / ((gfloat) self->playback_length));
      self->ignore_playpos = FALSE;

      if (1)
	{
	  guint i, qpos = pcm_pos / self->n_qsamplers;	/* n esample channels */
	  for (i = 0; i < self->n_qsamplers; i++)
	    {
	      BstQSampler *qsampler = self->qsamplers[i];
	      
	      bst_qsampler_set_mark (qsampler, 3, qpos, 0);
	      bst_qsampler_force_refresh (qsampler);
	      switch (self->auto_scroll_mode)
		{
		case SCROLL_LEFT:	bst_qsampler_scroll_lbounded (qsampler, qpos, 0.9, 0.0);	break;
		case SCROLL_RIGHT:	bst_qsampler_scroll_rbounded (qsampler, qpos, 0.9, 0.0);	break;
		case SCROLL_BOTH:	bst_qsampler_scroll_bounded (qsampler, qpos, 0.9, 0.0);		break;
		}
	      bst_qsampler_set_mark (qsampler, 3, qpos, BST_QSAMPLER_PRELIGHT);
	    }
	}
    }
  GDK_THREADS_LEAVE ();
  return TRUE;
}

static void
play_back_wchunk_on (BstWaveEditor *self)
{
  if (self->playback_marker)
    g_source_remove (self->playback_marker);
  self->playback_marker = 0;
  bst_play_back_handle_stop (self->phandle);
  if (self->esample && self->esample_open)
    {
      self->playback_length = bse_editable_sample_get_length (self->esample);
      bst_play_back_handle_set (self->phandle,
				self->esample,
				bse_editable_sample_get_osc_freq (self->esample));
      bst_play_back_handle_start (self->phandle);
      /* request updates */
      bst_play_back_handle_pcm_notify (self->phandle, 40, update_play_back_marks, self); /* request quick update */
    }
  if (bst_play_back_handle_is_playing (self->phandle))
    {
      gtk_widget_hide (self->preview_on);
      gtk_widget_show (self->preview_off);
      self->playback_marker = g_timeout_add (50, playback_marker, self);
      /* reset pcm aproximation stepper */
      self->tick_stamp = 0;
      self->pcm_pos = 0;
      self->pcm_per_tick = 0;		/* be conservative */
    }
  else
    {
      gtk_widget_hide (self->preview_off);
      gtk_widget_show (self->preview_on);
    }
}

static void
play_back_button_clicked (BstWaveEditor *self)
{
  if (bst_play_back_handle_is_playing (self->phandle))
    play_back_wchunk_off (self);
  else
    play_back_wchunk_on (self);
}

static void
wave_editor_set_n_qsamplers (BstWaveEditor *self,
			     guint          n_qsamplers)
{
  GtkWidget *qsampler_parent = self->qsampler_parent;

  if (!qsampler_parent)
    n_qsamplers = 0;

  if (self->n_qsamplers != n_qsamplers)
    {
      guint i;

      /* playback position scale */
      if (!self->qsampler_playpos && n_qsamplers)
	{
	  self->qsampler_playpos = g_object_new (GTK_TYPE_HSCALE,
						 "visible", TRUE,
						 "adjustment", gtk_adjustment_new (0, 0, 100, 1, 10, 0),
						 "draw_value", FALSE,
						 "can_focus", FALSE,
						 NULL);
	  gxk_nullify_on_destroy (self->qsampler_playpos, &self->qsampler_playpos);
	  gtk_box_pack_end (GTK_BOX (qsampler_parent), self->qsampler_playpos, FALSE, TRUE, 0);
	  g_object_connect (GTK_RANGE (self->qsampler_playpos)->adjustment,
			    "swapped_signal_after::value_changed", playpos_changed, self,
			    NULL);
	}
      /* horizontal scrollbar */
      if (!self->qsampler_hscroll && n_qsamplers)
	{
	  self->qsampler_hscroll = gtk_widget_new (GTK_TYPE_HSCROLLBAR,
						   "visible", TRUE,
						   NULL);
	  g_object_connect (self->qsampler_hscroll,
			    "swapped_signal::destroy", g_nullify_pointer, &self->qsampler_hscroll,
			    NULL);
	  gtk_box_pack_end (GTK_BOX (qsampler_parent), self->qsampler_hscroll, FALSE, TRUE, 0);
	}
      /* delete superfluous samplers */
      for (i = n_qsamplers; i < self->n_qsamplers; i++)
	gtk_widget_destroy (GTK_WIDGET (self->qsamplers[i]));
      /* resize */
      self->qsamplers = g_renew (BstQSampler*, self->qsamplers, n_qsamplers);
      /* create missing */
      for (i = self->n_qsamplers; i < n_qsamplers; i++)
	{
	  BstQSampler *qsampler = g_object_new (BST_TYPE_QSAMPLER,
						"visible", TRUE,
						"parent", qsampler_parent,
						"height_request", 80,
						NULL);
	  bst_qsampler_set_adjustment (qsampler, gtk_range_get_adjustment (GTK_RANGE (self->qsampler_hscroll)));
	  qsampler->owner = self;
	  qsampler->owner_index = i;
	  bst_qsampler_set_source (qsampler, 0, NULL, NULL, NULL);
	  bst_qsampler_set_draw_mode (qsampler, self->draw_mode);
	  self->qsamplers[i] = qsampler;
	}
      self->n_qsamplers = n_qsamplers;
    }
}

void
bst_wave_editor_set_esample (BstWaveEditor *self,
			     SfiProxy       esample)
{
  g_return_if_fail (BST_IS_WAVE_EDITOR (self));
  if (esample)
    g_return_if_fail (BSE_IS_EDITABLE_SAMPLE (esample));

  if (esample != self->esample)
    {
      guint i;

      if (self->phandle)
	play_back_wchunk_off (self);

      if (self->esample)
	{
	  if (self->esample_open)
	    bse_editable_sample_close (self->esample);
	  bse_item_unuse (self->esample);
	}
      self->esample = esample;
      if (self->esample)
	{
	  BseErrorType error;
	  bse_item_use (self->esample);
	  error = bse_editable_sample_open (self->esample);
	  self->esample_open = error == BSE_ERROR_NONE;
	  if (error)
	    g_message ("failed to open sample: %s", bse_error_blurb (error));
	}
      wave_editor_set_n_qsamplers (self, self->esample ? bse_editable_sample_get_n_channels (self->esample) : 0);

      for (i = 0; i < self->n_qsamplers; i++)
	if (self->esample)
	  {
	    bst_qsampler_set_mark (self->qsamplers[i], 3, 0, 0);
	    bst_qsampler_set_source_from_esample (self->qsamplers[i], self->esample, i);
	  }
	else
	  bst_qsampler_set_source (self->qsamplers[i], 0, NULL, NULL, NULL);
    }
}

GtkWidget*
bst_wave_editor_new (SfiProxy wave)
{
  GtkWidget *widget;
  
  widget = gtk_widget_new (BST_TYPE_WAVE_EDITOR,
			   "visible", TRUE,
			   NULL);
  bst_wave_editor_set_wave (BST_WAVE_EDITOR (widget), wave);
  
  return widget;
}

static void
tree_selection_changed (BstWaveEditor    *self,
			GtkTreeSelection *tsel)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (tsel, &model, &iter))
    {
      gchar *osc_str, *mix_str;
      gfloat osc_freq, mix_freq;
      SfiProxy esample;

      g_assert (self->chunk_wrapper == (GxkListWrapper*) model);

      gtk_tree_model_get (model, &iter,
			  COL_OSC_FREQ, &osc_str,
			  COL_MIX_FREQ, &mix_str,
			  -1);
      osc_freq = g_strtod (osc_str, NULL);
      mix_freq = g_strtod (mix_str, NULL);
      g_free (osc_str);
      g_free (mix_str);
      
      esample = bse_wave_use_editable (self->wave, gxk_list_wrapper_get_index (self->chunk_wrapper, &iter));
      bst_wave_editor_set_esample (self, esample);
      bse_item_unuse (esample);
    }
}

static void
tree_row_activated (BstWaveEditor     *self,
		    GtkTreePath       *path,
		    GtkTreeViewColumn *column,
		    GtkTreeView	      *tree_view)
{
  // GtkTreeSelection *tsel = gtk_tree_view_get_selection (tree_view);

  play_back_wchunk_on (self);
}

static void
change_scroll_mode (BstWaveEditor *self,
		    GtkOptionMenu *omenu)
{
  self->auto_scroll_mode = bst_choice_get_last (omenu->menu);
}

static void
change_draw_mode (BstWaveEditor *self,
		  GtkOptionMenu *omenu)
{
  guint i;

  self->draw_mode = bst_choice_get_last (omenu->menu);
  for (i = 0; i < self->n_qsamplers; i++)
    {
      BstQSampler *qsampler = self->qsamplers[i];
      bst_qsampler_set_draw_mode (qsampler, self->draw_mode);
    }
}

static void
adjustments_changed (BstWaveEditor *self,
		     GtkAdjustment *adjustment)
{
  guint i;

  for (i = 0; i < self->n_qsamplers; i++)
    {
      BstQSampler *qsampler = self->qsamplers[i];

      if (adjustment == self->zoom_adjustment)
	bst_qsampler_set_zoom (qsampler, adjustment->value);
      else if (adjustment == self->vscale_adjustment)
	bst_qsampler_set_vscale (qsampler, adjustment->value);
    }
}

static void
playpos_changed (BstWaveEditor *self,
		 GtkAdjustment *adjustment)
{
  if (self->phandle && !self->ignore_playpos && bst_play_back_handle_is_playing (self->phandle))
    {
      /* reset pcm aproximation stepper */
      self->tick_stamp = 0;
      self->pcm_pos = 0;
      self->pcm_per_tick = 0;	/* be conservative */
      bst_play_back_handle_seek_perc (self->phandle, adjustment->value);
      bst_play_back_handle_time_pcm_notify (self->phandle, 40); /* request quick update */
    }
}

static void
wave_chunk_fill_value (BstWaveEditor *self,
		       guint          column,
		       guint          row,
		       GValue        *value)
{
  SfiProxy wave = self->wave;
  guint cidx = row; /* wave chunk index */

  switch (column)
    {
      const gchar *string;
    case COL_OSC_FREQ:
      g_value_set_string_take_ownership (value, g_strdup_printf ("%.2f", bse_wave_chunk_get_osc_freq (wave, cidx)));
      break;
    case COL_MIX_FREQ:
      g_value_set_string_take_ownership (value, g_strdup_printf ("%.2f", bse_wave_chunk_get_mix_freq (wave, cidx)));
      break;
    case COL_LOOP:
      g_value_set_string_take_ownership (value, g_strdup_printf ("L:%u {0x%08x,0x%08x}", 0, 0, 0));
      break;
    case COL_WAVE_NAME:
      bse_proxy_get (wave, "wave-name", &string, NULL);
      g_value_set_string (value, string);
      break;
    case COL_FILE_NAME:
      bse_proxy_get (wave, "file-name", &string, NULL);
      g_value_set_string (value, string);
      break;
    }
}
