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
 */
#ifndef __BSE_MIDI_EVENT_H__
#define __BSE_MIDI_EVENT_H__

#include        <bse/bseobject.h>
#include        <bse/bsemidireceiver.h>


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
typedef struct _BseMidiKNote		BseMidiKNote;
typedef struct _BseMidiKanal		BseMidiKanal;
#if 0
typedef struct _BseMidiEvent		BseMidiEvent;
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
#endif
struct _BseMidiKNote
{
  guint note;			/* BSE_MIN_NOTE .. BSE_MAX_NOTE */
  guint	velocity;		/* 0 .. 127 */
  guint aftertouch;		/* intensity: 0 .. 127 */
};
struct _BseMidiKanal
{
  guint		 program;		/* 0..127 */
  guint 	 pressure;		/* intensity: 0..127 */
  guint 	 pitch_bend;		/* 0..0x2000..0x4000 */
  guint		 n_notes;
  BseMidiKNote	*notes;
  guint		 control_values[128];	/* 0..127 for each */
  /*< private >*/
  guint		 n_alloced_notes;
  guint		 use_count;
};
struct _BseMidiDecoder
{
  guint		   n_channels;
  BseMidiKanal   channels[BSE_MIDI_MAX_CHANNELS];

  /*< private >*/
  GslMutex	   mutex;
  BseMidiEvent    *events;
  BseMidiEventType status;
  BseMidiEventType last_status;
  guint		   echannel;
  guint            n_bytes;
  guint8	  *bytes;
  guint		   left_bytes;
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
BseMidiKanal*	 bse_midi_decoder_lock_channel_ASYNC    (BseMidiDecoder	*decoder,
							 guint		 channel);
void		 bse_midi_decoder_unlock_channel_ASYNC  (BseMidiDecoder	*decoder,
							 BseMidiKanal	*channel);
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
