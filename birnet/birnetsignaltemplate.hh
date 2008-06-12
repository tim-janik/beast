/* BirnetSignal
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */

/* this file is used to generate birnetsignalvariants.hh by mksignals.sh.
 * therein, certain phrases like "typename A1, typename A2, typename A3" are
 * rewritten into 0, 1, 2, ... 16 argument variants. so make sure all phrases
 * involving the signal argument count match those from mksignals.sh.
 */

/* --- Emission --- */
template<class Emitter, typename R0, typename A1, typename A2, typename A3>
struct Emission3 : public EmissionBase {
  typedef Trampoline3<R0, A1, A2, A3>           Trampoline;
  typedef Trampoline4<R0, Emitter&, A1, A2, A3> TrampolineE;
  Emitter *m_emitter;
  R0 m_result; A1 m_a1; A2 m_a2; A3 m_a3;
  TrampolineLink *m_last_link;
  Emission3 (Emitter *emitter, A1 a1, A2 a2, A3 a3) :
    m_emitter (emitter), m_result(), m_a1 (a1), m_a2 (a2), m_a3 (a3), m_last_link (NULL)
  {}
  /* call Trampoline and store result, so trampoline templates need no <void> specialization */
  R0 call (TrampolineLink *link)
  {
    if (m_last_link != link)
      {
        if (link->with_emitter)
          {
            TrampolineE *trampoline = trampoline_cast<TrampolineE*> (link);
            if (trampoline->callable)
              m_result = (*trampoline) (*m_emitter, m_a1, m_a2, m_a3);
          }
        else
          {
            Trampoline *trampoline = trampoline_cast<Trampoline*> (link);
            if (trampoline->callable)
              m_result = (*trampoline) (m_a1, m_a2, m_a3);
          }
        m_last_link = link;
      }
    return m_result;
  }
};
template<class Emitter, typename A1, typename A2, typename A3>
struct Emission3 <Emitter, void, A1, A2, A3> : public EmissionBase {
  typedef Trampoline3<void, A1, A2, A3>           Trampoline;
  typedef Trampoline4<void, Emitter&, A1, A2, A3> TrampolineE;
  Emitter *m_emitter;
  A1 m_a1; A2 m_a2; A3 m_a3;
  Emission3 (Emitter *emitter, A1 a1, A2 a2, A3 a3) :
    m_emitter (emitter), m_a1 (a1), m_a2 (a2), m_a3 (a3)
  {}
  /* call the trampoline and ignore result, so trampoline templates need no <void> specialization */
  void call (TrampolineLink *link)
  {
    if (link->with_emitter)
      {
        TrampolineE *trampoline = trampoline_cast<TrampolineE*> (link);
        if (trampoline->callable)
          (*trampoline) (*m_emitter, m_a1, m_a2, m_a3);
      }
    else
      {
        Trampoline *trampoline = trampoline_cast<Trampoline*> (link);
        if (trampoline->callable)
          (*trampoline) (m_a1, m_a2, m_a3);
      }
  }
};

/* --- SignalEmittable3 --- */
template<class Emitter, typename R0, typename A1, typename A2, typename A3, class Collector>
struct SignalEmittable3 : SignalBase {
  typedef Emission3 <Emitter, R0, A1, A2, A3> Emission;
  typedef typename Collector::result_type     Result;
  struct Iterator : public SignalBase::Iterator<Emission> {
    Iterator (Emission &emission, TrampolineLink *link) : SignalBase::Iterator<Emission> (emission, link) {}
    R0 operator* () { return this->emission.call (this->current); }
  };
  explicit SignalEmittable3 (Emitter *emitter) : m_emitter (emitter) {}
  inline Result emit (A1 a1, A2 a2, A3 a3)
  {
    ScopeReference<Emitter, Collector> lref (*m_emitter);
    Emission emission (m_emitter, a1, a2, a3);
    Iterator it (emission, &start), last (emission, &start);
    ++it; /* walk from start to first */
    Collector collector;
    Result result = collector (it, last);
    return result;
  }
private:
  Emitter *m_emitter;
};
/* SignalEmittable3 for void returns */
template<class Emitter, typename A1, typename A2, typename A3, class Collector>
struct SignalEmittable3 <Emitter, void, A1, A2, A3, Collector> : SignalBase {
  typedef Emission3 <Emitter, void, A1, A2, A3> Emission;
  struct Iterator : public SignalBase::Iterator<Emission> {
    Iterator (Emission &emission, TrampolineLink *link) : SignalBase::Iterator<Emission> (emission, link) {}
    void operator* () { return this->emission.call (this->current); }
  };
  explicit SignalEmittable3 (Emitter *emitter) : m_emitter (emitter) {}
  inline void emit (A1 a1, A2 a2, A3 a3)
  {
    ScopeReference<Emitter, Collector> lref (*m_emitter);
    Emission emission (m_emitter, a1, a2, a3);
    Iterator it (emission, &start), last (emission, &start);
    ++it; /* walk from start to first */
    Collector collector;
    collector (it, last);
  }
private:
  Emitter *m_emitter;
};

/* --- Signal3 --- */
/* Signal* */
template<class Emitter, typename R0, typename A1, typename A2, typename A3, class Collector = CollectorDefault<R0> >
struct Signal3 : SignalEmittable3<Emitter, R0, A1, A2, A3, Collector>
{
  typedef Emission3 <Emitter, R0, A1, A2, A3> Emission;
  typedef Slot3<R0, A1, A2, A3>               Slot;
  typedef Slot4<R0, Emitter&, A1, A2, A3>     SlotE;
  typedef SignalEmittable3<Emitter, R0, A1, A2, A3, Collector> SignalEmittable;
  explicit Signal3 (Emitter &emitter) :
    SignalEmittable (&emitter)
  { BIRNET_ASSERT (&emitter != NULL); }
  explicit Signal3 (Emitter &emitter, R0 (Emitter::*method) (A1, A2, A3)) :
    SignalEmittable (&emitter)
  {
    BIRNET_ASSERT (&emitter != NULL);
    connect (slot (emitter, method));
  }
  inline void connect    (const Slot  &s) { connect_link (s.get_trampoline_link()); }
  inline void connect    (const SlotE &s) { connect_link (s.get_trampoline_link(), true); }
  inline uint disconnect (const Slot  &s) { return disconnect_equal_link (*s.get_trampoline_link()); }
  inline uint disconnect (const SlotE &s) { return disconnect_equal_link (*s.get_trampoline_link(), true); }
  Signal3&    operator+= (const Slot  &s) { connect (s); return *this; }
  Signal3&    operator+= (const SlotE &s) { connect (s); return *this; }
  Signal3&    operator+= (R0 (*callback) (A1, A2, A3))            { connect (slot (callback)); return *this; }
  Signal3&    operator+= (R0 (*callback) (Emitter&, A1, A2, A3))  { connect (slot (callback)); return *this; }
  Signal3&    operator-= (const Slot  &s) { disconnect (s); return *this; }
  Signal3&    operator-= (const SlotE &s) { disconnect (s); return *this; }
  Signal3&    operator-= (R0 (*callback) (A1, A2, A3))            { disconnect (slot (callback)); return *this; }
  Signal3&    operator-= (R0 (*callback) (Emitter&, A1, A2, A3))  { disconnect (slot (callback)); return *this; }
  BIRNET_PRIVATE_CLASS_COPY (Signal3);
};

/* --- Signal<> --- */
template<class Emitter, typename R0, typename A1, typename A2, typename A3, class Collector>
struct Signal<Emitter, R0 (A1, A2, A3), Collector> : Signal3<Emitter, R0, A1, A2, A3, Collector>
{
  typedef Birnet::Signals::Signal3<Emitter, R0, A1, A2, A3, Collector> Signal3;
  explicit Signal (Emitter &emitter) :
    Signal3 (emitter)
    {}
  explicit Signal (Emitter &emitter, R0 (Emitter::*method) (A1, A2, A3)) :
    Signal3 (emitter, method)
    {}
  BIRNET_PRIVATE_CLASS_COPY (Signal);
};

