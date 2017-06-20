// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MIDI_EVENT_H__
#define __BSE_MIDI_EVENT_H__

#include <bse/bseobject.hh>

/* --- MIDI constants --- */
#define	BSE_MIDI_MAX_CHANNELS		(99)


/* --- MIDI event types --- */
#define BSE_MIDI_CHANNEL_VOICE_MESSAGE(s)       ((s) < 0x0F0)
#define BSE_MIDI_SYSTEM_COMMON_MESSAGE(s)       (((s) & 0x0F8) == 0x0F0)
#define BSE_MIDI_SYSTEM_REALTIME_MESSAGE(s)     (((s) & 0x0F8) == 0x0F8)
typedef enum
{
  /* channel voice messages */
  BSE_MIDI_NOTE_OFF             = 0x080,        /* 7bit note, 7bit velocity */
  BSE_MIDI_NOTE_ON              = 0x090,        /* 7bit note, 7bit velocity */
  BSE_MIDI_KEY_PRESSURE         = 0x0A0,        /* 7bit note, 7bit intensity */
  BSE_MIDI_CONTROL_CHANGE       = 0x0B0,        /* 7bit ctl-nr, 7bit value */
  BSE_MIDI_PROGRAM_CHANGE       = 0x0C0,        /* 7bit prg-nr */
  BSE_MIDI_CHANNEL_PRESSURE     = 0x0D0,        /* 7bit intensity */
  BSE_MIDI_PITCH_BEND           = 0x0E0,        /* 14bit signed: 7lsb, 7msb */
  /* system common messages */
  BSE_MIDI_SYS_EX               = 0x0F0,        /* data... (without final 0x7F) */
  BSE_MIDI_SONG_POINTER         = 0x0F2,        /* 14bit pointer: 7lsb, 7msb */
  BSE_MIDI_SONG_SELECT          = 0x0F3,        /* 7bit song-nr */
  BSE_MIDI_TUNE                 = 0x0F6,
  BSE_MIDI_END_EX               = 0x0F7,
  /* system realtime messages */
  BSE_MIDI_TIMING_CLOCK         = 0x0F8,
  BSE_MIDI_SONG_START           = 0x0FA,
  BSE_MIDI_SONG_CONTINUE        = 0x0FB,
  BSE_MIDI_SONG_STOP            = 0x0FC,
  BSE_MIDI_ACTIVE_SENSING       = 0x0FE,
  BSE_MIDI_SYSTEM_RESET         = 0x0FF,
  /* midi file meta events */
  BSE_MIDI_SEQUENCE_NUMBER      = 0x100,        /* 16bit sequence number (msb, lsb) */
  BSE_MIDI_TEXT_EVENT           = 0x101,        /* 8bit text */
  BSE_MIDI_COPYRIGHT_NOTICE     = 0x102,        /* 8bit text */
  BSE_MIDI_TRACK_NAME           = 0x103,        /* 8bit text */
  BSE_MIDI_INSTRUMENT_NAME      = 0x104,        /* 8bit text */
  BSE_MIDI_LYRIC                = 0x105,        /* 8bit text */
  BSE_MIDI_MARKER               = 0x106,        /* 8bit text */
  BSE_MIDI_CUE_POINT            = 0x107,        /* 8bit text */
  BSE_MIDI_TEXT_EVENT_08        = 0x108,        /* 8bit text */
  BSE_MIDI_TEXT_EVENT_09        = 0x109,        /* 8bit text */
  BSE_MIDI_TEXT_EVENT_0A        = 0x10A,        /* 8bit text */
  BSE_MIDI_TEXT_EVENT_0B        = 0x10B,        /* 8bit text */
  BSE_MIDI_TEXT_EVENT_0C        = 0x10C,        /* 8bit text */
  BSE_MIDI_TEXT_EVENT_0D        = 0x10D,        /* 8bit text */
  BSE_MIDI_TEXT_EVENT_0E        = 0x10E,        /* 8bit text */
  BSE_MIDI_TEXT_EVENT_0F        = 0x10F,        /* 8bit text */
  BSE_MIDI_CHANNEL_PREFIX       = 0x120,        /* 8bit channel number (0..15) */
  BSE_MIDI_END_OF_TRACK         = 0x12F,
  BSE_MIDI_SET_TEMPO            = 0x151,        /* 24bit usecs-per-quarter-note (msb first) */
  BSE_MIDI_SMPTE_OFFSET         = 0x154,        /* 8bit hour, minute, second, frame, 100th-frame-fraction */
  BSE_MIDI_TIME_SIGNATURE       = 0x158,        /* 8bit numerator, -ld(1/denominator), metro-clocks, 32nd-npq */
  BSE_MIDI_KEY_SIGNATURE        = 0x159,        /* 8bit sharpsflats, majorminor */
  BSE_MIDI_SEQUENCER_SPECIFIC   = 0x17F,        /* manufacturer specific sequencing data */
  /* implementation specific add-ons */
  BSE_MIDI_MULTI_SYS_EX_START   = 0x201,        /* BSE_MIDI_SYS_EX split across multiple events */
  BSE_MIDI_MULTI_SYS_EX_NEXT    = 0x202,        /* continuation, last data byte of final packet is 0xF7 */
  /* BSE specific extra events */
  BSE_MIDI_X_CONTINUOUS_CHANGE  = 0x400
} BseMidiEventType;


/* --- BSE MIDI Event --- */
#define BSE_TYPE_MIDI_EVENT	(bse_midi_event_get_type ())
typedef struct
{
  BseMidiEventType status;
  guint            channel;     /* 1 .. 16 for standard events */
  guint64          delta_time;  /* GSL tick stamp, SMF tpqn or SMTPE */
  union {
    struct {
      gfloat  frequency;
      gfloat  velocity;         /* or intensity: 0..+1 */
    }       note;
    struct {
      guint   control;          /* 0..0x7f */
      gfloat  value;            /* -1..+1 */
    }       control;
    guint   program;            /* 0..0x7f */
    gfloat  intensity;          /* 0..+1 */
    gfloat  pitch_bend;         /* -1..+1 */
    guint   song_pointer;       /* 0..0x3fff */
    guint   song_number;        /* 0..0x7f */
    /* meta event data */
    struct {
      guint8 *bytes;
      guint   n_bytes;
    }       sys_ex;             /* sys-ex variants and sequencer-specific */
    guint   sequence_number;    /* 0..0xffff */
    gchar  *text;
    guint   usecs_pqn;          /* micro seconds per quarter note */
    struct {
      guint8  hour, minute, second;
      guint8  frame, fraction;   /* fraction is always 100th of a frame */
    }       smpte_offset;
    struct {
      guint   denominator;
      guint8  numerator;
      guint8  metro_clocks;     /* # MIDI clocks in a metronome click */
      guint8  notated_32nd;     /* # of notated 32nd notes per quarter note */
    }       time_signature;
    struct {
      guint16 n_flats;          /* there's not n_sharps and n_flats at the same time */
      guint16 n_sharps;
      guint   major_key : 1;    /* dur */
      guint   minor_key : 1;    /* moll */
    }       key_signature;
    /* implementation specific */
    guint   zprefix;
  } data;
} BseMidiEvent;


/* --- API --- */
GType         bse_midi_event_get_type (void); /* boxed */
BseMidiEvent* bse_midi_alloc_event    (void);
BseMidiEvent* bse_midi_copy_event     (const BseMidiEvent *src);
void          bse_midi_free_event     (BseMidiEvent       *event);
BseMidiEvent* bse_midi_event_note_on  (uint                midi_channel,
                                       Bse::uint64         delta_time,
                                       float               frequency,
                                       float               velocity);
BseMidiEvent* bse_midi_event_note_off (uint                midi_channel,
                                       Bse::uint64         delta_time,
                                       gfloat              frequency);
BseMidiEvent* bse_midi_event_signal   (uint                midi_channel,
                                       Bse::uint64         delta_time,
                                       Bse::MidiSignal   signal_type,
                                       float               value);
double        bse_midi_signal_default (Bse::MidiSignal signal);
const char*   bse_midi_signal_name    (Bse::MidiSignal signal);

#endif /* __BSE_MIDI_EVENT_H__ */
