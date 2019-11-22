// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_OBJECT_HH__
#define __BSE_OBJECT_HH__

#include <bse/bseutils.hh>
#include <bse/serializable.hh>

namespace Bse {

class ObjectImpl : public Aida::EnableSharedFromThis<ObjectImpl>, public virtual ObjectIface, public virtual SerializableInterface {
  Aida::EventDispatcher  event_dispatcher_;
protected:
  using KV = Aida::KeyValue;
  virtual          ~ObjectImpl    ();
  virtual void      xml_serialize (SerializationNode &xs) override;
  virtual void      xml_reflink   (SerializationNode &xs) override;
public:
  explicit          ObjectImpl    ();
  virtual bool      set_prop      (const std::string &name, const Any &value) override;
  virtual Any       get_prop      (const std::string &name) override;
  virtual StringSeq find_prop     (const std::string &name) override;
  virtual StringSeq list_props    () override;
  virtual void      notify        (const String &detail) override;
  void              emit_event    (const std::string &type, const KV &a1 = KV(), const KV &a2 = KV(), const KV &a3 = KV(),
                                   const KV &a4 = KV(), const KV &a5 = KV(), const KV &a6 = KV(), const KV &a7 = KV());
  // boilerplate
  virtual Aida::IfaceEventConnection __attach__            (const String &eventselector, EventHandlerF handler) override
  { return event_dispatcher_.attach (eventselector, handler); }
  using EnableSharedFromThis<ObjectImpl>::shared_from_this;
};
typedef std::shared_ptr<ObjectImpl> ObjectImplP;

} // Bse

#endif // __BSE_OBJECT_HH__
