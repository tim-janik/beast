/* BsePCMModule - BSE PCM Sink GslModule
 * Copyright (C) 1999-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "gslengine.h"
#include "bsepcmdevice.h"



/* --- typedefs & structures --- */
typedef struct
{
  guint           n_values;     /* gsl_engine_block_size() * 2 (stereo) */
  BseSampleValue *buffer;
  BseSampleValue *bound;
  BsePcmHandle   *handle;
} BsePCMModuleData;
enum
{
  BSE_PCM_MODULE_ISTREAM_LEFT,
  BSE_PCM_MODULE_ISTREAM_RIGHT
};


/* --- prototypes --- */
static GslModule*	bse_pcm_module_insert	(BsePcmHandle	*handle,
						 GslTrans	*trans);
static void		bse_pcm_module_remove	(GslModule	*pcm_module,
						 GslTrans	*trans);


/* --- functions --- */
static gboolean
bse_pcm_module_poll (gpointer         data,
		     guint            n_values,
		     glong           *timeout_p,
		     guint            n_fds,
		     const GslPollFD *fds,
		     gboolean         revents_filled)
{
#if 0
  /* written for full OSS buffer fills */
  BsePCMModuleData *mdata = data;
  BsePcmStatus status;
  gfloat diff;
  
  /* get playback status */
  bse_pcm_handle_status (mdata->handle, &status);
  
  /* already enough space for another write? */
  if (status.n_playback_values_left >= mdata->n_values)
    return TRUE;
  
  /* when do we have enough space available? */
  diff = mdata->n_values - status.n_playback_values_left;
  *timeout_p = diff * 1000.0 / mdata->handle->mix_freq;
  
  return *timeout_p == 0;
#else
  /* check if less than 3 fragments are supplied */
  BsePCMModuleData *mdata = data;
  BsePcmStatus status;
  gfloat diff;
  guint need_values;
  
  /* get playback status */
  bse_pcm_handle_status (mdata->handle, &status);
  
  /* already enough space for another write? */
  need_values = MIN (mdata->handle->playback_watermark, status.total_playback_values);
  need_values = status.total_playback_values - mdata->handle->playback_watermark;
  need_values = MAX (need_values, n_values * 2 /* stereo */);
  if (status.n_playback_values_left >= need_values)
    return TRUE;
  
  /* when do we have enough space available? */
  diff = need_values - status.n_playback_values_left;
  *timeout_p = diff * 1000.0 / mdata->handle->mix_freq;
  
  return *timeout_p == 0;
#endif
}

static void
bse_pcm_module_process (GslModule *module,
			guint      n_values)
{
  BsePCMModuleData *mdata = module->user_data;
  const BseSampleValue *left = GSL_MODULE_IBUFFER (module, BSE_PCM_MODULE_ISTREAM_LEFT);
  const BseSampleValue *right = GSL_MODULE_IBUFFER (module, BSE_PCM_MODULE_ISTREAM_RIGHT);
  BseSampleValue *d = mdata->buffer;
  BseSampleValue *b = mdata->bound;
  
  g_return_if_fail (n_values == mdata->n_values >> 1);
  
  do
    {
      *d++ = *left++;
      *d++ = *right++;
    }
  while (d < b);
  
  bse_pcm_handle_write (mdata->handle, mdata->n_values, mdata->buffer);
}

static void
bse_pcm_module_data_free (gpointer        data,
			  const GslClass *klass)
{
  BsePCMModuleData *mdata = data;
  
  g_free (mdata->buffer);
  g_free (mdata);
}

GslModule*
bse_pcm_module_insert (BsePcmHandle *handle,
		       GslTrans     *trans)
{
  static const GslClass pcm_module_class = {
    2,				/* n_istreams */
    0,				/* n_jstreams */
    0,				/* n_ostreams */
    bse_pcm_module_process,	/* process */
    bse_pcm_module_data_free,	/* free */
    GSL_COST_CHEAP,		/* cost */
  };
  BsePCMModuleData *mdata;
  GslModule *module;
  
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (handle->write != NULL, NULL);
  g_return_val_if_fail (trans != NULL, NULL);
  
  mdata = g_new (BsePCMModuleData, 1);
  mdata->n_values = gsl_engine_block_size () * 2;
  mdata->buffer = g_new (BseSampleValue, mdata->n_values);
  mdata->bound = mdata->buffer + mdata->n_values;
  mdata->handle = handle;
  module = gsl_module_new (&pcm_module_class, mdata);
  
  gsl_trans_add (trans,
		 gsl_job_integrate (module));
  gsl_trans_add (trans,
		 gsl_job_add_poll (bse_pcm_module_poll, mdata, NULL, 0, NULL));
  
  return module;
}

void
bse_pcm_module_remove (GslModule *pcm_module,
		       GslTrans  *trans)
{
  BsePCMModuleData *mdata;
  
  g_return_if_fail (pcm_module != NULL);
  g_return_if_fail (trans != NULL);
  
  mdata = pcm_module->user_data;
  gsl_trans_add (trans,
		 gsl_job_remove_poll (bse_pcm_module_poll, mdata));
  gsl_trans_add (trans,
		 gsl_job_discard (pcm_module));
}
