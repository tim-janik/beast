/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2003 Tim Janik
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
#include "bsttrackview.h"
#include "bstactivatable.h"
#include "bstparam.h"
#include "bsttracksynthdialog.h"

#include <stdlib.h> /* strtol */
#include <string.h>

#define	SCROLLBAR_SPACING (3) /* from gtkscrolledwindow.c:DEFAULT_SCROLLBAR_SPACING */

/* --- prototypes --- */
static void	bst_track_view_class_init	(BstTrackViewClass	*klass);
static void	bst_track_view_init		(BstTrackView		*self);
static void	bst_track_view_finalize		(GObject		*object);
static void     bst_track_view_activate         (BstActivatable         *activatable,
                                                 gulong                  action);
static gboolean bst_track_view_can_activate     (BstActivatable         *activatable,
                                                 gulong                  action);
static void	bst_track_view_create_tree	(BstItemView		*iview);
static void     track_view_set_container        (BstItemView            *self,
						 SfiProxy                new_container);
static void	track_view_listen_on		(BstItemView		*iview,
						 SfiProxy		 item);
static void	track_view_unlisten_on		(BstItemView		*iview,
						 SfiProxy		 item);


/* --- columns --- */
enum {
  COL_SEQID,
  COL_NAME,
  COL_MUTE,
  COL_VOICES,
  COL_SYNTH,
  COL_MIDI_CHANNEL,
  COL_POST_SYNTH,
  COL_BLURB,
  N_COLS
};


/* --- track ops --- */
static BstItemViewOp track_view_ops[] = {
  { "Add",		BST_ACTION_ADD_TRACK,   	BST_STOCK_TRACK,
    "Add a new track to this song" },
  { "Delete",		BST_ACTION_DELETE_TRACK,	BST_STOCK_TRASHCAN,
    "Delete the currently selected track" },
};
static guint n_track_view_ops = sizeof (track_view_ops) / sizeof (track_view_ops[0]);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
GType
bst_track_view_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (BstTrackViewClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bst_track_view_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (BstTrackView),
        0,      /* n_preallocs */
        (GInstanceInitFunc) bst_track_view_init,
      };
      type = g_type_register_static (BST_TYPE_ITEM_VIEW, "BstTrackView", &type_info, 0);
      bst_type_implement_activatable (type,
                                      bst_track_view_activate,
                                      bst_track_view_can_activate,
                                      NULL);
    }
  return type;
}

static void
bst_track_view_class_init (BstTrackViewClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = bst_track_view_finalize;

  item_view_class->n_ops = n_track_view_ops;
  item_view_class->ops = track_view_ops;
  item_view_class->horizontal_ops = TRUE;
  item_view_class->show_properties = FALSE;
  item_view_class->create_tree = bst_track_view_create_tree;
  item_view_class->set_container = track_view_set_container;
  item_view_class->listen_on = track_view_listen_on;
  item_view_class->unlisten_on = track_view_unlisten_on;

  item_view_class->item_type = "BseTrack";
}

static void
bst_track_view_init (BstTrackView *self)
{
}

static void
bst_track_view_finalize (GObject *object)
{
  BstTrackView *self = BST_TRACK_VIEW (object);

  if (self->troll_ctrl)
    bst_track_roll_controller_unref (self->troll_ctrl);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget*
bst_track_view_new (SfiProxy song)
{
  GtkWidget *track_view;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  
  track_view = gtk_widget_new (BST_TYPE_TRACK_VIEW, NULL);
  bst_item_view_set_container (BST_ITEM_VIEW (track_view), song);
  
  return track_view;
}

static void
track_view_hzoom_changed (BstTrackView  *self,
			  GtkAdjustment *adjustment)
{
  if (self->troll)
    bst_track_roll_set_hzoom (self->troll, adjustment->value);
}

static void
track_view_fill_value (BstItemView *iview,
		       guint        column,
		       guint        row,
		       GValue      *value)
{
  BstTrackView *self = BST_TRACK_VIEW (iview);
  guint seqid = row + 1;
  switch (column)
    {
      const gchar *string;
      gboolean vbool;
      SfiInt vint;
      SfiProxy item, snet, wave;
    case COL_SEQID:
      sfi_value_take_string (value, g_strdup_printf ("%03d", seqid));
      break;
    case COL_NAME:
      item = bse_container_get_item (iview->container, BST_ITEM_VIEW_GET_CLASS (self)->item_type, seqid);
      g_value_set_string (value, bse_item_get_name (item));
      break;
    case COL_MUTE:
      item = bse_container_get_item (iview->container, BST_ITEM_VIEW_GET_CLASS (self)->item_type, seqid);
      bse_proxy_get (item, "muted", &vbool, NULL);
      g_value_set_boolean (value, !vbool);
      break;
    case COL_VOICES:
      item = bse_container_get_item (iview->container, BST_ITEM_VIEW_GET_CLASS (self)->item_type, seqid);
      bse_proxy_get (item, "n_voices", &vint, NULL);
      sfi_value_take_string (value, g_strdup_printf ("%2d", vint));
      break;
    case COL_SYNTH:
      item = bse_container_get_item (iview->container, BST_ITEM_VIEW_GET_CLASS (self)->item_type, seqid);
      snet = 0;
      bse_proxy_get (item, "snet", &snet, "wave", &wave, NULL);
      g_value_set_string (value, snet || wave ? bse_item_get_name (snet ? snet : wave) : "");
      break;
    case COL_MIDI_CHANNEL:
      item = bse_container_get_item (iview->container, BST_ITEM_VIEW_GET_CLASS (self)->item_type, seqid);
      bse_proxy_get (item, "midi-channel", &vint, NULL);
      sfi_value_take_string (value, g_strdup_printf ("%2d", vint));
      break;
    case COL_POST_SYNTH:
      item = bse_container_get_item (iview->container, BST_ITEM_VIEW_GET_CLASS (self)->item_type, seqid);
      snet = 0;
      bse_proxy_get (item, "pnet", &snet, NULL);
      g_value_set_string (value, snet ? bse_item_get_name (snet) : "");
      break;
    case COL_BLURB:
      item = bse_container_get_item (iview->container, BST_ITEM_VIEW_GET_CLASS (self)->item_type, seqid);
      bse_proxy_get (item, "blurb", &string, NULL);
      g_value_set_string (value, string ? string : "");
      break;
    }
}

static void
track_view_synth_edited (BstTrackView *self,
			 const gchar  *strpath,
			 const gchar  *text)
{
  g_return_if_fail (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy item = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      if (text)
	{
	  SfiProxy proxy = 0;
	  GSList *slist = NULL;
	  /* list possible snet/wave candidates */
	  slist = g_slist_append (slist, bse_item_list_proxies (item, "snet"));
	  slist = g_slist_append (slist, bse_item_list_proxies (item, "wave"));
	  /* find best match */
	  proxy = bst_proxy_seq_list_match (slist, text);
	  g_slist_free (slist);
	  if (proxy && BSE_IS_SNET (proxy))
	    bse_proxy_set (item, "snet", proxy, NULL);
	  else if (proxy && BSE_IS_WAVE (proxy))
	    bse_proxy_set (item, "wave", proxy, NULL);
	  else
	    bse_proxy_set (item, "snet", 0, "wave", 0, NULL);
	}
      else
	bse_proxy_set (item, "snet", 0, "wave", 0, NULL);
    }
}

static void
track_view_post_synth_edited (BstTrackView *self,
                              const gchar  *strpath,
                              const gchar  *text)
{
  g_return_if_fail (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy item = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      if (text)
	{
	  SfiProxy proxy = 0;
	  GSList *slist = NULL;
	  /* list possible snet candidates */
	  slist = g_slist_append (slist, bse_item_list_proxies (item, "pnet"));
	  /* find best match */
	  proxy = bst_proxy_seq_list_match (slist, text);
	  g_slist_free (slist);
	  if (proxy && BSE_IS_SNET (proxy))
	    bse_proxy_set (item, "pnet", proxy, NULL);
	  else
	    bse_proxy_set (item, "pnet", 0, NULL);
	}
      else
	bse_proxy_set (item, "pnet", 0, NULL);
    }
}

static void
track_view_synth_popup_cb (GxkCellRendererPopup *pcell,
                           SfiProxy              proxy,
                           BstTrackSynthDialog  *tsdialog)
{
  gxk_cell_renderer_enter (pcell,
                           proxy ? bse_item_get_uname_path (proxy) : "",
                           FALSE,
                           proxy == 0);
}

static void
track_view_synth_popup (BstTrackView         *self,
			const gchar          *strpath,
			const gchar          *text,
			GxkCellRendererPopup *pcell)
{
  g_return_if_fail (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy item = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      BseProxySeq *pseq = bse_item_list_proxies (item, "snet");
      GtkWidget *dialog = bst_track_synth_dialog_popup (self, item,
                                                        pseq,
                                                        bse_project_get_wave_repo (bse_item_get_project (item)),
                                                        track_view_synth_popup_cb, pcell);
      gxk_cell_renderer_popup_dialog (pcell, dialog);
    }
}

static void
track_view_post_synth_popup (BstTrackView         *self,
                             const gchar          *strpath,
                             const gchar          *text,
                             GxkCellRendererPopup *pcell)
{
  g_return_if_fail (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy item = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      BseProxySeq *pseq = bse_item_list_proxies (item, "pnet");
      GtkWidget *dialog = bst_track_synth_dialog_popup (self, item, pseq, 0,
                                                        track_view_synth_popup_cb, pcell);
      gxk_cell_renderer_popup_dialog (pcell, dialog);
    }
}

static void
track_view_mute_toggled (BstTrackView          *self,
			 const gchar           *strpath,
			 GtkCellRendererToggle *tcell)
{
  g_return_if_fail (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy item = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      if (item)
	{
	  gboolean muted;
	  bse_proxy_get (item, "muted", &muted, NULL);
	  bse_proxy_set (item, "muted", !muted, NULL);
	  bse_proxy_get (item, "muted", &muted, NULL);
	  gtk_cell_renderer_toggle_set_active (tcell, !muted);
	}
    }
}

static void
track_view_voice_edited (BstTrackView *self,
                         const gchar  *strpath,
                         const gchar  *text)
{
  g_return_if_fail (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy item = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      if (item)
	{
	  int i = strtol (text, NULL, 10);
	  if (i > 0)
	    bse_proxy_set (item, "n_voices", i, NULL);
	}
    }
}

static void
track_view_midi_channel_edited (BstTrackView *self,
                                const gchar  *strpath,
                                const gchar  *text)
{
  g_return_if_fail (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy item = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      if (item)
	{
	  int i = strtol (text, NULL, 10);
	  if (i >= 0)
	    bse_proxy_set (item, "midi-channel", i, NULL);
	}
    }
}

static SfiProxy
get_track (gpointer data,
	   gint     row)
{
  return bst_item_view_get_proxy (BST_ITEM_VIEW (data), row);
}

static void
update_selection (BstTrackView     *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  if (self->troll && iview->tree)
    {
      bst_track_roll_set_prelight_row (self->troll,
				       gxk_tree_view_get_selected_row (iview->tree));
      if (GTK_WIDGET_DRAWABLE (self->troll))
	gxk_window_process_next (GTK_WIDGET (self->troll)->window, TRUE);
    }
}

static void
track_view_marks_changed (BstTrackView *self)
{
  SfiProxy song = BST_ITEM_VIEW (self)->container;
  if (self->troll && song)
    {
      SfiInt lleft, lright, pointer;
      bse_proxy_get (song, "loop_left", &lleft, "loop_right", &lright, "tick_pointer", &pointer, NULL);
      bst_track_roll_set_mark (self->troll, 1, lleft, lleft >= 0 ? BST_MARKER_GREEN : 0);
      bst_track_roll_set_mark (self->troll, 2, lright, lright >= 0 ? BST_MARKER_BLUE : 0);
      bst_track_roll_set_mark (self->troll, 3, pointer, pointer >= 0 ? BST_MARKER_RED : 0);
    }
}

static void
track_view_repeat_toggled (BstTrackView *self)
{
  SfiProxy song = BST_ITEM_VIEW (self)->container;
  if (song && self->repeat_toggle)
    bse_proxy_set (song, "loop_enabled", GTK_TOGGLE_BUTTON (self->repeat_toggle)->active, NULL);
}

static void
track_view_repeat_changed (BstTrackView *self)
{
  SfiProxy song = BST_ITEM_VIEW (self)->container;
  if (song && self->repeat_toggle)
    {
      GtkToggleButton *toggle = GTK_TOGGLE_BUTTON (self->repeat_toggle);
      gboolean enabled;
      bse_proxy_get (song, "loop_enabled", &enabled, NULL);
      if (toggle->active != enabled)
	gtk_toggle_button_set_active (toggle, enabled);
    }
}

static void
bst_track_view_create_tree (BstItemView *iview)
{
  BstTrackView *self = BST_TRACK_VIEW (iview);
  GtkWidget *hscroll1, *hscroll2, *vscroll, *trframe, *tlframe, *vb1, *vb2, *vb3, *hb, *align1, *align2, *entry;
  GtkObject *adjustment;
  GtkPaned *paned;
  GtkTreeSelection *tsel;
  GtkTreeModel *smodel;
  GxkListWrapper *lwrapper;

  /* item list model */
  lwrapper = gxk_list_wrapper_new (N_COLS,
				   G_TYPE_STRING,	/* COL_SEQID */
				   G_TYPE_STRING,	/* COL_NAME */
				   G_TYPE_BOOLEAN,	/* COL_MUTE */
				   G_TYPE_STRING,	/* COL_VOICES */
				   G_TYPE_STRING,	/* COL_SYNTH */
				   G_TYPE_STRING,	/* COL_MIDI_CHANNEL */
				   G_TYPE_STRING,	/* COL_POST_SYNTH */
				   G_TYPE_STRING	/* COL_BLURB */
				   );
  smodel = bst_item_view_adapt_list_wrapper (iview, lwrapper);
  g_signal_connect_object (lwrapper, "fill-value",
			   G_CALLBACK (track_view_fill_value),
			   iview, G_CONNECT_SWAPPED);
  g_object_unref (lwrapper);
  
  /* scrollbars */
  hscroll1 = gtk_hscrollbar_new (NULL);
  hscroll2 = gtk_hscrollbar_new (NULL);
  vscroll = gtk_vscrollbar_new (NULL);
  align1 = g_object_new (GTK_TYPE_ALIGNMENT, NULL);
  align2 = g_object_new (GTK_TYPE_ALIGNMENT, NULL);

  /* tree view (track list) */
  tlframe = g_object_new (GTK_TYPE_FRAME, "shadow_type", GTK_SHADOW_IN, NULL);
  iview->tree = g_object_new (GTK_TYPE_TREE_VIEW,
			      "can_focus", TRUE,
			      "model", smodel,
			      "parent", tlframe,
			      "rules_hint", TRUE,
			      "height_request", BST_ITEM_VIEW_TREE_HEIGHT,
			      "width_request", 200,
			      NULL);
  gxk_nullify_on_destroy (iview->tree, &iview->tree);
  gtk_tree_view_set_hadjustment (iview->tree, gtk_range_get_adjustment (GTK_RANGE (hscroll1)));
  gtk_tree_view_set_vadjustment (iview->tree, gtk_range_get_adjustment (GTK_RANGE (vscroll)));
  tsel = gtk_tree_view_get_selection (iview->tree);
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_BROWSE);
  gxk_tree_selection_force_browse (tsel, smodel);
  g_object_unref (smodel);

  /* track roll */
  trframe = g_object_new (GTK_TYPE_FRAME, "shadow_type", GTK_SHADOW_IN, NULL);
  self->troll = g_object_new (BST_TYPE_TRACK_ROLL,
			      "parent", trframe,
			      NULL);
  gxk_nullify_on_destroy (self->troll, &self->troll);
  bst_track_roll_set_hadjustment (self->troll, gtk_range_get_adjustment (GTK_RANGE (hscroll2)));
  bst_track_roll_set_vadjustment (self->troll, gtk_range_get_adjustment (GTK_RANGE (vscroll)));
  bst_track_roll_set_track_callback (self->troll, self, get_track);
  track_view_marks_changed (self);
  
  /* link track roll to tree view and list model */
  g_signal_connect_object (tsel, "changed",
			   G_CALLBACK (update_selection),
			   self, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
  bst_track_roll_set_size_callbacks (self->troll,
				     iview->tree,
				     gxk_tree_view_get_bin_window_pos,
				     gxk_tree_view_get_row_area,
				     gxk_tree_view_get_row_from_coord);
  g_signal_connect_object (iview->tree, "size_allocate",
			   G_CALLBACK (bst_track_roll_reallocate),
			   self->troll, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
  g_signal_connect_object (iview->tree, "size_allocate",
			   G_CALLBACK (update_selection),
			   self, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
  g_signal_connect_object (self->troll, "select-row",
			   G_CALLBACK (gxk_tree_view_focus_row),
			   iview->tree, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->troll, "select-row",
			   G_CALLBACK (gtk_widget_grab_focus),
			   iview->tree, G_CONNECT_SWAPPED);
  g_signal_connect_object (iview->wlist, "row-change",
			   G_CALLBACK (bst_track_roll_abort_edit),
			   self->troll, G_CONNECT_SWAPPED);

  /* track roll controller */
  self->troll_ctrl = bst_track_roll_controller_new (self->troll);
  bst_track_roll_controller_set_song (self->troll_ctrl, iview->container);

  /* create toolbar */
  self->toolbar = gxk_toolbar_new (&self->toolbar);
  /* add radio tools to toolbar */
  bst_radio_tools_build_toolbar (self->troll_ctrl->canvas_rtools, self->toolbar);
  gxk_toolbar_append_separator (self->toolbar);
  bst_radio_tools_build_toolbar_choice (self->troll_ctrl->hpanel_rtools, self->toolbar);
  /* add repeat toggle */
  self->repeat_toggle = gxk_toolbar_append_stock (self->toolbar, GXK_TOOLBAR_TOGGLE,
						  "Repeat", "Repeat playback within loop points", BST_STOCK_REPEAT);
  gxk_nullify_on_destroy (self->repeat_toggle, &self->repeat_toggle);
  g_object_connect (self->repeat_toggle, "swapped_signal::toggled", track_view_repeat_toggled, self, NULL);
  track_view_repeat_changed (self);
  /* add quantization tools to toolbar */
  gxk_toolbar_append_separator (self->toolbar);
  bst_radio_tools_build_toolbar_choice (self->troll_ctrl->quant_rtools, self->toolbar);
  /* add zoom spinner */
  gxk_toolbar_append_separator (self->toolbar);
  adjustment = gtk_adjustment_new (50, 1, 100, 1, 5, 0);
  g_object_connect (adjustment,
		    "swapped_signal_after::value_changed", track_view_hzoom_changed, self,
		    NULL);
  entry = g_object_new (GTK_TYPE_SPIN_BUTTON,
			"visible", TRUE,
			"adjustment", adjustment,
			"digits", 0,
			"width_request", 2 * gxk_size_width (BST_SIZE_TOOLBAR),
			NULL);
  gxk_toolbar_append (self->toolbar, GXK_TOOLBAR_EXTRA_WIDGET,
		      "Zoom", "Horizontal Zoom", entry);

  /* setup accelerators */
  gxk_widget_activate_accel_group (GTK_WIDGET (self), self->troll_ctrl->canvas_rtools->accel_group);
  gxk_widget_activate_accel_group (GTK_WIDGET (self), self->troll_ctrl->hpanel_rtools->accel_group);
  gxk_widget_activate_accel_group (GTK_WIDGET (self), self->troll_ctrl->quant_rtools->accel_group);

  /* tree view box */
  vb1 = g_object_new (GTK_TYPE_VBOX, "spacing", SCROLLBAR_SPACING, NULL);
  gtk_box_pack_start (GTK_BOX (vb1), iview->tools, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vb1), tlframe, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vb1), hscroll1, FALSE, TRUE, 0);
  
  /* track roll box */
  vb2 = g_object_new (GTK_TYPE_VBOX, "spacing", SCROLLBAR_SPACING, NULL);
  gtk_box_pack_start (GTK_BOX (vb2), GTK_WIDGET (self->toolbar), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vb2), trframe, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vb2), hscroll2, FALSE, TRUE, 0);

  /* vscrollbar box */
  vb3 = g_object_new (GTK_TYPE_VBOX, "spacing", SCROLLBAR_SPACING, NULL);
  gtk_box_pack_start (GTK_BOX (vb3), align1, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vb3), vscroll, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vb3), align2, FALSE, TRUE, 0);

  /* pack boxes and group sizes */
  gxk_size_group (GTK_SIZE_GROUP_VERTICAL,
		  iview->tools, self->toolbar, align1,
		  NULL);
  gxk_size_group (GTK_SIZE_GROUP_VERTICAL,
		  tlframe, trframe, vscroll,
		  NULL);
  gxk_size_group (GTK_SIZE_GROUP_VERTICAL,
		  hscroll1, hscroll2, align2,
		  NULL);
  paned = g_object_new (GTK_TYPE_HPANED,
			"height_request", 120,
			"border_width", 0,
			"position", 240,
			NULL);
  gtk_paned_pack1 (paned, vb1, FALSE, TRUE);
  gtk_paned_pack2 (paned, vb2, TRUE, TRUE);
  hb = g_object_new (GTK_TYPE_HBOX,
		     "child", paned,
		     "spacing", SCROLLBAR_SPACING,
		     NULL);
  gtk_box_pack_start (GTK_BOX (hb), vb3, FALSE, TRUE, 0);

  /* add list view columns */
  if (BST_DVL_HINTS)
    gxk_tree_view_add_text_column (iview->tree, COL_SEQID, "S",
                                   0.0, "ID", NULL,
                                   NULL, NULL, 0);
  gxk_tree_view_add_text_column (iview->tree, COL_NAME, "S",
				 0.0, "Name", NULL,
				 bst_item_view_name_edited, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_toggle_column (iview->tree, COL_MUTE, "",
				   0.5, "M", "Notes from unchecked tracks are muted during playback",
				   track_view_mute_toggled, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_text_column (iview->tree, COL_VOICES, "",
				 0.5, "V", "Maximum number of voices for simultaneous playback",
				 track_view_voice_edited, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_popup_column (iview->tree, COL_SYNTH, "#",
				  0.5, "Synth", "Synthesizer network or wave to be used as voice by this track",
				  track_view_synth_edited, track_view_synth_popup, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_text_column (iview->tree, COL_MIDI_CHANNEL, "",
                                 0.5, "Ch", "Midi channel assigned to this track, 0 uses private per-track channel",
                                 track_view_midi_channel_edited, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_popup_column (iview->tree, COL_POST_SYNTH, "#",
				  0.5, "Post", "Postprocessing Synthesizer network for this track",
				  track_view_post_synth_edited, track_view_post_synth_popup, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_text_column (iview->tree, COL_BLURB, "",
				 0.0, "Comment", NULL,
				 bst_item_view_blurb_edited, self, G_CONNECT_SWAPPED);

  /* make widgets visible */
  gtk_widget_show_all (GTK_WIDGET (hb));
}

static void
track_changed (SfiProxy      track,
	       BstTrackView *self)
{
  if (self->troll)
    {
      gint row = bst_item_view_get_proxy_row (BST_ITEM_VIEW (self), track);
      bst_track_roll_queue_draw_row (self->troll, row);
    }
}

static void
track_view_pointer_changed (BstTrackView *self,
			    SfiInt        position)
{
  if (self->troll)
    bst_track_roll_set_mark (self->troll, 3, position, position >= 0 ? BST_MARKER_RED : 0);
}

static void
track_view_set_container (BstItemView *iview,
			  SfiProxy     new_container)
{
  BstTrackView *self = BST_TRACK_VIEW (iview);
  if (BSE_IS_SONG (iview->container))
    bse_proxy_disconnect (iview->container,
			  "any_signal", track_view_pointer_changed, self,
			  "any_signal", track_view_marks_changed, self,
			  "any_signal", track_view_repeat_changed, self,
			  NULL);
  BST_ITEM_VIEW_CLASS (parent_class)->set_container (iview, new_container);
  if (BSE_IS_SONG (iview->container))
    {
      bst_track_roll_controller_set_song (self->troll_ctrl, iview->container);
      bse_proxy_connect (iview->container,
			 "swapped_signal::pointer-changed", track_view_pointer_changed, self,
			 "swapped_signal::property-notify::loop-left", track_view_marks_changed, self,
			 "swapped_signal::property-notify::loop-right", track_view_marks_changed, self,
			 "swapped_signal::property-notify::tick-pointer", track_view_marks_changed, self,
			 "swapped_signal::property-notify::loop-enabled", track_view_repeat_changed, self,
			 NULL);
      track_view_marks_changed (self);
    }
}

static void
track_property_changed (SfiProxy     item,
                        const gchar *property_name,
                        BstItemView *iview)
{
  bst_item_view_refresh (iview, item);
}

static void
track_view_listen_on (BstItemView *iview,
		      SfiProxy     item)
{
  BST_ITEM_VIEW_CLASS (parent_class)->listen_on (iview, item);
  bse_proxy_connect (item,
		     "signal::changed", track_changed, iview,
		     NULL);
  bse_proxy_connect (item,
                     /* COL_SEQID handled by GxkListWrapper */
                     /* COL_NAME handled by GxkListWrapper */
                     "signal::property-notify::muted", track_property_changed, iview, /* COL_MUTE */
                     "signal::property-notify::n-voices", track_property_changed, iview, /* COL_VOICES */
                     "signal::property-notify::snet", track_property_changed, iview, /* COL_SYNTH */
                     "signal::property-notify::midi-channel", track_property_changed, iview, /* COL_MIDI_CHANNEL */
                     "signal::property-notify::pnet", track_property_changed, iview, /* COL_POST_SYNTH */
                     /* COL_BLURB handled by GxkListWrapper */
                     NULL);
}

static void
track_view_unlisten_on (BstItemView *iview,
			SfiProxy     item)
{
  bse_proxy_disconnect (item,
			"any_signal", track_changed, iview,
			"any_signal", track_property_changed, iview,
			NULL);
  BST_ITEM_VIEW_CLASS (parent_class)->unlisten_on (iview, item);
}

static void
bst_track_view_activate (BstActivatable         *activatable,
                         gulong                  action)
{
  BstTrackView *self = BST_TRACK_VIEW (activatable);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  SfiProxy song = item_view->container;

  switch (action)
    {
      SfiProxy item;
    case BST_ACTION_ADD_TRACK:
      bse_item_group_undo (song, "Add Track");
      item = bse_song_create_track (song);
      if (item)
	{
	  gchar *string = g_strdup_printf ("Track-%02X", bse_item_get_seqid (item));
	  bse_item_set_name (item, string);
	  g_free (string);
	  bst_item_view_select (item_view, item);
	}
      bse_item_ungroup_undo (song);
      break;
    case BST_ACTION_DELETE_TRACK:
      item = bst_item_view_get_current (item_view);
      bse_song_remove_track (song, item);
      break;
    }
  bst_widget_update_activatable (activatable);
}

static gboolean
bst_track_view_can_activate (BstActivatable *activatable,
                             gulong          action)
{
  BstTrackView *self = BST_TRACK_VIEW (activatable);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  
  switch (action)
    {
      SfiProxy item;
    case BST_ACTION_ADD_TRACK:
      return TRUE;
    case BST_ACTION_DELETE_TRACK:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    default:
      return FALSE;
    }
}
