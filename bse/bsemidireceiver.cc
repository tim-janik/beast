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


#define BSE_MIDI_CHANNEL_VOICE_MESSAGE(s)       ((s) < 0xf0)
#define BSE_MIDI_SYSTEM_COMMON_MESSAGE(s)       (((s) & 0xf8) == 0xf0)
#define BSE_MIDI_SYSTEM_REALTIME_MESSAGE(s)     (((s) & 0xf8) == 0xf8)



/* --- variables --- */
static GslMutex midi_mutex = { 0, };


/* --- function --- */
void
_bse_midi_init (void)
{
  static gboolean initialized = FALSE;

  g_assert (initialized++ == FALSE);

  gsl_mutex_init (&midi_mutex);
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

  return e1->usec_stamp < e2->usec_stamp ? -1 : e1->usec_stamp != e2->usec_stamp;
}

static void
receiver_enqueue_event_L (BseMidiReceiver *self,
                          guint64          usec_time)
{
  BseMidiEvent *event;

  g_return_if_fail (self->event_type & 0x80);
  g_return_if_fail (self->left_bytes == 0);

  /* special case completed SysEx */
  if (self->event_type == BSE_MIDI_END_EX)
    self->event_type = BSE_MIDI_SYS_EX;
  event = gsl_new_struct (BseMidiEvent, 1);
  event->next = NULL;
  event->status = self->event_type;
  event->channel = self->echannel;
  event->usec_stamp = usec_time;
  switch (event->status)
    {
      guint v;
    case BSE_MIDI_NOTE_OFF:
    case BSE_MIDI_NOTE_ON:
    case BSE_MIDI_KEY_PRESSURE:
      g_return_if_fail (self->n_bytes == 2);
      event->data.note.note = self->bytes[0] & 0x7F;
      event->data.note.velocity = self->bytes[1] & 0x7F;
      /* old MIDI devices send velocity=0 instead of note-off */
      if (event->status == BSE_MIDI_NOTE_ON && event->data.note.velocity == 0)
        event->status = BSE_MIDI_NOTE_OFF;
      /* some MIDI devices report junk velocity upon note-off */
      if (event->status == BSE_MIDI_NOTE_OFF)
        event->data.note.velocity = 0;
      break;
    case BSE_MIDI_CONTROL_CHANGE:
      g_return_if_fail (self->n_bytes == 2);
      event->data.control.control = self->bytes[0] & 0x7F;
      event->data.control.value = self->bytes[1] & 0x7F;
      break;
    case BSE_MIDI_PROGRAM_CHANGE:
      g_return_if_fail (self->n_bytes == 1);
      event->data.program = self->bytes[0] & 0x7F;
      break;
    case BSE_MIDI_CHANNEL_PRESSURE:
      g_return_if_fail (self->n_bytes == 1);
      event->data.intensity = self->bytes[0] & 0x7F;
      break;
    case BSE_MIDI_PITCH_BEND:
      g_return_if_fail (self->n_bytes == 2);
      event->data.pitch_bend = self->bytes[0] & 0x7F;
      v = self->bytes[1] & 0x7F;
      event->data.pitch_bend |= v << 7;
      break;
    case BSE_MIDI_SYS_EX:
      event->data.sys_ex.n_bytes = self->n_bytes;
      event->data.sys_ex.bytes = self->bytes;
      self->bytes = NULL;
      break;
    case BSE_MIDI_SONG_POINTER:
      g_return_if_fail (self->n_bytes == 2);
      event->data.song_pointer = self->bytes[0] & 0x7F;
      v = self->bytes[1] & 0x7F;
      event->data.song_pointer |= v << 7;
      break;
    case BSE_MIDI_SONG_SELECT:
      g_return_if_fail (self->n_bytes == 1);
      event->data.song_number = self->bytes[0] & 0x7F;
      break;
    default:
      event->data.sys_ex.n_bytes = 0;
      event->data.sys_ex.bytes = NULL;
      break;
    }
  self->n_bytes = 0;
  self->event_type = 0;

  self->events = gsl_ring_insert_sorted (self->events, event, events_cmp);
}

void
bse_midi_receiver_push_data (BseMidiReceiver *self,
                             guint            n_bytes,
                             guint8          *bytes,
                             guint64          usec_time)
{
  g_return_if_fail (self != NULL);
  if (n_bytes)
    g_return_if_fail (bytes != NULL);

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
      else /* self->event_type != 0 && self->left_bytes == 0; extract event */
        receiver_enqueue_event_L (self, usec_time);
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
}


/* --- MIDI Control Module --- */
typedef struct
{
  guint              midi_channel_id;
  gfloat             values[BSE_MIDI_CONTROL_MODULE_N_CHANNELS];
  BseMidiControlType signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS];
  guint              ref_count;
} MidiCModuleData;

static void
process_midi_cmodule (GslModule *module,
                      guint      n_values)
{
  MidiCModuleData *cdata = module->user_data;
  guint i;

  for (i = 0; i < GSL_MODULE_N_OSTREAMS (module); i++)
    if (module->ostreams[i].connected)
      module->ostreams[i].values = gsl_engine_const_values (cdata->values[i]);
}

static GslModule*
create_cmodule (guint midi_channel_id,
                guint signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS])
{
  static const GslClass midi_cmodule_class = {
    0,                                  /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_CONTROL_MODULE_N_CHANNELS, /* n_ostreams */
    process_midi_cmodule,               /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    (GslModuleFreeFunc) g_free,         /* free */
    GSL_COST_CHEAP
  };
  MidiCModuleData *cdata;
  GslModule *module;
  guint i;

  g_return_val_if_fail (midi_channel_id < BSE_MIDI_MAX_CHANNELS, NULL);
  g_return_val_if_fail (signals != NULL, NULL);

  cdata = g_new0 (MidiCModuleData, 1);
  cdata->midi_channel_id = midi_channel_id;
  for (i = 0; i < BSE_MIDI_CONTROL_MODULE_N_CHANNELS; i++)
    cdata->signals[i] = signals[i];
  cdata->ref_count = 1;
  module = gsl_module_new (&midi_cmodule_class, cdata);

  return module;
}

typedef struct {
  BseMidiControlType signal;
  gfloat             value;
} MidiCModuleAccessData;

static void
access_control_module (GslModule *module,
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
change_control_modules (GslRing           *modules,
			guint64            tick_stamp,
			BseMidiControlType signal,
			gfloat             value,
			GslTrans          *trans)
{
  MidiCModuleAccessData *adata;
  GslRing *ring = modules;

  if (!modules)
    return;

  adata = g_new0 (MidiCModuleAccessData, 1);
  adata->signal = signal;
  adata->value = value;
  do
    {
      GslRing *next = gsl_ring_walk (modules, ring);
      
      gsl_trans_add (trans, gsl_flow_job_access (ring->data, tick_stamp,
						 access_control_module, adata, next ? NULL : g_free));
      ring = next;
    }
  while (ring);
}

static gboolean
match_cmodule (GslModule *cmodule,
               guint      midi_channel_id,
               guint      signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS])
{
  MidiCModuleData *cdata = cmodule->user_data;
  gboolean match = TRUE;
  guint i;

  for (i = 0; i < BSE_MIDI_CONTROL_MODULE_N_CHANNELS; i++)
    match &= cdata->signals[i] == signals[i];
  match &= cdata->midi_channel_id == midi_channel_id;

  return match;
}


/* --- MIDI Voice Module --- */
typedef struct
{
  gfloat             values[BSE_MIDI_VOICE_MODULE_N_CHANNELS];
} MidiVModuleData;

static void
process_midi_vmodule (GslModule *module,
                      guint      n_values)
{
  MidiVModuleData *vdata = module->user_data;
  guint i;

  for (i = 0; i < GSL_MODULE_N_OSTREAMS (module); i++)
    if (module->ostreams[i].connected)
      module->ostreams[i].values = gsl_engine_const_values (vdata->values[i]);
}

static GslModule*
create_voice_module (void)
{
  static const GslClass midi_vmodule_class = {
    0,                                  /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_MODULE_N_CHANNELS,   /* n_ostreams */
    process_midi_vmodule,               /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    (GslModuleFreeFunc) g_free,         /* free */
    GSL_COST_CHEAP
  };
  MidiVModuleData *vdata;
  GslModule *module;

  vdata = g_new0 (MidiVModuleData, 1);
  module = gsl_module_new (&midi_vmodule_class, vdata);

  return module;
}

static void
access_voice_module (GslModule *module,
                     gpointer   data)
{
  MidiVModuleData *vdata = module->user_data;
  gfloat *values = data;

  if (values[0] >= 0.0)         /* frequency */
    vdata->values[0] = values[0];
  vdata->values[1] = values[1]; /* gate */
  if (values[2] >= 0.0)         /* velocity */
    vdata->values[2] = values[2];
  vdata->values[3] = values[3]; /* aftertouch */
}

static void
change_voice_module (GslModule *module,
                     guint64    tick_stamp,
                     gfloat     freq,           /* maybe -1 for note-off */
                     gfloat     velocity,       /* maybe -1 to keep value */
                     gfloat     aftertouch,
                     GslTrans  *trans)
{
  gfloat values[4];

  values[0] = freq;                     /* frequency output */
  values[1] = freq > 0 ? 1.0 : 0.0;     /* gate ouput */
  values[2] = velocity;                 /* velocity output */
  values[3] = aftertouch;               /* aftertouch output */
  gsl_trans_add (trans,
		 gsl_flow_job_access (module, tick_stamp,
                                      access_voice_module, g_memdup (values, sizeof (values)), g_free));
}


/* --- MIDI Switch Module --- */
typedef struct
{
  gboolean   available;
  GslModule *omodule;
} MidiSModuleData;

static void
process_midi_smodule (GslModule *module,
                      guint      n_values)
{
  MidiSModuleData *sdata = module->user_data;
  guint i;

  /* dumb pass-through task */
  for (i = 0; i < GSL_MODULE_N_OSTREAMS (module); i++)
    if (module->ostreams[i].connected)
      module->ostreams[i].values = (gfloat*) module->istreams[i].values;

  /* check Done state on last stream */
  if (module->ostreams[GSL_MODULE_N_OSTREAMS (module) - 1].values[n_values - 1] >= 1.0)
    {
      GslTrans *trans = gsl_trans_open ();

      /* disconnect all inputs */
      for (i = 0; i < GSL_MODULE_N_OSTREAMS (module); i++)
        gsl_trans_add (trans, gsl_job_disconnect (sdata->omodule, i));
      gsl_trans_add (trans, gsl_job_suspend (module));
      gsl_trans_commit (trans);

      /* may be reconnected again */
      sdata->available = TRUE;
    }
}

static GslModule*
create_switch_module (GslModule *omodule)
{
  static const GslClass midi_smodule_class = {
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_ostreams */
    process_midi_smodule,               /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    (GslModuleFreeFunc) g_free,         /* free */
    GSL_COST_CHEAP
  };
  MidiSModuleData *sdata;
  GslModule *module;

  sdata = g_new0 (MidiSModuleData, 1);
  sdata->available = TRUE;
  sdata->omodule = omodule;
  module = gsl_module_new (&midi_smodule_class, sdata);

  return module;
}


/* --- BseMidiReceiver functions --- */
BseMidiReceiver*
bse_midi_receiver_new (const gchar *receiver_name)
{
  BseMidiReceiver *self = g_new0 (BseMidiReceiver, 1);

  self->receiver_name = g_strdup (receiver_name);
  self->n_cmodules = 0;
  self->cmodules = NULL;
  self->n_voices = 0;
  self->voices = NULL;
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
  BSE_MIDI_RECEIVER_UNLOCK (self);

  if (need_destroy)
    {
      if (self->n_cmodules)
	g_warning ("destroying MIDI receiver (%p) with active control modules (%u)", self, self->n_cmodules);
      if (self->n_voices)
	g_warning ("destroying MIDI receiver (%p) with active voice modules (%u)", self, self->n_voices);
      g_free (self->receiver_name);
      g_free (self->cmodules);
      g_free (self->voices);
      g_message ("FIXME: not freeing MIDI receiver events");
      g_free (self->bytes);
      g_free (self);
    }
}

GslModule*
bse_midi_receiver_retrive_control_module (BseMidiReceiver   *self,
                                          guint              channel_id,
                                          BseMidiControlType signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS])
{
  guint i;
  GslModule *cmodule;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (channel_id < BSE_MIDI_MAX_CHANNELS, NULL);
  g_return_val_if_fail (signals != NULL, NULL);

  BSE_MIDI_RECEIVER_LOCK (self);
  for (i = 0; i < self->n_cmodules; i++)
    {
      cmodule = self->cmodules[i];
      if (match_cmodule (cmodule, channel_id, signals))
        {
          MidiCModuleData *cdata = cmodule->user_data;
          cdata->ref_count++;
	  BSE_MIDI_RECEIVER_UNLOCK (self);
          return cmodule;
        }
    }
  cmodule = create_cmodule (channel_id, signals);
  i = self->n_cmodules++;
  self->cmodules = g_renew (GslModule*, self->cmodules, self->n_cmodules);
  self->cmodules[i] = cmodule;
  gsl_transact (gsl_job_integrate (cmodule), NULL);
  BSE_MIDI_RECEIVER_UNLOCK (self);
  return cmodule;
}

void
bse_midi_receiver_discard_control_module (BseMidiReceiver *self,
                                          GslModule       *module)
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
              self->n_cmodules--;
              self->cmodules[i] = self->cmodules[self->n_cmodules];
	      BSE_MIDI_RECEIVER_UNLOCK (self);
              gsl_transact (gsl_job_discard (cmodule), NULL);
            }
          else
	    BSE_MIDI_RECEIVER_UNLOCK (self);
          return;
        }
    }
  BSE_MIDI_RECEIVER_UNLOCK (self);
  g_warning ("no such control module: %p", module);
}

guint
bse_midi_reciver_retrive_voice (BseMidiReceiver *self,
                                guint            channel_id)
{
  guint i;

  g_return_val_if_fail (self != NULL, G_MAXUINT);
  g_return_val_if_fail (channel_id < BSE_MIDI_MAX_CHANNELS, G_MAXUINT);

  BSE_MIDI_RECEIVER_LOCK (self);
  /* find free voice slot */
  for (i = 0; i < self->n_voices; i++)
    if (self->voices[i].vmodule == NULL)
      break;
  /* alloc voice slot */
  if (i >= self->n_voices)
    {
      i = self->n_voices++;
      self->voices = g_renew (BseMidiVoice, self->voices, self->n_voices);
    }
  self->voices[i].channel_id = channel_id;
  self->voices[i].vmodule = create_voice_module ();
  self->voices[i].omodule = gsl_module_new_virtual (BSE_MIDI_VOICE_N_CHANNELS);
  self->voices[i].smodule = create_switch_module (self->voices[i].omodule);
  gsl_transact (gsl_job_integrate (self->voices[i].vmodule),
                gsl_job_integrate (self->voices[i].smodule),
		gsl_job_suspend (self->voices[i].smodule),
                gsl_job_integrate (self->voices[i].omodule),
                NULL);
  BSE_MIDI_RECEIVER_UNLOCK (self);

  return i;
}

void
bse_midi_reciver_discard_voice (BseMidiReceiver *self,
                                guint            nth_voice)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (nth_voice < self->n_voices);

  BSE_MIDI_RECEIVER_LOCK (self);
  if (!self->voices[nth_voice].vmodule)
    {
      BSE_MIDI_RECEIVER_UNLOCK (self);
      g_warning ("MIDI receiver %p has no voice %u", self, nth_voice);
      return;
    }
  gsl_transact (gsl_job_discard (self->voices[nth_voice].vmodule),
                gsl_job_discard (self->voices[nth_voice].smodule),
                gsl_job_discard (self->voices[nth_voice].omodule),
                NULL);
  self->voices[nth_voice].channel_id = G_MAXUINT;
  self->voices[nth_voice].vmodule = NULL;
  self->voices[nth_voice].smodule = NULL;
  self->voices[nth_voice].omodule = NULL;
  BSE_MIDI_RECEIVER_UNLOCK (self);
}

GslModule*
bse_midi_receiver_get_note_module (BseMidiReceiver *self,
                                   guint            nth_voice)
{
  GslModule *module;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (nth_voice < self->n_voices, NULL);

  BSE_MIDI_RECEIVER_LOCK (self);
  module = self->voices[nth_voice].vmodule;
  BSE_MIDI_RECEIVER_UNLOCK (self);
  
  return module;
}

GslModule*
bse_midi_receiver_get_input_module (BseMidiReceiver *self,
                                    guint            nth_voice)
{
  GslModule *module;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (nth_voice < self->n_voices, NULL);

  BSE_MIDI_RECEIVER_LOCK (self);
  module = self->voices[nth_voice].smodule;
  BSE_MIDI_RECEIVER_UNLOCK (self);

  return module;
}

GslModule*
bse_midi_receiver_get_output_module (BseMidiReceiver *self,
                                     guint            nth_voice)
{
  GslModule *module;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (nth_voice < self->n_voices, NULL);

  BSE_MIDI_RECEIVER_LOCK (self);
  module = self->voices[nth_voice].omodule;
  BSE_MIDI_RECEIVER_UNLOCK (self);

  return module;
}


/* --- event processing --- */
static void
activate_voice (BseMidiReceiver *self,
                guint            channel_id,
                guint64          tick_stamp,
                gfloat           freq,
                gfloat           velocity,
                GslTrans        *trans)
{
  MidiSModuleData *sdata;
  guint i, j;

  g_return_if_fail (freq > 0 && velocity > 0);

  /* find free voice */
  for (i = 0; i < self->n_voices; i++)
    if (channel_id == self->voices[i].channel_id && self->voices[i].vmodule)
      {
        sdata = self->voices[i].smodule->user_data;
        if (sdata->available)
          break;
      }
  if (i >= self->n_voices)
    {
      g_warning ("unable to find free voice to play note at: %fHz", freq); // FIXME: should be user message
      return;
    }
  /* set voice outputs */
  change_voice_module (self->voices[i].vmodule, tick_stamp, freq, velocity, velocity, trans);
  /* connect modules */
  for (j = 0; j < GSL_MODULE_N_ISTREAMS (self->voices[i].smodule); j++)
    gsl_trans_add (trans, gsl_job_connect (self->voices[i].smodule, j, self->voices[i].omodule, j));
  sdata->available = FALSE;
  gsl_trans_add (trans, gsl_flow_job_resume (self->voices[i].smodule, tick_stamp));
}

static void
adjust_voice (BseMidiReceiver *self,
              guint            channel_id,
              guint64          tick_stamp,
              gfloat           freq,
              gfloat           aftertouch,
              gboolean         gate_off,
              GslTrans        *trans)
{
  guint i;

  g_return_if_fail (freq > 0 && aftertouch >= 0);

  for (i = 0; i < self->n_voices; i++)
    if (channel_id == self->voices[i].channel_id && self->voices[i].vmodule)
      {
        MidiSModuleData *sdata = self->voices[i].smodule->user_data;
        MidiVModuleData *vdata = self->voices[i].vmodule->user_data;
        if (!sdata->available && freq == vdata->values[0])
          break;
      }
  if (i >= self->n_voices)
    {
      g_warning ("bad, unable to find allocated voice for aftertouch at: %fHz", freq);
      return;
    }
  /* set voice outputs */
  change_voice_module (self->voices[i].vmodule, tick_stamp, gate_off ? -1 : freq, -1, aftertouch, trans);
}
