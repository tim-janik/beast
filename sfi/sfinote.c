/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
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
#include <stdlib.h>
#include <string.h>
#include "sfinote.h"

#define to_lower(c)             ( \
        (guchar) (                                                      \
          ( (((guchar)(c))>='A' && ((guchar)(c))<='Z') * ('a'-'A') ) |  \
          ((guchar)(c))                                                 \
        )                                                               \
)
#define is_note_letter(c)       (strchr ("cdefgabh", to_lower (c)) != NULL) /* + german alias */


/* --- variables --- */
static const struct {
  gchar *name;
  gint note;
} sfi_note_table[] = {
  { "ces",	SFI_KAMMER_NOTE - 10 - SFI_KAMMER_OCTAVE * 12 },
  { "cis",	SFI_KAMMER_NOTE -  8 - SFI_KAMMER_OCTAVE * 12 },
  { "c",	SFI_KAMMER_NOTE -  9 - SFI_KAMMER_OCTAVE * 12 },
  { "des",	SFI_KAMMER_NOTE -  8 - SFI_KAMMER_OCTAVE * 12 },
  { "dis",	SFI_KAMMER_NOTE -  6 - SFI_KAMMER_OCTAVE * 12 },
  { "d",	SFI_KAMMER_NOTE -  7 - SFI_KAMMER_OCTAVE * 12 },
  { "es",	SFI_KAMMER_NOTE -  6 - SFI_KAMMER_OCTAVE * 12 },
  { "eis",	SFI_KAMMER_NOTE -  4 - SFI_KAMMER_OCTAVE * 12 },
  { "e",	SFI_KAMMER_NOTE -  5 - SFI_KAMMER_OCTAVE * 12 },
  { "fes",	SFI_KAMMER_NOTE -  5 - SFI_KAMMER_OCTAVE * 12 },
  { "fis",	SFI_KAMMER_NOTE -  3 - SFI_KAMMER_OCTAVE * 12 },
  { "f",	SFI_KAMMER_NOTE -  4 - SFI_KAMMER_OCTAVE * 12 },
  { "ges",	SFI_KAMMER_NOTE -  3 - SFI_KAMMER_OCTAVE * 12 },
  { "gis",	SFI_KAMMER_NOTE -  1 - SFI_KAMMER_OCTAVE * 12 },
  { "g",	SFI_KAMMER_NOTE -  2 - SFI_KAMMER_OCTAVE * 12 },
  { "as",	SFI_KAMMER_NOTE -  1 - SFI_KAMMER_OCTAVE * 12 },
  { "ais",	SFI_KAMMER_NOTE +  1 - SFI_KAMMER_OCTAVE * 12 },
  { "a",	SFI_KAMMER_NOTE	     - SFI_KAMMER_OCTAVE * 12 },
  { "bes",	SFI_KAMMER_NOTE +  1 - SFI_KAMMER_OCTAVE * 12 },
  { "bis",	SFI_KAMMER_NOTE +  3 - SFI_KAMMER_OCTAVE * 12 },
  { "b",	SFI_KAMMER_NOTE +  2 - SFI_KAMMER_OCTAVE * 12 },
  { "h",	SFI_KAMMER_NOTE +  2 - SFI_KAMMER_OCTAVE * 12 }, /* german alias */
};
static const gchar *sfi_note_name_table[12] = {
  "C", "Cis", "D", "Dis", "E", "F",
  "Fis", "G", "Gis", "A", "Ais", "B",
};


/* --- functions --- */
SfiInt
sfi_note_from_string (const gchar *note_string)
{
  return sfi_note_from_string_err (note_string, NULL);
}

SfiInt
sfi_note_from_string_err (const gchar *note_string,
			  gchar      **error_p)
{
  gchar *string, *freeme;
  gint i, fits, note, sharp = 0;

  if (error_p)
    *error_p = NULL;
  g_return_val_if_fail (note_string != NULL, SFI_NOTE_VOID);
  
  string = freeme = g_strdup_stripped (note_string);
  g_ascii_strdown (string, -1);
  
  note = SFI_NOTE_VOID;
  if (strcmp (string, "void") == 0)	/* *valid* SFI_NOTE_VOID path */
    {
      g_free (freeme);
      return note;
    }
  
  if (string[0] == '#' && is_note_letter (string[1]))   /* #C-0 */
    {
      sharp++;
      string++;
    }
  if (is_note_letter (string[0]) && string[1] == '#')   /* C#-0 */
    {
      sharp++;
      string[1] = string[0];
      string++;
    }
  
  fits = FALSE;
  for (i = 0; i < G_N_ELEMENTS (sfi_note_table); i++)
    {
      guint p = 0;
      do
	fits = to_lower (sfi_note_table[i].name[p]) == to_lower (string[p]);
      while (sfi_note_table[i].name[++p] && fits);
      if (fits)
	break;
    }
  
  note = SFI_KAMMER_NOTE;		/* *invalid* note value */
  if (fits)
    {
      gchar *s;
      gint o;
      
      if (*(string + strlen (sfi_note_table[i].name)))
	{
	  o = strtol (string + strlen (sfi_note_table[i].name), &s, 10);
	  if (s && *s)
	    fits = FALSE;
	}
      else
	o = 0;
      
      if (fits)
	note = SFI_NOTE_CLAMP (sfi_note_table[i].note + sharp + o * 12);
    }
  
  g_free (freeme);

  if (!fits && error_p)
    *error_p = g_strdup_printf ("invalid note specification: %s", note_string);

  return note;
}

gchar*
sfi_note_to_string (SfiInt note)
{
  if (SFI_NOTE_IS_VALID (note))
    {
      guint ht = 0;
      gint o = 0;
      
      sfi_note_examine (note, &o, &ht, NULL, NULL);
      if (o)
	return g_strdup_printf ("%s%+d", sfi_note_name_table[ht], o);
      else
	return g_strdup (sfi_note_name_table[ht]);
    }
  else
    return g_strdup ("void");
}

void
sfi_note_examine (gint      note,
		  gint     *octave_p,
		  guint    *semitone_p,
		  gboolean *black_semitone_p,
		  gchar	   *letter_p)
{
  static const gint8 semitone_flags[12] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };
  guint semitone;
  gint octave;
  
  g_return_if_fail (SFI_NOTE_IS_VALID (note));
  
  semitone = note % 12 + (9 - (SFI_KAMMER_NOTE % 12));
  note -= semitone;
  octave = note - (SFI_KAMMER_NOTE - 9);
  octave = octave / 12 + SFI_KAMMER_OCTAVE;
  
  if (octave_p)
    *octave_p = octave;
  if (semitone_p)
    *semitone_p = semitone;
  if (black_semitone_p)
    *black_semitone_p = semitone_flags[semitone];
  if (letter_p)
    *letter_p = sfi_note_name_table[semitone][0];
}
