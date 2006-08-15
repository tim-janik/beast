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

/* this file is used to generate birnetsignalvariants.hh by mksignals.sh.
 * therein, certain phrases like "typename A1, typename A2, typename A3" are
 * rewritten into 0, 1, 2, ... 16 argument variants. so make sure all phrases
 * involving the signal argument count match those from mksignals.sh.
 */

/* --- Trampoline (basis signature) --- */
template<typename R0, typename A1, typename A2, typename A3>
struct Trampoline3 : public TrampolineLink {
  /* signature type for all signal trampolines, used for trampoline invocations by Emission */
  virtual R0 operator() (A1 a1, A2 a2, A3 a3) = 0;
};

/* --- Trampoline (plain) --- */
template<typename R0, typename A1, typename A2, typename A3>
class FunctionTrampoline3 : public Trampoline3 <R0, A1, A2, A3> {
  friend void FIXME_dummy_friend_for_gcc33();
  typedef R0 (*Callback) (A1, A2, A3);
  Callback callback;
  virtual R0 operator() (A1 a1, A2 a2, A3 a3)
  { return callback (a1, a2, a3); }
  virtual      ~FunctionTrampoline3() {}
  virtual bool operator== (const TrampolineLink &bother) const {
    const FunctionTrampoline3 *other = dynamic_cast<const FunctionTrampoline3*> (&bother);
    return other and other->callback == callback; }
public:
  FunctionTrampoline3 (Callback c) :
    callback (c)
  {}
};
template<class Class, typename R0, typename A1, typename A2, typename A3>
class MethodTrampoline3 : public Trampoline3 <R0, A1, A2, A3>, public virtual Deletable::DestructionHook {
  friend void FIXME_dummy_friend_for_gcc33();
  typedef R0 (Class::*Method) (A1, A2, A3);
  Class *instance;
  Method method;
  virtual R0 operator() (A1 a1, A2 a2, A3 a3)
  { return (instance->*method) (a1, a2, a3); }
  virtual bool operator== (const TrampolineLink &bother) const {
    const MethodTrampoline3 *other = dynamic_cast<const MethodTrampoline3*> (&bother);
    return other and other->instance == instance and other->method == method; }
  virtual      ~MethodTrampoline3()                     { deletable_remove_hook (instance); }
  virtual void deletable_dispose (Deletable &deletable) { instance = NULL; this->callable = false; }
public:
  MethodTrampoline3 (Class &obj, Method m) :
    instance (&obj), method (m)                         { deletable_add_hook (instance); }
};

/* --- Trampoline with Data --- */
template<typename R0, typename A1, typename A2, typename A3, typename Data>
class DataFunctionTrampoline3 : public Trampoline3 <R0, A1, A2, A3> {
  friend void FIXME_dummy_friend_for_gcc33();
  typedef R0 (*Callback) (A1, A2, A3, Data);
  Callback callback; Data data;
  virtual R0 operator() (A1 a1, A2 a2, A3 a3)
  { return callback (a1, a2, a3, data); }
  virtual      ~DataFunctionTrampoline3() {}
  virtual bool operator== (const TrampolineLink &bother) const {
    const DataFunctionTrampoline3 *other = dynamic_cast<const DataFunctionTrampoline3*> (&bother);
    return other and other->callback == callback and other->data == data; }
public:
  DataFunctionTrampoline3 (Callback c, const Data &d) :
    callback (c), data (d)
  {}
};
template<class Class, typename R0, typename A1, typename A2, typename A3, typename Data>
class DataMethodTrampoline3 : public Trampoline3 <R0, A1, A2, A3>, public virtual Deletable::DestructionHook {
  friend void FIXME_dummy_friend_for_gcc33();
  typedef R0 (Class::*Method) (A1, A2, A3, Data);
  Class *instance; Method method; Data data;
  virtual R0 operator() (A1 a1, A2 a2, A3 a3)
  { return (instance->*method) (a1, a2, a3, data); }
  virtual bool operator== (const TrampolineLink &bother) const {
    const DataMethodTrampoline3 *other = dynamic_cast<const DataMethodTrampoline3*> (&bother);
    return other and other->instance == instance and other->method == method and other->data == data; }
  virtual      ~DataMethodTrampoline3()                 { deletable_remove_hook (instance); }
  virtual void deletable_dispose (Deletable &deletable) { instance = NULL; this->callable = false; }
public:
  DataMethodTrampoline3 (Class &obj, Method m, const Data &d) :
    instance (&obj), method (m), data (d)               { deletable_add_hook (instance); }
};

/* --- Slots (Trampoline wrappers) --- */
template<typename R0, typename A1, typename A2, typename A3, class Emitter = void>
struct Slot3 : SlotBase {
  Slot3 (Trampoline3<R0, A1, A2, A3> *trampoline) : SlotBase (trampoline) {}
  Trampoline3<R0, A1, A2, A3>* get_trampoline() const
  { return trampoline_cast< Trampoline3<R0, A1, A2, A3>* > (get_trampoline_link()); }
};
/* slot constructors */
template<typename R0, typename A1, typename A2, typename A3> Slot3<R0, A1, A2, A3>
slot (R0 (*callback) (A1, A2, A3))
{
  return Slot3<R0, A1, A2, A3> (new FunctionTrampoline3<R0, A1, A2, A3> (callback));
}
template<typename R0, typename A1, typename A2, typename A3, typename Data> Slot3<R0, A1, A2, A3>
slot (R0 (*callback) (A1, A2, A3, Data), const Data &data)
{
  return Slot3<R0, A1, A2, A3> (new DataFunctionTrampoline3<R0, A1, A2, A3, Data> (callback, data));
}
template<class Class, typename R0, typename A1, typename A2, typename A3> Slot3<R0, A1, A2, A3>
slot (Class &obj, R0 (Class::*method) (A1, A2, A3))
{
  return Slot3<R0, A1, A2, A3> (new MethodTrampoline3<Class, R0, A1, A2, A3> (obj, method));
}
template<class Class, typename R0, typename A1, typename A2, typename A3, typename Data> Slot3<R0, A1, A2, A3>
slot (Class &obj, R0 (Class::*method) (A1, A2, A3, Data), const Data &data)
{
  return Slot3<R0, A1, A2, A3> (new DataMethodTrampoline3<Class, R0, A1, A2, A3, Data> (obj, method, data));
}
template<typename Obj, typename R0, typename A1, typename A2, typename A3> Slot3<R0, A1, A2, A3>
slot (Signal<Obj, R0 (A1, A2, A3)> &sigref)
{
  typedef Signal<Obj, R0 (A1, A2, A3)> CustomSignal;
  return Slot3<R0, A1, A2, A3> (new MethodTrampoline3<CustomSignal, R0, A1, A2, A3> (sigref, &CustomSignal::emit));
}
