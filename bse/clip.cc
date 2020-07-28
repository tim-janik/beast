// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "clip.hh"
#include "bseserver.hh"
#include "internal.hh"

namespace Bse {

// == ClipImpl ==
ClipImplP
ClipImpl::create_clip ()
{
  ClipImplP clipp;
  clipp = FriendAllocator<ClipImpl>::make_shared();
  return clipp;
}

ClipImpl::ClipImpl()
{}

ClipImpl::~ClipImpl ()
{}

void
ClipImpl::xml_serialize (SerializationNode &xs)
{
  ObjectImpl::xml_serialize (xs); // always chain to parent's method
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

PartNoteSeq
ClipImpl::list_all_notes ()
{
  return {};
}

PartControlSeq
ClipImpl::list_controls (MidiSignal control_type)
{
  return {};
}

int
ClipImpl::change_note (int id, int tick, int duration, int note, int fine_tune, double velocity)
{
  emit_event ("notify:notes");
  return 0;
}

} // Bse
