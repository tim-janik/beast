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

#include	"bststatusbar.h"
#include	"bstplayback.h"
#include	"bstprocedure.h"
#include	"bstmenus.h"
#include	<gsl/gsldatacache.h>
#include	<gsl/gslwavechunk.h>
#include	<gsl/gsldatahandle.h>
#include	<gdk/gdkkeysyms.h>


enum {
  SCROLL_NONE  = 0,
  SCROLL_BOTH  = 3,
  SCROLL_LEFT  = 1,
  SCROLL_RIGHT = 2,
};

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_WAVE
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
static void	bst_wave_editor_set_property	(GObject		*object,
						 guint			 prop_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	bst_wave_editor_get_property	(GObject		*object,
						 guint			 prop_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	wave_chunk_fill_value		(BstWaveEditor *self,
						 guint          column,
						 guint          row,
						 GValue        *value);


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

  gobject_class->set_property = bst_wave_editor_set_property;
  gobject_class->get_property = bst_wave_editor_get_property;
  gobject_class->finalize = bst_wave_editor_finalize;

  object_class->destroy = bst_wave_editor_destroy;

  g_object_class_install_property (gobject_class, PARAM_WAVE,
				   g_param_spec_uint ("wave", "Wave", NULL,
						      0, G_MAXINT, 0,
						      G_PARAM_READWRITE));
}

static void
bst_wave_editor_init (BstWaveEditor *self)
{
  /* setup main container */
  self->main_vbox = GTK_WIDGET (self);

  /* setup wave chunk list model
   */
  self->chunk_wrapper = gtk_list_wrapper_new (N_COLS,
					      G_TYPE_STRING,  /* COL_OSC_FREQ */
					      G_TYPE_STRING,  /* COL_MIX_FREQ */
					      G_TYPE_STRING,  /* COL_LOOP */
					      G_TYPE_STRING,  /* COL_WAVE_NAME */
					      G_TYPE_STRING   /* COL_FILE_NAME */
					      );
  g_signal_connect_object (self->chunk_wrapper, "fill-value",
			   G_CALLBACK (wave_chunk_fill_value),
			   self, G_CONNECT_SWAPPED);

  /* playback handle */
  self->phandle = bst_play_back_handle_new ();
}

static void
bst_wave_editor_destroy (GtkObject *object)
{
  BstWaveEditor *self = BST_WAVE_EDITOR (object);

  bst_wave_editor_set_sample (self, 0);
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

  g_free (self->qsampler);
  g_object_unref (self->chunk_wrapper);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bst_wave_editor_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  BstWaveEditor *self = BST_WAVE_EDITOR (object);

  switch (prop_id)
    {
    case PARAM_WAVE:
      bst_wave_editor_set_wave (self, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_wave_editor_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
  BstWaveEditor *self = BST_WAVE_EDITOR (object);

  switch (prop_id)
    {
    case PARAM_WAVE:
      g_value_set_uint (value, self->wave);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
bst_wave_editor_set_wave (BstWaveEditor *self,
			  BswProxy	 wave)
{
  g_return_if_fail (BST_IS_WAVE_EDITOR (self));

  if (wave != self->wave)
    {
      if (self->wave)
	{
	  bsw_item_unuse (self->wave);
	}
      if (BSW_IS_WAVE (wave))
	self->wave = wave;
      else
	self->wave = 0;
      self->n_channels = 0;
      if (self->wave)
	{
	  bsw_item_use (self->wave);
	  self->n_channels = bsw_wave_n_channels (self->wave);
	}
      bst_wave_editor_rebuild (self);
      g_object_notify (G_OBJECT (self), "wave");
    }
}

static void
play_back_wchunk_off (BstWaveEditor *self)
{
  bst_play_back_handle_stop (self->phandle);
  bst_play_back_handle_set (self->phandle, 0, 440);
  if (self->preview_off)
    gtk_widget_hide (self->preview_off);
  if (self->preview_on)
    gtk_widget_show (self->preview_on);
  if (self->qsampler)
    {
      guint i;
      for (i = 0; i < self->n_channels; i++)
	bst_qsampler_set_mark (self->qsampler[i], 3, 0, 0);
    }
}

static void
update_play_back_marks (gpointer data,
			guint    pcm_pos)
{
  BstWaveEditor *self = data;
  guint i, qpos;

  qpos = pcm_pos / self->n_channels;
  for (i = 0; i < self->n_channels; i++)
    {
      BstQSampler *qsampler = self->qsampler[i];

      bst_qsampler_set_mark (qsampler, 3, qpos, 0);
      bst_qsampler_force_refresh (qsampler);
      switch (self->scroll_mode)
	{
	case SCROLL_LEFT:	bst_qsampler_scroll_lbounded (qsampler, qpos, 0.98, 0.05);	break;
	case SCROLL_RIGHT:	bst_qsampler_scroll_rbounded (qsampler, qpos, 0.98, 0.05);	break;
	case SCROLL_BOTH:	bst_qsampler_scroll_bounded (qsampler, qpos, 0.98, 0.05);	break;
	}
      bst_qsampler_set_mark (qsampler, 3, qpos, BST_QSAMPLER_PRELIGHT);
    }
  if (pcm_pos > self->playback_length)
    play_back_wchunk_off (self);	/* stop looping */
}

static void
play_back_wchunk_on (BstWaveEditor *self)
{
  bst_play_back_handle_stop (self->phandle);
  if (self->esample)
    {
      self->playback_length = bsw_editable_sample_get_length (self->esample);
      bst_play_back_handle_set (self->phandle,
				self->esample,
				bsw_editable_sample_get_osc_freq (self->esample));
      bst_play_back_handle_start (self->phandle);
      bst_play_back_handle_pcm_notify (self->phandle, 50, update_play_back_marks, self);
      if (self->preview_on)
	gtk_widget_hide (self->preview_on);
      if (self->preview_off)
	gtk_widget_show (self->preview_off);
    }
  else
    {
      if (self->preview_off)
	gtk_widget_hide (self->preview_off);
      if (self->preview_on)
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

void
bst_wave_editor_set_sample (BstWaveEditor *self,
			    BswProxy       esample)
{
  g_return_if_fail (BST_IS_WAVE_EDITOR (self));
  if (esample)
    {
      g_return_if_fail (BSW_IS_EDITABLE_SAMPLE (esample));
      g_return_if_fail (self->n_channels == bsw_editable_sample_get_n_channels (esample));
    }

  if (esample != self->esample)
    {
      if (self->phandle)
	play_back_wchunk_off (self);

      if (self->esample)
	bsw_item_unuse (self->esample);
      self->esample = esample;
      if (self->esample)
	bsw_item_use (self->esample);

      if (self->qsampler)
	{
	  guint i;
	  
	  for (i = 0; i < self->n_channels; i++)
	    if (self->esample)
	      {
		bst_qsampler_set_mark (self->qsampler[i], 3, 0, 0);
		bst_qsampler_set_source_from_esample (self->qsampler[i], self->esample, i);
	      }
	    else
	      bst_qsampler_set_source (self->qsampler[i], 0, NULL, NULL, NULL);
	}
    }
}

GtkWidget*
bst_wave_editor_new (BswProxy wave)
{
  GtkWidget *widget;
  
  widget = gtk_widget_new (BST_TYPE_WAVE_EDITOR, "wave", wave, NULL);
  
  return widget;
}

static void
tree_selection (BstWaveEditor    *self,
		GtkTreeSelection *tsel)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (tsel, &model, &iter))
    {
      gchar *osc_str, *mix_str;
      gfloat osc_freq, mix_freq;
      BswProxy esample;

      g_assert (model == self->chunk_wrapper);

      gtk_tree_model_get (model, &iter,
			  COL_OSC_FREQ, &osc_str,
			  COL_MIX_FREQ, &mix_str,
			  -1);
      osc_freq = g_strtod (osc_str, NULL);
      mix_freq = g_strtod (mix_str, NULL);
      g_free (osc_str);
      g_free (mix_str);
      
      self->wchunk = bse_wave_lookup_chunk (bse_object_from_id (self->wave), osc_freq, mix_freq);
      esample = bsw_wave_use_editable (self->wave, gtk_list_wrapper_get_index (self->chunk_wrapper, &iter));
      bst_wave_editor_set_sample (self, esample);
      bsw_item_unuse (esample);
    }

  // BstQSampler *qsampler = self->qsampler;
  // bst_qsampler_set_source (qsampler, 0, NULL, NULL, NULL);
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
  guint mode = bst_choice_get_last (omenu->menu);

  self->scroll_mode = mode;
}

static void
change_draw_mode (BstWaveEditor *self,
		  GtkOptionMenu *omenu)
{
  guint mode = bst_choice_get_last (omenu->menu);
  guint i;

  for (i = 0; i < self->n_channels; i++)
    {
      BstQSampler *qsampler = self->qsampler[i];

      bst_qsampler_set_draw_mode (qsampler, mode);
    }
}

static void
adjustments_changed (BstWaveEditor *self,
		     GtkAdjustment *adjustment)
{
  guint i;

  for (i = 0; i < self->n_channels; i++)
    {
      BstQSampler *qsampler = self->qsampler[i];

      if (adjustment == self->zoom_adjustment)
	bst_qsampler_set_zoom (qsampler, adjustment->value);
      else if (adjustment == self->vscale_adjustment)
	bst_qsampler_set_vscale (qsampler, adjustment->value);
    }
}

static void
wave_chunk_fill_value (BstWaveEditor *self,
		       guint          column,
		       guint          row,
		       GValue        *value)
{
  BseWave *bwave = bse_object_from_id (self->wave);
  GslWaveChunk *wchunk = g_slist_nth_data (bwave->wave_chunks, row);

  switch (column)
    {
    case COL_OSC_FREQ:
      g_value_set_string_take_ownership (value, g_strdup_printf ("%.2f", wchunk->osc_freq));
      break;
    case COL_MIX_FREQ:
      g_value_set_string_take_ownership (value, g_strdup_printf ("%.2f", wchunk->mix_freq));
      break;
    case COL_LOOP:
      g_value_set_string_take_ownership (value, g_strdup_printf ("L:%u {0x%08lx,0x%08lx}",
								 wchunk->loop_count,
								 wchunk->loop_first,
								 wchunk->loop_last));
      break;
    case COL_WAVE_NAME:
      g_value_set_string (value, bwave->wave_name);
      break;
    case COL_FILE_NAME:
      g_value_set_string (value, bwave->file_name);
      break;
    }
}

void
bst_wave_editor_rebuild (BstWaveEditor *self)
{
  GtkWidget *qsampler_parent, *sbar, *tree, *scwin, *entry, *mask_parent, *any;
  GtkTreeSelection *tsel;
  BseWave *bwave;
  gpointer gmask;
  guint i;
  
  g_return_if_fail (BST_IS_WAVE_EDITOR (self));

  /* clear GUI and chunk list
   */
  bst_wave_editor_set_sample (self, 0);
  gtk_container_foreach (GTK_CONTAINER (self), (GtkCallback) gtk_widget_destroy, NULL);
  gtk_list_wrapper_notify_clear (self->chunk_wrapper);
  self->wchunk = NULL;
  g_free (self->qsampler);
  self->qsampler = NULL;

  /* no wave - nothing to do
   */
  if (!self->wave)
    return;

  /* add chunks to list
   */
  bwave = bse_object_from_id (self->wave);
  gtk_list_wrapper_notify_prepend (self->chunk_wrapper, bwave->n_wchunks);

  /* setup chunk list widgets
   */
  scwin = gtk_widget_new (GTK_TYPE_SCROLLED_WINDOW,
			  "visible", TRUE,
			  "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
			  "vscrollbar_policy", GTK_POLICY_ALWAYS,
			  "height_request", 120,
			  "border_width", 5,
			  "shadow_type", GTK_SHADOW_IN,
			  NULL);
  tree = g_object_new (GTK_TYPE_TREE_VIEW,
		       "visible", TRUE,
		       "can_focus", TRUE, /* FALSE, */
		       "model", self->chunk_wrapper,
		       "border_width", 10,
		       "parent", scwin,
		       NULL);
  g_object_connect (tree,
		    "swapped_object_signal::row_activated", tree_row_activated, self,
		    NULL);
  tsel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_SINGLE);
  gtk_box_pack_start (GTK_BOX (self->main_vbox), scwin, FALSE, TRUE, 0);

  /* setup qsampler
   */
  qsampler_parent = gtk_widget_new (GTK_TYPE_VBOX,
				    "visible", TRUE,
				    "spacing", 1,
				    "parent", self->main_vbox,
				    "border_width", 0,
				    "height_request", self->n_channels * 80 + 20,
				    NULL);
  sbar = gtk_widget_new (GTK_TYPE_HSCROLLBAR,
			 "visible", TRUE,
			 NULL);
  self->qsampler = g_new (BstQSampler*, self->n_channels);
  for (i = 0; i < self->n_channels; i++)
    {
      BstQSampler *qsampler = g_object_new (BST_TYPE_QSAMPLER,
					    "visible", TRUE,
					    "parent", qsampler_parent,
					    NULL);
      bst_qsampler_set_adjustment (qsampler, gtk_range_get_adjustment (GTK_RANGE (sbar)));
      qsampler->owner = self;
      qsampler->owner_index = i;
      bst_qsampler_set_source (qsampler, 0, NULL, NULL, NULL);
      self->qsampler[i] = qsampler;
    }
  gtk_box_pack_start (GTK_BOX (qsampler_parent), sbar, FALSE, TRUE, 0);

  /* GUI mask for various widgets
   */
  mask_parent = bst_gmask_container_create (BST_TOOLTIPS, 5, TRUE);
  gtk_box_pack_start (GTK_BOX (self->main_vbox), mask_parent, FALSE, TRUE, 0);
  
  /* setup qsampler draw type
   */
  any = g_object_new (GTK_TYPE_OPTION_MENU,
		      "visible", TRUE,
		      NULL);
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
  g_object_connect (any,
		    "swapped_signal::changed", change_draw_mode, self,
		    NULL);
  gtk_option_menu_set_history (GTK_OPTION_MENU (any), 0);
  gmask = bst_gmask_quick (mask_parent, 2, NULL, any, NULL);

  /* setup qsampler zoom and vscale
   */
  self->zoom_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (100, 1e-16, 1e+16, 0.1, 10, 0));
  g_object_connect (self->zoom_adjustment,
		    "swapped_signal_after::value_changed", adjustments_changed, self,
		    "swapped_signal::destroy", g_nullify_pointer, &self->zoom_adjustment,
		    NULL);
  entry = g_object_new (GTK_TYPE_SPIN_BUTTON,
			"adjustment", self->zoom_adjustment,
			"digits", 5,
			"visible", TRUE,
			NULL);
  gmask = bst_gmask_quick (mask_parent, 0, "Zoom:", entry, NULL);
  self->vscale_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (100, 1e-16, 1e+16, 1, 10, 0));
  g_object_connect (self->vscale_adjustment,
		    "swapped_signal_after::value_changed", adjustments_changed, self,
		    "swapped_signal::destroy", g_nullify_pointer, &self->vscale_adjustment,
		    NULL);
  entry = g_object_new (GTK_TYPE_SPIN_BUTTON,
			"adjustment", self->vscale_adjustment,
			"digits", 5,
			"visible", TRUE,
			NULL);
  gmask = bst_gmask_quick (mask_parent, 1, "VScale:", entry, NULL);
  
  /* setup preview scroll choice
   */
  any = g_object_new (GTK_TYPE_OPTION_MENU,
		      "visible", TRUE,
		      NULL);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (any),
			    bst_choice_menu_createv ("<BEAST-WaveEditor>/PreviewScrollType",
						     BST_CHOICE (SCROLL_NONE, "Scroll None", NONE),
						     BST_CHOICE (SCROLL_BOTH, "Scroll Both", NONE),
						     BST_CHOICE (SCROLL_LEFT, "Scroll Left", NONE),
						     BST_CHOICE (SCROLL_RIGHT, "Scroll Right", NONE),
						     BST_CHOICE_END));
  g_object_connect (any,
		    "swapped_signal::changed", change_scroll_mode, self,
		    NULL);
  gtk_option_menu_set_history (GTK_OPTION_MENU (any), 1);
  self->scroll_mode = SCROLL_BOTH;
  gmask = bst_gmask_quick (mask_parent, 1, NULL, any, NULL);

  /* setup preview button
   */
  any = g_object_new (GTK_TYPE_HBOX,
		      "visible", TRUE,
		      NULL);
  self->preview_on = bst_stock_button_child (BST_STOCK_PREVIEW_AUDIO, "Start _Preview");
  self->preview_off = bst_stock_button_child (BST_STOCK_PREVIEW_NOAUDIO, "Stop _Preview");
  gtk_container_add (GTK_CONTAINER (any), self->preview_on);
  gtk_container_add (GTK_CONTAINER (any), self->preview_off);
  g_object_connect (self->preview_on,
		    "swapped_signal::destroy", g_nullify_pointer, &self->preview_on,
		    NULL);
  g_object_connect (self->preview_off,
		    "swapped_signal::destroy", g_nullify_pointer, &self->preview_off,
		    NULL);
  any = g_object_new (GTK_TYPE_BUTTON,
		      "visible", TRUE,
		      "child", any,
		      NULL);
  g_object_connect (any,
		    "swapped_signal::clicked", play_back_button_clicked, self,
		    NULL);
  gtk_widget_show (self->preview_on);
  gtk_widget_hide (self->preview_off);
  gmask = bst_gmask_quick (mask_parent, 2, NULL, any, NULL);
  
  /* add columns to chunk list
   */
  gtk_tree_view_add_column (GTK_TREE_VIEW (tree), -1,
			    g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
					  "title", "OscFreq",
					  "sizing", GTK_TREE_VIEW_COLUMN_GROW_ONLY,
					  "resizable", TRUE,
					  NULL),
			    g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
					  "xalign", 0.0,
					  NULL),
			    "text", COL_OSC_FREQ,
			    NULL);
  gtk_tree_view_add_column (GTK_TREE_VIEW (tree), -1,
			    g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
					  "title", "MixFreq",
					  "sizing", GTK_TREE_VIEW_COLUMN_GROW_ONLY,
					  "resizable", TRUE,
					  NULL),
			    g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
					  "xalign", 0.0,
					  NULL),
			    "text", COL_MIX_FREQ,
			    NULL);
  gtk_tree_view_add_column (GTK_TREE_VIEW (tree), -1,
			    g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
					  "title", "Loop",
					  "sizing", GTK_TREE_VIEW_COLUMN_GROW_ONLY,
					  "resizable", TRUE,
					  NULL),
			    g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
					  "xalign", 0.0,
					  NULL),
			    "text", COL_LOOP,
			    NULL);
  gtk_tree_view_add_column (GTK_TREE_VIEW (tree), -1,
			    g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
					  "title", "WaveName",
					  "sizing", GTK_TREE_VIEW_COLUMN_GROW_ONLY,
					  "resizable", TRUE,
					  NULL),
			    g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
					  "xalign", 0.0,
					  NULL),
			    "text", COL_WAVE_NAME,
			    NULL);
  gtk_tree_view_add_column (GTK_TREE_VIEW (tree), -1,
			    g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
					  "title", "FileName",
					  "sizing", GTK_TREE_VIEW_COLUMN_GROW_ONLY,
					  "resizable", TRUE,
					  NULL),
			    g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
					  "xalign", 0.0,
					  NULL),
			    "text", COL_FILE_NAME,
			    NULL);
  
  /* setup (initial) list selection
   */
  g_object_connect (tsel,
		    "swapped_signal::changed", tree_selection, self,
		    NULL);
  gtk_tree_selection_select_spath (tsel, "0");
}
