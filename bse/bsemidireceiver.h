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
#ifndef __BSE_MIDI_RECEIVER_H__
#define __BSE_MIDI_RECEIVER_H__

#include        <bse/bseobject.h>
#include        <bse/bsemidievent.h>

G_BEGIN_DECLS

/* --- BSE MIDI structs --- */
typedef struct _BseMidiMonoSynth BseMidiMonoSynth;
typedef struct _BseMidiVoice     BseMidiVoice;
typedef struct {
  guint             midi_channel;
  BseMidiMonoSynth *msynth;
  guint             n_voices;
  BseMidiVoice    **voices;
} BseMidiChannel;
struct _BseMidiReceiver
{
  gchar		    *receiver_name;

  guint		     n_cmodules;
  GslModule	   **cmodules;                  /* control signals */
  
  /*< private >*/
  gpointer	     ctrl_slot_array;	        /* BSA of BseMidiControlSlot* */
  gpointer           midi_channel_array;        /* BSA of BseMidiChannel */
  SfiRing	    *events;                    /* contains BseMidiEvent* */
  guint		     ref_count;
  BseMidiNotifier   *notifier;
  SfiRing	    *notifier_events;
};


/* --- API --- */
#define	BSE_MIDI_CONTROL_MODULE_N_CHANNELS		   (4)
#define	BSE_MIDI_VOICE_MODULE_N_CHANNELS		   (4)
#define	BSE_MIDI_VOICE_N_CHANNELS			   (3)
BseMidiReceiver* bse_midi_receiver_new                     (const gchar       *receiver_name);
BseMidiReceiver* bse_midi_receiver_ref                     (BseMidiReceiver   *self);
void             bse_midi_receiver_unref                   (BseMidiReceiver   *self);
void             bse_midi_receiver_push_event              (BseMidiReceiver   *self,
                                                            BseMidiEvent      *event);
void             bse_midi_receiver_process_events          (BseMidiReceiver   *self,
                                                            guint64            max_tick_stamp);
GslModule*       bse_midi_receiver_retrieve_control_module (BseMidiReceiver   *self,
                                                            guint              midi_channel,
                                                            BseMidiSignalType  signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS],
                                                            GslTrans          *trans);
void             bse_midi_receiver_discard_control_module  (BseMidiReceiver   *self,
                                                            GslModule         *cmodule,
                                                            GslTrans          *trans);
GslModule*       bse_midi_receiver_retrieve_mono_voice     (BseMidiReceiver   *self,
                                                            guint              midi_channel,
                                                            GslTrans          *trans);
void             bse_midi_receiver_discard_mono_voice      (BseMidiReceiver   *self,
                                                            guint              midi_channel,
                                                            GslModule         *fmodule,
                                                            GslTrans          *trans);
guint            bse_midi_receiver_create_poly_voice       (BseMidiReceiver   *self,
                                                            guint              midi_channel,
                                                            GslTrans          *trans);
void             bse_midi_receiver_discard_poly_voice      (BseMidiReceiver   *self,
                                                            guint              midi_channel,
                                                            guint              voice_id,
                                                            GslTrans          *trans);
GslModule*       bse_midi_receiver_get_poly_voice_input    (BseMidiReceiver   *self,
                                                            guint              midi_channel,
                                                            guint              voice_id);
GslModule*       bse_midi_receiver_get_poly_voice_output   (BseMidiReceiver   *self,
                                                            guint              midi_channel,
                                                            guint              voice_id);
GslModule*       bse_midi_receiver_create_sub_voice        (BseMidiReceiver   *self,
                                                            guint              midi_channel,
                                                            guint              voice_id,
                                                            GslTrans          *trans);
void             bse_midi_receiver_discard_sub_voice       (BseMidiReceiver   *self,
                                                            guint              midi_channel,
                                                            guint              voice_id,
                                                            GslModule         *fmodule,
                                                            GslTrans          *trans);
void             bse_midi_receiver_set_notifier            (BseMidiReceiver   *self,
                                                            BseMidiNotifier   *notifier);
BseMidiNotifier* bse_midi_receiver_get_notifier            (BseMidiReceiver   *self);
gboolean         bse_midi_receiver_has_notify_events       (BseMidiReceiver   *self);
SfiRing*         bse_midi_receiver_fetch_notify_events     (BseMidiReceiver   *self);
gboolean         bse_midi_receiver_voices_pending          (BseMidiReceiver   *self,
                                                            guint              midi_channel);
void             bse_midi_receiver_enter_farm              (BseMidiReceiver   *self);
void             bse_midi_receiver_farm_distribute_event   (BseMidiEvent      *event);
void             bse_midi_receiver_farm_process_events     (guint64            max_tick_stamp);
void             bse_midi_receiver_leave_farm              (BseMidiReceiver   *self);


/* --- internal --- */
void		 _bse_midi_init			(void);


G_END_DECLS

#endif /* __BSE_MIDI_RECEIVER_H__ */
