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
  guint         n_values;	/* gsl_engine_block_size() * 2 (stereo) */
  gfloat       *buffer;
  gfloat       *bound;
  BsePcmHandle *handle;
} BsePCMModuleData;
enum
{
  BSE_PCM_MODULE_JSTREAM_LEFT,
  BSE_PCM_MODULE_JSTREAM_RIGHT,
  BSE_PCM_MODULE_N_JSTREAMS
};
enum
{
  BSE_PCM_MODULE_OSTREAM_LEFT,
  BSE_PCM_MODULE_OSTREAM_RIGHT,
  BSE_PCM_MODULE_N_OSTREAMS
};


/* --- prototypes --- */
static GslModule*	bse_pcm_omodule_insert	(BsePcmHandle	*handle,
						 GslTrans	*trans);
static void		bse_pcm_omodule_remove	(GslModule	*pcm_module,
						 GslTrans	*trans);
static GslModule*	bse_pcm_imodule_insert	(BsePcmHandle	*handle,
						 GslTrans	*trans);
static void		bse_pcm_imodule_remove	(GslModule	*pcm_module,
						 GslTrans	*trans);


/* --- functions --- */
static gboolean
bse_pcm_module_poll (gpointer       data,
		     guint          n_values,
		     glong         *timeout_p,
		     guint          n_fds,
		     const GPollFD *fds,
		     gboolean       revents_filled)
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
  BsePCMModuleData *mdata = data;
  BsePcmHandle *handle = mdata->handle;
  BsePcmStatus status;
  guint fillmark, watermark;
  
  /* get playback status */
  bse_pcm_handle_status (mdata->handle, &status);
  
  watermark = status.total_playback_values - MIN (mdata->n_values, status.total_playback_values);
  watermark = MIN (watermark, handle->playback_watermark);
  fillmark = status.total_playback_values - status.n_playback_values_available;
  if (fillmark <= watermark)
    return TRUE;	/* need to write out stuff now */
  
  fillmark -= watermark;
  fillmark /= handle->n_channels;
  *timeout_p = fillmark * 1000.0 / mdata->handle->mix_freq;
  
  return *timeout_p == 0;
#endif
}

static void
bse_pcm_omodule_process (GslModule *module,
			 guint      n_values)
{
  BsePCMModuleData *mdata = module->user_data;
  gfloat *d = mdata->buffer;
  gfloat *b = mdata->bound;
  const gfloat *src;
  guint i;
  
  g_return_if_fail (n_values == mdata->n_values >> 1);
  
  if (GSL_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_LEFT).n_connections)
    src = GSL_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_LEFT, 0);
  else
    src = gsl_engine_const_values (0);
  d = mdata->buffer;
  do { *d = *src++; d += 2; } while (d < b);
  for (i = 1; i < GSL_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_LEFT).n_connections; i++)
    {
      src = GSL_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_LEFT, i);
      d = mdata->buffer;
      do { *d += *src++; d += 2; } while (d < b);
    }

  if (GSL_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_RIGHT).n_connections)
    src = GSL_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_RIGHT, 0);
  else
    src = gsl_engine_const_values (0);
  d = mdata->buffer + 1;
  do { *d = *src++; d += 2; } while (d < b);
  for (i = 1; i < GSL_MODULE_JSTREAM (module, BSE_PCM_MODULE_JSTREAM_RIGHT).n_connections; i++)
    {
      src = GSL_MODULE_JBUFFER (module, BSE_PCM_MODULE_JSTREAM_RIGHT, i);
      d = mdata->buffer + 1;
      do { *d += *src++; d += 2; } while (d < b);
    }
  
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

static GslModule*
bse_pcm_omodule_insert (BsePcmHandle *handle,
			GslTrans     *trans)
{
  static const GslClass pcm_omodule_class = {
    0,				/* n_istreams */
    BSE_PCM_MODULE_N_JSTREAMS,	/* n_jstreams */
    0,				/* n_ostreams */
    bse_pcm_omodule_process,	/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reconnect */
    bse_pcm_module_data_free,	/* free */
    GSL_COST_CHEAP,		/* cost */
  };
  BsePCMModuleData *mdata;
  GslModule *module;
  
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (handle->write != NULL, NULL);
  g_return_val_if_fail (trans != NULL, NULL);
  
  mdata = g_new (BsePCMModuleData, 1);
  mdata->n_values = gsl_engine_block_size () * BSE_PCM_MODULE_N_JSTREAMS;
  mdata->buffer = g_new (gfloat, mdata->n_values);
  mdata->bound = mdata->buffer + mdata->n_values;
  mdata->handle = handle;
  module = gsl_module_new (&pcm_omodule_class, mdata);
  
  gsl_trans_add (trans,
		 gsl_job_integrate (module));
  gsl_trans_add (trans,
		 gsl_job_set_consumer (module, TRUE));
  gsl_trans_add (trans,
		 gsl_job_add_poll (bse_pcm_module_poll, mdata, NULL, 0, NULL));
  
  return module;
}

static void
bse_pcm_omodule_remove (GslModule *pcm_module,
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

static void
bse_pcm_imodule_process (GslModule *module,
			 guint      n_values)
{
  BsePCMModuleData *mdata = module->user_data;
  gfloat *left = GSL_MODULE_OBUFFER (module, BSE_PCM_MODULE_OSTREAM_LEFT);
  gfloat *right = GSL_MODULE_OBUFFER (module, BSE_PCM_MODULE_OSTREAM_RIGHT);
  const gfloat *s = mdata->buffer;
  const gfloat *b = mdata->bound;
  gsize l;

  g_return_if_fail (n_values == mdata->n_values >> 1);
  
  l = bse_pcm_handle_read (mdata->handle, mdata->n_values, mdata->buffer);

  do
    {
      *left++ = *s++;
      *right++ = *s++;
    }
  while (s < b);
  
  g_return_if_fail (l == mdata->n_values);
}

static GslModule*
bse_pcm_imodule_insert (BsePcmHandle *handle,
			GslTrans     *trans)
{
  static const GslClass pcm_imodule_class = {
    0,				/* n_istreams */
    0,				/* n_jstreams */
    BSE_PCM_MODULE_N_OSTREAMS,	/* n_ostreams */
    bse_pcm_imodule_process,	/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reconnect */
    bse_pcm_module_data_free,	/* free */
    GSL_COST_EXPENSIVE,		/* cost */
  };
  BsePCMModuleData *mdata;
  GslModule *module;
  
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (handle->write != NULL, NULL);
  g_return_val_if_fail (trans != NULL, NULL);
  
  mdata = g_new (BsePCMModuleData, 1);
  mdata->n_values = gsl_engine_block_size () * BSE_PCM_MODULE_N_OSTREAMS;
  mdata->buffer = g_new0 (gfloat, mdata->n_values);
  mdata->bound = mdata->buffer + mdata->n_values;
  mdata->handle = handle;
  module = gsl_module_new (&pcm_imodule_class, mdata);
  
  gsl_trans_add (trans,
		 gsl_job_integrate (module));
  
  return module;
}

static void
bse_pcm_imodule_remove (GslModule *pcm_module,
			GslTrans  *trans)
{
  BsePCMModuleData *mdata;
  
  g_return_if_fail (pcm_module != NULL);
  g_return_if_fail (trans != NULL);
  
  mdata = pcm_module->user_data;
  gsl_trans_add (trans,
		 gsl_job_discard (pcm_module));
}
