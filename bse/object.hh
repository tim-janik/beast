// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_OBJECT_HH__
#define __BSE_OBJECT_HH__

#include <bse/bseutils.hh>

namespace Bse {

class ObjectImpl : public Aida::EnableSharedFromThis<ObjectImpl>, public virtual ObjectIface {
  Aida::EventDispatcher  event_dispatcher_;
protected:
  virtual              ~ObjectImpl       ();
public:
  explicit              ObjectImpl      ();
  // boilerplate
  virtual Aida::ExecutionContext& __execution_context_mt__ () const override    { return execution_context(); }
  virtual Aida::IfaceEventConnection __attach__            (const String &eventselector, EventHandlerF handler) override
  { return event_dispatcher_.attach (eventselector, handler); }
  using EnableSharedFromThis<ObjectImpl>::shared_from_this;
};
typedef std::shared_ptr<ObjectImpl> ObjectImplP;

} // Bse

#endif // __BSE_OBJECT_HH__
