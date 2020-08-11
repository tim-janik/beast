// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CLIP_HH__
#define __BSE_CLIP_HH__

#include <bse/object.hh>
#include <bse/midievent.hh>
#include <bse/eventlist.hh>

namespace Bse {

/// First (internal) MIDI note event ID (lower IDs are reserved for external notes).
constexpr const uint MIDI_NOTE_ID_FIRST = 0x10000001;
/// Last valid (internal) MIDI note event ID.
constexpr const uint MIDI_NOTE_ID_LAST = 0xfffffffe;

// == ClipImpl ==
class ClipImpl : public ObjectImpl, public virtual ClipIface {
  int starttick_ = 0;
  int stoptick_ = 0;
  struct CmpNoteTicks { int operator() (const PartNote &a, const PartNote &b) const; };
  struct CmpNoteIds   { int operator() (const PartNote &a, const PartNote &b) const; };
  using OrderedEventList = OrderedEventList<PartNote,CmpNoteTicks>;
  EventList<PartNote,CmpNoteIds> notes_;
protected:
  friend class FriendAllocator<ClipImpl>;
  virtual     ~ClipImpl      ();
  virtual void xml_serialize (SerializationNode &xs) override;
  virtual void xml_reflink   (SerializationNode &xs) override;
  explicit     ClipImpl      ();
  bool         find_key_at_tick (PartNote &ev);
public:
  using ClipImplP = std::shared_ptr<ClipImpl>;
  virtual int            end_tick       () override;
  virtual int            start_tick     () override;
  virtual int            stop_tick      () override;
  virtual void           assign_range   (int starttick, int endtick) override;
  virtual PartNoteSeq    list_all_notes () override;
  virtual PartControlSeq list_controls  (MidiSignal control_type) override;
  virtual int            change_note    (int id, int tick, int duration, int key, int fine_tune, double velocity) override;
  static ClipImplP       create_clip    ();
};
using ClipImplP = ClipImpl::ClipImplP;
using ClipImplW = std::weak_ptr<ClipImpl>;

inline int
ClipImpl::CmpNoteTicks::operator() (const PartNote &a, const PartNote &b) const
{
  int cmp = int (a.tick) - int (b.tick);
  if (BSE_ISLIKELY (cmp))
    return cmp;
  cmp = int (a.key) - int (b.key);
  if (BSE_ISLIKELY (cmp))
    return cmp;
  cmp = int (a.id) - int (b.id);
  return cmp;
}

inline int
ClipImpl::CmpNoteIds::operator() (const PartNote &a, const PartNote &b) const
{
  return int (a.id) - int (b.id);
}

} // Bse

#endif // __BSE_CLIP_HH__
