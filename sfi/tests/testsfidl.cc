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
#undef G_LOG_DOMAIN
#define  G_LOG_DOMAIN __FILE__
// #define TEST_VERBOSE
#include <birnet/birnettests.h>
#include "../sfidl-generator.hh"
#include "../sfidl-factory.hh"
#include <stdio.h>

using namespace Sfidl;
using namespace std;


#define ASSERT_EQ(got,expectedcstr) do {                                \
  TPRINT ("{check equality: %s == %s}", expectedcstr, got.c_str());     \
  TASSERT (expectedcstr == got);                                            \
} while (0)

class TestCG : public CodeGenerator
{
  string one, two, done;
public:
  TestCG(const Parser& parser) : CodeGenerator (parser)
  {
  }
  OptionVector getOptions()
  {
    OptionVector opts;

    opts.push_back (make_pair ("--one", true));
    opts.push_back (make_pair ("--two", true));
    opts.push_back (make_pair ("--done", false));

    return opts;
  }
  void setOption (const string& option, const string& value)
  {
    if (option == "--one") one = value;
    if (option == "--two") two = value;
    if (option == "--done") done = value;
  }
  bool run()
  {
    TSTART ("Testing Option parser");
    ASSERT_EQ (one, "1");
    ASSERT_EQ (two, "2");
    ASSERT_EQ (done, "1");
    TDONE ();

    TSTART ("Testing CodeGenerator::rename()");

    vector<string> procedures;
    vector<string> empty;
    vector<string> type;
    procedures.push_back("Procedures");
    type.push_back("Type");

    TASSERT (procedures.size() == 1);
    TASSERT (type.size() == 1);
    TASSERT (empty.size() == 0);

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

    ASSERT_EQ (rename (ABSOLUTE, "Bse::FFTSize", Capitalized, "",
                       empty, Capitalized, ""),
              "BseFFTSize");

    ASSERT_EQ (rename (ABSOLUTE, "Bse::FFTSize", lower, "_",
                       empty, lower, "_"),
              "bse_fft_size");

    ASSERT_EQ (rename (ABSOLUTE, "XWindows::WMHints", Capitalized, "",
                       empty, Capitalized, ""),
              "XWindowsWMHints");

    ASSERT_EQ (rename (ABSOLUTE, "XWindows::WMHints", UPPER, "_",
                       empty, UPPER, "_"),
              "XWINDOWS_WM_HINTS");

    ASSERT_EQ (rename (ABSOLUTE, "Bse::MIDI_SIGNAL_PROGRAM", UPPER, "_",
                       empty, UPPER, "_"),
              "BSE_MIDI_SIGNAL_PROGRAM");

    TDONE();
    return true;
  }
};

class TestCGFactory : public Factory {
public:
  string option() const	      { return "--test"; }
  string description() const  { return "test code generator"; }
  
  CodeGenerator *create (const Parser& parser) const
  {
    return new TestCG (parser);
  }
} static_factory;

int
main (int   argc,
      char *argv[])
{
  Options options;
  Parser parser;

  int fake_argc = 6;
  char **fake_argv = g_new0 (gchar*, fake_argc);
  fake_argv[0] = "testsfidl";
  fake_argv[1] = "--test";
  fake_argv[2] = "--one";
  fake_argv[3] = "1";
  fake_argv[4] = "--two=2";
  fake_argv[5] = "--done";
  options.parse (&fake_argc, &fake_argv, parser);

  TSTART ("Testing factory");
  TASSERT (options.codeGenerator != 0);
  TDONE();

  if (options.codeGenerator->run())
    {
      delete options.codeGenerator;
      return 0;
    }
  else
    {
      delete options.codeGenerator;
      return 1;
    }
}

/* vim:set ts=8 sts=2 sw=2: */
