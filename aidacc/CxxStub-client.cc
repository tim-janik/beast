#ifndef __AIDA_CXXSTUB_CLIENT_CC__
#define __AIDA_CXXSTUB_CLIENT_CC__

#include <semaphore.h>

namespace { // Anon

namespace __AIDA_Local__ {
using namespace Aida;

static inline sem_t*
thread_semaphore()
{
  class DeletableSemaphore {
    DeletableSemaphore (const DeletableSemaphore&) = delete;            // disable copy ctor
    DeletableSemaphore& operator= (const DeletableSemaphore&) = delete; // disable copy assignment
  public:
    sem_t semaphore = sem_t();
    DeletableSemaphore()
    {
      const int ret = sem_init (&semaphore, 0 /*pshared*/, 0 /*value*/);
      AIDA_ASSERT_RETURN (ret == 0);
    }
    ~DeletableSemaphore()
    {
      sem_destroy (&semaphore);
    }
  };
  static thread_local DeletableSemaphore thread_local_semaphore;
  return &thread_local_semaphore.semaphore;
}

template<class T, class... I, class... A> void
remote_callv (Aida::RemoteHandle &h, void (T::*const mfp) (I...), A&&... args)
{
  sem_t *const semp = thread_semaphore();
  T *const self = dynamic_cast<T*> (h.__iface_ptr__().get());
  std::function<void()> wrapper = [semp, self, mfp, &args...] () {
    (self->*mfp) (args...);
    sem_post (semp);
  };
  self->__execution_context_mt__()->enqueue_mt (wrapper);
  sem_wait (semp);
}

template<class T, class R, class... I, class... A> R
remote_callr (Aida::RemoteHandle &h, R (T::*const mfp) (I...), A&&... args)
{
  sem_t *const semp = thread_semaphore();
  T *const self = dynamic_cast<T*> (h.__iface_ptr__().get());
  R r {};
  std::function<void()> wrapper = [semp, self, mfp, &args..., &r] () {
    r = (self->*mfp) (args...);
    sem_post (semp);
  };
  self->__execution_context_mt__()->enqueue_mt (wrapper);
  sem_wait (semp);
  return r;
}

template<class T, class R, class... I, class... A> R
remote_callc (const Aida::RemoteHandle &h, R (T::*const mfp) (I...) const, A&&... args)
{
  sem_t *const semp = thread_semaphore();
  T *const self = dynamic_cast<T*> (const_cast<Aida::RemoteHandle&> (h).__iface_ptr__().get());
  R r {};
  std::function<void()> wrapper = [semp, self, mfp, &args..., &r] () {
    r = (self->*mfp) (args...);
    sem_post (semp);
  };
  self->__execution_context_mt__()->enqueue_mt (wrapper);
  sem_wait (semp);
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
