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


#define BSE_MIDI_CHANNEL_VOICE_MESSAGE(s)       ((s) < 0xf0)
#define BSE_MIDI_SYSTEM_COMMON_MESSAGE(s)       (((s) & 0xf8) == 0xf0)
#define BSE_MIDI_SYSTEM_REALTIME_MESSAGE(s)     (((s) & 0xf8) == 0xf8)


/* --- prototypes --- */
static void	midi_receiver_process_events_L (BseMidiReceiver        *self,
						guint64                 max_tick_stamp);
static gint	midi_control_slots_compare	(gconstpointer		bsearch_node1, /* key */
						 gconstpointer		bsearch_node2);


/* --- variables --- */
static SfiMutex             midi_mutex = { 0, };
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

static void
receiver_enqueue_event_L (BseMidiReceiver *self,
                          guint64          tick_stamp)
{
  BseMidiEvent *event;
  
  g_return_if_fail (self->event_type & 0x80);
  g_return_if_fail (self->left_bytes == 0);
  
  /* special case completed SysEx */
  if (self->event_type == BSE_MIDI_END_EX)
    self->event_type = BSE_MIDI_SYS_EX;
  event = sfi_new_struct (BseMidiEvent, 1);
  event->status = self->event_type;
  event->channel = self->echannel;
  event->tick_stamp = tick_stamp;
  switch (event->status)
    {
      guint v;
      gint ival;
    case BSE_MIDI_NOTE_OFF:
    case BSE_MIDI_NOTE_ON:
    case BSE_MIDI_KEY_PRESSURE:
      g_return_if_fail (self->n_bytes == 2);
      event->data.note.frequency = bse_note_to_freq (self->bytes[0] & 0x7f);
      ival = self->bytes[1] & 0x7f;
      /* old MIDI devices send velocity=0 instead of note-off */
      if (event->status == BSE_MIDI_NOTE_ON && ival == 0)
        event->status = BSE_MIDI_NOTE_OFF;
      /* some MIDI devices report junk velocity upon note-off */
      if (event->status == BSE_MIDI_NOTE_OFF)
        event->data.note.velocity = 0;
      else
	event->data.note.velocity = ival / (gfloat) 0x7f;
      break;
    case BSE_MIDI_CONTROL_CHANGE:
      g_return_if_fail (self->n_bytes == 2);
      event->data.control.control = self->bytes[0] & 0x7f;
      ival = self->bytes[1] & 0x7f;
      event->data.control.value = ival / (gfloat) 0x7f;
      break;
    case BSE_MIDI_PROGRAM_CHANGE:
      g_return_if_fail (self->n_bytes == 1);
      event->data.program = self->bytes[0] & 0x7f;
      break;
    case BSE_MIDI_CHANNEL_PRESSURE:	/* 0..0x7f */
      g_return_if_fail (self->n_bytes == 1);
      ival = self->bytes[0] & 0x7f;
      event->data.intensity = ival / (gfloat) 0x7f;
      break;
    case BSE_MIDI_PITCH_BEND:	/* 0..0x3fff; center: 0x2000 */
      g_return_if_fail (self->n_bytes == 2);
      ival = self->bytes[0] & 0x7f;
      v = self->bytes[1] & 0x7f;
      ival |= v << 7;
      ival -= 0x2000;	/* pitch bend center */
      event->data.pitch_bend = ival / (gfloat) 0x2000;
      break;
    case BSE_MIDI_SYS_EX:
      event->data.sys_ex.n_bytes = self->n_bytes;
      event->data.sys_ex.bytes = self->bytes;
      self->bytes = NULL;
      break;
    case BSE_MIDI_SONG_POINTER:
      g_return_if_fail (self->n_bytes == 2);
      event->data.song_pointer = self->bytes[0] & 0x7f;
      v = self->bytes[1] & 0x7f;
      event->data.song_pointer |= v << 7;
      break;
    case BSE_MIDI_SONG_SELECT:
      g_return_if_fail (self->n_bytes == 1);
      event->data.song_number = self->bytes[0] & 0x7f;
      break;
    default:
      event->data.sys_ex.n_bytes = 0;
      event->data.sys_ex.bytes = NULL;
      break;
    }
  self->n_bytes = 0;
  self->event_type = 0;
  
  self->events = sfi_ring_insert_sorted (self->events, event, events_cmp);
}

void
bse_midi_receiver_push_data (BseMidiReceiver *self,
                             guint            n_bytes,
                             guint8          *bytes,
                             guint64          usec_systime)
{
  guint64 tick_stamp;
  
  g_return_if_fail (self != NULL);
  if (n_bytes)
    g_return_if_fail (bytes != NULL);
  
  tick_stamp = gsl_engine_tick_stamp_from_systime (usec_systime);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  while (n_bytes)
    {
      if (!self->event_type)    /* decode next event from byte stream */
        {
          guint status = *bytes;
          
          /* check for command/status byte */
          if (status & 0x80)
            {
              if (BSE_MIDI_CHANNEL_VOICE_MESSAGE (status))
                {
                  self->event_type = status & 0xf0;
                  self->echannel = status & 0x0f;
                  self->running_mode = status;          /* remember MIDI running mode */
                }
              else /* system-realtime or system-common */
                {
                  self->event_type = status;
                  self->echannel = ~0;
                  if (BSE_MIDI_SYSTEM_COMMON_MESSAGE (status))
                    self->running_mode = 0;              /* reset MIDI running mode */
                }
              /* we read the command byte */
              n_bytes--;
              bytes++;
            }
          else /* data byte, MIDI running mode command */
            {
              self->event_type = self->running_mode & 0xF0;
              self->echannel = self->running_mode & 0x0F;
            }
          /* setup data byte counter */
          switch (self->event_type)
            {
            case 0:
              /* ignore data byte as long as we don't know the running mode */
              n_bytes--;
              bytes++;
              self->left_bytes = 0;
              break;
            case BSE_MIDI_NOTE_OFF:
            case BSE_MIDI_NOTE_ON:
            case BSE_MIDI_KEY_PRESSURE:         self->left_bytes = 2;   break;
            case BSE_MIDI_CONTROL_CHANGE:       self->left_bytes = 2;   break;
            case BSE_MIDI_PROGRAM_CHANGE:
            case BSE_MIDI_CHANNEL_PRESSURE:     self->left_bytes = 1;   break;
            case BSE_MIDI_PITCH_BEND:           self->left_bytes = 2;   break;
            case BSE_MIDI_SYS_EX:               self->left_bytes = ~0;  break;
            case BSE_MIDI_SONG_POINTER:         self->left_bytes = 2;   break;
            case BSE_MIDI_SONG_SELECT:          self->left_bytes = 1;   break;
            case BSE_MIDI_TUNE:
            case BSE_MIDI_TIMING_CLOCK:
            case BSE_MIDI_SONG_START:
            case BSE_MIDI_SONG_CONTINUE:
            case BSE_MIDI_SONG_STOP:
            case BSE_MIDI_ACTIVE_SENSING:
            case BSE_MIDI_SYSTEM_RESET:         self->left_bytes = 0;   break;
            case BSE_MIDI_END_EX:
            default:
              g_message ("%s: unhandled midi %s byte 0x%02X\n", G_STRLOC,
                         status < 0x80 ? "data" : "command",
                         status);
              self->event_type = 0;
              self->left_bytes = 0;
              break;
            }
        }
      else if (self->left_bytes)        /* self->event_type != 0; read remaining command data */
        {
          /* special casing SYS_EX since we need to read up until end mark */
          if (self->event_type == BSE_MIDI_SYS_EX)
            {
              guint i;
	      
              /* search for end mark */
              for (i = 0; i < n_bytes; i++)
                if (bytes[i] == BSE_MIDI_END_EX)
                  break;
              /* append data bytes */
              if (i)
                {
                  guint n = self->n_bytes;
                  self->n_bytes += i - 1;
                  self->bytes = g_renew (guint8, self->bytes, self->n_bytes + 1);
                  memcpy (self->bytes + n, bytes, i - 1);
                }
              n_bytes -= i;
              bytes += i;
              /* did we find end mark? */
              if (i < n_bytes)
                {
                  self->event_type = BSE_MIDI_END_EX;
                  self->left_bytes = 0;
                }
            }
          else  /* read normal event data bytes */
            {
              guint i = MIN (self->left_bytes, n_bytes);
              guint n = self->n_bytes;
              self->n_bytes += i;
              self->bytes = g_renew (guint8, self->bytes, self->n_bytes);
              memcpy (self->bytes + n, bytes, i);
              self->left_bytes -= i;
              n_bytes -= i;
              bytes += i;
            }
        }
      if (self->event_type != 0 && self->left_bytes == 0)
        receiver_enqueue_event_L (self, tick_stamp);
    }
  midi_receiver_process_events_L (self, tick_stamp);
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
  g_return_if_fail (self != NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  midi_receiver_process_events_L (self, max_tick_stamp);
  BSE_MIDI_RECEIVER_UNLOCK (self);
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
process_midi_control_module (GslModule *module,
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
    process_midi_control_module,        /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    (GslModuleFreeFunc) g_free,         /* free */
    GSL_COST_CHEAP
  };
  MidiCModuleData *cdata;
  GslModule *module;
  guint i;
  
  g_return_val_if_fail (midi_channel < BSE_MIDI_MAX_CHANNELS, NULL);
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
access_midi_control_module (GslModule *module,
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
					       access_midi_control_module,
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


/* --- BseMidiVoice modules --- */
struct _BseMidiVoice
{
  guint      midi_channel;
  GslModule *fmodule;		/* note module */
  GslModule *smodule;		/* input module (switches and suspends) */
  GslModule *omodule;		/* output module (virtual) */
  guint      ref_count;
  gfloat     freq_value;
  gfloat     gate;		/* mutatable while active */
  gfloat     velocity;
  gfloat     aftertouch;	/* mutatable while active */
  gboolean   active;		/* reset in process() method */
  gboolean   note_playing;
  gboolean   sustained;
  gboolean   discarded;
};

static void
process_switch_module (GslModule *module,
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
process_voice_module (GslModule *module,
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
access_voice_module (GslModule *module,
                     gpointer   data)
{
  BseMidiVoice *voice = module->user_data;
  gfloat *values = data;
  
  voice->gate = values[0];
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
  values[1] = aftertouch;
  gsl_trans_add (trans, gsl_flow_job_access (voice->fmodule, tick_stamp,
					     access_voice_module,
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
  voice->note_playing = TRUE;
  voice->sustained = FALSE;
  /* connect modules */
  for (i = 0; i < GSL_MODULE_N_ISTREAMS (voice->smodule); i++)
    gsl_trans_add (trans, gsl_job_connect (voice->smodule, i, voice->omodule, i));
  gsl_trans_add (trans, gsl_flow_job_resume (voice->smodule, tick_stamp));
}

static void
midi_voice_unref (gpointer        data,
		  const GslClass *klass)
{
  BseMidiVoice *voice = data;
  
  g_return_if_fail (voice->ref_count > 0);
  voice->ref_count--;
  if (!voice->ref_count)
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
    process_voice_module,               /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    midi_voice_unref,			/* free */
    GSL_COST_CHEAP
  };
  static const GslClass switch_module_class = {
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_ostreams */
    process_switch_module,              /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    midi_voice_unref,                   /* free */
    GSL_COST_CHEAP
  };
  BseMidiVoice *voice = g_new0 (BseMidiVoice, 1);
  
  voice->midi_channel = midi_channel;
  voice->fmodule = gsl_module_new (&voice_module_class, voice);
  voice->smodule = gsl_module_new (&switch_module_class, voice);
  voice->omodule = gsl_module_new_virtual (BSE_MIDI_VOICE_N_CHANNELS, NULL, NULL);
  voice->ref_count = 2;		/* voice module, switch module */
  voice->freq_value = 0;
  voice->gate = 0;
  voice->velocity = 0;
  voice->aftertouch = 0;
  voice->active = FALSE;
  voice->note_playing = FALSE;
  voice->sustained = FALSE;
  voice->discarded = FALSE;
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
  self->n_voices = 0;
  self->voices = NULL;
  self->n_cmodules = 0;
  self->cmodules = NULL;
  self->ctrl_slot_array = g_bsearch_array_create (&ctrl_slot_config);
  self->events = NULL;
  self->event_type = 0;
  self->running_mode = 0;
  self->echannel = 0;
  self->n_bytes = 0;
  self->bytes = NULL;
  self->left_bytes = 0;
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
  gboolean need_destroy;
  
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->ref_count > 0);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  self->ref_count--;
  need_destroy = self->ref_count == 0;
  if (need_destroy && self->notifier)
    g_object_unref (self->notifier);
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  if (need_destroy)
    {
      guint i;
      
      if (self->n_cmodules)
	g_warning ("destroying MIDI receiver (%p) with active control modules (%u)", self, self->n_cmodules);
      for (i = 0; i < self->n_voices; i++)
	if (self->voices[i])
	  {
	    g_warning ("destroying MIDI receiver (%p) with active voice modules", self);
	    break;
	  }
      g_free (self->receiver_name);
      g_free (self->voices);
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
      g_free (self->bytes);
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
  g_return_val_if_fail (midi_channel < BSE_MIDI_MAX_CHANNELS, NULL);
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

guint
bse_midi_receiver_create_voice (BseMidiReceiver *self,
				guint            midi_channel,
				GslTrans        *trans)
{
  guint i;
  
  g_return_val_if_fail (self != NULL, G_MAXUINT);
  g_return_val_if_fail (midi_channel < BSE_MIDI_MAX_CHANNELS, G_MAXUINT);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  /* find free voice slot */
  for (i = 0; i < self->n_voices; i++)
    if (self->voices[i] == NULL)
      break;
  /* alloc voice slot */
  if (i >= self->n_voices)
    {
      i = self->n_voices++;
      self->voices = g_renew (BseMidiVoice*, self->voices, self->n_voices);
    }
  self->voices[i] = create_midi_voice (midi_channel, trans);
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return i;
}

void
bse_midi_receiver_discard_voice (BseMidiReceiver *self,
				 guint            nth_voice,
				 GslTrans        *trans)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (nth_voice < self->n_voices);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  if (!self->voices[nth_voice])
    {
      BSE_MIDI_RECEIVER_UNLOCK (self);
      g_warning ("MIDI receiver %p has no voice %u", self, nth_voice);
      return;
    }
  destroy_midi_voice (self->voices[nth_voice], trans);
  self->voices[nth_voice] = NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
}

GslModule*
bse_midi_receiver_get_note_module (BseMidiReceiver *self,
                                   guint            nth_voice)
{
  BseMidiVoice *voice;
  GslModule *module;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (nth_voice < self->n_voices, NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  voice = self->voices[nth_voice];
  module = voice ? voice->fmodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return module;
}

GslModule*
bse_midi_receiver_get_input_module (BseMidiReceiver *self,
                                    guint            nth_voice)
{
  BseMidiVoice *voice;
  GslModule *module;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (nth_voice < self->n_voices, NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  voice = self->voices[nth_voice];
  module = voice ? voice->smodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return module;
}

GslModule*
bse_midi_receiver_get_output_module (BseMidiReceiver *self,
                                     guint            nth_voice)
{
  BseMidiVoice *voice;
  GslModule *module;
  
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (nth_voice < self->n_voices, NULL);
  
  BSE_MIDI_RECEIVER_LOCK (self);
  voice = self->voices[nth_voice];
  module = voice ? voice->omodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return module;
}

gboolean
bse_midi_receiver_has_active_voices (BseMidiReceiver *self)
{
  guint i;

  g_return_val_if_fail (self != NULL, FALSE);

  BSE_MIDI_RECEIVER_LOCK (self);
  /* find busy voice */
  for (i = 0; i < self->n_voices; i++)
    {
      BseMidiVoice *voice = self->voices[i];
      if (voice && voice->active)
	break;
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);

  return i < self->n_voices;
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
  
  g_return_if_fail (freq > 0 && velocity > 0);
  
  /* find free voice */
  for (i = 0; i < self->n_voices; i++)
    {
      voice = self->voices[i];
      if (voice && midi_channel == voice->midi_channel && !voice->active)
	break;
    }
  if (i >= self->n_voices)
    {
      BSE_IF_DEBUG (MIDI)
	g_printerr ("MIDI<%s:%u>: no voice available for note-on (%fHz)\n", self->receiver_name, midi_channel, freq);
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
		gfloat           aftertouch,
		gboolean         note_off,
		GslTrans        *trans)
{
  gfloat freq_val = BSE_VALUE_FROM_FREQ (freq);
  BseMidiVoice *voice;
  guint i;
  
  g_return_if_fail (freq > 0 && aftertouch >= 0);
  
  for (i = 0; i < self->n_voices; i++)
    {
      voice = self->voices[i];
      if (voice && midi_channel == voice->midi_channel && voice->active &&
	  voice->note_playing && !GSL_SIGNAL_FREQ_CHANGED (voice->freq_value, freq_val))
	{
	  if (note_off)
	    voice->note_playing = FALSE;
	  break;
	}
    }
  if (i >= self->n_voices)
    {
      BSE_IF_DEBUG (MIDI)
	g_printerr ("MIDI<%s:%u>: no voice available for %s (%fHz)\n", self->receiver_name, midi_channel,
		    note_off ? "note-off" : "aftertouch",
		    freq);
      return;
    }
  /* set voice outputs */
  if (note_off &&	/* sustain check: */
      (BSE_GCONFIG (invert_sustain) ^ (midi_control_get_L (self, midi_channel, BSE_MIDI_SIGNAL_CONTROL_64) >= 0.5)))
    {
      voice->sustained = TRUE;
      change_midi_voice (voice, tick_stamp, TRUE, aftertouch, trans);
    }
  else
    change_midi_voice (voice, tick_stamp, !note_off, aftertouch, trans);
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
  
  for (i = 0; i < self->n_voices; i++)
    {
      voice = self->voices[i];
      if (voice && midi_channel == voice->midi_channel && voice->active)
	{
	  if ((sustained_only && !voice->note_playing && voice->sustained) ||
	      (!sustained_only && (voice->note_playing || voice->sustained)))
	    {
	      voice->note_playing = FALSE;
	      voice->sustained = FALSE;
	      change_midi_voice (voice, tick_stamp, FALSE, 0, trans);
	      count++;
	    }
	}
    }
  if (count)
    {
      BSE_IF_DEBUG (MIDI)
	g_printerr ("MIDI<%s:%u>: Voices Killed: %u\n", self->receiver_name, midi_channel,
		    count);
    }
}

static void
debug_voices_L (BseMidiReceiver *self,
		guint            midi_channel,
		guint64          tick_stamp,
		GslTrans        *trans)
{
  BseMidiVoice *voice;
  guint i;
  
  BSE_IF_DEBUG (MIDI)
    for (i = 0; i < self->n_voices; i++)
      {
	voice = self->voices[i];
	if (voice)
	  g_printerr ("MIDI<%s>: Voice: Channel=%u SynthActive=%u Playing=%u Sustained=%u Freq=%fHz\n", self->receiver_name,
		      voice->midi_channel, voice->active, voice->note_playing, voice->sustained,
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
  BSE_IF_DEBUG (MIDI)
    g_printerr ("MIDI<%s:%u>: Signal %3u Value=%f (%s)\n", self->receiver_name, channel,
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
			GslTrans        *trans)
{
  /* here, we need to translate midi control numbers
   * into BSE MIDI signals. some control numbers affect
   * multiple MIDI signals.
   */
  
  /* all controls are passed literally as BSE_MIDI_SIGNAL_CONTROL_* */
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

static void
midi_receiver_process_events_L (BseMidiReceiver *self,
				guint64          max_tick_stamp)
{
  BseMidiEvent *event;
  GslTrans *trans;
  gboolean need_wakeup = FALSE;
  
  if (!self->events)
    return;
  
  trans = gsl_trans_open ();
  event = self->events->data;
  if (event->tick_stamp <= max_tick_stamp)
    {
      self->events = sfi_ring_remove_node (self->events, self->events);
      switch (event->status)
	{
	case BSE_MIDI_NOTE_ON:
	  BSE_IF_DEBUG (MIDI)
	    g_printerr ("MIDI<%s:%u>: NoteOn  %fHz Velo=%f (stamp:%llu)\n", self->receiver_name, event->channel,
			event->data.note.frequency, event->data.note.velocity, event->tick_stamp);
	  activate_voice_L (self, event->channel, event->tick_stamp,
			    event->data.note.frequency,
			    event->data.note.velocity,
			    trans);
	  break;
	case BSE_MIDI_KEY_PRESSURE:
	case BSE_MIDI_NOTE_OFF:
	  BSE_IF_DEBUG (MIDI)
	    g_printerr ("MIDI<%s:%u>: %s %fHz (stamp:%llu)\n", self->receiver_name, event->channel,
			event->status == BSE_MIDI_NOTE_OFF ? "NoteOff" : "NotePressure",
			event->data.note.frequency, event->tick_stamp);
	  adjust_voice_L (self, event->channel, event->tick_stamp,
			  event->data.note.frequency,
			  event->data.note.velocity, event->status == BSE_MIDI_NOTE_OFF,
			  trans);
	  break;
	case BSE_MIDI_CONTROL_CHANGE:
	  BSE_IF_DEBUG (MIDI)
	    g_printerr ("MIDI<%s:%u>: Control %2u Value=%f (stamp:%llu)\n", self->receiver_name, event->channel,
			event->data.control.control, event->data.control.value, event->tick_stamp);
	  process_midi_control_L (self, event->channel, event->tick_stamp,
				  event->data.control.control, event->data.control.value,
				  trans);
	  break;
	case BSE_MIDI_PROGRAM_CHANGE:
	  BSE_IF_DEBUG (MIDI)
	    g_printerr ("MIDI<%s:%u>: Program %u (Value=%f) (stamp:%llu)\n", self->receiver_name, event->channel,
			event->data.program, event->data.program / (gfloat) 0x7f, event->tick_stamp);
	  update_midi_signal_L (self, event->channel, event->tick_stamp,
				BSE_MIDI_SIGNAL_PROGRAM, event->data.program / (gfloat) 0x7f,
				trans);
	  break;
	case BSE_MIDI_CHANNEL_PRESSURE:
	  BSE_IF_DEBUG (MIDI)
	    g_printerr ("MIDI<%s:%u>: Channel Pressure Value=%f (stamp:%llu)\n", self->receiver_name, event->channel,
			event->data.intensity, event->tick_stamp);
	  update_midi_signal_L (self, event->channel, event->tick_stamp,
				BSE_MIDI_SIGNAL_PRESSURE, event->data.intensity,
				trans);
	  break;
	case BSE_MIDI_PITCH_BEND:
	  BSE_IF_DEBUG (MIDI)
	    g_printerr ("MIDI<%s:%u>: Pitch Bend Value=%f (stamp:%llu)\n", self->receiver_name, event->channel,
			event->data.pitch_bend, event->tick_stamp);
	  update_midi_signal_L (self, event->channel, event->tick_stamp,
				BSE_MIDI_SIGNAL_PITCH_BEND, event->data.pitch_bend,
				trans);
	  break;
	default:
	  BSE_IF_DEBUG (MIDI)
	    g_printerr ("MIDI<%s:%u>: Ignoring Event %u (stamp:%llu)\n", self->receiver_name, event->channel,
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
    }
  gsl_trans_commit (trans);
  
#if 0   /* FIXME: wake up midi notifer if necessary */
  if (need_wakeup)
    sfi_thread_wakeup (sfi_thread_main ());
#endif
}
