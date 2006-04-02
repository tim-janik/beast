/* BirnetSignal
 * Copyright (C) 2005 Tim Janik
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
#ifndef __BIRNET_SIGNAL_HH__
#define __BIRNET_SIGNAL_HH__

#include <birnet/birnetutilsxx.hh>

namespace Birnet {
namespace Signals {

/* --- EmissionBase --- */
struct EmissionBase {
  bool restart_emission;
  bool stop_emission;
  EmissionBase() :
    restart_emission (true),
    stop_emission (false)
  {}
};

/* --- SignalBase --- */
struct SignalBase {
  struct Link : public ReferenceCountImpl {
    Link *next, *prev;
    uint  callable : 1;
    uint  with_emitter : 1;
    explicit
    Link() :
      next (NULL), prev (NULL), callable (true), with_emitter (false)
    {}
    virtual bool operator== (const SignalBase::Link &other) const = 0;
    virtual
    ~Link()
    {
      if (next || prev)
        {
          next->prev = prev;
          prev->next = next;
          prev = next = NULL;
        }
    }
  };
private:
  class EmbeddedLink : public Link {
    virtual bool operator== (const SignalBase::Link &other) const { return false; }
    virtual void delete_this () { /* embedded */ }
  };
protected:
  EmbeddedLink      start;
  void
  connect_link (Link *link,
                bool  with_emitter = false)
  {
    BIRNET_ASSERT (link->prev == NULL && link->next == NULL);
    link->ref_sink();
    link->prev = start.prev;
    link->next = &start;
    start.prev->next = link;
    start.prev = link;
    link->with_emitter = with_emitter;
  }
  uint
  disconnect_equal_link (const Link &link,
                         bool        with_emitter = false)
  {
    for (Link *walk = start.next; walk != &start; walk = walk->next)
      if (walk->with_emitter == with_emitter and *walk == link)
        {
          walk->callable = false;
          /* unlink */
          walk->next->prev = walk->prev;
          walk->prev->next = walk->next;
          /* leave ->next, ->prev intact for iterators */
          walk->unref();
          return 1;
        }
    return 0;
  }
  template<class Emission> struct Iterator;
public:
  explicit SignalBase ()
  {
    start.next = &start;
    start.prev = &start;
    start.ref_sink();
  }
  ~SignalBase()
  {
    while (start.next != &start)
      destruct_link (start.next);
    BIRNET_ASSERT (start.next == &start);
    BIRNET_ASSERT (start.prev == &start);
    start.prev = start.next = NULL;
    start.unref();
  }
private:
  void
  destruct_link (Link *link)
  {
    link->callable = false;
    link->next->prev = link->prev;
    link->prev->next = link->next;
    link->prev = link->next = NULL;
    link->unref();
  }
  BIRNET_PRIVATE_CLASS_COPY (SignalBase);
};

/* --- Signal Iterator --- */
template<class Emission>
class SignalBase::Iterator {
  Iterator& operator= (const Iterator&);
public:
  Emission &emission;
  Link     *current, *start;
  Iterator (Emission &_emission, Link *last) :
    emission (_emission),
    current (ref (last)),
    start (ref (last))
  {}
  Iterator (const Iterator &dup) :
    emission (dup.emission),
    current (ref (dup.current)),
    start (ref (dup.start))
  {}
  ~Iterator()
  {
    current->unref();
    current = NULL;
    start->unref();
    start = NULL;
  }
  Iterator& operator++ ()
  {
    if (emission.restart_emission)
      {
        current->unref();
        current = start->next;
        current->ref();
        emission.restart_emission = false;
      }
    else
      do
        {
          Link *next = current->next;
          next->ref();
          current->unref();
          current = next;
        }
      while (!current->callable && current != start);
    if (emission.stop_emission)
      {
        current->unref();
        current = start;
        current->ref();
      }
    return *this;
  }
  void operator++ (int)
  {
    /* not supporting iterator copies for post-increment */
    operator++();
  }
  bool operator!= (const Iterator &b) const
  {
    return current != b.current || &emission != &b.emission;
  }
  bool operator== (const Iterator &b) const
  {
    return !operator!= (b);
  }
};

/* --- Collector --- */
template<typename Result>
struct CollectorLast {
  typedef Result result_type;
  template<typename InputIterator>
  Result operator() (InputIterator begin, InputIterator end)
  {
    Result result = Result();
    while (begin != end)
      {
        result = *begin;
        ++begin;
      }
    return result;
  }
};
template<typename Result> struct CollectorDefault : CollectorLast<Result> {};
template<>
struct CollectorDefault<void> {
  typedef void result_type;
  template<typename InputIterator>
  void operator() (InputIterator begin, InputIterator end)
  {
    while (begin != end)
      {
        *begin;
        ++begin;
      }
  }
};
template<typename Result>
struct CollectorSum {
  typedef Result result_type;
  template<typename InputIterator>
  Result operator() (InputIterator begin, InputIterator end)
  {
    Result result = Result();
    while (begin != end)
      {
        result += *begin;
        ++begin;
      }
    return result;
  }
};
template<typename Result>
struct CollectorWhile0 {
  typedef Result result_type;
  template<typename InputIterator>
  Result operator() (InputIterator begin, InputIterator end)
  {
    Result result = Result();   // result = 0
    while (begin != end)
      {
        result = *begin;
        if (result)             // result != 0
          break;
        ++begin;
      }
    return result;
  }
};
template<typename Result>
struct CollectorUntil0 {
  typedef Result result_type;
  template<typename InputIterator>
  Result operator() (InputIterator begin, InputIterator end)
  {
    Result result = !Result();  // result = !0
    while (begin != end)
      {
        result = *begin;
        if (!result)            // result == 0
          break;
        ++begin;
      }
    return result;
  }
};

/* --- ScopeReference --- */
template<class Instance, typename Mark>
class ScopeReference {
  Instance &m_instance;
public:
  ScopeReference  (Instance &instance) : m_instance (instance) { m_instance.ref(); }
  ~ScopeReference ()                                           { m_instance.unref(); }
};
struct ScopeReferenceFinalizationMark : CollectorDefault<void> {};
template<class Instance>
class ScopeReference<Instance, ScopeReferenceFinalizationMark> {
  Instance &m_instance;
public:
  ScopeReference  (Instance &instance) : m_instance (instance) { BIRNET_ASSERT (m_instance.finalizing() == true); }
  ~ScopeReference ()                                           { BIRNET_ASSERT (m_instance.finalizing() == true); }
};

/* --- SlotBase --- */
class SlotBase {
protected:
  typedef SignalBase::Link Link;
  Link *m_link;
  void
  set_trampoline (Link *cl)
  {
    if (cl)
      cl->ref_sink();
    if (m_link)
      m_link->unref();
    m_link = cl;
  }
public:
  SlotBase (Link *link) :
    m_link (NULL)
  { set_trampoline (link); }
  SlotBase (const SlotBase &src) :
    m_link (NULL)
  { *this = src; }
  SlotBase&
  operator= (const SlotBase &src)
  {
    set_trampoline (src.m_link);
    return *this;
  }
  Link*
  get_trampoline() const
  { return m_link; }
};

/* --- Signature --- */
template<typename>                      struct Signature;
template<typename R0>                   struct Signature<R0 ()>   { typedef R0 result_type; };
template<typename R0, typename A1>      struct Signature<R0 (A1)> { typedef A1 argument1_type;  typedef R0 result_type; };
template<typename R0, typename A1, typename A2>
struct Signature<R0 (A1, A2)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3>
struct Signature<R0 (A1, A2, A3)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4>
struct Signature<R0 (A1, A2, A3, A4)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5>
struct Signature<R0 (A1, A2, A3, A4, A5)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct Signature<R0 (A1, A2, A3, A4, A5, A6)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11, typename A12>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef A12 argument12_type;
  typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11, typename A12, typename A13>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef A12 argument12_type;
  typedef A13 argument13_type;  typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11, typename A12, typename A13, typename A14>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef A12 argument12_type;
  typedef A13 argument13_type;  typedef A14 argument14_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef A12 argument12_type;
  typedef A13 argument13_type;  typedef A14 argument14_type;    typedef A15 argument15_type;    typedef R0 result_type;
};
template<typename R0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8,
         typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15, typename A16>
struct Signature<R0 (A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16)> {
  typedef A1 argument1_type;    typedef A2 argument2_type;      typedef A3 argument3_type;      typedef A4 argument4_type;
  typedef A5 argument5_type;    typedef A6 argument6_type;      typedef A7 argument7_type;      typedef A8 argument8_type;
  typedef A9 argument9_type;    typedef A10 argument10_type;    typedef A11 argument11_type;    typedef A12 argument12_type;
  typedef A13 argument13_type;  typedef A14 argument14_type;    typedef A15 argument15_type;    typedef A16 argument16_type;
  typedef R0 result_type;
};

template<class Emitter, typename SignalSignature, class Collector = CollectorDefault<typename Signature<SignalSignature>::result_type> > struct Signal;

/* --- Casting --- */
template<class TrampolineP> TrampolineP
trampoline_cast (SignalBase::Link *link)
{
#ifdef  PARANOID
  return dynamic_cast<TrampolineP> (link);
#else
  return reinterpret_cast<TrampolineP> (link);
#endif
}

/* --- Trampoline + Slot + Signal generation --- */
#include <birnet/birnetsignalvariants.hh> // contains multiple versions of "birnetsignaltemplate.hh"

/* --- predefined signals --- */
template<class Emitter>
struct SignalFinalize : Signal0 <Emitter, void, ScopeReferenceFinalizationMark> {
  typedef Signal0<Emitter, void, ScopeReferenceFinalizationMark> Signal0;
  explicit SignalFinalize (Emitter &emitter)                             : Signal0 (emitter) {}
  explicit SignalFinalize (Emitter &emitter, void (Emitter::*method) ()) : Signal0 (emitter, method) {}
  BIRNET_PRIVATE_CLASS_COPY (SignalFinalize);
};

template<class Emitter>
struct SignalVoid : Signal0 <Emitter, void> {
  typedef Signal0<Emitter, void> Signal0;
  explicit SignalVoid (Emitter &emitter)                                 : Signal0 (emitter) {}
  explicit SignalVoid (Emitter &emitter, void (Emitter::*method) (void)) : Signal0 (emitter, method) {}
  BIRNET_PRIVATE_CLASS_COPY (SignalVoid);
};

} // Signals

/* --- Birnet::Signal imports --- */
using Signals::CollectorDefault;
using Signals::CollectorWhile0;
using Signals::CollectorUntil0;
using Signals::CollectorLast;
using Signals::CollectorSum;
using Signals::SignalFinalize;
using Signals::SignalVoid;
using Signals::Signal;
using Signals::slot;

} // Birnet

#endif  /* __BIRNET_SIGNAL_HH__ */
