#ifndef __AIDA_CXXSTUB_CLIENT_CC__
#define __AIDA_CXXSTUB_CLIENT_CC__

namespace { // Anon

namespace __AIDA_Local__ {
using namespace Aida;

template<class T, class... I, class... A> void
remote_callv (Aida::RemoteHandle &h, void (T::*const mfp) (I...), A&&... args)
{
  ScopedSemaphore sem;
  T *const self = dynamic_cast<T*> (h.__iface_ptr__().get());
  std::function<void()> wrapper = [&sem, self, mfp, &args...] () {
    (self->*mfp) (args...);
    sem.post();
  };
  self->__execution_context_mt__()->enqueue_mt (wrapper);
  sem.wait();
}

template<class T, class R, class... I, class... A> R
remote_callr (Aida::RemoteHandle &h, R (T::*const mfp) (I...), A&&... args)
{
  ScopedSemaphore sem;
  T *const self = dynamic_cast<T*> (h.__iface_ptr__().get());
  R r {};
  std::function<void()> wrapper = [&sem, self, mfp, &args..., &r] () {
    r = (self->*mfp) (args...);
    sem.post();
  };
  self->__execution_context_mt__()->enqueue_mt (wrapper);
  sem.wait();
  return r;
}

template<class T, class R, class... I, class... A> R
remote_callc (const Aida::RemoteHandle &h, R (T::*const mfp) (I...) const, A&&... args)
{
  ScopedSemaphore sem;
  T *const self = dynamic_cast<T*> (const_cast<Aida::RemoteHandle&> (h).__iface_ptr__().get());
  R r {};
  std::function<void()> wrapper = [&sem, self, mfp, &args..., &r] () {
    r = (self->*mfp) (args...);
    sem.post();
  };
  self->__execution_context_mt__()->enqueue_mt (wrapper);
  sem.wait();
  return r;
}

// helper
static inline ProtoMsg*
new_emit_result (const ProtoMsg *fb, uint64 h, uint64 l, uint32 n)
{
  return ProtoMsg::renew_into_result (const_cast<ProtoMsg*> (fb), Aida::MSGID_EMIT_RESULT, h, l, n);
}

} } // Anon::__AIDA_Local__

#endif // __AIDA_CXXSTUB_CLIENT_CC__
