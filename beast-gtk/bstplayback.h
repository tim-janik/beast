





static BswProxy mixer, snet, wave_repo, const_one, project = 0;

void
bst_play_back_init (void)
{
  if (!project)
    {
      BswProxy speaker;

      project = bsw_server_use_new_project (BSW_SERVER, "# BEAST play back");
      gtk_idle_show_widget (bst_app_new (project));
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

typedef struct
{
  BswProxy wave;
  BswProxy wave_osc;
  BswProxy const_freq;
} BstPlayBackHandle;

BstPlayBackHandle*	bst_play_back_handle_new	(GslWaveChunk		*wave_chunk);
void			bst_play_back_handle_start	(BstPlayBackHandle	*handle);
void			bst_play_back_handle_stop	(BstPlayBackHandle	*handle);
gboolean		bst_play_back_handle_done	(BstPlayBackHandle	*handle);
void			bst_play_back_handle_destroy	(BstPlayBackHandle	*handle);

#include	<gsl/gslwavechunk.h>

BstPlayBackHandle*
bst_play_back_handle_new (GslWaveChunk *wave_chunk)
{
  BstPlayBackHandle *handle = g_new0 (BstPlayBackHandle, 1);

  bst_play_back_init ();
  handle->wave = BSE_OBJECT_ID (g_object_new (BSE_TYPE_WAVE, NULL));
  bse_container_add_item (bse_object_from_id (wave_repo),
			  bse_object_from_id (handle->wave));
  bse_wave_add_chunk (bse_object_from_id (handle->wave),
		      gsl_wave_chunk_copy (wave_chunk));
  handle->wave_osc = bsw_snet_create_source (snet, "BseWaveOsc");

  handle->const_freq = bsw_snet_create_source (snet, "BseConstant");
  bsw_proxy_set (handle->const_freq, "freq", wave_chunk->osc_freq, NULL);
  bsw_source_set_input (handle->wave_osc, 0, handle->const_freq, 0);
  
  /* bad hack, just try any of mixer's four input channels */
  if (bsw_source_set_input (mixer, 0, handle->wave_osc, 0) != BSW_ERROR_NONE &&
      bsw_source_set_input (mixer, 1, handle->wave_osc, 0) != BSW_ERROR_NONE &&
      bsw_source_set_input (mixer, 2, handle->wave_osc, 0) != BSW_ERROR_NONE &&
      bsw_source_set_input (mixer, 3, handle->wave_osc, 0) != BSW_ERROR_NONE)
    g_message (G_STRLOC ": unable to hook up playback handle");
  bsw_proxy_set (handle->wave_osc, "wave", handle->wave, NULL);
  bsw_source_set_input (handle->wave_osc, 1, const_one, 0);

  return handle;
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
}
