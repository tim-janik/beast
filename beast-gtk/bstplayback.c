/* BEAST - Bedevilled Audio System
 * Copyright (C) 2001 Tim Janik
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
#include "bstplayback.h"
#include "bstapp.h"
#include "bststatusbar.h"

#include "../PKG_config.h"	/* BST_HAVE_BIRNET */



/* --- prototypes --- */
static void wave_oscillator_pcm_notify (BstPlayBackHandle *handle,
					guint		   pcm_position,
					BswProxy	   wosc);


/* --- functions --- */
BstPlayBackHandle*
bst_play_back_handle_new (void)
{
  BstPlayBackHandle *handle;

  handle = g_new0 (BstPlayBackHandle, 1);
  handle->project = bsw_server_use_new_project (BSW_SERVER, "# BEAST Play Back");
  if (BST_DVL_EXT)
    gtk_idle_show_widget (GTK_WIDGET (bst_app_new (handle->project)));

  handle->snet = bsw_project_create_snet (handle->project, NULL);
  bsw_proxy_set (handle->snet, "auto_activate", TRUE, NULL);
  handle->speaker = bsw_snet_create_source (handle->snet, "BsePcmOutput");
  handle->wosc = bsw_snet_create_source (handle->snet, "BseWaveOsc");
  bsw_source_set_input_by_id (handle->speaker, 0, handle->wosc, 0);
  bsw_source_set_input_by_id (handle->speaker, 1, handle->wosc, 0);
  handle->constant = bsw_snet_create_source (handle->snet, "BseConstant");
  bsw_source_set_input_by_id (handle->wosc, 0, handle->constant, 0);
  bsw_proxy_connect (handle->wosc,
		     "swapped_signal::notify_pcm_position", wave_oscillator_pcm_notify, handle,
		     NULL);
  return handle;
}

void
bst_play_back_handle_set (BstPlayBackHandle *handle,
			  BswProxy	     esample,
			  gdouble            osc_freq)
{
  g_return_if_fail (handle != NULL);
  if (esample)
    g_return_if_fail (BSW_IS_EDITABLE_SAMPLE (esample));

  bsw_proxy_set (handle->constant, "frequency_1", osc_freq, NULL);
  bsw_proxy_set (handle->wosc, "editable_sample", esample, NULL);
}

void
bst_play_back_handle_start (BstPlayBackHandle *handle)
{
  BseErrorType error;

  error = bsw_server_run_project (BSW_SERVER, handle->project);
  if (error)
    bst_status_eprintf (error, "Playback");
}

void
bst_play_back_handle_seek_perc (BstPlayBackHandle *handle,
				gfloat             perc)
{
  bsw_wave_osc_pcm_seek_perc (handle->wosc, perc);
}

void
bst_play_back_handle_stop (BstPlayBackHandle *handle)
{
  bsw_server_halt_project (BSW_SERVER, handle->project);
  bst_play_back_handle_pcm_notify (handle, 0, NULL, NULL);
}

void
bst_play_back_handle_toggle (BstPlayBackHandle *handle)
{
  if (bsw_project_is_playing (handle->project))
    bst_play_back_handle_stop (handle);
  else
    bst_play_back_handle_start (handle);
}

gboolean
bst_play_back_handle_is_playing (BstPlayBackHandle *handle)
{
  return bsw_project_is_playing (handle->project);
}

static void
wave_oscillator_pcm_notify (BstPlayBackHandle *handle,
			    guint              pcm_position,
			    BswProxy           wosc)
{
  g_assert (handle->wosc == wosc);

  if (handle->pcm_notify)
    handle->pcm_notify (handle->pcm_data, pcm_position);
}

static gboolean
pcm_timer (gpointer data)
{
  BstPlayBackHandle *handle = data;

  GDK_THREADS_ENTER ();
  bsw_wave_osc_request_pcm_position (handle->wosc);
  GDK_THREADS_LEAVE ();

  return TRUE;
}

void
bst_play_back_handle_pcm_notify (BstPlayBackHandle *handle,
				 guint		    timeout,
				 BstPlayBackNotify  notify,
				 gpointer           data)
{
  if (!bsw_project_is_playing (handle->project))
    {
      notify = NULL;
      data = NULL;
    }
  handle->pcm_notify = notify;
  handle->pcm_data = data;
  if (handle->pcm_notify && !handle->pcm_timeout)
    handle->pcm_timeout = g_timeout_add_full (GTK_PRIORITY_HIGH, timeout, pcm_timer, handle, NULL);
  else if (!handle->pcm_notify && handle->pcm_timeout)
    {
      g_source_remove (handle->pcm_timeout);
      handle->pcm_timeout = 0;
    }
}

void
bst_play_back_handle_destroy (BstPlayBackHandle *handle)
{
  g_return_if_fail (handle != NULL);

  bst_play_back_handle_stop (handle);

  bsw_proxy_disconnect (handle->wosc,
			"any_signal", wave_oscillator_pcm_notify, handle,
			NULL);

  if (handle->pcm_timeout)
    g_source_remove (handle->pcm_timeout);

  bsw_item_unuse (handle->project);
  g_free (handle);
}
