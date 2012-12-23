// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "birnetthread.hh"
#include <list>

#define birnet_threads_initialized()    ISLIKELY ((void*) ThreadTable.mutex_lock != (void*) ThreadTable.mutex_unlock)

namespace Birnet {

/* --- Thread::ThreadWrapperInternal --- */
struct Thread::ThreadWrapperInternal : public Thread {
  ThreadWrapperInternal (BirnetThread *bthread) :
    Thread (bthread)
  {}
  virtual void
  run ()
  {}
  static Thread*
  thread_from_c (BirnetThread *bthread)
  {
    Thread::ThreadWrapperInternal *ithread = new ThreadWrapperInternal (bthread);
    if (!ithread->bthread)
      {
        /* someone else was faster */
        ithread->ref_sink();
        ithread->unref();
      }
    void *threadxx = ThreadTable.thread_getxx (bthread);
    BIRNET_ASSERT (threadxx != NULL);
    return reinterpret_cast<Thread*> (threadxx);
  }
  static void
  thread_reset_c (Thread *thread)
  {
    BirnetThread *bthread = thread->bthread;
    BIRNET_ASSERT (thread->bthread != NULL);
    thread->data_list.clear_like_destructor();
    thread->bthread = NULL;
    ThreadTable.thread_setxx (bthread, NULL);
  }
  static void
  trampoline (void *thread_data)
  {
    Thread &self = *reinterpret_cast<Thread*> (thread_data);
    ref_sink (self);
    self.run();
    unref (self);
  }
};

/* --- ThreadWrapperInternal (public version of Thread::ThreadWrapperInternal --- */
struct ThreadDescendant : public Thread {
  typedef ThreadWrapperInternal PublicThreadWrapperInternal;
  ThreadDescendant (const String &name) : Thread (name) {}
};
typedef ThreadDescendant::PublicThreadWrapperInternal ThreadWrapperInternal;

/* --- Thread methods --- */
void
Thread::threadxx_wrap (BirnetThread *cthread)
{
  ThreadWrapperInternal::thread_from_c (cthread);
}

void
Thread::threadxx_delete (void *cxxthread)
{
  Thread *thread = reinterpret_cast<Thread*> (cxxthread);
  ThreadWrapperInternal::thread_reset_c (thread);
}

Thread::Thread (BirnetThread* thread) :
  bthread (NULL)
{
  ThreadTable.thread_ref (thread);
  if (ThreadTable.thread_setxx (thread, this))
    {
      bthread = thread;
      ThreadTable.thread_ref_sink (thread);
      BIRNET_ASSERT (ThreadTable.thread_getxx (thread) == this);
    }
  else
    ; /* invalid object state; this should be reaped by thread_from_c() */
  ThreadTable.thread_unref (thread);
}

static BirnetThread*
bthread_create_for_thread (const String &name,
                           void         *threadxx)
{
  BirnetThread *bthread = ThreadTable.thread_new (name.c_str());
  bool success = ThreadTable.thread_setxx (bthread, threadxx);
  BIRNET_ASSERT (success);
  ThreadTable.thread_ref_sink (bthread);
  return bthread;
}

Thread::Thread (const String &_name) :
  bthread (bthread_create_for_thread (_name, this))
{}

Thread::~Thread ()
{
  if (bthread)  /* can be NULL in thread_from_c() */
    {
      ThreadTable.thread_setxx (bthread, NULL);
      ThreadTable.thread_unref (bthread);
    }
}

void
Thread::start ()
{
  bool success = false;
  while (!success)
    {
      success = ThreadTable.thread_start (bthread, Thread::ThreadWrapperInternal::trampoline, this);
      if (!success)
        ThreadTable.thread_yield();
    }
}

void
Thread::emit_wakeups (uint64 stamp)
{
  ThreadTable.thread_emit_wakeups (stamp);
}

int
Thread::pid () const
{
  return ThreadTable.thread_pid (bthread);
}

String
Thread::name () const
{
  return ThreadTable.thread_name (bthread);
}

void
Thread::queue_abort ()
{
  ThreadTable.thread_queue_abort (bthread);
}

void
Thread::abort ()
{
  ThreadTable.thread_abort (bthread);
}

bool
Thread::aborted ()
{
  return ThreadTable.thread_get_aborted (bthread);
}

void
Thread::wakeup ()
{
  ThreadTable.thread_wakeup (bthread);
}

bool
Thread::running ()
{
  return ThreadTable.thread_get_running (bthread);
}

void
Thread::wait_for_exit ()
{
  ThreadTable.thread_wait_for_exit (bthread);
}

Thread&
Thread::self ()
{
  Thread *thread = (Thread*) ThreadTable.thread_selfxx();
  return *thread;
}

String
Thread::Self::name ()
{
  return ThreadTable.thread_name (ThreadTable.thread_self());
}

void
Thread::Self::name (const String &name)
{
  ThreadTable.thread_set_name (name.c_str());
}

/**
 * @param max_useconds  maximum amount of micro seconds to sleep (-1 for infinite time)
 * @param returns       TRUE while the thread should continue execution
 *
 * This is a wrapper for birnet_thread_sleep().
 */
bool
Thread::Self::sleep (long max_useconds)
{
  return ThreadTable.thread_sleep (max_useconds);
}

bool
Thread::Self::aborted ()
{
  return ThreadTable.thread_aborted();
}

int
Thread::Self::pid ()
{
  return ThreadTable.thread_pid (ThreadTable.thread_self());
}

void
Thread::Self::awake_after (uint64 stamp)
{
  ThreadTable.thread_awake_after (stamp);
}

void
Thread::Self::set_wakeup (BirnetThreadWakeup   wakeup_func,
                          void                *wakeup_data,
                          void               (*destroy_data) (void*))
{
  ThreadTable.thread_set_wakeup (wakeup_func, wakeup_data, destroy_data);
}

OwnedMutex&
Thread::Self::owned_mutex ()
{
  return self().m_omutex;
}

void
Thread::Self::yield ()
{
  ThreadTable.thread_yield ();
}

void
Thread::Self::exit (void *retval)
{
  ThreadTable.thread_exit (retval);
}

static const BirnetMutex zero_mutex = { 0, };

Mutex::Mutex () :
  mutex (zero_mutex)
{
  if (birnet_threads_initialized())
    ThreadTable.mutex_init (&mutex);
  else
    ThreadTable.mutex_chain4init (&mutex);
}

Mutex::~Mutex ()
{
  if (birnet_threads_initialized())
    ThreadTable.mutex_destroy (&mutex);
  else
    ThreadTable.mutex_unchain (&mutex);
}

static const BirnetRecMutex zero_rec_mutex = { { 0, }, };

RecMutex::RecMutex () :
  rmutex (zero_rec_mutex)
{
  if (birnet_threads_initialized())
    ThreadTable.rec_mutex_init (&rmutex);
  else
    ThreadTable.rec_mutex_chain4init (&rmutex);
}

RecMutex::~RecMutex ()
{
  if (birnet_threads_initialized())
    ThreadTable.rec_mutex_destroy (&rmutex);
  else
    ThreadTable.rec_mutex_unchain (&rmutex);
}

static const BirnetCond zero_cond = { 0, };

Cond::Cond () :
  cond (zero_cond)
{
  if (birnet_threads_initialized())
    ThreadTable.cond_init (&cond);
  else
    ThreadTable.cond_chain4init (&cond);
}

Cond::~Cond ()
{
  if (birnet_threads_initialized())
    ThreadTable.cond_destroy (&cond);
  else
    ThreadTable.cond_unchain (&cond);
}

OwnedMutex::OwnedMutex () :
  m_rec_mutex (zero_rec_mutex),
  m_owner (NULL)
{
  if (birnet_threads_initialized())
    ThreadTable.rec_mutex_init (&m_rec_mutex);
  else
    ThreadTable.rec_mutex_chain4init (&m_rec_mutex);
}

OwnedMutex::~OwnedMutex()
{
  BIRNET_ASSERT (m_owner == NULL);
  if (birnet_threads_initialized())
    ThreadTable.rec_mutex_destroy (&m_rec_mutex);
  else
    ThreadTable.rec_mutex_unchain (&m_rec_mutex);
}

} // Birnet
