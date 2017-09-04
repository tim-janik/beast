// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstplayback.hh"
#include "bstapp.hh"




/* --- prototypes --- */
static void wave_oscillator_pcm_notify (BstPlayBackHandle *handle,
					SfiNum             tick_stamp,
					guint		   pcm_position,
					SfiProxy	   wosc);


/* --- functions --- */
BstPlayBackHandle*
bst_play_back_handle_new (void)
{
  BstPlayBackHandle *handle = new BstPlayBackHandle();

  handle->project = bse_server.create_project ("# BEAST Play Back");
  if (BST_DBG_EXT)
    gxk_idle_show_widget (GTK_WIDGET (bst_app_new (handle->project)));

  handle->snet = handle->project.create_csynth ("");
  handle->snet.auto_activate (true);
  handle->speaker = handle->snet.create_source ("BsePcmOutput");
  handle->wosc1 = Bse::WaveOscH::down_cast (handle->snet.create_source ("BseWaveOsc"));
  handle->wosc2 = Bse::WaveOscH::down_cast (handle->snet.create_source ("BseWaveOsc"));
  bse_proxy_set (handle->wosc2.proxy_id(), "channel", 2, NULL);
  handle->speaker.set_input_by_id (0, handle->wosc1, 0);
  handle->speaker.set_input_by_id (1, handle->wosc2, 0);
  handle->constant = handle->snet.create_source ("BseConstant");
  handle->wosc1.set_input_by_id (0, handle->constant, 0);
  handle->wosc2.set_input_by_id (0, handle->constant, 0);
  bse_proxy_connect (handle->wosc1.proxy_id(),
		     "swapped_signal::notify_pcm_position", wave_oscillator_pcm_notify, handle,
		     NULL);
  return handle;
}

void
bst_play_back_handle_set (BstPlayBackHandle *handle,
			  SfiProxy	     esample,
			  gdouble            osc_freq)
{
  assert_return (handle != NULL);
  if (esample)
    assert_return (BSE_IS_EDITABLE_SAMPLE (esample));

  bse_proxy_set (handle->constant.proxy_id(), "frequency_1", osc_freq, NULL);
  Bse::EditableSampleH es = Bse::EditableSampleH::down_cast (bse_server.from_proxy (esample));
  handle->wosc1.set_from_editable_sample (es);
  handle->wosc2.set_from_editable_sample (es);
}

void
bst_play_back_handle_start (BstPlayBackHandle *handle)
{
  Bse::Error error;

  error = handle->project.play();;
  if (error != 0)
    bst_status_eprintf (error, _("Playback"));
}

void
bst_play_back_handle_seek_perc (BstPlayBackHandle *handle, float perc)
{
  Bse::WaveOscSeq woscs;
  woscs.push_back (handle->wosc1);
  if (handle->wosc2)
    woscs.push_back (handle->wosc2);
  handle->wosc1.sync_seek_perc (perc, woscs);
  if (handle->waiting_for_notify)
    handle->discard_next_notify = TRUE;
}

void
bst_play_back_handle_stop (BstPlayBackHandle *handle)
{
  handle->project.stop();;
  bst_play_back_handle_pcm_notify (handle, 0, NULL, NULL);
}

void
bst_play_back_handle_toggle (BstPlayBackHandle *handle)
{
  if (handle->project.is_playing())
    bst_play_back_handle_stop (handle);
  else
    bst_play_back_handle_start (handle);
}

gboolean
bst_play_back_handle_is_playing (BstPlayBackHandle *handle)
{
  return handle->project.is_playing();
}

static void
wave_oscillator_pcm_notify (BstPlayBackHandle *handle,
			    SfiNum             tick_stamp,
			    guint              pcm_position,
			    SfiProxy           wosc)
{
  gboolean discard_next_notify = handle->discard_next_notify;

  assert_return (handle->wosc1.proxy_id() == wosc);

  handle->waiting_for_notify = FALSE;
  handle->discard_next_notify = FALSE;
  if (handle->pcm_notify && !discard_next_notify)
    handle->pcm_notify (handle->pcm_data, tick_stamp, pcm_position);
}

static gboolean
pcm_timer (gpointer data)
{
  BstPlayBackHandle *handle = (BstPlayBackHandle*) data;

  GDK_THREADS_ENTER ();
  if (!handle->waiting_for_notify)
    {
      handle->wosc1.request_pcm_position();
      handle->waiting_for_notify = TRUE;
    }
  GDK_THREADS_LEAVE ();

  return TRUE;
}

void
bst_play_back_handle_pcm_notify (BstPlayBackHandle *handle,
				 guint		    timeout,
				 BstPlayBackNotify  notify,
				 gpointer           data)
{
  if (!handle->project.is_playing())
    {
      notify = NULL;
      data = NULL;
    }
  handle->pcm_notify = notify;
  handle->pcm_data = data;
  if (handle->pcm_timeout)
    {
      g_source_remove (handle->pcm_timeout);
      handle->pcm_timeout = 0;
      if (handle->waiting_for_notify)
	handle->discard_next_notify = TRUE;
    }
  if (handle->pcm_notify)
    {
      handle->current_delay = timeout;
      handle->pcm_timeout = g_timeout_add_full (GTK_PRIORITY_HIGH, handle->current_delay,
						pcm_timer, handle, NULL);
    }
}

void
bst_play_back_handle_time_pcm_notify (BstPlayBackHandle *handle,
				      guint              timeout)
{
  if (handle->current_delay != timeout && handle->pcm_timeout)
    {
      handle->current_delay = timeout;
      g_source_remove (handle->pcm_timeout);
      handle->pcm_timeout = g_timeout_add_full (GTK_PRIORITY_HIGH, handle->current_delay,
						pcm_timer, handle, NULL);
    }
}

void
bst_play_back_handle_destroy (BstPlayBackHandle *handle)
{
  assert_return (handle != NULL);

  bst_play_back_handle_stop (handle);

  bse_proxy_disconnect (handle->wosc1.proxy_id(), "any_signal", wave_oscillator_pcm_notify, handle, NULL);

  if (handle->pcm_timeout)
    g_source_remove (handle->pcm_timeout);

  bse_server.destroy_project (handle->project);
  delete handle;
}
