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
static void	wave_editor_operate		(BstWaveEditor		*wave_editor,
						 BstWaveOps     	 op,
						 GtkWidget        	*menu_item);
static void	wave_editor_exec_proc		(BstWaveEditor		*wave_editor,
						 GType  		 proc_type,
						 GtkWidget        	*menu_item);


/* --- menus --- */
static gchar		  *bst_wave_editor_factories_path = "<BstWave>";
static BstMenuEntry popup_entries[] =
{
#define BST_OP(op) (wave_editor_operate), (BST_WAVE_OP_ ## op)
  { "/<<<<<<",			NULL,		NULL, 0,		"<Tearoff>",	0 },
  { "/Wave",			NULL,		NULL, 0,		"<Title>",	/* FIXME:BST_ICON_WAVE*/0 },
  { "/-----",			NULL,		NULL, 0,		"<Separator>",	0 },
  { "/_Edit/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>",	0 },
  { "/_Select/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>",	0 },
  { "/_Tools/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>",	0 },
  { "/-----",			NULL,		NULL, 0,		"<Separator>",	0 },
  { "/To_ys",			NULL,		NULL, 0,		"<LastBranch>",	0 },
  { "/Toys/<<<<<<",		NULL,		NULL, 0,		"<Tearoff>",	0 },
#undef	BST_OP
};
static guint n_popup_entries = sizeof (popup_entries) / sizeof (popup_entries[0]);


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
  BseCategory *cats;
  guint n_cats;
  
  parent_class = g_type_class_peek_parent (class);

  gobject_class->set_property = bst_wave_editor_set_property;
  gobject_class->get_property = bst_wave_editor_get_property;
  gobject_class->finalize = bst_wave_editor_finalize;

  object_class->destroy = bst_wave_editor_destroy;

  class->popup_entries = NULL;
#if 0
  class->popup_entries = bst_menu_entries_add_bentries (class->popup_entries,
							n_popup_entries,
							popup_entries);
  cats = bse_categories_match_typed ("/Method/BseWave/*", BSE_TYPE_PROCEDURE, &n_cats);
  class->popup_entries = bst_menu_entries_add_categories (class->popup_entries,
							  n_cats,
							  cats,
							  wave_editor_exec_proc);
  g_free (cats);
  cats = bse_categories_match_typed ("/Proc/Toys/*", BSE_TYPE_PROCEDURE, &n_cats);
  class->popup_entries = bst_menu_entries_add_categories (class->popup_entries,
							  n_cats,
							  cats,
							  wave_editor_exec_proc);
  g_free (cats);
#endif
  class->popup_entries = bst_menu_entries_sort (class->popup_entries);

  g_object_class_install_property (gobject_class, PARAM_WAVE,
				   g_param_spec_uint ("wave", "Wave", NULL,
						      0, G_MAXINT, 0,
						      G_PARAM_READWRITE));
}

static void
bst_wave_editor_init (BstWaveEditor *wave_editor)
{
  BstWaveEditorClass *class = BST_WAVE_EDITOR_GET_CLASS (wave_editor);
  GtkItemFactory *factory;

  /* setup main container */
  wave_editor->main_vbox = GTK_WIDGET (wave_editor);
  if (0)
    g_object_connect (wave_editor->main_vbox,
		      "signal::destroy", gtk_widget_destroyed, &wave_editor->main_vbox,
		      NULL);

  wave_editor->chunk_store = gtk_list_store_new (N_COLS,
						 G_TYPE_STRING,  /* COL_OSC_FREQ */
						 G_TYPE_STRING,  /* COL_MIX_FREQ */
						 G_TYPE_STRING,  /* COL_NAME */
						 G_TYPE_STRING   /* COL_LOOP */
						 );

  /* setup the popup menu
   */
#if 0
  factory = gtk_item_factory_new (GTK_TYPE_MENU, bst_wave_editor_factories_path, NULL);
  gtk_window_add_accel_group (GTK_WINDOW (wave_editor), factory->accel_group);
  bst_menu_entries_create_list (factory, class->popup_entries, wave_editor);
  wave_editor->popup = factory->widget;
  gtk_object_set_data_full (GTK_OBJECT (wave_editor),
			    bst_wave_editor_factories_path,
			    factory,
			    (GtkDestroyNotify) gtk_object_unref);
  wave_editor->proc_editor = NULL;
#endif

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

void
bst_wave_editor_gtkfix_default_accels (void)
{
  static gchar *accel_defaults[][2] = {
    { "<BstWave>/Select/All",               "<Control>A", },
    { "<BstWave>/Select/None",              "<Shift><Control>A", },
    { "<BstWave>/Select/Invert",            "<Control>I", },
    { "<BstWave>/Edit/Undo",                "<Control>Z", },
    { "<BstWave>/Edit/Redo",                "<Control>R", },
    { "<BstWave>/Edit/Copy",                "<Control>C", },
    { "<BstWave>/Edit/Cut",                 "<Control>X", },
    { "<BstWave>/Edit/Cut Named...",        "<Shift><Control>X", },
    { "<BstWave>/Edit/Paste",               "<Control>V", },
    { "<BstWave>/Edit/Paste Named...",      "<Shift><Control>V", },
    { "<BstWave>/Edit/Clear",               "<Control>K", },
    { "<BstWave>/Edit/Fill",                "<Control>period", },
    { "<BstWave>/Edit/Clear",               "<Control>K", },
  };
  static guint n_accel_defaults = sizeof (accel_defaults) / sizeof (accel_defaults[0]);
  guint i;

  for (i = 0; i < n_accel_defaults; i++)
    {
      gchar *string = g_strconcat ("(menu-path \"",
				   accel_defaults[i][0],
				   "\" \"",
				   accel_defaults[i][1],
				   "\")",
				   NULL);

      //FIXME; gtk_item_factory_parse_rc_string (string);
      g_free (string);
    }
}

#if 0
static void
wave_editor_exec_proc (BstWaveEditor *wave_editor,
			  GType             proc_type,
			  GtkWidget        *menu_item)
{
  GValue value = { 0, };
  BstWaveEditor *wave_editor;
  BseProcedureClass *procedure;
  BstProcedureShell *proc_shell;
  BseWave *wave;
  GtkWidget *widget;
  guint channel, row;

  widget = GTK_WIDGET (wave_editor);
  wave_editor = BST_WAVE_EDITOR (wave_editor->wave_editor);
  wave = BSE_WAVE (wave_editor->wave);
  pe_channel_row_from_popup (wave_editor, menu_item, &channel, &row);

  gtk_widget_ref (widget);
  bse_object_ref (BSE_OBJECT (wave));

  /* ensure procedure shell
   */
  if (!wave_editor->proc_editor)
    {
      proc_shell = BST_PROCEDURE_SHELL (bst_procedure_shell_new (NULL));
      wave_editor->proc_editor = bst_procedure_editor_from_shell (proc_shell, &wave_editor->proc_editor);
    }
  else
    proc_shell = bst_procedure_editor_get_shell (wave_editor->proc_editor);

  /* setup procedure
   */
  procedure = g_type_class_ref (proc_type);
  bst_procedure_shell_set_proc (proc_shell, procedure);
  g_type_class_unref (procedure);

  /* ok, now we build a list of possible preset parameters to
   * pass into the procedure and attempt applying them
   */
  bst_procedure_shell_unpreset (proc_shell);
  g_value_init (&value, BSE_TYPE_WAVE);
  g_value_set_object (&value, G_OBJECT (wave));
  bst_procedure_shell_preset (proc_shell, "wave", &value, TRUE);
  g_value_unset (&value);
  g_value_init (&value, G_TYPE_UINT);
  g_value_set_uint (&value, wave_editor->focus_channel);
  bst_procedure_shell_preset (proc_shell, "focus-channel", &value, TRUE);
  g_value_set_uint (&value, wave_editor->focus_row);
  bst_procedure_shell_preset (proc_shell, "focus-row", &value, TRUE);
  g_value_unset (&value);

  /* invoke procedure with selection already residing in the wave
   */
  bst_status_window_push (widget);
  bst_procedure_editor_exec_modal (wave_editor->proc_editor, FALSE);
  bst_status_window_pop ();

  bst_wave_editor_update (wave_editor);

  bse_object_unref (BSE_OBJECT (wave));
  gtk_widget_unref (widget);
}

static void
wave_editor_operate (BstWaveEditor *wave_editor,
			BstWaveOps     op,
			GtkWidget        *menu_item)
{
  BseWave *wave;
  GtkWidget *widget;
  guint channel, row;
  
  widget = GTK_WIDGET (wave_editor);
  wave = BSE_WAVE (BST_WAVE_EDITOR (wave_editor->wave_editor)->wave);
  pe_channel_row_from_popup (wave_editor, menu_item, &channel, &row);
  
  gtk_widget_ref (widget);
  bse_object_ref (BSE_OBJECT (wave));
  
  switch (op)
    {
    case BST_WAVE_OP_HUHU:
      g_message ("HUHU c%u r%u", channel, row);
      break;
    default:
      break;
    }
  
  bst_wave_editor_update (wave_editor);
  
  bse_object_unref (BSE_OBJECT (wave));
  gtk_widget_unref (widget);
}

void
bst_wave_editor_operate (BstWaveEditor *wave_editor,
			    BstWaveOps     op)
{
  g_return_if_fail (BST_IS_WAVE_EDITOR (wave_editor));
  g_return_if_fail (bst_wave_editor_can_operate (wave_editor, op));

  wave_editor_operate (wave_editor, op, NULL);
}

gboolean
bst_wave_editor_can_operate (BstWaveEditor *wave_editor,
				BstWaveOps	  op)
{
  BseWave *wave;
  GtkWidget *widget;
  
  g_return_val_if_fail (BST_IS_WAVE_EDITOR (wave_editor), FALSE);
  
  widget = GTK_WIDGET (wave_editor);
  wave = BSE_WAVE (BST_WAVE_EDITOR (wave_editor->wave_editor)->wave);
  
  switch (op)
    {
    case BST_WAVE_OP_HUHU:
      return TRUE;
    default:
      return FALSE;
    }
}
#endif


#define QSAMPLER_SELECTION_TIMEOUT      (33)
static gulong qsampler_selection_timeout_id = 0;

static void
qsampler_set_selection (BstQSampler *qsampler,
			gint         m1,
			gint         m2,
			gboolean     visible_mark)
{
  BstWaveEditor *wave_editor = BST_WAVE_EDITOR (g_object_get_data (G_OBJECT (qsampler), "wave_editor"));
  guint i;

  for (i = 0; i < wave_editor->n_channels; i++)
    {
      BstQSampler *qs = wave_editor->qsampler[i];
      gchar *s;

      bst_qsampler_set_mark (qs, 1, m1, BST_QSAMPLER_SELECTED);
      bst_qsampler_set_mark (qs, 2, m2, BST_QSAMPLER_SELECTED);
      bst_qsampler_set_region (qs, 1, MIN (m1, m2), 1 + MAX (m1, m2) - MIN (m1, m2), BST_QSAMPLER_SELECTED);
      if (visible_mark == 1)
	bst_qsampler_scroll_show (qs, m1);
      else if (visible_mark == 2)
	bst_qsampler_scroll_show (qs, m2);
      s = g_strdup_printf ("%d", MIN (m1, m2));
      gtk_entry_set_text (wave_editor->sstart, s);
      g_free (s);
      s = g_strdup_printf ("%d", MAX (m1, m2));
      gtk_entry_set_text (wave_editor->send, s);
      g_free (s);
    }
}

static gboolean
qsampler_selection_timeout (gpointer data)
{
  BstQSampler *qsampler = BST_QSAMPLER (data);
  gboolean retain = FALSE;

  if (GTK_WIDGET_DRAWABLE (qsampler))
    {
      gint m1 = bst_qsampler_get_mark_offset (qsampler, 1);
      gint m2 = bst_qsampler_get_mark_offset (qsampler, 2);
      gint x;

      gdk_window_get_pointer (GTK_WIDGET (qsampler)->window, &x, NULL, NULL);
      if (!bst_qsampler_get_offset_at (qsampler, &x))
	{
	  gint b;

	  if (x < 0)
	    {
	      bst_qsampler_get_bounds (qsampler, &b, NULL);
	      x = MAX (0, b + x);
	    }
	  else
	    {
	      bst_qsampler_get_bounds (qsampler, NULL, &b);
	      x = MIN (qsampler->pcm_length - 1, b + x);
	    }
	  retain = TRUE;
	}
      if (ABS (m1 - x) < ABS (m2 - x))
	qsampler_set_selection (qsampler, m2, x, 2);
      else
	qsampler_set_selection (qsampler, m1, x, 2);
    }

  if (retain && qsampler_selection_timeout_id)
    return TRUE;
  else
    {
      qsampler_selection_timeout_id = 0;
      g_object_unref (qsampler);
      return FALSE;
    }
}

static gboolean
qsampler_button_event (BstQSampler    *qsampler,
		       GdkEventButton *event)
{
  gboolean handled = FALSE;

  if (event->button == 1)
    {
      gint m1 = bst_qsampler_get_mark_offset (qsampler, 1);
      gint m2 = bst_qsampler_get_mark_offset (qsampler, 2);
      gint x = event->x;

      handled = TRUE;
      if (!bst_qsampler_get_offset_at (qsampler, &x))
	{
	  if (x < 0)
	    bst_qsampler_get_bounds (qsampler, &x, NULL);
	  else
	    bst_qsampler_get_bounds (qsampler, NULL, &x);
	}

      if (event->type == GDK_BUTTON_PRESS && (event->state & GDK_SHIFT_MASK) &&
	  m1 >= 0 && m2 >= 0)
	{
	  if (ABS (m1 - x) < ABS (m2 - x))
	    qsampler_set_selection (qsampler, m2, x, 2);
	  else
	    qsampler_set_selection (qsampler, m1, x, 2);
	}
      else if (event->type == GDK_BUTTON_PRESS)
	qsampler_set_selection (qsampler, x, x, 2);
      else if (event->type == GDK_BUTTON_RELEASE)
	{
	  if (qsampler_selection_timeout_id)
	    {
	      gtk_timeout_remove (qsampler_selection_timeout_id);
	      qsampler_selection_timeout_id = 0;
	    }
	}
    }

  return handled;
}

static gboolean
qsampler_motion_event (BstQSampler    *qsampler,
		       GdkEventMotion *event)
{
  gboolean handled = FALSE;

  if (event->type == GDK_MOTION_NOTIFY)
    {
      gint m1 = bst_qsampler_get_mark_offset (qsampler, 1);
      // gint m2 = bst_qsampler_get_mark_offset (qsampler, 2);
      gint x = event->x;

      handled = TRUE;
      if (bst_qsampler_get_offset_at (qsampler, &x))
	qsampler_set_selection (qsampler, m1, x, 2);
      else if (!qsampler_selection_timeout_id)
	qsampler_selection_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT + 1,
							    QSAMPLER_SELECTION_TIMEOUT,
							    qsampler_selection_timeout,
							    g_object_ref (qsampler), NULL);
    }

  return handled;
}

static void
qsampler_filler (gpointer     data,
		 guint        voffset,
		 guint        n_values,
		 gint16      *values,
		 BstQSampler *qsampler)
{
  BstWaveEditor *wave_editor = BST_WAVE_EDITOR (data);
  GslDataCache *dcache = wave_editor->wchunk->dcache;
  GslDataCacheNode *dnode;
  glong dnode_length;
  guint channel = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (qsampler), "channel"));
  gint i;

  voffset = voffset * wave_editor->n_channels + channel;
  n_values *= wave_editor->n_channels;
  dnode = gsl_data_cache_ref_node (dcache, voffset, TRUE);
  dnode_length = dcache->node_size;
  for (i = 0; i < n_values; i += wave_editor->n_channels)
    {
      glong offset = voffset + i;

      if (offset < 0 || offset >= dcache->dhandle->n_values)
	*values++ = 0;
      else
	{
	  if (offset < dnode->offset || offset >= dnode->offset + dnode_length)
	    {
	      gsl_data_cache_unref_node (dcache, dnode);
	      dnode = gsl_data_cache_ref_node (dcache, offset, TRUE);
	    }
	  *values++ = dnode->data[offset - dnode->offset] * 32768.0;
	}
    }
  gsl_data_cache_unref_node (dcache, dnode);
}

static void
tree_selection (BstWaveEditor    *wave_editor,
		GtkTreeSelection *tsel)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  guint i;

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

  for (i = 0; i < wave_editor->n_channels; i++)
    {
      BstQSampler *qsampler = wave_editor->qsampler[i];
      
      if (!wave_editor->wchunk)
	bst_qsampler_set_source (qsampler, 0, NULL, NULL, NULL);
      else
	bst_qsampler_set_source (qsampler, wave_editor->wchunk->dcache->dhandle->n_values / wave_editor->n_channels,
				 qsampler_filler, wave_editor, NULL);
    }
}

static void
play_back_wchunk (BstWaveEditor *wave_editor)
{
  bst_play_back_handle_stop (wave_editor->phandle);
  // bst_play_back_handle_set (wave_editor->phandle, wave_editor->wchunk, wave_editor->wchunk->osc_freq);
  bst_play_back_handle_start (wave_editor->phandle);
}

static void
tree_row_activated (BstWaveEditor     *wave_editor,
		    GtkTreePath       *path,
		    GtkTreeViewColumn *column,
		    GtkTreeView	      *tree_view)
{
  // GtkTreeSelection *tsel = gtk_tree_view_get_selection (tree_view);

  play_back_wchunk (wave_editor);
}

static void
change_draw_mode (BstWaveEditor *wave_editor,
		  GtkOptionMenu *omenu)
{
  guint i;
  guint mode = bst_choice_get_last (omenu->menu);

  for (i = 0; i < wave_editor->n_channels; i++)
    {
      BstQSampler *qsampler = wave_editor->qsampler[i];

      bst_qsampler_set_draw_mode (qsampler, mode);
    }
}

static void
adjustments_changed (BstWaveEditor *wave_editor,
		     GtkAdjustment *adjustment)
{
  guint i;
  
  for (i = 0; i < wave_editor->n_channels; i++)
    {
      BstQSampler *qsampler = wave_editor->qsampler[i];
      
      if (adjustment == wave_editor->zoom_adjustment)
	bst_qsampler_set_zoom (qsampler, adjustment->value);
      else if (adjustment == wave_editor->vscale_adjustment)
	bst_qsampler_set_vscale (qsampler, adjustment->value);
    }
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
  wave_editor->qsampler = g_renew (BstQSampler*, wave_editor->qsampler, wave_editor->n_channels);

  /* setup qsampler widgets
   */
  sbar = gtk_widget_new (GTK_TYPE_HSCROLLBAR,
			 "visible", TRUE,
			 NULL);
  for (i = 0; i < wave_editor->n_channels; i++)
    {
      GtkWidget *qbox = gtk_widget_new (GTK_TYPE_VBOX,
					"visible", TRUE,
					"border_width", 0,
					"parent", qsampler_parent,
					NULL);
      BstQSampler *qsampler = g_object_new (BST_TYPE_QSAMPLER,
					    "visible", TRUE,
					    "events", (GDK_BUTTON_PRESS_MASK |
						       GDK_BUTTON_RELEASE_MASK |
						       GDK_BUTTON1_MOTION_MASK),
					    "parent", qbox,
					    "height_request", 80,
					    NULL);
      g_object_connect (qsampler,
			"signal::button_press_event", qsampler_button_event, NULL,
			"signal::button_release_event", qsampler_button_event, NULL,
			"signal::motion_notify_event", qsampler_motion_event, NULL,
			NULL);
      bst_qsampler_set_adjustment (qsampler, gtk_range_get_adjustment (GTK_RANGE (sbar)));
      g_object_set_data (G_OBJECT (qsampler), "channel", GUINT_TO_POINTER (i));
      g_object_set_data (G_OBJECT (qsampler), "wave_editor", wave_editor);
      wave_editor->qsampler[i] = qsampler;
    }
  gtk_box_pack_start (GTK_BOX (qsampler_parent), sbar, FALSE, TRUE, 0);

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
  wave_editor->zoom_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (100, 1e-16, 1e+16, 0.1, 10, 0));
  g_object_connect (wave_editor->zoom_adjustment,
		    "swapped_signal_after::value_changed", adjustments_changed, wave_editor,
		    "swapped_signal::destroy", g_nullify_pointer, &wave_editor->zoom_adjustment,
		    NULL);
  entry = g_object_new (GTK_TYPE_SPIN_BUTTON,
			"adjustment", wave_editor->zoom_adjustment,
			"digits", 5,
			"visible", TRUE,
			NULL);
  gmask = bst_gmask_quick (mask_parent, 0, "Zoom:", entry, NULL);
  wave_editor->vscale_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (100, 1e-16, 1e+16, 1, 10, 0));
  g_object_connect (wave_editor->vscale_adjustment,
		    "swapped_signal_after::value_changed", adjustments_changed, wave_editor,
		    "swapped_signal::destroy", g_nullify_pointer, &wave_editor->vscale_adjustment,
		    NULL);
  entry = g_object_new (GTK_TYPE_SPIN_BUTTON,
			"adjustment", wave_editor->vscale_adjustment,
			"digits", 5,
			"visible", TRUE,
			NULL);
  gmask = bst_gmask_quick (mask_parent, 1, "VScale:", entry, NULL);

  /* setup qsampler selection start and end
   */
  wave_editor->sstart = g_object_new (GTK_TYPE_ENTRY,
				      "visible", TRUE,
				      NULL);
  g_object_connect (wave_editor->sstart,
		    "swapped_signal::destroy", g_nullify_pointer, &wave_editor->sstart,
		    NULL);
  gmask = bst_gmask_quick (mask_parent, 0, "Start:", wave_editor->sstart, NULL);
  wave_editor->send = g_object_new (GTK_TYPE_ENTRY,
				    "visible", TRUE,
				    NULL);
  g_object_connect (wave_editor->send,
		    "swapped_signal::destroy", g_nullify_pointer, &wave_editor->sstart,
		    NULL);
  gmask = bst_gmask_quick (mask_parent, 1, "End:", wave_editor->send, NULL);

  /* setup wave display type
   */
  any = g_object_new (GTK_TYPE_OPTION_MENU,
		      "visible", TRUE,
		      NULL);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (any),
			    bst_choice_menu_createv ("<BEAST-WaveEditor>/Popup",
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
		    "swapped_signal::changed", change_draw_mode, wave_editor,
		    NULL);
  gtk_option_menu_set_history (GTK_OPTION_MENU (any), 0);
  gmask = bst_gmask_quick (mask_parent, 2, NULL, any, NULL);

  /* setup preview button
   */
  any = g_object_new (GTK_TYPE_BUTTON,
		      "visible", TRUE,
		      "label", "Preview",
		      NULL);
  g_object_connect (any,
		    "swapped_signal::clicked", play_back_wchunk, wave_editor,
		    NULL);
  gmask = bst_gmask_quick (mask_parent, 2, NULL, any, NULL);
}
