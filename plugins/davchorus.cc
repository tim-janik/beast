/* DavChorus - DAV Chorus Effect
 * Copyright (c) 2000 David A. Bartold
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
#include "davchorus.h"

#include <bse/bsechunk.h>
#include <bse/bsehunkmixer.h>


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_WET_OUT
};


/* --- prototypes --- */
static void	   dav_chorus_init		(DavChorus	 *chorus);
static void	   dav_chorus_class_init	(DavChorusClass	 *class);
static void	   dav_chorus_class_finalize	(DavChorusClass	 *class);
static void	   dav_chorus_set_param	    	(DavChorus	 *chorus,
						 guint            param_id,
						 GValue          *value,
						 GParamSpec      *pspec,
						 const gchar     *trailer);
static void	   dav_chorus_get_param	    	(DavChorus	 *chorus,
						 guint            param_id,
						 GValue          *value,
						 GParamSpec      *pspec,
						 const gchar     *trailer);
static void	   dav_chorus_prepare	    	(BseSource	 *source,
						 BseIndex	  index);
static BseChunk*   dav_chorus_calc_chunk	(BseSource	 *source,
						 guint		  ochannel_id);
static void	   dav_chorus_reset	    	(BseSource	 *source);
static inline void dav_chorus_update_locals    	(DavChorus	 *chorus);


/* --- variables --- */
static GType	       type_id_chorus = 0;
static gpointer	       parent_class = NULL;
static const GTypeInfo type_info_chorus = {
  sizeof (DavChorusClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) dav_chorus_class_init,
  (GClassFinalizeFunc) dav_chorus_class_finalize,
  NULL /* class_data */,
  
  sizeof (DavChorus),
  0 /* n_preallocs */,
  (GInstanceInitFunc) dav_chorus_init,
};


/* --- functions --- */

static inline void
dav_chorus_update_locals (DavChorus *chorus)
{
}

static void
dav_chorus_class_init (DavChorusClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id, ichannel_id;
  
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  
  gobject_class->set_param = (GObjectSetParamFunc) dav_chorus_set_param;
  gobject_class->get_param = (GObjectGetParamFunc) dav_chorus_get_param;
  
  source_class->prepare = dav_chorus_prepare;
  source_class->calc_chunk = dav_chorus_calc_chunk;
  source_class->reset = dav_chorus_reset;
  
  bse_object_class_add_param (object_class, "Parameters", PARAM_WET_OUT,
			      b_param_spec_float ("wet_out", "Wet out [%]",
						  "Set the amount of modified data to mix",
						  0.0, 100.0, 50.0, 0.1,
						  B_PARAM_DEFAULT | B_PARAM_HINT_SCALE));
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out", "Chorus Output", 1);
  g_assert (ochannel_id == DAV_CHORUS_OCHANNEL_MONO);
  
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in", "Sound Input", 1, 1);
  g_assert (ichannel_id == DAV_CHORUS_ICHANNEL_MONO);
}

static void
dav_chorus_class_finalize (DavChorusClass *class)
{
}

static void
dav_chorus_init (DavChorus *chorus)
{
  chorus->delay = NULL;
  chorus->delay_length = 0;
  chorus->delay_pos = 0;
  chorus->sine_pos = 0.0;
  chorus->sine_delta = 0.08 * 2.0 * M_PI / 44100.0;
  chorus->wet_out = 0.5;
  
  dav_chorus_update_locals (chorus);
}

static void
dav_chorus_set_param (DavChorus   *chorus,
		      guint        param_id,
		      GValue      *value,
		      GParamSpec  *pspec,
		      const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_WET_OUT:
      chorus->wet_out = b_value_get_float (value) / 100.0;
      break;
      
    default:
      G_WARN_INVALID_PARAM_ID (chorus, param_id, pspec);
      break;
    }
}

static void
dav_chorus_get_param (DavChorus   *chorus,
		      guint        param_id,
		      GValue      *value,
		      GParamSpec  *pspec,
		      const gchar *trailer)
{
  switch (param_id)
    {
    case PARAM_WET_OUT:
      b_value_set_float (value, chorus->wet_out * 100.0);
      break;
      
    default:
      G_WARN_INVALID_PARAM_ID (chorus, param_id, pspec);
      break;
    }
}

static void
dav_chorus_prepare (BseSource *source,
		    BseIndex   index)
{
  DavChorus *chorus = DAV_CHORUS (source);
  
  g_free (chorus->delay);
  
  chorus->delay_length = BSE_MIX_FREQ / 40;
  chorus->delay = g_new0 (BseSampleValue, chorus->delay_length);
  
  chorus->delay_pos = 0;
  chorus->sine_pos = 0.0;
  
  dav_chorus_update_locals (chorus);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
dav_chorus_calc_chunk (BseSource *source,
		       guint      ochannel_id)
{
  DavChorus *chorus = DAV_CHORUS (source);
  BseSampleValue *hunk;
  BseSourceInput *input;
  BseChunk *input_chunk;
  BseSampleValue *inputs;
  gint i;
  gint hi_pos, lo_pos;
  BseMixValue wet;
  BseMixValue wet_out_1024;
  
  g_return_val_if_fail (ochannel_id == DAV_CHORUS_OCHANNEL_MONO, NULL);
  
  hunk = bse_hunk_alloc (1);
  
  input = bse_source_get_input (source, DAV_CHORUS_ICHANNEL_MONO);
  
  if (input != NULL)
    {
      input_chunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);
      inputs = input_chunk->hunk;
    }
  else
    {
      input_chunk = NULL;
      inputs = NULL;
    }
  
  wet_out_1024 = (BseMixValue) (chorus->wet_out * 1024.0);
  
  for (i = 0; i < BSE_TRACK_LENGTH; i++)
    {
      if (input_chunk == NULL)
	{
          chorus->delay [chorus->delay_pos] = 0.0;
	}
      else
	{
          chorus->delay [chorus->delay_pos] = inputs[i];
	}
      
      hi_pos = chorus->delay_pos;
      lo_pos = (gint) ((sin (chorus->sine_pos) + 1.0) * (chorus->delay_length - 1) * 256.0 * 0.5);
      
      /* Normalize hi_pos and lo_pos counters. */
      hi_pos += lo_pos >> 8;
      lo_pos &= 0xff;
      
      /* Find hi_pos modulus delay_length. */
      while (hi_pos >= chorus->delay_length)
        hi_pos -= chorus->delay_length;
      
      /* Perform linear interpolation between hi_pos and hi_pos + 1. */
      wet = chorus->delay [hi_pos] * (256 - lo_pos);
      
      hi_pos++;
      if (hi_pos >= chorus->delay_length)
        hi_pos -= chorus->delay_length;
      
      wet += chorus->delay [hi_pos] * lo_pos;
      
      wet = ((wet >> 8) + chorus->delay [chorus->delay_pos]) >> 1;
      hunk[i] = (wet * wet_out_1024 + chorus->delay [chorus->delay_pos] * (1024 - wet_out_1024)) >> 10;
      
      chorus->delay_pos++;
      if (chorus->delay_pos >= chorus->delay_length)
        chorus->delay_pos = 0;
      
      chorus->sine_pos += chorus->sine_delta;
      while (chorus->sine_pos >= 2.0 * M_PI)
        chorus->sine_pos -= 2.0 * M_PI;
    }
  
  if (input_chunk != NULL)
    bse_chunk_unref (input_chunk);
  
  return bse_chunk_new_orphan (1, hunk);
}

static void
dav_chorus_reset (BseSource *source)
{
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to DAV --- */
#include "./icons/chorus.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_chorus, "DavChorus", "BseSource",
    "DavChorus adds more depth to sounds",
    &type_info_chorus,
    "/Source/Chorus",
    { CHORUS_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      CHORUS_IMAGE_WIDTH, CHORUS_IMAGE_HEIGHT,
      CHORUS_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
