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
  return 0;
}

int
ClipImpl::stop_tick ()
{
  return 0;
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

} // Bse
