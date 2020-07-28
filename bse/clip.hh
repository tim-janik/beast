// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_CLIP_HH__
#define __BSE_CLIP_HH__

#include <bse/object.hh>
#include <bse/midievent.hh>

namespace Bse {

// == ClipImpl ==
class ClipImpl : public ObjectImpl, public virtual ClipIface {
  int starttick_ = 0;
  int stoptick_ = 0;
protected:
  friend class FriendAllocator<ClipImpl>;
  virtual     ~ClipImpl      ();
  virtual void xml_serialize (SerializationNode &xs) override;
  virtual void xml_reflink   (SerializationNode &xs) override;
  explicit     ClipImpl      ();
public:
  using ClipImplP = std::shared_ptr<ClipImpl>;
  virtual int            end_tick       () override;
  virtual int            start_tick     () override;
  virtual int            stop_tick      () override;
  virtual void           assign_range   (int starttick, int endtick) override;
  virtual PartNoteSeq    list_all_notes () override;
  virtual PartControlSeq list_controls  (MidiSignal control_type) override;
  virtual int            change_note    (int id, int tick, int duration, int note, int fine_tune, double velocity) override;
  static ClipImplP       create_clip    ();
};
using ClipImplP = ClipImpl::ClipImplP;
using ClipImplW = std::weak_ptr<ClipImpl>;

} // Bse

#endif // __BSE_CLIP_HH__
