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

#define	DEBUG_EVENTS    sfi_debug_keyfunc ("midi-events")
#define	DEBUG	        sfi_debug_keyfunc ("midi-receiver")


#define	BSE_MIDI_RECEIVER_LOCK(self)            GSL_SPIN_LOCK (&midi_mutex)
#define	BSE_MIDI_RECEIVER_UNLOCK(self)          GSL_SPIN_UNLOCK (&midi_mutex)


/* --- prototypes --- */
static gint	midi_receiver_process_event_L  (BseMidiReceiver        *self,
						guint64                 max_tick_stamp);
static gint	midi_control_slots_compare	(gconstpointer		bsearch_node1, /* key */
						 gconstpointer		bsearch_node2);


/* --- variables --- */
static SfiMutex             midi_mutex = { 0, };
static SfiRing             *farm_residents = NULL;


/* --- function --- */
void
_bse_midi_init (void)
{
  static gboolean initialized = FALSE;
  
  g_assert (initialized++ == FALSE);
  
  sfi_mutex_init (&midi_mutex);
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
typedef struct {
  BseMidiSignalType	 type;
  guint			 midi_channel;
  gfloat		 value;
  GSList		*cmodules;
} BseMidiControlSlot;

static const GBSearchConfig ctrl_slot_config = {
  sizeof (BseMidiControlSlot),
  midi_control_slots_compare,
  0, /* G_BSEARCH_ARRAY_ALIGN_POWER2 */
};

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


/* --- BseMidiMonoSynth --- */
typedef enum {
  VOICE_ON = 1,
  VOICE_PRESSURE,
  VOICE_SUSTAIN,
  VOICE_OFF,
  VOICE_KILL_SUSTAIN,
  VOICE_KILL
} VoiceChangeType;
static const char*
voice_change_to_string (VoiceChangeType t)
{
  switch (t)
    {
    case VOICE_ON:              return "voice-on";
    case VOICE_PRESSURE:        return "pressure";
    case VOICE_SUSTAIN:         return "sustain";
    case VOICE_OFF:             return "voice-off";
    case VOICE_KILL_SUSTAIN:    return "kill-sustain";
    case VOICE_KILL:            return "voice-kill";
    }
  return "<invalid>";
}

typedef enum {
  VSTATE_IDLE,
  VSTATE_BUSY,      /* got note-on, waiting for note-off */
  VSTATE_SUSTAINED, /* holding due to sustain, after note-off */
} VoiceState;
static const char*
voice_state_to_string (VoiceState s)
{
  switch (s)
    {
    case VSTATE_IDLE:           return "idle";
    case VSTATE_BUSY:           return "busy";
    case VSTATE_SUSTAINED:      return "sustained";
    }
  return "<invalid>";
}

struct _BseMidiMonoSynth
{
  /* module state */
  gfloat     freq_value;
  gfloat     gate;
  gfloat     velocity;
  gfloat     aftertouch;        /* mutatable while within_note */
  VoiceState vstate;
  /* mono synth */
  guint      ref_count;
  GslModule *fmodule;		/* freq module */
  guint64    tick_stamp;        /* time of last event change */
  VoiceState queue_state;       /* vstate according to jobs queued so far */
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
mono_synth_module_reset (GslModule *module)
{
  BseMidiMonoSynth *msynth = module->user_data;
  msynth->freq_value = 0;
  msynth->gate = 0;
  msynth->velocity = 0.5;
  msynth->aftertouch = 0.5;
  msynth->vstate = VSTATE_IDLE;
}

static void
mono_synth_module_access (GslModule *module,
                          gpointer   data)
{
  BseMidiMonoSynth *msynth = module->user_data;
  BseMidiMonoSynthData *mdata = data;

  DEBUG ("Synth<%p:%08llx>: ProcessEvent=%s Freq=%.2fHz",
         msynth, gsl_module_tick_stamp (module),
         voice_change_to_string (mdata->vtype),
         BSE_FREQ_FROM_VALUE (mdata->freq_value));
  switch (mdata->vtype)
    {
    case VOICE_ON:
      msynth->freq_value = mdata->freq_value;
      msynth->gate = 1.0;
      msynth->velocity = mdata->velocity;
      msynth->aftertouch = mdata->velocity;
      msynth->vstate = VSTATE_BUSY;
      break;
    case VOICE_PRESSURE:
      if (msynth->vstate == VSTATE_BUSY &&
          GSL_SIGNAL_FREQ_EQUALS (msynth->freq_value, mdata->freq_value))
        msynth->aftertouch = mdata->velocity;
      break;
    case VOICE_SUSTAIN:
      if (msynth->vstate == VSTATE_BUSY &&
          GSL_SIGNAL_FREQ_EQUALS (msynth->freq_value, mdata->freq_value))
        msynth->vstate = VSTATE_SUSTAINED;
      break;
    case VOICE_OFF:
      if (msynth->vstate >= VSTATE_BUSY &&
          GSL_SIGNAL_FREQ_EQUALS (msynth->freq_value, mdata->freq_value))
        goto kill_voice;
      break;
    case VOICE_KILL_SUSTAIN:
      if (msynth->vstate == VSTATE_SUSTAINED)
        goto kill_voice;
      break;
    case VOICE_KILL:
    kill_voice:
      msynth->gate = 0.0;
      msynth->vstate = VSTATE_IDLE;
      break;
    }
}

static void
change_mono_synth (BseMidiMonoSynth *msynth,
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
  switch (mdata.vtype)
    {
    case VOICE_ON:
      // g_assert (msynth->queue_state == VSTATE_IDLE);
      msynth->queue_state = VSTATE_BUSY;
    case VOICE_PRESSURE:
      break;
    case VOICE_SUSTAIN:
      // g_assert (msynth->queue_state == VSTATE_BUSY);
      msynth->queue_state = VSTATE_SUSTAINED;
      break;
    case VOICE_KILL_SUSTAIN:
      // g_assert (msynth->queue_state == VSTATE_SUSTAINED);
    case VOICE_OFF:
      // g_assert (msynth->queue_state >= VSTATE_BUSY);
    case VOICE_KILL:
      msynth->queue_state = VSTATE_IDLE;
      break;
    }
  msynth->tick_stamp = tick_stamp;
  DEBUG ("Synth<%p:%08llx>: QueueEvent=%s Freq=%.2fHz",
         msynth, tick_stamp,
         voice_change_to_string (vtype),
         BSE_FREQ_FROM_VALUE (freq_value));
}

static void
mono_synth_module_free (gpointer        data,
                        const GslClass *klass)
{
  BseMidiMonoSynth *msynth = data;

  g_free (msynth);
}

static BseMidiMonoSynth*
create_mono_synth (GslTrans *trans)
{
  static const GslClass mono_synth_module_class = {
    0,                                  /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_MODULE_N_CHANNELS,   /* n_ostreams */
    mono_synth_module_process,          /* process */
    NULL,                               /* process_defer */
    mono_synth_module_reset,            /* reset */
    mono_synth_module_free,		/* free */
    GSL_COST_CHEAP
  };
  BseMidiMonoSynth *msynth = g_new0 (BseMidiMonoSynth, 1);

  msynth->fmodule = gsl_module_new (&mono_synth_module_class, msynth);
  msynth->freq_value = 0;
  msynth->gate = 0;
  msynth->velocity = 0.5;
  msynth->aftertouch = 0.5;
  msynth->vstate = VSTATE_IDLE;
  msynth->ref_count = 1;
  msynth->tick_stamp = 0;
  gsl_trans_add (trans, gsl_job_integrate (msynth->fmodule));
  
  return msynth;
}

static void
destroy_mono_synth (BseMidiMonoSynth *msynth,
		    GslTrans         *trans)
{
  g_return_if_fail (msynth->ref_count == 0);
  
  gsl_trans_add (trans, gsl_job_discard (msynth->fmodule));
}


/* --- BseMidiVoice modules --- */
struct _BseMidiVoice
{
  /* module state: */
  guint64            alive_stamp;       /* module stays connected until alive_stamp */
  gboolean           connect_pending;   /* whether reconnect jobs are pending */
  gboolean           disconnected;      /* a hint towards module currently being non-busy */
  /* switchable midi voice */
  guint              activate_pending;
  guint              n_msynths;
  BseMidiMonoSynth **msynths;
  guint              ref_count;
  GslModule         *smodule;           /* input module (switches and suspends) */
  GslModule         *vmodule;           /* output module (virtual) */
};

static void
switch_module_reset_pending_flag (GslModule *module,
                                  gpointer   data)
{
  BseMidiVoice *voice = module->user_data;
  voice->connect_pending = FALSE;
}

static void
switch_module_check_connections (GslModule *module,
                                 gpointer   data)
{
  BseMidiVoice *voice = module->user_data;
  if (data)
    {
      guint64 *stamp = data;
      voice->alive_stamp = MAX (voice->alive_stamp, *stamp);
    }
  if ((data || gsl_module_tick_stamp (module) <= voice->alive_stamp) &&
      !voice->connect_pending && !gsl_module_has_source (voice->vmodule, 0))
    {
      GslTrans *trans = gsl_trans_open ();
      guint i;
      for (i = 0; i < GSL_MODULE_N_ISTREAMS (voice->vmodule); i++)
        gsl_trans_add (trans, gsl_job_connect (voice->smodule, i, voice->vmodule, i));
      voice->connect_pending = TRUE;
      gsl_trans_add (trans, gsl_job_resume_at (voice->smodule, voice->alive_stamp));
      gsl_trans_add (trans, gsl_job_access (voice->smodule, switch_module_reset_pending_flag, NULL, NULL));
      gsl_trans_commit (trans);
      voice->disconnected = FALSE;      /* reset hint early */
    }
}

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
  if (GSL_MODULE_IBUFFER (module, GSL_MODULE_N_ISTREAMS (module) - 1)[n_values - 1] >= 1.0 &&
      gsl_module_tick_stamp (module) > voice->alive_stamp)
    {
      GslTrans *trans = gsl_trans_open ();
      /* disconnect all inputs */
      gsl_trans_add (trans, gsl_job_suspend_now (module));
      gsl_trans_add (trans, gsl_job_kill_inputs (voice->vmodule));
      gsl_trans_add (trans, gsl_job_access (voice->smodule, switch_module_check_connections, NULL, NULL));
      /* reset all msynths */
      gsl_trans_commit (trans);
      voice->disconnected = TRUE;       /* hint towards possible reuse */
    }
}

static void
switch_module_free (gpointer        data,
                    const GslClass *klass)
{
  BseMidiVoice *voice = data;
  
  g_free (voice);
  g_free (voice->msynths);
}

static BseMidiVoice*
create_switch_module (GslTrans *trans)
{
  static const GslClass switch_module_class = {
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_ostreams */
    switch_module_process,              /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    switch_module_free,                 /* free */
    GSL_COST_CHEAP
  };
  BseMidiVoice *voice = g_new0 (BseMidiVoice, 1);
  
  voice->alive_stamp = 0;
  voice->connect_pending = FALSE;
  voice->disconnected = TRUE;
  voice->activate_pending = 0;
  voice->ref_count = 1;
  voice->smodule = gsl_module_new (&switch_module_class, voice);
  voice->vmodule = gsl_module_new_virtual (BSE_MIDI_VOICE_N_CHANNELS, NULL, NULL);
  gsl_trans_add (trans, gsl_job_integrate (voice->smodule));
  gsl_trans_add (trans, gsl_job_integrate (voice->vmodule));
  gsl_trans_add (trans, gsl_job_suspend_now (voice->smodule));
  
  return voice;
}

typedef struct {
  guint64 avail_stamp;
  BseMidiVoice *voice;
} SwitchModuleActivateData;

static void
free_switch_module_activate_data (gpointer data)
{
  SwitchModuleActivateData *adata = data;
  adata->voice->activate_pending--;
  g_free (adata);
}

static void
activate_switch_module (BseMidiVoice *voice,
                        guint64       tick_stamp,
                        GslTrans     *trans)
{
  SwitchModuleActivateData *adata = g_new (SwitchModuleActivateData, 1);
  adata->avail_stamp = tick_stamp;
  adata->voice = voice;
  voice->activate_pending++;
  /* make sure the module is connected before tick_stamp */
  gsl_trans_add (trans, gsl_job_access (voice->smodule, switch_module_check_connections, adata, free_switch_module_activate_data));
  /* make sure the module is not suspended at tick_stamp */
  gsl_trans_add (trans, gsl_job_resume_at (voice->smodule, tick_stamp));
  voice->disconnected = FALSE;  /* reset hint early */
}

static inline gboolean
check_switch_module_available (BseMidiVoice *voice)
{
  return (voice->disconnected && !voice->activate_pending);
}

static void
commit_trans_accessor (GslModule *module,
                       gpointer   data)
{
  GslTrans *trans = data;
  
  gsl_trans_commit (trans);
}

static void
destroy_switch_module (BseMidiVoice *voice,
                       GslTrans     *trans)
{
  GslTrans *tmp_trans;
  
  g_return_if_fail (voice->ref_count == 0);
  g_return_if_fail (voice->n_msynths == 0);
  
  tmp_trans = gsl_trans_open ();
  gsl_trans_add (tmp_trans, gsl_job_discard (voice->smodule));
  gsl_trans_add (tmp_trans, gsl_job_discard (voice->vmodule));
  /* we can't commit the transaction right away, because the switch
   * module might currently be processing and is about to queue
   * disconnection jobs on the modules we're just discarding.
   * so we use a normal accessor to defer destruction which makes
   * sure that pending disconnect jobs have been processed already.
   */
  gsl_trans_add (trans, gsl_job_access (voice->smodule, commit_trans_accessor, tmp_trans, NULL));
}


/* --- BseMidiChannel --- */
static gint
midi_channels_compare (gconstpointer bsearch_node1, /* key */
                       gconstpointer bsearch_node2)
{
  const BseMidiChannel *ch1 = bsearch_node1;
  const BseMidiChannel *ch2 = bsearch_node2;
  return G_BSEARCH_ARRAY_CMP (ch1->midi_channel, ch2->midi_channel);
}

static const GBSearchConfig midi_channel_config = { sizeof (BseMidiChannel), midi_channels_compare, 0 };

static inline BseMidiChannel*
peek_midi_channel (BseMidiReceiver *self,
                   guint            midi_channel)
{
  BseMidiChannel key;
  key.midi_channel = midi_channel;
  return g_bsearch_array_lookup (self->midi_channel_array, &midi_channel_config, &key);
}

static BseMidiChannel*
get_midi_channel (BseMidiReceiver *self,
                  guint            midi_channel)
{
  BseMidiChannel *ch = peek_midi_channel (self, midi_channel);
  if (!ch)
    {
      BseMidiChannel key = { 0, };
      key.midi_channel = midi_channel;
      self->midi_channel_array = g_bsearch_array_insert (self->midi_channel_array, &midi_channel_config, &key);
      ch = peek_midi_channel (self, midi_channel);
    }
  return ch;
}

static inline gboolean
check_mono_synth_improvement (BseMidiMonoSynth *msynth1, /* msynth1 better than msynth2? */
                              BseMidiMonoSynth *msynth2)
{
  if (msynth1->vstate == msynth2->vstate)
    return msynth1->tick_stamp < msynth2->tick_stamp;
  if (msynth1->vstate == VSTATE_IDLE)
    return TRUE;
  if (msynth1->vstate == VSTATE_SUSTAINED)
    return msynth2 == VSTATE_IDLE ? FALSE : TRUE;
  return FALSE; /* msynth1->vstate == VSTATE_BUSY && msynth1->vstate != msynth2->vstate */
}

static void
midi_channel_start_note_L (BseMidiChannel *mchannel,
                           guint64         tick_stamp,
                           gfloat          freq,
                           gfloat          velocity,
                           GslTrans       *trans)
{
  gfloat freq_val = BSE_VALUE_FROM_FREQ (freq);
  BseMidiVoice *voice, *override_candidate = NULL;
  guint i;
  
  g_return_if_fail (freq > 0);

  /* adjust channel global mono synth */
  if (mchannel->msynth)
    change_mono_synth (mchannel->msynth, tick_stamp, VOICE_ON, freq_val, velocity, trans);
  
  /* figure voice from event */
  voice = NULL; // voice numbers on events not currently supported
  /* find free poly voice */
  if (!voice)
    for (i = 0; i < mchannel->n_voices; i++)
      if (mchannel->voices[i] && mchannel->voices[i]->n_msynths)
        {
          override_candidate = mchannel->voices[i];
          if (check_switch_module_available (mchannel->voices[i]))
            {
              voice = mchannel->voices[i];
              break;
            }
        }
  /* grab voice to override */
  if (!voice)
    ; // FIXME: voice = override_candidate;

  /* start note */
  if (voice && voice->n_msynths)
    {
      BseMidiMonoSynth *msynth = voice->msynths[0];
      /* figure mono synth */
      for (i = 1; i < voice->n_msynths; i++)
        if (check_mono_synth_improvement (voice->msynths[i], msynth))
          msynth = voice->msynths[i];
      /* setup voice */
      activate_switch_module (voice, tick_stamp, trans);
      change_mono_synth (msynth, tick_stamp, VOICE_ON, freq_val, velocity, trans);
    }
  else
    sfi_info ("MidiChannel(%u): no voice available for note-on (%fHz)", mchannel->midi_channel, freq);
}

static void
midi_channel_adjust_note_L (BseMidiChannel  *mchannel,
                            guint64          tick_stamp,
                            gfloat           freq,
                            BseMidiEventType etype, /* BSE_MIDI_KEY_PRESSURE or BSE_MIDI_NOTE_OFF */
                            gfloat           velocity,
                            gboolean         sustain_note, /* may be TRUE for BSE_MIDI_NOTE_OFF */
                            GslTrans        *trans)
{
  VoiceChangeType vctype = etype == BSE_MIDI_KEY_PRESSURE ? VOICE_PRESSURE : (sustain_note ? VOICE_SUSTAIN : VOICE_OFF);
  gfloat freq_val = BSE_VALUE_FROM_FREQ (freq);
  BseMidiVoice *voice;
  BseMidiMonoSynth *msynth = NULL;
  guint i, j;

  g_return_if_fail (freq > 0 && velocity >= 0);

  /* adjust channel global mono synth */
  if (mchannel->msynth)
    change_mono_synth (mchannel->msynth, tick_stamp, vctype, freq_val, velocity, trans);

  /* find corresponding msynth */
  for (i = 0; i < mchannel->n_voices; i++)
    {
      voice = mchannel->voices[i];
      if (voice && voice->n_msynths && !check_switch_module_available (voice))
        for (j = 0; j < voice->n_msynths; j++)
          if (voice->msynths[j]->queue_state == VSTATE_BUSY &&
              !GSL_SIGNAL_FREQ_CHANGED (voice->msynths[j]->freq_value, freq_val))
            {
              msynth = voice->msynths[j];
              i = mchannel->n_voices;   // break outer loop
              break;
            }
    }

  /* adjust note */
  if (msynth)
    change_mono_synth (msynth, tick_stamp, vctype, freq_val, velocity, trans);
  else
    sfi_info ("MidiChannel(%u): no voice available for %s (%fHz)", mchannel->midi_channel,
              etype == BSE_MIDI_NOTE_OFF ? "note-off" : "velocity", freq);
}

static void
midi_channel_kill_notes_L (BseMidiChannel  *mchannel,
                           guint64          tick_stamp,
                           gboolean         sustained_only,
                           GslTrans        *trans)
{
  guint i, j;

  if (mchannel->msynth && sustained_only && mchannel->msynth->queue_state == VSTATE_SUSTAINED)
    change_mono_synth (mchannel->msynth, tick_stamp, VOICE_KILL_SUSTAIN, 0, 0, trans);
  else if (mchannel->msynth && !sustained_only)
    change_mono_synth (mchannel->msynth, tick_stamp, VOICE_KILL, 0, 0, trans);

  for (i = 0; i < mchannel->n_voices; i++)
    {
      BseMidiVoice *voice = mchannel->voices[i];
      if (voice && voice->n_msynths)
        {
          for (j = 0; j < voice->n_msynths; j++)
            if (sustained_only && voice->msynths[j]->queue_state == VSTATE_SUSTAINED)
              change_mono_synth (voice->msynths[j], tick_stamp, VOICE_KILL_SUSTAIN, 0, 0, trans);
            else if (!sustained_only)
              change_mono_synth (voice->msynths[j], tick_stamp, VOICE_KILL, 0, 0, trans);
        }
    }
}

static void
midi_channel_debug_notes_L (BseMidiChannel  *mchannel,
                            guint64          tick_stamp,
                            GslTrans        *trans)
{
  guint i, j;
  for (i = 0; i < mchannel->n_voices; i++)
    {
      BseMidiVoice *voice = mchannel->voices[i];
      if (voice)
        for (j = 0; j < voice->n_msynths; j++)
          sfi_info ("MidiChannel(%u):Voice<%p>=%c: Synth<%p:%08llx>: State=%s Queued=%s Freq=%.2fHz",
                    mchannel->midi_channel, voice, voice->disconnected ? 'd' : 'C',
                    voice->msynths[j], gsl_module_tick_stamp (voice->msynths[j]->fmodule),
                    voice_state_to_string (voice->msynths[j]->vstate),
                    voice_state_to_string (voice->msynths[j]->queue_state),
                    BSE_FREQ_FROM_VALUE (voice->msynths[j]->freq_value));
    }
}


/* --- BseMidiReceiver functions --- */
BseMidiReceiver*
bse_midi_receiver_new (const gchar *receiver_name)
{
  BseMidiReceiver *self;
  
  g_return_val_if_fail (receiver_name != NULL, NULL);
  
  self = g_new0 (BseMidiReceiver, 1);
  self->receiver_name = g_strdup (receiver_name);
  self->n_cmodules = 0;
  self->cmodules = NULL;
  self->ctrl_slot_array = g_bsearch_array_create (&ctrl_slot_config);
  self->midi_channel_array = g_bsearch_array_create (&midi_channel_config);
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
      guint i, j;
      if (leave_farm)
        bse_midi_receiver_leave_farm (self);
      for (i = 0; i < g_bsearch_array_get_n_nodes (self->midi_channel_array); i++)
	{
          BseMidiChannel *mchannel = g_bsearch_array_get_nth (self->midi_channel_array, &midi_channel_config, i);
          if (mchannel->msynth)
            g_warning ("destroying MIDI channel (%u) with active mono synth", mchannel->midi_channel);
          for (j = 0; j < mchannel->n_voices; j++)
            if (mchannel->voices[j])
              g_warning ("destroying MIDI channel (%u) with active voices", mchannel->midi_channel);
          g_free (mchannel->voices);
        }
      g_bsearch_array_free (self->midi_channel_array, &midi_channel_config);
      if (self->n_cmodules)
	g_warning ("destroying MIDI receiver (%p) with active control modules (%u)", self, self->n_cmodules);
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
      g_free (self->receiver_name);
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
bse_midi_receiver_retrieve_mono_voice (BseMidiReceiver *self,
                                       guint            midi_channel,
                                       GslTrans        *trans)
{
  BseMidiChannel *mchannel;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = get_midi_channel (self, midi_channel);
  if (mchannel->msynth)
    mchannel->msynth->ref_count++;
  else
    mchannel->msynth = create_mono_synth (trans);
  BSE_MIDI_RECEIVER_UNLOCK (self);
  return mchannel->msynth->fmodule;
}

void
bse_midi_receiver_discard_mono_voice (BseMidiReceiver *self,
                                      guint            midi_channel,
                                      GslModule       *fmodule,
                                      GslTrans        *trans)
{
  BseMidiChannel *mchannel;
  
  g_return_if_fail (self != NULL);
  g_return_if_fail (fmodule != NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = get_midi_channel (self, midi_channel);
  if (mchannel->msynth && mchannel->msynth->fmodule == fmodule)
    {
      mchannel->msynth->ref_count--;
      if (!mchannel->msynth->ref_count)
        {
          destroy_mono_synth (mchannel->msynth, trans);
          mchannel->msynth = NULL;
        }
      BSE_MIDI_RECEIVER_UNLOCK (self);
      return;
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
  g_warning ("no such mono synth module: %p", fmodule);
}

guint
bse_midi_receiver_create_poly_voice (BseMidiReceiver *self,
                                     guint            midi_channel,
                                     GslTrans        *trans)
{
  BseMidiChannel *mchannel;
  guint i;
  
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (midi_channel > 0, 0);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = get_midi_channel (self, midi_channel);
  /* find free voice slot */
  for (i = 0; i < mchannel->n_voices; i++)
    if (mchannel->voices[i] == NULL)
      break;
  /* alloc voice slot */
  if (i >= mchannel->n_voices)
    {
      i = mchannel->n_voices++;
      mchannel->voices = g_renew (BseMidiVoice*, mchannel->voices, mchannel->n_voices);
    }
  mchannel->voices[i] = create_switch_module (trans);
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return i + 1;
}

void
bse_midi_receiver_discard_poly_voice (BseMidiReceiver *self,
                                      guint            midi_channel,
                                      guint            voice_id,
                                      GslTrans        *trans)
{
  BseMidiChannel *mchannel;
  BseMidiVoice *voice;

  g_return_if_fail (self != NULL);
  g_return_if_fail (midi_channel > 0);
  g_return_if_fail (voice_id > 0);
  voice_id -= 1;

  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = get_midi_channel (self, midi_channel);
  voice = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  if (voice)
    {
      g_return_if_fail (voice->ref_count > 0);
      voice->ref_count--;
      if (!voice->ref_count)
        {
          destroy_switch_module (voice, trans);
          mchannel->voices[voice_id] = NULL;
        }
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
  if (!voice)
    g_warning ("MIDI channel %u has no voice %u", midi_channel, voice_id + 1);
}

GslModule*
bse_midi_receiver_get_poly_voice_input (BseMidiReceiver   *self,
                                        guint              midi_channel,
                                        guint              voice_id)
{
  BseMidiChannel *mchannel;
  BseMidiVoice *voice;
  GslModule *module;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  g_return_val_if_fail (voice_id > 0, NULL);
  voice_id -= 1;

  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = get_midi_channel (self, midi_channel);
  voice = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  module = voice ? voice->smodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  return module;
}

GslModule*
bse_midi_receiver_get_poly_voice_output (BseMidiReceiver   *self,
                                         guint              midi_channel,
                                         guint              voice_id)
{
  BseMidiChannel *mchannel;
  BseMidiVoice *voice;
  GslModule *module;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  g_return_val_if_fail (voice_id > 0, NULL);
  voice_id -= 1;

  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = get_midi_channel (self, midi_channel);
  voice = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  module = voice ? voice->vmodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  return module;
}

GslModule*
bse_midi_receiver_create_sub_voice (BseMidiReceiver   *self,
                                    guint              midi_channel,
                                    guint              voice_id,
                                    GslTrans          *trans)
{
  BseMidiChannel *mchannel;
  BseMidiVoice *voice;
  GslModule *module = NULL;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  g_return_val_if_fail (voice_id > 0, NULL);
  voice_id -= 1;

  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = get_midi_channel (self, midi_channel);
  voice = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  if (voice)
    {
      guint i = voice->n_msynths++;
      voice->msynths = g_renew (BseMidiMonoSynth*, voice->msynths, voice->n_msynths);
      voice->msynths[i] = create_mono_synth (trans);
      voice->ref_count++;
      module = voice->msynths[i]->fmodule;
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
  return module;
}

void
bse_midi_receiver_discard_sub_voice (BseMidiReceiver   *self,
                                     guint              midi_channel,
                                     guint              voice_id,
                                     GslModule         *fmodule,
                                     GslTrans          *trans)
{
  BseMidiChannel *mchannel;
  BseMidiVoice *voice;
  guint i, need_unref = FALSE;

  g_return_if_fail (self != NULL);
  g_return_if_fail (midi_channel > 0);
  g_return_if_fail (fmodule != NULL);
  g_return_if_fail (voice_id > 0);
  voice_id -= 1;
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = get_midi_channel (self, midi_channel);
  voice = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  if (voice)
    for (i = 0; i < voice->n_msynths; i++)
      if (voice->msynths[i]->fmodule == fmodule)
        {
          voice->msynths[i]->ref_count--;
          if (!voice->msynths[i]->ref_count)
            {
              destroy_mono_synth (voice->msynths[i], trans);
              voice->msynths[i] = voice->msynths[--voice->n_msynths];
              need_unref = TRUE;
            }
          fmodule = NULL;
          break;
        }
  BSE_MIDI_RECEIVER_UNLOCK (self);
  if (need_unref)
    bse_midi_receiver_discard_poly_voice (self, midi_channel, voice_id + 1, trans);
  if (fmodule)
    g_warning ("MIDI channel %u, poly voice %u, no such sub voice: %p", midi_channel, voice_id, fmodule);
}

gboolean
bse_midi_receiver_voices_pending (BseMidiReceiver *self,
                                  guint            midi_channel)
{
  BseMidiChannel *mchannel;
  SfiRing *ring = NULL;
  guint i, active = 0;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (midi_channel > 0, FALSE);

  if (self->events)
    return TRUE;

  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = get_midi_channel (self, midi_channel);
  if (mchannel)
    {
      active = active || (mchannel->msynth && (mchannel->msynth->vstate != VSTATE_IDLE ||
                                               mchannel->msynth->queue_state != VSTATE_IDLE));
      /* find busy poly voice */
      for (i = 0; i < mchannel->n_voices && !active; i++)
        active = active || !check_switch_module_available (mchannel->voices[i]);
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
      BseMidiChannel *mchannel;
    case 64:			/* Damper Pedal Switch (Sustain) */
      mchannel = peek_midi_channel (self, channel);
      if (mchannel && (BSE_GCONFIG (invert_sustain) ^ (value < 0.5)))
	midi_channel_kill_notes_L (mchannel, tick_stamp, TRUE, trans);
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
      mchannel = peek_midi_channel (self, channel);
      if (mchannel)
        midi_channel_kill_notes_L (mchannel, tick_stamp, FALSE, trans);
      break;
    case 122:			/* Local Control Switch */
      mchannel = peek_midi_channel (self, channel);
      if (mchannel && value < 0.00006)
        midi_channel_debug_notes_L (mchannel, tick_stamp, trans);
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
          BseMidiChannel *mchannel;
        case BSE_MIDI_NOTE_ON:
          mchannel = peek_midi_channel (self, event->channel);
	  DEBUG_EVENTS ("Receiver<%s:%u>: NoteOn  %fHz Velo=%f (stamp:%llu)", self->receiver_name, event->channel,
                        event->data.note.frequency, event->data.note.velocity, event->tick_stamp);
	  if (mchannel)
            midi_channel_start_note_L (mchannel, event->tick_stamp,
                                       event->data.note.frequency,
                                       event->data.note.velocity,
                                       trans);
	  break;
	case BSE_MIDI_KEY_PRESSURE:
	case BSE_MIDI_NOTE_OFF:
          mchannel = peek_midi_channel (self, event->channel);
          DEBUG_EVENTS ("Receiver<%s:%u>: %s %fHz (stamp:%llu)", self->receiver_name, event->channel,
                        event->status == BSE_MIDI_NOTE_OFF ? "NoteOff" : "NotePressure",
                        event->data.note.frequency, event->tick_stamp);
          if (mchannel)
            {
              gboolean sustained_note = event->status == BSE_MIDI_NOTE_OFF &&
                                        (BSE_GCONFIG (invert_sustain) ^
                                         (midi_control_get_L (self, event->channel, BSE_MIDI_SIGNAL_CONTROL_64) >= 0.5));
              midi_channel_adjust_note_L (mchannel, event->tick_stamp,
                                          event->data.note.frequency, event->status,
                                          event->data.note.velocity, sustained_note, trans);
            }
	  break;
	case BSE_MIDI_CONTROL_CHANGE:
	  DEBUG_EVENTS ("Receiver<%s:%u>: Control %2u Value=%f (stamp:%llu)", self->receiver_name, event->channel,
                        event->data.control.control, event->data.control.value, event->tick_stamp);
	  process_midi_control_L (self, event->channel, event->tick_stamp,
				  event->data.control.control, event->data.control.value,
				  FALSE,
                                  trans);
	  break;
	case BSE_MIDI_X_CONTINUOUS_CHANGE:
	  DEBUG_EVENTS ("Receiver<%s:%u>: X Continuous Control %2u Value=%f (stamp:%llu)", self->receiver_name, event->channel,
                        event->data.control.control, event->data.control.value, event->tick_stamp);
	  process_midi_control_L (self, event->channel, event->tick_stamp,
				  event->data.control.control, event->data.control.value,
                                  TRUE,
				  trans);
	  break;
	case BSE_MIDI_PROGRAM_CHANGE:
	  DEBUG_EVENTS ("Receiver<%s:%u>: Program %u (Value=%f) (stamp:%llu)", self->receiver_name, event->channel,
                        event->data.program, event->data.program / (gfloat) 0x7f, event->tick_stamp);
	  update_midi_signal_L (self, event->channel, event->tick_stamp,
				BSE_MIDI_SIGNAL_PROGRAM, event->data.program / (gfloat) 0x7f,
				trans);
	  break;
	case BSE_MIDI_CHANNEL_PRESSURE:
	  DEBUG_EVENTS ("Receiver<%s:%u>: Channel Pressure Value=%f (stamp:%llu)", self->receiver_name, event->channel,
                        event->data.intensity, event->tick_stamp);
	  update_midi_signal_L (self, event->channel, event->tick_stamp,
				BSE_MIDI_SIGNAL_PRESSURE, event->data.intensity,
				trans);
	  break;
	case BSE_MIDI_PITCH_BEND:
	  DEBUG_EVENTS ("Receiver<%s:%u>: Pitch Bend Value=%f (stamp:%llu)", self->receiver_name, event->channel,
                        event->data.pitch_bend, event->tick_stamp);
	  update_midi_signal_L (self, event->channel, event->tick_stamp,
				BSE_MIDI_SIGNAL_PITCH_BEND, event->data.pitch_bend,
				trans);
	  break;
	default:
	  DEBUG_EVENTS ("Receiver<%s:%u>: Ignoring Event %u (stamp:%llu)", self->receiver_name, event->channel,
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
