/* Birnet
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "birnetthreadxx.hh"
#include <list>

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
    void *threadxx = _birnet_thread_get_cxx (bthread);
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
    _birnet_thread_set_cxx (bthread, NULL);
  }
  static void
  trampoline (void *thread_data)
  {
    Thread &self = *reinterpret_cast<Thread*> (thread_data);
    self.run();
  }
};

/* --- ThreadWrapperInternal (public version of Thread::ThreadWrapperInternal --- */
struct ThreadDescendant : public Thread {
  typedef ThreadWrapperInternal PublicThreadWrapperInternal;
};
typedef ThreadDescendant::PublicThreadWrapperInternal ThreadWrapperInternal;

/* --- Thread methods --- */
static BirnetThread*
bthread_create_for_thread (const String &name,
                           void         *threadxx)
{
  BirnetThread *bthread = birnet_thread_new (name.c_str());
  bool success = _birnet_thread_set_cxx (bthread, threadxx);
  BIRNET_ASSERT (success);
  birnet_thread_ref_sink (bthread);
  return bthread;
}

Thread::Thread (const String &_name) :
  bthread (bthread_create_for_thread (_name, this))
{}

Thread::Thread (BirnetThread* thread) :
  bthread (NULL)
{
  birnet_thread_ref (thread);
  if (_birnet_thread_set_cxx (thread, this))
    {
      bthread = thread;
      birnet_thread_ref_sink (thread);
      BIRNET_ASSERT (_birnet_thread_get_cxx (thread) == this);
    }
  else
    ; /* invalid object state; this should be reaped by thread_from_c() */
  birnet_thread_unref (thread);
}

Thread::~Thread ()
{
  if (bthread)  /* can be NULL in thread_from_c() */
    {
      _birnet_thread_set_cxx (bthread, NULL);
      birnet_thread_unref (bthread);
    }
}

void
Thread::start ()
{
  bool success = false;
  while (!success)
    {
      success = birnet_thread_start (bthread, Thread::ThreadWrapperInternal::trampoline, this);
      if (!success)
        birnet_thread_yield();
    }
}

void
Thread::emit_wakeups (uint64 stamp)
{
  birnet_thread_emit_wakeups (stamp);
}

int
Thread::pid () const
{
  return birnet_thread_get_pid (bthread);
}

String
Thread::name () const
{
  return birnet_thread_get_name (bthread);
}

void
Thread::queue_abort ()
{
  birnet_thread_queue_abort (bthread);
}

void
Thread::abort ()
{
  birnet_thread_abort (bthread);
}

bool
Thread::aborted ()
{
  return birnet_thread_get_aborted (bthread);
}

void
Thread::wakeup ()
{
  birnet_thread_wakeup (bthread);
}

bool
Thread::running ()
{
  return birnet_thread_get_running (bthread);
}

void
Thread::wait_for_exit ()
{
  birnet_thread_wait_for_exit (bthread);
}

Thread&
Thread::self ()
{
  Thread *thread = (Thread*) _birnet_thread_self_cxx();
  return *thread;
}

String
Thread::Self::name ()
{
  return birnet_thread_get_name (birnet_thread_self());
}

void
Thread::Self::name (const String &name)
{
  birnet_thread_set_name (name.c_str());
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
  return birnet_thread_sleep (max_useconds);
}

bool
Thread::Self::aborted ()
{
  return birnet_thread_aborted();
}

int
Thread::Self::pid ()
{
  return birnet_thread_self_pid();
}

void
Thread::Self::awake_after (uint64 stamp)
{
  birnet_thread_awake_after (stamp);
}

void
Thread::Self::set_wakeup (BirnetThreadWakeup      wakeup_func,
                          void                   *wakeup_data,
                          GDestroyNotify          destroy)
{
  birnet_thread_set_wakeup (wakeup_func, wakeup_data, destroy);
}

OwnedMutex&
Thread::Self::owned_mutex ()
{
  return self().m_omutex;
}

void
Thread::Self::exit (void *retval)
{
  birnet_thread_exit (retval);
}

static std::list<BirnetMutex*> cxx_init_mutex_list;

Mutex::Mutex ()
{
  if (birnet_threads_initialized())
    birnet_thread_table.mutex_init (&mutex);
  else
    cxx_init_mutex_list.push_back (&mutex);
}

Mutex::~Mutex ()
{
  if (birnet_threads_initialized())
    birnet_thread_table.mutex_destroy (&mutex);
  else
    cxx_init_mutex_list.remove (&mutex);
}

static std::list<BirnetRecMutex*> cxx_init_rec_mutex_list;

RecMutex::RecMutex ()
{
  if (birnet_threads_initialized())
    birnet_thread_table.rec_mutex_init (&rmutex);
  else
    cxx_init_rec_mutex_list.push_back (&rmutex);
}

RecMutex::~RecMutex ()
{
  if (birnet_threads_initialized())
    birnet_thread_table.rec_mutex_destroy (&rmutex);
  else
    cxx_init_rec_mutex_list.remove (&rmutex);
}

static std::list<BirnetCond*> cxx_init_cond_list;

Cond::Cond ()
{
  if (birnet_threads_initialized())
    birnet_thread_table.cond_init (&cond);
  else
    cxx_init_cond_list.push_back (&cond);
}

Cond::~Cond ()
{
  if (birnet_threads_initialized())
    birnet_thread_table.cond_destroy (&cond);
  else
    cxx_init_cond_list.remove (&cond);
}

OwnedMutex::OwnedMutex () :
  m_owner (NULL)
{
  if (birnet_threads_initialized())
    birnet_thread_table.rec_mutex_init (&m_rec_mutex);
  else
    cxx_init_rec_mutex_list.push_back (&m_rec_mutex);
}

OwnedMutex::~OwnedMutex()
{
  BIRNET_ASSERT (m_owner == NULL);
  if (birnet_threads_initialized())
    birnet_thread_table.rec_mutex_destroy (&m_rec_mutex);
  else
    cxx_init_rec_mutex_list.remove (&m_rec_mutex);
}

} // Birnet

extern "C" void
_birnet_init_threads_cxx (void)
{
  using namespace Birnet;
  while (!cxx_init_mutex_list.empty())
    {
      BirnetMutex *mutex = cxx_init_mutex_list.front();
      cxx_init_mutex_list.pop_front();
      birnet_thread_table.mutex_init (mutex);
    }
  while (!cxx_init_rec_mutex_list.empty())
    {
      BirnetRecMutex *rmutex = cxx_init_rec_mutex_list.front();
      cxx_init_rec_mutex_list.pop_front();
      birnet_thread_table.rec_mutex_init (rmutex);
    }
  while (!cxx_init_cond_list.empty())
    {
      BirnetCond *cond = cxx_init_cond_list.front();
      cxx_init_cond_list.pop_front();
      birnet_thread_table.cond_init (cond);
    }
}

extern "C" void
_birnet_thread_cxx_wrap (BirnetThread *cthread)
{
  using namespace Birnet;
  ThreadWrapperInternal::thread_from_c (cthread);
}

extern "C" void
_birnet_thread_cxx_delete (void *cxxthread)
{
  using namespace Birnet;
  Thread *thread = reinterpret_cast<Thread*> (cxxthread);
  ThreadWrapperInternal::thread_reset_c (thread);
}
