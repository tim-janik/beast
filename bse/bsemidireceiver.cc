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
#include "bseengine.h"
#include "gslsignal.h"
#include "bsecxxutils.h"
#include <string.h>
#include <sfi/gbsearcharray.h>
#include <map>

namespace {
using namespace Bse;
using namespace std;

#define	DEBUG(...)              sfi_debug ("midi-receiver", __VA_ARGS__)
#define	DEBUG_EVENTS(...)       sfi_debug ("midi-events", __VA_ARGS__)


#define	BSE_MIDI_RECEIVER_LOCK(self)            GSL_SPIN_LOCK (&midi_mutex)
#define	BSE_MIDI_RECEIVER_UNLOCK(self)          GSL_SPIN_UNLOCK (&midi_mutex)


/* --- midi controls --- */
struct ControlKey {
  guint                  midi_channel;
  BseMidiSignalType      type;
  explicit ControlKey (guint             _mc,
                       BseMidiSignalType _tp) :
    midi_channel (_mc),
    type (_tp) {}
  bool operator< (const ControlKey &k) const
  {
    if (type == k.type)
      return midi_channel < k.midi_channel;
    return type < k.type;
  }
};
struct ControlValue {
  gfloat                 value;
  GSList                *cmodules;
  explicit ControlValue (gfloat  v) :
    value (v),
    cmodules (NULL) {}
};


/* --- voice prototypes --- */
typedef struct VoiceSwitch          VoiceSwitch;
typedef struct VoiceInput           VoiceInput;
typedef std::map<float,VoiceInput*> VoiceInputTable;


/* --- midi channel --- */
struct MidiChannel {
  guint           midi_channel;
  VoiceInput     *vinput;
  guint           n_voices;
  VoiceSwitch   **voices;
  VoiceInputTable voice_input_table;
  MidiChannel (guint mc) :
    midi_channel (mc)
  {
    vinput = NULL;
    n_voices = 0;
    voices = NULL;
  }
  ~MidiChannel()
  {
    if (vinput)
      g_warning ("destroying MIDI channel (%u) with active mono synth", midi_channel);
    for (guint j = 0; j < n_voices; j++)
      if (voices[j])
        g_warning ("destroying MIDI channel (%u) with active voices", midi_channel);
    g_free (voices);
  }
  void  start_note      (guint64         tick_stamp,
                         gfloat          freq,
                         gfloat          velocity,
                         BseTrans       *trans);
  void  adjust_note     (guint64          tick_stamp,
                         gfloat           freq,
                         BseMidiEventType etype,
                         gfloat           velocity,
                         gboolean         sustain_note,
                         BseTrans        *trans);
  void  kill_notes      (guint64          tick_stamp,
                         gboolean         sustained_only,
                         BseTrans        *trans);
  void  debug_notes     (guint64          tick_stamp,
                         BseTrans        *trans);
};
static inline int
midi_channel_compare (const guint        midi_channel,
                      const MidiChannel *c2)
{
  return midi_channel < c2->midi_channel ? -1 : midi_channel > c2->midi_channel;
}


/* --- midi receiver --- */
struct MidiReceiver
{
  typedef std::map<ControlKey,ControlValue>     Controls;
  typedef std::vector<MidiChannel*>             Channels;
  Controls      controls;
  guint		n_cmodules;
  BseModule   **cmodules;                       /* control signals */
  Channels      midi_channels;
  SfiRing      *events;                         /* contains BseMidiEvent* */
  guint		ref_count;
  BseMidiNotifier *notifier;
  SfiRing	  *notifier_events;
public:
  explicit MidiReceiver ()
  {
    n_cmodules = 0;
    cmodules = NULL;
    events = NULL;
    ref_count = 1;
    notifier = NULL;
    notifier_events = NULL;
  }
  ~MidiReceiver()
  {
    g_assert (ref_count == 0);
    for (Channels::iterator it = midi_channels.begin(); it != midi_channels.end(); it++)
      delete *it;
    while (events)
      {
        BseMidiEvent *event = (BseMidiEvent*) sfi_ring_pop_head (&events);
        bse_midi_free_event (event);
      }
    while (notifier_events)
      {
        BseMidiEvent *event = (BseMidiEvent*) sfi_ring_pop_head (&notifier_events);
        bse_midi_free_event (event);
      }
    if (notifier)
      g_object_unref (notifier);
    if (n_cmodules)
      g_warning ("destroying MIDI receiver (%p) with active control modules (%u)", this, n_cmodules);
    g_free (cmodules);
  }
  MidiChannel*
  peek_channel (guint midi_channel)
  {
    Channels::iterator iter =
      binary_lookup (midi_channels.begin(), midi_channels.end(), midi_channel_compare, midi_channel);
    return iter == midi_channels.end() ? NULL : *iter;
  }
  MidiChannel*
  get_channel (guint midi_channel)
  {
    std::pair<Channels::iterator,bool> result =
      binary_lookup_insertion_pos (midi_channels.begin(), midi_channels.end(), midi_channel_compare, midi_channel);
    if (!result.second)
      {
        static guint i = 23478634;
        result.first = midi_channels.insert (result.first, new MidiChannel (midi_channel));
        if (i != midi_channel)
          i = midi_channel;
      }
    return *result.first;
  }
  ControlValue*
  get_control_value (guint             midi_channel,
                     BseMidiSignalType type)
  {
    Controls::iterator it = controls.find (ControlKey (midi_channel, type));
    if (it != controls.end())
      return &it->second;
    else
      return &controls.insert (std::make_pair (ControlKey (midi_channel, type),
                                               ControlValue (bse_midi_signal_default (type)))).first->second;
  }
  gfloat
  get_control (guint             midi_channel,
               BseMidiSignalType type)
  {
    Controls::iterator it = controls.find (ControlKey (midi_channel, type));
    return it != controls.end() ? it->second.value : bse_midi_signal_default (type);
  }
  GSList*
  set_control (guint             midi_channel,
               BseMidiSignalType type,
               gfloat            value)
  {
    ControlValue *v = get_control_value (midi_channel, type);
    if (v->value != value)
      {
        v->value = value;
        return v->cmodules;
      }
    else
      return NULL;
  }
  void
  add_control (guint             midi_channel,
               BseMidiSignalType type,
               BseModule        *module)
  {
    ControlValue *v = get_control_value (midi_channel, type);
    v->cmodules = g_slist_prepend (v->cmodules, module);
  }
  void
  remove_control (guint             midi_channel,
                  BseMidiSignalType type,
                  BseModule        *module)
  {
    ControlValue *v = get_control_value (midi_channel, type);
    v->cmodules = g_slist_remove (v->cmodules, module);
  }
};


/* --- MIDI Control Module --- */
typedef struct
{
  guint             midi_channel;
  gfloat            values[BSE_MIDI_CONTROL_MODULE_N_CHANNELS];
  BseMidiSignalType signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS];
  guint             ref_count;
} MidiCModuleData;

static void
midi_control_module_process (BseModule *module,
			     guint      n_values)
{
  MidiCModuleData *cdata = (MidiCModuleData *) module->user_data;
  guint i;
  
  for (i = 0; i < BSE_MODULE_N_OSTREAMS (module); i++)
    if (module->ostreams[i].connected)
      module->ostreams[i].values = bse_engine_const_values (cdata->values[i]);
}

static BseModule*
create_midi_control_module_L (MidiReceiver      *self,
                              guint              midi_channel,
                              BseMidiSignalType  signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS])
{
  static const BseModuleClass midi_cmodule_class = {
    0,                                  /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_CONTROL_MODULE_N_CHANNELS, /* n_ostreams */
    midi_control_module_process,        /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    (BseModuleFreeFunc) g_free,         /* free */
    BSE_COST_CHEAP
  };
  MidiCModuleData *cdata;
  BseModule *module;
  guint i;
  
  g_return_val_if_fail (signals != NULL, NULL);
  
  cdata = g_new0 (MidiCModuleData, 1);
  cdata->midi_channel = midi_channel;
  for (i = 0; i < BSE_MIDI_CONTROL_MODULE_N_CHANNELS; i++)
    {
      cdata->signals[i] = signals[i];
      cdata->values[i] = self->get_control (midi_channel, cdata->signals[i]);
    }
  cdata->ref_count = 1;
  module = bse_module_new (&midi_cmodule_class, cdata);
  
  return module;
}

typedef struct {
  BseMidiSignalType signal;
  gfloat            value;
} MidiCModuleAccessData;

static void
midi_control_module_access (BseModule *module,
			    gpointer   data)
{
  MidiCModuleData *cdata = (MidiCModuleData *) module->user_data;
  MidiCModuleAccessData *adata = (MidiCModuleAccessData *) data;
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
			     BseTrans         *trans)
{
  MidiCModuleAccessData *adata;
  GSList *slist = modules;
  
  if (!modules)
    return;
  adata = g_new0 (MidiCModuleAccessData, 1);
  adata->signal = signal;
  adata->value = value;
  for (slist = modules; slist; slist = slist->next)
    bse_trans_add (trans, bse_job_flow_access ((BseModule *) slist->data,
					       tick_stamp,
					       midi_control_module_access,
					       adata,
					       slist->next ? NULL : g_free));
}

static gboolean
match_midi_control_module (BseModule         *cmodule,
                           guint              midi_channel,
                           BseMidiSignalType  signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS])
{
  MidiCModuleData *cdata = (MidiCModuleData *) cmodule->user_data;
  gboolean match = TRUE;
  guint i;
  
  for (i = 0; i < BSE_MIDI_CONTROL_MODULE_N_CHANNELS; i++)
    match &= cdata->signals[i] == signals[i];
  match &= cdata->midi_channel == midi_channel;
  
  return match;
}


/* --- VoiceInput module --- */
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

struct VoiceInput
{
  /* module state */
  gfloat          freq_value;
  gfloat          gate;
  gfloat          velocity;
  gfloat          aftertouch;        /* mutatable while within_note */
  VoiceState      vstate;
  /* mono synth */
  guint           ref_count;
  BseModule      *fmodule;	     /* freq module */
  guint64         tick_stamp;        /* time of last event change */
  VoiceState      queue_state;       /* vstate according to jobs queued so far */
  VoiceInput     *next;
  VoiceInputTable::iterator iter;
};

static void
voice_input_module_process (BseModule *module,
                            guint      n_values)
{
  VoiceInput *vinput = (VoiceInput*) module->user_data;
  
  if (BSE_MODULE_OSTREAM (module, 0).connected)
    BSE_MODULE_OSTREAM (module, 0).values = bse_engine_const_values (vinput->freq_value);
  if (BSE_MODULE_OSTREAM (module, 1).connected)
    BSE_MODULE_OSTREAM (module, 1).values = bse_engine_const_values (vinput->gate);
  if (BSE_MODULE_OSTREAM (module, 2).connected)
    BSE_MODULE_OSTREAM (module, 2).values = bse_engine_const_values (vinput->velocity);
  if (BSE_MODULE_OSTREAM (module, 3).connected)
    BSE_MODULE_OSTREAM (module, 3).values = bse_engine_const_values (vinput->aftertouch);
}

typedef struct {
  VoiceChangeType vtype;
  gfloat          freq_value;
  gfloat          velocity;
} VoiceInputData;

static void
voice_input_module_reset (BseModule *module)
{
  VoiceInput *vinput = (VoiceInput*) module->user_data;
  vinput->freq_value = 0;
  vinput->gate = 0;
  vinput->velocity = 0.5;
  vinput->aftertouch = 0.5;
  vinput->vstate = VSTATE_IDLE;
}

static void
voice_input_module_access (BseModule *module,
                           gpointer   data)
{
  VoiceInput *vinput = (VoiceInput*) module->user_data;
  VoiceInputData *mdata = (VoiceInputData *) data;
  
  DEBUG ("Synth<%p:%08llx>: ProcessEvent=%s Freq=%.2fHz",
         vinput, bse_module_tick_stamp (module),
         voice_change_to_string (mdata->vtype),
         BSE_FREQ_FROM_VALUE (mdata->freq_value));
  switch (mdata->vtype)
    {
    case VOICE_ON:
      vinput->freq_value = mdata->freq_value;
      vinput->gate = 1.0;
      vinput->velocity = mdata->velocity;
      vinput->aftertouch = mdata->velocity;
      vinput->vstate = VSTATE_BUSY;
      break;
    case VOICE_PRESSURE:
      if (vinput->vstate == VSTATE_BUSY &&
          GSL_SIGNAL_FREQ_EQUALS (vinput->freq_value, mdata->freq_value))
        vinput->aftertouch = mdata->velocity;
      break;
    case VOICE_SUSTAIN:
      if (vinput->vstate == VSTATE_BUSY &&
          GSL_SIGNAL_FREQ_EQUALS (vinput->freq_value, mdata->freq_value))
        vinput->vstate = VSTATE_SUSTAINED;
      break;
    case VOICE_OFF:
      if (vinput->vstate == VSTATE_BUSY &&
          GSL_SIGNAL_FREQ_EQUALS (vinput->freq_value, mdata->freq_value))
        goto kill_voice;
      break;
    case VOICE_KILL_SUSTAIN:
      if (vinput->vstate == VSTATE_SUSTAINED)
        goto kill_voice;
      break;
    case VOICE_KILL:
    kill_voice:
      vinput->gate = 0.0;
      vinput->vstate = VSTATE_IDLE;
      break;
    }
}

static void
voice_input_table_iter_remove (VoiceInputTable::iterator iter,
                               VoiceInput               *vinput)
{
  VoiceInput *last = NULL, *cur;
  for (cur = iter->second; cur; last = cur, cur = last->next)
    if (cur == vinput)
      {
        if (last)
          last->next = cur->next;
        else
          iter->second = cur->next;
        vinput->next = NULL;
        return;
      }
  g_assert_not_reached ();
}

static void
change_voice_input (VoiceInput      *vinput,
                    VoiceInputTable *table,
                    guint64          tick_stamp,
                    VoiceChangeType  vtype,
                    gfloat           freq_value,
                    gfloat           velocity,
                    BseTrans        *trans)
{
  VoiceInputData mdata;
  
  mdata.vtype = vtype;
  mdata.freq_value = freq_value;
  mdata.velocity = velocity;
  
  bse_trans_add (trans, bse_job_flow_access (vinput->fmodule, tick_stamp,
					     voice_input_module_access,
					     g_memdup (&mdata, sizeof (mdata)), g_free));
  switch (mdata.vtype)
    {
    case VOICE_ON:
      if (table)
        {
          g_assert (vinput->iter == table->end());
          vinput->next = (*table)[freq_value];
          vinput->iter = table->find (freq_value);
          g_assert (vinput->iter != table->end());
          vinput->iter->second = vinput;
        }
      vinput->queue_state = VSTATE_BUSY;
      break;
    case VOICE_PRESSURE:
      if (table)
        g_assert (vinput->iter != table->end());
      break;
    case VOICE_SUSTAIN:
      if (table)
        {
          g_assert (vinput->iter != table->end());
          voice_input_table_iter_remove (vinput->iter, vinput);
          vinput->iter = table->end();
        }
      vinput->queue_state = VSTATE_SUSTAINED;
      break;
    case VOICE_OFF:
      if (table)
        {
          g_assert (vinput->iter != table->end());
          voice_input_table_iter_remove (vinput->iter, vinput);
          vinput->iter = table->end();
        }
      vinput->queue_state = VSTATE_IDLE;
      break;
    case VOICE_KILL_SUSTAIN:
    case VOICE_KILL:
      if (table && vinput->iter != table->end())
        {
          voice_input_table_iter_remove (vinput->iter, vinput);
          vinput->iter = table->end();
        }
      vinput->queue_state = VSTATE_IDLE;
      break;
    }
  vinput->tick_stamp = tick_stamp;
  DEBUG ("Synth<%p:%08llx>: QueueEvent=%s Freq=%.2fHz",
         vinput, tick_stamp,
         voice_change_to_string (vtype),
         BSE_FREQ_FROM_VALUE (freq_value));
}

static void
voice_input_module_free (gpointer        data,
                         const BseModuleClass *klass)
{
  VoiceInput *vinput = (VoiceInput*) data;
  g_assert (vinput->next == NULL);
  delete vinput;
}

static VoiceInput*
create_voice_input (VoiceInputTable *table,
                    BseTrans        *trans)
{
  static const BseModuleClass mono_synth_module_class = {
    0,                                  /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_MODULE_N_CHANNELS,   /* n_ostreams */
    voice_input_module_process,          /* process */
    NULL,                               /* process_defer */
    voice_input_module_reset,           /* reset */
    voice_input_module_free,		/* free */
    BSE_COST_CHEAP
  };
  VoiceInput *vinput = new VoiceInput;
  
  vinput->fmodule = bse_module_new (&mono_synth_module_class, vinput);
  vinput->freq_value = 0;
  vinput->gate = 0;
  vinput->velocity = 0.5;
  vinput->aftertouch = 0.5;
  vinput->vstate = VSTATE_IDLE;
  vinput->ref_count = 1;
  vinput->tick_stamp = 0;
  vinput->queue_state = VSTATE_IDLE;
  vinput->next = NULL;
  vinput->iter = table->end();
  bse_trans_add (trans, bse_job_integrate (vinput->fmodule));
  
  return vinput;
}

static void
destroy_voice_input (VoiceInput      *vinput,
                     VoiceInputTable *table,
                     BseTrans        *trans)
{
  g_return_if_fail (vinput->ref_count == 0);
  
  if (vinput->iter != table->end())
    {
      voice_input_table_iter_remove (vinput->iter, vinput);
      vinput->iter = table->end();
    }
  bse_trans_add (trans, bse_job_discard (vinput->fmodule));
}


/* --- VoiceSwitch module --- */
struct VoiceSwitch
{
  /* module state: */
  volatile gboolean disconnected;       /* a hint towards module currently being idle */
  /* switchable midi voice */
  guint             n_vinputs;
  VoiceInput      **vinputs;
  guint             ref_count;
  BseModule        *smodule;            /* input module (switches and suspends) */
  BseModule        *vmodule;            /* output module (virtual) */
};

static void
voice_switch_module_process (BseModule *module,
                             guint      n_values)
{
  VoiceSwitch *voice = (VoiceSwitch*) module->user_data;
  guint i;
  
  /* dumb pass-through task */
  for (i = 0; i < BSE_MODULE_N_OSTREAMS (module); i++)
    if (BSE_MODULE_OSTREAM (module, i).connected)
      BSE_MODULE_OSTREAM (module, i).values = (gfloat*) BSE_MODULE_IBUFFER (module, i);
  
  /* check Done state on last stream */
  if (BSE_MODULE_IBUFFER (module, BSE_MODULE_N_ISTREAMS (module) - 1)[n_values - 1] >= 1.0)
    {
      BseTrans *trans = bse_trans_open ();
      /* disconnect all inputs */
      bse_trans_add (trans, bse_job_suspend_now (module));
      bse_trans_add (trans, bse_job_kill_inputs (voice->vmodule));
      bse_trans_commit (trans);
      voice->disconnected = TRUE;       /* hint towards possible reuse */
    }
}

static void
voice_switch_module_boundary_check (BseModule *module,
                                    gpointer   data)
{
  VoiceSwitch *voice = (VoiceSwitch*) module->user_data;
  if (!bse_module_has_source (voice->vmodule, 0))
    {
      BseTrans *trans = bse_trans_open ();
      guint i;
      for (i = 0; i < BSE_MODULE_N_ISTREAMS (voice->vmodule); i++)
        bse_trans_add (trans, bse_job_connect (voice->smodule, i, voice->vmodule, i));
      bse_trans_commit (trans);
      voice->disconnected = FALSE;      /* reset hint */
    }
}

static void
activate_voice_switch (VoiceSwitch *voice,
                       guint64      tick_stamp,
                       BseTrans    *trans)
{
  g_return_if_fail (voice->disconnected == TRUE);
  /* make sure the module is connected before tick_stamp */
  bse_trans_add (trans, bse_job_boundary_access (voice->smodule, tick_stamp, voice_switch_module_boundary_check, NULL, NULL));
  /* make sure the module is not suspended at tick_stamp */
  bse_trans_add (trans, bse_job_resume_at (voice->smodule, tick_stamp));
  voice->disconnected = FALSE;  /* reset hint early */
}

static void
voice_switch_module_free (gpointer        data,
                          const BseModuleClass *klass)
{
  VoiceSwitch *voice = (VoiceSwitch*) data;
  
  g_free (voice->vinputs);
  g_free (voice);
}

static VoiceSwitch*
create_voice_switch_module (BseTrans *trans)
{
  static const BseModuleClass switch_module_class = {
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_ostreams */
    voice_switch_module_process,        /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    voice_switch_module_free,           /* free */
    BSE_COST_CHEAP
  };
  VoiceSwitch *voice = g_new0 (VoiceSwitch, 1);
  
  voice->disconnected = TRUE;
  voice->ref_count = 1;
  voice->smodule = bse_module_new (&switch_module_class, voice);
  voice->vmodule = bse_module_new_virtual (BSE_MIDI_VOICE_N_CHANNELS, NULL, NULL);
  bse_trans_add (trans, bse_job_integrate (voice->smodule));
  bse_trans_add (trans, bse_job_integrate (voice->vmodule));
  bse_trans_add (trans, bse_job_suspend_now (voice->smodule));
  
  return voice;
}

static inline gboolean
check_voice_switch_available (VoiceSwitch *voice)
{
  return voice->disconnected;
}

static void
voice_switch_module_commit_accessor (BseModule *module,
                                     gpointer   data)
{
  BseTrans *trans = (BseTrans*) data;
  bse_trans_commit (trans);
}

static void
destroy_voice_switch (VoiceSwitch *voice,
                      BseTrans     *trans)
{
  BseTrans *tmp_trans;
  
  g_return_if_fail (voice->ref_count == 0);
  g_return_if_fail (voice->n_vinputs == 0);
  
  tmp_trans = bse_trans_open ();
  bse_trans_add (tmp_trans, bse_job_discard (voice->smodule));
  bse_trans_add (tmp_trans, bse_job_discard (voice->vmodule));
  /* we can't commit the transaction right away, because the switch
   * module might currently be processing and is about to queue
   * disconnection jobs on the modules we're just discarding.
   * so we use a normal accessor to defer destruction which makes
   * sure that pending disconnect jobs have been processed already.
   */
  bse_trans_add (trans, bse_job_access (voice->smodule, voice_switch_module_commit_accessor, tmp_trans, NULL));
}


/* --- MidiChannel --- */
static inline gboolean
check_voice_input_improvement (VoiceInput *vinput1, /* vinput1 better than vinput2? */
                               VoiceInput *vinput2)
{
  if (vinput1->vstate == vinput2->vstate)
    return vinput1->tick_stamp < vinput2->tick_stamp;
  if (vinput1->vstate == VSTATE_IDLE)
    return TRUE;
  if (vinput1->vstate == VSTATE_SUSTAINED)
    return vinput2->vstate == VSTATE_IDLE ? FALSE : TRUE;
  return FALSE; /* vinput1->vstate == VSTATE_BUSY && vinput1->vstate != vinput2->vstate */
}

void
MidiChannel::start_note (guint64         tick_stamp,
                         gfloat          freq,
                         gfloat          velocity,
                         BseTrans       *trans)
{
  MidiChannel *mchannel = this;
  gfloat freq_val = BSE_VALUE_FROM_FREQ (freq);
  VoiceSwitch *voice, *override_candidate = NULL;
  guint i;
  
  g_return_if_fail (freq > 0);
  
  /* adjust channel global mono synth */
  if (mchannel->vinput)
    change_voice_input (mchannel->vinput, NULL, tick_stamp, VOICE_ON, freq_val, velocity, trans);
  
  /* figure voice from event */
  voice = NULL; // voice numbers on events not currently supported
  /* find free poly voice */
  if (!voice)
    for (i = 0; i < mchannel->n_voices; i++)
      if (mchannel->voices[i] && mchannel->voices[i]->n_vinputs)
        {
          override_candidate = mchannel->voices[i];
          if (check_voice_switch_available (mchannel->voices[i]))
            {
              voice = mchannel->voices[i];
              break;
            }
        }
  /* grab voice to override */
  if (!voice)
    ; // FIXME: voice = override_candidate;
  
  if (voice && voice->n_vinputs)
    {
      /* start note */
      VoiceInput *vinput = voice->vinputs[0];
      /* figure mono synth */
      for (i = 1; i < voice->n_vinputs; i++)
        if (check_voice_input_improvement (voice->vinputs[i], vinput))
          vinput = voice->vinputs[i];
      /* setup voice */
      activate_voice_switch (voice, tick_stamp, trans);
      change_voice_input (vinput, &mchannel->voice_input_table, tick_stamp, VOICE_ON, freq_val, velocity, trans);
    }
  else
    sfi_diag ("MidiChannel(%u): no voice available for note-on (%fHz)", mchannel->midi_channel, freq);
}

void
MidiChannel::adjust_note (guint64         tick_stamp,
                          gfloat           freq,
                          BseMidiEventType etype, /* BSE_MIDI_KEY_PRESSURE or BSE_MIDI_NOTE_OFF */
                          gfloat           velocity,
                          gboolean         sustain_note, /* may be TRUE for BSE_MIDI_NOTE_OFF */
                          BseTrans        *trans)
{
  MidiChannel *mchannel = this;
  VoiceChangeType vctype = etype == BSE_MIDI_KEY_PRESSURE ? VOICE_PRESSURE : (sustain_note ? VOICE_SUSTAIN : VOICE_OFF);
  gfloat freq_val = BSE_VALUE_FROM_FREQ (freq);
  VoiceInput *vinput = NULL;
  
  g_return_if_fail (freq > 0 && velocity >= 0);
  
  /* adjust channel global mono synth */
  if (mchannel->vinput)
    change_voice_input (mchannel->vinput, NULL, tick_stamp, vctype, freq_val, velocity, trans);
  
  /* find corresponding vinput */
  vinput = mchannel->voice_input_table[freq_val];
  
  /* adjust note */
  if (vinput)
    change_voice_input (vinput, &mchannel->voice_input_table, tick_stamp, vctype, freq_val, velocity, trans);
  else
    sfi_diag ("MidiChannel(%u): no voice available for %s (%fHz)", mchannel->midi_channel,
              etype == BSE_MIDI_NOTE_OFF ? "note-off" : "velocity", freq);
}

void
MidiChannel::kill_notes (guint64       tick_stamp,
                         gboolean      sustained_only,
                         BseTrans     *trans)
{
  MidiChannel *mchannel = this;
  guint i, j;
  
  /* adjust channel global voice inputs */
  if (mchannel->vinput && sustained_only && mchannel->vinput->queue_state == VSTATE_SUSTAINED)
    change_voice_input (mchannel->vinput, NULL, tick_stamp, VOICE_KILL_SUSTAIN, 0, 0, trans);
  else if (mchannel->vinput && !sustained_only)
    change_voice_input (mchannel->vinput, NULL, tick_stamp, VOICE_KILL, 0, 0, trans);
  
  /* adjust poly voice inputs */
  for (i = 0; i < mchannel->n_voices; i++)
    {
      VoiceSwitch *voice = mchannel->voices[i];
      if (voice)
        for (j = 0; j < voice->n_vinputs; j++)
          if (sustained_only && voice->vinputs[j]->queue_state == VSTATE_SUSTAINED)
            change_voice_input (voice->vinputs[j], &mchannel->voice_input_table,
                                tick_stamp, VOICE_KILL_SUSTAIN, 0, 0, trans);
          else if (!sustained_only)
            change_voice_input (voice->vinputs[j], &mchannel->voice_input_table,
                                tick_stamp, VOICE_KILL, 0, 0, trans);
    }
}

void
MidiChannel::debug_notes (guint64          tick_stamp,
                          BseTrans        *trans)
{
  MidiChannel *mchannel = this;
  guint i, j;
  for (i = 0; i < mchannel->n_voices; i++)
    {
      VoiceSwitch *voice = mchannel->voices[i];
      if (voice)
        for (j = 0; j < voice->n_vinputs; j++)
          sfi_diag ("MidiChannel(%u):Voice<%p>=%c: Synth<%p:%08llx>: State=%s Queued=%s Freq=%.2fHz",
                    mchannel->midi_channel, voice, voice->disconnected ? 'd' : 'C',
                    voice->vinputs[j], bse_module_tick_stamp (voice->vinputs[j]->fmodule),
                    voice_state_to_string (voice->vinputs[j]->vstate),
                    voice_state_to_string (voice->vinputs[j]->queue_state),
                    BSE_FREQ_FROM_VALUE (voice->vinputs[j]->freq_value));
    }
}

} // namespace anon


/* --- BseMidiReceiver C API --- */
extern "C" {

struct _BseMidiReceiver : public MidiReceiver {
  explicit _BseMidiReceiver () :
    MidiReceiver () {}
};


/* --- prototypes --- */
static gint	midi_receiver_process_event_L  (BseMidiReceiver        *self,
						guint64                 max_tick_stamp);


/* --- variables --- */
static SfiMutex                 midi_mutex = { 0, };
static vector<BseMidiReceiver*> farm_residents;


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
            gconstpointer b,
            gpointer      data)
{
  const BseMidiEvent *e1 = (const BseMidiEvent *) a;
  const BseMidiEvent *e2 = (const BseMidiEvent *) b;
  
  return e1->delta_time < e2->delta_time ? -1 : e1->delta_time != e2->delta_time;
}

void
bse_midi_receiver_enter_farm (BseMidiReceiver *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (find (farm_residents.begin(), farm_residents.end(), self) == farm_residents.end());
  
  BSE_MIDI_RECEIVER_LOCK (self);
  farm_residents.push_back (self);
  BSE_MIDI_RECEIVER_UNLOCK (self);
}

void
bse_midi_receiver_farm_distribute_event (BseMidiEvent *event)
{
  g_return_if_fail (event != NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  for (vector<BseMidiReceiver*>::iterator it = farm_residents.begin(); it != farm_residents.end(); it++)
    (*it)->events = sfi_ring_insert_sorted ((*it)->events, bse_midi_copy_event (event), events_cmp, NULL);
  BSE_MIDI_RECEIVER_UNLOCK (self);
}

void
bse_midi_receiver_farm_process_events (guint64 max_tick_stamp)
{
  gboolean seen_event;
  do
    {
      seen_event = FALSE;
      BSE_MIDI_RECEIVER_LOCK (self);
      for (vector<BseMidiReceiver*>::iterator it = farm_residents.begin(); it != farm_residents.end(); it++)
        seen_event |= midi_receiver_process_event_L (*it, max_tick_stamp);
      BSE_MIDI_RECEIVER_UNLOCK (self);
    }
  while (seen_event);
}

void
bse_midi_receiver_leave_farm (BseMidiReceiver *self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (find (farm_residents.begin(), farm_residents.end(), self) != farm_residents.end());
  
  BSE_MIDI_RECEIVER_LOCK (self);
  farm_residents.erase (find (farm_residents.begin(), farm_residents.end(), self));
  BSE_MIDI_RECEIVER_UNLOCK (self);
}

void
bse_midi_receiver_push_event (BseMidiReceiver *self,
			      BseMidiEvent    *event)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (event != NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  self->events = sfi_ring_insert_sorted (self->events, event, events_cmp, NULL);
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


BseMidiReceiver*
bse_midi_receiver_new (const gchar *receiver_name)  // FIXME
{
  BseMidiReceiver *self;
  
  self = new BseMidiReceiver ();
  
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
  leave_farm = need_destroy && find (farm_residents.begin(),
                                     farm_residents.end(), self) != farm_residents.end();
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  if (need_destroy)
    {
      if (leave_farm)
        bse_midi_receiver_leave_farm (self);
      delete self;
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
	BseMidiEvent *event = (BseMidiEvent *) sfi_ring_pop_head (&self->notifier_events);
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

BseModule*
bse_midi_receiver_retrieve_control_module (BseMidiReceiver  *self,
					   guint             midi_channel,
					   BseMidiSignalType signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS],
					   BseTrans         *trans)
{
  BseModule *cmodule;
  guint i;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  g_return_val_if_fail (signals != NULL, NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  for (i = 0; i < self->n_cmodules; i++)
    {
      cmodule = self->cmodules[i];
      if (match_midi_control_module (cmodule, midi_channel, signals))
        {
          MidiCModuleData *cdata = (MidiCModuleData *) cmodule->user_data;
          cdata->ref_count++;
	  BSE_MIDI_RECEIVER_UNLOCK (self);
          return cmodule;
        }
    }
  cmodule = create_midi_control_module_L (self, midi_channel, signals);
  i = self->n_cmodules++;
  self->cmodules = g_renew (BseModule*, self->cmodules, self->n_cmodules);
  self->cmodules[i] = cmodule;
  bse_trans_add (trans, bse_job_integrate (cmodule));
  self->add_control (midi_channel, signals[0], cmodule);
  if (signals[1] != signals[0])
    self->add_control (midi_channel, signals[1], cmodule);
  if (signals[2] != signals[1] && signals[2] != signals[0])
    self->add_control (midi_channel, signals[2], cmodule);
  if (signals[3] != signals[2] && signals[3] != signals[1] && signals[3] != signals[0])
    self->add_control (midi_channel, signals[3], cmodule);
  BSE_MIDI_RECEIVER_UNLOCK (self);
  return cmodule;
}

void
bse_midi_receiver_discard_control_module (BseMidiReceiver *self,
                                          BseModule       *module,
					  BseTrans        *trans)
{
  guint i;
  
  g_return_if_fail (self != NULL);
  g_return_if_fail (module != NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  for (i = 0; i < self->n_cmodules; i++)
    {
      BseModule *cmodule = self->cmodules[i];
      if (cmodule == module)
        {
          MidiCModuleData *cdata = (MidiCModuleData *) cmodule->user_data;
          g_return_if_fail (cdata->ref_count > 0);
          cdata->ref_count--;
          if (!cdata->ref_count)
            {
	      BseMidiSignalType *signals = cdata->signals;
	      guint midi_channel = cdata->midi_channel;
              self->n_cmodules--;
              self->cmodules[i] = self->cmodules[self->n_cmodules];
              bse_trans_add (trans, bse_job_discard (cmodule));
	      self->remove_control (midi_channel, signals[0], cmodule);
	      if (signals[1] != signals[0])
		self->remove_control (midi_channel, signals[1], cmodule);
	      if (signals[2] != signals[1] && signals[2] != signals[0])
		self->remove_control (midi_channel, signals[2], cmodule);
	      if (signals[3] != signals[2] && signals[3] != signals[1] && signals[3] != signals[0])
		self->remove_control (midi_channel, signals[3], cmodule);
	    }
	  BSE_MIDI_RECEIVER_UNLOCK (self);
          return;
        }
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
  g_warning ("no such control module: %p", module);
}

BseModule*
bse_midi_receiver_retrieve_mono_voice (BseMidiReceiver *self,
                                       guint            midi_channel,
                                       BseTrans        *trans)
{
  MidiChannel *mchannel;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = self->get_channel (midi_channel);
  if (mchannel->vinput)
    mchannel->vinput->ref_count++;
  else
    mchannel->vinput = create_voice_input (&mchannel->voice_input_table, trans);
  BSE_MIDI_RECEIVER_UNLOCK (self);
  return mchannel->vinput->fmodule;
}

void
bse_midi_receiver_discard_mono_voice (BseMidiReceiver *self,
                                      guint            midi_channel,
                                      BseModule       *fmodule,
                                      BseTrans        *trans)
{
  MidiChannel *mchannel;
  
  g_return_if_fail (self != NULL);
  g_return_if_fail (fmodule != NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = self->get_channel (midi_channel);
  if (mchannel->vinput && mchannel->vinput->fmodule == fmodule)
    {
      mchannel->vinput->ref_count--;
      if (!mchannel->vinput->ref_count)
        {
          destroy_voice_input (mchannel->vinput, &mchannel->voice_input_table, trans);
          mchannel->vinput = NULL;
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
                                     BseTrans        *trans)
{
  MidiChannel *mchannel;
  guint i;
  
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (midi_channel > 0, 0);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = self->get_channel (midi_channel);
  /* find free voice slot */
  for (i = 0; i < mchannel->n_voices; i++)
    if (mchannel->voices[i] == NULL)
      break;
  /* alloc voice slot */
  if (i >= mchannel->n_voices)
    {
      i = mchannel->n_voices++;
      mchannel->voices = g_renew (VoiceSwitch*, mchannel->voices, mchannel->n_voices);
    }
  mchannel->voices[i] = create_voice_switch_module (trans);
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return i + 1;
}

void
bse_midi_receiver_discard_poly_voice (BseMidiReceiver *self,
                                      guint            midi_channel,
                                      guint            voice_id,
                                      BseTrans        *trans)
{
  MidiChannel *mchannel;
  VoiceSwitch *voice;
  
  g_return_if_fail (self != NULL);
  g_return_if_fail (midi_channel > 0);
  g_return_if_fail (voice_id > 0);
  voice_id -= 1;
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = self->get_channel (midi_channel);
  voice = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  if (voice)
    {
      g_return_if_fail (voice->ref_count > 0);
      voice->ref_count--;
      if (!voice->ref_count)
        {
          destroy_voice_switch (voice, trans);
          mchannel->voices[voice_id] = NULL;
        }
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
  if (!voice)
    g_warning ("MIDI channel %u has no voice %u", midi_channel, voice_id + 1);
}

BseModule*
bse_midi_receiver_get_poly_voice_input (BseMidiReceiver   *self,
                                        guint              midi_channel,
                                        guint              voice_id)
{
  MidiChannel *mchannel;
  VoiceSwitch *voice;
  BseModule *module;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  g_return_val_if_fail (voice_id > 0, NULL);
  voice_id -= 1;
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = self->get_channel (midi_channel);
  voice = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  module = voice ? voice->smodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  return module;
}

BseModule*
bse_midi_receiver_get_poly_voice_output (BseMidiReceiver   *self,
                                         guint              midi_channel,
                                         guint              voice_id)
{
  MidiChannel *mchannel;
  VoiceSwitch *voice;
  BseModule *module;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  g_return_val_if_fail (voice_id > 0, NULL);
  voice_id -= 1;
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = self->get_channel (midi_channel);
  voice = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  module = voice ? voice->vmodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  return module;
}

BseModule*
bse_midi_receiver_create_sub_voice (BseMidiReceiver   *self,
                                    guint              midi_channel,
                                    guint              voice_id,
                                    BseTrans          *trans)
{
  MidiChannel *mchannel;
  VoiceSwitch *voice;
  BseModule *module = NULL;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (midi_channel > 0, NULL);
  g_return_val_if_fail (voice_id > 0, NULL);
  voice_id -= 1;
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = self->get_channel (midi_channel);
  voice = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  if (voice)
    {
      guint i = voice->n_vinputs++;
      voice->vinputs = g_renew (VoiceInput*, voice->vinputs, voice->n_vinputs);
      voice->vinputs[i] = create_voice_input (&mchannel->voice_input_table, trans);
      voice->ref_count++;
      module = voice->vinputs[i]->fmodule;
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
  return module;
}

void
bse_midi_receiver_discard_sub_voice (BseMidiReceiver   *self,
                                     guint              midi_channel,
                                     guint              voice_id,
                                     BseModule         *fmodule,
                                     BseTrans          *trans)
{
  MidiChannel *mchannel;
  VoiceSwitch *voice;
  guint i, need_unref = FALSE;
  
  g_return_if_fail (self != NULL);
  g_return_if_fail (midi_channel > 0);
  g_return_if_fail (fmodule != NULL);
  g_return_if_fail (voice_id > 0);
  voice_id -= 1;
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = self->get_channel (midi_channel);
  voice = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  if (voice)
    for (i = 0; i < voice->n_vinputs; i++)
      if (voice->vinputs[i]->fmodule == fmodule)
        {
          voice->vinputs[i]->ref_count--;
          if (!voice->vinputs[i]->ref_count)
            {
              destroy_voice_input (voice->vinputs[i], &mchannel->voice_input_table, trans);
              voice->vinputs[i] = voice->vinputs[--voice->n_vinputs];
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
  MidiChannel *mchannel;
  SfiRing *ring = NULL;
  guint i, active = 0;
  
  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (midi_channel > 0, FALSE);
  
  if (self->events)
    return TRUE;
  
  BSE_MIDI_RECEIVER_LOCK (self);
  mchannel = self->get_channel (midi_channel);
  if (mchannel)
    {
      active = active || (mchannel->vinput && (mchannel->vinput->vstate != VSTATE_IDLE ||
                                               mchannel->vinput->queue_state != VSTATE_IDLE));
      /* find busy poly voice */
      for (i = 0; i < mchannel->n_voices && !active; i++)
        active = active || !check_voice_switch_available (mchannel->voices[i]);
    }
  /* find pending events */
  for (ring = self->events; ring && !active; ring = sfi_ring_next (ring, self->events))
    {
      BseMidiEvent *event = (BseMidiEvent *) ring->data;
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
		      BseTrans         *trans)
{
  GSList *signal_modules;
  
  signal_modules = self->set_control (channel, signal, value);
  change_midi_control_modules (signal_modules,
                               tick_stamp,
                               signal,
                               value,
                               trans);
#if 0
  DEBUG ("MidiChannel[%u]: Signal %3u Value=%f (%s)", channel,
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
				     BseTrans         *trans)
{
  gint ival;
  /* LSB part */
  ival = gsl_ftoi (self->get_control (channel, lsb_signal) * 0x7f);
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
				     BseTrans         *trans)
{
  gint ival;
  /* LSB part */
  ival = gsl_ftoi (value * 0x7f);
  /* add MSB part */
  ival |= gsl_ftoi (self->get_control (channel, msb_signal) * 0x7f) << 7;
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
			BseTrans        *trans)
{
  /* here, we need to translate midi control numbers
   * into BSE MIDI signals. some control numbers affect
   * multiple MIDI signals. extra_continuous are used
   * internally to update only continuous signals.
   */
  
  if (extra_continuous)
    {
      /* internal BSE_MIDI_SIGNAL_CONTINUOUS_* change */
      update_midi_signal_L (self, channel, tick_stamp, static_cast<BseMidiSignalType> (64 + control), value, trans);
      return;
    }
  
  /* all MIDI controls are passed literally as BSE_MIDI_SIGNAL_CONTROL_* */
  update_midi_signal_L (self, channel, tick_stamp, static_cast<BseMidiSignalType> (128 + control), value, trans);
  
  if (control < 32)		/* MSB part of continuous 14bit signal */
    update_midi_signal_continuous_msb_L (self, channel, tick_stamp,
					 static_cast<BseMidiSignalType> (control + 64),		/* continuous signal */
					 value,							/* MSB value */
					 static_cast<BseMidiSignalType> (128 + control + 32),	/* LSB signal */
					 trans);
  else if (control < 64)	/* LSB part of continuous 14bit signal */
    update_midi_signal_continuous_lsb_L (self, channel, tick_stamp,
					 static_cast<BseMidiSignalType> (control + 32),		/* continuous signal */
					 static_cast<BseMidiSignalType> (128 + control - 32),	/* MSB signal */
					 value,							/* LSB value */
					 trans);
  else switch (control)
    {
      MidiChannel *mchannel;
    case 64:			/* Damper Pedal Switch (Sustain) */
      mchannel = self->peek_channel (channel);
      if (mchannel && (BSE_GCONFIG (invert_sustain) ^ (value < 0.5)))
	mchannel->kill_notes (tick_stamp, TRUE, trans);
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
      mchannel = self->peek_channel (channel);
      if (mchannel)
        mchannel->kill_notes (tick_stamp, FALSE, trans);
      break;
    case 122:			/* Local Control Switch */
      mchannel = self->peek_channel (channel);
      if (mchannel && value < 0.00006)
        mchannel->debug_notes (tick_stamp, trans);
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
  
  event = (BseMidiEvent *) self->events->data;
  if (event->delta_time <= max_tick_stamp)
    {
      BseTrans *trans = bse_trans_open ();
      self->events = sfi_ring_remove_node (self->events, self->events);
      switch (event->status)
	{
          MidiChannel *mchannel;
        case BSE_MIDI_NOTE_ON:
          mchannel = self->peek_channel (event->channel);
	  DEBUG_EVENTS ("MidiChannel[%u]: NoteOn  %fHz Velo=%f (stamp:%llu)", event->channel,
                        event->data.note.frequency, event->data.note.velocity, event->delta_time);
	  if (mchannel)
            mchannel->start_note (event->delta_time,
                                  event->data.note.frequency,
                                  event->data.note.velocity,
                                  trans);
          else
            sfi_diag ("ignoring note-on (%fHz) for foreign midi channel: %u", event->data.note.frequency, event->channel);
	  break;
	case BSE_MIDI_KEY_PRESSURE:
	case BSE_MIDI_NOTE_OFF:
          mchannel = self->peek_channel (event->channel);
          DEBUG_EVENTS ("MidiChannel[%u]: %s %fHz (stamp:%llu)", event->channel,
                        event->status == BSE_MIDI_NOTE_OFF ? "NoteOff" : "NotePressure",
                        event->data.note.frequency, event->delta_time);
          if (mchannel)
            {
              gboolean sustained_note = event->status == BSE_MIDI_NOTE_OFF &&
                                        (BSE_GCONFIG (invert_sustain) ^
                                         (self->get_control (event->channel, BSE_MIDI_SIGNAL_CONTROL_64) >= 0.5));
              mchannel->adjust_note (event->delta_time,
                                     event->data.note.frequency, event->status,
                                     event->data.note.velocity, sustained_note, trans);
            }
	  break;
	case BSE_MIDI_CONTROL_CHANGE:
	  DEBUG_EVENTS ("MidiChannel[%u]: Control %2u Value=%f (stamp:%llu)", event->channel,
                        event->data.control.control, event->data.control.value, event->delta_time);
	  process_midi_control_L (self, event->channel, event->delta_time,
				  event->data.control.control, event->data.control.value,
				  FALSE,
                                  trans);
	  break;
	case BSE_MIDI_X_CONTINUOUS_CHANGE:
	  DEBUG_EVENTS ("MidiChannel[%u]: X Continuous Control %2u Value=%f (stamp:%llu)", event->channel,
                        event->data.control.control, event->data.control.value, event->delta_time);
	  process_midi_control_L (self, event->channel, event->delta_time,
				  event->data.control.control, event->data.control.value,
                                  TRUE,
				  trans);
	  break;
	case BSE_MIDI_PROGRAM_CHANGE:
	  DEBUG_EVENTS ("MidiChannel[%u]: Program %u (Value=%f) (stamp:%llu)", event->channel,
                        event->data.program, event->data.program / (gfloat) 0x7f, event->delta_time);
	  update_midi_signal_L (self, event->channel, event->delta_time,
				BSE_MIDI_SIGNAL_PROGRAM, event->data.program / (gfloat) 0x7f,
				trans);
	  break;
	case BSE_MIDI_CHANNEL_PRESSURE:
	  DEBUG_EVENTS ("MidiChannel[%u]: Channel Pressure Value=%f (stamp:%llu)", event->channel,
                        event->data.intensity, event->delta_time);
	  update_midi_signal_L (self, event->channel, event->delta_time,
				BSE_MIDI_SIGNAL_PRESSURE, event->data.intensity,
				trans);
	  break;
	case BSE_MIDI_PITCH_BEND:
	  DEBUG_EVENTS ("MidiChannel[%u]: Pitch Bend Value=%f (stamp:%llu)", event->channel,
                        event->data.pitch_bend, event->delta_time);
	  update_midi_signal_L (self, event->channel, event->delta_time,
				BSE_MIDI_SIGNAL_PITCH_BEND, event->data.pitch_bend,
				trans);
	  break;
	default:
	  DEBUG_EVENTS ("MidiChannel[%u]: Ignoring Event %u (stamp:%llu)", event->channel,
                        event->status, event->delta_time);
	  break;
	}
      if (self->notifier)
	{
	  self->notifier_events = sfi_ring_prepend (self->notifier_events, event);
	  need_wakeup = TRUE;
	}
      else
	bse_midi_free_event (event);
      bse_trans_commit (trans);
    }
  else
    return FALSE;
  
#if 0   /* FIXME: wake up midi notifer if necessary */
  if (need_wakeup)
    sfi_thread_wakeup (sfi_thread_main ());
#endif
  
  return TRUE;
}

} // "C"
