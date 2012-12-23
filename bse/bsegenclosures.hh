/* Void0Closure */
template<class T>
class Void0Closure : public CxxClosure {
  typedef void (T::*MCb) (); /* Member Callback */
  T      *o;
  MCb     f;
public:
  Void0Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = "|";
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    (o->*f) ();
  }
};
template<class T>
Void0Closure<T>* Closure (T *t, void (T::*f) ()) {
  return new Void0Closure<T> (t, f);
}
/* Void1Closure */
template<class T, typename A1>
class Void1Closure : public CxxClosure {
  typedef void (T::*MCb) (A1); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<A1> a1;
public:
  Void1Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = "|" + a1.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    (o->*f) (a1.get (param_values + 0));
  }
};
template<class T, typename A1>
Void1Closure<T, A1>* Closure (T *t, void (T::*f) (A1)) {
  return new Void1Closure<T, A1> (t, f);
}
/* Void2Closure */
template<class T, typename A1, typename A2>
class Void2Closure : public CxxClosure {
  typedef void (T::*MCb) (A1, A2); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<A1> a1; Arg<A2> a2;
public:
  Void2Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = "|" + a1.token() + a2.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1));
  }
};
template<class T, typename A1, typename A2>
Void2Closure<T, A1, A2>* Closure (T *t, void (T::*f) (A1, A2)) {
  return new Void2Closure<T, A1, A2> (t, f);
}
/* Void3Closure */
template<class T, typename A1, typename A2, typename A3>
class Void3Closure : public CxxClosure {
  typedef void (T::*MCb) (A1, A2, A3); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3;
public:
  Void3Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = "|" + a1.token() + a2.token() + a3.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2));
  }
};
template<class T, typename A1, typename A2, typename A3>
Void3Closure<T, A1, A2, A3>* Closure (T *t, void (T::*f) (A1, A2, A3)) {
  return new Void3Closure<T, A1, A2, A3> (t, f);
}
/* Void4Closure */
template<class T, typename A1, typename A2, typename A3, typename A4>
class Void4Closure : public CxxClosure {
  typedef void (T::*MCb) (A1, A2, A3, A4); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3; Arg<A4> a4;
public:
  Void4Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = "|" + a1.token() + a2.token() + a3.token() + a4.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2), a4.get (param_values + 3));
  }
};
template<class T, typename A1, typename A2, typename A3, typename A4>
Void4Closure<T, A1, A2, A3, A4>* Closure (T *t, void (T::*f) (A1, A2, A3, A4)) {
  return new Void4Closure<T, A1, A2, A3, A4> (t, f);
}
/* Void5Closure */
template<class T, typename A1, typename A2, typename A3, typename A4, typename A5>
class Void5Closure : public CxxClosure {
  typedef void (T::*MCb) (A1, A2, A3, A4, A5); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3; Arg<A4> a4; Arg<A5> a5;
public:
  Void5Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = "|" + a1.token() + a2.token() + a3.token() + a4.token() + a5.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2), a4.get (param_values + 3), a5.get (param_values + 4));
  }
};
template<class T, typename A1, typename A2, typename A3, typename A4, typename A5>
Void5Closure<T, A1, A2, A3, A4, A5>* Closure (T *t, void (T::*f) (A1, A2, A3, A4, A5)) {
  return new Void5Closure<T, A1, A2, A3, A4, A5> (t, f);
}
/* Void6Closure */
template<class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
class Void6Closure : public CxxClosure {
  typedef void (T::*MCb) (A1, A2, A3, A4, A5, A6); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3; Arg<A4> a4; Arg<A5> a5; Arg<A6> a6;
public:
  Void6Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = "|" + a1.token() + a2.token() + a3.token() + a4.token() + a5.token() + a6.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2), a4.get (param_values + 3), a5.get (param_values + 4), a6.get (param_values + 5));
  }
};
template<class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
Void6Closure<T, A1, A2, A3, A4, A5, A6>* Closure (T *t, void (T::*f) (A1, A2, A3, A4, A5, A6)) {
  return new Void6Closure<T, A1, A2, A3, A4, A5, A6> (t, f);
}
/* Void7Closure */
template<class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
class Void7Closure : public CxxClosure {
  typedef void (T::*MCb) (A1, A2, A3, A4, A5, A6, A7); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3; Arg<A4> a4; Arg<A5> a5; Arg<A6> a6; Arg<A7> a7;
public:
  Void7Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = "|" + a1.token() + a2.token() + a3.token() + a4.token() + a5.token() + a6.token() + a7.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2), a4.get (param_values + 3), a5.get (param_values + 4), a6.get (param_values + 5), a7.get (param_values + 6));
  }
};
template<class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
Void7Closure<T, A1, A2, A3, A4, A5, A6, A7>* Closure (T *t, void (T::*f) (A1, A2, A3, A4, A5, A6, A7)) {
  return new Void7Closure<T, A1, A2, A3, A4, A5, A6, A7> (t, f);
}
/* Void8Closure */
template<class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
class Void8Closure : public CxxClosure {
  typedef void (T::*MCb) (A1, A2, A3, A4, A5, A6, A7, A8); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3; Arg<A4> a4; Arg<A5> a5; Arg<A6> a6; Arg<A7> a7; Arg<A8> a8;
public:
  Void8Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = "|" + a1.token() + a2.token() + a3.token() + a4.token() + a5.token() + a6.token() + a7.token() + a8.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2), a4.get (param_values + 3), a5.get (param_values + 4), a6.get (param_values + 5), a7.get (param_values + 6), a8.get (param_values + 7));
  }
};
template<class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
Void8Closure<T, A1, A2, A3, A4, A5, A6, A7, A8>* Closure (T *t, void (T::*f) (A1, A2, A3, A4, A5, A6, A7, A8)) {
  return new Void8Closure<T, A1, A2, A3, A4, A5, A6, A7, A8> (t, f);
}
/* Ret0Closure */
template<typename R, class T>
class Ret0Closure : public CxxClosure {
  typedef R (T::*MCb) (); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<R>  r;
public:
  Ret0Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = r.token() + "|";
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    r.set (return_value, (o->*f) ());
  }
};
template<typename R, class T>
Ret0Closure<R, T>* Closure (T *t, R (T::*f) ()) {
  return new Ret0Closure<R, T> (t, f);
}
/* Ret1Closure */
template<typename R, class T, typename A1>
class Ret1Closure : public CxxClosure {
  typedef R (T::*MCb) (A1); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<R>  r;
  Arg<A1> a1;
public:
  Ret1Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = r.token() + "|" + a1.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    r.set (return_value, (o->*f) (a1.get (param_values + 0)));
  }
};
template<typename R, class T, typename A1>
Ret1Closure<R, T, A1>* Closure (T *t, R (T::*f) (A1)) {
  return new Ret1Closure<R, T, A1> (t, f);
}
/* Ret2Closure */
template<typename R, class T, typename A1, typename A2>
class Ret2Closure : public CxxClosure {
  typedef R (T::*MCb) (A1, A2); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<R>  r;
  Arg<A1> a1; Arg<A2> a2;
public:
  Ret2Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = r.token() + "|" + a1.token() + a2.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    r.set (return_value, (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1)));
  }
};
template<typename R, class T, typename A1, typename A2>
Ret2Closure<R, T, A1, A2>* Closure (T *t, R (T::*f) (A1, A2)) {
  return new Ret2Closure<R, T, A1, A2> (t, f);
}
/* Ret3Closure */
template<typename R, class T, typename A1, typename A2, typename A3>
class Ret3Closure : public CxxClosure {
  typedef R (T::*MCb) (A1, A2, A3); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<R>  r;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3;
public:
  Ret3Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = r.token() + "|" + a1.token() + a2.token() + a3.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    r.set (return_value, (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2)));
  }
};
template<typename R, class T, typename A1, typename A2, typename A3>
Ret3Closure<R, T, A1, A2, A3>* Closure (T *t, R (T::*f) (A1, A2, A3)) {
  return new Ret3Closure<R, T, A1, A2, A3> (t, f);
}
/* Ret4Closure */
template<typename R, class T, typename A1, typename A2, typename A3, typename A4>
class Ret4Closure : public CxxClosure {
  typedef R (T::*MCb) (A1, A2, A3, A4); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<R>  r;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3; Arg<A4> a4;
public:
  Ret4Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = r.token() + "|" + a1.token() + a2.token() + a3.token() + a4.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    r.set (return_value, (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2), a4.get (param_values + 3)));
  }
};
template<typename R, class T, typename A1, typename A2, typename A3, typename A4>
Ret4Closure<R, T, A1, A2, A3, A4>* Closure (T *t, R (T::*f) (A1, A2, A3, A4)) {
  return new Ret4Closure<R, T, A1, A2, A3, A4> (t, f);
}
/* Ret5Closure */
template<typename R, class T, typename A1, typename A2, typename A3, typename A4, typename A5>
class Ret5Closure : public CxxClosure {
  typedef R (T::*MCb) (A1, A2, A3, A4, A5); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<R>  r;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3; Arg<A4> a4; Arg<A5> a5;
public:
  Ret5Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = r.token() + "|" + a1.token() + a2.token() + a3.token() + a4.token() + a5.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    r.set (return_value, (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2), a4.get (param_values + 3), a5.get (param_values + 4)));
  }
};
template<typename R, class T, typename A1, typename A2, typename A3, typename A4, typename A5>
Ret5Closure<R, T, A1, A2, A3, A4, A5>* Closure (T *t, R (T::*f) (A1, A2, A3, A4, A5)) {
  return new Ret5Closure<R, T, A1, A2, A3, A4, A5> (t, f);
}
/* Ret6Closure */
template<typename R, class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
class Ret6Closure : public CxxClosure {
  typedef R (T::*MCb) (A1, A2, A3, A4, A5, A6); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<R>  r;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3; Arg<A4> a4; Arg<A5> a5; Arg<A6> a6;
public:
  Ret6Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = r.token() + "|" + a1.token() + a2.token() + a3.token() + a4.token() + a5.token() + a6.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    r.set (return_value, (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2), a4.get (param_values + 3), a5.get (param_values + 4), a6.get (param_values + 5)));
  }
};
template<typename R, class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
Ret6Closure<R, T, A1, A2, A3, A4, A5, A6>* Closure (T *t, R (T::*f) (A1, A2, A3, A4, A5, A6)) {
  return new Ret6Closure<R, T, A1, A2, A3, A4, A5, A6> (t, f);
}
/* Ret7Closure */
template<typename R, class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
class Ret7Closure : public CxxClosure {
  typedef R (T::*MCb) (A1, A2, A3, A4, A5, A6, A7); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<R>  r;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3; Arg<A4> a4; Arg<A5> a5; Arg<A6> a6; Arg<A7> a7;
public:
  Ret7Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = r.token() + "|" + a1.token() + a2.token() + a3.token() + a4.token() + a5.token() + a6.token() + a7.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    r.set (return_value, (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2), a4.get (param_values + 3), a5.get (param_values + 4), a6.get (param_values + 5), a7.get (param_values + 6)));
  }
};
template<typename R, class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
Ret7Closure<R, T, A1, A2, A3, A4, A5, A6, A7>* Closure (T *t, R (T::*f) (A1, A2, A3, A4, A5, A6, A7)) {
  return new Ret7Closure<R, T, A1, A2, A3, A4, A5, A6, A7> (t, f);
}
/* Ret8Closure */
template<typename R, class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
class Ret8Closure : public CxxClosure {
  typedef R (T::*MCb) (A1, A2, A3, A4, A5, A6, A7, A8); /* Member Callback */
  T      *o;
  MCb     f;
  Arg<R>  r;
  Arg<A1> a1; Arg<A2> a2; Arg<A3> a3; Arg<A4> a4; Arg<A5> a5; Arg<A6> a6; Arg<A7> a7; Arg<A8> a8;
public:
  Ret8Closure (T *t, MCb _f) : o(t), f(_f) {
    sig_tokens = r.token() + "|" + a1.token() + a2.token() + a3.token() + a4.token() + a5.token() + a6.token() + a7.token() + a8.token();
  }
  void operator() (Value       *return_value,
                   const Value *param_values,
                   gpointer     invocation_hint,
                   gpointer     marshal_data) {
    r.set (return_value, (o->*f) (a1.get (param_values + 0), a2.get (param_values + 1), a3.get (param_values + 2), a4.get (param_values + 3), a5.get (param_values + 4), a6.get (param_values + 5), a7.get (param_values + 6), a8.get (param_values + 7)));
  }
};
template<typename R, class T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
Ret8Closure<R, T, A1, A2, A3, A4, A5, A6, A7, A8>* Closure (T *t, R (T::*f) (A1, A2, A3, A4, A5, A6, A7, A8)) {
  return new Ret8Closure<R, T, A1, A2, A3, A4, A5, A6, A7, A8> (t, f);
}
