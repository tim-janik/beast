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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef	BSE_MIDI_MAX_CHANNELS			// FIXME
#define  BSE_MIDI_MAX_CHANNELS           (16)	// FIXME
#endif

/* --- BSE MIDI structs --- */
typedef struct _BseMidiEvent     BseMidiEvent;
typedef struct _BseMidiNote      BseMidiNote;
struct _BseMidiNote
{
  gfloat freq;
  gfloat velocity;	/* 0..+1 */
  gfloat aftertouch;	/* 0..+1 */
};
typedef enum
{
  /* channel voice messages */
  BSE_MIDI_NOTE_OFF             = 0x80,         /* note, velocity */
  BSE_MIDI_NOTE_ON              = 0x90,         /* note, velocity */
  BSE_MIDI_KEY_PRESSURE         = 0xA0,         /* note, intensity */
  BSE_MIDI_CONTROL_CHANGE       = 0xB0,         /* ctl-nr, value */
  BSE_MIDI_PROGRAM_CHANGE       = 0xC0,         /* prg-nr */
  BSE_MIDI_CHANNEL_PRESSURE     = 0xD0,         /* intensity */
  BSE_MIDI_PITCH_BEND           = 0xE0,         /* 7lsb, 7msb */
  /* system common messages */
  BSE_MIDI_SYS_EX               = 0xF0,         /* data... */
  BSE_MIDI_SONG_POINTER         = 0xF2,         /* p1, p2 */
  BSE_MIDI_SONG_SELECT          = 0xF3,         /* song-nr */
  BSE_MIDI_TUNE                 = 0xF6,
  BSE_MIDI_END_EX               = 0xF7,
  /* system realtime messages */
  BSE_MIDI_TIMING_CLOCK         = 0xF8,
  BSE_MIDI_SONG_START           = 0xFA,
  BSE_MIDI_SONG_CONTINUE        = 0xFB,
  BSE_MIDI_SONG_STOP            = 0xFC,
  BSE_MIDI_ACTIVE_SENSING       = 0xFE,
  BSE_MIDI_SYSTEM_RESET         = 0xFF
} BseMidiEventType;
struct _BseMidiEvent
{
  BseMidiEventType status;
  guint            channel;     /* 0 .. 15 if valid */
  guint64          usec_stamp;  /* arrival in micro seconds */
  union {
    struct {
      guint   note;		/* 0..0x7f */
      guint   velocity;		/* or intensity: 0..0x7f */
    }       note;
    struct {
      guint   control;		/* 0..0x7f */
      guint   value;		/* 0..0x7f */
    }       control;
    guint   program;		/* 0..0x7f */
    guint   intensity;		/* 0..0x7f */
    guint   pitch_bend;		/* 0..0x3fff; center: 0x2000 */
    struct {
      guint   n_bytes;
      guint8 *bytes;
    }       sys_ex;
    guint   song_pointer;	/* 0..0x3fff */
    guint   song_number;	/* 0..0x7f */
  } data;
  BseMidiEvent    *next;
};
typedef struct
{
  guint      channel_id;
  GslModule *vmodule;	/* note module */
  GslModule *smodule;	/* input module (switches and suspends) */
  GslModule *omodule;	/* output module */
} BseMidiVoice;
struct _BseMidiReceiver
{
  gchar		  *receiver_name;

  guint		   n_cmodules;
  GslModule	 **cmodules;

  guint		   n_voices;
  BseMidiVoice	  *voices;

  /*< private >*/
  GslRing	  *events;
  BseMidiEventType event_type;	/* event currently being decoded */
  BseMidiEventType running_mode;
  guint		   echannel;	/* channel of current event */
  guint            n_bytes;
  guint8	  *bytes;
  guint		   left_bytes;
  guint		   ref_count;
};


/* --- API --- */
#define	BSE_MIDI_RECEIVER_LOCK(rec)		bse_midi_global_lock ()
#define	BSE_MIDI_RECEIVER_UNLOCK(rec)		bse_midi_global_unlock ()
#define	BSE_MIDI_CONTROL_MODULE_N_CHANNELS	(4)
#define	BSE_MIDI_VOICE_MODULE_N_CHANNELS	(4)
#define	BSE_MIDI_VOICE_N_CHANNELS		(3)
BseMidiReceiver* bse_midi_receiver_new		 (const gchar		*receiver_name);
BseMidiReceiver* bse_midi_receiver_ref		 (BseMidiReceiver	*self);
void		 bse_midi_receiver_unref	 (BseMidiReceiver	*self);
void             bse_midi_receiver_push_data    (BseMidiReceiver	*self,
						 guint		         n_bytes,
						 guint8		        *bytes,
						 guint64	         usec_time);
GslModule*	bse_midi_receiver_retrive_control_module (BseMidiReceiver	*self,
							  guint			 channel_id,
							  BseMidiControlType	 signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS]);
void		bse_midi_receiver_discard_control_module (BseMidiReceiver	*self,
							  GslModule		*cmodule);
guint		bse_midi_reciver_retrive_voice		 (BseMidiReceiver	*self,
							  guint			 channel_id);
void		bse_midi_reciver_discard_voice		 (BseMidiReceiver	*self,
							  guint			 voice_id);
GslModule*	bse_midi_receiver_get_note_module	 (BseMidiReceiver	*self,
							  guint			 voice_id);
GslModule*	bse_midi_receiver_get_input_module	 (BseMidiReceiver	*self,
							  guint			 voice_id);
GslModule*	bse_midi_receiver_get_output_module	 (BseMidiReceiver	*self,
							  guint			 voice_id);

/* --- internal --- */
void		 bse_midi_global_lock		(void);
void		 bse_midi_global_unlock		(void);
     



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MIDI_RECEIVER_H__ */
