/* BseMixer - BSE Mixer
 * Copyright (C) 1999, 2000 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsemixer.h"

#include <bse/bsechunk.h>
#include <bse/bsehunkmixer.h>


#define	BSE_DFL_MIXER_VOLUME_dB	(BSE_DFL_MASTER_VOLUME_dB)

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_MVOLUME_f,
  PARAM_MVOLUME_dB,
  PARAM_MVOLUME_PERC,
  PARAM_VOLUME_f1,
  PARAM_VOLUME_dB1,
  PARAM_VOLUME_PERC1,
  PARAM_VOLUME_f2,
  PARAM_VOLUME_dB2,
  PARAM_VOLUME_PERC2,
  PARAM_VOLUME_f3,
  PARAM_VOLUME_dB3,
  PARAM_VOLUME_PERC3,
  PARAM_VOLUME_f4,
  PARAM_VOLUME_dB4,
  PARAM_VOLUME_PERC4
};


/* --- prototypes --- */
static void	 bse_mixer_init			(BseMixer	*mixer);
static void	 bse_mixer_class_init		(BseMixerClass	*class);
static void	 bse_mixer_class_destroy	(BseMixerClass	*class);
static void      bse_mixer_set_param            (BseMixer	*mixer,
						 BseParam       *param,
						 guint           param_id);
static void      bse_mixer_get_param            (BseMixer	*mixer,
						 BseParam       *param,
						 guint           param_id);
static void	 bse_mixer_do_shutdown		(BseObject     	*object);
static void      bse_mixer_prepare               (BseSource      *source,
						 BseIndex        index);
static BseChunk* bse_mixer_calc_chunk            (BseSource      *source,
						 guint           ochannel_id);
static void      bse_mixer_reset                 (BseSource      *source);


/* --- variables --- */
static GType             type_id_mixer = 0;
static gpointer          parent_class = NULL;
static const GTypeInfo type_info_mixer = {
  sizeof (BseMixerClass),
  
  (GBaseInitFunc) NULL,
  (GBaseDestroyFunc) NULL,
  (GClassInitFunc) bse_mixer_class_init,
  (GClassDestroyFunc) bse_mixer_class_destroy,
  NULL /* class_data */,
  
  sizeof (BseMixer),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_mixer_init,
};


/* --- functions --- */
static void
bse_mixer_class_init (BseMixerClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ichannel_id, ochannel_id;
    
  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);
  
  object_class->set_param = (BseObjectSetParamFunc) bse_mixer_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_mixer_get_param;
  object_class->shutdown = bse_mixer_do_shutdown;
  
  source_class->prepare = bse_mixer_prepare;
  source_class->calc_chunk = bse_mixer_calc_chunk;
  source_class->reset = bse_mixer_reset;

  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_f,
			      bse_param_spec_float ("master_volume_f", "Master [float]", NULL,
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    0.1,
						    bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB),
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_dB,
			      bse_param_spec_float ("master_volume_dB", "Master [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_STP_VOLUME_dB,
						    BSE_DFL_MASTER_VOLUME_dB,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Adjustments",
			      PARAM_MVOLUME_PERC,
			      bse_param_spec_uint ("master_volume_perc", "Master [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   1,
						   bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB) * 100,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Channel1",
			      PARAM_VOLUME_f1,
			      bse_param_spec_float ("volume_f1", "Channel1 [float]", NULL,
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    0.1,
						    bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB),
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Channel1",
			      PARAM_VOLUME_dB1,
			      bse_param_spec_float ("volume_dB1", "Channel1 [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_STP_VOLUME_dB,
						    BSE_DFL_MIXER_VOLUME_dB,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Channel1",
			      PARAM_VOLUME_PERC1,
			      bse_param_spec_uint ("volume_perc1", "Channel1 [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   1,
						   bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB) * 100,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Channel2",
			      PARAM_VOLUME_f2,
			      bse_param_spec_float ("volume_f2", "Channel2 [float]", NULL,
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    0.1,
						    bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB),
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Channel2",
			      PARAM_VOLUME_dB2,
			      bse_param_spec_float ("volume_dB2", "Channel2 [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_STP_VOLUME_dB,
						    BSE_DFL_MIXER_VOLUME_dB,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Channel2",
			      PARAM_VOLUME_PERC2,
			      bse_param_spec_uint ("volume_perc2", "Channel2 [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   1,
						   bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB) * 100,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Channel3",
			      PARAM_VOLUME_f3,
			      bse_param_spec_float ("volume_f3", "Channel3 [float]", NULL,
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    0.1,
						    bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB),
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Channel3",
			      PARAM_VOLUME_dB3,
			      bse_param_spec_float ("volume_dB3", "Channel3 [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_STP_VOLUME_dB,
						    BSE_DFL_MIXER_VOLUME_dB,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Channel3",
			      PARAM_VOLUME_PERC3,
			      bse_param_spec_uint ("volume_perc3", "Channel3 [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   1,
						   bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB) * 100,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Channel4",
			      PARAM_VOLUME_f4,
			      bse_param_spec_float ("volume_f4", "Channel4 [float]", NULL,
						    0, bse_dB_to_factor (BSE_MAX_VOLUME_dB),
						    0.1,
						    bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB),
						    BSE_PARAM_STORAGE));
  bse_object_class_add_param (object_class, "Channel4",
			      PARAM_VOLUME_dB4,
			      bse_param_spec_float ("volume_dB4", "Channel4 [dB]", NULL,
						    BSE_MIN_VOLUME_dB, BSE_MAX_VOLUME_dB,
						    BSE_STP_VOLUME_dB,
						    BSE_DFL_MIXER_VOLUME_dB,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Channel4",
			      PARAM_VOLUME_PERC4,
			      bse_param_spec_uint ("volume_perc4", "Channel4 [%]", NULL,
						   0, bse_dB_to_factor (BSE_MAX_VOLUME_dB) * 100,
						   1,
						   bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB) * 100,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));

  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in1", "Mono Input 1", 1, 1);
  g_assert (ichannel_id == BSE_MIXER_ICHANNEL_MONO1);
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in2", "Mono Input 2", 1, 1);
  g_assert (ichannel_id == BSE_MIXER_ICHANNEL_MONO2);
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in3", "Mono Input 3", 1, 1);
  g_assert (ichannel_id == BSE_MIXER_ICHANNEL_MONO3);
  ichannel_id = bse_source_class_add_ichannel (source_class, "mono_in4", "Mono Input 4", 1, 1);
  g_assert (ichannel_id == BSE_MIXER_ICHANNEL_MONO4);
  ochannel_id = bse_source_class_add_ochannel (source_class, "mono_out", "Mono Output", 1);
  g_assert (ochannel_id == BSE_MIXER_OCHANNEL_MONO);
}

static void
bse_mixer_class_destroy (BseMixerClass *class)
{
}

static void
bse_mixer_init (BseMixer *mixer)
{
  mixer->volume_factor[0] = bse_dB_to_factor (BSE_DFL_MASTER_VOLUME_dB);
  mixer->volume_factor[1] = bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB);
  mixer->volume_factor[2] = bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB);
  mixer->volume_factor[3] = bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB);
  mixer->volume_factor[4] = bse_dB_to_factor (BSE_DFL_MIXER_VOLUME_dB);
  mixer->mix_buffer = NULL;
}

static void
bse_mixer_do_shutdown (BseObject *object)
{
  BseMixer *mixer;
  
  mixer = BSE_MIXER (object);
  
  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
bse_mixer_set_param (BseMixer *mixer,
                     BseParam *param,
		     guint     param_id)
{
  switch (param_id)
    {
    case PARAM_MVOLUME_f:
      mixer->volume_factor[0] = param->value.v_float;
      bse_object_param_changed (BSE_OBJECT (mixer), "master_volume_dB");
      bse_object_param_changed (BSE_OBJECT (mixer), "master_volume_perc");
      break;
    case PARAM_MVOLUME_dB:
      mixer->volume_factor[0] = bse_dB_to_factor (param->value.v_float);
      bse_object_param_changed (BSE_OBJECT (mixer), "master_volume_f");
      bse_object_param_changed (BSE_OBJECT (mixer), "master_volume_perc");
      break;
    case PARAM_MVOLUME_PERC:
      mixer->volume_factor[0] = param->value.v_uint / 100.0;
      bse_object_param_changed (BSE_OBJECT (mixer), "master_volume_f");
      bse_object_param_changed (BSE_OBJECT (mixer), "master_volume_dB");
      break;
    case PARAM_VOLUME_f1:
      mixer->volume_factor[1] = param->value.v_float;
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_dB1");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_perc1");
      break;
    case PARAM_VOLUME_dB1:
      mixer->volume_factor[1] = bse_dB_to_factor (param->value.v_float);
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_f1");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_perc1");
      break;
    case PARAM_VOLUME_PERC1:
      mixer->volume_factor[1] = param->value.v_uint / 100.0;
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_f1");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_dB1");
      break;
    case PARAM_VOLUME_f2:
      mixer->volume_factor[2] = param->value.v_float;
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_dB2");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_perc2");
      break;
    case PARAM_VOLUME_dB2:
      mixer->volume_factor[2] = bse_dB_to_factor (param->value.v_float);
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_f2");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_perc2");
      break;
    case PARAM_VOLUME_PERC2:
      mixer->volume_factor[2] = param->value.v_uint / 100.0;
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_f2");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_dB2");
      break;
    case PARAM_VOLUME_f3:
      mixer->volume_factor[3] = param->value.v_float;
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_dB3");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_perc3");
      break;
    case PARAM_VOLUME_dB3:
      mixer->volume_factor[3] = bse_dB_to_factor (param->value.v_float);
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_f3");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_perc3");
      break;
    case PARAM_VOLUME_PERC3:
      mixer->volume_factor[3] = param->value.v_uint / 100.0;
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_f3");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_dB3");
      break;
    case PARAM_VOLUME_f4:
      mixer->volume_factor[4] = param->value.v_float;
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_dB4");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_perc4");
      break;
    case PARAM_VOLUME_dB4:
      mixer->volume_factor[4] = bse_dB_to_factor (param->value.v_float);
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_f4");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_perc4");
      break;
    case PARAM_VOLUME_PERC4:
      mixer->volume_factor[4] = param->value.v_uint / 100.0;
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_f4");
      bse_object_param_changed (BSE_OBJECT (mixer), "volume_dB4");
      break;
    default:
      BSE_UNHANDLED_PARAM_ID (mixer, param, param_id);
      break;
    }
}

static void
bse_mixer_get_param (BseMixer *mixer,
                     BseParam *param,
		     guint     param_id)
{
  switch (param_id)
    {
    case PARAM_MVOLUME_f:
      param->value.v_float = mixer->volume_factor[0];
      break;
    case PARAM_MVOLUME_dB:
      param->value.v_float = bse_dB_from_factor (mixer->volume_factor[0], BSE_MIN_VOLUME_dB);
      break;
    case PARAM_MVOLUME_PERC:
      param->value.v_uint = mixer->volume_factor[0] * 100.0 + 0.5;
      break;
    case PARAM_VOLUME_f1:
      param->value.v_float = mixer->volume_factor[1];
      break;
    case PARAM_VOLUME_dB1:
      param->value.v_float = bse_dB_from_factor (mixer->volume_factor[1], BSE_MIN_VOLUME_dB);
      break;
    case PARAM_VOLUME_PERC1:
      param->value.v_uint = mixer->volume_factor[1] * 100.0 + 0.5;
      break;
    case PARAM_VOLUME_f2:
      param->value.v_float = mixer->volume_factor[2];
      break;
    case PARAM_VOLUME_dB2:
      param->value.v_float = bse_dB_from_factor (mixer->volume_factor[2], BSE_MIN_VOLUME_dB);
      break;
    case PARAM_VOLUME_PERC2:
      param->value.v_uint = mixer->volume_factor[2] * 100.0 + 0.5;
      break;
    case PARAM_VOLUME_f3:
      param->value.v_float = mixer->volume_factor[3];
      break;
    case PARAM_VOLUME_dB3:
      param->value.v_float = bse_dB_from_factor (mixer->volume_factor[3], BSE_MIN_VOLUME_dB);
      break;
    case PARAM_VOLUME_PERC3:
      param->value.v_uint = mixer->volume_factor[3] * 100.0 + 0.5;
      break;
    case PARAM_VOLUME_f4:
      param->value.v_float = mixer->volume_factor[4];
      break;
    case PARAM_VOLUME_dB4:
      param->value.v_float = bse_dB_from_factor (mixer->volume_factor[4], BSE_MIN_VOLUME_dB);
      break;
    case PARAM_VOLUME_PERC4:
      param->value.v_uint = mixer->volume_factor[4] * 100.0 + 0.5;
      break;
    default:
      BSE_UNHANDLED_PARAM_ID (mixer, param, param_id);
      break;
    }
}

static void
bse_mixer_prepare (BseSource *source,
		  BseIndex   index)
{
  BseMixer *mixer = BSE_MIXER (source);

  mixer->mix_buffer = g_new (BseMixValue, BSE_TRACK_LENGTH);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
bse_mixer_calc_chunk (BseSource *source,
		     guint      ochannel_id)
{
  BseMixer *mixer = BSE_MIXER (source);
  BseMixValue *mv, *bound;
  BseSampleValue *hunk;
  guint c;
  
  g_return_val_if_fail (ochannel_id == BSE_MIXER_OCHANNEL_MONO, NULL);

  if (source->n_inputs == 0)
    return bse_chunk_new_static_zero (1);
  else if (source->n_inputs == 1 &&
	   BSE_EPSILON_CMP (1.0,
			    mixer->volume_factor[0] *
			    mixer->volume_factor[source->inputs[0].ichannel_id]) == 0)
    return bse_source_ref_chunk (source->inputs[0].osource, source->inputs[0].ochannel_id, source->index);

  bound = mixer->mix_buffer + BSE_TRACK_LENGTH;

  /* fill mix buffer with the first input channel */
  for (c = BSE_MIXER_ICHANNEL_MONO1; c <= BSE_MIXER_ICHANNEL_MONO4; c++)
    {
      BseSourceInput *input = bse_source_get_input (source, c);

      if (input)
	{
	  BseChunk *chunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);
	  BseSampleValue *s = bse_chunk_complete_hunk (chunk);
	  gfloat volume_factor = mixer->volume_factor[c];

	  mv = mixer->mix_buffer;

	  /* don't do the volume multiplication for factors of 1.0 */
	  if (BSE_EPSILON_CMP (1.0, volume_factor) != 0)
	    do
	      *(mv++) = *(s++) * volume_factor;
	    while (mv < bound);
	  else
	    do
	      *(mv++) = *(s++);
	    while (mv < bound);
	    
	  bse_chunk_unref (chunk);
	  break;
	}
    }

  /* add up remaining input channels */
  for (c = c + 1; c <= BSE_MIXER_ICHANNEL_MONO4; c++)
    {
      BseSourceInput *input = bse_source_get_input (source, c);
      
      if (input)
	{
	  BseChunk *chunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);
	  BseSampleValue *s = bse_chunk_complete_hunk (chunk);
          gfloat volume_factor = mixer->volume_factor[c];
	  
	  mv = mixer->mix_buffer;
	  
	  /* don't do the volume multiplication for factors of 1.0 */
	  if (BSE_EPSILON_CMP (1.0, volume_factor) != 0)
	    do
	      *(mv++) += *(s++) * volume_factor;
	    while (mv < bound);
	  else
	    do
	      *(mv++) += *(s++);
	    while (mv < bound);
	  
	  bse_chunk_unref (chunk);
	}
    }

  /* clip the mix buffer to output hunk */
  hunk = bse_hunk_alloc (1);
  bse_hunk_clip_from_mix_buffer (1, hunk, mixer->volume_factor[0], mixer->mix_buffer);

  return bse_chunk_new_orphan (1, hunk);
}

static void
bse_mixer_reset (BseSource *source)
{
  BseMixer *mixer = BSE_MIXER (source);
  
  g_free (mixer->mix_buffer);
  mixer->mix_buffer = NULL;

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}


/* --- Export to BSE --- */
#include "./icons/ampmix.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_mixer, "BseMixer", "BseSource",
    "BseMixer is a channel mixer to sum up incomiong signals",
    &type_info_mixer,
    "/Source/Mixer",
    { AMP_MIX_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      AMP_MIX_IMAGE_WIDTH, AMP_MIX_IMAGE_HEIGHT,
      AMP_MIX_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
