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
#include "bstparam.h"
#include "bstgrowbar.h"
#include "bsttracksynthdialog.h"
#include <stdlib.h> /* strtol */
#include <string.h>

#define	SCROLLBAR_SPACING (3) /* from gtkscrolledwindow.c:DEFAULT_SCROLLBAR_SPACING */

/* --- prototypes --- */
static void	bst_track_view_finalize		(GObject		*object);
static gboolean track_view_action_check         (gpointer                data,
                                                 gulong                  action);
static void     track_view_action_exec          (gpointer                data,
                                                 gulong                  action);
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


/* --- track actions --- */
enum {
  ACTION_ADD_TRACK,
  ACTION_DELETE_TRACK
};
static const GxkStockAction track_view_actions[] = {
  { N_("Add"),            NULL,         N_("Add a new track to this song"),
    ACTION_ADD_TRACK,     BST_STOCK_TRACK,
  },
  { N_("Delete"),         NULL,         N_("Delete the currently selected track"),
    ACTION_DELETE_TRACK,  BST_STOCK_TRASHCAN,
  },
};


/* --- functions --- */
G_DEFINE_TYPE (BstTrackView, bst_track_view, BST_TYPE_ITEM_VIEW);

static void
bst_track_view_class_init (BstTrackViewClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (class);

  gobject_class->finalize = bst_track_view_finalize;

  item_view_class->set_container = track_view_set_container;
  item_view_class->listen_on = track_view_listen_on;
  item_view_class->unlisten_on = track_view_unlisten_on;

  item_view_class->item_type = "BseTrack";
}

static void
bst_track_view_finalize (GObject *object)
{
  BstTrackView *self = BST_TRACK_VIEW (object);

  if (self->tctrl)
    bst_track_roll_controller_unref (self->tctrl);

  G_OBJECT_CLASS (bst_track_view_parent_class)->finalize (object);
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
          BsePropertyCandidates *pc = bse_item_get_property_candidates (item, "snet");
	  slist = g_slist_append (slist, pc->items);
          pc = bse_item_get_property_candidates (item, "wave");
	  slist = g_slist_append (slist, pc->items);
	  /* find best match */
	  proxy = bst_item_seq_list_match (slist, text);
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
          BsePropertyCandidates *pc = bse_item_get_property_candidates (item, "pnet");
	  slist = g_slist_append (slist, pc->items);
	  /* find best match */
	  proxy = bst_item_seq_list_match (slist, text);
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
      BsePropertyCandidates *pc = bse_item_get_property_candidates (item, "snet");
      GtkWidget *dialog = bst_track_synth_dialog_popup (self, item,
                                                        pc->items,
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
      BsePropertyCandidates *pc = bse_item_get_property_candidates (item, "pnet");
      GtkWidget *dialog = bst_track_synth_dialog_popup (self, item, pc->items, 0,
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
track_view_marks_changed (BstTrackView *self)
{
  SfiProxy song = BST_ITEM_VIEW (self)->container;
  if (self->troll && song)
    {
      SfiInt lleft, lright, pointer;
      bse_proxy_get (song, "loop_left", &lleft, "loop_right", &lright, "tick_pointer", &pointer, NULL);
      bst_track_roll_set_marker (self->troll, 1, lleft, lleft >= 0 ? BST_TRACK_ROLL_MARKER_LOOP : 0);
      bst_track_roll_set_marker (self->troll, 2, lright, lright >= 0 ? BST_TRACK_ROLL_MARKER_LOOP : 0);
      bst_track_roll_set_marker (self->troll, 3, pointer, pointer >= 0 ? BST_TRACK_ROLL_MARKER_POS : 0);
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
bst_track_view_init (BstTrackView *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
  GtkWidget *treehs, *trackgb, *vscroll;
  GtkObject *adjustment;
  GtkTreeView *tview;
  GtkTreeSelection *tsel;
  GtkTreeModel *smodel;
  GxkListWrapper *lwrapper;
  GxkRadget *radget;

  /* create GUI */
  gxk_widget_publish_actions (self, "track-view-actions",
                              G_N_ELEMENTS (track_view_actions), track_view_actions,
                              NULL, track_view_action_check, track_view_action_exec);
  radget = gxk_radget_complete (GTK_WIDGET (self), "beast", "track-view", NULL);

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
  treehs = gxk_radget_find (radget, "tree-hscrollbar");
  trackgb = gxk_radget_find (radget, "track-hgrow-bar");
  vscroll = gxk_radget_find (radget, "tree-vscrollbar");
  
  /* tree view (track list) */
  tview = gxk_radget_find (radget, "tree-view");
  gtk_tree_view_set_model (tview, smodel);
  bst_item_view_set_tree (iview, tview);
  gtk_tree_view_set_hadjustment (iview->tree, gtk_range_get_adjustment (GTK_RANGE (treehs)));
  gtk_tree_view_set_vadjustment (iview->tree, gtk_range_get_adjustment (GTK_RANGE (vscroll)));
  tsel = gtk_tree_view_get_selection (iview->tree);
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_BROWSE);
  gxk_tree_selection_force_browse (tsel, smodel);
  g_object_unref (smodel);

  /* track roll */
  self->troll = g_object_new (BST_TYPE_TRACK_ROLL,
                              "visible", TRUE,
			      "parent", gxk_radget_find (radget, "track-area"),
			      NULL);
  gxk_nullify_in_object (self, &self->troll);
  gxk_scroll_canvas_set_hadjustment (GXK_SCROLL_CANVAS (self->troll), bst_grow_bar_get_adjustment (BST_GROW_BAR (trackgb)));
  gxk_scroll_canvas_set_vadjustment (GXK_SCROLL_CANVAS (self->troll), gtk_range_get_adjustment (GTK_RANGE (vscroll)));
  bst_track_roll_set_track_callback (self->troll, self, get_track);
  track_view_marks_changed (self);

  /* link track roll to tree view and list model */
  g_signal_connect_object (tsel, "changed",
			   G_CALLBACK (bst_track_roll_reselect),
			   self->troll, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
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
  self->tctrl = bst_track_roll_controller_new (self->troll);
  bst_track_roll_controller_set_song (self->tctrl, iview->container);
  gxk_widget_publish_action_list (self, "tctrl-canvas-tools", bst_track_roll_controller_canvas_actions (self->tctrl));
  gxk_widget_publish_action_list (self, "tctrl-hpanel-tools", bst_track_roll_controller_hpanel_actions (self->tctrl));
  gxk_widget_publish_action_list (self, "tctrl-quant-tools", bst_track_roll_controller_quant_actions (self->tctrl));
  
  /* add repeat toggle */
  self->repeat_toggle = gxk_radget_find (radget, "repeat-toggle");
  gxk_nullify_in_object (self, &self->repeat_toggle);
  g_object_connect (self->repeat_toggle, "swapped_signal::toggled", track_view_repeat_toggled, self, NULL);
  track_view_repeat_changed (self);

  /* add zoom spinner */
  adjustment = gtk_adjustment_new (50, 1, 100, 1, 5, 0);
  g_object_connect (adjustment,
		    "swapped_signal_after::value_changed", track_view_hzoom_changed, self,
		    NULL);
  gxk_radget_add (self, "hzoom-area",
                  g_object_new (GTK_TYPE_SPIN_BUTTON,
                                "visible", TRUE,
                                "adjustment", adjustment,
                                "digits", 0,
                                "width_request", 2 * gxk_size_width (GXK_ICON_SIZE_TOOLBAR),
                                NULL));

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
				 0.0, _("Comment"), NULL,
				 bst_item_view_blurb_edited, self, G_CONNECT_SWAPPED);
}

static void
track_changed (SfiProxy      track,
	       BstTrackView *self)
{
  if (self->troll)
    {
      gint row = bst_item_view_get_proxy_row (BST_ITEM_VIEW (self), track);
      bst_track_roll_queue_row_change (self->troll, row);
    }
}

static void
track_view_pointer_changed (BstTrackView *self,
			    SfiInt        position)
{
  if (self->troll)
    bst_track_roll_set_marker (self->troll, 3, position, position >= 0 ? BST_TRACK_ROLL_MARKER_POS : 0);
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
  BST_ITEM_VIEW_CLASS (bst_track_view_parent_class)->set_container (iview, new_container);
  if (self->troll)
    bst_track_roll_setup (self->troll, iview->container ? iview->tree : NULL, iview->container);
  if (BSE_IS_SONG (iview->container))
    {
      bst_track_roll_controller_set_song (self->tctrl, iview->container);
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
  BST_ITEM_VIEW_CLASS (bst_track_view_parent_class)->listen_on (iview, item);
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
  BST_ITEM_VIEW_CLASS (bst_track_view_parent_class)->unlisten_on (iview, item);
}

static void
track_view_action_exec (gpointer data,
                        gulong   action)
{
  BstTrackView *self = BST_TRACK_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  SfiProxy song = item_view->container;

  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_TRACK:
      bse_item_group_undo (song, "Add Track");
      item = bse_song_create_track (song);
      if (item)
	{
	  gchar *string = g_strdup_printf ("Track-%02X", bse_item_get_seqid (item));
	  bse_item_set_name (item, string);
	  g_free (string);
	  bst_item_view_select (item_view, item);
          bse_song_ensure_output_busses (song);
	}
      bse_item_ungroup_undo (song);
      break;
    case ACTION_DELETE_TRACK:
      item = bst_item_view_get_current (item_view);
      bse_song_remove_track (song, item);
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
track_view_action_check (gpointer data,
                         gulong   action)
{
  BstTrackView *self = BST_TRACK_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);
  
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_TRACK:
      return TRUE;
    case ACTION_DELETE_TRACK:
      item = bst_item_view_get_current (item_view);
      return item != 0;
    default:
      return FALSE;
    }
}
