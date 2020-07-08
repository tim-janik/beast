// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bse/midievent.hh"
#include "internal.hh"

#define EDEBUG(...)     Bse::debug ("event", __VA_ARGS__)

namespace Bse {
namespace AudioSignal {

// == ==
Event::Event (const Event &other) :
  Event()
{
  *this = other;
}

Event::Event (EventType etype)
{
  memset (this, 0, sizeof (*this));
  type = etype;
  // one main design consideration is minimized size
  static_assert (sizeof (Event) <= 2 * sizeof (void*));
}

Event&
Event::operator= (const Event &other)
{
  if (this != &other)
    memcpy (this, &other, sizeof (*this));
  return *this;
}

Event::~Event ()
{}

/// Determine extended message type an Event.
Message
Event::message () const
{
  if (type == Event::CONTROL_CHANGE)
    {
      if (param >= uint (Message::ALL_SOUND_OFF) &&
          param <= uint (Message::POLY_MODE_ON))
        return Message (param);
    }
  return Message (type);
}

std::string
Event::to_string () const
{
  const char *et = nullptr;
  switch (type)
    {
    case NOTE_OFF:        if (!et) et = "NOTE_OFF";
    case NOTE_ON:         if (!et) et = "NOTE_ON";
    case AFTERTOUCH:      if (!et) et = "AFTERTOUCH";
      return string_format ("%+4d ch=%-2u %s pitch=%d vel=%f tune=%f id=%x",
                            frame, channel, et, pitch, velocity, tuning, noteid);
    case CONTROL_CHANGE:        if (!et) et = "CONTROL_CHANGE";
      return string_format ("%+4d ch=%-2u %s control=%d value=%f (%02x)",
                            frame, channel, et, param, value, cval);
    case PROGRAM_CHANGE:        if (!et) et = "PROGRAM_CHANGE";
      return string_format ("%+4d ch=%-2u %s program=%d",
                            frame, channel, et, param);
    case CHANNEL_PRESSURE:      if (!et) et = "CHANNEL_PRESSURE";
    case PITCH_BEND:            if (!et) et = "PITCH_BEND";
      return string_format ("%+4d ch=%-2u %s value=%+f",
                            frame, channel, et, value);
    case SYSEX:                 if (!et) et = "SYSEX";
      return string_format ("%+4d %s (unhandled)", frame, et);
    default:
      return string_format ("%+4d Event-%u (unhandled)", frame, type);
    }
}

Event
make_note_on (uint16 chnl, uint8 pch_, float velo, float tune, uint nid)
{
  Event ev (velo > 0 ? Event::NOTE_ON : Event::NOTE_OFF);
  ev.channel = chnl;
  ev.pitch = pch_;
  ev.velocity = velo;
  ev.tuning = tune;
  ev.noteid = nid;
  return ev;
}

Event
make_note_off (uint16 chnl, uint8 pch_, float velo, float tune, uint nid)
{
  Event ev (Event::NOTE_OFF);
  ev.channel = chnl;
  ev.pitch = pch_;
  ev.velocity = velo;
  ev.tuning = tune;
  ev.noteid = nid;
  return ev;
}

Event
make_aftertouch (uint16 chnl, uint8 pch_, float velo, float tune, uint nid)
{
  Event ev (Event::AFTERTOUCH);
  ev.channel = chnl;
  ev.pitch = pch_;
  ev.velocity = velo;
  ev.tuning = tune;
  ev.noteid = nid;
  return ev;
}

Event
make_pressure (uint16 chnl, float velo)
{
  Event ev (Event::CHANNEL_PRESSURE);
  ev.channel = chnl;
  ev.velocity = velo;
  return ev;
}

Event
make_control (uint16 chnl, uint prm, float val)
{
  Event ev (Event::CONTROL_CHANGE);
  ev.channel = chnl;
  ev.param = prm;
  ev.value = val;
  ev.cval = ev.value * 127;
  return ev;
}

Event
make_control8 (uint16 chnl, uint prm, uint8 cval)
{
  Event ev (Event::CONTROL_CHANGE);
  ev.channel = chnl;
  ev.param = prm;
  ev.cval = cval;
  ev.value = ev.cval * (1.0 / 127.0);
  return ev;
}

Event
make_program (uint16 chnl, uint prgrm)
{
  Event ev (Event::PROGRAM_CHANGE);
  ev.channel = chnl;
  ev.param = prgrm;
  return ev;
}

Event
make_pitch_bend (uint16 chnl, float val)
{
  Event ev (Event::PITCH_BEND);
  ev.channel = chnl;
  ev.value = val;
  return ev;
}

// == EventStream ==
EventStream::EventStream ()
{}

void
EventStream::append (int8_t frame, const Event &event)
{
  if (events_.empty())
    events_.reserve (16);
  else
    assert_return (frame >= events_.back().frame);
  events_.push_back (event);
  events_.back().frame = frame;
}

// == EventRange ==
EventRange::EventRange (const EventStream estream) :
  estream_ (estream)
{}

} // AudioSignal
} // Bse