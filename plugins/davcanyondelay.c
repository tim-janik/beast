/* DavCanyonDelay - DAV Canyon Delay
 * Copyright (c) 1999, 2000 David A. Bartold
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
#include "davcanyondelay.h"

#include <bse/bsechunk.h>
#include <bse/bsehunkmixer.h>

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_LEFT_TO_RIGHT_TIME,
  PARAM_LEFT_TO_RIGHT_FEEDBACK,
  PARAM_RIGHT_TO_LEFT_TIME,
  PARAM_RIGHT_TO_LEFT_FEEDBACK,
  PARAM_FILTER
};


/* --- prototypes --- */
static void        dav_canyon_delay_init             (DavCanyonDelay       *delay);
static void        dav_canyon_delay_class_init       (DavCanyonDelayClass  *class);
static void        dav_canyon_delay_class_destroy    (DavCanyonDelayClass  *class);
static void        dav_canyon_delay_do_shutdown      (BseObject            *object);
static void        dav_canyon_delay_set_param        (DavCanyonDelay       *delay,
						      BseParam             *param,
						      guint                 param_id);
static void        dav_canyon_delay_get_param        (DavCanyonDelay       *delay,
						      BseParam             *param,
						      guint                 param_id);
static void        dav_canyon_delay_prepare          (BseSource            *source,
						      BseIndex              index);
static BseChunk*   dav_canyon_delay_calc_chunk       (BseSource            *source,
						      guint                 ochannel_id);
static void        dav_canyon_delay_reset            (BseSource            *source);
static inline void dav_canyon_delay_update_locals    (DavCanyonDelay       *delay);


/* --- variables --- */
static GType             type_id_canyon_delay = 0;
static gpointer          parent_class = NULL;
static const GTypeInfo type_info_canyon_delay = {
  sizeof (DavCanyonDelayClass),
  
  (GBaseInitFunc) NULL,
  (GBaseDestroyFunc) NULL,
  (GClassInitFunc) dav_canyon_delay_class_init,
  (GClassDestroyFunc) dav_canyon_delay_class_destroy,
  NULL /* class_data */,
  
  sizeof (DavCanyonDelay),
  0 /* n_preallocs */,
  (GInstanceInitFunc) dav_canyon_delay_init,
};


/* --- functions --- */

/* Calculate the half life rate given:
 *  half - the length of the half life
 *  rate - time divisor (usually the # calcs per second)
 *
 * Basically, find r given 1/2 = e^(-r*(half/rate))
 */
static inline gfloat
calc_exponent (gfloat half, gfloat rate)
{
  /* ln (1 / 2) = ~-0.69314718056 */
  return exp (-0.69314718056 / (half * rate));
}

static inline void
dav_canyon_delay_update_locals (DavCanyonDelay *delay)
{
  delay->l_to_r_mag = delay->l_to_r_feedback / 100.0 * 128.0;
  delay->l_to_r_invmag = 128 - abs (delay->l_to_r_mag);

  delay->r_to_l_mag = delay->r_to_l_feedback / 100.0 * 128.0;
  delay->r_to_l_invmag = 128 - abs (delay->r_to_l_mag);

  delay->l_to_r_pos = delay->l_to_r_seconds * BSE_MIX_FREQ;
  delay->r_to_l_pos = delay->r_to_l_seconds * BSE_MIX_FREQ;

  /* The stuff inside the calc_exponent call (except the multiplicative  */
  /* inverse) is a guesstimate.  The calculations seem to be right, tho. */
  /* Compare to the FIR filter for a reference.                          */

  delay->filter_invmag = calc_exponent (1.0 / (4.0 * PI * delay->filter_freq), BSE_MIX_FREQ) * 128.0;
  delay->filter_mag = 128 - delay->filter_invmag;
}

static void
dav_canyon_delay_class_init (DavCanyonDelayClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ochannel_id, ichannel_id;

  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);

  object_class->set_param = (BseObjectSetParamFunc) dav_canyon_delay_set_param;
  object_class->get_param = (BseObjectGetParamFunc) dav_canyon_delay_get_param;
  object_class->shutdown = dav_canyon_delay_do_shutdown;

  source_class->prepare = dav_canyon_delay_prepare;
  source_class->calc_chunk = dav_canyon_delay_calc_chunk;
  source_class->reset = dav_canyon_delay_reset;

  bse_object_class_add_param (object_class, "Left to Right", PARAM_LEFT_TO_RIGHT_TIME,
                              bse_param_spec_float ("left_to_right_time", "Delay (seconds)",
                                                   "Set the time for the left to right delay",
                                                    0.01, 0.99, 0.01, 0.09,
                                                    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));

  bse_object_class_add_param (object_class, "Left to Right", PARAM_LEFT_TO_RIGHT_FEEDBACK,
                              bse_param_spec_float ("left_to_right_feedback", "Feedback [%]",
                                                   "Set the feedback amount; a negative feedback inverts the signal",
                                                    -100.0, 100.0, 0.01, 60.0,
                                                    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));

  bse_object_class_add_param (object_class, "Right to Left", PARAM_RIGHT_TO_LEFT_TIME,
                              bse_param_spec_float ("right_to_left_time", "Delay (seconds)",
                                                    "Set the time for the right to left delay",
                                                    0.01, 0.99, 0.01, 0.26,
                                                    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));

  bse_object_class_add_param (object_class, "Right to Left", PARAM_RIGHT_TO_LEFT_FEEDBACK,
                              bse_param_spec_float ("right_to_left_feedback", "Feedback [%]",
                                                    "Set the feedback amount; a negative feedback inverts the signal",
                                                    -100.0, 100.0, 0.01, -70.0,
                                                    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));

  bse_object_class_add_param (object_class, "IIR Low-Pass Filter", PARAM_FILTER,
                              bse_param_spec_float ("base_freq", "Frequency",
                                                    "Set the filter frequency",
                                                    1.0, 5000.0, 1.0, 1000.0,
                                                    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_SCALE));

  ochannel_id = bse_source_class_add_ochannel (source_class, "stereo_out", "CanyonDelay Output", 2);
  g_assert (ochannel_id == DAV_CANYON_DELAY_OCHANNEL_STEREO);

  ichannel_id = bse_source_class_add_ichannel (source_class, "multi_in", "Sound Input", 1, 2);
  g_assert (ichannel_id == DAV_CANYON_DELAY_ICHANNEL_MULTI);
}

static void
dav_canyon_delay_class_destroy (DavCanyonDelayClass *class)
{
}

static void
dav_canyon_delay_init (DavCanyonDelay *delay)
{
  delay->data_l = NULL;
  delay->data_r = NULL;

  delay->r_to_l_seconds = 0.26;
  delay->l_to_r_seconds = 0.09;

  delay->l_to_r_feedback = 60.0;
  delay->r_to_l_feedback = -70.0;

  delay->filter_freq = 1000.0;

  dav_canyon_delay_update_locals (delay);
}

static void
dav_canyon_delay_do_shutdown (BseObject *object)
{
  DavCanyonDelay *delay;
 
  delay = DAV_CANYON_DELAY (object);

  if (delay->data_l != NULL)
    g_free (delay->data_l);
  delay->data_l = NULL;

  if (delay->data_r != NULL)
    g_free (delay->data_r);
  delay->data_r = NULL;

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
dav_canyon_delay_set_param (DavCanyonDelay *delay,
			    BseParam       *param,
			    guint           param_id)
{
  switch (param_id)
    {
    case PARAM_LEFT_TO_RIGHT_TIME:
      delay->l_to_r_seconds = param->value.v_float;
      dav_canyon_delay_update_locals (delay);
      break;

    case PARAM_LEFT_TO_RIGHT_FEEDBACK:
      delay->l_to_r_feedback = param->value.v_float;
      dav_canyon_delay_update_locals (delay);
      break;

    case PARAM_RIGHT_TO_LEFT_TIME:
      delay->r_to_l_seconds = param->value.v_float;
      dav_canyon_delay_update_locals (delay);
      break;

    case PARAM_RIGHT_TO_LEFT_FEEDBACK:
      delay->r_to_l_feedback = param->value.v_float;
      dav_canyon_delay_update_locals (delay);
      break;

    case PARAM_FILTER:
      delay->filter_freq = param->value.v_float;
      dav_canyon_delay_update_locals (delay);
      break;

    default:
      BSE_UNHANDLED_PARAM_ID (delay, param, param_id);
      break;
    }
}

static void
dav_canyon_delay_get_param (DavCanyonDelay *delay,
			    BseParam       *param,
			    guint           param_id)
{
  switch (param_id)
    {
    case PARAM_LEFT_TO_RIGHT_TIME:
      param->value.v_float = delay->l_to_r_seconds;
      break;

    case PARAM_LEFT_TO_RIGHT_FEEDBACK:
      param->value.v_float = delay->l_to_r_feedback;
      break;

    case PARAM_RIGHT_TO_LEFT_TIME:
      param->value.v_float = delay->r_to_l_seconds;
      break;

    case PARAM_RIGHT_TO_LEFT_FEEDBACK:
      param->value.v_float = delay->r_to_l_feedback;
      break;

    case PARAM_FILTER:
      param->value.v_float = delay->filter_freq;
      break;

    default:
      BSE_UNHANDLED_PARAM_ID (delay, param, param_id);
      break;
    }
}

static void
dav_canyon_delay_prepare (BseSource *source,
			  BseIndex   index)
{
  gint i;
  DavCanyonDelay *delay = DAV_CANYON_DELAY (source);

  dav_canyon_delay_update_locals (delay);

  /* Free tables */
  if (delay->data_l != NULL)
    g_free (delay->data_l);

  if (delay->data_r != NULL)
    g_free (delay->data_r);

  delay->datasize = BSE_MIX_FREQ;
  delay->data_l = g_new (BseSampleValue, delay->datasize);
  delay->data_r = g_new (BseSampleValue, delay->datasize);
  delay->accum_l = 0;
  delay->accum_r = 0;

  delay->pos = 0;

  for (i = 0; i < delay->datasize; i++)
    {
      delay->data_l[i] = 0;
      delay->data_r[i] = 0;
    }
 
  dav_canyon_delay_update_locals (delay);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
dav_canyon_delay_calc_chunk (BseSource *source,
			     guint      ochannel_id)
{
  DavCanyonDelay *delay = DAV_CANYON_DELAY (source);
  BseSampleValue *hunk;
  BseSourceInput *input;
  BseChunk *input_chunk;
  BseSampleValue *inputs;
  gint i;
  gint pos1, pos2;
  BseMixValue accum_l, accum_r;

  g_return_val_if_fail (ochannel_id == DAV_CANYON_DELAY_OCHANNEL_STEREO, NULL);

  hunk = bse_hunk_alloc (2);
  input = bse_source_get_input (source, DAV_CANYON_DELAY_ICHANNEL_MULTI);

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
  
  for (i = 0; i < BSE_TRACK_LENGTH; i++)
    {
      if (inputs == NULL)  
        {
          accum_l = 0;
          accum_r = 0;
        }
      else if (input_chunk->n_tracks == 1)
        {
          accum_l = inputs[i];
          accum_r = inputs[i];
        }
      else
        {
          accum_l = inputs[i << 1];
          accum_r = inputs[(i << 1) + 1];
        }

      pos1 = delay->pos - delay->r_to_l_pos + delay->datasize;
      while (pos1 >= delay->datasize)
        pos1 -= delay->datasize;

      pos2 = delay->pos - delay->l_to_r_pos + delay->datasize;
      while (pos2 >= delay->datasize)
        pos2 -= delay->datasize;

      /* Mix channels with past samples. */
      accum_l = (accum_l * delay->r_to_l_invmag + ((BseMixValue) delay->data_r[pos1]) * delay->r_to_l_mag) >> 7;
      accum_r = (accum_r * delay->l_to_r_invmag + ((BseMixValue) delay->data_l[pos2]) * delay->l_to_r_mag) >> 7;

      /* Low-pass filter output. */
      accum_l = BSE_CLIP_SAMPLE_VALUE ((((BseMixValue) delay->accum_l) * delay->filter_invmag + accum_l * delay->filter_mag) >> 7);
      accum_r = BSE_CLIP_SAMPLE_VALUE ((((BseMixValue) delay->accum_r) * delay->filter_invmag + accum_r * delay->filter_mag) >> 7);

      /* Store IIR samples. */
      delay->accum_l = accum_l;
      delay->accum_r = accum_r;

      /* Store samples in arrays. */
      delay->data_l[delay->pos] = accum_l;
      delay->data_r[delay->pos] = accum_r;

      /* Write output. */
      hunk [(i << 1)] = accum_l;
      hunk [(i << 1) + 1] = accum_r;

      delay->pos++;
      if (delay->pos >= delay->datasize)
        delay->pos -= delay->datasize;
    }
  
  if (input_chunk != NULL)
    bse_chunk_unref (input_chunk);

  return bse_chunk_new_orphan (2, hunk);
}

static void
dav_canyon_delay_reset (BseSource *source)
{
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}

/* --- Export to DAV --- */
#include "./icons/canyon.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_canyon_delay, "DavCanyonDelay", "BseSource",
    "DavCanyonDelay is a deep, long delay",
    &type_info_canyon_delay,
    "/Source/CanyonDelay",
    { CANYON_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      CANYON_IMAGE_WIDTH, CANYON_IMAGE_HEIGHT,
      CANYON_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
