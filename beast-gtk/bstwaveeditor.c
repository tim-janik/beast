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
  COL_NAME,
  COL_LOOP,
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
bst_wave_editor_init (BstWaveEditor *wave_editor)
{
  /* setup main container */
  wave_editor->main_vbox = GTK_WIDGET (wave_editor);

  wave_editor->chunk_store = gtk_list_store_new (N_COLS,
						 G_TYPE_STRING,  /* COL_OSC_FREQ */
						 G_TYPE_STRING,  /* COL_MIX_FREQ */
						 G_TYPE_STRING,  /* COL_NAME */
						 G_TYPE_STRING   /* COL_LOOP */
						 );

  /* playback handle */
  wave_editor->phandle = bst_play_back_handle_new ();
}

static void
bst_wave_editor_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  BstWaveEditor *wave_editor = BST_WAVE_EDITOR (object);

  switch (prop_id)
    {
    case PARAM_WAVE:
      bst_wave_editor_set_wave (wave_editor, g_value_get_uint (value));
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
  BstWaveEditor *wave_editor = BST_WAVE_EDITOR (object);

  switch (prop_id)
    {
    case PARAM_WAVE:
      g_value_set_uint (value, wave_editor->wave);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
bst_wave_editor_set_wave (BstWaveEditor *wave_editor,
			  BswProxy	 wave)
{
  g_return_if_fail (BST_IS_WAVE_EDITOR (wave_editor));

  if (wave != wave_editor->wave)
    {
      if (wave_editor->wave)
	{
	  bsw_item_unuse (wave_editor->wave);
	}
      if (BSW_IS_WAVE (wave))
	wave_editor->wave = wave;
      else
	wave_editor->wave = 0;
      wave_editor->n_channels = 0;
      if (wave_editor->wave)
	{
	  bsw_item_use (wave_editor->wave);
	  wave_editor->n_channels = bsw_wave_n_channels (wave_editor->wave);
	}
      bst_wave_editor_rebuild (wave_editor);
      g_object_notify (G_OBJECT (wave_editor), "wave");
    }
}

static void
bst_wave_editor_destroy (GtkObject *object)
{
  BstWaveEditor *wave_editor = BST_WAVE_EDITOR (object);

  if (wave_editor->phandle)
    {
      bst_play_back_handle_destroy (wave_editor->phandle);
      wave_editor->phandle = NULL;
    }

  if (wave_editor->proc_editor)
    gtk_widget_destroy (wave_editor->proc_editor);

  bst_wave_editor_set_wave (wave_editor, 0);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_wave_editor_finalize (GObject *object)
{
  BstWaveEditor *wave_editor = BST_WAVE_EDITOR (object);

  g_object_unref (wave_editor->chunk_store);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget*
bst_wave_editor_new (BswProxy wave)
{
  GtkWidget *widget;
  
  widget = gtk_widget_new (BST_TYPE_WAVE_EDITOR, "wave", wave, NULL);
  
  return widget;
}

static void
tree_selection (BstWaveEditor    *wave_editor,
		GtkTreeSelection *tsel)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  BstQSampler *qsampler = wave_editor->qsampler;

  if (gtk_tree_selection_get_selected (tsel, &model, &iter))
    {
      gchar *osc_str, *mix_str;
      gfloat osc_freq, mix_freq;

      gtk_tree_model_get (model, &iter,
			  COL_OSC_FREQ, &osc_str,
			  COL_MIX_FREQ, &mix_str,
			  -1);
      osc_freq = g_strtod (osc_str, NULL);
      mix_freq = g_strtod (mix_str, NULL);
      g_free (osc_str);
      g_free (mix_str);
      
      wave_editor->wchunk = bse_wave_lookup_chunk (bse_object_from_id (wave_editor->wave), osc_freq, mix_freq);
    }

  bst_qsampler_set_source (qsampler, 0, NULL, NULL, NULL);
}

static void
play_back_wchunk_on (BstWaveEditor *wave_editor)
{
  bst_play_back_handle_stop (wave_editor->phandle);
  // bst_play_back_handle_set (wave_editor->phandle, wave_editor->wchunk, wave_editor->wchunk->osc_freq);
  bst_play_back_handle_start (wave_editor->phandle);
  gtk_widget_hide (wave_editor->preview_on);
  gtk_widget_show (wave_editor->preview_off);
}

static void
play_back_wchunk_off (BstWaveEditor *wave_editor)
{
  bst_play_back_handle_stop (wave_editor->phandle);
  gtk_widget_hide (wave_editor->preview_off);
  gtk_widget_show (wave_editor->preview_on);
}

static void
tree_row_activated (BstWaveEditor     *wave_editor,
		    GtkTreePath       *path,
		    GtkTreeViewColumn *column,
		    GtkTreeView	      *tree_view)
{
  // GtkTreeSelection *tsel = gtk_tree_view_get_selection (tree_view);

  play_back_wchunk_on (wave_editor);
}

void
bst_wave_editor_rebuild (BstWaveEditor *wave_editor)
{
  GtkWidget *qsampler_parent, *sbar, *tree, *scwin, *entry, *mask_parent, *any;
  GtkTreeSelection *tsel;
  BseWave *bwave;
  gpointer gmask;
  guint i;

  g_return_if_fail (BST_IS_WAVE_EDITOR (wave_editor));

  gtk_container_foreach (GTK_CONTAINER (wave_editor), (GtkCallback) gtk_widget_destroy, NULL);
  gtk_list_store_clear (wave_editor->chunk_store);
  wave_editor->wchunk = NULL;
  if (!wave_editor->wave)
    return;

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
		       "model", wave_editor->chunk_store,
		       "border_width", 10,
		       "parent", scwin,
		       NULL);
  g_object_connect (tree,
		    "swapped_object_signal::row_activated", tree_row_activated, wave_editor,
		    NULL);
  tsel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_SINGLE);
  gtk_box_pack_start (GTK_BOX (wave_editor->main_vbox), scwin, FALSE, TRUE, 0);
  qsampler_parent = gtk_widget_new (GTK_TYPE_VBOX,
				    "visible", TRUE,
				    "spacing", 1,
				    "parent", wave_editor->main_vbox,
				    "border_width", 0,
				    NULL);

  /* setup qsampler
   */
  sbar = gtk_widget_new (GTK_TYPE_HSCROLLBAR,
			 "visible", TRUE,
			 NULL);
  wave_editor->qsampler = g_object_new (BST_TYPE_QSAMPLER,
					"visible", TRUE,
					"parent", qsampler_parent,
					"height_request", 80,
					NULL);
  g_object_set_data (G_OBJECT (wave_editor->qsampler), "wave_editor", wave_editor);

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
					  "title", "Name",
					  "sizing", GTK_TREE_VIEW_COLUMN_GROW_ONLY,
					  "resizable", TRUE,
					  NULL),
			    g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
					  "xalign", 0.0,
					  NULL),
			    "text", COL_NAME,
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

  /* add chunks to list
   */
  bwave = bse_object_from_id (wave_editor->wave);
  for (i = 0; i < bwave->n_wchunks; i++)
    {
      GslWaveChunk *wchunk = g_slist_nth_data (bwave->wave_chunks, i); // FIXME
      GtkTreeIter iter;
      gchar *l = g_strdup_printf ("L:%u {0x%08lx,0x%08lx}", wchunk->loop_count, wchunk->loop_start, wchunk->loop_end);
      gchar *str_ofreq = g_strdup_printf ("%.2f", wchunk->osc_freq);
      gchar *str_mfreq = g_strdup_printf ("%.2f", wchunk->mix_freq);
      
      gtk_list_store_append (wave_editor->chunk_store, &iter);
      gtk_list_store_set (wave_editor->chunk_store, &iter,
			  COL_OSC_FREQ, str_ofreq,
			  COL_MIX_FREQ, str_mfreq,
			  COL_NAME, bwave->wave_name,	// FIXME
			  COL_LOOP, l,
			  -1);
      g_free (str_ofreq);
      g_free (str_mfreq);
      g_free (l);
    }

  /* setup (initial) list selection
   */
  g_object_connect (tsel,
		    "swapped_signal::changed", tree_selection, wave_editor,
		    NULL);
  gtk_tree_selection_select_spath (tsel, "0");

  /* setup qsampler zoom and vscale
   */
  mask_parent = bst_gmask_container_create (BST_TOOLTIPS, 5);
  gtk_box_pack_start (GTK_BOX (wave_editor->main_vbox), mask_parent, FALSE, TRUE, 0);
  entry = g_object_new (GTK_TYPE_SPIN_BUTTON,
			"adjustment", wave_editor->zoom_adjustment,
			"digits", 5,
			"visible", TRUE,
			NULL);
  gmask = bst_gmask_quick (mask_parent, 0, "Stuff:", entry, NULL);

  /* setup preview button
   */
  any = g_object_new (GTK_TYPE_HBOX,
		      "visible", TRUE,
		      NULL);
  wave_editor->preview_on = bst_stock_button (BST_STOCK_PREVIEW_AUDIO, "_Preview");
  wave_editor->preview_off = bst_stock_button (BST_STOCK_PREVIEW_NOAUDIO, "_Preview");
  gtk_container_add (GTK_CONTAINER (any), wave_editor->preview_on);
  gtk_container_add (GTK_CONTAINER (any), wave_editor->preview_off);
  g_object_connect (wave_editor->preview_on,
		    "swapped_signal::clicked", play_back_wchunk_on, wave_editor,
		    NULL);
  g_object_connect (wave_editor->preview_off,
		    "swapped_signal::clicked", play_back_wchunk_off, wave_editor,
		    NULL);
  gtk_widget_show (wave_editor->preview_on);
  gtk_widget_hide (wave_editor->preview_off);
  gmask = bst_gmask_quick (mask_parent, 2, NULL, any, NULL);
}
