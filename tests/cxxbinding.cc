/*
 * Copyright (C) 2003 Stefan Westerfeld
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
#include "bsecxxapi.h"
#include <bse/bse.h>

static SfiGlueContext *bse_context = NULL;

using namespace Bse;
using namespace std;

int main(int argc, char **argv)
{
  g_thread_init (NULL);
  sfi_init ();
  bse_init_async (&argc, &argv, NULL);
  bse_context = bse_init_glue_context (argv[0]);

  sfi_glue_context_push (bse_context);

  printf ("BseContainer: %s\n", type_blurb("BseContainer").c_str());

  SampleFileInfoPtr info = sample_file_info ("empty.ogg");
  if (info)
    {
      printf("file = %s, loader = %s\n", info->file.c_str(), info->loader.c_str());
      printf("%d waves contained:\n", info->waves.size());
      for (unsigned int i = 0; i < info->waves.length(); i++)
	{
	  printf("  - %s\n", info->waves[i].c_str());
	}
    }
  else
    {
      printf("no samplefileinfo for x.mp3\n");
    }

  printf ("error_blurb: %s\n", error_blurb (ERROR_DEVICE_ASYNC).c_str());

  Server server = 1;
  /*
   * FIXME: the plugin path should probably
   *  (a) be relative to $(top_builddir)
   *  (b) include the directories containing plugins as well
   */
  GConfigPtr prefs = GConfig::_from_rec (server.bse_preferences ());
  prefs->plugin_path = "./.libs:" + prefs->plugin_path;
  SfiRec *rec = GConfig::_to_rec (prefs);
  server.set_bse_preferences (rec);
  sfi_rec_unref (rec);
  prefs = GConfig::_from_rec (server.bse_preferences());

  printf ("plugin path = %s\n", prefs->plugin_path.c_str());
  printf ("register core plugins ... "); fflush (stdout);
  server.register_core_plugins();
  sleep(2);
  printf ("register ladspa plugins ... "); fflush (stdout);
  server.register_ladspa_plugins();
  sleep(2);
  printf ("register scripts ... "); fflush (stdout);
  server.register_scripts();
  sleep(2);
  printf("done\n");

  /* ... test plugin ... */
  Project test_project = server.use_new_project("test_project");
  CSynth synth = test_project.create_csynth("synth");

  // FIXME: dynamic_cast me
  Test::Plugin plugin = synth.create_source("TestPlugin")._proxy();
  if (plugin)
    printf("success creating plugin.\n");

  /* ... */
  printf("playing test-song.bse ... "); fflush(stdout);
  Project project = server.use_new_project("foo");
  project.restore_from_file("../test/test-song.bse");
  project.play();
  sleep(5);
  printf("done\n");

  printf ("instrument dir: %s\n", server.get_custom_instrument_dir().c_str());
  sfi_glue_context_pop ();
}

/* vim:set ts=8 sts=2 sw=2: */
