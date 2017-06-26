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
  handle->speaker = handle->snet.create_source ("BsePcmOutput").proxy_id();
  handle->wosc1 = handle->snet.create_source ("BseWaveOsc").proxy_id();
  handle->wosc2 = handle->snet.create_source ("BseWaveOsc").proxy_id();
  bse_proxy_set (handle->wosc2, "channel", 2, NULL);
  bse_source_set_input_by_id (handle->speaker, 0, handle->wosc1, 0);
  bse_source_set_input_by_id (handle->speaker, 1, handle->wosc2, 0);
  handle->constant = handle->snet.create_source ("BseConstant").proxy_id();
  bse_source_set_input_by_id (handle->wosc1, 0, handle->constant, 0);
  bse_source_set_input_by_id (handle->wosc2, 0, handle->constant, 0);
  bse_proxy_connect (handle->wosc1,
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

  bse_proxy_set (handle->constant, "frequency_1", osc_freq, NULL);
  bse_wave_osc_set_from_editable_sample (handle->wosc1, esample);
  bse_wave_osc_set_from_editable_sample (handle->wosc2, esample);
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
bst_play_back_handle_seek_perc (BstPlayBackHandle *handle,
				gfloat             perc)
{
  BseIt3mSeq *iseq = bse_it3m_seq_new();
  bse_it3m_seq_append (iseq, handle->wosc1);
  if (handle->wosc2)
    bse_it3m_seq_append (iseq, handle->wosc2);
  bse_wave_osc_mass_seek_perc (iseq, perc);
  if (handle->waiting_for_notify)
    handle->discard_next_notify = TRUE;
  bse_it3m_seq_free (iseq);
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

  assert_return (handle->wosc1 == wosc);

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
      bse_wave_osc_request_pcm_position (handle->wosc1);
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

  bse_proxy_disconnect (handle->wosc1, "any_signal", wave_oscillator_pcm_notify, handle, NULL);

  if (handle->pcm_timeout)
    g_source_remove (handle->pcm_timeout);

  bse_server.destroy_project (handle->project);
  delete handle;
}
