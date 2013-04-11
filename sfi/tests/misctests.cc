// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#undef G_LOG_DOMAIN
#define  G_LOG_DOMAIN __FILE__
// #define TEST_VERBOSE
#include <sfi/sfitests.hh>
#include <unistd.h>
#include <string.h>
#include <signal.h>	/* G_BREAKPOINT() */
#include <math.h>
/* provide IDL type initializers */
#define sfidl_pspec_Real(group, name, nick, blurb, dflt, min, max, step, hints)  \
  sfi_pspec_real (name, nick, blurb, dflt, min, max, step, hints)
#define sfidl_pspec_Record(group, name, nick, blurb, hints, fields)            \
  sfi_pspec_rec (name, nick, blurb, fields, hints)
#define sfidl_pspec_Choice(group, name, nick, blurb, default_value, hints, choices) \
  sfi_pspec_choice (name, nick, blurb, default_value, choices, hints)
/* FIXME: small hackery */
#define sfidl_pspec_Rec(group, name, nick, blurb, hints)            \
  sfi_pspec_int (name, nick, blurb, 0, 0, 0, 0, hints)
#define sfidl_pspec_PSpec(group, name, nick, blurb, hints)            \
  sfi_pspec_int (name, nick, blurb, 0, 0, 0, 0, hints)
#include "testidl.h"
static void
test_misc (void)
{
  TSTART ("Misc");
  TASSERT (0 == 0);
  TDONE ();
}

static void
test_time (void)
{
  SfiTime t;
  gchar *error = NULL;
  gchar *str;
  const gchar *time_strings[] = {
    "1990-01-01 00:00:00",
    "1999-03-12 23:41:00",
    "2037-12-31 23:59:59",
  };
  gint i;
  TSTART ("Time");
  TASSERT (SFI_USEC_FACTOR == 1000000);
  TASSERT (SFI_MIN_TIME + 1000000 < SFI_MAX_TIME);
  t = sfi_time_system ();
  if (t < SFI_MIN_TIME || t > SFI_MAX_TIME)
    {
      TACK ();
      t = SFI_MIN_TIME / 2 + SFI_MAX_TIME / 2;
    }
  else
    TICK ();
  t /= SFI_USEC_FACTOR;
  t *= SFI_USEC_FACTOR;
  str = sfi_time_to_string (t);
  TASSERT (sfi_time_from_string_err (str, &error) == t);
  TASSERT (error == NULL);
  g_free (str);
  /* test hard boundaries */
  TASSERT (sfi_time_from_string ("1990-01-01 00:00:00") == SFI_MIN_TIME);
  TASSERT (sfi_time_from_string ("2038-01-19 03:14:07") == SFI_MAX_TIME);
  /* test error returns */
  TASSERT (sfi_time_from_string_err ("foo 22", &error) == 0);
  TASSERT (error != NULL);
  // g_print ("{%s}", error);
  g_free (error);
  for (i = 0; size_t (i) < G_N_ELEMENTS (time_strings); i++)
    {
      t = sfi_time_from_string_err (time_strings[i], &error);
      if (!error)
	TICK ();
      else
	g_print ("{failed to parse \"%s\": %s (got: %s)\n}", time_strings[i], error, sfi_time_to_string (t)); /* memleak */
      g_free (error);
      str = sfi_time_to_string (t);
      TASSERT (sfi_time_from_string_err (str, &error) == t);
      TASSERT (error == NULL);
      g_free (str);
    }
  TDONE ();
}
static void
test_com_ports (void)
{
  gint afds[2], pipe_error;
  SfiComPort *port1, *port2;
  GValue *value, *rvalue;
  TSTART ("Communication Ports");
  pipe_error = pipe (afds);
  TASSERT (pipe_error == 0);
  port1 = sfi_com_port_from_pipe ("portA", -1, afds[1]);
  TASSERT (port1->connected == TRUE);
  port2 = sfi_com_port_from_pipe ("portB", afds[0], -1);
  TASSERT (port2->connected == TRUE);
  /* transport a value */
  {	/* create complex value */
    GParamSpec *pspec = sfi_pspec_log_scale ("name", "Nick", "The Blurb", 440, 110, 1760, 0.03, 440, 2, 2, SFI_PARAM_GUI);
    SfiRec *rec = sfi_pspec_to_rec (pspec);
    g_param_spec_ref (pspec);
    g_param_spec_sink (pspec);
    g_param_spec_unref (pspec);
    value = sfi_value_rec (rec);
    sfi_rec_unref (rec);
  }
  sfi_com_port_send (port1, value);
  rvalue = sfi_com_port_recv (port2);
  TASSERT (rvalue != NULL);
  {	/* assert equality of values */
    GString *s1 = g_string_new (NULL), *s2 = g_string_new (NULL);
    sfi_value_store_typed (value, s1);
    sfi_value_store_typed (rvalue, s2);
    TASSERT (strcmp (s1->str, s2->str) == 0);
    g_string_free (s1, TRUE);
    g_string_free (s2, TRUE);
  }
  sfi_value_free (value);
  sfi_value_free (rvalue);
  sfi_com_port_close_remote (port1, TRUE);
  pipe_error = close (afds[1]);
  TASSERT (pipe_error == -1);
  sfi_com_port_close_remote (port2, TRUE);
  pipe_error = close (afds[0]);
  TASSERT (pipe_error == -1);
  sfi_com_port_unref (port1);
  sfi_com_port_unref (port2);
  TDONE ();
}
#define SCANNER_ASSERT64(scanner, printout, token, text, svalue) { \
  g_scanner_input_text (scanner, text, strlen (text)); \
  TASSERT (g_scanner_get_next_token (scanner) == token); \
  if (printout) g_print ("{scanner.v_int64:%llu}", (long long unsigned int) (scanner->value.v_int64)); \
  TASSERT (scanner->value.v_int64 == svalue); \
  TASSERT (g_scanner_get_next_token (scanner) == '#'); \
}
#define SCANNER_ASSERTf(scanner, printout, vtoken, text, svalue) { \
  g_scanner_input_text (scanner, text, strlen (text)); \
  if (g_scanner_get_next_token (scanner) != vtoken) \
    g_scanner_unexp_token (scanner, vtoken, NULL, NULL, NULL, NULL, TRUE); \
  TASSERT (scanner->token == vtoken); \
  if (printout) g_print ("{scanner.v_float:%17g}", scanner->value.v_float); \
  TASSERT (scanner->value.v_float == svalue); \
  TASSERT (g_scanner_get_next_token (scanner) == '#'); \
}
static void
test_scanner64 (void)
{
  GScanner *scanner = g_scanner_new64 (sfi_storage_scanner_config);
  TSTART ("64Bit Scanner");
  scanner->config->numbers_2_int = FALSE;
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_BINARY, " 0b0 #", 0);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_BINARY, " 0b10000000000000000 #", 65536);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_BINARY, " 0b11111111111111111111111111111111 #", 4294967295U);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_BINARY, " 0b1111111111111111111111111111111111111111111111111111111111111111 #", 18446744073709551615ULL);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_OCTAL, " 0 #", 0);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_OCTAL, " 0200000 #", 65536);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_OCTAL, " 037777777777 #", 4294967295U);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_OCTAL, " 01777777777777777777777 #", 18446744073709551615ULL);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_HEX, " 0x0 #", 0);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_HEX, " 0x10000 #", 65536);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_HEX, " 0xffffffff #", 4294967295U);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_HEX, " 0xffffffffffffffff #", 18446744073709551615ULL);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_INT, " 65536 #", 65536);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_INT, " 4294967295 #", 4294967295U);
  SCANNER_ASSERT64 (scanner, FALSE, G_TOKEN_INT, " 18446744073709551615 #", 18446744073709551615ULL);
  SCANNER_ASSERTf (scanner, FALSE, G_TOKEN_FLOAT, " 0.0 #", 0);
  SCANNER_ASSERTf (scanner, FALSE, G_TOKEN_FLOAT, " 2.2250738585072014e-308 #", 2.2250738585072014e-308);
  SCANNER_ASSERTf (scanner, FALSE, G_TOKEN_FLOAT, " 1.7976931348623157e+308 #", 1.7976931348623157e+308);
  g_scanner_destroy (scanner);
  TDONE ();
}
typedef enum /*< skip >*/
{
  SERIAL_TEST_TYPED = 1,
  SERIAL_TEST_PARAM,
  SERIAL_TEST_PSPEC
} SerialTest;
static SerialTest serial_test_type = SerialTest (0);
static void
serial_pspec_check (GParamSpec *pspec,
		    GScanner   *scanner)
{
  GValue *value = sfi_value_pspec (pspec), rvalue = { 0, };
  GString *s1 = g_string_new (NULL);
  GString *s2 = g_string_new (NULL);
  GTokenType token;
  sfi_value_store_typed (value, s1);
  g_scanner_input_text (scanner, s1->str, s1->len);
  token = sfi_value_parse_typed (&rvalue, scanner);
  if (token != G_TOKEN_NONE)
    {
      g_print ("{while parsing pspec \"%s\":\n\t%s\n", pspec->name, s1->str);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
			     g_strdup_printf ("failed to serialize pspec \"%s\"", pspec->name), TRUE);
    }
  TASSERT (token == G_TOKEN_NONE);
  sfi_value_store_typed (&rvalue, s2);
  if (strcmp (s1->str, s2->str))
    g_print ("{while comparing pspecs \"%s\":\n\t%s\n\t%s\n", pspec->name, s1->str, s2->str);
  TASSERT (strcmp (s1->str, s2->str) == 0);
  g_value_unset (&rvalue);
  sfi_value_free (value);
  g_string_free (s1, TRUE);
  g_string_free (s2, TRUE);
}
// serialize @a value according to @a pspec, deserialize and assert a matching result
static void
serialize_cmp (GValue     *value,
	       GParamSpec *pspec)
{
  GScanner *scanner = g_scanner_new64 (sfi_storage_scanner_config);
  GString *gstring = g_string_new (NULL);
  GValue rvalue = { 0, };
  GTokenType token;
  gint cmp;
  if (serial_test_type == SERIAL_TEST_PSPEC)
    serial_pspec_check (pspec, scanner);
  else
    {
      // if (pspec && strcmp (pspec->name, "real-max") == 0) G_BREAKPOINT ();
      if (serial_test_type == SERIAL_TEST_TYPED)
	sfi_value_store_typed (value, gstring);
      else /* SERIAL_TEST_PARAM */
	sfi_value_store_param (value, gstring, pspec, 2);
      g_scanner_input_text (scanner, gstring->str, gstring->len);
      if (serial_test_type == SERIAL_TEST_TYPED)
	token = sfi_value_parse_typed (&rvalue, scanner);
      else /* SERIAL_TEST_PARAM */
	{
	  if (g_scanner_get_next_token (scanner) == '(')
	    if (g_scanner_get_next_token (scanner) == G_TOKEN_IDENTIFIER &&
		strcmp (scanner->value.v_identifier, pspec->name) == 0)
	      token = sfi_value_parse_param_rest (&rvalue, scanner, pspec);
	    else
	      token = G_TOKEN_IDENTIFIER;
	  else
	    token = GTokenType ('(');
	}
      if (0)
	g_print ("{parsing:%s}", gstring->str);
      if (token != G_TOKEN_NONE)
	{
	  g_print ("{while parsing \"%s\":\n\t%s\n", pspec->name, gstring->str);
	  g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
				 g_strdup_printf ("failed to serialize \"%s\"", pspec->name), TRUE);
	}
      TASSERT (token == G_TOKEN_NONE);
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
      TASSERT (cmp == 0);
      if (0) /* generate testoutput */
	g_print ("OK=================(%s)=================:\n%s\n", pspec->name, gstring->str);
    }
  g_scanner_destroy (scanner);
  g_string_free (gstring, TRUE);
  if (G_VALUE_TYPE (&rvalue))
    g_value_unset (&rvalue);
  sfi_value_free (value);
  sfi_pspec_sink (pspec);
}
static void
test_typed_serialization (SerialTest test_type)
{
  static const SfiChoiceValue test_choices[] = {
    { "ozo-foo", "Ozo-foo blurb", },
    { "emptyblurb", "", },
    { "noblurb", NULL, },
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
  serial_test_type = test_type;
  switch (serial_test_type)
    {
    case SERIAL_TEST_TYPED:	TSTART ("Typed Serialization"); break;
    case SERIAL_TEST_PARAM:	TSTART ("Param Serialization"); break;
    case SERIAL_TEST_PSPEC:	TSTART ("Pspec Serialization"); break;
    }
  serialize_cmp (sfi_value_bool (FALSE),
		 sfi_pspec_bool ("bool-false", NULL, NULL, FALSE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_bool (TRUE),
		 sfi_pspec_bool ("bool-true", NULL, NULL, FALSE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_int (SFI_MAXINT),
		 sfi_pspec_int ("int-max", NULL, NULL, 0, SFI_MININT, SFI_MAXINT, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_int (SFI_MININT),
		 sfi_pspec_int ("int-min", NULL, NULL, 0, SFI_MININT, SFI_MAXINT, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_num (SFI_MAXNUM),
		 sfi_pspec_num ("num-max", NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_num (SFI_MINNUM),
		 sfi_pspec_num ("num-min", NULL, NULL, 0, SFI_MINNUM, SFI_MAXNUM, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (SFI_MINREAL),
		 sfi_pspec_real ("real-min", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (SFI_MAXREAL),
		 sfi_pspec_real ("real-max", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (-SFI_MINREAL),
		 sfi_pspec_real ("real-min-neg", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (-SFI_MAXREAL),
		 sfi_pspec_real ("real-max-neg", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (SFI_MININT),
		 sfi_pspec_real ("real-minint", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_real (SFI_MINNUM),
		 sfi_pspec_real ("real-minnum", NULL, NULL, 0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_note (SFI_MIN_NOTE),
		 sfi_pspec_note ("vnote-min", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, TRUE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_note (SFI_MAX_NOTE),
		 sfi_pspec_note ("vnote-max", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, TRUE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_note (SFI_NOTE_VOID),
		 sfi_pspec_note ("vnote-void", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, TRUE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_note (SFI_MIN_NOTE),
		 sfi_pspec_note ("note-min", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, FALSE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_note (SFI_MAX_NOTE),
		 sfi_pspec_note ("note-max", NULL, NULL, SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE, FALSE, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_string (NULL),
		 sfi_pspec_string ("string-nil", NULL, NULL, NULL, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_string ("test\"string'with\\character-?\007rubbish\177H"),
		 sfi_pspec_string ("string", NULL, NULL, NULL, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_string (""),
		 sfi_pspec_string ("string-empty", NULL, NULL, NULL, SFI_PARAM_STANDARD));
  for (i = 0; i < 255; i++)
    str256[i] = i + 1;
  str256[i] = 0;
  serialize_cmp (sfi_value_string (str256),
		 sfi_pspec_string ("string-255", NULL, NULL, NULL, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_choice (NULL),
		 sfi_pspec_choice ("choice-nil", NULL, NULL, NULL, choice_values, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_choice ("test-choice-with-valid-characters_9876543210"),
		 sfi_pspec_choice ("choice", NULL, NULL, NULL, choice_values, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_proxy (SFI_MAXINT),
		 sfi_pspec_proxy ("proxy-max", NULL, NULL, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_proxy (G_MAXUINT),
		 sfi_pspec_proxy ("proxy-umax", NULL, NULL, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_bblock (NULL),
		 sfi_pspec_bblock ("bblock-nil", NULL, NULL, SFI_PARAM_STANDARD));
  bblock = sfi_bblock_new ();
  serialize_cmp (sfi_value_bblock (bblock),
		 sfi_pspec_bblock ("bblock-empty", NULL, NULL, SFI_PARAM_STANDARD));
  for (i = 0; i < 256; i++)
    sfi_bblock_append1 (bblock, i);
  serialize_cmp (sfi_value_bblock (bblock),
		 sfi_pspec_bblock ("bblock", NULL, NULL, SFI_PARAM_STANDARD));
  sfi_bblock_unref (bblock);
  serialize_cmp (sfi_value_fblock (NULL),
		 sfi_pspec_fblock ("fblock-nil", NULL, NULL, SFI_PARAM_STANDARD));
  fblock = sfi_fblock_new ();
  serialize_cmp (sfi_value_fblock (fblock),
		 sfi_pspec_fblock ("fblock-empty", NULL, NULL, SFI_PARAM_STANDARD));
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
		 sfi_pspec_fblock ("fblock", NULL, NULL, SFI_PARAM_STANDARD));
  serialize_cmp (sfi_value_seq (NULL),
		 sfi_pspec_seq ("seq-nil", NULL, NULL, NULL, SFI_PARAM_STANDARD));
  seq = sfi_seq_new ();
  serialize_cmp (sfi_value_seq (seq),
		 sfi_pspec_seq ("seq-empty", NULL, NULL, NULL, SFI_PARAM_STANDARD));
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
  if (serial_test_type != SERIAL_TEST_PARAM)
    serialize_cmp (sfi_value_seq (seq),
		   sfi_pspec_seq ("seq-heterogeneous", NULL, NULL,
				  sfi_pspec_real ("dummy", NULL, NULL,
						  0, -SFI_MAXREAL, SFI_MAXREAL, 1, SFI_PARAM_STANDARD),
				  SFI_PARAM_STANDARD));
  sfi_seq_clear (seq);
  for (i = 0; i < 12; i++)
    {
      val = sfi_value_int (2000 - 3 * i);
      sfi_seq_append (seq, val);
      sfi_value_free (val);
    }
  pspec_homo_seq = sfi_pspec_seq ("seq-homogeneous", NULL, NULL,
				  sfi_pspec_int ("integer", NULL, NULL,
						 1500, 1000, 2000, 1, SFI_PARAM_STANDARD),
				  SFI_PARAM_STANDARD);
  sfi_pspec_ref (pspec_homo_seq);
  serialize_cmp (sfi_value_seq (seq),
		 sfi_pspec_seq ("seq-homogeneous", NULL, NULL,
				sfi_pspec_int ("integer", NULL, NULL,
					       1500, 1000, 2000, 1, SFI_PARAM_STANDARD),
				SFI_PARAM_STANDARD));
  if (serial_test_type == SERIAL_TEST_PSPEC)
    {
      serialize_cmp (sfi_value_pspec (NULL),
		     sfi_pspec_pspec ("pspec-nil", NULL, NULL, SFI_PARAM_STANDARD));
      serialize_cmp (sfi_value_pspec (pspec_homo_seq),
		     sfi_pspec_pspec ("pspec-hseq", NULL, NULL, SFI_PARAM_STANDARD));
    }
  serialize_cmp (sfi_value_rec (NULL),
		 sfi_pspec_rec ("rec-nil", NULL, NULL, rec_fields, SFI_PARAM_STANDARD));
  rec = sfi_rec_new ();
  serialize_cmp (sfi_value_rec (rec),
		 sfi_pspec_rec ("rec-empty", NULL, NULL, rec_fields, SFI_PARAM_STANDARD));
  val = sfi_value_string ("huhu");
  sfi_rec_set (rec, "exo-string", val);
  if (serial_test_type != SERIAL_TEST_PARAM)
    sfi_rec_set (rec, "exo-string2", val);
  sfi_value_free (val);
  val = sfi_value_seq (seq);
  sfi_rec_set (rec, "seq-homogeneous", val);
  sfi_value_free (val);
  val = sfi_value_proxy (102);
  sfi_rec_set (rec, "baz-proxy", val);
  sfi_value_free (val);
  rec_fields.fields = g_new (GParamSpec*, 20); /* should be static mem */
  rec_fields.fields[rec_fields.n_fields++] = sfi_pspec_proxy ("baz-proxy", NULL, NULL, SFI_PARAM_STANDARD);
  rec_fields.fields[rec_fields.n_fields++] = sfi_pspec_string ("exo-string", NULL, NULL, NULL, SFI_PARAM_STANDARD);
  rec_fields.fields[rec_fields.n_fields++] = pspec_homo_seq;
  serialize_cmp (sfi_value_rec (rec),
		 sfi_pspec_rec ("rec", NULL, NULL, rec_fields, SFI_PARAM_STANDARD));
  sfi_fblock_unref (fblock);
  sfi_seq_unref (seq);
  sfi_pspec_unref (pspec_homo_seq);
  sfi_rec_unref (rec);
  TDONE ();
}
static void
test_notes (void)
{
  gchar *str, *error = NULL;
  guint i;
  TSTART ("Notes");
  str = sfi_note_to_string (SFI_MIN_NOTE);
  TASSERT (sfi_note_from_string_err (str, &error) == SFI_MIN_NOTE);
  TASSERT (error == NULL);
  g_free (str);
  str = sfi_note_to_string (SFI_KAMMER_NOTE);
  TASSERT (sfi_note_from_string_err (str, &error) == SFI_KAMMER_NOTE);
  TASSERT (error == NULL);
  g_free (str);
  str = sfi_note_to_string (SFI_MAX_NOTE);
  TASSERT (sfi_note_from_string_err (str, &error) == SFI_MAX_NOTE);
  TASSERT (error == NULL);
  g_free (str);
  for (i = SFI_MIN_NOTE; i <= SFI_MAX_NOTE; i++)
    {
      int octave, semitone;
      gboolean black_semitone;
      gchar letter;
      sfi_note_examine (i, &octave, &semitone, &black_semitone, &letter);
      TASSERT (octave == SFI_NOTE_OCTAVE (i));
      TASSERT (semitone == SFI_NOTE_SEMITONE (i));
      TASSERT (SFI_NOTE_GENERIC (octave, semitone) == int (i));
    }
  sfi_note_from_string_err ("NeverNote", &error);
  TASSERT (error != NULL);
  // g_print ("{%s}", error);
  g_free (error);
  TDONE ();
}

static void
test_renames (void)
{
  gchar *str;
  TSTART ("Renames");
  str = g_type_name_to_cname ("PrefixTypeName");
  TASSERT (strcmp (str, "prefix_type_name") == 0);
  g_free (str);
  str = g_type_name_to_sname ("PrefixTypeName");
  TASSERT (strcmp (str, "prefix-type-name") == 0);
  g_free (str);
  str = g_type_name_to_cupper ("PrefixTypeName");
  TASSERT (strcmp (str, "PREFIX_TYPE_NAME") == 0);
  g_free (str);
  str = g_type_name_to_type_macro ("PrefixTypeName");
  TASSERT (strcmp (str, "PREFIX_TYPE_TYPE_NAME") == 0);
  g_free (str);
  str = g_type_name_to_sname ("prefix_type_name");
  TASSERT (strcmp (str, "prefix-type-name") == 0);
  g_free (str);
  str = g_type_name_to_cname ("prefix-type-name");
  TASSERT (strcmp (str, "prefix_type_name") == 0);
  g_free (str);
  TDONE ();
}
static gboolean vmarshal_switch = TRUE;
static guint    vmarshal_count = 0;
static void
generate_vmarshal (guint sig)
{
  gchar *s, signame[32 * 4 + 1];
  guint i;
  vmarshal_count++;
  s = signame + sizeof (signame);
  *--s = 0;
  for (i = sig; i; i >>= 2)
    *--s = '0' + (i & 3);
  if (!vmarshal_switch)
    {
      g_print ("static void /* %u */\nsfi_vmarshal_%s (gpointer func, gpointer arg0, Arg *alist)\n{\n",
	       vmarshal_count, s);
      g_print ("  void (*f) (gpointer");
      for (i = 0; s[i]; i++)
	switch (s[i] - '0')
	  {
	  case 1:	g_print (", guint32");		break;
	  case 2:	g_print (", guint64");		break;
	  case 3:	g_print (", double");		break;
	  }
      g_print (", gpointer) = func;\n");
      g_print ("  f (arg0");
      for (i = 0; s[i]; i++)
	switch (s[i] - '0')
	  {
	  case 1:	g_print (", alist[%u].v32", i);		break;
	  case 2:	g_print (", alist[%u].v64", i);		break;
	  case 3:	g_print (", alist[%u].vdbl", i);	break;
	  }
      g_print (", alist[%u].vpt);\n}\n", i);
    }
  else
    g_print ("    case 0x%03x: return sfi_vmarshal_%s; /* %u */\n", sig, s, vmarshal_count);
}
static void
generate_vmarshal_loop (void)
{
  guint sig, i, ki[SFI_VMARSHAL_MAX_ARGS + 1];
  vmarshal_count = 0;
  /* initialize digits */
  for (i = 0; i < SFI_VMARSHAL_MAX_ARGS; i++)
    ki[i] = 1;
  /* initialize overflow */
  ki[SFI_VMARSHAL_MAX_ARGS] = 0;
  while (ki[SFI_VMARSHAL_MAX_ARGS] == 0)	/* abort on overflow */
    {
      /* construct signature */
      sig = 0;
      for (i = SFI_VMARSHAL_MAX_ARGS; i > 0; i--)
	{
	  sig <<= 2;
	  sig |= ki[i - 1];
	}
      /* generate */
      generate_vmarshal (sig);
      /* increase digit wise: 1, 2, 3, 11, 12, 13, 21, 22, 23, 31, ... */
      for (i = 0; ; i++)
	{
	  if (++ki[i] <= 3)
	    break;
	  ki[i] = 1;
	}
    }
}
static void
generate_vmarshal_code (void)
{
  vmarshal_switch = FALSE;
  generate_vmarshal_loop ();
  vmarshal_switch = TRUE;
  g_print ("static VMarshal\nsfi_vmarshal_switch (guint sig)\n{\n");
  g_print ("  switch (sig)\n    {\n");
  generate_vmarshal_loop ();
  g_print ("    default: g_assert_not_reached (); return NULL;\n");
  g_print ("    }\n}\n");
}
static const char *pointer1 = "huhu";
static const char *pointer2 = "haha";
static const char *pointer3 = "zoot";
static void
test_vmarshal_func4 (gpointer o,
		     SfiReal  r,
		     SfiNum   n,
		     gpointer data)
{
  TASSERT (o == pointer1);
  TASSERT (r == -426.9112e-267);
  TASSERT (n == -2598768763298128732LL);
  TASSERT (data == pointer3);
}
static void
test_vmarshal_func7 (gpointer o,
		     SfiReal  r,
		     SfiNum   n,
		     SfiProxy p,
		     SfiInt   i,
		     SfiNum   self,
		     gpointer data)
{
  TASSERT (o == pointer1);
  TASSERT (r == -426.9112e-267);
  TASSERT (n == -2598768763298128732LL);
  TASSERT (p == (SfiProxy) pointer2);
  TASSERT (i == -2134567);
  TASSERT (self == (long) test_vmarshal_func7);
  TASSERT (data == pointer3);
}
static void
test_vmarshals (void)
{
  SfiSeq *seq = sfi_seq_new ();
  TSTART ("Vmarshals");
  sfi_seq_append_real (seq, -426.9112e-267);
  sfi_seq_append_num (seq, -2598768763298128732LL);
  sfi_vmarshal_void ((void*) test_vmarshal_func4, (void*) pointer1, seq->n_elements, seq->elements, (void*) pointer3);
  sfi_seq_append_proxy (seq, (SfiProxy) pointer2);
  sfi_seq_append_int (seq, -2134567);
  sfi_seq_append_num (seq, (long) test_vmarshal_func7);
  sfi_vmarshal_void ((void*) test_vmarshal_func7, (void*) pointer1, seq->n_elements, seq->elements, (void*) pointer3);
  TDONE ();
  sfi_seq_unref (seq);
}
static void
test_sfidl_seq (void)
{
  TestPositionSeq* pseq;
  TestPosition* pos;
  TestPosition* pos2;
  SfiRec* rec;
  GValue* value;
  TSTART ("Sfidl generated code");
  /* test that types are registered properly */
  // TASSERT (TEST_TYPE_POSITION != 0);
  // TASSERT (TEST_TYPE_POSITION_SEQ != 0);
  // TASSERT (TEST_TYPE_YES_NO_UNDECIDED != 0);
  /* test sequences and structs generated for Position record */
  pseq = test_position_seq_new ();
  TASSERT (pseq != NULL);
  TASSERT (pseq->n_positions == 0);
  pos = test_position_new ();
  TASSERT (pos != NULL);
  pos->x = 1.0;
  pos->y = -1.0;
  pos->relevant = TEST_NO;
  test_position_seq_append (pseq, pos);
  TASSERT (pseq->n_positions == 1);
  test_position_seq_resize (pseq, 4);
  TASSERT (pseq->n_positions == 4);
  test_position_seq_resize (pseq, 1);
  TASSERT (pseq->n_positions == 1);
  rec = test_position_to_rec (pos);
  value = sfi_rec_get (rec, "relevant");
  TASSERT (SFI_VALUE_HOLDS_CHOICE (value));
  TASSERT (strcmp (sfi_value_get_choice (value), "test-no") == 0);
  pos2 = test_position_from_rec (rec);
  TASSERT (pos->x == pos2->x);
  TASSERT (pos->y == pos2->y);
  TASSERT (pos->relevant == pos2->relevant);
  sfi_rec_unref (rec);
  test_position_seq_free (pseq);
  test_position_free (pos);
  test_position_free (pos2);
  /* test validation and defaulting */
  {
    GParamSpec *pspec;
    GValue rec_value = { 0, };
    /* create empty record */
    g_value_init (&rec_value, SFI_TYPE_REC);
    sfi_value_take_rec (&rec_value, sfi_rec_new ());
    /* validate record against pspec */
    pspec = sfi_pspec_rec ("foo", "bar", "bazz", test_position_fields, SFI_PARAM_STANDARD);
    g_param_value_validate (pspec, &rec_value);
    g_param_spec_unref (pspec);
#if 0
    GValue pos_value = { 0, };
    /* transform record to boxed type */
    g_value_init (&pos_value, TEST_TYPE_POSITION);
    TASSERT (sfi_value_type_transformable (SFI_TYPE_REC, TEST_TYPE_POSITION));
    sfi_value_transform (&rec_value, &pos_value);
    /* get boxed type */
    TASSERT (G_VALUE_HOLDS (&pos_value, TEST_TYPE_POSITION));
    pos = g_value_get_boxed (&pos_value);
    /* check that values match defaults */
    TASSERT (pos->x == 2.0);
    TASSERT (pos->y == 3.0);
    TASSERT (pos->relevant == TEST_YES);
    /* cleanup */
    g_value_unset (&rec_value);
    g_value_unset (&pos_value);
#endif
  }
  /* test that correct code is generated for double constants */
  TASSERT (test_real_division_fields.n_fields == 1);
  SfiReal pi = sfi_pspec_get_real_default (test_real_division_fields.fields[0]);
  TASSERT (fabs (3.14 - pi) < 0.01);
  /* test constants */
  // TASSERT (TEST_ANSWER_B == 42);
  // TASSERT (strcmp(TEST_ULTIMATE_ANSWER, "the answer to all questions is 42") == 0);
  TDONE ();
}
#include "testidl.c"
int
main (int   argc,
      char *argv[])
{
  sfi_init_test (&argc, &argv, NULL);
  test_types_init ();
  if (0)
    {
      generate_vmarshal_code ();
      return 0;
    }
  test_notes ();
  test_time ();
  test_renames ();
  test_scanner64 ();
  test_typed_serialization (SERIAL_TEST_PARAM);
  test_typed_serialization (SERIAL_TEST_TYPED);
  test_typed_serialization (SERIAL_TEST_PSPEC);
  test_vmarshals ();
  test_com_ports ();
  test_sfidl_seq ();
  test_misc ();
  return 0;
}
/* distcc preprocessing test */
const char *test_distcc_strings = "ÿÿÿÿ";
/* vim:set ts=8 sts=2 sw=2: */
