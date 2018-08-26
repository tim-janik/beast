// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsttrackview.hh"
#include "bstparam.hh"
#include "bstgrowbar.hh"
#include "bsttracksynthdialog.hh"
#include "bstitemseqdialog.hh"
#include <stdlib.h> /* strtol */
#include <string.h>

#define	SCROLLBAR_SPACING (3) /* from gtkscrolledwindow.c:DEFAULT_SCROLLBAR_SPACING */

/* --- prototypes --- */
static void	bst_track_view_finalize		(GObject		*object);
static gboolean track_view_action_check         (gpointer                data,
                                                 size_t                  action,
                                                 guint64                 action_stamp);
static void     track_view_action_exec          (gpointer                data,
                                                 size_t                  action);
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
  COL_OUTPUTS,
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
    ACTION_ADD_TRACK,     BST_STOCK_TRACKS_ADD,
  },
  { N_("Delete"),         NULL,         N_("Delete the currently selected track"),
    ACTION_DELETE_TRACK,  BST_STOCK_TRASHCAN,
  },
};


/* --- functions --- */
G_DEFINE_TYPE (BstTrackView, bst_track_view, BST_TYPE_ITEM_VIEW);

static void
bst_track_view_class_init (BstTrackViewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (klass);

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

  delete_inplace (self->song);
}

GtkWidget*
bst_track_view_new (SfiProxy song)
{
  GtkWidget *track_view;

  assert_return (BSE_IS_SONG (song), NULL);

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
  Bse::ContainerH container = iview->container;
  Bse::ItemH item = container.get_item (BST_ITEM_VIEW_GET_CLASS (self)->item_type, seqid);
  if (!item)
    return; // item is probably already destructed
  switch (column)
    {
      gboolean vbool;
      SfiInt vint;
      SfiProxy snet, wave, sound_font_preset;
    case COL_SEQID:
      sfi_value_take_string (value, g_strdup_format ("%03d", seqid));
      break;
    case COL_NAME:
      g_value_set_string (value, item.get_name().c_str());
      break;
    case COL_MUTE:
      bse_proxy_get (item.proxy_id(), "muted", &vbool, NULL);
      g_value_set_boolean (value, !vbool);
      break;
    case COL_VOICES:
      bse_proxy_get (item.proxy_id(), "n_voices", &vint, NULL);
      sfi_value_take_string (value, g_strdup_format ("%2d", vint));
      break;
    case COL_SYNTH:
      snet = 0;
      wave = 0;
      sound_font_preset = 0;
      bse_proxy_get (item.proxy_id(), "snet", &snet, "wave", &wave, "sound_font_preset", &sound_font_preset, NULL);
      {
        String string;
        if (snet)
          {
            Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (snet));
            string = item.get_name();
          }
        else if (wave)
          {
            Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (wave));
            string = item.get_name();
          }
        else if (sound_font_preset)
          {
            Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (sound_font_preset));
            string = item.get_name();
          }
        else
          string = "";
        g_value_set_string (value, string.c_str());
      }
      break;
    case COL_MIDI_CHANNEL:
      bse_proxy_get (item.proxy_id(), "midi-channel", &vint, NULL);
      sfi_value_take_string (value, g_strdup_format ("%2d", vint));
      break;
    case COL_OUTPUTS:
      {
        Bse::TrackH track = Bse::TrackH::down_cast (item);
        Bse::ItemSeq items = track.outputs();
        if (items.size() == 1)
          g_value_take_string (value, g_strdup_format ("%s", items[0].get_name_or_type()));
        else if (items.size() > 1)
          g_value_take_string (value, g_strdup_format ("#%u", items.size()));
        else if (items.size() > 1)
          g_value_set_string (value, "");
      }
      break;
    case COL_POST_SYNTH:
      snet = 0;
      bse_proxy_get (item.proxy_id(), "pnet", &snet, NULL);
      {
        Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (snet));
        g_value_set_string (value, item ? item.get_name().c_str() : "");
      }
      break;
    case COL_BLURB:
      {
        char *cstring = NULL;
        bse_proxy_get (item.proxy_id(), "blurb", &cstring, NULL);
        g_value_set_string (value, cstring ? cstring : "");
      }
      break;
    }
}

static void
track_view_synth_edited (BstTrackView *self,
			 const gchar  *strpath,
			 const gchar  *text)
{
  assert_return (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy itemid = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      if (text)
	{
	  SfiProxy proxy = 0;
	  GSList *slist = NULL;
          Bse::PropertyCandidates pc;
	  /* list possible snet/wave/sound_font_preset candidates */
          Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (itemid));
          pc = item.get_property_candidates ("snet");
          auto snet_i3s = std::unique_ptr<BseIt3mSeq> (bst_it3m_seq_from_item_seq (pc.items));
	  slist = g_slist_append (slist, snet_i3s.get());
          pc = item.get_property_candidates ("wave");
          auto wave_i3s = std::unique_ptr<BseIt3mSeq> (bst_it3m_seq_from_item_seq (pc.items));
	  slist = g_slist_append (slist, wave_i3s.get());
          pc = item.get_property_candidates ("sound_font_preset");
          auto sfpr_i3s = std::unique_ptr<BseIt3mSeq> (bst_it3m_seq_from_item_seq (pc.items));
          slist = g_slist_append (slist, sfpr_i3s.get());
	  /* find best match */
	  proxy = bst_item_seq_list_match (slist, text);
	  g_slist_free (slist);
	  if (proxy && BSE_IS_SNET (proxy))
	    bse_proxy_set (itemid, "snet", proxy, NULL);
	  else if (proxy && BSE_IS_WAVE (proxy))
	    bse_proxy_set (itemid, "wave", proxy, NULL);
	  else if (proxy && BSE_IS_SOUND_FONT_PRESET (proxy))
	    bse_proxy_set (itemid, "sound_font_preset", proxy, NULL);
	  else
	    bse_proxy_set (itemid, "snet", 0, "wave", 0, "sound_font_preset", 0, NULL);
	}
      else
	bse_proxy_set (itemid, "snet", 0, "wave", 0, "sound_font_preset", 0, NULL);
    }
}

static void
track_view_post_synth_edited (BstTrackView *self,
                              const gchar  *strpath,
                              const gchar  *text)
{
  assert_return (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy itemid = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      if (text)
	{
	  SfiProxy proxy = 0;
	  GSList *slist = NULL;
	  /* list possible snet candidates */

          Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (itemid));
          Bse::PropertyCandidates pc = item.get_property_candidates ("pnet");
          auto snet_i3s = std::unique_ptr<BseIt3mSeq> (bst_it3m_seq_from_item_seq (pc.items));
	  slist = g_slist_append (slist, snet_i3s.get());
	  /* find best match */
	  proxy = bst_item_seq_list_match (slist, text);
	  g_slist_free (slist);
	  if (proxy && BSE_IS_SNET (proxy))
	    bse_proxy_set (itemid, "pnet", proxy, NULL);
	  else
	    bse_proxy_set (itemid, "pnet", 0, NULL);
	}
      else
	bse_proxy_set (itemid, "pnet", 0, NULL);
    }
}

typedef struct {
  BstTrackView         *self;
  GxkCellRendererPopup *pcell;
} SynthPopup;

static void
track_view_synth_popup_cleanup (gpointer data)
{
  SynthPopup *sdata = (SynthPopup*) data;
  gxk_cell_renderer_popup_change (sdata->pcell, NULL, FALSE, TRUE);
  g_free (sdata);
}

static void
track_view_synth_popup_cb (gpointer              data,
                           SfiProxy              proxy,
                           BstTrackSynthDialog  *tsdialog)
{
  SynthPopup *sdata = (SynthPopup*) data;
  Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (proxy));
  gxk_cell_renderer_popup_change (sdata->pcell,
                                  item ? item.get_uname_path().c_str() : "",
                                  FALSE,
                                  item == NULL);
}

static void
track_view_synth_popup (BstTrackView         *self,
			const gchar          *strpath,
			const gchar          *text,
			GxkCellRendererPopup *pcell)
{
  assert_return (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy itemid = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (itemid));
      if (item.editable_property ("snet"))
        {
          Bse::PropertyCandidates pc = item.get_property_candidates ("snet");
          BseIt3mSeq *pc_items = bst_it3m_seq_from_item_seq (pc.items);
          SynthPopup sdata = { self, pcell, };
          Bse::ProjectH project = item.get_project();
          GtkWidget *dialog = bst_track_synth_dialog_popup (self, itemid,
                                                            pc.label.c_str(), pc.tooltip.c_str(), pc_items,
                                                            _("Available Waves"),
                                                            _("List of available waves to choose a track instrument from"),
                                                            project.get_wave_repo().proxy_id(),
							    _("Available Sound Fonts"),
							    _("List of available sound fonts to choose track instrument from"),
							    project.get_sound_font_repo().proxy_id(),
                                                            track_view_synth_popup_cb, g_memdup (&sdata, sizeof (sdata)), track_view_synth_popup_cleanup);
          bse_it3m_seq_free (pc_items);
          gxk_cell_renderer_popup_dialog (pcell, dialog);
        }
      else
        bst_gui_error_bell (self);
    }
}

static void
track_view_post_synth_popup (BstTrackView         *self,
                             const gchar          *strpath,
                             const gchar          *text,
                             GxkCellRendererPopup *pcell)
{
  assert_return (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy itemid = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      Bse::ItemH item = Bse::ItemH::down_cast (bse_server.from_proxy (itemid));
      if (item.editable_property ("pnet"))
        {
          Bse::PropertyCandidates pc = item.get_property_candidates ("pnet");
          BseIt3mSeq *pc_items = bst_it3m_seq_from_item_seq (pc.items);
          SynthPopup sdata = { self, pcell, };
          GtkWidget *dialog = bst_track_synth_dialog_popup (self, itemid,
                                                            pc.label.c_str(), pc.tooltip.c_str(), pc_items,
                                                            NULL, NULL, 0,
							    NULL, NULL, 0,
                                                            track_view_synth_popup_cb, g_memdup (&sdata, sizeof (sdata)), track_view_synth_popup_cleanup);
          bse_it3m_seq_free (pc_items);
          gxk_cell_renderer_popup_dialog (pcell, dialog);
        }
      else
        bst_gui_error_bell (self);
    }
}

typedef struct {
  BstTrackView         *self;
  GxkCellRendererPopup *pcell;
  SfiProxy              item;
} OutputsPopup;

static void
track_view_outputs_cleanup (gpointer data)
{
  OutputsPopup *odata = (OutputsPopup*) data;
  gxk_cell_renderer_popup_change (odata->pcell, NULL, FALSE, FALSE);
  g_free (odata);
}

static void
track_view_outputs_changed (gpointer              data,
                            BseIt3mSeq           *iseq,
                            BstItemSeqDialog     *isdialog)
{
  OutputsPopup *odata = (OutputsPopup*) data;
  gxk_cell_renderer_popup_change (odata->pcell, NULL, FALSE, FALSE);
  Bse::TrackH track = Bse::TrackH::down_cast (bse_server.from_proxy (odata->item));
  track.outputs (bst_item_seq_from_it3m_seq (iseq));
}

static void
track_view_outputs_popup (BstTrackView         *self,
                          const gchar          *strpath,
                          const gchar          *text,
                          GxkCellRendererPopup *pcell)
{
  assert_return (BST_IS_TRACK_VIEW (self));

  if (strpath)
    {
      gint row = gxk_tree_spath_index0 (strpath);
      SfiProxy itemid = bst_item_view_get_proxy (BST_ITEM_VIEW (self), row);
      Bse::TrackH track = Bse::TrackH::down_cast (bse_server.from_proxy (itemid));
      assert_return (track != NULL);
      Bse::PropertyCandidates pc = track.get_property_candidates ("outputs");
      Bse::ItemSeq items = track.outputs();
      BseIt3mSeq *iseq = bst_it3m_seq_from_item_seq (items);
      OutputsPopup odata = { self, pcell, track.proxy_id() };
      BseIt3mSeq *pc_items = bst_it3m_seq_from_item_seq (pc.items);
      GtkWidget *dialog = bst_item_seq_dialog_popup (self, track.proxy_id(),
                                                     pc.label.c_str(), pc.tooltip.c_str(), pc_items,
                                                     _("Output Signals"), _("Mixer busses used as output for this track"), iseq,
                                                     track_view_outputs_changed, g_memdup (&odata, sizeof (odata)), track_view_outputs_cleanup);
      bse_it3m_seq_free (pc_items);
      bse_it3m_seq_free (iseq);
      gxk_cell_renderer_popup_dialog (pcell, dialog);
    }
}

static void
track_view_mute_toggled (BstTrackView          *self,
			 const gchar           *strpath,
			 GtkCellRendererToggle *tcell)
{
  assert_return (BST_IS_TRACK_VIEW (self));

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
  assert_return (BST_IS_TRACK_VIEW (self));

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
  assert_return (BST_IS_TRACK_VIEW (self));

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

static Bse::TrackH
get_track (void *data, int row)
{
  SfiProxy proxy = bst_item_view_get_proxy (BST_ITEM_VIEW (data), row);
  Bse::TrackH track = Bse::TrackH::down_cast (bse_server.from_proxy (proxy));
  return track;
}

static void
track_view_marks_changed (BstTrackView *self)
{
  if (self->troll && self->song)
    {
      SfiInt lleft = self->song.loop_left();
      SfiInt lright, pointer;
      bse_proxy_get (self->song.proxy_id(), "loop_right", &lright, "tick_pointer", &pointer, NULL);
      bst_track_roll_set_marker (self->troll, 1, lleft, lleft >= 0 ? BST_TRACK_ROLL_MARKER_LOOP : BST_TRACK_ROLL_MARKER_NONE);
      bst_track_roll_set_marker (self->troll, 2, lright, lright >= 0 ? BST_TRACK_ROLL_MARKER_LOOP : BST_TRACK_ROLL_MARKER_NONE);
      bst_track_roll_set_marker (self->troll, 3, pointer, pointer >= 0 ? BST_TRACK_ROLL_MARKER_POS : BST_TRACK_ROLL_MARKER_NONE);
    }
}

static void
track_view_repeat_toggled (BstTrackView *self)
{
  if (self->song && self->repeat_toggle)
    self->song.loop_enabled (GTK_TOGGLE_BUTTON (self->repeat_toggle)->active);
}

static void
track_view_repeat_changed (BstTrackView *self)
{
  if (self->song && self->repeat_toggle)
    {
      GtkToggleButton *toggle = GTK_TOGGLE_BUTTON (self->repeat_toggle);
      gboolean enabled = self->song.loop_enabled();
      if (toggle->active != enabled)
	gtk_toggle_button_set_active (toggle, enabled);
    }
}

static void
bst_track_view_init (BstTrackView *self)
{
  new_inplace (self->song);

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
				   G_TYPE_STRING,	/* COL_OUTPUTS */
				   G_TYPE_STRING,	/* COL_POST_SYNTH */
				   G_TYPE_STRING	/* COL_BLURB */
				   );
  smodel = bst_item_view_adapt_list_wrapper (iview, lwrapper);
  g_signal_connect_object (lwrapper, "fill-value",
			   G_CALLBACK (track_view_fill_value),
			   iview, G_CONNECT_SWAPPED);
  g_object_unref (lwrapper);

  /* scrollbars */
  treehs = (GtkWidget*) gxk_radget_find (radget, "tree-hscrollbar");
  trackgb = (GtkWidget*) gxk_radget_find (radget, "track-hgrow-bar");
  vscroll = (GtkWidget*) gxk_radget_find (radget, "tree-vscrollbar");

  /* tree view (track list) */
  tview = (GtkTreeView*) gxk_radget_find (radget, "tree-view");
  gtk_tree_view_set_model (tview, smodel);
  bst_item_view_set_tree (iview, tview);
  gtk_tree_view_set_hadjustment (iview->tree, gtk_range_get_adjustment (GTK_RANGE (treehs)));
  gtk_tree_view_set_vadjustment (iview->tree, gtk_range_get_adjustment (GTK_RANGE (vscroll)));
  tsel = gtk_tree_view_get_selection (iview->tree);
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_BROWSE);
  gxk_tree_selection_force_browse (tsel, smodel);
  g_object_unref (smodel);

  /* track roll */
  self->troll = (BstTrackRoll*) g_object_new (BST_TYPE_TRACK_ROLL,
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
  bst_track_roll_controller_set_song (self->tctrl, self->song ? self->song.proxy_id() : 0);
  gxk_widget_publish_action_list (self, "tctrl-canvas-tools", bst_track_roll_controller_canvas_actions (self->tctrl));
  gxk_widget_publish_action_list (self, "tctrl-hpanel-tools", bst_track_roll_controller_hpanel_actions (self->tctrl));
  gxk_widget_publish_action_list (self, "tctrl-quant-tools", bst_track_roll_controller_quant_actions (self->tctrl));

  /* add repeat toggle */
  self->repeat_toggle = (GtkWidget*) gxk_radget_find (radget, "repeat-toggle");
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
                                   NULL, NULL, GConnectFlags (0));
  gxk_tree_view_add_text_column (iview->tree, COL_NAME, "S",
				 0.0, _("Name"), NULL,
				 (void*) bst_item_view_name_edited, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_toggle_column (iview->tree, COL_MUTE, "",
				   0.5, "M", _("Notes from unchecked tracks are ignored by the sequencer during playback"),
				   (void*) track_view_mute_toggled, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_text_column (iview->tree, COL_VOICES, "",
				 0.5, "V", _("Maximum number of voices for simultaneous playback"),
				 (void*) track_view_voice_edited, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_popup_column (iview->tree, COL_SYNTH, "#",
				  0.5, "Synth", _("Synthesis network or wave to be used as instrument by this track"),
                                  (void*) track_view_synth_edited, (void*) track_view_synth_popup, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_text_column (iview->tree, COL_MIDI_CHANNEL, "",
                                 0.5, "Ch", _("Midi channel assigned to this track, 0 uses private per-track channel"),
                                 (void*) track_view_midi_channel_edited, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_popup_column (iview->tree, COL_OUTPUTS, "#",
				  0.5, "Outputs", _("Mixer busses connected to track output"),
				  NULL, (void*) track_view_outputs_popup, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_popup_column (iview->tree, COL_POST_SYNTH, "",
				  0.5, "Post", _("Synthesis network to be used as postprocessor"),
				  (void*) track_view_post_synth_edited, (void*) track_view_post_synth_popup, self, G_CONNECT_SWAPPED);
  gxk_tree_view_add_text_column (iview->tree, COL_BLURB, "",
				 0.0, _("Comment"), NULL,
				 (void*) bst_item_view_blurb_edited, self, G_CONNECT_SWAPPED);
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
    bst_track_roll_set_marker (self->troll, 3, position, position >= 0 ? BST_TRACK_ROLL_MARKER_POS : BST_TRACK_ROLL_MARKER_NONE);
}

static void
track_view_set_container (BstItemView *iview,
			  SfiProxy     new_container)
{
  BstTrackView *self = BST_TRACK_VIEW (iview);
  if (self->song)
    {
      bse_proxy_disconnect (self->song.proxy_id(),
                            "any_signal", track_view_pointer_changed, self,
                            "any_signal", track_view_marks_changed, self,
                            NULL);
      self->song = NULL; // disconnects event handlers
    }
  BST_ITEM_VIEW_CLASS (bst_track_view_parent_class)->set_container (iview, new_container);
  if (self->troll)
    {
      if (iview->container)
        bst_track_roll_setup (self->troll, iview->tree, Bse::SongH::down_cast (iview->container));
      else
        bst_track_roll_setup (self->troll, NULL, Bse::SongH());
    }
  self->song = Bse::SongH::down_cast (iview->container);
  if (self->song)
    {
      bst_track_roll_controller_set_song (self->tctrl, self->song.proxy_id());
      self->song.on ("notify:loop_enabled", [self]() { track_view_repeat_changed (self); });
      self->song.on ("notify:loop_left", [self]() { track_view_marks_changed (self); });
      bse_proxy_connect (self->song.proxy_id(),
			 "swapped_signal::pointer-changed", track_view_pointer_changed, self,
			 "swapped_signal::property-notify::loop-right", track_view_marks_changed, self,
			 "swapped_signal::property-notify::tick-pointer", track_view_marks_changed, self,
			 NULL);
      track_view_marks_changed (self);
      track_view_repeat_changed (self);
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
                     "signal::property-notify::outputs", track_property_changed, iview, /* COL_OUTPUTS */
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
                        size_t   action)
{
  BstTrackView *self = BST_TRACK_VIEW (data);
  BstItemView *item_view = BST_ITEM_VIEW (self);

  Bse::TrackH track;
  switch (action)
    {
      SfiProxy item;
    case ACTION_ADD_TRACK:
      self->song.group_undo ("Add Track");
      track = self->song.create_track();
      if (track)
	{
	  gchar *string = g_strdup_format ("Track-%02X", track.get_seqid());
	  track.set_name (string);
	  g_free (string);
	  bst_item_view_select (item_view, track.proxy_id());
          track.ensure_output();
	}
      self->song.ungroup_undo();
      break;
    case ACTION_DELETE_TRACK:
      item = bst_item_view_get_current (item_view);
      track = Bse::TrackH::down_cast (bse_server.from_proxy (item));
      self->song.group_undo ("Delete Track");
      Bse::PartSeq pseq = track.list_parts_uniq();
      self->song.remove_track (track);
      for (const auto &part : pseq)
        {
          Bse::PartH p = part;
          if (!self->song.find_any_track_for_part (p))
            self->song.remove_part (p);
        }
      self->song.ungroup_undo();
      break;
    }
  gxk_widget_update_actions_downwards (self);
}

static gboolean
track_view_action_check (gpointer data,
                         size_t   action,
                         guint64  action_stamp)
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
