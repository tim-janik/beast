// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MIDI_EVENT_HH__
#define __BSE_MIDI_EVENT_HH__

#include <bse/memory.hh>

namespace Bse {

namespace AudioSignal {

/// Type of MIDI Events.
enum class EventType : uint8_t {};

/// MIDI Event data structure.
struct Event {
  constexpr static EventType NOTE_OFF         = EventType (0x80);
  constexpr static EventType NOTE_ON          = EventType (0x90);
  constexpr static EventType AFTERTOUCH       = EventType (0xA0); ///< Key Pressure, polyphonic aftertouch
  constexpr static EventType CONTROL_CHANGE   = EventType (0xB0); ///< Control Change
  constexpr static EventType PROGRAM_CHANGE   = EventType (0xC0);
  constexpr static EventType CHANNEL_PRESSURE = EventType (0xD0); ///< Channel Aftertouch
  constexpr static EventType PITCH_BEND       = EventType (0xE0);
  constexpr static EventType SYSEX            = EventType (0xF0);
  EventType type;       ///< Event type, one of the EventType members
  int8      frame;      ///< Offset into current block, delayed if negative
  uint16    channel;    ///< 1…16 for standard events
  union {
    uint    length;     ///< Data event length of byte array.
    uint    param;      ///< PROGRAM_CHANGE program, CONTROL_CHANGE controller, 0…0x7f
    struct {
      uint8 pitch;      ///< NOTE, KEY_PRESSURE MIDI note, 0…0x7f, 60 = middle C at 261.63 Hz.
      uint  noteid :24; ///< NOTE, identifier for note expression handling or 0xffffff.
    };
  };
  union {
    char   *data;       ///< Data event byte array.
    struct {
      float  value;     ///< CONTROL_CHANGE 0…+1, CHANNEL_PRESSURE, 0…+1, PITCH_BEND -1…+1
      uint   cval;      ///< CONTROL_CHANGE control value, 0…0x7f
    };
    struct {
      float velocity;   ///< NOTE, KEY_PRESSURE, CHANNEL_PRESSURE, 0…+1
      float tuning;     ///< NOTE, fine tuning in ±cents
    };
  };
  explicit Event     (EventType etype = EventType (0));
  /*copy*/ Event     (const Event &other);
  Event&   operator= (const Event &other);
  /*des*/ ~Event     ();
  std::string  to_string  () const;
};

Event make_note_on    (uint16 chnl, uint8 ptch, float velo, float tune = 0, uint nid = 0xffffff);
Event make_note_off   (uint16 chnl, uint8 ptch, float velo, float tune = 0, uint nid = 0xffffff);
Event make_aftertouch (uint16 chnl, uint8 ptch, float velo, float tune = 0, uint nid = 0xffffff);
Event make_pressure   (uint16 chnl, float velo);
Event make_control    (uint16 chnl, uint prm, float val);
Event make_control8   (uint16 chnl, uint prm, uint8 cval);
Event make_program    (uint16 chnl, uint prgrm);
Event make_pitch_bend (uint16 chnl, float val);

/// A stream of writable Event structures.
class EventStream {
  std::vector<Event> events_; // TODO: use O(1) allocator
  friend class EventRange;
public:
  explicit     EventStream ();
  void         append      (int8_t frame, const Event &event);
  const Event* begin       () const     { return &*events_.begin(); }
  const Event* end         () const     { return &*events_.end(); }
  size_t       size        () const     { return events_.size(); }
  void         clear       () noexcept  { events_.clear(); }
};

/// A readonly view and iterator into an EventStream.
class EventRange {
  const EventStream estream_;
public:
  const Event* begin          () const  { return &*estream_.begin(); }
  const Event* end            () const  { return &*estream_.end(); }
  size_t       events_pending () const  { return estream_.size(); }
  explicit     EventRange     (const EventStream estream);
};

} // AudioSignal
} // Bse

#endif // __BSE_MIDI_EVENT_HH__
