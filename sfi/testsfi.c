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
  sfi_pspec_sink (pspec);
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
		 sfi_pspec_bool ("bool-false", NULL, NULL, FALSE, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_bool (TRUE),
		 sfi_pspec_bool ("bool-true", NULL, NULL, FALSE, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_int (SFI_MAXINT),
		 sfi_pspec_int ("int-max", NULL, NULL, 0, SFI_MININT, SFI_MAXINT, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_int (SFI_MININT),
		 sfi_pspec_int ("int-min", NULL, NULL, 0, SFI_MININT, SFI_MAXINT, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_num (SFI_MAXNUM),
		 sfi_pspec_num ("num-max", NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_num (SFI_MINNUM),
		 sfi_pspec_num ("num-min", NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MAXREAL),
		 sfi_pspec_real ("real-max", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MINREAL),
		 sfi_pspec_real ("real-min", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (-SFI_MINREAL),
		 sfi_pspec_real ("real-min-neg", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (-SFI_MAXREAL),
		 sfi_pspec_real ("real-max-neg", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MININT),
		 sfi_pspec_real ("real-minint", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_real (SFI_MINNUM),
		 sfi_pspec_real ("real-minnum", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_string (NULL),
		 sfi_pspec_string ("string-nil", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_string ("test\"string'with\\character-?\007rubbish\177H"),
		 sfi_pspec_string ("string", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  for (i = 0; i < 256; i++)
    str256[i] = i;
  str256[i] = 0;
  serialize_cmp (sfi_value_string (str256),
		 sfi_pspec_string ("string-256", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_choice (NULL),
		 sfi_pspec_choice ("choice-nil", NULL, NULL, NULL, choice_values, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_choice ("test-choice-with-valid-characters_9876543210"),
		 sfi_pspec_choice ("choice", NULL, NULL, NULL, choice_values, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_proxy (SFI_MAXINT),
		 sfi_pspec_proxy ("proxy-max", NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_proxy (SFI_MININT),
		 sfi_pspec_proxy ("proxy-min", NULL, NULL, SFI_PARAM_DEFAULT));
  serialize_cmp (sfi_value_bblock (NULL),
		 sfi_pspec_bblock ("bblock-nil", NULL, NULL, SFI_PARAM_DEFAULT));
  bblock = sfi_bblock_new ();
  serialize_cmp (sfi_value_bblock (bblock),
		 sfi_pspec_bblock ("bblock-empty", NULL, NULL, SFI_PARAM_DEFAULT));
  for (i = 0; i < 256; i++)
    sfi_bblock_append1 (bblock, i);
  serialize_cmp (sfi_value_bblock (bblock),
		 sfi_pspec_bblock ("bblock", NULL, NULL, SFI_PARAM_DEFAULT));
  sfi_bblock_unref (bblock);
  serialize_cmp (sfi_value_fblock (NULL),
		 sfi_pspec_fblock ("fblock-nil", NULL, NULL, SFI_PARAM_DEFAULT));
  fblock = sfi_fblock_new ();
  serialize_cmp (sfi_value_fblock (fblock),
		 sfi_pspec_fblock ("fblock-empty", NULL, NULL, SFI_PARAM_DEFAULT));
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
		 sfi_pspec_fblock ("fblock", NULL, NULL, SFI_PARAM_DEFAULT));
  
  serialize_cmp (sfi_value_seq (NULL),
		 sfi_pspec_seq ("seq-nil", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
  seq = sfi_seq_new ();
  serialize_cmp (sfi_value_seq (seq),
		 sfi_pspec_seq ("seq-empty", NULL, NULL, NULL, SFI_PARAM_DEFAULT));
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
		   sfi_pspec_seq ("seq-heterogeneous", NULL, NULL,
				  sfi_pspec_real ("dummy", NULL, NULL,
						  0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_DEFAULT),
				  SFI_PARAM_DEFAULT));
  sfi_seq_clear (seq);
  for (i = 0; i < 12; i++)
    {
      val = sfi_value_int (2000 - 3 * i);
      sfi_seq_append (seq, val);
      sfi_value_free (val);
    }
  pspec_homo_seq = sfi_pspec_seq ("seq-homogeneous", NULL, NULL,
				  sfi_pspec_int ("integer", NULL, NULL,
						 1500, 1000, 2000, 1, SFI_PARAM_DEFAULT),
				  SFI_PARAM_DEFAULT);
  sfi_pspec_ref (pspec_homo_seq);
  serialize_cmp (sfi_value_seq (seq),
		 sfi_pspec_seq ("seq-homogeneous", NULL, NULL,
				sfi_pspec_int ("integer", NULL, NULL,
					       1500, 1000, 2000, 1, SFI_PARAM_DEFAULT),
				SFI_PARAM_DEFAULT));
  
  serialize_cmp (sfi_value_rec (NULL),
		 sfi_pspec_rec ("rec-nil", NULL, NULL, rec_fields, SFI_PARAM_DEFAULT));
  rec = sfi_rec_new ();
  serialize_cmp (sfi_value_rec (rec),
		 sfi_pspec_rec ("rec-empty", NULL, NULL, rec_fields, SFI_PARAM_DEFAULT));
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
  rec_fields.fields[rec_fields.n_fields++] = sfi_pspec_proxy ("baz-proxy", NULL, NULL, SFI_PARAM_DEFAULT);
  rec_fields.fields[rec_fields.n_fields++] = sfi_pspec_string ("exo-string", NULL, NULL, NULL, SFI_PARAM_DEFAULT);
  rec_fields.fields[rec_fields.n_fields++] = pspec_homo_seq;
  serialize_cmp (sfi_value_rec (rec),
		 sfi_pspec_rec ("rec", NULL, NULL, rec_fields, SFI_PARAM_DEFAULT));
  
  sfi_fblock_unref (fblock);
  sfi_seq_unref (seq);
  sfi_pspec_unref (pspec_homo_seq);
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

static gboolean gen_vcall_switch = TRUE;

static void
vcall_gen (guint sig)
{
  guint i, n;
  if (!gen_vcall_switch)
    {
      g_print ("static void\nvcall_%u (gpointer func, gpointer arg0, Arg *alist)\n{\n", sig);
      g_print ("  void (*f) (gpointer");
      for (i = sig; i; i >>= 2)
	switch (i & 3)
	  {
	  case 1:	g_print (", guint32");		break;
	  case 2:	g_print (", guint64");		break;
	  case 3:	g_print (", double");		break;
	  }
      g_print (") = func;\n");
      g_print ("  f (arg0");
      for (i = sig, n = 0; i; i >>= 2, n++)
	switch (i & 3)
	  {
	  case 1:	g_print (", alist[%u].v32", n);		break;
	  case 2:	g_print (", alist[%u].v64", n);		break;
	  case 3:	g_print (", alist[%u].vdbl", n);	break;
	  }
      g_print (");\n}\n");
    }
  else
    {
      g_print ("    case %6u: return vcall_%u;\n", sig, sig);
    }
}

static void
vcall_rec (guint sig,
	   guint n_args)
{
  if (n_args--)
    {
      sig <<= 2;
      vcall_rec (sig | 1, n_args);
      vcall_rec (sig | 2, n_args);
      vcall_rec (sig | 3, n_args);
    }
  else
    vcall_gen (sig);
}

static void
vcalls_generate (void)
{
  guint n;
  
  gen_vcall_switch = FALSE;
  for (n = 0; n <= SFI_VCALL_MAX_ARGS; n++)
    vcall_rec (SFI_VCALL_PTR_ID, n);
  
  gen_vcall_switch = TRUE;
  g_print ("static VCall\nvcall_switch (guint sig)\n{\n");
  g_print ("  switch (sig)\n    {\n");
  for (n = 0; n <= SFI_VCALL_MAX_ARGS; n++)
    vcall_rec (SFI_VCALL_PTR_ID, n);
  g_print ("    default: g_assert_not_reached (); return NULL;\n");
  g_print ("    }\n}\n");
}

static gchar *pointer1 = "huhu";
static gchar *pointer2 = "haha";
static gchar *pointer3 = "zoot";

static void
test_vcalls_func (gpointer o,
		  SfiInt   i,
		  SfiNum   n,
		  SfiProxy p,
		  SfiReal  r,
		  SfiNum   self,
		  gpointer data)
{
  ASSERT (o == pointer1);
  ASSERT (i == -2134567);
  ASSERT (n == -2598768763298128732);
  ASSERT (p == (SfiProxy) pointer2);
  ASSERT (r == -426.9112e-267);
  ASSERT (self == (SfiNum) test_vcalls_func);
  ASSERT (data == pointer3);
}

static void
test_vcalls (void)
{
  GValue *val;
  SfiSeq *seq = sfi_seq_new ();
  MSG ("VCalls:");
  val = sfi_value_int (-2134567);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_num (-2598768763298128732);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_proxy ((SfiProxy) pointer2);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_real (-426.9112e-267);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  val = sfi_value_num ((SfiNum) test_vcalls_func);
  sfi_seq_append (seq, val);
  sfi_value_free (val);
  sfi_vcall_void (test_vcalls_func, pointer1,
		  seq->n_elements, seq->elements,
		  pointer3);
  ASSERT (0 == 0);
  DONE ();
  sfi_seq_unref (seq);
}

int
main (int   argc,
      char *argv[])
{
  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
  
  sfi_init ();
  
  if (0)
    {
      vcalls_generate ();
      return 0;
    }
  
  test_notes ();
  test_time ();
  test_renames ();
  test_scanner64 ();
  test_typed_serialization (TRUE);
  test_typed_serialization (FALSE);
  test_vcalls ();
  test_misc ();
  
  return 0;
}
