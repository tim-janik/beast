/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2004 Tim Janik
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
#include "sficxx.h"
#include <stdio.h>

using namespace Sfi;


#define MSG(what)       do g_print ("%s [", what); while (0)
#define TICK()          do g_print ("-"); while (0)
#define XTICK()         do g_print ("X"); while (0)
#define DONE()          do g_print ("]\n"); while (0)
#define ASSERT(code)    do { if (code) TICK (); else g_error ("(line:%u) failed to assert: %s", __LINE__, #code); } while (0)

struct Bar {
  int i;
};


int
main (int   argc,
      char *argv[])
{
  MSG ("Test String:");
  ASSERT (sizeof (String) == sizeof (const char*));
  String s1 = "huhu";
  String s2;
  s2 += "huhu";
  ASSERT (strcmp (s1.c_str(), "huhu") == 0);
  ASSERT (s1 == s2);
  ASSERT (s1 == std::string ("huhu"));
  s2 += "1";
  ASSERT (s1 != s2);
  String s3 = "huhu1";
  ASSERT (s2 == s3);
  DONE();

  MSG ("Test RecordHandle<>:");
  ASSERT (sizeof (RecordHandle<Bar>) == sizeof (void*));
  RecordHandle<Bar> b1;
  ASSERT (b1.c_ptr() == NULL);
  RecordHandle<Bar> b2 (INIT_DEFAULT);
  ASSERT (b2->i == 0);
  RecordHandle<Bar> b3 (INIT_EMPTY);
  Bar b;
  b.i = 5;
  RecordHandle<Bar> b4 = b;
  ASSERT (b4->i == 5);
  DONE();

  return 0;
}
