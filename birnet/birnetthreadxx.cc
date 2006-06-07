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

namespace Birnet {

static BirnetThread*
create_thread (const gchar *name,
               void        *threadxx)
{
  BirnetThread *bthread = birnet_thread_new (name);
  bool success = _birnet_thread_set_cxx (bthread, threadxx);
  BIRNET_ASSERT (success);
  birnet_thread_ref_sink (bthread);
  return bthread;
}

Thread::Thread (const String &_name) :
  bthread (create_thread (_name.c_str(), this))
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

Thread*
Thread::thread_from_c (BirnetThread *bthread)
{
  struct CThread : public virtual Thread {
    explicit CThread (BirnetThread *bthread) :
      Thread (bthread)
    {}
    virtual void run() {}
  };
  CThread *cthread = new CThread (bthread);
  if (!cthread->bthread)
    {
      /* someone else was faster */
      cthread->ref_sink();
      cthread->unref();
    }
  void *threadxx = _birnet_thread_get_cxx (bthread);
  BIRNET_ASSERT (threadxx != NULL);
  return reinterpret_cast<Thread*> (threadxx);
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
Thread::trampoline (void *thread_data)
{
  Thread &self = *reinterpret_cast<Thread*> (thread_data);
  self.run();
}

void
Thread::start ()
{
  bool success = false;
  while (!success)
    {
      success = birnet_thread_start (bthread, trampoline, this);
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

bool
OwnedMutex::trylock ()
{
  if (birnet_thread_table.mutex_trylock (&m_mutex) == 0)
    {
      Atomic::ptr_set (&m_owner, &Thread::self());
      return true; /* TRUE indicates success */
    }
  else
    return false;
}

OwnedMutex::~OwnedMutex()
{
  BIRNET_ASSERT (m_owner == NULL);
  birnet_thread_table.mutex_destroy (&m_mutex);
}

} // Birnet

extern "C" void
_birnet_thread_cxx_wrap (BirnetThread *cthread)
{
  using namespace Birnet;
  struct ThreadWrapper : Thread {
    static Thread*
    wrapped_thread_from_c (BirnetThread *bthread)
    {
      return thread_from_c (bthread);
    }
  };
  ThreadWrapper::wrapped_thread_from_c (cthread);
}

extern "C" void
_birnet_thread_cxx_delete (void *cxxthread)
{
  using namespace Birnet;
  Thread *thread = reinterpret_cast<Thread*> (cxxthread);
  delete thread;
}
