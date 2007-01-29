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
//#define TEST_VERBOSE
#include <birnet/birnettests.h>

namespace {
using namespace Birnet;

/* --- utilities --- */
BirnetThread*
birnet_thread_run (const gchar     *name,
                   BirnetThreadFunc func,
                   gpointer         user_data)
{
  g_return_val_if_fail (name && name[0], NULL);

  BirnetThread *thread = ThreadTable.thread_new (name);
  ThreadTable.thread_ref_sink (thread);
  if (ThreadTable.thread_start (thread, func, user_data))
    return thread;
  else
    {
      ThreadTable.thread_unref (thread);
      return NULL;
    }
}

#define birnet_mutex_init(mtx)        (ThreadTable.mutex_init (mtx))
#define birnet_mutex_lock(mtx)        (ThreadTable.mutex_lock (mtx))
#define birnet_mutex_trylock(mtx)     (0 == ThreadTable.mutex_trylock (mtx))
#define birnet_mutex_unlock(mtx)      (ThreadTable.mutex_unlock (mtx))
#define birnet_mutex_destroy(mtx)     (ThreadTable.mutex_destroy (mtx))
#define birnet_rec_mutex_init(mtx)    (ThreadTable.rec_mutex_init (mtx))
#define birnet_rec_mutex_lock(mtx)    (ThreadTable.rec_mutex_lock (mtx))
#define birnet_rec_mutex_trylock(mtx) (0 == ThreadTable.rec_mutex_trylock (mtx))
#define birnet_rec_mutex_unlock(mtx)  (ThreadTable.rec_mutex_unlock (mtx))
#define birnet_rec_mutex_destroy(mtx) (ThreadTable.rec_mutex_destroy (mtx))

#define BIRNET_MUTEX_DECLARE_INITIALIZED(mutexname)                             \
  BirnetMutex mutexname = { 0 };                                                \
  static void BIRNET_CONSTRUCTOR                                                \
  BIRNET_CPP_PASTE4 (__birnet_mutex__autoinit, __LINE__, __, mutexname) (void)  \
  { ThreadTable.mutex_chain4init (&mutexname); }

#define BIRNET_REC_MUTEX_DECLARE_INITIALIZED(recmtx)                            \
  BirnetRecMutex recmtx = { { 0 } };                                            \
  static void BIRNET_CONSTRUCTOR                                                \
  BIRNET_CPP_PASTE4 (__birnet_rec_mutex__autoinit, __LINE__, __, recmtx) (void) \
  { ThreadTable.rec_mutex_chain4init (&recmtx); }

#define BIRNET_COND_DECLARE_INITIALIZED(condname)                               \
  BirnetCond condname = { 0 };                                                  \
  static void BIRNET_CONSTRUCTOR                                                \
  BIRNET_CPP_PASTE4 (__birnet_cond__autoinit, __LINE__, __, condname) (void)    \
  { ThreadTable.cond_chain4init (&condname); }

/* --- atomicity tests --- */
static volatile guint atomic_count = 0;
static BirnetMutex    atomic_mutex;
static BirnetCond     atomic_cond;

static void
atomic_up_thread (gpointer data)
{
  volatile int *ip = (int*) data;
  for (guint i = 0; i < 25; i++)
    ThreadTable.atomic_int_add (ip, +3);
  birnet_mutex_lock (&atomic_mutex);
  atomic_count -= 1;
  ThreadTable.cond_signal (&atomic_cond);
  birnet_mutex_unlock (&atomic_mutex);
  TASSERT (strcmp (ThreadTable.thread_name (ThreadTable.thread_self()), "AtomicTest") == 0);
}

static void
atomic_down_thread (gpointer data)
{
  volatile int *ip = (int*) data;
  for (guint i = 0; i < 25; i++)
    ThreadTable.atomic_int_add (ip, -4);
  birnet_mutex_lock (&atomic_mutex);
  atomic_count -= 1;
  ThreadTable.cond_signal (&atomic_cond);
  birnet_mutex_unlock (&atomic_mutex);
  TASSERT (strcmp (ThreadTable.thread_name (ThreadTable.thread_self()), "AtomicTest") == 0);
}

static void
test_atomic (void)
{
  TSTART ("AtomicThreading");
  int count = 44;
  BirnetThread *threads[count];
  volatile int atomic_counter = 0;
  birnet_mutex_init (&atomic_mutex);
  ThreadTable.cond_init (&atomic_cond);
  atomic_count = count;
  for (int i = 0; i < count; i++)
    {
      threads[i] = birnet_thread_run ("AtomicTest", (i&1) ? atomic_up_thread : atomic_down_thread, (void*) &atomic_counter);
      TASSERT (threads[i]);
    }
  birnet_mutex_lock (&atomic_mutex);
  while (atomic_count > 0)
    {
      TACK();
      ThreadTable.cond_wait (&atomic_cond, &atomic_mutex);
    }
  birnet_mutex_unlock (&atomic_mutex);
  int result = count / 2 * 25 * +3 + count / 2 * 25 * -4;
  // g_printerr ("{ %d ?= %d }", atomic_counter, result);
  for (int i = 0; i < count; i++)
    ThreadTable.thread_unref (threads[i]);
  TASSERT (atomic_counter == result);
  TDONE ();
}

/* --- basic threading tests --- */
static void
plus1_thread (gpointer data)
{
  guint *tdata = (guint*) data;
  ThreadTable.thread_sleep (-1);
  *tdata += 1;
  while (!ThreadTable.thread_aborted ())
    ThreadTable.thread_sleep (-1);
}

static BIRNET_MUTEX_DECLARE_INITIALIZED (static_mutex);
static BIRNET_REC_MUTEX_DECLARE_INITIALIZED (static_rec_mutex);
static BIRNET_COND_DECLARE_INITIALIZED (static_cond);

static void
test_threads (void)
{
  static BirnetMutex test_mutex;
  gboolean locked;
  TSTART ("Threading");
  /* test C mutex */
  birnet_mutex_init (&test_mutex);
  locked = birnet_mutex_trylock (&test_mutex);
  TASSERT (locked);
  locked = birnet_mutex_trylock (&test_mutex);
  TASSERT (!locked);
  birnet_mutex_unlock (&test_mutex);
  birnet_mutex_destroy (&test_mutex);
  /* not initializing static_mutex */
  locked = birnet_mutex_trylock (&static_mutex);
  TASSERT (locked);
  locked = birnet_mutex_trylock (&static_mutex);
  TASSERT (!locked);
  birnet_mutex_unlock (&static_mutex);
  locked = birnet_mutex_trylock (&static_mutex);
  TASSERT (locked);
  birnet_mutex_unlock (&static_mutex);
  /* not initializing static_rec_mutex */
  locked = birnet_rec_mutex_trylock (&static_rec_mutex);
  TASSERT (locked);
  birnet_rec_mutex_lock (&static_rec_mutex);
  locked = birnet_rec_mutex_trylock (&static_rec_mutex);
  TASSERT (locked);
  birnet_rec_mutex_unlock (&static_rec_mutex);
  birnet_rec_mutex_unlock (&static_rec_mutex);
  birnet_rec_mutex_unlock (&static_rec_mutex);
  locked = birnet_rec_mutex_trylock (&static_rec_mutex);
  TASSERT (locked);
  birnet_rec_mutex_unlock (&static_rec_mutex);
  /* not initializing static_cond */
  ThreadTable.cond_signal (&static_cond);
  ThreadTable.cond_broadcast (&static_cond);
  /* test C++ mutex */
  static Mutex mutex;
  static RecMutex rmutex;
  mutex.lock();
  rmutex.lock();
  mutex.unlock();
  rmutex.unlock();
  guint thread_data1 = 0;
  BirnetThread *thread1 = birnet_thread_run ("plus1", plus1_thread, &thread_data1);
  guint thread_data2 = 0;
  BirnetThread *thread2 = birnet_thread_run ("plus2", plus1_thread, &thread_data2);
  guint thread_data3 = 0;
  BirnetThread *thread3 = birnet_thread_run ("plus3", plus1_thread, &thread_data3);
  TASSERT (thread1 != NULL);
  TASSERT (thread2 != NULL);
  TASSERT (thread3 != NULL);
  TASSERT (thread_data1 == 0);
  TASSERT (thread_data2 == 0);
  TASSERT (thread_data3 == 0);
  TASSERT (ThreadTable.thread_get_running (thread1) == TRUE);
  TASSERT (ThreadTable.thread_get_running (thread2) == TRUE);
  TASSERT (ThreadTable.thread_get_running (thread3) == TRUE);
  ThreadTable.thread_wakeup (thread1);
  ThreadTable.thread_wakeup (thread2);
  ThreadTable.thread_wakeup (thread3);
  ThreadTable.thread_abort (thread1);
  ThreadTable.thread_abort (thread2);
  ThreadTable.thread_abort (thread3);
  TASSERT (thread_data1 > 0);
  TASSERT (thread_data2 > 0);
  TASSERT (thread_data3 > 0);
  ThreadTable.thread_unref (thread1);
  ThreadTable.thread_unref (thread2);
  ThreadTable.thread_unref (thread3);
  TDONE ();
}

/* --- C++ threading tests --- */
struct ThreadA : public virtual Birnet::Thread {
  int value;
  volatile int *counter;
  ThreadA (volatile int *counterp,
           int           v) :
    Thread ("ThreadA"),
    value (v), counter (counterp)
  {}
  virtual void
  run ()
  {
    TASSERT (this->name() == "ThreadA");
    TASSERT (this->name() == Thread::Self::name());
    for (int j = 0; j < 17905; j++)
      ThreadTable.atomic_int_add (counter, value);
  }
};

template<class M> static bool
lockable (M &mutex)
{
  bool lockable = mutex.trylock();
  if (lockable)
    mutex.unlock();
  return lockable;
}

static void
test_thread_cxx (void)
{
  TSTART ("C++Threading");
  TASSERT (NULL != &Thread::self());
  volatile int atomic_counter = 0;
  int result = 0;
  int count = 35;
  Birnet::Thread *threads[count];
  for (int i = 0; i < count; i++)
    {
      int v = rand();
      for (int j = 0; j < 17905; j++)
        result += v;
      threads[i] = new ThreadA (&atomic_counter, v);
      TASSERT (threads[i]);
      ref_sink (threads[i]);
    }
  TASSERT (atomic_counter == 0);
  for (int i = 0; i < count; i++)
    threads[i]->start();
  for (int i = 0; i < count; i++)
    {
      threads[i]->wait_for_exit();
      unref (threads[i]);
    }
  TASSERT (atomic_counter == result);
  TDONE ();

  TSTART ("C++OwnedMutex");
  static OwnedMutex static_omutex;
  static_omutex.lock();
  TASSERT (static_omutex.mine() == true);
  static_omutex.unlock();
  TASSERT (NULL != &Thread::self());
  OwnedMutex omutex;
  TASSERT (omutex.owner() == NULL);
  TASSERT (omutex.mine() == false);
  omutex.lock();
  TASSERT (omutex.owner() == &Thread::self());
  TASSERT (omutex.mine() == true);
  TASSERT (lockable (omutex) == true);
  bool locked = omutex.trylock();
  TASSERT (locked == true);
  omutex.unlock();
  omutex.unlock();
  TASSERT (omutex.owner() == NULL);
  TASSERT (lockable (omutex) == true);
  TASSERT (omutex.owner() == NULL);
  locked = omutex.trylock();
  TASSERT (locked == true);
  TASSERT (omutex.owner() == &Thread::self());
  TASSERT (lockable (omutex) == true);
  omutex.unlock();
  TASSERT (omutex.owner() == NULL);
  TDONE();
}

/* --- auto locker tests --- */
static void
test_simple_auto_lock (Mutex &mutex1,
                       Mutex &mutex2)
{
  TASSERT (lockable (mutex1) == true);
  TASSERT (lockable (mutex2) == true);

  AutoLocker locker1 (mutex1);

  TASSERT (lockable (mutex1) == false);
  TASSERT (lockable (mutex2) == true);

  AutoLocker locker2 (&mutex2);

  TASSERT (lockable (mutex1) == false);
  TASSERT (lockable (mutex2) == false);
}

static void
test_recursive_auto_lock (RecMutex &rec_mutex,
                          guint     depth)
{
  AutoLocker locker (rec_mutex);
  if (depth > 1)
    test_recursive_auto_lock (rec_mutex, depth - 1);
  else
    {
      locker.relock();
      locker.relock();
      locker.relock();
      bool lockable1 = rec_mutex.trylock();
      bool lockable2 = rec_mutex.trylock();
      TASSERT (lockable1 && lockable2);
      rec_mutex.unlock();
      rec_mutex.unlock();
      locker.unlock();
      locker.unlock();
      locker.unlock();
    }
}

// helper class for testing auto locking, which counts the lock() and unlock() calls
class LockCounter {
  guint m_lock_count;
public:
  LockCounter() :
    m_lock_count (0)
  {
  }
  void
  lock()
  {
    m_lock_count++;
  }
  void
  unlock()
  {
    TASSERT (m_lock_count > 0);
    m_lock_count--;
  }
  guint
  lock_count() const
  {
    return m_lock_count;
  }
};

class LockCountAssert {
  const LockCounter &m_lock_counter;
  const guint        m_required_lock_count;
public:
  LockCountAssert (const LockCounter& lock_counter,
		   guint              required_lock_count) :
    m_lock_counter (lock_counter),
    m_required_lock_count (required_lock_count)
  {
    TASSERT (m_lock_counter.lock_count() == m_required_lock_count);
  }
  ~LockCountAssert()
  {
    TASSERT (m_lock_counter.lock_count() == m_required_lock_count);
  }
};

/* Check that C++ constructors and destructors and the AutoLocker constructor
 * and destructor will be executed in the order we need, that is: an AutoLocker
 * that is created before an object should protect its constructor and
 * destructor, an AutoLocker created after an object should not affect its
 * constructor and destructor.
 */
static void
test_auto_locker_order()
{
  LockCounter lock_counter1;
  LockCounter lock_counter2;

  for (guint i = 0; i < 3; i++)
    {
      LockCountAssert lc_assert1 (lock_counter1, 0);
      LockCountAssert lc_assert2 (lock_counter2, 0);

      AutoLocker      auto_locker1 (lock_counter1);

      LockCountAssert lc_assert3 (lock_counter1, 1);
      LockCountAssert lc_assert4 (lock_counter2, 0);

      AutoLocker      auto_locker2 (lock_counter2);

      LockCountAssert lc_assert5 (lock_counter1, 1);
      LockCountAssert lc_assert6 (lock_counter2, 1);

      AutoLocker      auto_locker3 (lock_counter1);

      LockCountAssert lc_assert7 (lock_counter1, 2);
      LockCountAssert lc_assert8 (lock_counter2, 1);
    }
}

static void
test_auto_locker_counting()
{
  /* check Birnet::AutoLocker lock counting */
  LockCounter lock_counter;
  TASSERT (lock_counter.lock_count() == 0);
  {
    AutoLocker auto_locker (lock_counter);
    TASSERT (lock_counter.lock_count() == 1);
    auto_locker.relock();
    TASSERT (lock_counter.lock_count() == 2);
    auto_locker.relock();
    TASSERT (lock_counter.lock_count() == 3);
    auto_locker.unlock();
    TASSERT (lock_counter.lock_count() == 2);
    auto_locker.relock();
    TASSERT (lock_counter.lock_count() == 3);
    auto_locker.relock();
    TASSERT (lock_counter.lock_count() == 4);
  }
  TASSERT (lock_counter.lock_count() == 0);
}

static void
test_auto_locker_cxx()
{
  TSTART ("C++AutoLocker");
  if (true)
    {
      struct CheckAutoLocker : public AutoLocker {
        CheckAutoLocker (Mutex &dummy) :
          AutoLocker (dummy)
        {}
        void
        assert_auto_locker_impl (Mutex &dummy)
        {
          assert_impl (dummy);
        }
      };
      Mutex dummy;
      CheckAutoLocker check_auto_locker (dummy);
      check_auto_locker.assert_auto_locker_impl (dummy);
      TICK();
    }
  Mutex mutex1, mutex2;
  RecMutex rec_mutex;

  TASSERT (lockable (mutex1) == true);
  TASSERT (lockable (mutex2) == true);
  test_simple_auto_lock (mutex1, mutex2);
  test_simple_auto_lock (mutex1, mutex2);
  TASSERT (lockable (mutex1) == true);
  TASSERT (lockable (mutex2) == true);

  test_recursive_auto_lock (rec_mutex, 30);
  AutoLocker locker (&rec_mutex);
  test_recursive_auto_lock (rec_mutex, 17);

  test_auto_locker_order();
  
  test_auto_locker_counting();

  TDONE();
}

/* --- auto locker benchmarks --- */
#define RUNS (500000)

class HeapLocker {
  // like PtrAutoLocker but allocates on the heap
  struct Lockable {
    virtual void lock     () = 0;
    virtual void unlock   () = 0;
    virtual     ~Lockable () {}
  };
  template<class L>
  struct Wrapper : public virtual Lockable {
    L &l;
    explicit     Wrapper  (L &o) : l (o) {}
    virtual void lock     () { l.lock(); }
    virtual void unlock   () { l.unlock(); }
  };
  Lockable &l;
public:
  template<class C>
  HeapLocker (C &c) :
    l (*new Wrapper<C> (c))
  {
    lock();
  }
  ~HeapLocker()
  {
    unlock();
    delete &l;
  }
  void lock     () { l.lock(); }
  void unlock   () { l.unlock(); }
};

static void
bench_heap_auto_locker()
{
  Mutex mutex;
  RecMutex rmutex;
  for (uint i = 0; i < RUNS; i++)
    {
      HeapLocker locker1 (mutex);
      HeapLocker locker2 (rmutex);
    }
}

static void
bench_direct_auto_locker()
{
  // supports auto locking only for RecMutex and Mutex
  class AutoLocker2 {
    union {
      Mutex    *m_mutex;
      RecMutex *m_rec_mutex;
    };
    const bool m_recursive;
    BIRNET_PRIVATE_CLASS_COPY (AutoLocker2);
  public:
    AutoLocker2 (Mutex &mutex) :
      m_recursive (false)
    {
      m_mutex = &mutex;
      relock();
    }
    AutoLocker2 (RecMutex &mutex) :
      m_recursive (true)
    {
      m_rec_mutex = &mutex;
      relock();
    }
    AutoLocker2 (RecMutex *rec_mutex) :
      m_recursive (true) {
      BIRNET_ASSERT (rec_mutex != NULL);
      m_rec_mutex = rec_mutex;
      relock();
    }
    void          relock        () const { if (m_recursive) m_rec_mutex->lock(); else m_mutex->lock(); }
    void          unlock        () const { if (m_recursive) m_rec_mutex->unlock(); else m_mutex->unlock(); }
    /*Des*/       ~AutoLocker2   () { unlock(); }
  };
  Mutex mutex;
  RecMutex rmutex;
  for (uint i = 0; i < RUNS; i++)
    {
      AutoLocker2 locker1 (mutex);
      AutoLocker2 locker2 (rmutex);
    }
}

class GenericAutoLocker {
  // supports automated scope-bound lock/unlock for all kinds of objects
  struct Locker {
    virtual void lock   () const = 0;
    virtual void unlock () const = 0;
    virtual     ~Locker () {}
  };
  template<class Lockable>
  struct LockerImpl : public Locker {
    Lockable    *lockable;
    virtual void lock       () const      { lockable->lock(); }
    virtual void unlock     () const      { lockable->unlock(); }
    explicit     LockerImpl (Lockable *l) : lockable (l) {}
  };
  void  *space[2];
  BIRNET_PRIVATE_CLASS_COPY (GenericAutoLocker);
  inline const Locker*          locker      () const             { return static_cast<const Locker*> ((const void*) &space); }
protected:
  /* assert implicit assumption of the GenericAutoLocker implementation */
  template<class Lockable> void
  assert_impl (Lockable &lockable)
  {
    BIRNET_ASSERT (sizeof (LockerImpl<Lockable>) <= sizeof (space));
    Locker *laddr = new (space) LockerImpl<Lockable> (&lockable);
    BIRNET_ASSERT (laddr == locker());
  }
public:
  template<class Lockable>      GenericAutoLocker  (Lockable *lockable) { new (space) LockerImpl<Lockable> (lockable); relock(); }
  template<class Lockable>      GenericAutoLocker  (Lockable &lockable) { new (space) LockerImpl<Lockable> (&lockable); relock(); }
  void                          relock             () const             { locker()->lock(); }
  void                          unlock             () const             { locker()->unlock(); }
  /*Des*/                       ~GenericAutoLocker ()                   { unlock(); }
};

static void
bench_generic_auto_locker()
{
  Mutex mutex;
  RecMutex rmutex;
  for (uint i = 0; i < RUNS; i++)
    {
      GenericAutoLocker locker1 (mutex);
      GenericAutoLocker locker2 (rmutex);
    }
}

class PtrAutoLocker {
  // like GenericAutoLocker but uses an extra pointer
  struct Locker {
    virtual void lock   () = 0;
    virtual void unlock () = 0;
    virtual     ~Locker () {}
  };
  template<class Lockable>
  struct LockerImpl : public Locker {
    Lockable    *lockable;
    virtual void lock       () { lockable->lock(); }
    virtual void unlock     () { lockable->unlock(); }
    explicit     LockerImpl (Lockable *l) : lockable (l) {}
  };
  void  *space[2];
  Locker *locker;
  BIRNET_PRIVATE_CLASS_COPY (PtrAutoLocker);
public:
  template<class Lockable>      PtrAutoLocker  (Lockable *lockable) { locker = new (space) LockerImpl<Lockable> (lockable); relock(); }
  template<class Lockable>      PtrAutoLocker  (Lockable &lockable) { locker = new (space) LockerImpl<Lockable> (&lockable); relock(); }
  void                          relock         () { locker->lock(); }
  void                          unlock         () { locker->unlock(); }
  /*Des*/                       ~PtrAutoLocker () { unlock(); }
};

static void
bench_ptr_auto_locker()
{
  Mutex mutex;
  RecMutex rmutex;
  for (uint i = 0; i < RUNS; i++)
    {
      PtrAutoLocker locker1 (mutex);
      PtrAutoLocker locker2 (rmutex);
    }
}

static void
bench_birnet_auto_locker()
{
  Mutex mutex;
  RecMutex rmutex;
  for (uint i = 0; i < RUNS; i++)
    {
      AutoLocker locker1 (mutex);
      AutoLocker locker2 (rmutex);
    }
}

static void
bench_manual_locker()
{
  Mutex mutex;
  RecMutex rmutex;
  for (uint i = 0; i < RUNS; i++)
    {
      mutex.lock();
      rmutex.lock();
      mutex.unlock();
      rmutex.unlock();
    }
}

static void
bench_auto_locker_cxx()
{
  TSTART ("C++AutoLocker-Benchmark");
  GTimer *timer = g_timer_new();
  const guint n_repeatitions = 7;
  TICK();
  /* bench manual locker */
  guint xdups = TEST_CALIBRATION (35.0, bench_manual_locker());
  double xmin = 9e300;
  for (guint i = 0; i < n_repeatitions; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < xdups; j++)
        bench_manual_locker();
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < xmin)
        xmin = e;
      TICK();
    }
  TACK();
  /* bench direct auto locker */
  guint sdups = TEST_CALIBRATION (60.0, bench_direct_auto_locker());
  double smin = 9e300;
  for (guint i = 0; i < n_repeatitions; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < sdups; j++)
        bench_direct_auto_locker();
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < smin)
        smin = e;
      TICK();
    }
  TACK();
  /* bench birnet auto locker */
  guint bdups = TEST_CALIBRATION (60.0, bench_birnet_auto_locker());
  double bmin = 9e300;
  for (guint i = 0; i < n_repeatitions; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < bdups; j++)
        bench_birnet_auto_locker();
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < bmin)
        bmin = e;
      TICK();
    }
  TACK();
  /* bench generic auto locker */
  guint gdups = TEST_CALIBRATION (60.0, bench_generic_auto_locker());
  double gmin = 9e300;
  for (guint i = 0; i < n_repeatitions; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < gdups; j++)
        bench_generic_auto_locker();
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < gmin)
        gmin = e;
      TICK();
    }
  TACK();
  /* bench ptr auto locker */
  guint pdups = TEST_CALIBRATION (60.0, bench_ptr_auto_locker());
  double pmin = 9e300;
  for (guint i = 0; i < n_repeatitions; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < pdups; j++)
        bench_ptr_auto_locker();
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < pmin)
        pmin = e;
      TICK();
    }
  TACK();
  /* bench heap auto locker */
  guint tdups = TEST_CALIBRATION (60.0, bench_heap_auto_locker());
  double tmin = 9e300;
  for (guint i = 0; i < n_repeatitions; i++)
    {
      g_timer_start (timer);
      for (guint j = 0; j < tdups; j++)
        bench_heap_auto_locker();
      g_timer_stop (timer);
      double e = g_timer_elapsed (timer, NULL);
      if (e < tmin)
        tmin = e;
      TICK();
    }
  TACK();
  /* done, report */
  TDONE();
  treport_minimized ("Manual-Locker",      xmin / xdups / RUNS * 1000. * 1000. * 1000., TUNIT_NSEC);
  treport_minimized ("Direct-AutoLocker",  smin / sdups / RUNS * 1000. * 1000. * 1000., TUNIT_NSEC);
  treport_minimized ("Generic-AutoLocker", gmin / gdups / RUNS * 1000. * 1000. * 1000., TUNIT_NSEC);
  treport_minimized ("Birnet-AutoLocker",  bmin / bdups / RUNS * 1000. * 1000. * 1000., TUNIT_NSEC);
  treport_minimized ("Pointer-AutoLocker", pmin / pdups / RUNS * 1000. * 1000. * 1000., TUNIT_NSEC);
  treport_minimized ("Heap-AutoLocker",    tmin / tdups / RUNS * 1000. * 1000. * 1000., TUNIT_NSEC);
}

/* --- C++ atomicity tests --- */
static void
test_thread_atomic_cxx (void)
{
  TSTART ("C++AtomicThreading");
  /* integer functions */
  volatile int ai, r;
  Atomic::int_set (&ai, 17);
  TASSERT (ai == 17);
  r = Atomic::int_get (&ai);
  TASSERT (r == 17);
  Atomic::int_add (&ai, 9);
  r = Atomic::int_get (&ai);
  TASSERT (r == 26);
  Atomic::int_set (&ai, -1147483648);
  TASSERT (ai == -1147483648);
  r = Atomic::int_get (&ai);
  TASSERT (r == -1147483648);
  Atomic::int_add (&ai, 9);
  r = Atomic::int_get (&ai);
  TASSERT (r == -1147483639);
  Atomic::int_add (&ai, -20);
  r = Atomic::int_get (&ai);
  TASSERT (r == -1147483659);
  r = Atomic::int_cas (&ai, 17, 19);
  TASSERT (r == false);
  r = Atomic::int_get (&ai);
  TASSERT (r == -1147483659);
  r = Atomic::int_cas (&ai, -1147483659, 19);
  TASSERT (r == true);
  r = Atomic::int_get (&ai);
  TASSERT (r == 19);
  r = Atomic::int_swap_add (&ai, 1);
  TASSERT (r == 19);
  r = Atomic::int_get (&ai);
  TASSERT (r == 20);
  r = Atomic::int_swap_add (&ai, -20);
  TASSERT (r == 20);
  r = Atomic::int_get (&ai);
  TASSERT (r == 0);
  /* pointer functions */
  void * volatile ap, * volatile p;
  Atomic::ptr_set (&ap, (void*) 119);
  TASSERT (ap == (void*) 119);
  p = Atomic::ptr_get (&ap);
  TASSERT (p == (void*) 119);
  r = Atomic::ptr_cas (&ap, (void*) 17, (void*) -42);
  TASSERT (r == false);
  p = Atomic::ptr_get (&ap);
  TASSERT (p == (void*) 119);
  r = Atomic::ptr_cas (&ap, (void*) 119, (void*) 4294967279U);
  TASSERT (r == true);
  p = Atomic::ptr_get (&ap);
  TASSERT (p == (void*) 4294967279U);
  TDONE ();
}

/* --- thread_yield --- */
static inline void
handle_contention ()
{
  /* we're waiting for our contention counterpart if we got here:
   * - sched_yield(3posix) will immediately give up the CPU and let another
   *   task run. but if the contention counterpart is running on another
   *   CPU this will lead to scheduler trashing on our CPU. and if other
   *   bacground tasks are running, they could get all our CPU time,
   *   because sched_yield() effectively discards the current time slice.
   * - busy spinning is useful if the contention counterpart runs on a
   *   different CPU, as long as the loop doesn't involve syncronization
   *   primitives which cause IO bus trashing ("lock" prefix in x86 asm).
   * - usleep(3posix) is a way to give up the CPU without discarding our
   *   time slices and avoids scheduler or bus trashing. allthough it is
   *   not the perfect or optimum syncronization/timing primitive, it
   *   avoids most ill effects and still allows for a sufficient number
   *   of task switches.
   */
  usleep (500); // 1usec is the minimum value to cause an effect
}

/* --- ring buffer --- */
typedef Atomic::RingBuffer<int> IntRingBuffer;
class IntSequence {
  uint32 accu;
public:
  explicit      IntSequence() : accu (123456789) {}
  inline int32  gen_int    () { accu = 1664525 * accu + 1013904223; return accu; }
};
#define CONTENTION_PRINTF       if(1);else g_printerr
struct RingBufferWriter : public virtual Birnet::Thread, IntSequence {
  IntRingBuffer *ring;
  uint           ring_buffer_test_length;
  RingBufferWriter (IntRingBuffer *rb,
                    uint           rbtl) :
    Thread ("RingBufferWriter"),
    ring (rb), ring_buffer_test_length (rbtl)
  {}
  virtual void
  run ()
  {
    TPRINT ("%s start.", Thread::Self::name().c_str());
    for (uint l = 0; l < ring_buffer_test_length;)
      {
        uint k, n = g_random_int() % MIN (ring_buffer_test_length - l + 1, 65536 * 2);
        int buffer[n], *b = buffer;
        for (uint i = 0; i < n; i++)
          b[i] = gen_int();
        uint j = n;
        while (j)
          {
            k = ring->write (j, b);
            TCHECK (k <= j);
            j -= k;
            b += k;
            if (!k)     // waiting for reader thread
              handle_contention();
            CONTENTION_PRINTF (k ? "*" : "/");
          }
        if (l / 499999 != (l + n) / 499999)
          TICK();
        l += n;
      }
    TPRINT ("%s done.", Thread::Self::name().c_str());
  }
};
struct RingBufferReader : public virtual Birnet::Thread, IntSequence {
  IntRingBuffer *ring;
  uint           ring_buffer_test_length;
  RingBufferReader (IntRingBuffer *rb,
                    uint           rbtl) :
    Thread ("RingBufferReader"),
    ring (rb), ring_buffer_test_length (rbtl)
  {}
  virtual void
  run ()
  {
    TPRINT ("%s start.", Thread::Self::name().c_str());
    for (uint l = 0; l < ring_buffer_test_length;)
      {
        uint k, n = ring->n_readable();
        n = lrand48() % MIN (n + 1, 65536 * 2);
        int buffer[n], *b = buffer;
        if (rand() & 1)
          {
            k = ring->read (n, b, false);
            TCHECK (n == k);
            if (k)
              CONTENTION_PRINTF ("+");
          }
        else
          {
            k = ring->read (n, b, true);
            TCHECK (k <= n);
            if (!k)         // waiting for writer thread
              handle_contention();
            CONTENTION_PRINTF (k ? "+" : "\\");
          }
        for (uint i = 0; i < k; i++)
          TCHECK (b[i] == gen_int());
        if (l / 499999 != (l + k) / 499999)
          TACK();
        l += k;
      }
    TPRINT ("%s done.", Thread::Self::name().c_str());
  }
};

static void
test_ring_buffer ()
{
  static const gchar *testtext = "Ring Buffer test Text (47\xff)";
  uint n, ttl = strlen (testtext);
  TSTART ("RingBuffer");
  Atomic::RingBuffer<char> rb1 (ttl);
  TASSERT (rb1.n_writable() == ttl);
  n = rb1.write (ttl, testtext);
  TASSERT (n == ttl);
  TASSERT (rb1.n_writable() == 0);
  TASSERT (rb1.n_readable() == ttl);
  char buffer[8192];
  n = rb1.read (8192, buffer);
  TASSERT (n == ttl);
  TASSERT (rb1.n_readable() == 0);
  TASSERT (rb1.n_writable() == ttl);
  TASSERT (strncmp (buffer, testtext, n) == 0);
  TDONE();

  /* check lower end ring buffer sizes (high contention test) */
  for (uint step = 1; step < 8; step++)
    {
      uint ring_buffer_test_length = 17 * step + (rand() % 19);
      TSTART ("AsyncRingBuffer-%d-%d", step, ring_buffer_test_length);
      IntRingBuffer irb (step);
      RingBufferReader *rbr = new RingBufferReader (&irb, ring_buffer_test_length);
      ref_sink (rbr);
      RingBufferWriter *rbw = new RingBufferWriter (&irb, ring_buffer_test_length);
      ref_sink (rbw);
      TASSERT (rbr && rbw);
      rbr->start();
      rbw->start();
      rbw->wait_for_exit();
      rbr->wait_for_exit();
      TASSERT (rbr && rbw);
      unref (rbr);
      unref (rbw);
      TDONE();
    }

  /* check big ring buffer sizes */
  if (true)
    {
      TSTART ("AsyncRingBuffer-big");
      uint ring_buffer_test_length = 999999 * (init_settings().test_quick ? 1 : 20);
      IntRingBuffer irb (16384 + (lrand48() % 8192));
      RingBufferReader *rbr = new RingBufferReader (&irb, ring_buffer_test_length);
      ref_sink (rbr);
      RingBufferWriter *rbw = new RingBufferWriter (&irb, ring_buffer_test_length);
      ref_sink (rbw);
      TASSERT (rbr && rbw);
      rbr->start();
      rbw->start();
      rbw->wait_for_exit();
      rbr->wait_for_exit();
      TASSERT (rbr && rbw);
      unref (rbr);
      unref (rbw);
      TDONE();
    }
}

/* --- late deletable destruction --- */
static bool deletable_destructor = false;
struct MyDeletable : public virtual Deletable {
  virtual
  ~MyDeletable()
  {
    deletable_destructor = true;
  }
  void
  force_deletion_hooks()
  {
    invoke_deletion_hooks();
  }
};
struct MyDeletableHook : public Deletable::DeletionHook {
  Deletable *deletable;
  explicit     MyDeletableHook () :
    deletable (NULL)
  {}
  virtual void
  monitoring_deletable (Deletable &deletable_obj)
  {
    TASSERT (deletable == NULL);
    deletable = &deletable_obj;
  }
  virtual void
  dismiss_deletable ()
  {
    if (deletable)
      deletable = NULL;
  }
  virtual
  ~MyDeletableHook ()
  {
    // g_printerr ("~MyDeletableHook(): deletable=%p\n", deletable);
    if (deletable)
      deletable_remove_hook (deletable);
    deletable = NULL;
  }
};

static MyDeletable early_deletable __attribute__ ((init_priority (101)));
static MyDeletable late_deletable __attribute__ ((init_priority (65535)));

static void
test_deletable_destruction ()
{
  TSTART ("Deletable destruction");
  {
    MyDeletable test_deletable;
    TICK();
    MyDeletableHook dhook1;
    // g_printerr ("TestHook=%p\n", (Deletable::DeletionHook*) &dhook1);
    dhook1.deletable_add_hook (&test_deletable);
    TICK();
    dhook1.deletable_remove_hook (&test_deletable);
    dhook1.dismiss_deletable();
    TICK();
    MyDeletableHook dhook2;
    dhook2.deletable_add_hook (&test_deletable);
    test_deletable.force_deletion_hooks ();
    TICK();
    MyDeletableHook dhook3;
    dhook3.deletable_add_hook (&test_deletable);
    TICK();
    /* automatic deletion hook invocation */
    /* FIXME: deletable destructor is called first and doesn't auto-remove
     * - if deletion hooks were ring-linked, we could at least catch this case in ~DeletionHook
     */
  }
  MyDeletable *deletable2 = new MyDeletable;
  TASSERT (deletable2 != NULL);
  deletable_destructor = false;
  delete deletable2;
  TASSERT (deletable_destructor == true);
  TDONE();
  /* early_deletable and late_deletable are only tested at program end */
}

/* --- Mutextes before g_thread_init() --- */
static void
test_before_thread_init()
{
  /* check C++ mutex init + destruct before g_thread_init() */
  Mutex *mutex = new Mutex;
  RecMutex *rmutex = new RecMutex;
  Cond *cond = new Cond;
  delete mutex;
  delete rmutex;
  delete cond;
}

} // Anon

static guint constructur_attribute_test = 0;

static void BIRNET_CONSTRUCTOR
constructur_attribute_test_initializer (void)
{
  constructur_attribute_test = 0x1237ABBA;
}

int
main (int   argc,
      char *argv[])
{
  if (constructur_attribute_test != 305638330)
    g_error ("%s: static constructors have not been called before main", G_STRFUNC);

  test_before_thread_init();

  birnet_init_test (&argc, &argv);

  test_threads();
  test_atomic();
  test_thread_cxx();
  test_thread_atomic_cxx();
  test_auto_locker_cxx();
  test_deletable_destruction();
  test_ring_buffer(); 
  if (init_settings().test_perf)
    bench_auto_locker_cxx();
  
  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
