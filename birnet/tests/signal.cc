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
// #define TEST_VERBOSE
#include <birnet/birnettests.h>

namespace {
using namespace Birnet;
using Birnet::uint;

struct ExtraType {
  virtual const char* message() { return "ExtraType::message()"; }
  bool operator== (const ExtraType &other) const { return false; }
};

struct EmitterMany {
  virtual ~EmitterMany() {}
  virtual const char* emitter_check() { return __func__; }
  Signal<EmitterMany, void ()> sig_void_0;
  Signal<EmitterMany, void (int   )> sig_void_1;
  Signal<EmitterMany, void (int   , double)> sig_void_2;
  Signal<EmitterMany, void (int   , double, int   )> sig_void_3;
  Signal<EmitterMany, void (int   , double, int   , double)> sig_void_4;
  Signal<EmitterMany, void (int   , double, int   , double, int   )> sig_void_5;
  Signal<EmitterMany, void (double, int   , double, int   , double, int   )> sig_void_6;
  Signal<EmitterMany, void (int   , double, int   , double, int   , double, int   )> sig_void_7;
  Signal<EmitterMany, void (double, int   , double, int   , double, int   , double, int   )> sig_void_8;
  Signal<EmitterMany, void (int   , double, int   , double, int   , double, int   , double, int   )> sig_void_9;
  Signal<EmitterMany, void (double, int   , double, int   , double, int   , double, int   , double, int   )> sig_void_10;
  Signal<EmitterMany, void (int   , float , int   , float , int   , float , int   , float , int   , float , int   )> sig_void_11;
  Signal<EmitterMany, void (int   , char  , int   , char  , int   , char  , int   , char  , int   , char  , int   , char  )> sig_void_12;
  Signal<EmitterMany, void (short , int   , short , int   , short , int   , short , int   , short , int   , short , int   , short )> sig_void_13;
  Signal<EmitterMany, void (int   , double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double)> sig_void_14;
  Signal<EmitterMany, void (double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double)> sig_void_15;
  Signal<EmitterMany, void (int   , double, short , uint  , float , char  , uint8 , long  , int64 , char  , uint  , float , char  , double, uint64, short )> sig_void_16;
  Signal<EmitterMany, String ()> sig_string_0;
  Signal<EmitterMany, String (int   )> sig_string_1;
  Signal<EmitterMany, String (int   , double)> sig_string_2;
  Signal<EmitterMany, String (int   , double, int   )> sig_string_3;
  Signal<EmitterMany, String (int   , double, int   , double)> sig_string_4;
  Signal<EmitterMany, String (int   , double, int   , double, int   )> sig_string_5;
  Signal<EmitterMany, String (double, int   , double, int   , double, int   )> sig_string_6;
  Signal<EmitterMany, String (int   , double, int   , double, int   , double, int   )> sig_string_7;
  Signal<EmitterMany, String (double, int   , double, int   , double, int   , double, int   )> sig_string_8;
  Signal<EmitterMany, String (int   , double, int   , double, int   , double, int   , double, int   )> sig_string_9;
  Signal<EmitterMany, String (double, int   , double, int   , double, int   , double, int   , double, int   )> sig_string_10;
  Signal<EmitterMany, String (int   , float , int   , float , int   , float , int   , float , int   , float , int   )> sig_string_11;
  Signal<EmitterMany, String (int   , char  , int   , char  , int   , char  , int   , char  , int   , char  , int   , char  )> sig_string_12;
  Signal<EmitterMany, String (short , int   , short , int   , short , int   , short , int   , short , int   , short , int   , short )> sig_string_13;
  Signal<EmitterMany, String (int   , double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double)> sig_string_14;
  Signal<EmitterMany, String (double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double, int   , double)> sig_string_15;
  Signal<EmitterMany, String (int   , double, short , uint  , float , char  , uint8 , long  , int64 , char  , uint  , float , char  , double, uint64, short )> sig_string_16;
  EmitterMany() :
    sig_void_0 (*this), sig_void_1 (*this), sig_void_2 (*this), sig_void_3 (*this),
    sig_void_4 (*this), sig_void_5 (*this), sig_void_6 (*this), sig_void_7 (*this),
    sig_void_8 (*this), sig_void_9 (*this), sig_void_10 (*this), sig_void_11 (*this),
    sig_void_12 (*this), sig_void_13 (*this), sig_void_14 (*this), sig_void_15 (*this), sig_void_16 (*this),
    sig_string_0 (*this), sig_string_1 (*this), sig_string_2 (*this), sig_string_3 (*this),
    sig_string_4 (*this), sig_string_5 (*this), sig_string_6 (*this), sig_string_7 (*this),
    sig_string_8 (*this), sig_string_9 (*this), sig_string_10 (*this), sig_string_11 (*this),
    sig_string_12 (*this), sig_string_13 (*this), sig_string_14 (*this), sig_string_15 (*this), sig_string_16 (*this)
  {}
  static String test_string_14_emitter_data (EmitterMany &em,int a1, double a2, int a3, double a4, int a5, double a6, int a7, double a8, int a9,
                                             double a10, int a11, double a12, int a13, double a14, ExtraType x)
  {
    TPRINT ("  callback: %s (%s, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %s);\n",
            __func__, em.emitter_check(), a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, x.message());
    return __func__;
  }
  static String test_string_14_data (int a1, double a2, int a3, double a4, int a5, double a6, int a7, double a8, int a9,
                                     double a10, int a11, double a12, int a13, double a14, ExtraType x)
  {
    TPRINT ("  callback: %s (%d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %d, %.1f, %s);\n",
            __func__, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, x.message());
    return __func__;
  }
  void testme ()
  {
    TSTART ("Signals, multi-arg");
    sig_void_0.emit ();
    sig_void_1.emit (1);
    sig_void_2.emit (1, 2);
    sig_void_3.emit (1, 2, 3);
    sig_void_4.emit (1, 2, 3, 4);
    sig_void_5.emit (1, 2, 3, 4, 5);
    sig_void_6.emit (1, 2, 3, 4, 5, 6);
    sig_void_7.emit (1, 2, 3, 4, 5, 6, 7);
    sig_void_8.emit (1, 2, 3, 4, 5, 6, 7, 8);
    sig_void_9.emit (1, 2, 3, 4, 5, 6, 7, 8, 9);
    sig_void_10.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    sig_void_11.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
    sig_void_12.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
    sig_void_13.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
    sig_void_14.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
    sig_void_15.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    sig_void_16.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    ExtraType xt;
    sig_string_14 += slot (test_string_14_emitter_data, xt);
    sig_string_14 += slot (test_string_14_data, xt);
    String results;
    results += sig_string_0.emit ();
    results += sig_string_1.emit (1);
    results += sig_string_2.emit (1, 2);
    results += sig_string_3.emit (1, 2, 3);
    results += sig_string_4.emit (1, 2, 3, 4);
    results += sig_string_5.emit (1, 2, 3, 4, 5);
    results += sig_string_6.emit (1, 2, 3, 4, 5, 6);
    results += sig_string_7.emit (1, 2, 3, 4, 5, 6, 7);
    results += sig_string_8.emit (1, 2, 3, 4, 5, 6, 7, 8);
    results += sig_string_9.emit (1, 2, 3, 4, 5, 6, 7, 8, 9);
    results += sig_string_10.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    results += sig_string_11.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
    results += sig_string_12.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
    results += sig_string_13.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
    results += sig_string_14.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
    results += sig_string_15.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    results += sig_string_16.emit (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    TPRINT ("32 signals emitted: %s\n", results.c_str());
    TDONE();
  }
  void ref()   {}
  void unref() {}
};

struct Emitter3 {
  // Signal3<Emitter3, String, int, String, float, Signal::CollectorSum<String> > sig_mixed;
  Signal<Emitter3, String (int, String, float), Signals::CollectorSum<String> > sig_mixed;
  // Signal3<Emitter3, void,   int, String, float> sig_void_mixed;
  Signal<Emitter3, void (int, String, float)> sig_void_mixed;
  Emitter3() : sig_mixed (*this), sig_void_mixed (*this) {}
  void
  test_emissions()
  {
    TPRINT ("Emitter3.emit()\n");
    String s = sig_mixed.emit (7, "seven.seven", 7.7);
    TPRINT ("Emitter3: result=%s\n", s.c_str());
    TPRINT ("Emitter3.emit() (void)\n");
    sig_void_mixed.emit (3, "three.three", 3.3);
    TPRINT ("Emitter3: done.\n");
  }
  void ref() {}
  void unref() {}
};

struct Connection3 {
  static String mixed_func (int i, String s, float f)
  {
    TASSERT (i == 7 && s == "seven.seven" && f == (float) 7.7);
    TPRINT ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  static String mixed_efunc      (Emitter3 &obj, int i, String s, float f)
  {
    TASSERT (i == 7 && s == "seven.seven" && f == (float) 7.7);
    TPRINT ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  static void   void_mixed_func (int i, String s, float f)
  {
    TASSERT (i == 3 && s == "three.three" && f == (float) 3.3);
    TPRINT ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  static void   void_mixed_efunc (Emitter3 &obj, int i, String s, float f)
  {
    TASSERT (i == 3 && s == "three.three" && f == (float) 3.3);
    TPRINT ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  String string_callback (int i, String s, float f)
  {
    TPRINT ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  String string_emitter_callback (Emitter3 &emitter, int i, String s, float f)
  {
    TPRINT ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
    return __func__;
  }
  void void_callback (int i, String s, float f)
  {
    TPRINT ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  void void_emitter_callback (Emitter3 &emitter, int i, String s, float f)
  {
    TPRINT ("  callback: %s (%d, %s, %f);\n", __func__, i, s.c_str(), f);
  }
  void test_signal (Emitter3 &e3)
  {
    TSTART ("Signals, mixed emissions");
    e3.sig_mixed += mixed_efunc;
    e3.sig_mixed += mixed_func;
    e3.sig_mixed += slot (*this, &Connection3::string_emitter_callback);
    e3.sig_mixed += slot (*this, &Connection3::string_callback);
    e3.sig_void_mixed += void_mixed_efunc;
    e3.sig_void_mixed += void_mixed_func;
    e3.sig_void_mixed += slot (*this, &Connection3::void_emitter_callback);
    e3.sig_void_mixed += slot (*this, &Connection3::void_callback);
    e3.test_emissions();
    TDONE();
  }
};

extern "C" int
main (int   argc,
      char *argv[])
{
  birnet_init_test (&argc, &argv);
#if 0
  SignalTest signal_test;
  signal_test.basic_signal_tests();
  signal_test.member_pointer_tests();
#endif
  Connection3 c3;
  Emitter3 e3;
  c3.test_signal (e3);
  EmitterMany many;
  many.testme();
  return 0;
}

} // anon
