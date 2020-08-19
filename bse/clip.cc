// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "clip.hh"
#include "bseserver.hh"
#include "bsetrack.hh"
#include "internal.hh"

namespace Bse {

// == ClipImpl ==
ClipImplP
ClipImpl::create_clip (TrackImpl &track)
{
  ClipImplP clipp;
  clipp = FriendAllocator<ClipImpl>::make_shared();
  clipp->track_ = track.as<TrackImplP>();
  return clipp;
}

ClipImpl::ClipImpl()
{
  __attach__ ("notify", [this] (const Aida::Event &event) {
    TrackImplP track = track_.lock();
    return_unless (track);
    track->update_clip();
  });
}

ClipImpl::~ClipImpl ()
{}

ssize_t
ClipImpl::clip_index () const
{
  TrackImplP track = track_.lock();
  if (track)
    {
      ClipSeq cs = track->list_clips();
      for (size_t i = 0; i < cs.size(); i++)
        if (cs[i].get() == this)
          return i;
    }
  return -1;
}

#define PPQ 384

void
ClipImpl::xml_serialize (SerializationNode &xs)
{
  if (xs.in_save())
    {
      size_t n = clip_index();
      xs["index"] & n;
      n = PPQ;
      xs["ppq"] & n;
    }
  ObjectImpl::xml_serialize (xs); // always chain to parent's method
  if (xs.in_save())
    notes_.xml_serialize (xs, "notes");
  else if (xs.in_load())
    {
      int ppq = PPQ;
      xs["ppq"] & ppq;
      std::vector<PartNote> notes;
      xs["notes"] & notes;
      for (const auto &cnote : notes)
        {
          PartNote note = cnote;
          note.id = next_noteid();
          note.tick = note.tick * PPQ / ppq;
          note.duration = note.duration * PPQ / ppq;
          notes_.insert (note);
        }
    }
  printerr ("LOADED: clip index=%d notes=%d\n", clip_index(), notes_.size());
}

uint
ClipImpl::next_noteid()
{
  static std::atomic<uint> next_noteid { MIDI_NOTE_ID_FIRST };
  return next_noteid++;
}

void
ClipImpl::xml_reflink (SerializationNode &xs)
{
  ObjectImpl::xml_reflink (xs); // always chain to parent's method
}

int
ClipImpl::end_tick ()
{
  return 0;
}

int
ClipImpl::start_tick ()
{
  return starttick_;
}

int
ClipImpl::stop_tick ()
{
  return stoptick_;
}

void
ClipImpl::assign_range (int starttick, int stoptick)
{
  assert_return (starttick >= 0);
  assert_return (stoptick >= starttick);
  const auto last_starttick_ = starttick_;
  const auto last_stoptick_ = stoptick_;
  starttick_ = starttick;
  stoptick_ = stoptick;
  if (last_stoptick_ != stoptick_)
    emit_event ("notify:stop_tick");
  if (last_starttick_ != starttick_)
    emit_event ("notify:start_tick");
}

/// Retrieve const vector with all notes ordered by tick.
ClipImpl::OrderedEventList::ConstP
ClipImpl::tick_events ()
{
  return notes_.ordered_events<OrderedEventList> ();
}

/// List all notes ordered by tick.
PartNoteSeq
ClipImpl::list_all_notes ()
{
  PartNoteSeq ns;
  auto events = tick_events();
  ns.assign (events->begin(), events->end());
  return ns;
}

PartControlSeq
ClipImpl::list_controls (MidiSignal control_type)
{
  return {};
}

/// Change note `id`, or delete (`duration=0`) or create (`id=-1`) it.
int
ClipImpl::change_note (int id, int tick, int duration, int key, int fine_tune, double velocity)
{
  if (id < 0 && duration > 0)
    id = next_noteid(); // automatic id allocation for new notes
  assert_return (id >= MIDI_NOTE_ID_FIRST && id <= MIDI_NOTE_ID_LAST, 0);
  assert_return (duration >= 0, 0);
  if (tick < 0)
    return -1;
  PartNote ev;
  ev.tick = tick;
  ev.key = key;
  ev.id = 0;
  if (find_key_at_tick (ev) && ev.id != id)
    notes_.remove (ev);
  ev.id = id;
  ev.channel = 0;
  ev.duration = duration;
  ev.fine_tune = fine_tune;
  ev.velocity = velocity;
  ev.selected = false;
  int ret = ev.id;
  if (duration > 0)
    notes_.insert (ev);
  else
    ret = notes_.remove (ev) ? 0 : -1;
  emit_event ("notify:notes"); // FIXME: move to eventlist
  return ret;
}

bool
ClipImpl::find_key_at_tick (PartNote &ev)
{
  for (const auto &e : notes_)
    if (e.key == ev.key && e.tick == ev.tick)
      {
        ev = e;
        return true;
      }
  return false;
}

} // Bse
