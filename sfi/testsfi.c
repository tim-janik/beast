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
test_misc (void)
{
  MSG ("Misc:");
  ASSERT (0 == 0);
  DONE ();
}

static GScannerConfig test_scanner_config = {
  (
   " \t\r\n"
   )                    /* cset_skip_characters */,
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   )                    /* cset_identifier_first */,
  (
   G_CSET_a_2_z
   ".:-+_0123456789*!?"
   G_CSET_A_2_Z
   )                    /* cset_identifier_nth */,
  ( ";\n" )             /* cpair_comment_single */,
  
  FALSE                 /* case_sensitive */,
  
  TRUE                  /* skip_comment_multi */,
  TRUE                  /* skip_comment_single */,
  FALSE                 /* scan_comment_multi */,
  TRUE                  /* scan_identifier */,
  FALSE                 /* scan_identifier_1char */,
  FALSE                 /* scan_identifier_NULL */,
  FALSE                 /* scan_symbols */,
  TRUE                  /* scan_binary */,
  TRUE                  /* scan_octal */,
  TRUE                  /* scan_float */,
  TRUE                  /* scan_hex */,
  FALSE                 /* scan_hex_dollar */,
  FALSE                 /* scan_string_sq */,
  TRUE                  /* scan_string_dq */,
  TRUE                  /* numbers_2_int */,
  FALSE                 /* int_2_float */,
  FALSE                 /* identifier_2_string */,
  TRUE                  /* char_2_token */,
  TRUE                  /* symbol_2_token */,
  FALSE                 /* scope_0_fallback */,
  TRUE                  /* store_int64 */,
};

#define SCANNER_ASSERT(scanner, printout, token, text, svalue) { \
  g_scanner_input_text (scanner, text, strlen (text)); \
  ASSERT (g_scanner_get_next_token (scanner) == token); \
  if (printout) g_print ("{scanner64value:%llu}", scanner->value.v_int64); \
  ASSERT (scanner->value.v_int64 == svalue); \
  ASSERT (g_scanner_get_next_token (scanner) == '#'); \
}

static void
test_scanner64 (void)
{
  GScanner *scanner = g_scanner_new (&test_scanner_config);
  MSG ("64Bit Scanner:");
  scanner->config->numbers_2_int = FALSE;
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_BINARY, " 0b0 #", 0);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_BINARY, " 0b10000000000000000 #", 65536);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_BINARY, " 0b11111111111111111111111111111111 #", 4294967295U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_BINARY, " 0b1111111111111111111111111111111111111111111111111111111111111111 #", 18446744073709551615U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_OCTAL, " 0 #", 0);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_OCTAL, " 0200000 #", 65536);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_OCTAL, " 037777777777 #", 4294967295U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_OCTAL, " 01777777777777777777777 #", 18446744073709551615U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_HEX, " 0x0 #", 0);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_HEX, " 0x10000 #", 65536);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_HEX, " 0xffffffff #", 4294967295U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_HEX, " 0xffffffffffffffff #", 18446744073709551615U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_INT, " 65536 #", 65536);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_INT, " 4294967295 #", 4294967295U);
  SCANNER_ASSERT (scanner, FALSE, G_TOKEN_INT, " 18446744073709551615 #", 18446744073709551615U);
  g_scanner_destroy (scanner);
  DONE ();
}

static gboolean serialize_cmp_serialize_typed = FALSE;

static void
serialize_cmp (GValue     *value,
	       GParamSpec *pspec)
{
  GScanner *scanner = g_scanner_new (&test_scanner_config);
  GString *gstring = g_string_new (NULL);
  GValue rvalue = { 0, };
  GTokenType token;
  gint cmp;
  if (serialize_cmp_serialize_typed)
    sfi_value_store_typed (value, gstring);
  else
    sfi_value_store_param (value, gstring, pspec, 2);
  g_scanner_input_text (scanner, gstring->str, gstring->len);
  if (serialize_cmp_serialize_typed)
    token = sfi_value_parse_typed (&rvalue, scanner);
  else
    {
      if (g_scanner_get_next_token (scanner) == '(')
	if (g_scanner_get_next_token (scanner) == G_TOKEN_IDENTIFIER &&
	    strcmp (scanner->value.v_identifier, pspec->name) == 0)
	  token = sfi_value_parse_param_rest (&rvalue, scanner, pspec);
	else
	  token = G_TOKEN_IDENTIFIER;
      else
	token = '(';
    }
  if (0)
    g_print ("{parsing:%s}", gstring->str);
  if (token != G_TOKEN_NONE)
    {
      g_print ("{while parsing:\n\t%s\n", gstring->str);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
			     g_strdup_printf ("failed to serialize \"%s\"", pspec->name), TRUE);
    }
  ASSERT (token == G_TOKEN_NONE);
  cmp = g_param_values_cmp (pspec, value, &rvalue);
  if (cmp)
    {
      g_print ("{after parsing:\n\t%s\n", gstring->str);
      g_print ("while comparing:\n\t%s\nwith:\n\t%s\nresult:\n\t%d\n",
	       g_strdup_value_contents (value),
	       g_strdup_value_contents (&rvalue),
	       cmp);
      if (0)
	{
	  G_BREAKPOINT ();
	  g_value_unset (&rvalue);
	  g_scanner_input_text (scanner, gstring->str, gstring->len);
	  token = sfi_value_parse_typed (&rvalue, scanner);
	}
    }
  ASSERT (cmp == 0);
  g_scanner_destroy (scanner);
  g_string_free (gstring, TRUE);
  g_value_unset (&rvalue);
  sfi_value_free (value);
  g_param_spec_sink (pspec);
}

static void
test_typed_serialization (gboolean serialize_typed)
{
  static const GEnumValue test_choices[] = {
    { 42, "ozo-foo", "exo", },
    { 0, NULL, NULL, },
  };
  static const SfiChoiceValues choice_values = { G_N_ELEMENTS (test_choices), test_choices };
  SfiRecFields rec_fields = { 0, NULL, };
  GParamSpec *pspec_homo_seq;
  SfiFBlock *fblock;
  SfiBBlock *bblock;
  SfiSeq *seq;
  SfiRec *rec;
  GValue *val;
  gchar str256[257];
  guint i;
  serialize_cmp_serialize_typed = serialize_typed;
  if (serialize_typed)
    MSG ("TypedSerialization:");
  else
    MSG ("ParamSerialization:");
  serialize_cmp (sfi_value_bool (FALSE),
		 sfi_param_spec_bool ("bool-false", NULL, NULL, FALSE, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_bool (TRUE),
		 sfi_param_spec_bool ("bool-true", NULL, NULL, FALSE, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_int (SFI_MAXINT),
		 sfi_param_spec_int ("int-max", NULL, NULL, 0, SFI_MININT, SFI_MAXINT, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_int (SFI_MININT),
		 sfi_param_spec_int ("int-min", NULL, NULL, 0, SFI_MININT, SFI_MAXINT, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_num (SFI_MAXNUM),
		 sfi_param_spec_num ("num-max", NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_num (SFI_MINNUM),
		 sfi_param_spec_num ("num-min", NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MAXREAL),
		 sfi_param_spec_real ("real-max", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MINREAL),
		 sfi_param_spec_real ("real-min", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (-SFI_MINREAL),
		 sfi_param_spec_real ("real-min-neg", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (-SFI_MAXREAL),
		 sfi_param_spec_real ("real-max-neg", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MININT),
		 sfi_param_spec_real ("real-minint", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MINNUM),
		 sfi_param_spec_real ("real-minnum", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_string (NULL),
		 sfi_param_spec_string ("string-nil", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_string ("test\"string'with\\character-?\007rubbish\177H"),
		 sfi_param_spec_string ("string", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  for (i = 0; i < 256; i++)
    str256[i] = i;
  str256[i] = 0;
  serialize_cmp (sfi_value_string (str256),
		 sfi_param_spec_string ("string-256", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_choice (NULL),
		 sfi_param_spec_choice ("choice-nil", NULL, NULL, NULL, choice_values, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_choice ("test-choice-with-valid-characters_9876543210"),
		 sfi_param_spec_choice ("choice", NULL, NULL, NULL, choice_values, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_proxy (SFI_MAXINT),
		 sfi_param_spec_proxy ("proxy-max", NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_proxy (SFI_MININT),
		 sfi_param_spec_proxy ("proxy-min", NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_bblock (NULL),
		 sfi_param_spec_bblock ("bblock-nil", NULL, NULL, SFI_PARAM_DEFAULT));
  bblock = sfi_bblock_new ();
  serialize_cmp (sfi_value_bblock (bblock),
		 sfi_param_spec_bblock ("bblock-empty", NULL, NULL, SFI_PARAM_DEFAULT));
  for (i = 0; i < 256; i++)
    sfi_bblock_append1 (bblock, i);
  serialize_cmp (sfi_value_bblock (bblock),
		 sfi_param_spec_bblock ("bblock", NULL, NULL, SFI_PARAM_DEFAULT));
  sfi_bblock_unref (bblock);
  serialize_cmp (sfi_value_fblock (NULL),
		 sfi_param_spec_fblock ("fblock-nil", NULL, NULL, SFI_PARAM_DEFAULT));
  fblock = sfi_fblock_new ();
  serialize_cmp (sfi_value_fblock (fblock),
		 sfi_param_spec_fblock ("fblock-empty", NULL, NULL, SFI_PARAM_DEFAULT));
  sfi_fblock_append1 (fblock, -G_MINFLOAT);
  sfi_fblock_append1 (fblock, G_MINFLOAT);
  sfi_fblock_append1 (fblock, -G_MAXFLOAT);
  sfi_fblock_append1 (fblock, G_MAXFLOAT);
  sfi_fblock_append1 (fblock, SFI_MININT);
  sfi_fblock_append1 (fblock, -SFI_MAXINT);
  sfi_fblock_append1 (fblock, SFI_MAXINT);
  sfi_fblock_append1 (fblock, SFI_MINNUM);
  sfi_fblock_append1 (fblock, -SFI_MAXNUM);
  sfi_fblock_append1 (fblock, SFI_MAXNUM);
  serialize_cmp (sfi_value_fblock (fblock),
		 sfi_param_spec_fblock ("fblock", NULL, NULL, SFI_PARAM_DEFAULT));

  serialize_cmp (sfi_value_seq (NULL),
		 sfi_param_spec_seq ("seq-nil", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  seq = sfi_seq_new ();
  serialize_cmp (sfi_value_seq (seq),
		 sfi_param_spec_seq ("seq-empty", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  val = sfi_value_bool (TRUE);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_int (42);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_string ("huhu");
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_fblock (fblock);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  if (serialize_typed)
    serialize_cmp (sfi_value_seq (seq),
		   sfi_param_spec_seq ("seq-heterogeneous", NULL, NULL,
				       sfi_param_spec_real ("dummy", NULL, NULL,
							    0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT),
				       SFI_PARAM_DEFAULT));
  sfi_seq_clear (seq);
  for (i = 0; i < 12; i++)
    {
      val = sfi_value_int (2000 - 3 * i);
      sfi_seq_append (seq, val);
      sfi_value_free (val);
    }
  pspec_homo_seq = sfi_param_spec_seq ("seq-homogeneous", NULL, NULL,
				       sfi_param_spec_int ("integer", NULL, NULL,
							   1500, 1000, 2000, 1, SFI_PARAM_DEFAULT),
				       SFI_PARAM_DEFAULT);
  g_param_spec_ref (pspec_homo_seq);
  serialize_cmp (sfi_value_seq (seq),
		 sfi_param_spec_seq ("seq-homogeneous", NULL, NULL,
				     sfi_param_spec_int ("integer", NULL, NULL,
							 1500, 1000, 2000, 1, SFI_PARAM_DEFAULT),
				     SFI_PARAM_DEFAULT));
  
  serialize_cmp (sfi_value_rec (NULL),
		 sfi_param_spec_rec ("rec-nil", NULL, NULL, rec_fields, SFI_PARAM_DEFAULT));
  rec = sfi_rec_new ();
  serialize_cmp (sfi_value_rec (rec),
		 sfi_param_spec_rec ("rec-empty", NULL, NULL, rec_fields, SFI_PARAM_DEFAULT));
  val = sfi_value_string ("huhu");
  sfi_rec_set (rec, "exo-string", val);
  if (serialize_typed)
    sfi_rec_set (rec, "exo-string2", val);
  sfi_value_free (val);
  val = sfi_value_seq (seq);
  sfi_rec_set (rec, "seq-homogeneous", val);
  sfi_value_free (val);
  val = sfi_value_proxy (102);
  sfi_rec_set (rec, "baz-proxy", val);
  sfi_value_free (val);
  rec_fields.fields = g_new (GParamSpec*, 20); /* should be static mem */
  rec_fields.fields[rec_fields.n_fields++] = sfi_param_spec_proxy ("baz-proxy", NULL, NULL, SFI_PARAM_DEFAULT);
  rec_fields.fields[rec_fields.n_fields++] = sfi_param_spec_string ("exo-string", NULL, NULL, NULL, SFI_PARAM_DEFAULT);
  rec_fields.fields[rec_fields.n_fields++] = pspec_homo_seq;
  serialize_cmp (sfi_value_rec (rec),
		 sfi_param_spec_rec ("rec", NULL, NULL, rec_fields, SFI_PARAM_DEFAULT));

  sfi_fblock_unref (fblock);
  sfi_seq_unref (seq);
  g_param_spec_unref (pspec_homo_seq);
  sfi_rec_unref (rec);
  DONE ();
}

static void
test_notes (void)
{
  gchar *str, *error = NULL;
  guint i;
  MSG ("Notes:");
  str = sfi_note_to_string (SFI_MIN_NOTE);
  ASSERT (sfi_note_from_string_err (str, &error) == SFI_MIN_NOTE);
  ASSERT (error == NULL);
  g_free (str);
  str = sfi_note_to_string (SFI_KAMMER_NOTE);
  ASSERT (sfi_note_from_string_err (str, &error) == SFI_KAMMER_NOTE);
  ASSERT (error == NULL);
  g_free (str);
  str = sfi_note_to_string (SFI_MAX_NOTE);
  ASSERT (sfi_note_from_string_err (str, &error) == SFI_MAX_NOTE);
  ASSERT (error == NULL);
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
  sfi_note_from_string_err ("NeverNote", &error);
  ASSERT (error != NULL);
  // g_print ("{%s}", error);
  g_free (error);
  DONE ();
}

static void
test_time (void)
{
  SfiTime t;
  gchar *error = NULL;
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
  ASSERT (sfi_time_from_string_err (str, &error) == t);
  ASSERT (error == NULL);
  g_free (str);
  /* test error returns */
  ASSERT (sfi_time_from_string_err ("foo 22", &error) == 0);
  ASSERT (error != NULL);
  // g_print ("{%s}", error);
  g_free (error);
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
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);

  sfi_init ();

  test_notes ();
  test_time ();
  test_renames ();
  test_scanner64 ();
  test_typed_serialization (TRUE);
  test_typed_serialization (FALSE);
  test_misc ();
  
  return 0;
}
