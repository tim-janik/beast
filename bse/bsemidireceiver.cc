/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2002 Tim Janik
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
#include "bsemidireceiver.h"

#include "bsemain.h"
#include "gslcommon.h"
#include "gslengine.h"
#include "gslsignal.h"
#include <string.h>
#include <sfi/gbsearcharray.h>

#define	DEBUG	sfi_debug_keyfunc ("midi-receiver")


typedef enum {
  VOICE_ON = 1,
  VOICE_PRESSURE,
  VOICE_SUSTAIN,
  VOICE_OFF,
  VOICE_KILL_SUSTAIN,
  VOICE_KILL
} VoiceChangeType;


/* --- prototypes --- */
static gint	midi_receiver_process_event_L  (BseMidiReceiver        *self,
						guint64                 max_tick_stamp);
static gint	midi_control_slots_compare	(gconstpointer		bsearch_node1, /* key */
						 gconstpointer		bsearch_node2);


/* --- variables --- */
static SfiMutex             midi_mutex = { 0, };
static SfiRing             *farm_residents = NULL;
static const GBSearchConfig ctrl_slot_config = {
  sizeof (BseMidiControlSlot),
  midi_control_slots_compare,
  0, /* G_BSEARCH_ARRAY_ALIGN_POWER2 */
};


/* --- function --- */
void
_bse_midi_init (void)
{
  static gboolean initialized = FALSE;
  
  g_assert (initialized++ == FALSE);
  
  sfi_mutex_init (&midi_mutex);
}

void
bse_midi_global_lock (void)
{
  GSL_SPIN_LOCK (&midi_mutex);
}

void
bse_midi_global_unlock (void)
{
  GSL_SPIN_UNLOCK (&midi_mutex);
}

static gint
events_cmp (gconstpointer a,
            gconstpointer b)
{
  const BseMidiEvent *e1 = a;
  const BseMidiEvent *e2 = b;
  
  return e1->tick_stamp < e2->tick_stamp ? -1 : e1->tick_stamp != e2->tick_stamp;
}

void
bse_midi_receiver_enter_farm (BseMidiReceiver *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (sfi_ring_find (farm_residents, self) == NULL);

  BSE_MIDI_RECEIVER_LOCK (self);
  farm_residents = sfi_ring_append (farm_residents, self);
  BSE_MIDI_RECEIVER_UNLOCK (self);
}

void
bse_midi_receiver_farm_distribute_event (BseMidiEvent *event)
{
  SfiRing *ring;

  g_return_if_fail (event != NULL);

  BSE_MIDI_RECEIVER_LOCK (self);
  for (ring = farm_residents; ring; ring = sfi_ring_walk (ring, farm_residents))
    {
      BseMidiReceiver *self = ring->data;
      self->events = sfi_ring_insert_sorted (self->events, bse_midi_copy_event (event), events_cmp);
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
}

void
bse_midi_receiver_farm_process_events (guint64 max_tick_stamp)
{
  gboolean seen_event;
  do
    {
      SfiRing *ring;
      seen_event = FALSE;
      BSE_MIDI_RECEIVER_LOCK (self);
      for (ring = farm_residents; ring; ring = sfi_ring_walk (ring, farm_residents))
        seen_event |= midi_receiver_process_event_L (ring->data, max_tick_stamp);
      BSE_MIDI_RECEIVER_UNLOCK (self);
    }
  while (seen_event);
}

void
bse_midi_receiver_leave_farm (BseMidiReceiver *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (sfi_ring_find (farm_residents, self) != NULL);

  BSE_MIDI_RECEIVER_LOCK (self);
  farm_residents = sfi_ring_remove (farm_residents, self);
  BSE_MIDI_RECEIVER_UNLOCK (self);
}

void
bse_midi_receiver_push_event (BseMidiReceiver *self,
			      BseMidiEvent    *event)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (event != NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  self->events = sfi_ring_insert_sorted (self->events, event, events_cmp);
  BSE_MIDI_RECEIVER_UNLOCK (self);
}

void
bse_midi_receiver_process_events (BseMidiReceiver *self,
				  guint64          max_tick_stamp)
{
  gboolean seen_event;

  g_return_if_fail (self != NULL);

  do
    {
      BSE_MIDI_RECEIVER_LOCK (self);
      seen_event = midi_receiver_process_event_L (self, max_tick_stamp);
      BSE_MIDI_RECEIVER_UNLOCK (self);
    }
  while (seen_event);
}


/* --- MIDI Control Slots --- */
static gint
midi_control_slots_compare (gconstpointer bsearch_node1, /* key */
			    gconstpointer bsearch_node2)
{
  const BseMidiControlSlot *slot1 = bsearch_node1;
  const BseMidiControlSlot *slot2 = bsearch_node2;
  gint cmp;
  
  cmp = G_BSEARCH_ARRAY_CMP (slot1->type, slot2->type);
  
  return cmp ? cmp : G_BSEARCH_ARRAY_CMP (slot1->midi_channel, slot2->midi_channel);
}

static inline gfloat
midi_control_get_L (BseMidiReceiver  *self,
                    guint             midi_channel,
		    BseMidiSignalType type)
{
  BseMidiControlSlot key, *slot;
  
  key.type = type;
  key.midi_channel = midi_channel;
  slot = g_bsearch_array_lookup (self->ctrl_slot_array, &ctrl_slot_config, &key);
  return slot ? slot->value : bse_midi_signal_default (type);
}

static inline GSList*
midi_control_set_L (BseMidiReceiver  *self,
		    guint             midi_channel,
		    BseMidiSignalType type,
		    gfloat            value)
{
  BseMidiControlSlot key, *slot;
  
  key.type = type;
  key.midi_channel = midi_channel;
  slot = g_bsearch_array_lookup (self->ctrl_slot_array, &ctrl_slot_config, &key);
  if (!slot)
    {
      key.value = bse_midi_signal_default (type);
      key.cmodules = NULL;
      self->ctrl_slot_array = g_bsearch_array_insert (self->ctrl_slot_array, &ctrl_slot_config, &key);
      slot = g_bsearch_array_lookup (self->ctrl_slot_array, &ctrl_slot_config, &key);
    }
  if (slot->value != value)
    {
      slot->value = value;
      return slot->cmodules;
    }
  else
    return NULL;
}

static void
midi_control_add_L (BseMidiReceiver  *self,
		    guint             midi_channel,
		    BseMidiSignalType type,
		    GslModule        *module)
{
  BseMidiControlSlot key, *slot;
  
  key.type = type;
  key.midi_channel = midi_channel;
  slot = g_bsearch_array_lookup (self->ctrl_slot_array, &ctrl_slot_config, &key);
  if (!slot)
    {
      key.value = bse_midi_signal_default (type);
      key.cmodules = NULL;
      self->ctrl_slot_array = g_bsearch_array_insert (self->ctrl_slot_array, &ctrl_slot_config, &key);
      slot = g_bsearch_array_lookup (self->ctrl_slot_array, &ctrl_slot_config, &key);
    }
  slot->cmodules = g_slist_prepend (slot->cmodules, module);
}

static void
midi_control_remove_L (BseMidiReceiver  *self,
		       guint             midi_channel,
		       BseMidiSignalType type,
		       GslModule        *module)
{
  BseMidiControlSlot key, *slot;
  
  key.type = type;
  key.midi_channel = midi_channel;
  slot = g_bsearch_array_lookup (self->ctrl_slot_array, &ctrl_slot_config, &key);
  g_return_if_fail (slot != NULL);
  slot->cmodules = g_slist_remove (slot->cmodules, module);
}


/* --- MIDI Control Module --- */
typedef struct
{
  guint             midi_channel;
  gfloat            values[BSE_MIDI_CONTROL_MODULE_N_CHANNELS];
  BseMidiSignalType signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS];
  guint             ref_count;
} MidiCModuleData;

static void
midi_control_module_process (GslModule *module,
			     guint      n_values)
{
  MidiCModuleData *cdata = module->user_data;
  guint i;
  
  for (i = 0; i < GSL_MODULE_N_OSTREAMS (module); i++)
    if (module->ostreams[i].connected)
      module->ostreams[i].values = gsl_engine_const_values (cdata->values[i]);
}

static GslModule*
create_control_module_L (BseMidiReceiver *self,
			 guint            midi_channel,
			 guint            signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS])
{
  static const GslClass midi_cmodule_class = {
    0,                                  /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_CONTROL_MODULE_N_CHANNELS, /* n_ostreams */
    midi_control_module_process,        /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    (GslModuleFreeFunc) g_free,         /* free */
    GSL_COST_CHEAP
  };
  MidiCModuleData *cdata;
  GslModule *module;
  guint i;
  
  g_return_val_if_fail (signals != NULL, NULL);
  
  cdata = g_new0 (MidiCModuleData, 1);
  cdata->midi_channel = midi_channel;
  for (i = 0; i < BSE_MIDI_CONTROL_MODULE_N_CHANNELS; i++)
    {
      cdata->signals[i] = signals[i];
      cdata->values[i] = midi_control_get_L (self, midi_channel, cdata->signals[i]);
    }
  cdata->ref_count = 1;
  module = gsl_module_new (&midi_cmodule_class, cdata);
  
  return module;
}

typedef struct {
  BseMidiSignalType signal;
  gfloat            value;
} MidiCModuleAccessData;

static void
midi_control_module_access (GslModule *module,
			    gpointer   data)
{
  MidiCModuleData *cdata = module->user_data;
  MidiCModuleAccessData *adata = data;
  guint i;
  
  for (i = 0; i < BSE_MIDI_CONTROL_MODULE_N_CHANNELS; i++)
    if (cdata->signals[i] == adata->signal)
      cdata->values[i] = adata->value;
}

static void
change_midi_control_modules (GSList           *modules,
			     guint64           tick_stamp,
			     BseMidiSignalType signal,
			     gfloat            value,
			     GslTrans         *trans)
{
  MidiCModuleAccessData *adata;
  GSList *slist = modules;
  
  if (!modules)
    return;
  adata = g_new0 (MidiCModuleAccessData, 1);
  adata->signal = signal;
  adata->value = value;
  for (slist = modules; slist; slist = slist->next)
    gsl_trans_add (trans, gsl_flow_job_access (slist->data,
					       tick_stamp,
					       midi_control_module_access,
					       adata,
					       slist->next ? NULL : g_free));
}

static gboolean
match_cmodule (GslModule *cmodule,
               guint      midi_channel,
               guint      signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS])
{
  MidiCModuleData *cdata = cmodule->user_data;
  gboolean match = TRUE;
  guint i;
  
  for (i = 0; i < BSE_MIDI_CONTROL_MODULE_N_CHANNELS; i++)
    match &= cdata->signals[i] == signals[i];
  match &= cdata->midi_channel == midi_channel;
  
  return match;
}


/* --- BseMidiMonoSynth modules --- */
struct _BseMidiMonoSynth
{
  /* module state */
  gfloat     freq_value;
  gfloat     gate;
  gfloat     velocity;
  gfloat     aftertouch;
  gboolean   within_note;
  gboolean   sustained;
  /* mono synth */
  guint      midi_channel;
  guint      ref_count;
  GslModule *fmodule;		/* note module */
  gboolean   discarded;
};

static void
mono_synth_module_process (GslModule *module,
                           guint      n_values)
{
  BseMidiMonoSynth *msynth = module->user_data;
  
  if (GSL_MODULE_OSTREAM (module, 0).connected)
    GSL_MODULE_OSTREAM (module, 0).values = gsl_engine_const_values (msynth->freq_value);
  if (GSL_MODULE_OSTREAM (module, 1).connected)
    GSL_MODULE_OSTREAM (module, 1).values = gsl_engine_const_values (msynth->gate);
  if (GSL_MODULE_OSTREAM (module, 2).connected)
    GSL_MODULE_OSTREAM (module, 2).values = gsl_engine_const_values (msynth->velocity);
  if (GSL_MODULE_OSTREAM (module, 3).connected)
    GSL_MODULE_OSTREAM (module, 3).values = gsl_engine_const_values (msynth->aftertouch);
}

typedef struct {
  VoiceChangeType vtype;
  gfloat          freq_value;
  gfloat          velocity;
} BseMidiMonoSynthData;

static void
mono_synth_module_access (GslModule *module,
                          gpointer   data)
{
  BseMidiMonoSynth *msynth = module->user_data;
  BseMidiMonoSynthData *mdata = data;

  switch (mdata->vtype)
    {
    case VOICE_ON:
      msynth->freq_value = mdata->freq_value;
      msynth->gate = 1.0;
      msynth->velocity = mdata->velocity;
      msynth->aftertouch = mdata->velocity;
      msynth->within_note = TRUE;
      msynth->sustained = FALSE;
      break;
    case VOICE_PRESSURE:
      if (msynth->within_note && GSL_SIGNAL_FREQ_EQUALS (msynth->freq_value,
                                                         mdata->freq_value))
        msynth->aftertouch = mdata->velocity;
      break;
    case VOICE_SUSTAIN:
      if (msynth->within_note && GSL_SIGNAL_FREQ_EQUALS (msynth->freq_value,
                                                         mdata->freq_value))
        {
          msynth->within_note = FALSE;
          msynth->sustained = TRUE;
        }
      break;
    case VOICE_OFF:
      if ((msynth->within_note || msynth->sustained) &&
          GSL_SIGNAL_FREQ_EQUALS (msynth->freq_value,
                                  mdata->freq_value))
        goto kill_voice;
      break;
    case VOICE_KILL_SUSTAIN:
      if (msynth->sustained)
        goto kill_voice;
      break;
    case VOICE_KILL:
    kill_voice:
      msynth->gate = 0.0;
      msynth->within_note = FALSE;
      msynth->sustained = FALSE;
      break;
    }
}

static void
change_mono_synth_voice (BseMidiMonoSynth *msynth,
                         guint64           tick_stamp,
                         VoiceChangeType   vtype,
                         gfloat            freq_value,
                         gfloat            velocity,
                         GslTrans         *trans)
{
  BseMidiMonoSynthData mdata;

  mdata.vtype = vtype;
  mdata.freq_value = freq_value;
  mdata.velocity = velocity;

  gsl_trans_add (trans, gsl_flow_job_access (msynth->fmodule, tick_stamp,
					     mono_synth_module_access,
					     g_memdup (&mdata, sizeof (mdata)), g_free));
}

static void
mono_synth_module_free (gpointer        data,
                        const GslClass *klass)
{
  BseMidiMonoSynth *msynth = data;

  g_free (msynth);
}

static BseMidiMonoSynth*
create_mono_synth (guint     midi_channel,
		   GslTrans *trans)
{
  static const GslClass mono_synth_module_class = {
    0,                                  /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_MODULE_N_CHANNELS,   /* n_ostreams */
    mono_synth_module_process,          /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    mono_synth_module_free,		/* free */
    GSL_COST_CHEAP
  };
  BseMidiMonoSynth *msynth = g_new0 (BseMidiMonoSynth, 1);

  msynth->midi_channel = midi_channel;
  msynth->fmodule = gsl_module_new (&mono_synth_module_class, msynth);
  msynth->freq_value = 0;
  msynth->gate = 0;
  msynth->velocity = 0.5;
  msynth->aftertouch = 0.5;
  msynth->within_note = FALSE;
  msynth->sustained = FALSE;
  msynth->discarded = FALSE;
  msynth->ref_count = 1;
  gsl_trans_add (trans, gsl_job_integrate (msynth->fmodule));
  
  return msynth;
}

static void
destroy_mono_synth (BseMidiMonoSynth *msynth,
		    GslTrans         *trans)
{
  g_return_if_fail (msynth->discarded == FALSE);
  g_return_if_fail (msynth->ref_count == 0);
  
  msynth->discarded = TRUE;
  gsl_trans_add (trans, gsl_job_discard (msynth->fmodule));
}


/* --- BseMidiVoice modules --- */
struct _BseMidiVoice
{
  guint      midi_channel;
  GslModule *fmodule;		/* note module */
  GslModule *smodule;		/* input module (switches and suspends) */
  GslModule *omodule;		/* output module (virtual) */
  guint      module_ref;
  gfloat     freq_value;
  gfloat     gate;		/* mutatable while active */
  gfloat     velocity;
  gfloat     aftertouch;	/* mutatable while active */
  gboolean   active;		/* reset in process() method */
  gboolean   within_note;       /* waiting for NOTE_OFF */
  gboolean   sustained;
  gboolean   discarded;
  guint      ref_count;
};

static void
switch_module_process (GslModule *module,
		       guint      n_values)
{
  BseMidiVoice *voice = module->user_data;
  guint i;
  
  /* dumb pass-through task */
  for (i = 0; i < GSL_MODULE_N_OSTREAMS (module); i++)
    if (GSL_MODULE_OSTREAM (module, i).connected)
      GSL_MODULE_OSTREAM (module, i).values = (gfloat*) GSL_MODULE_IBUFFER (module, i);
  
  /* check Done state on last stream */
  if (GSL_MODULE_IBUFFER (module, GSL_MODULE_N_ISTREAMS (module) - 1)[n_values - 1] >= 1.0)
    {
      GslTrans *trans = gsl_trans_open ();
      
      /* disconnect all inputs */
      for (i = 0; i < GSL_MODULE_N_OSTREAMS (module); i++)
        gsl_trans_add (trans, gsl_job_disconnect (voice->omodule, i));
      gsl_trans_add (trans, gsl_job_suspend (module));
      gsl_trans_commit (trans);
      
      /* may be reconnected again */
      voice->active = FALSE;
    }
}

static void
midi_voice_module_process (GslModule *module,
                           guint      n_values)
{
  BseMidiVoice *voice = module->user_data;
  
  if (GSL_MODULE_OSTREAM (module, 0).connected)
    GSL_MODULE_OSTREAM (module, 0).values = gsl_engine_const_values (voice->freq_value);
  if (GSL_MODULE_OSTREAM (module, 1).connected)
    GSL_MODULE_OSTREAM (module, 1).values = gsl_engine_const_values (voice->gate);
  if (GSL_MODULE_OSTREAM (module, 2).connected)
    GSL_MODULE_OSTREAM (module, 2).values = gsl_engine_const_values (voice->velocity);
  if (GSL_MODULE_OSTREAM (module, 3).connected)
    GSL_MODULE_OSTREAM (module, 3).values = gsl_engine_const_values (voice->aftertouch);
}

static void
midi_voice_module_access (GslModule *module,
                          gpointer   data)
{
  BseMidiVoice *voice = module->user_data;
  gfloat *values = data;
  
  voice->gate = values[0];
  if (values[1] >= 0)
    voice->aftertouch = values[1];
}

static void
change_midi_voice (BseMidiVoice *voice,
		   guint64       tick_stamp,
		   gboolean      gate_on,
		   gfloat        aftertouch,
		   GslTrans     *trans)
{
  gfloat values[2];
  
  values[0] = gate_on ? 1.0 : 0.0;
  if (gate_on)
    values[1] = aftertouch;
  else
    values[1] = -1;
  gsl_trans_add (trans, gsl_flow_job_access (voice->fmodule, tick_stamp,
					     midi_voice_module_access,
					     g_memdup (values, sizeof (values)), g_free));
}

static void
activate_midi_voice (BseMidiVoice *voice,
		     guint64       tick_stamp,
		     gfloat        freq_value,
		     gfloat        velocity,
		     GslTrans     *trans)
{
  guint i;
  
  g_return_if_fail (voice->active == FALSE);
  
  voice->freq_value = freq_value;
  voice->gate = 1.0;
  voice->velocity = velocity;
  voice->aftertouch = velocity;
  voice->active = TRUE;
  voice->within_note = TRUE;
  voice->sustained = FALSE;
  /* connect modules */
  for (i = 0; i < GSL_MODULE_N_ISTREAMS (voice->smodule); i++)
    gsl_trans_add (trans, gsl_job_connect (voice->smodule, i, voice->omodule, i));
  gsl_trans_add (trans, gsl_flow_job_resume (voice->smodule, tick_stamp));
}

static void
midi_voice_module_free (gpointer        data,
                        const GslClass *klass)
{
  BseMidiVoice *voice = data;
  
  g_return_if_fail (voice->module_ref > 0);
  voice->module_ref--;
  if (!voice->module_ref)
    g_free (voice);
}

static BseMidiVoice*
create_midi_voice (guint     midi_channel,
		   GslTrans *trans)
{
  static const GslClass voice_module_class = {
    0,                                  /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_MODULE_N_CHANNELS,   /* n_ostreams */
    midi_voice_module_process,          /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    midi_voice_module_free,		/* free */
    GSL_COST_CHEAP
  };
  static const GslClass switch_module_class = {
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_ostreams */
    switch_module_process,              /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    midi_voice_module_free,             /* free */
    GSL_COST_CHEAP
  };
  BseMidiVoice *voice = g_new0 (BseMidiVoice, 1);
  
  voice->midi_channel = midi_channel;
  voice->fmodule = gsl_module_new (&voice_module_class, voice);
  voice->smodule = gsl_module_new (&switch_module_class, voice);
  voice->omodule = gsl_module_new_virtual (BSE_MIDI_VOICE_N_CHANNELS, NULL, NULL);
  voice->module_ref = 2;		/* voice module, switch module */
  voice->freq_value = 0;
  voice->gate = 0;
  voice->velocity = 0;
  voice->aftertouch = 0;
  voice->active = FALSE;
  voice->within_note = FALSE;
  voice->sustained = FALSE;
  voice->discarded = FALSE;
  voice->ref_count = 1;
  gsl_trans_add (trans, gsl_job_integrate (voice->fmodule));
  gsl_trans_add (trans, gsl_job_integrate (voice->smodule));
  gsl_trans_add (trans, gsl_job_integrate (voice->omodule));
  gsl_trans_add (trans, gsl_job_suspend (voice->smodule));
  
  return voice;
}

static void
discard_accessor (GslModule *module,
		  gpointer   data)
{
  GslTrans *trans = data;
  
  gsl_trans_commit (trans);
}

static void
destroy_midi_voice (BseMidiVoice *voice,
		    GslTrans     *trans)
{
  GslTrans *tmp_trans;
  
  g_return_if_fail (voice->discarded == FALSE);
  g_return_if_fail (voice->ref_count == 0);
  
  voice->discarded = TRUE;
  tmp_trans = gsl_trans_open ();
  gsl_trans_add (tmp_trans, gsl_job_discard (voice->fmodule));
  gsl_trans_add (tmp_trans, gsl_job_discard (voice->smodule));
  gsl_trans_add (tmp_trans, gsl_job_discard (voice->omodule));
  /* we can't commit the transaction right away, because the switch
   * module might currently be processing and is about to queue
   * disconnection jobs on the modules we're just discarding.
   * so we use a normal accessor which makes sure that pending
   * disconnect jobs have been processed already.
   */
  gsl_trans_add (trans, gsl_job_access (voice->smodule, discard_accessor, tmp_trans, NULL));
}


/* --- BseMidiReceiver functions --- */
BseMidiReceiver*
bse_midi_receiver_new (const gchar *receiver_name)
{
  BseMidiReceiver *self;
  
  g_return_val_if_fail (receiver_name != NULL, NULL);
  
  self = g_new0 (BseMidiReceiver, 1);
  self->receiver_name = g_strdup (receiver_name);
  self->n_msynths = 0;
  self->msynths = NULL;
  self->n_pvoices = 0;
  self->pvoices = NULL;
  self->n_cmodules = 0;
  self->cmodules = NULL;
  self->ctrl_slot_array = g_bsearch_array_create (&ctrl_slot_config);
  self->events = NULL;
  self->ref_count = 1;
  
  return self;
}

BseMidiReceiver*
bse_midi_receiver_ref (BseMidiReceiver *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->ref_count > 0, NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  self->ref_count++;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return self;
}

void
bse_midi_receiver_unref (BseMidiReceiver *self)
{
  gboolean need_destroy, leave_farm;

  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count > 0);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  self->ref_count--;
  need_destroy = self->ref_count == 0;
  if (need_destroy && self->notifier)
    {
      g_object_unref (self->notifier);
      self->notifier = NULL;
    }
  leave_farm = need_destroy && sfi_ring_find (farm_residents, self);
  BSE_MIDI_RECEIVER_UNLOCK (self);

  if (need_destroy)
    {
      guint i;

      if (leave_farm)
        bse_midi_receiver_leave_farm (self);

      for (i = 0; i < self->n_msynths; i++)
	if (self->msynths[i])
	  {
	    g_warning ("destroying MIDI receiver (%p) with active mono synth modules", self);
	    break;
	  }
      for (i = 0; i < self->n_pvoices; i++)
	if (self->pvoices[i])
	  {
	    g_warning ("destroying MIDI receiver (%p) with active voice modules", self);
	    break;
	  }
      if (self->n_cmodules)
	g_warning ("destroying MIDI receiver (%p) with active control modules (%u)", self, self->n_cmodules);
      g_free (self->receiver_name);
      g_free (self->msynths);
      g_free (self->pvoices);
      g_free (self->cmodules);
      g_bsearch_array_free (self->ctrl_slot_array, &ctrl_slot_config);
      while (self->events)
	{
	  BseMidiEvent *event = sfi_ring_pop_head (&self->events);
	  bse_midi_free_event (event);
	}
      while (self->notifier_events)
	{
	  BseMidiEvent *event = sfi_ring_pop_head (&self->notifier_events);
	  bse_midi_free_event (event);
	}
      g_free (self);
    }
}

void
bse_midi_receiver_set_notifier (BseMidiReceiver *self,
				BseMidiNotifier *notifier)
{
  BseMidiNotifier *old_notifier;
  
  g_return_if_fail (self != NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  old_notifier = self->notifier;
  self->notifier = notifier;
  if (self->notifier)
    g_object_ref (notifier);
  if (old_notifier)
    g_object_unref (old_notifier);
  if (!self->notifier)
    while (self->notifier_events)
      {
	BseMidiEvent *event = sfi_ring_pop_head (&self->notifier_events);
	bse_midi_free_event (event);
      }
  BSE_MIDI_RECEIVER_UNLOCK (self);
}

BseMidiNotifier*
bse_midi_receiver_get_notifier (BseMidiReceiver *self)
{
  BseMidiNotifier *notifier;
  
  g_return_val_if_fail (self != NULL, NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  notifier = self->notifier;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return notifier;
}

gboolean
bse_midi_receiver_has_notify_events (BseMidiReceiver *self)
{
  /* prolly don't need a lock */
  return self->notifier && self->notifier_events;
}

SfiRing*
bse_midi_receiver_fetch_notify_events (BseMidiReceiver *self)
{
  SfiRing *ring;
  
  g_return_val_if_fail (self != NULL, NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  ring = self->notifier_events;
  self->notifier_events = NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return ring;
}

GslModule*
bse_midi_receiver_retrieve_control_module (BseMidiReceiver  *self,
					   guint             midi_channel,
					   BseMidiSignalType signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS],
					   GslTrans         *trans)
{
  GslModule *cmodule;
  guint i;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  g_return_val_if_fail (signals != NULL, NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  for (i = 0; i < self->n_cmodules; i++)
    {
      cmodule = self->cmodules[i];
      if (match_cmodule (cmodule, midi_channel, signals))
        {
          MidiCModuleData *cdata = cmodule->user_data;
          cdata->ref_count++;
	  BSE_MIDI_RECEIVER_UNLOCK (self);
          return cmodule;
        }
    }
  cmodule = create_control_module_L (self, midi_channel, signals);
  i = self->n_cmodules++;
  self->cmodules = g_renew (GslModule*, self->cmodules, self->n_cmodules);
  self->cmodules[i] = cmodule;
  gsl_trans_add (trans, gsl_job_integrate (cmodule));
  midi_control_add_L (self, midi_channel, signals[0], cmodule);
  if (signals[1] != signals[0])
    midi_control_add_L (self, midi_channel, signals[1], cmodule);
  if (signals[2] != signals[1] && signals[2] != signals[0])
    midi_control_add_L (self, midi_channel, signals[2], cmodule);
  if (signals[3] != signals[2] && signals[3] != signals[1] && signals[3] != signals[0])
    midi_control_add_L (self, midi_channel, signals[3], cmodule);
  BSE_MIDI_RECEIVER_UNLOCK (self);
  return cmodule;
}

void
bse_midi_receiver_discard_control_module (BseMidiReceiver *self,
                                          GslModule       *module,
					  GslTrans        *trans)
{
  guint i;
  
  g_return_if_fail (self != NULL);
  g_return_if_fail (module != NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  for (i = 0; i < self->n_cmodules; i++)
    {
      GslModule *cmodule = self->cmodules[i];
      if (cmodule == module)
        {
          MidiCModuleData *cdata = cmodule->user_data;
          g_return_if_fail (cdata->ref_count > 0);
          cdata->ref_count--;
          if (!cdata->ref_count)
            {
	      BseMidiSignalType *signals = cdata->signals;
	      guint midi_channel = cdata->midi_channel;
              self->n_cmodules--;
              self->cmodules[i] = self->cmodules[self->n_cmodules];
              gsl_trans_add (trans, gsl_job_discard (cmodule));
	      midi_control_remove_L (self, midi_channel, signals[0], cmodule);
	      if (signals[1] != signals[0])
		midi_control_remove_L (self, midi_channel, signals[1], cmodule);
	      if (signals[2] != signals[1] && signals[2] != signals[0])
		midi_control_remove_L (self, midi_channel, signals[2], cmodule);
	      if (signals[3] != signals[2] && signals[3] != signals[1] && signals[3] != signals[0])
		midi_control_remove_L (self, midi_channel, signals[3], cmodule);
	    }
	  BSE_MIDI_RECEIVER_UNLOCK (self);
          return;
        }
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
  g_warning ("no such control module: %p", module);
}

GslModule*
bse_midi_receiver_retrieve_mono_synth (BseMidiReceiver *self,
                                       guint            midi_channel,
                                       GslTrans        *trans)
{
  BseMidiMonoSynth *msynth;
  guint i;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  for (i = 0; i < self->n_msynths; i++)
    {
      msynth = self->msynths[i];
      if (msynth && midi_channel == msynth->midi_channel)
        {
          msynth->ref_count++;
	  BSE_MIDI_RECEIVER_UNLOCK (self);
          return msynth->fmodule;
        }
    }

  i = self->n_msynths++;
  self->msynths = g_renew (BseMidiMonoSynth*, self->msynths, self->n_msynths);

  msynth = create_mono_synth (midi_channel, trans);
  self->msynths[i] = msynth;
  BSE_MIDI_RECEIVER_UNLOCK (self);

  return msynth->fmodule;
}

void
bse_midi_receiver_discard_mono_synth (BseMidiReceiver *self,
                                      GslModule       *fmodule,
                                      GslTrans        *trans)
{
  BseMidiMonoSynth *msynth;
  guint i;
  
  g_return_if_fail (self != NULL);
  g_return_if_fail (fmodule != NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  for (i = 0; i < self->n_msynths; i++)
    {
      msynth = self->msynths[i];
      if (msynth->fmodule == fmodule)
        {
          g_return_if_fail (msynth->ref_count > 0);
          msynth->ref_count--;
          if (!msynth->ref_count)
            {
              self->n_msynths--;
              self->msynths[i] = self->msynths[self->n_msynths];
	      destroy_mono_synth (msynth, trans);
	    }
	  BSE_MIDI_RECEIVER_UNLOCK (self);
          return;
        }
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
  g_warning ("no such mono synth module: %p", fmodule);
}

guint
bse_midi_receiver_create_voice (BseMidiReceiver *self,
				guint            midi_channel,
				GslTrans        *trans)
{
  guint i;
  
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (midi_channel > 0, 0);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  /* find free voice slot */
  for (i = 0; i < self->n_pvoices; i++)
    if (self->pvoices[i] == NULL)
      break;
  /* alloc voice slot */
  if (i >= self->n_pvoices)
    {
      i = self->n_pvoices++;
      self->pvoices = g_renew (BseMidiVoice*, self->pvoices, self->n_pvoices);
    }
  self->pvoices[i] = create_midi_voice (midi_channel, trans);
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return i + 1;
}

void
bse_midi_receiver_discard_voice (BseMidiReceiver *self,
				 guint            voice_id,
				 GslTrans        *trans)
{
  BseMidiVoice *voice;

  g_return_if_fail (self != NULL);
  g_return_if_fail (voice_id > 0);
  g_return_if_fail (voice_id <= self->n_pvoices);
  voice_id -= 1;

  BSE_MIDI_RECEIVER_LOCK (self);
  voice = self->pvoices[voice_id];
  if (!voice)
    {
      BSE_MIDI_RECEIVER_UNLOCK (self);
      g_warning ("MIDI receiver %p has no voice %u", self, voice_id);
      return;
    }
  g_return_if_fail (voice->ref_count > 0);
  voice->ref_count--;
  if (!voice->ref_count)
    {
      destroy_midi_voice (self->pvoices[voice_id], trans);
      self->pvoices[voice_id] = NULL;
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
}

GslModule*
bse_midi_receiver_get_note_module (BseMidiReceiver *self,
                                   guint            voice_id)
{
  BseMidiVoice *voice;
  GslModule *module;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (voice_id > 0, NULL);
  g_return_val_if_fail (voice_id <= self->n_pvoices, NULL);
  voice_id -= 1;
  
  BSE_MIDI_RECEIVER_LOCK (self);
  voice = self->pvoices[voice_id];
  module = voice ? voice->fmodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return module;
}

GslModule*
bse_midi_receiver_get_input_module (BseMidiReceiver *self,
                                    guint            voice_id)
{
  BseMidiVoice *voice;
  GslModule *module;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (voice_id > 0, NULL);
  g_return_val_if_fail (voice_id <= self->n_pvoices, NULL);
  voice_id -= 1;
  
  BSE_MIDI_RECEIVER_LOCK (self);
  voice = self->pvoices[voice_id];
  module = voice ? voice->smodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return module;
}

GslModule*
bse_midi_receiver_get_output_module (BseMidiReceiver *self,
                                     guint            voice_id)
{
  BseMidiVoice *voice;
  GslModule *module;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (voice_id > 0, NULL);
  g_return_val_if_fail (voice_id <= self->n_pvoices, NULL);
  voice_id -= 1;
  
  BSE_MIDI_RECEIVER_LOCK (self);
  voice = self->pvoices[voice_id];
  module = voice ? voice->omodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return module;
}

gboolean
bse_midi_receiver_voices_pending (BseMidiReceiver *self,
                                  guint            midi_channel)
{
  SfiRing *ring = NULL;
  guint i, active = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (midi_channel > 0, FALSE);

  if (self->events)
    return TRUE;

  BSE_MIDI_RECEIVER_LOCK (self);
  /* find busy voice */
  for (i = 0; i < self->n_msynths && !active; i++)
    {
      BseMidiMonoSynth *msynth = self->msynths[i];
      active += msynth && msynth->midi_channel == midi_channel && (msynth->within_note || msynth->sustained);
    }
  /* find busy poly voice */
  for (i = 0; i < self->n_pvoices && !active; i++)
    {
      BseMidiVoice *voice = self->pvoices[i];
      active += voice && voice->midi_channel == midi_channel && voice->active;
    }
  /* find pending events */
  for (ring = self->events; ring && !active; ring = sfi_ring_next (ring, self->events))
    {
      BseMidiEvent *event = ring->data;
      active += event->channel == midi_channel;
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);

  return active > 0;
}


/* --- event processing --- */
static void
activate_voice_L (BseMidiReceiver *self,
		  guint            midi_channel,
		  guint64          tick_stamp,
		  gfloat           freq,
		  gfloat           velocity,
		  GslTrans        *trans)
{
  gfloat freq_val = BSE_VALUE_FROM_FREQ (freq);
  BseMidiVoice *voice;
  guint i;
  
  g_return_if_fail (freq > 0);

  /* adjust mono synth if any */
  for (i = 0; i < self->n_msynths; i++)
    {
      BseMidiMonoSynth *msynth = self->msynths[i];
      if (msynth && midi_channel == msynth->midi_channel)
        {
          change_mono_synth_voice (msynth, tick_stamp, VOICE_ON, freq_val, velocity, trans);
          break;
        }
    }

  /* find free poly voice */
  for (i = 0; i < self->n_pvoices; i++)
    {
      voice = self->pvoices[i];
      if (voice && midi_channel == voice->midi_channel && !voice->active)
	break;
    }
  if (i >= self->n_pvoices)
    {
      DEBUG ("Receiver<%s:%u>: no voice available for note-on (%fHz)", self->receiver_name, midi_channel, freq);
      return;
    }
  /* setup voice */
  activate_midi_voice (voice, tick_stamp, freq_val, velocity, trans);
}

static void
adjust_voice_L (BseMidiReceiver *self,
		guint            midi_channel,
		guint64          tick_stamp,
		gfloat           freq,
                BseMidiEventType etype, /* BSE_MIDI_KEY_PRESSURE or BSE_MIDI_NOTE_OFF */
		gfloat           velocity,
		GslTrans        *trans)
{
  gfloat freq_val = BSE_VALUE_FROM_FREQ (freq);
  BseMidiVoice *voice;
  guint i;
  gboolean sustained_note = etype == BSE_MIDI_NOTE_OFF &&
                            (BSE_GCONFIG (invert_sustain) ^
                             (midi_control_get_L (self, midi_channel, BSE_MIDI_SIGNAL_CONTROL_64) >= 0.5));
  
  g_return_if_fail (freq > 0 && velocity >= 0);
  
  /* adjust mono synth if any */
  for (i = 0; i < self->n_msynths; i++)
    {
      BseMidiMonoSynth *msynth = self->msynths[i];
      if (msynth && midi_channel == msynth->midi_channel)
        {
          if (sustained_note)
            change_mono_synth_voice (msynth, tick_stamp, VOICE_SUSTAIN, freq_val, velocity, trans);
          else if (etype == BSE_MIDI_KEY_PRESSURE)
            change_mono_synth_voice (msynth, tick_stamp, VOICE_PRESSURE, freq_val, velocity, trans);
          else
            change_mono_synth_voice (msynth, tick_stamp, VOICE_OFF, freq_val, velocity, trans);
          break;
        }
    }

  /* adjust poly voice if any */
  for (i = 0; i < self->n_pvoices; i++)
    {
      voice = self->pvoices[i];
      if (voice && midi_channel == voice->midi_channel && voice->active &&
	  voice->within_note && !GSL_SIGNAL_FREQ_CHANGED (voice->freq_value, freq_val))
	{
	  if (etype == BSE_MIDI_NOTE_OFF)
	    voice->within_note = FALSE;
	  break;
	}
    }
  if (i >= self->n_pvoices)
    {
      DEBUG ("Receiver<%s:%u>: no voice available for %s (%fHz)", self->receiver_name, midi_channel,
	     etype == BSE_MIDI_NOTE_OFF ? "note-off" : "velocity",
	     freq);
      return;
    }
  /* set voice outputs */
  if (etype == BSE_MIDI_NOTE_OFF &&	/* sustain check: */
      (BSE_GCONFIG (invert_sustain) ^ (midi_control_get_L (self, midi_channel, BSE_MIDI_SIGNAL_CONTROL_64) >= 0.5)))
    {
      voice->sustained = TRUE;
      change_midi_voice (voice, tick_stamp, TRUE, velocity, trans);
    }
  else
    change_midi_voice (voice, tick_stamp, etype == BSE_MIDI_KEY_PRESSURE, velocity, trans);
}

static void
kill_voices_L (BseMidiReceiver *self,
	       guint            midi_channel,
	       guint64          tick_stamp,
	       gboolean         sustained_only,
	       GslTrans        *trans)
{
  BseMidiVoice *voice;
  guint i, count = 0;

  for (i = 0; i < self->n_msynths; i++)
    {
      BseMidiMonoSynth *msynth = self->msynths[i];
      if (msynth && midi_channel == msynth->midi_channel)
        {
          if (sustained_only)
            change_mono_synth_voice (msynth, tick_stamp, VOICE_KILL_SUSTAIN, 0, 0, trans);
          else
            change_mono_synth_voice (msynth, tick_stamp, VOICE_KILL, 0, 0, trans);
          break;
        }
    }

  for (i = 0; i < self->n_pvoices; i++)
    {
      voice = self->pvoices[i];
      if (voice && midi_channel == voice->midi_channel && voice->active)
	{
	  if ((sustained_only && !voice->within_note && voice->sustained) ||
	      (!sustained_only && (voice->within_note || voice->sustained)))
	    {
	      voice->within_note = FALSE;
	      voice->sustained = FALSE;
	      change_midi_voice (voice, tick_stamp, FALSE, 0, trans);
	      count++;
	    }
	}
    }
  if (count)
    DEBUG ("Receiver<%s:%u>: Voices Killed: %u", self->receiver_name, midi_channel, count);
}

static void
debug_voices_L (BseMidiReceiver *self,
		guint            midi_channel,
		guint64          tick_stamp,
		GslTrans        *trans)
{
  BseMidiVoice *voice;
  guint i;
  
  if (sfi_debug_test_key ("midi"))
    for (i = 0; i < self->n_pvoices; i++)
      {
	voice = self->pvoices[i];
	if (voice)
	  DEBUG ("Receiver<%s>: Voice: Channel=%u SynthActive=%u Playing=%u Sustained=%u Freq=%fHz", self->receiver_name,
		 voice->midi_channel, voice->active, voice->within_note, voice->sustained,
		 BSE_FREQ_FROM_VALUE (voice->freq_value));
      }
}

static inline void
update_midi_signal_L (BseMidiReceiver  *self,
		      guint             channel,
		      guint64           tick_stamp,
		      BseMidiSignalType signal,
		      gfloat            value,
		      GslTrans         *trans)
{
  GSList *signal_modules;
  
  signal_modules = midi_control_set_L (self, channel, signal, value);
  change_midi_control_modules (signal_modules,
                               tick_stamp,
                               signal,
                               value,
                               trans);
#if 0
  DEBUG ("Receiver<%s:%u>: Signal %3u Value=%f (%s)", self->receiver_name, channel,
	 signal, value, bse_midi_signal_name (signal));
#endif
}

static inline void
update_midi_signal_continuous_msb_L (BseMidiReceiver  *self,
				     guint             channel,
				     guint64           tick_stamp,
				     BseMidiSignalType continuous_signal,
				     gfloat            value,
				     BseMidiSignalType lsb_signal,
				     GslTrans         *trans)
{
  gint ival;
  /* LSB part */
  ival = gsl_ftoi (midi_control_get_L (self, channel, lsb_signal) * 0x7f);
  /* add MSB part */
  ival |= gsl_ftoi (value * 0x7f) << 7;
  /* set continuous */
  value = ival / (gfloat) 0x3fff;
  update_midi_signal_L (self, channel, tick_stamp, continuous_signal, value, trans);
}

static inline void
update_midi_signal_continuous_lsb_L (BseMidiReceiver  *self,
				     guint             channel,
				     guint64           tick_stamp,
				     BseMidiSignalType continuous_signal,
				     BseMidiSignalType msb_signal,
				     gfloat            value,
				     GslTrans         *trans)
{
  gint ival;
  /* LSB part */
  ival = gsl_ftoi (value * 0x7f);
  /* add MSB part */
  ival |= gsl_ftoi (midi_control_get_L (self, channel, msb_signal) * 0x7f) << 7;
  value = ival / (gfloat) 0x3fff;
  update_midi_signal_L (self, channel, tick_stamp, continuous_signal, value, trans);
}

static inline void
process_midi_control_L (BseMidiReceiver *self,
			guint            channel,
			guint64          tick_stamp,
			guint            control,
			gfloat           value,
                        gboolean         extra_continuous,
			GslTrans        *trans)
{
  /* here, we need to translate midi control numbers
   * into BSE MIDI signals. some control numbers affect
   * multiple MIDI signals. extra_continuous are used
   * internally to update only continuous signals.
   */

  if (extra_continuous)
    {
      /* internal BSE_MIDI_SIGNAL_CONTINUOUS_* change */
      update_midi_signal_L (self, channel, tick_stamp, 64 + control, value, trans);
      return;
    }

  /* all MIDI controls are passed literally as BSE_MIDI_SIGNAL_CONTROL_* */
  update_midi_signal_L (self, channel, tick_stamp, 128 + control, value, trans);

  if (control < 32)		/* MSB part of continuous 14bit signal */
    update_midi_signal_continuous_msb_L (self, channel, tick_stamp,
					 control + 64,		/* continuous signal */
					 value,			/* MSB value */
					 128 + control + 32,	/* LSB signal */
					 trans);
  else if (control < 64)	/* LSB part of continuous 14bit signal */
    update_midi_signal_continuous_lsb_L (self, channel, tick_stamp,
					 control + 32,		/* continuous signal */
					 128 + control - 32,	/* MSB signal */
					 value,			/* LSB value */
					 trans);
  else switch (control)
    {
    case 64:			/* Damper Pedal Switch (Sustain) */
      if (BSE_GCONFIG (invert_sustain) ^ (value < 0.5))
	kill_voices_L (self, channel, tick_stamp, TRUE, trans);
      break;
    case 98:			/* Non-Registered Parameter MSB */
      update_midi_signal_continuous_msb_L (self, channel, tick_stamp,
					   BSE_MIDI_SIGNAL_NON_PARAMETER,	/* continuous signal */
					   value,                 		/* MSB value */
					   BSE_MIDI_SIGNAL_CONTROL_99,		/* LSB signal */
					   trans);
      break;
    case 99:			/* Non-Registered Parameter LSB */
      update_midi_signal_continuous_lsb_L (self, channel, tick_stamp,
					   BSE_MIDI_SIGNAL_NON_PARAMETER,	/* continuous signal */
					   BSE_MIDI_SIGNAL_CONTROL_98,		/* MSB signal */
					   value,                 		/* LSB value */
					   trans);
      break;
    case 100:			/* Registered Parameter MSB */
      update_midi_signal_continuous_msb_L (self, channel, tick_stamp,
					   BSE_MIDI_SIGNAL_PARAMETER,		/* continuous signal */
					   value,                 		/* MSB value */
					   BSE_MIDI_SIGNAL_CONTROL_101,		/* LSB signal */
					   trans);
      break;
    case 101:			/* Registered Parameter LSB */
      update_midi_signal_continuous_lsb_L (self, channel, tick_stamp,
					   BSE_MIDI_SIGNAL_PARAMETER,		/* continuous signal */
					   BSE_MIDI_SIGNAL_CONTROL_100,		/* MSB signal */
					   value,                 		/* LSB value */
					   trans);
      break;
    case 120:			/* All Sound Off ITrigger */
    case 123:			/* All Notes Off ITrigger */
      kill_voices_L (self, channel, tick_stamp, FALSE, trans);
      break;
    case 122:			/* Local Control Switch */
      if (value < 0.00006)
	debug_voices_L (self, channel, tick_stamp, trans);
      break;
    }
}

static gint
midi_receiver_process_event_L (BseMidiReceiver *self,
			       guint64          max_tick_stamp)
{
  BseMidiEvent *event;
  gboolean need_wakeup = FALSE;
  
  if (!self->events)
    return FALSE;
  
  event = self->events->data;
  if (event->tick_stamp <= max_tick_stamp)
    {
      GslTrans *trans = gsl_trans_open ();
      self->events = sfi_ring_remove_node (self->events, self->events);
      switch (event->status)
	{
	case BSE_MIDI_NOTE_ON:
	  DEBUG ("Receiver<%s:%u>: NoteOn  %fHz Velo=%f (stamp:%llu)", self->receiver_name, event->channel,
		 event->data.note.frequency, event->data.note.velocity, event->tick_stamp);
	  activate_voice_L (self, event->channel, event->tick_stamp,
			    event->data.note.frequency,
			    event->data.note.velocity,
			    trans);
	  break;
	case BSE_MIDI_KEY_PRESSURE:
	case BSE_MIDI_NOTE_OFF:
	  DEBUG ("Receiver<%s:%u>: %s %fHz (stamp:%llu)", self->receiver_name, event->channel,
		 event->status == BSE_MIDI_NOTE_OFF ? "NoteOff" : "NotePressure",
		 event->data.note.frequency, event->tick_stamp);
	  adjust_voice_L (self, event->channel, event->tick_stamp,
			  event->data.note.frequency, event->status,
			  event->data.note.velocity, trans);
	  break;
	case BSE_MIDI_CONTROL_CHANGE:
	  DEBUG ("Receiver<%s:%u>: Control %2u Value=%f (stamp:%llu)", self->receiver_name, event->channel,
		 event->data.control.control, event->data.control.value, event->tick_stamp);
	  process_midi_control_L (self, event->channel, event->tick_stamp,
				  event->data.control.control, event->data.control.value,
				  FALSE,
                                  trans);
	  break;
	case BSE_MIDI_X_CONTINUOUS_CHANGE:
	  DEBUG ("Receiver<%s:%u>: X Continuous Control %2u Value=%f (stamp:%llu)", self->receiver_name, event->channel,
		 event->data.control.control, event->data.control.value, event->tick_stamp);
	  process_midi_control_L (self, event->channel, event->tick_stamp,
				  event->data.control.control, event->data.control.value,
                                  TRUE,
				  trans);
	  break;
	case BSE_MIDI_PROGRAM_CHANGE:
	  DEBUG ("Receiver<%s:%u>: Program %u (Value=%f) (stamp:%llu)", self->receiver_name, event->channel,
		 event->data.program, event->data.program / (gfloat) 0x7f, event->tick_stamp);
	  update_midi_signal_L (self, event->channel, event->tick_stamp,
				BSE_MIDI_SIGNAL_PROGRAM, event->data.program / (gfloat) 0x7f,
				trans);
	  break;
	case BSE_MIDI_CHANNEL_PRESSURE:
	  DEBUG ("Receiver<%s:%u>: Channel Pressure Value=%f (stamp:%llu)", self->receiver_name, event->channel,
		 event->data.intensity, event->tick_stamp);
	  update_midi_signal_L (self, event->channel, event->tick_stamp,
				BSE_MIDI_SIGNAL_PRESSURE, event->data.intensity,
				trans);
	  break;
	case BSE_MIDI_PITCH_BEND:
	  DEBUG ("Receiver<%s:%u>: Pitch Bend Value=%f (stamp:%llu)", self->receiver_name, event->channel,
		 event->data.pitch_bend, event->tick_stamp);
	  update_midi_signal_L (self, event->channel, event->tick_stamp,
				BSE_MIDI_SIGNAL_PITCH_BEND, event->data.pitch_bend,
				trans);
	  break;
	default:
	  DEBUG ("Receiver<%s:%u>: Ignoring Event %u (stamp:%llu)", self->receiver_name, event->channel,
		 event->status, event->tick_stamp);
	  break;
	}
      if (self->notifier)
	{
	  self->notifier_events = sfi_ring_prepend (self->notifier_events, event);
	  need_wakeup = TRUE;
	}
      else
	bse_midi_free_event (event);
      gsl_trans_commit (trans);
    }
  else
    return FALSE;
  
#if 0   /* FIXME: wake up midi notifer if necessary */
  if (need_wakeup)
    sfi_thread_wakeup (sfi_thread_main ());
#endif

  return TRUE;
}
