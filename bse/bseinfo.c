/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2001, 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bseutils.h"
#include "bsemain.h"

#define	PREC_SHIFT	16
#define	FLF	"26.20"

static void
print_note (const gchar *note_name,
	    gint         note)
{
  gchar *string;
  
  string = bse_note_to_string (note);
  g_print ("%s =%-4d \tfactor=%"FLF"f [%-5s] (freq=%"FLF"f)\n",
	   note_name, note,
	   BSE_SEMITONE_FACTOR (note),
	   string, bse_note_to_freq (note));
  g_free (string);
}

static void
print_fine_tune (const gchar *tune_name,
		 gint         tune)
{
  g_print ("%s =%-4d \tfactor=%"FLF"f\n",
	   tune_name, tune,
	   BSE_FINE_TUNE_FACTOR (tune));
}

int
main (gint   argc,
      gchar *argv[])
{
  gint j, k;
  
  g_thread_init (NULL);
  bse_init_intern (&argc, &argv, NULL);
  
  g_print ("Rate relevant limits:\n");
  print_note      ("BSE_MIN_NOTE     ", BSE_MIN_NOTE);
  print_note      ("BSE_KAMMER_NOTE  ", BSE_KAMMER_NOTE);
  print_note      ("BSE_MAX_NOTE     ", BSE_MAX_NOTE);
  print_note      ("BSE_KAMMER_NOTE-1", BSE_KAMMER_NOTE - 1);
  print_fine_tune ("BSE_MIN_FINE_TUNE", BSE_MIN_FINE_TUNE);
  print_fine_tune ("bse-mid-fine-tune", (BSE_MIN_FINE_TUNE + BSE_MAX_FINE_TUNE) / 2);
  print_fine_tune ("BSE_MAX_FINE_TUNE", BSE_MAX_FINE_TUNE);
  print_note      ("BSE_KAMMER_NOTE+1", BSE_KAMMER_NOTE + 1);
  
  if (0)
    for (j = BSE_MIN_NOTE; j <= BSE_MAX_NOTE; j += 3)
      print_note (":", j);
  if (0)
    for (j = BSE_MIN_FINE_TUNE; j <= BSE_MAX_FINE_TUNE; j += 10)
      print_fine_tune (":", j);
  
  if (0)
    for (j = BSE_MIN_NOTE; j <= BSE_MAX_NOTE; j += 3)
      for (k = BSE_MIN_FINE_TUNE / 2; k <= BSE_MAX_FINE_TUNE / 2; k += 10)
	{
	  gdouble f, freq = bse_note_to_tuned_freq (j, k);
	  gint note, fine_tune;
	  g_print ("compose  : note=%4d fine_tune=%4d freq=%"FLF"f\n", j, k, freq);
	  f = freq;
	  note = bse_note_from_freq (freq);
	  fine_tune = bse_note_fine_tune_from_note_freq (note, freq);
	  freq = bse_note_to_tuned_freq (note, fine_tune);
	  g_print ("decompose: note=%4d fine_tune=%4d freq=%"FLF"f   (diff=%g)\n", note, fine_tune, freq, freq - f);
	}
  if (0)
    for (j = BSE_MIN_NOTE; j <= BSE_MAX_NOTE; j += 1)
      {
	gint octave = SFI_NOTE_OCTAVE (j);
	gint semitone = SFI_NOTE_SEMITONE (j);
	gint note = BSE_NOTE_GENERIC (octave, semitone);
	gchar *name = bse_note_to_string (j);
	
	g_print ("note[%3d]: name=%-8s octave=%3d semitone=%3d note=%3d match=%u\n",
		 j, name, octave, semitone, note, j == note);
	g_free (name);
      }
  
  if (argc == 2)
    {
      SfiRing *ring;
      g_print ("search path: \"%s\"\n", argv[1]);
      // plist = bse_search_path_list_files (argv[1], NULL);
      ring = sfi_file_crawler_list_files (argv[1], NULL, 0);
      while (ring)
        {
          gchar *name = sfi_ring_pop_head (&ring);
          g_print ("%s\n", name);
        }
    }
  
  return 0;
}
