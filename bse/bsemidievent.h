/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2001 Tim Janik
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
 *
 * bsemidievent.h: glue structures for midi interface and midi drivers
 */
#ifndef __BSE_MIDI_EVENT_H__
#define __BSE_MIDI_EVENT_H__

#include        <bse/bseobject.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define BSE_TYPE_MIDI_EVENT	(bse_midi_event_get_type ())


/* --- MIDI constants --- */
#define	BSE_MIDI_MAX_CHANNELS		(16)
#define	BSE_MIDI_MAX_VELOCITY		(0x7f)
#define	BSE_MIDI_MAX_AFTERTOUCH		(0x7f)
#define	BSE_MIDI_MAX_PRESSURE		(0x7f)
#define	BSE_MIDI_MAX_PROGRAM		(0x7f)
#define	BSE_MIDI_MAX_CONTROL_VALUE	(0x7f)
#define	BSE_MIDI_MAX_PITCH_BEND		(0x3fff)


/* --- BSE MIDI structs --- */
typedef struct _BseMidiNote		BseMidiNote;
typedef struct _BseMidiChannel		BseMidiChannel;
typedef struct _BseMidiEvent		BseMidiEvent;
typedef enum
{
  /* channel voice messages */
  BSE_MIDI_NOTE_OFF		= 0x80,		/* note, velocity */
  BSE_MIDI_NOTE_ON		= 0x90,		/* note, velocity */
  BSE_MIDI_KEY_PRESSURE		= 0xA0,		/* note, intensity */
  BSE_MIDI_CONTROL_CHANGE	= 0xB0,		/* ctl-nr, value */
  BSE_MIDI_PROGRAM_CHANGE	= 0xC0,		/* prg-nr */
  BSE_MIDI_CHANNEL_PRESSURE	= 0xD0,		/* intensity */
  BSE_MIDI_PITCH_BEND		= 0xE0,		/* 7lsb, 7msb */
  /* system common messages */
  BSE_MIDI_SYS_EX		= 0xF0,		/* data... */
  BSE_MIDI_SONG_POINTER		= 0xF2,		/* p1, p2 */
  BSE_MIDI_SONG_SELECT		= 0xF3,		/* song-nr */
  BSE_MIDI_TUNE			= 0xF6,
  BSE_MIDI_END_EX		= 0xF7,
  /* system realtime messages */
  BSE_MIDI_TIMING_CLOCK		= 0xF8,
  BSE_MIDI_SONG_START		= 0xFA,
  BSE_MIDI_SONG_CONTINUE	= 0xFB,
  BSE_MIDI_SONG_STOP		= 0xFC,
  BSE_MIDI_ACTIVE_SENSING	= 0xFE,
  BSE_MIDI_SYSTEM_RESET		= 0xFF
} BseMidiEventType;
#define	BSE_MIDI_CHANNEL_VOICE_MESSAGE(s)	((s) < 0xf0)
#define	BSE_MIDI_SYSTEM_COMMON_MESSAGE(s)	(((s) & 0xf8) == 0xf0)
#define	BSE_MIDI_SYSTEM_REALTIME_MESSAGE(s)	(((s) & 0xf8) == 0xf8)
struct _BseMidiEvent
{
  BseMidiEventType status;
  guint		   channel;	/* 0 .. 15 if valid */
  guint64	   usec_stamp;	/* arrival in micro seconds */
  union {
    struct {
      guint   note;
      guint   velocity;	/* or intensity */
    }       note;
    struct {
      guint   control;
      guint   value;
    }       control;
    guint   program;
    guint   intensity;
    guint   pitch_bend;
    struct {
      guint   n_bytes;
      guint8 *bytes;
    }       sys_ex;
    guint   song_pointer;
    guint   song_number;
  } data;
  BseMidiEvent    *next;
};
struct _BseMidiNote
{
  guint note;			/* BSE_MIN_NOTE .. BSE_MAX_NOTE */
  guint	velocity;		/* 0 .. 127 */
  guint aftertouch;		/* intensity: 0 .. 127 */
};
struct _BseMidiChannel
{
  guint		 program;		/* 0..127 */
  guint 	 pressure;		/* intensity: 0..127 */
  guint 	 pitch_bend;		/* 0..0x2000..0x4000 */
  guint		 n_notes;
  BseMidiNote	*notes;
  guint		 control_values[128];	/* 0..127 for each */
  /*< private >*/
  guint		 n_alloced_notes;
  guint		 use_count;
};
struct _BseMidiDecoder
{
  guint		   n_channels;
  BseMidiChannel   channels[BSE_MIDI_MAX_CHANNELS];

  /*< private >*/
  GslMutex	   mutex;
  BseMidiEvent    *events;
  BseMidiEventType status;
  BseMidiEventType last_status;
  guint		   echannel;
  guint            n_bytes;
  guint8	  *bytes;
  guint		   left_bytes;
  guint64	   usec_stamp;
};


/* --- API --- */
GType		 bse_midi_event_get_type	  (void);	/* boxed */
BseMidiDecoder*	 bse_midi_decoder_new		  (void);
void		 bse_midi_decoder_destroy	  (BseMidiDecoder	*decoder);
void		 bse_midi_decoder_use_channel	  (BseMidiDecoder	*decoder,
						   guint		 channel);
void		 bse_midi_decoder_unuse_channel   (BseMidiDecoder	*decoder,
						   guint		 channel);


/* --- internal --- */
void		 _bse_midi_set_notifier		  (BseMidiNotifier	*notifier);
BseMidiNotifier* _bse_midi_get_notifier		  (void);


/* --- threaded functions --- */
BseMidiChannel*	 bse_midi_decoder_lock_channel_ASYNC    (BseMidiDecoder	*decoder,
							 guint		 channel);
void		 bse_midi_decoder_unlock_channel_ASYNC  (BseMidiDecoder	*decoder,
							 BseMidiChannel	*channel);
void		 _bse_midi_decoder_push_data_ASYNC	(BseMidiDecoder	*decoder,
							 guint		 n_bytes,
							 guint8		*bytes,
							 guint64         usec_time);
BseMidiEvent*	 _bse_midi_fetch_notify_events_ASYNC	(void);
gboolean	 _bse_midi_has_notify_events_ASYNC	(void);
void		 _bse_midi_free_event_ASYNC	        (BseMidiEvent	*event);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_MIDI_EVENT_H__ */
