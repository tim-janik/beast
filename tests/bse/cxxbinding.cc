/* BSE test program
 * Copyright (C) 2003 Stefan Westerfeld
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
#include "bsecxxapi.hh"
#include <bse/bse.h>
#include <unistd.h>

static SfiGlueContext *bse_context = NULL;

using namespace Bse;
using namespace std;

void do_sleep (int seconds)
{
  /*
   * sleeps at least the required time, even in the presence of signals
   * for instance due to (gdb)
   */
  while (seconds > 0)
    seconds = sleep (seconds);
}

int
main (int   argc,
      char *argv[])
{
  std::set_terminate (__gnu_cxx::__verbose_terminate_handler);
  bse_init_async (&argc, &argv, "CxxBindingTest", NULL);
  sfi_msg_allow ("misc");
  bse_context = bse_init_glue_context (argv[0]);

  sfi_glue_context_push (bse_context);

  g_print ("type_blurb(BseContainer)=%s\n", type_blurb("BseContainer").c_str());

  const gchar *file_name = "empty.ogg";
  SampleFileInfoHandle info = sample_file_info (file_name);
  if (info.c_ptr())
    {
      g_print ("sample_file_info(\"%s\"): file = %s, loader = %s\n",
               file_name, info->file.c_str(), info->loader.c_str());
      g_print ("  %d waves contained:\n", info->waves.length());
      for (unsigned int i = 0; i < info->waves.length(); i++)
        g_print ("  - %s\n", info->waves[i].c_str());
    }
  else
    {
      g_print ("sample_file_info(\"%s\"): failed\n", file_name);
    }

  g_print ("error_blurb(ERROR_DEVICE_ASYNC): %s\n", error_blurb (ERROR_DEVICE_ASYNC).c_str());

  Server server = 1;    // FIXME: users may not hardcode this

  g_print ("server.get_custom_instrument_dir()=%s\n", server.get_custom_instrument_dir().c_str());

  GConfigHandle prefs = GConfig::from_rec (server.bse_preferences ());
  prefs->plugin_path = "./.libs/testplugin.so";
  SfiRec *rec = GConfig::to_rec (prefs);
  server.set_bse_preferences (rec);
  sfi_rec_unref (rec);
  prefs = GConfig::from_rec (server.bse_preferences());

  printf ("server.bse_preferences().plugin_path=%s\n", prefs->plugin_path.c_str());
  printf ("register core plugins...\n");
  server.register_core_plugins();
  printf ("waiting... (FIXME: need to connect to server signals here)\n");
  do_sleep (2);
  g_print ("done.\n");

  /* ... test plugin ... */
  Project test_project = server.use_new_project ("test_project");
  CSynth synth = test_project.create_csynth ("synth");

  g_print ("--- creating TestObject ---\n");
  Namespace::TestObject to = synth.create_source("NamespaceTestObject")._proxy(); // FIXME: dynamic_cast me
  if (to)
    g_print ("success creating TestObject: %ld\n", to._proxy());
  else
    g_error ("failed.");

  /* --- test procedures --- */
  g_print ("--- calling procedure test_exception() ---\n");
  g_print ("invoking as: result = test_exception (21, %ld, 42, \"moderately-funky\");\n", to._proxy());
  SfiSeq *pseq = sfi_seq_new ();
  sfi_seq_append_int (pseq, 21);
  sfi_seq_append_proxy (pseq, to._proxy());
  sfi_seq_append_int (pseq, 42);
  sfi_seq_append_choice (pseq, "moderately-funky");
  GValue *rvalue = sfi_glue_call_seq ("namespace-test-exception", pseq);
  sfi_seq_unref (pseq);
  if (!rvalue || !SFI_VALUE_HOLDS_INT (rvalue))
    g_error ("failed (no result).");
  SfiInt result = sfi_value_get_int (rvalue);
  g_print ("result=%d\n", result);
  if (result != 21 + 42)
    g_error ("wrong result.");
  g_print ("invoking to trigger exception: result = test_exception ();\n");
  pseq = sfi_seq_new ();
  rvalue = sfi_glue_call_seq ("namespace-test-exception", pseq);
  sfi_seq_unref (pseq);
  if (!rvalue || !SFI_VALUE_HOLDS_INT (rvalue))
    g_error ("failed (no result).");
  result = sfi_value_get_int (rvalue);
  g_print ("result=%d\n", result);

  /* --- test playback --- */
  file_name = "../test/test-song.bse";
  g_print ("--- playing %s... ---\n", file_name);
  Project project = server.use_new_project ("foo");
  project.restore_from_file (file_name);
  project.play();
  sleep (3);
  g_print ("done.\n");

  sfi_glue_context_pop ();
}

/* vim:set ts=8 sts=2 sw=2: */
