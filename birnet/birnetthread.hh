// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BIRNET_THREAD_XX_HH__
#define __BIRNET_THREAD_XX_HH__
#include <birnet/birnetutils.hh>
namespace Birnet {
class Thread;
class Mutex {
  BirnetMutex mutex;
  friend class Cond;
  BIRNET_PRIVATE_CLASS_COPY (Mutex);
public:
  explicit      Mutex   ();
  void          lock    ()                      { ThreadTable.mutex_lock (&mutex); }
  void          unlock  ()                      { ThreadTable.mutex_unlock (&mutex); }
  bool          trylock ()                      { return 0 == ThreadTable.mutex_trylock (&mutex); /* TRUE indicates success */ }
  /*Des*/       ~Mutex  ();
};
class RecMutex {
  BirnetRecMutex rmutex;
  BIRNET_PRIVATE_CLASS_COPY (RecMutex);
public:
  explicit      RecMutex  ();
  void          lock      ()                    { ThreadTable.rec_mutex_lock (&rmutex); }
  void          unlock    ()                    { ThreadTable.rec_mutex_unlock (&rmutex); }
  bool          trylock   ()                    { return 0 == ThreadTable.rec_mutex_trylock (&rmutex); /* TRUE indicates success */ }
  /*Des*/       ~RecMutex ();
};
class Cond {
  BirnetCond cond;
  BIRNET_PRIVATE_CLASS_COPY (Cond);
public:
  explicit      Cond          ();
  void          signal        ()                { ThreadTable.cond_signal (&cond); }
  void          broadcast     ()                { ThreadTable.cond_broadcast (&cond); }
  void          wait          (Mutex &m)        { ThreadTable.cond_wait (&cond, &m.mutex); }
  void          wait_timed    (Mutex &m,
                               int64 max_usecs) { ThreadTable.cond_wait_timed (&cond, &m.mutex, max_usecs); }
  /*Des*/       ~Cond         ();
};
namespace Atomic {
inline void    read_barrier  (void)                                { BIRNET_MEMORY_BARRIER_RO (ThreadTable); }
inline void    write_barrier (void)                                { BIRNET_MEMORY_BARRIER_WO (ThreadTable); }
inline void    full_barrier  (void)                                { BIRNET_MEMORY_BARRIER_RW (ThreadTable); }
/* atomic integers */
inline void    int_set       (volatile int  *iptr, int value)      { ThreadTable.atomic_int_set (iptr, value); }
inline int     int_get       (volatile int  *iptr)                 { return ThreadTable.atomic_int_get (iptr); }
inline bool    int_cas       (volatile int  *iptr, int o, int n)   { return ThreadTable.atomic_int_cas (iptr, o, n); }
inline void    int_add       (volatile int  *iptr, int diff)       { ThreadTable.atomic_int_add (iptr, diff); }
inline int     int_swap_add  (volatile int  *iptr, int diff)       { return ThreadTable.atomic_int_swap_add (iptr, diff); }
/* atomic unsigned integers */
inline void    uint_set      (volatile uint *uptr, uint value)     { ThreadTable.atomic_uint_set (uptr, value); }
inline uint    uint_get      (volatile uint *uptr)                 { return ThreadTable.atomic_uint_get (uptr); }
inline bool    uint_cas      (volatile uint *uptr, uint o, uint n) { return ThreadTable.atomic_uint_cas (uptr, o, n); }
inline void    uint_add      (volatile uint *uptr, uint diff)      { ThreadTable.atomic_uint_add (uptr, diff); }
inline uint    uint_swap_add (volatile uint *uptr, uint diff)      { return ThreadTable.atomic_uint_swap_add (uptr, diff); }
/* atomic pointers */
template<class V>
inline void    ptr_set       (V* volatile *ptr_addr, V *n)      { ThreadTable.atomic_pointer_set ((void**) ptr_addr, (void*) n); }
template<class V>
inline V*      ptr_get       (V* volatile *ptr_addr)            { return (V*) ThreadTable.atomic_pointer_get ((void**) ptr_addr); }
template<class V>
inline V*      ptr_get       (V* volatile const *ptr_addr)      { return (V*) ThreadTable.atomic_pointer_get ((void**) ptr_addr); }
template<class V>
inline bool    ptr_cas       (V* volatile *ptr_adr, V *o, V *n) { return ThreadTable.atomic_pointer_cas ((void**) ptr_adr, (void*) o, (void*) n); }
} // Atomic
class OwnedMutex {
  BirnetRecMutex    m_rec_mutex;
  Thread * volatile m_owner;
  BIRNET_PRIVATE_CLASS_COPY (OwnedMutex);
public:
  explicit       OwnedMutex ();
  inline void    lock       ();
  inline bool    trylock    ();
  inline void    unlock     ();
  inline Thread* owner      ();
  inline bool    mine       ();
  /*Des*/       ~OwnedMutex ();
};
class Thread : public virtual ReferenceCountImpl {
protected:
  explicit              Thread          (const String      &name);
  virtual void          run             () = 0;
  virtual               ~Thread         ();
public:
  void                  start           ();
  int                   pid             () const;
  String                name            () const;
  void                  queue_abort     ();
  void                  abort           ();
  bool                  aborted         ();
  void                  wakeup          ();
  bool                  running         ();
  void                  wait_for_exit   ();
  /* event loop */
  void                  exec_loop       ();
  void                  quit_loop       ();
  /* global methods */
  static void           emit_wakeups    (uint64             stamp);
  static Thread&        self            ();
  /* Self thread */
  struct Self {
    static String       name            ();
    static void         name            (const String      &name);
    static bool         sleep           (long               max_useconds);
    static bool         aborted         ();
    static int          pid             ();
    static void         awake_after     (uint64             stamp);
    static void         set_wakeup      (BirnetThreadWakeup wakeup_func,
                                         void              *wakeup_data,
                                         void             (*destroy_data) (void*));
    static OwnedMutex&  owned_mutex     ();
    static void         yield           ();
    static void         exit            (void              *retval = NULL) BIRNET_NORETURN;
  };
  /* DataListContainer API */
  template<typename Type> inline void set_data    (DataKey<Type> *key,
                                                   Type           data) { thread_lock(); data_list.set (key, data); thread_unlock(); }
  template<typename Type> inline Type get_data    (DataKey<Type> *key)  { thread_lock(); Type d = data_list.get (key); thread_unlock(); return d; }
  template<typename Type> inline Type swap_data   (DataKey<Type> *key)  { thread_lock(); Type d = data_list.swap (key); thread_unlock(); return d; }
  template<typename Type> inline Type swap_data   (DataKey<Type> *key,
                                                   Type           data) { thread_lock(); Type d = data_list.swap (key, data); thread_unlock(); return d; }
  template<typename Type> inline void delete_data (DataKey<Type> *key)  { thread_lock(); data_list.del (key); thread_unlock(); }
  /* implementaiton details */
private:
  DataList              data_list;
  BirnetThread         *bthread;
  OwnedMutex            m_omutex;
  explicit              Thread          (BirnetThread      *thread);
  void                  thread_lock     ()                              { m_omutex.lock(); }
  bool                  thread_trylock  ()                              { return m_omutex.trylock(); }
  void                  thread_unlock   ()                              { m_omutex.unlock(); }
  BIRNET_PRIVATE_CLASS_COPY (Thread);
protected:
  class ThreadWrapperInternal;
  static void threadxx_wrap   (BirnetThread *cthread);
  static void threadxx_delete (void         *cxxthread);
};
/**
 * The AutoLocker class locks mutex like objects on construction, and automatically
 * unlocks on destruction. So putting an AutoLocker object on the stack conveniently
 * ensures that a mutex will be automatically locked and properly unlocked when
 * the function returns or throws an exception.
 * Objects intended to be used by an AutoLocker need to provide the public methods
 * lock() and unlock().
 */
class AutoLocker {
  struct Locker {
    explicit     Locker  () {}
    virtual     ~Locker  () {}
    virtual void lock    () const = 0;
    virtual void unlock  () const = 0;
    BIRNET_PRIVATE_CLASS_COPY (Locker);
  };
  template<class Lockable>
  struct LockerImpl : public Locker {
    Lockable    *lockable;
    explicit     LockerImpl (Lockable *l) : lockable (l) {}
    virtual     ~LockerImpl () {}
    virtual void lock       () const      { lockable->lock(); }
    virtual void unlock     () const      { lockable->unlock(); }
    BIRNET_PRIVATE_CLASS_COPY (LockerImpl);
  };
  union {
    void       *pointers[sizeof (LockerImpl<Mutex>) / sizeof (void*)];  // union needs pointer alignment
    char        chars[sizeof (LockerImpl<Mutex>)];                      // char may_alias any type
  }             space;                                                  // BIRNET_MAY_ALIAS; ICE: GCC#30894
  volatile uint lcount;
  inline const Locker*          locker      () const             { return static_cast<const Locker*> ((const void*) &space); }
  BIRNET_PRIVATE_CLASS_COPY (AutoLocker);
protected:
  template<class Lockable> void initlock () { BIRNET_STATIC_ASSERT (sizeof (LockerImpl<Lockable>) <= sizeof (space)); relock(); }
  /* assert implicit assumption of the AutoLocker implementation */
  template<class Lockable> void
  assert_impl (Lockable &lockable)
  {
    BIRNET_ASSERT (sizeof (LockerImpl<Lockable>) <= sizeof (space));
    Locker *laddr = new (&space) LockerImpl<Lockable> (&lockable);
    BIRNET_ASSERT (laddr == locker());
  }
public:
  /*Des*/                      ~AutoLocker  ()                                { while (lcount) unlock(); }
  template<class Lockable>      AutoLocker  (Lockable *lockable) : lcount (0) { new (&space) LockerImpl<Lockable>  (lockable); initlock<Lockable>(); }
  template<class Lockable>      AutoLocker  (Lockable &lockable) : lcount (0) { new (&space) LockerImpl<Lockable> (&lockable); initlock<Lockable>(); }
  inline void                   relock      ()                                { locker()->lock(); lcount++; }
  inline void                   unlock      ()                                { BIRNET_ASSERT (lcount > 0); lcount--; locker()->unlock(); }
};
namespace Atomic {
template<typename T>
class RingBuffer {
  const uint    m_size;
  T            *m_buffer;
  volatile uint m_wmark, m_rmark;
  BIRNET_PRIVATE_CLASS_COPY (RingBuffer);
public:
  explicit
  RingBuffer (uint bsize) :
    m_size (bsize + 1), m_wmark (0), m_rmark (bsize)
  {
    m_buffer = new T[m_size];
    Atomic::uint_set (&m_wmark, 0);
    Atomic::uint_set (&m_rmark, 0);
  }
  ~RingBuffer()
  {
    Atomic::uint_set ((volatile uint*) &m_size, 0);
    Atomic::uint_set (&m_rmark, 0);
    Atomic::uint_set (&m_wmark, 0);
    delete[] m_buffer;
  }
  uint
  n_writable()
  {
    const uint rm = Atomic::uint_get (&m_rmark);
    const uint wm = Atomic::uint_get (&m_wmark);
    uint space = (m_size - 1 + rm - wm) % m_size;
    return space;
  }
  uint
  write (uint     length,
         const T *data,
         bool     partial = true)
  {
    const uint orig_length = length;
    const uint rm = Atomic::uint_get (&m_rmark);
    uint wm = Atomic::uint_get (&m_wmark);
    uint space = (m_size - 1 + rm - wm) % m_size;
    if (!partial && length > space)
      return 0;
    while (length)
      {
        if (rm <= wm)
          space = m_size - wm + (rm == 0 ? -1 : 0);
        else
          space = rm - wm -1;
        if (!space)
          break;
        space = MIN (space, length);
        std::copy (data, &data[space], &m_buffer[wm]);
        wm = (wm + space) % m_size;
        data += space;
        length -= space;
      }
    Atomic::write_barrier();
    /* the barrier ensures m_buffer writes are seen before the m_wmark update */
    Atomic::uint_set (&m_wmark, wm);
    return orig_length - length;
  }
  uint
  n_readable()
  {
    const uint wm = Atomic::uint_get (&m_wmark);
    const uint rm = Atomic::uint_get (&m_rmark);
    uint space = (m_size + wm - rm) % m_size;
    return space;
  }
  uint
  read (uint length,
        T   *data,
        bool partial = true)
  {
    const uint orig_length = length;
    /* need Atomic::read_barrier() here to ensure m_buffer writes are seen before m_wmark updates */
    const uint wm = Atomic::uint_get (&m_wmark); /* includes Atomic::read_barrier(); */
    uint rm = Atomic::uint_get (&m_rmark);
    uint space = (m_size + wm - rm) % m_size;
    if (!partial && length > space)
      return 0;
    while (length)
      {
        if (wm < rm)
          space = m_size - rm;
        else
          space = wm - rm;
        if (!space)
          break;
        space = MIN (space, length);
        std::copy (&m_buffer[rm], &m_buffer[rm + space], data);
        rm = (rm + space) % m_size;
        data += space;
        length -= space;
      }
    Atomic::uint_set (&m_rmark, rm);
    return orig_length - length;
  }
};
} // Atomic
/* --- implementation --- */
inline void
OwnedMutex::lock ()
{
  ThreadTable.rec_mutex_lock (&m_rec_mutex);
  Atomic::ptr_set (&m_owner, &Thread::self());
}
inline bool
OwnedMutex::trylock ()
{
  if (ThreadTable.rec_mutex_trylock (&m_rec_mutex) == 0)
    {
      Atomic::ptr_set (&m_owner, &Thread::self());
      return true; /* TRUE indicates success */
    }
  else
    return false;
}
inline void
OwnedMutex::unlock ()
{
  Atomic::ptr_set (&m_owner, (Thread*) 0);
  ThreadTable.rec_mutex_unlock (&m_rec_mutex);
}
inline Thread*
OwnedMutex::owner ()
{
  return Atomic::ptr_get (&m_owner);
}
inline bool
OwnedMutex::mine ()
{
  return Atomic::ptr_get (&m_owner) == &Thread::self();
}
} // Birnet
#endif /* __BIRNET_THREAD_XX_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
