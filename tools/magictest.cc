// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include <bse/bsemain.hh>
#include "topconfig.h"
#include <bse/gslmagic.hh>
#include <bse/gslcommon.hh>
#include <bse/bseloader.hh>
#include <bse/gsldatahandle.hh>
#include <stdio.h>
#include <string.h>
static gint
help (gchar *arg)
{
  fprintf (stderr, "usage: magictest [-{h|p|}] [files...]\n");
  fprintf (stderr, "       -p         include plugins\n");
  fprintf (stderr, "       -t         test loading file info\n");
  fprintf (stderr, "       -h         guess what ;)\n");
  return arg != NULL;
}
int
main (gint   argc,
      gchar *argv[])
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
  bse_init_inprocess (&argc, argv, "BseMagicTest", Bse::cstrings_to_vector ("stand-alone=1", NULL));
  for (i = 0; i < n_magic_presets; i++)
    magic_list = sfi_ring_append (magic_list,
				  gsl_magic_create ((void*) magic_presets[i][0],
						    0,
						    0,
						    magic_presets[i][1]));
  for (i = 1; i < uint (argc); i++)
    {
      if (strcmp ("-p", argv[i]) == 0)
	; // FIXME: bsw_register_plugins (BSE_PATH_PLUGINS, FALSE, NULL, NULL, NULL);
      else if (strcmp ("-t", argv[i]) == 0)
	test_open = TRUE;
      else if (strcmp ("-h", argv[i]) == 0)
	{
	  return help (NULL);
	}
      else
	{
	  BseLoader *loader = bse_loader_match (argv[i]);
	  GslMagic *magic = gsl_magic_list_match_file (magic_list, argv[i]);
	  guint l = strlen (argv[i]);
	  gchar *pad;
	  g_print ("%s:", argv[i]);
	  pad = g_strnfill (MAX (40, l) - l, ' ');
	  g_print ("%s", pad);
	  g_free (pad);
	  if (!magic && !loader)
	    g_print (" no magic/loader found");
	  else
	    {
	      if (magic)
		g_print (" MAGIC: %s", (char*) magic->data);
	      if (loader)
                {
                  if (test_open)
                    {
                      BseWaveFileInfo *wfi;
                      BseErrorType error = BSE_ERROR_NONE;
                      g_print ("\n  LOADER: %s\n", loader->name);
                      wfi = bse_wave_file_info_load (argv[i], &error);
                      if (wfi)
                        bse_wave_file_info_unref (wfi);
                      g_print ("  ERROR: %s", bse_error_blurb (error));
                    }
                  else
                    g_print (" LOADER: %s", loader->name);
                }
	    }
	  g_print ("\n");
	}
    }
  return 0;
}
