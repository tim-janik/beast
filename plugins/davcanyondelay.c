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
  PARAM_BASE_FREQ,
  PARAM_BASE_NOTE,
  PARAM_TRIGGER_VEL,
  PARAM_TRIGGER_HIT,
  PARAM_RES,
  PARAM_RATIO
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
static inline BseSampleValue clip                    (gint32 sample);


/* --- variables --- */
static BseType           type_id_canyon_delay = 0;
static gpointer          parent_class = NULL;
static const BseTypeInfo type_info_canyon_delay = {
  sizeof (DavCanyonDelayClass),
  
  (BseBaseInitFunc) NULL,
  (BseBaseDestroyFunc) NULL,
  (BseClassInitFunc) dav_canyon_delay_class_init,
  (BseClassDestroyFunc) dav_canyon_delay_class_destroy,
  NULL /* class_data */,
  
  sizeof (DavCanyonDelay),
  0 /* n_preallocs */,
  (BseObjectInitFunc) dav_canyon_delay_init,
};


/* --- functions --- */
static inline void
dav_canyon_delay_update_locals (DavCanyonDelay *delay)
{
}

static void
dav_canyon_delay_class_init (DavCanyonDelayClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ochannel_id, ichannel_id;

  parent_class = bse_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);

  object_class->set_param = (BseObjectSetParamFunc) dav_canyon_delay_set_param;
  object_class->get_param = (BseObjectGetParamFunc) dav_canyon_delay_get_param;
  object_class->shutdown = dav_canyon_delay_do_shutdown;

  source_class->prepare = dav_canyon_delay_prepare;
  source_class->calc_chunk = dav_canyon_delay_calc_chunk;
  source_class->reset = dav_canyon_delay_reset;

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

  dav_canyon_delay_update_locals (delay);
}

static void
dav_canyon_delay_do_shutdown (BseObject *object)
{
  DavCanyonDelay *delay;
 
  delay = DAV_CANYON_DELAY (object);

  g_free (delay->data_l);
  delay->data_l = NULL;

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

  delay->rate = BSE_MIX_FREQ;

  /* Free tables */
  g_free (delay->data_l);
  delay->data_l = NULL;

  g_free (delay->data_r);
  delay->data_r = NULL;

  delay->datasize = (12000 * delay->rate) / 44100;
  delay->data_l = g_new (BseSampleValue, delay->datasize);
  delay->data_r = g_new (BseSampleValue, delay->datasize);
  delay->sum_l = 0;
  delay->sum_r = 0;

  delay->past1 = (11309 * delay->rate) / 44100;
  delay->past2 = (4211 * delay->rate) / 44100;

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

static inline BseSampleValue
clip (gint32 sample)
{
  if (sample > BSE_MAX_SAMPLE_VALUE_d)
    return BSE_MAX_SAMPLE_VALUE_d;
  else
    if (sample < BSE_MIN_SAMPLE_VALUE_d)
      return BSE_MIN_SAMPLE_VALUE_d;

  return sample;
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
  gint32 sum_l, sum_r;

  hunk = bse_hunk_alloc (2);
  input = bse_source_get_input (source, DAV_CANYON_DELAY_ICHANNEL_MULTI);

  if (input!=NULL)
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
          delay->data_l[delay->pos] = 0;
          delay->data_r[delay->pos] = 0;
        }
      else if (input_chunk->n_tracks == 1)
        {
          delay->data_l[delay->pos] = inputs[i];
          delay->data_r[delay->pos] = inputs[i];
        }
      else
        {
          delay->data_l[delay->pos] = inputs[(i << 1)];
          delay->data_r[delay->pos] = inputs[(i << 1) + 1];
        }

      pos1 = delay->pos - delay->past1 + delay->datasize;
      while (pos1 > delay->datasize)
        pos1 -= delay->datasize;

      pos2 = delay->pos - delay->past2 + delay->datasize;
      while (pos2 > delay->datasize)
        pos2 -= delay->datasize;

      sum_l = delay->data_l[delay->pos] + ((gint32) delay->data_r[pos1]) * -7 / 10;
      sum_r = delay->data_r[delay->pos] + ((gint32) delay->data_l[pos2]) * 6 / 10;

      delay->data_l[delay->pos] = clip (sum_l);
      delay->data_r[delay->pos] = clip (sum_r);

      sum_l = (((gint32) delay->sum_l) * 7 / 10) + delay->data_l[delay->pos] * 3 / 10;
      sum_r = (((gint32) delay->sum_r) * 7 / 10) + delay->data_r[delay->pos] * 3 / 10;

      delay->sum_l = clip (sum_l);
      delay->sum_r = clip (sum_r);

      delay->data_l[delay->pos] = delay->sum_l;
      delay->data_r[delay->pos] = delay->sum_r;

      hunk [(i << 1)] = delay->sum_l;
      hunk [(i << 1) + 1] = delay->sum_r;

      delay->pos++;
      if (delay->pos > delay->datasize)
        delay->pos -= delay->datasize;
    }
  
  if (input_chunk!=NULL)
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
#include "./icons/noicon.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_canyon_delay, "DavCanyonDelay", "BseSource",
    "DavCanyonDelay is a deep, long delay",
    &type_info_canyon_delay,
    "/Source/CanyonDelay",
    { NOICON_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      NOICON_WIDTH, NOICON_HEIGHT,
      NOICON_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
