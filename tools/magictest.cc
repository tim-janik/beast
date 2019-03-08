// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsetool.hh"
#include <bse/bsemain.hh>
#include <bse/gslmagic.hh>
#include <bse/gslcommon.hh>
#include <bse/bseloader.hh>
#include <bse/gsldatahandle.hh>
#include <stdio.h>
#include <string.h>

using namespace Bse;
using namespace BseTool;

static String
magic_test (const ArgParser &ap)
{
  static const char *magic_presets[][2] = {
    /* some test entries, order is important for some cases */
    { "Berkeley DB 2.X Hash/Little Endian",	"12 lelong 0x061561", },
    { "MS-DOS executable (EXE)",		"0 string MZ", },
    { "ELF object file",			"0 string \177ELF", },
    { "Bourne shell script text",		"0,string,#!/bin/sh", },
    { "Bourne shell script text",		"0,string,#!\\ /bin/sh", },
    { "Bourne shell script text",		"0,string,#!\\t/bin/sh", },
    { "GIF image data",				"0,string,GIF8", },
    { "X window image dump (v7)",		("# .xwd files\n"
						 "0x04,ubelong,0x0000007"), },
    { "RIFF/WAVE audio",			("0 string RIFF\n"
						 "8 string WAVE"), },
    { "RIFF data",				"0 string RIFF", },
    { "GslWave file",				"0 string #GslWave\n", },
  };
  static const guint n_magic_presets = sizeof (magic_presets) / sizeof (magic_presets[0]);
  guint i;
  SfiRing *magic_list = NULL;
  gboolean test_open = FALSE;
  /* initialization */
  for (i = 0; i < n_magic_presets; i++)
    magic_list = sfi_ring_append (magic_list,
				  gsl_magic_create ((void*) magic_presets[i][0],
						    0,
						    0,
						    magic_presets[i][1]));
  if (ap["-t"] == "1")
    test_open = TRUE;

  for (const auto &file_name : ap.dynamics())
    {
      BseLoader *loader = bse_loader_match (file_name.c_str());
      GslMagic *magic = gsl_magic_list_match_file (magic_list, file_name.c_str());
      const uint l = file_name.size();
      char *pad;

      printout ("%s:", file_name);
      pad = g_strnfill (MAX (40, l) - l, ' ');
      printout ("%s", pad);
      g_free (pad);
      if (!magic && !loader)
        printout (" no magic/loader found");
      else
        {
          if (magic)
            printout (" MAGIC: %s", (char*) magic->data);
          if (loader)
            {
              if (test_open)
                {
                  BseWaveFileInfo *wfi;
                  Bse::Error error = Bse::Error::NONE;
                  printout ("\n  LOADER: %s\n", loader->name);
                  wfi = bse_wave_file_info_load (file_name.c_str(), &error);
                  if (wfi)
                    bse_wave_file_info_unref (wfi);
                  printout ("  ERROR: %s", bse_error_blurb (error));
                }
              else
                printout (" LOADER: %s", loader->name);
            }
        }
      printout ("\n");
    }
  return ""; // no error
}

static ArgDescription magic_options[] = {
  { "-t",        "",    "Test loading file info", "" },
  { "[file...]", "",    "Files to match against magics", "" },
};

static CommandRegistry magic_cmd (magic_options, magic_test, "magic", "Identify files via magic definitions");
