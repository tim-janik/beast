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
#include <sfi.h>


#define	MSG(what)	do g_print ("%s [", what); while (0)
#define	TICK()		do g_print ("-"); while (0)
#define	GLITCH()	do g_print ("X"); while (0)
#define	DONE()		do g_print ("]\n"); while (0)
#define	ASSERT(code)	do { if (code) TICK (); else g_error ("failed to assert: %s", G_STRINGIFY (code)); } while (0)

static void
test_notes (void)
{
  gchar *str;
  guint i;
  MSG ("Notes:");
  str = sfi_note_to_string (SFI_MIN_NOTE);
  ASSERT (sfi_note_from_string (str) == SFI_MIN_NOTE);
  g_free (str);
  str = sfi_note_to_string (SFI_KAMMER_NOTE);
  ASSERT (sfi_note_from_string (str) == SFI_KAMMER_NOTE);
  g_free (str);
  str = sfi_note_to_string (SFI_MAX_NOTE);
  ASSERT (sfi_note_from_string (str) == SFI_MAX_NOTE);
  g_free (str);
  for (i = SFI_MIN_NOTE; i <= SFI_MAX_NOTE; i++)
    {
      gint octave;
      guint semitone;
      gboolean black_semitone;
      gchar letter;

      sfi_note_examine (i, &octave, &semitone, &black_semitone, &letter);
      ASSERT (octave == SFI_NOTE_OCTAVE (i));
      ASSERT (semitone == SFI_NOTE_SEMITONE (i));
      ASSERT (SFI_NOTE_GENERIC (octave, semitone) == i);
    }
  DONE ();
}

static void
test_time (void)
{
  SfiTime t;
  GError *error = NULL;
  gchar *str;

  MSG ("Time:");
  ASSERT (SFI_USEC_FACTOR == 1000000);
  ASSERT (SFI_MIN_TIME + 1000000 < SFI_MAX_TIME);
  t = sfi_time_system ();
  if (t < SFI_MIN_TIME || t > SFI_MAX_TIME)
    {
      GLITCH ();
      t = SFI_MIN_TIME / 2 + SFI_MAX_TIME / 2;
    }
  else
    TICK ();
  t /= SFI_USEC_FACTOR;
  t *= SFI_USEC_FACTOR;
  str = sfi_time_to_string (t);
  ASSERT (sfi_time_from_string (str, &error) == t);
  ASSERT (error == NULL);
  g_free (str);
  /* test error returns */
  ASSERT (sfi_time_from_string ("foo 22", &error) == 0);
  ASSERT (error != NULL);
  // g_print ("{%s}", error->message);
  g_clear_error (&error);
  DONE ();
}

static void
test_renames (void)
{
  gchar *str;
  MSG ("Renames:");
  str = g_type_name_to_cname ("PrefixTypeName");
  ASSERT (strcmp (str, "prefix_type_name") == 0);
  g_free (str);
  str = g_type_name_to_sname ("PrefixTypeName");
  ASSERT (strcmp (str, "prefix-type-name") == 0);
  g_free (str);
  str = g_type_name_to_cupper ("PrefixTypeName");
  ASSERT (strcmp (str, "PREFIX_TYPE_NAME") == 0);
  g_free (str);
  str = g_type_name_to_type_macro ("PrefixTypeName");
  ASSERT (strcmp (str, "PREFIX_TYPE_TYPE_NAME") == 0);
  g_free (str);
  str = g_type_name_to_sname ("prefix_type_name");
  ASSERT (strcmp (str, "prefix-type-name") == 0);
  g_free (str);
  str = g_type_name_to_cname ("prefix-type-name");
  ASSERT (strcmp (str, "prefix_type_name") == 0);
  g_free (str);
  DONE ();
}

int
main (int   argc,
      char *argv[])
{
  sfi_init ();

  test_notes ();
  test_time ();
  test_renames ();
  
  return 0;
}
