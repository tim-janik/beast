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


/* --- variables --- */
static BswProxy project = 0;
static BswProxy const_one = 0;
static BswProxy wave_repo = 0;
static BswProxy snet = 0;
static BswProxy mixer = 0;


/* --- functions --- */
void
bst_play_back_init (void)
{
  if (!project)
    {
      BswProxy speaker;

      project = bsw_server_use_new_project (BSW_SERVER, "# BEAST Play Back");
      gtk_idle_show_widget (GTK_WIDGET (bst_app_new (project)));	// FIXME
      snet = bsw_project_create_snet (project);
      bsw_proxy_set (snet, "auto_activate", TRUE, NULL);
      mixer = bsw_snet_create_source (snet, "BseMixer");
      speaker = bsw_snet_create_source (snet, "BsePcmOutput");
      bsw_source_set_input (speaker, 0, mixer, 0);
      bsw_source_set_input (speaker, 1, mixer, 0);
      wave_repo = bsw_project_ensure_wave_repo (project);
      const_one = bsw_snet_create_source (snet, "BseConstant");
      bsw_proxy_set (const_one, "value", 1.0, NULL);
    }
}

BstPlayBackHandle*
bst_play_back_handle_new (void)
{
  BstPlayBackHandle *handle;

  bst_play_back_init ();

  handle = g_new0 (BstPlayBackHandle, 1);
  handle->wave = BSE_OBJECT_ID (g_object_new (BSE_TYPE_WAVE, NULL));
  bse_container_add_item (bse_object_from_id (wave_repo),
			  bse_object_from_id (handle->wave));
  handle->wave_osc = bsw_snet_create_source (snet, "BseWaveOsc");

  handle->const_freq = bsw_snet_create_source (snet, "BseConstant");
  bsw_source_set_input (handle->wave_osc, 0, handle->const_freq, 0);
  
  /* bad hack, just try any of mixer's four input channels */
  if (bsw_source_set_input (mixer, 0, handle->wave_osc, 0) != BSW_ERROR_NONE &&
      bsw_source_set_input (mixer, 1, handle->wave_osc, 0) != BSW_ERROR_NONE &&
      bsw_source_set_input (mixer, 2, handle->wave_osc, 0) != BSW_ERROR_NONE &&
      bsw_source_set_input (mixer, 3, handle->wave_osc, 0) != BSW_ERROR_NONE)
    g_message (G_STRLOC ": unable to hook up playback handle");
  bsw_source_set_input (handle->wave_osc, 1, const_one, 0);

  return handle;
}

void
bst_play_back_handle_set (BstPlayBackHandle *handle,
			  GslWaveChunk      *wave_chunk,
			  gdouble            osc_freq)
{
  g_return_if_fail (handle != NULL);
  g_return_if_fail (wave_chunk != NULL);

  bst_play_back_init ();

  bse_wave_add_chunk (bse_object_from_id (handle->wave),
		      gsl_wave_chunk_copy (wave_chunk));
  bsw_proxy_set (handle->const_freq, "freq", osc_freq, NULL);
  
  bsw_proxy_set (handle->wave_osc, "wave", handle->wave, NULL);
}

void
bst_play_back_handle_start (BstPlayBackHandle *handle)
{
  BseErrorType error;

  error = bsw_server_run_project (BSW_SERVER, project);
  if (error)
    bst_status_printf (0, bse_error_blurb (error), "Playback");
}

void
bst_play_back_handle_destroy (BstPlayBackHandle *handle)
{
  g_return_if_fail (handle != NULL);

  bsw_wave_repo_remove_wave (wave_repo, handle->wave);
  handle->wave = 0;
  bsw_snet_remove_source (snet, handle->wave_osc);
  handle->wave_osc = 0;
  bsw_snet_remove_source (snet, handle->const_freq);
  handle->const_freq = 0;

  g_free (handle);
}
