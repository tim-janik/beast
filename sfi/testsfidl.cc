/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2004 Tim Janik, Stefan Westerfeld
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
#include "sfidl-generator.h"
#include <stdio.h>

using namespace Sfidl;
using namespace std;


#define MSG(what)       do g_print ("%s [", what); while (0)
#define TICK()          do g_print ("-"); while (0)
#define XTICK()         do g_print ("X"); while (0)
#define DONE()          do g_print ("]\n"); while (0)
#define ASSERT(code)    do { if (code) TICK (); else g_error ("(line:%u) failed to assert: %s", __LINE__, #code); } while (0)
#define ASSERT_EQ(got,expected) \
  do { if (expected == got) TICK (); else g_error ("(line:%u) failed to assert: %s == %s\nexpected: %s, got: %s", __LINE__, #got, #expected, expected, got.c_str()); } while (0)

class TestCG : public CodeGenerator
{
public:
  TestCG(const Parser& parser) : CodeGenerator (parser)
  {
  }
  void run()
  {
    MSG ("Testing CodeGenerator::rename():");

    vector<string> procedures;
    vector<string> empty;
    vector<string> type;
    procedures.push_back("Procedures");
    type.push_back("Type");

    ASSERT (procedures.size() == type.size() == 1);
    ASSERT (empty.size() == 0);

    ASSERT_EQ (rename (ABSOLUTE, "A::B::processMessagesSlowly", Capitalized, "::",
		       procedures, lower, "_"),
               "::A::B::Procedures::process_messages_slowly");

    ASSERT_EQ (rename (NONE, "A::B::processMessagesSlowly", Capitalized, "::",
		       procedures, lower, "_"),
	       "Procedures::process_messages_slowly");

    ASSERT_EQ (rename (ABSOLUTE, "Bse::SNet", Capitalized, "",
		       empty, Capitalized, ""),
	       "BseSNet");

    ASSERT_EQ (rename (ABSOLUTE, "Bse::AlphaSynth", UPPER, "_",
		       type, UPPER, "_"),
	       "BSE_TYPE_ALPHA_SYNTH");

    DONE();
  }
};

int
main (int   argc,
      char *argv[])
{
  Options options;
  Parser parser;
  TestCG testCG (parser);
  testCG.run();
  return 0;
}
