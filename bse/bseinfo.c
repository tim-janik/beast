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
#include "bse/bse.h"

#define	PREC_SHIFT	16

static void
print_note (const gchar *note_name,
	    gint         note)
{
  gchar *string;

  string = bse_note_to_string (note);
  g_print ("%s =%-4d \tfactor=%-10f (%-10u>>%u) [%-5s] (freq=%-15f)\n",
	   note_name, note,
	   BSE_HALFTONE_FACTOR (note),
	   BSE_HALFTONE_FACTOR_FIXED (note), PREC_SHIFT,
	   string, bse_note_to_freq (note));
  g_free (string);
}

static void
print_fine_tune (const gchar *tune_name,
		 gint         tune)
{
  g_print ("%s =%-4d \tfactor=%-10f\n",
	   tune_name, tune,
	   BSE_FINE_TUNE_FACTOR (tune));
}

static void
print_freq (const gchar *freq_name,
	    gdouble      freq)
{
  g_print ("%s =%-15.3f\n", freq_name, freq);
}

static void
print_rate (const gchar *freq_name,
	    gdouble      freq,
	    const gchar *blurb)
{
  g_print ("%s =%-15.10f \t(%-10Lu>>%u)%s%s\n",
	   freq_name, freq, (guint64) (freq * (1<<PREC_SHIFT)), PREC_SHIFT,
	   blurb ? "  // " : "",
	   blurb ? blurb : "");
}


int
main (gint   argc,
      gchar *argv[])
{
  g_thread_init (NULL);
  bse_init (&argc, &argv, NULL);

  g_print ("Rate relevant limits:\n");
  print_note      ("BSE_MIN_NOTE     ", BSE_MIN_NOTE);
  print_note      ("BSE_KAMMER_NOTE  ", BSE_KAMMER_NOTE);
  print_note      ("BSE_MAX_NOTE     ", BSE_MAX_NOTE);
  print_fine_tune ("BSE_MIN_FINE_TUNE", BSE_MIN_FINE_TUNE);
  print_fine_tune ("BSE_MAX_FINE_TUNE", BSE_MAX_FINE_TUNE);
  print_freq      ("BSE_MIN_MIX_FREQ ",  BSE_MIN_MIX_FREQ);
  print_freq      ("BSE_MAX_MIX_FREQ ",  BSE_MAX_MIX_FREQ);
  print_rate      ("min_freq_factor  ",  BSE_MIN_MIX_FREQ_d / BSE_MAX_MIX_FREQ_d, "min_mix_freq/max_mix_freq");
  print_rate      ("max_freq_factor  ",  BSE_MAX_MIX_FREQ_d / BSE_MIN_MIX_FREQ_d, "max_mix_freq/min_mix_freq");
  print_rate      ("min rate factor  ",
		   ((BSE_MIN_MIX_FREQ_d / BSE_MAX_MIX_FREQ_d) *
		    BSE_FINE_TUNE_FACTOR (BSE_MIN_FINE_TUNE) *
		    BSE_HALFTONE_FACTOR (BSE_MIN_NOTE)),
		   "min_freq_factor*min_fine_tune*min_note");
  print_rate      ("max rate factor  ",
		   ((BSE_MAX_MIX_FREQ_d / BSE_MIN_MIX_FREQ_d) *
		    BSE_FINE_TUNE_FACTOR (BSE_MAX_FINE_TUNE) *
		    BSE_HALFTONE_FACTOR (BSE_MAX_NOTE)),
		   "max_freq_factor*max_fine_tune*max_note");
  
  if (argc == 2)
    {
      GSList *plist, *slist;

      g_print ("search path: \"%s\"\n", argv[1]);
      // plist = bse_search_path_list_files (argv[1], NULL);
      plist = bse_path_pattern_list_matches (argv[1], NULL, 0);
      for (slist = plist; slist; slist = slist->next)
	g_print ("%s\n", (char*) slist->data);
      bse_str_slist_free (plist);
    }

  return 0;
}
