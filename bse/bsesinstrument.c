/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bsesinstrument.h"

#include	"bsechunk.h"
#include	"bsecategories.h"
#include	"bsehunkmixer.h"
#include	"bsesong.h"

#include	"./icons/sinstrument.c"


enum {
  PARAM_0,
  PARAM_INSTRUMENT
};


/* --- prototypes --- */
static void	 bse_sinstrument_init		(BseSInstrument	     *sinstrument);
static void	 bse_sinstrument_class_init	(BseSInstrumentClass *class);
static void	 bse_sinstrument_class_destroy	(BseSInstrumentClass *class);
static void	 bse_sinstrument_do_shutdown	(BseObject     	     *object);
static void      bse_sinstrument_set_param      (BseSInstrument      *sinstrument,
						 BseParam            *param);
static void      bse_sinstrument_get_param      (BseSInstrument      *sinstrument,
						 BseParam            *param);
static void      bse_sinstrument_prepare        (BseSource           *source,
						 BseIndex             index);
static BseChunk* bse_sinstrument_calc_chunk     (BseSource           *source,
						 guint                ochannel_id);
static void      bse_sinstrument_reset          (BseSource           *source);


/* --- variables --- */
static gpointer  parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSInstrument)
{
  BseType sinstrument_type;
  
  static const BseTypeInfo sinstrument_info = {
    sizeof (BseSInstrumentClass),
    
    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_sinstrument_class_init,
    (BseClassDestroyFunc) bse_sinstrument_class_destroy,
    NULL /* class_data */,
    
    sizeof (BseSInstrument),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_sinstrument_init,
  };
  static const BsePixdata sinstrument_pixdata = {
    SINSTRUMENT_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    SINSTRUMENT_IMAGE_WIDTH, SINSTRUMENT_IMAGE_HEIGHT,
    SINSTRUMENT_IMAGE_RLE_PIXEL_DATA,
  };
  
  sinstrument_type = bse_type_register_static (BSE_TYPE_SOURCE,
					       "BseSInstrument",
					       "BSE Synthesis network Instrument",
					       &sinstrument_info);
  bse_categories_register_icon ("/Source/SInstrument", sinstrument_type, &sinstrument_pixdata);
  
  return sinstrument_type;
}

static void
bse_sinstrument_class_init (BseSInstrumentClass *class)
{
  BseObjectClass *object_class;
  BseSourceClass *source_class;
  guint ichannel_id, ochannel_id;

  parent_class = bse_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);

  object_class->set_param = (BseObjectSetParamFunc) bse_sinstrument_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_sinstrument_get_param;
  object_class->shutdown = bse_sinstrument_do_shutdown;

  source_class->prepare = bse_sinstrument_prepare;
  source_class->calc_chunk = bse_sinstrument_calc_chunk;
  source_class->reset = bse_sinstrument_reset;
  
  bse_object_class_add_param (object_class, NULL,
			      PARAM_INSTRUMENT,
			      bse_param_spec_item ("instrument", "Instrument", NULL,
						   BSE_TYPE_INSTRUMENT,
						   BSE_PARAM_GUI | BSE_PARAM_HINT_RDONLY));

  ichannel_id = bse_source_class_add_ichannel (source_class, "multi_in", "Multi Track Instrument Input", 1, 2);
  g_assert (ichannel_id == BSE_SINSTRUMENT_ICHANNEL_MULTI);
  ochannel_id = bse_source_class_add_ochannel (source_class, "stereo_out", "Stereo Out", 2);
  BSE_SOURCE_CLASS_OCHANNEL_DEF (source_class, ochannel_id)->min_history = 2;
  g_assert (ochannel_id == BSE_SINSTRUMENT_OCHANNEL_STEREO);
  ochannel_id = bse_source_class_add_ochannel (source_class, "freq_out", "Instrument Frequency Output", 1);
  g_assert (ochannel_id == BSE_SINSTRUMENT_OCHANNEL_FREQ);
}

static void
bse_sinstrument_class_destroy (BseSInstrumentClass *class)
{
}

static void
bse_sinstrument_init (BseSInstrument *sinstrument)
{
  sinstrument->instrument = NULL;
  sinstrument->voice = NULL;
}

static void
bse_sinstrument_set_param (BseSInstrument *sinstrument,
			   BseParam       *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_INSTRUMENT:
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to set parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (sinstrument),
		 BSE_OBJECT_NAME (sinstrument),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
bse_sinstrument_get_param (BseSInstrument *sinstrument,
			   BseParam       *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_INSTRUMENT:
      if (param->value.v_item)
	bse_object_unref (BSE_OBJECT (param->value.v_item));
      param->value.v_item = (BseItem*) sinstrument->instrument;
      if (param->value.v_item)
	bse_object_ref (BSE_OBJECT (param->value.v_item));
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to get parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (sinstrument),
		 BSE_OBJECT_NAME (sinstrument),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
bse_sinstrument_do_shutdown (BseObject *object)
{
  BseSInstrument *sinstrument = BSE_SINSTRUMENT (object);

  if (sinstrument->instrument)
    {
      g_warning ("stale BseInstrument link in BseSInstrument");
      bse_instrument_set_sinstrument (sinstrument->instrument, NULL);
    }

  if (sinstrument->voice)
    g_error ("stale BseVoice link in BseSInstrument");   // FIXME: paranoid

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
instrument_changed (BseSInstrument *sinstrument)
{
  bse_object_param_changed (BSE_OBJECT (sinstrument), "instrument");
}

void
bse_sinstrument_poke_foreigns (BseSInstrument *sinstrument,
			       BseInstrument  *instrument,
			       BseVoice       *voice)
{
  g_return_if_fail (BSE_IS_SINSTRUMENT (sinstrument));
  if (voice && sinstrument->voice != voice)
    {
      g_return_if_fail (sinstrument->voice == NULL);
      g_return_if_fail (voice->input_type == BSE_VOICE_INPUT_SYNTH);
    }

  sinstrument->voice = voice;
  if (sinstrument->instrument != instrument)
    {
      if (sinstrument->instrument)
	bse_object_remove_notifiers_by_func (sinstrument->instrument,
					     instrument_changed,
					     sinstrument);
      sinstrument->instrument = instrument;
      if (sinstrument->instrument)
	bse_object_add_data_notifier (sinstrument->instrument,
				      "name_set",
				      instrument_changed,
				      sinstrument);
      instrument_changed (sinstrument);
    }
}

static void
bse_sinstrument_prepare (BseSource *source,
			 BseIndex   index)
{
  BseSInstrument *sinstrument;

  sinstrument = BSE_SINSTRUMENT (source);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source, index);
}

static BseChunk*
bse_sinstrument_calc_chunk (BseSource *source,
			    guint      ochannel_id)
{
  BseSInstrument *sinstrument = BSE_SINSTRUMENT (source);
  
  if (ochannel_id == BSE_SINSTRUMENT_OCHANNEL_STEREO)
    {
      BseSourceInput *input = bse_source_get_input (source, BSE_SINSTRUMENT_ICHANNEL_MULTI);
      BseChunk *ichunk;
      BseSampleValue *hunk;

      if (!input) /* silence */
	return bse_chunk_new_static_zero (2);

      ichunk = bse_source_ref_chunk (input->osource, input->ochannel_id, source->index);
      if (ichunk->n_tracks == 2) /* stereo already */
	return ichunk;

      /* ok, mix input (mono or stereo) to stereo */
      hunk = bse_hunk_alloc (2);
      bse_hunk_mix (2, hunk, NULL, ichunk->n_tracks, ichunk->hunk);
      bse_chunk_unref (ichunk);

      return bse_chunk_new_orphan (2, hunk);
    }
  else if (ochannel_id == BSE_SINSTRUMENT_OCHANNEL_FREQ)
    {
      BseSampleValue state;
      gfloat freq;

      /* sync song sequencer, so the note freq is up to date */
      if (sinstrument->instrument)
	{
	  BseItem *item = BSE_ITEM (sinstrument->instrument)->parent;

	  if (BSE_IS_SONG (item))
	    bse_song_update_sequencer (BSE_SONG (item));
	}

      freq = sinstrument->voice ? sinstrument->voice->input.synth.freq : BSE_KAMMER_FREQ_f;
      state = freq;
      
      return bse_chunk_new_from_state (1, &state);
    }
  else
    {
      g_assert_not_reached ();
      return NULL;
    }
}

static void
bse_sinstrument_reset (BseSource *source)
{
  BseSInstrument *sinstrument = BSE_SINSTRUMENT (source);

  if (sinstrument->voice)
    g_warning (G_STRLOC ": stale BseVoice link in BseSInstrument");   // FIXME

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
