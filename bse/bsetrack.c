/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bsetrack.h"

#include	"bseglobals.h"
#include	"bsesnet.h"
#include	"bsepart.h"
#include	"bsemain.h"
#include	"gslcommon.h"
#include	"bsesubsynth.h"
#include        "bsemidivoice.h"
#include	"bsecontextmerger.h"
#include        "bsemidireceiver.h"


enum {
  PARAM_0,
  PARAM_SYNTH_NET,
  PARAM_PART
};


/* --- prototypes --- */
static void	bse_track_class_init	(BseTrackClass		*class);
static void	bse_track_init		(BseTrack		*self);
static void	bse_track_destroy	(BseObject		*object);
static void	bse_track_set_property	(GObject		*object,
					 guint                   param_id,
					 const GValue           *value,
					 GParamSpec             *pspec);
static void	bse_track_get_property	(GObject		*object,
					 guint                   param_id,
					 GValue                 *value,
					 GParamSpec             *pspec);


/* --- variables --- */
static GTypeClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseTrack)
{
  static const GTypeInfo track_info = {
    sizeof (BseTrackClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_track_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseTrack),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_track_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseTrack",
				   "BSE track type",
				   &track_info);
}

static void
bse_track_class_init (BseTrackClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_track_set_property;
  gobject_class->get_property = bse_track_get_property;
  
  object_class->destroy = bse_track_destroy;
  
  bse_object_class_add_param (object_class, "Synth Input",
			      PARAM_SYNTH_NET,
			      g_param_spec_object ("snet", "Custom Synth Net", "Synthesis network to be used as instrument",
						   BSE_TYPE_SNET,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Play List",
			      PARAM_PART,
			      g_param_spec_object ("part", "Part", NULL,
						   BSE_TYPE_PART,
						   BSE_PARAM_DEFAULT));
}

static void
bse_track_init (BseTrack *self)
{
  self->snet = NULL;
  self->part_SL = NULL;
  self->midi_receiver_SL = bse_midi_receiver_new ("intern");
}

static void
bse_track_destroy (BseObject *object)
{
  BseTrack *self = BSE_TRACK (object);

  g_assert (self->sub_synth == NULL);

  /* check uncrossed references */
  g_assert (self->snet == NULL);
  g_assert (self->part_SL == NULL);

  bse_midi_receiver_unref (self->midi_receiver_SL);
  self->midi_receiver_SL = NULL;

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
notify_snet_changed (BseTrack *self)
{
  g_object_notify (G_OBJECT (self), "snet");
}

static void
snet_uncross (BseItem *owner,
	      BseItem *ref_item)
{
  BseTrack *self = BSE_TRACK (owner);

  g_object_disconnect (self->snet,
		       "any_signal", notify_snet_changed, self,
		       NULL);
  self->snet = NULL;
  g_object_notify (G_OBJECT (self), "snet");
}

static void
notify_part_changed (BseTrack *self)
{
  g_object_notify (G_OBJECT (self), "part");
}

static void
part_uncross (BseItem *owner,
	      BseItem *ref_item)
{
  BseTrack *self = BSE_TRACK (owner);

  g_object_disconnect (self->part_SL,
		       "any_signal", notify_part_changed, self,
		       NULL);
  BSE_SEQUENCER_LOCK ();
  self->part_SL = NULL;
  BSE_SEQUENCER_UNLOCK ();
  g_object_notify (G_OBJECT (self), "part");
}

static void
bse_track_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  BseTrack *self = BSE_TRACK (object);

  switch (param_id)
    {
    case PARAM_SYNTH_NET:
      if (self->snet)
	{
	  bse_item_uncross (BSE_ITEM (self), BSE_ITEM (self->snet));
	  g_assert (self->snet == NULL);
	}
      self->snet = g_value_get_object (value);
      if (self->snet)
	{
	  bse_item_cross_ref (BSE_ITEM (self), BSE_ITEM (self->snet), snet_uncross);
	  g_object_connect (self->snet,
			    "swapped_signal::notify::name", notify_snet_changed, self,
			    NULL);
	}
      if (self->sub_synth)
	g_object_set (self->sub_synth,
		      "snet", self->snet,
		      NULL);
      break;
    case PARAM_PART:
      if (self->part_SL)
	{
	  bse_item_uncross (BSE_ITEM (self), BSE_ITEM (self->part_SL));
	  g_assert (self->part_SL == NULL);
	}
      BSE_SEQUENCER_LOCK ();
      self->part_SL = g_value_get_object (value);
      BSE_SEQUENCER_UNLOCK ();
      if (self->part_SL)
	{
	  bse_item_cross_ref (BSE_ITEM (self), BSE_ITEM (self->part_SL), part_uncross);
	  g_object_connect (self->part_SL,
			    "swapped_signal::notify::name", notify_part_changed, self,
			    NULL);
	}
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_track_get_property (GObject    *object,
			guint       param_id,
			GValue     *value,
			GParamSpec *pspec)
{
  BseTrack *self = BSE_TRACK (object);
  
  switch (param_id)
    {
    case PARAM_SYNTH_NET:
      g_value_set_object (value, self->snet);
      break;
    case PARAM_PART:
      g_value_set_object (value, self->part_SL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

void
bse_track_add_modules (BseTrack     *self,
		       BseContainer *container,
		       BseSource    *merger)
{
  g_return_if_fail (BSE_IS_TRACK (self));
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (BSE_IS_CONTEXT_MERGER (merger));
  g_return_if_fail (self->sub_synth == NULL);

  /* midi voice input */
  self->voice_input = bse_container_new_item (container, BSE_TYPE_MIDI_VOICE_INPUT, NULL);
  BSE_OBJECT_SET_FLAGS (self->voice_input, BSE_ITEM_FLAG_STORAGE_IGNORE);
  bse_midi_voice_input_set_midi_receiver (BSE_MIDI_VOICE_INPUT (self->voice_input), self->midi_receiver_SL, 0);

  /* sub synth */
  self->sub_synth = bse_container_new_item (container, BSE_TYPE_SUB_SYNTH,
					    "in_port_1", "frequency",
					    "in_port_2", "gate",
					    "in_port_3", "velocity",
					    "in_port_4", "aftertouch",
					    "out_port_1", "left-audio",
					    "out_port_2", "right-audio",
					    "out_port_3", "unused",
					    "out_port_4", "synth-done",
					    "snet", self->snet,
					    NULL);
  BSE_OBJECT_SET_FLAGS (self->sub_synth, BSE_ITEM_FLAG_STORAGE_IGNORE);
  bse_sub_synth_set_midi_receiver (BSE_SUB_SYNTH (self->sub_synth), self->midi_receiver_SL, 0);

  /* midi voice switch */
  self->voice_switch = bse_container_new_item (container, BSE_TYPE_MIDI_VOICE_SWITCH, NULL);
  BSE_OBJECT_SET_FLAGS (self->voice_switch, BSE_ITEM_FLAG_STORAGE_IGNORE);
  bse_midi_voice_switch_set_voice_input (BSE_MIDI_VOICE_SWITCH (self->voice_switch), BSE_MIDI_VOICE_INPUT (self->voice_input));

  /* context merger */
  self->context_merger = bse_container_new_item (container, BSE_TYPE_CONTEXT_MERGER, NULL);
  BSE_OBJECT_SET_FLAGS (self->context_merger, BSE_ITEM_FLAG_STORAGE_IGNORE);

  /* voice input <-> sub-synth */
  bse_source_must_set_input (self->sub_synth, 0,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_FREQUENCY);
  bse_source_must_set_input (self->sub_synth, 1,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_GATE);
  bse_source_must_set_input (self->sub_synth, 2,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_VELOCITY);
  bse_source_must_set_input (self->sub_synth, 3,
			     self->voice_input, BSE_MIDI_VOICE_INPUT_OCHANNEL_AFTERTOUCH);

  /* sub-synth <-> voice switch */
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_LEFT,
			     self->sub_synth, 0);
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_RIGHT,
			     self->sub_synth, 1);
  bse_source_must_set_input (self->voice_switch, BSE_MIDI_VOICE_SWITCH_ICHANNEL_DISCONNECT,
			     self->sub_synth, 3);

  /* midi voice switch <-> context merger */
  bse_source_must_set_input (self->context_merger, 0,
			     self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_LEFT);
  bse_source_must_set_input (self->context_merger, 1,
			     self->voice_switch, BSE_MIDI_VOICE_SWITCH_OCHANNEL_RIGHT);

  /* context merger <-> container's merger */
  bse_source_must_set_input (merger, 0,
			     self->context_merger, 0);
  bse_source_must_set_input (merger, 1,
			     self->context_merger, 1);
}

void
bse_track_remove_modules (BseTrack     *self,
			  BseContainer *container)
{
  g_return_if_fail (BSE_IS_TRACK (self));
  g_return_if_fail (BSE_IS_CONTAINER (container));
  g_return_if_fail (self->sub_synth != NULL);

  bse_container_remove_item (container, BSE_ITEM (self->voice_input));
  self->voice_input = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->sub_synth));
  self->sub_synth = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->voice_switch));
  self->voice_switch = NULL;
  bse_container_remove_item (container, BSE_ITEM (self->context_merger));
  self->context_merger = NULL;
}

void
bse_track_clone_voices (BseTrack *self,
			BseSNet  *snet,
			guint     context,
			GslTrans *trans)
{
  guint i;

  g_return_if_fail (BSE_IS_TRACK (self));
  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (trans != NULL);

  for (i = 0; i < 10; i++)
    bse_snet_context_clone_branch (snet, context, self->context_merger, self->midi_receiver_SL, 0, trans);
}
