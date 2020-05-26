// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sfistore.hh"
#include "sfiprimitives.hh"
#include "sfiserial.hh"
#include "sfiparams.hh"
#include "path.hh"
#include "internal.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>



/* --- structures --- */
typedef struct {
  SfiStoreReadBin reader;
  gpointer	  data;
  GDestroyNotify  destroy;
  off_t		  patch_offset;
  off_t		  offset, length;
} BBlock;


/* --- writable store --- */
SfiWStore*
sfi_wstore_new (void)
{
  SfiWStore *wstore = g_new0 (SfiWStore, 1);
  wstore->text = g_string_new (NULL);
  wstore->indent = 0;
  wstore->bblocks = NULL;
  wstore->comment_start = ';';
  return wstore;
}

void
sfi_wstore_destroy (SfiWStore *wstore)
{
  assert_return (wstore != NULL);

  g_string_free (wstore->text, TRUE);
  wstore->text = NULL;
  while (wstore->bblocks)
    {
      BBlock *bblock = (BBlock*) sfi_ring_pop_head (&wstore->bblocks);
      if (bblock->destroy)
        bblock->destroy (bblock->data);
      g_free (bblock);
    }
  g_free (wstore);
}

static inline void
sfi_wstore_text_changed (SfiWStore *wstore)
{
  wstore->needs_break = wstore->text->len && wstore->text->str[wstore->text->len - 1] != '\n';
}

void
sfi_wstore_break (SfiWStore *wstore)
{
  assert_return (wstore != NULL);

  if (wstore->needs_break)
    {
      guint n;
      g_string_append_c (wstore->text, '\n');
      /* don't interpret indentation text as needs-break */
      sfi_wstore_text_changed (wstore);
      for (n = 0; n < wstore->indent; n += 2)
	g_string_append (wstore->text, "  ");
    }
}

void
sfi_wstore_push_level (SfiWStore *wstore)
{
  assert_return (wstore != NULL);

  wstore->indent += 2;
}

void
sfi_wstore_pop_level (SfiWStore *wstore)
{
  assert_return (wstore != NULL);

  if (wstore->indent >= 2)
    wstore->indent -= 2;
}

void
sfi_wstore_puts (SfiWStore   *wstore,
		 const gchar *string)
{
  assert_return (wstore != NULL);

  if (string)
    {
      g_string_append (wstore->text, string);
      if (string[0])
        sfi_wstore_text_changed (wstore);
    }
}

void
sfi_wstore_putc (SfiWStore *wstore,
		 gchar	    character)
{
  assert_return (wstore != NULL);

  g_string_append_c (wstore->text, character);
  sfi_wstore_text_changed (wstore);
}

void
sfi_wstore_putf (SfiWStore      *wstore,
                 gfloat          vfloat)
{
  gchar numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = "";

  assert_return (wstore != NULL);

  g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.7g", vfloat);

  sfi_wstore_puts (wstore, numbuf);
}

void
sfi_wstore_putd (SfiWStore      *wstore,
                 gdouble         vdouble)
{
  gchar numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = "";

  assert_return (wstore != NULL);

  g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.17g", vdouble);

  sfi_wstore_puts (wstore, numbuf);
}

void
sfi_wstore_put_param (SfiWStore	   *wstore,
		      const GValue *value,
		      GParamSpec   *pspec)
{
  GValue svalue = { 0, };
  GParamSpec *spspec;

  assert_return (wstore != NULL);
  assert_return (G_IS_VALUE (value));
  assert_return (G_IS_PARAM_SPEC (pspec));

  spspec = sfi_pspec_to_serializable (pspec);
  if (!spspec)          /* we really can't do anything here */
    {
      Bse::warning ("unable to (de-)serialize \"%s\" of type `%s'", pspec->name,
                    g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      return;
    }

  g_value_init (&svalue, G_PARAM_SPEC_VALUE_TYPE (spspec));
  if (sfi_value_transform (value, &svalue))
    {
      GString *gstring = g_string_new (NULL);
      if (g_param_value_validate (spspec, &svalue))
	{
	  if (G_VALUE_TYPE (&svalue) != G_VALUE_TYPE (value))
	    Bse::info ("fixing up value for \"%s\" of type `%s' (converted from `%s')",
                       pspec->name, g_type_name (G_VALUE_TYPE (&svalue)),
                       g_type_name (G_VALUE_TYPE (value)));
	  else
	    Bse::info ("fixing up value for \"%s\" of type `%s'",
                       pspec->name, g_type_name (G_VALUE_TYPE (&svalue)));
	}
      sfi_value_store_param (&svalue, gstring, spspec, wstore->indent);
      sfi_wstore_break (wstore);
      sfi_wstore_puts (wstore, gstring->str);
      g_string_free (gstring, TRUE);
    }
  else
    Bse::warning ("unable to transform \"%s\" of type `%s' to `%s'",
                  pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
                  g_type_name (G_VALUE_TYPE (&svalue)));
  g_value_unset (&svalue);
  g_param_spec_unref (spspec);
}

void
sfi_wstore_put_binary (SfiWStore      *wstore,
		       SfiStoreReadBin reader,
		       gpointer	       data,
		       GDestroyNotify  destroy)
{
  BBlock *bblock;

  assert_return (wstore != NULL);
  assert_return (wstore->flushed == FALSE);
  assert_return (reader != NULL);

  bblock = g_new0 (BBlock, 1);
  bblock->reader = reader;
  bblock->data = data;
  bblock->destroy = destroy;
  wstore->bblocks = sfi_ring_append (wstore->bblocks, bblock);

  sfi_wstore_puts (wstore, "(binary-appendix ");
  bblock->patch_offset = wstore->text->len;
  sfi_wstore_puts (wstore, "0x00000000 0x00000000)");
}

const gchar*
sfi_wstore_peek_text (SfiWStore      *wstore,
                      guint          *length_p)
{
  assert_return (wstore != NULL, NULL);

  if (length_p)
    *length_p = wstore->text->len;

  return wstore->text->str;
}

gint /* -errno */
sfi_wstore_flush_fd (SfiWStore *wstore,
		     gint      fd)
{
  guint8 buffer[8192] = { 0, };
  const guint bsize = sizeof (buffer);
  SfiRing *ring;
  off_t text_offset, binary_offset;
  guint l;

  assert_return (wstore != NULL, -EINVAL);
  assert_return (wstore->flushed == FALSE, -EINVAL);
  assert_return (fd >= 0, -EINVAL);

  wstore->flushed = TRUE;

  sfi_wstore_break (wstore);

  /* save text offset */
  do
    text_offset = lseek (fd, 0, SEEK_CUR);
  while (text_offset < 0 && errno == EINTR);
  if (text_offset < 0 && errno)
    return -errno;

  /* dump text */
  do
    l = write (fd, wstore->text->str, wstore->text->len);
  while (l < 0 && errno == EINTR);
  if (l < 0 && errno)
    return -errno;

  /* binary data header */
  if (wstore->bblocks)
    {
      gchar term[] = "\nX binary appendix:\n";
      guint n = strlen (term) + 1;
      term[1] = wstore->comment_start;
      do
	l = write (fd, term, n);
      while (l < 0 && errno == EINTR);
      if (l < 0 && errno)
        return -errno;
    }

  /* save binary offset */
  do
    binary_offset = lseek (fd, 0, SEEK_CUR);
  while (binary_offset < 0 && errno == EINTR);
  /* binary_offset is position of the first byte *after* \000 */
  if (binary_offset < 0 && errno)
    return -errno;

  /* store binary data */
  for (ring = wstore->bblocks; ring; ring = sfi_ring_walk (ring, wstore->bblocks))
    {
      BBlock *bblock = (BBlock*) ring->data;
      int n;

      /* save block offset */
      do
	bblock->offset = lseek (fd, 0, SEEK_CUR);
      while (bblock->offset < 0 && errno == EINTR);
      bblock->length = 0;
      if (bblock->offset < 0 && errno)
        return -errno;

      /* dump binary */
      do
	{
	  n = bblock->reader (bblock->data, buffer, bsize);
	  if (n < 0)
	    break;	// FIXME: error handling
	  assert_return (n <= int (bsize), -EINVAL);
	  do
	    l = write (fd, buffer, n);
	  while (l < 0 && errno == EINTR);
	  bblock->length += n;
          if (l < 0 && errno)
            return -errno;
	}
      while (n);
    }

  /* patch binary offsets and lengths */
  for (ring = wstore->bblocks; ring; ring = sfi_ring_walk (ring, wstore->bblocks))
    {
      BBlock *bblock = (BBlock*) ring->data;
      off_t foff;

      do
	foff = lseek (fd, text_offset + bblock->patch_offset, SEEK_SET);
      while (foff < 0 && errno == EINTR);
      if (foff < 0 && errno)
        return -errno;
      std::string str = Bse::string_format ("0x%08x 0x%08x", guint32 (bblock->offset - binary_offset), guint32 (bblock->length));
      do
	l = write (fd, str.data(), str.size());
      while (l < 0 && errno == EINTR);
      if (l < 0 && errno)
        return -errno;
    }

  /* finished successfully */
  return 0;
}


/* --- readable store --- */
SfiRStore*
sfi_rstore_new (void)
{
  SfiRStore *rstore;

  rstore = g_new0 (SfiRStore, 1);
  rstore->scanner = g_scanner_new64 (sfi_storage_scanner_config);
  rstore->scanner->max_parse_errors = 1;
  rstore->fname = NULL;
  rstore->parser_this = rstore;
  rstore->close_fd = -1;
  rstore->bin_offset = -1;

  return rstore;
}

SfiRStore*
sfi_rstore_new_open (const gchar *fname)
{
  size_t length = 0;
  char *text = Bse::Path::memread (fname, &length);
  if (!text || length < 1)
    return NULL; // pass errno
  SfiRStore *rstore = sfi_rstore_new ();
  rstore->fname = g_strdup (fname);
  rstore->scanner->input_name = rstore->fname;
  rstore->scanner->parse_errors = 0;
  rstore->textstart_ = text;
  g_scanner_input_text (rstore->scanner, text, length);
  return rstore;
}

void
sfi_rstore_destroy (SfiRStore *rstore)
{
  assert_return (rstore != NULL);

  if (rstore->close_fd >= 0)
    close (rstore->close_fd);
  g_scanner_destroy (rstore->scanner);
  if (rstore->textstart_)
    Bse::Path::memfree (rstore->textstart_);
  g_free (rstore->fname);
  g_free (rstore);
}

void
sfi_rstore_input_text (SfiRStore   *rstore,
		       const gchar *text,
                       const gchar *text_name)
{
  assert_return (rstore != NULL);
  assert_return (text != NULL);

  g_free (rstore->fname);
  rstore->fname = g_strdup (text_name ? text_name : "<memory>");
  rstore->scanner->input_name = rstore->fname;
  rstore->scanner->parse_errors = 0;
  g_scanner_input_text (rstore->scanner, text, strlen (text));
}

gboolean
sfi_rstore_eof (SfiRStore *rstore)
{
  GScanner *scanner;

  assert_return (rstore != NULL, TRUE);

  scanner = rstore->scanner;

  return g_scanner_eof (scanner) || scanner->parse_errors >= scanner->max_parse_errors;
}

void
sfi_rstore_error (SfiRStore *rstore, const std::string &msg)
{
  assert_return (rstore);

  if (rstore->scanner->parse_errors < rstore->scanner->max_parse_errors)
    g_scanner_error (rstore->scanner, "%s", msg.c_str());
}

void
sfi_rstore_unexp_token (SfiRStore *rstore,
			GTokenType expected_token)
{
  GScanner *scanner;

  assert_return (rstore);

  scanner = rstore->scanner;
  if (scanner->parse_errors < scanner->max_parse_errors)
    {
      const char *message;

      if (scanner->parse_errors + 1 >= scanner->max_parse_errors)
        message = "aborting...";
      else
        message = NULL;
      g_scanner_unexp_token (scanner, expected_token, NULL, NULL, NULL, message, TRUE);
    }
}

void
sfi_rstore_warn (SfiRStore *rstore, const std::string &msg)
{
  assert_return (rstore);

  if (rstore->scanner->parse_errors < rstore->scanner->max_parse_errors)
    g_scanner_warn (rstore->scanner, "%s", msg.c_str());
}

static GTokenType
scanner_skip_statement (GScanner *scanner,
			guint     level) /* == number of closing parens left to read */
{
  assert_return (scanner != NULL, G_TOKEN_ERROR);
  assert_return (level > 0, G_TOKEN_ERROR);

  do
    {
      g_scanner_get_next_token (scanner);
      switch (scanner->token)
	{
	case G_TOKEN_EOF:
	case G_TOKEN_ERROR:
	  return GTokenType (')');
	case '(':
	  level++;
	  break;
	case ')':
	  level--;
	  break;
	default:
	  break;
	}
    }
  while (level);
  return G_TOKEN_NONE;
}

GTokenType
sfi_rstore_warn_skip (SfiRStore *rstore, const std::string &msg)
{
  assert_return (rstore, G_TOKEN_ERROR);

  if (rstore->scanner->parse_errors < rstore->scanner->max_parse_errors)
    /* construct warning *before* modifying scanner state */
    g_scanner_warn (rstore->scanner, "%s - skipping...", msg.c_str());

  return scanner_skip_statement (rstore->scanner, 1);
}

GTokenType
sfi_rstore_parse_param (SfiRStore  *rstore,
			GValue     *value,
			GParamSpec *pspec)
{
  GParamSpec *spspec;
  GValue pvalue = { 0, };
  GTokenType token;

  assert_return (rstore != NULL, G_TOKEN_ERROR);
  assert_return (G_IS_VALUE (value), G_TOKEN_ERROR);
  assert_return (G_IS_PARAM_SPEC (pspec), G_TOKEN_ERROR);

  spspec = sfi_pspec_to_serializable (pspec);
  if (!spspec)          /* we really can't do anything here */
    {
      Bse::warning ("unable to (de-)serialize \"%s\" of type `%s'", pspec->name,
                    g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      return G_TOKEN_ERROR;
    }

  token = sfi_value_parse_param_rest (&pvalue, rstore->scanner, spspec);
  if (token == G_TOKEN_NONE)
    {
      if (sfi_value_transform (&pvalue, value))
	{
	  if (g_param_value_validate (pspec, value))
	    {
	      if (G_VALUE_TYPE (&pvalue) != G_VALUE_TYPE (value))
		sfi_rstore_warn (rstore,
                                 Bse::string_format ("fixing up value for \"%s\" of type `%s' (converted from `%s')",
                                                     pspec->name, g_type_name (G_VALUE_TYPE (value)),
                                                     g_type_name (G_VALUE_TYPE (&pvalue))));
	      else
		sfi_rstore_warn (rstore,
                                 Bse::string_format ("fixing up value for \"%s\" of type `%s'",
                                                     pspec->name, g_type_name (G_VALUE_TYPE (value))));
	    }
	}
      else
	{
	  Bse::warning ("unable to transform \"%s\" of type `%s' to `%s'",
                        pspec->name, g_type_name (G_VALUE_TYPE (&pvalue)),
                        g_type_name (G_VALUE_TYPE (value)));
	  return G_TOKEN_ERROR;
	}
      g_value_unset (&pvalue);
    }
  g_param_spec_unref (spspec);
  return token;
}

static gboolean
rstore_ensure_bin_offset (SfiRStore *rstore)
{
  if (rstore->bin_offset < 0)
    {
      assert_return (rstore->scanner->input_fd < 0 && rstore->textstart_, false); // input is always in memory
      // find first literal '\0'
      const char *p = (const char*) memchr (rstore->textstart_, 0, rstore->scanner->text_end - rstore->textstart_);
      const bool seen_zero = p != NULL;
      if (!seen_zero)
	return false;
      const off_t zero_offset = p - rstore->textstart_;
      rstore->bin_offset = zero_offset + 1;
    }
  return true;
}

GTokenType
sfi_rstore_ensure_bin_offset (SfiRStore *rstore)
{
  assert_return (rstore != NULL, G_TOKEN_ERROR);

  if (!rstore_ensure_bin_offset (rstore))
    {
      /* this is _bad_, we can't actually continue parsing after
       * here because the read position is probably screwed
       */
      sfi_rstore_error (rstore, "failed to detect binary appendix");
      return G_TOKEN_ERROR;
    }
  return G_TOKEN_NONE;
}

guint64
sfi_rstore_get_bin_offset (SfiRStore *rstore)
{
  assert_return (rstore != NULL, 0);
  assert_return (rstore->bin_offset >= 0, 0);    /* sfi_rstore_ensure_bin_offset() must be called before hand */

  return rstore->bin_offset;
}

GTokenType
sfi_rstore_parse_zbinary (SfiRStore *rstore,
                          SfiNum    *offset_p,
                          SfiNum    *length_p)
{
  assert_return (rstore != NULL, G_TOKEN_ERROR);
  assert_return (offset_p && length_p, G_TOKEN_ERROR);

  if (g_scanner_get_next_token (rstore->scanner) != '(')
    return GTokenType ('(');
  if (g_scanner_get_next_token (rstore->scanner) != G_TOKEN_IDENTIFIER ||
      strcmp (rstore->scanner->value.v_identifier, "binary-appendix") != 0)
    return G_TOKEN_IDENTIFIER;
  if (g_scanner_get_next_token (rstore->scanner) != G_TOKEN_INT)
    return G_TOKEN_INT;
  SfiNum offset = rstore->scanner->value.v_int64;
  if (g_scanner_get_next_token (rstore->scanner) != G_TOKEN_INT)
    return G_TOKEN_INT;
  SfiNum length = rstore->scanner->value.v_int64;
  if (g_scanner_get_next_token (rstore->scanner) != ')')
    return GTokenType (')');
  *offset_p = offset;
  *length_p = length;
  return G_TOKEN_NONE;
}

GTokenType
sfi_rstore_parse_binary (SfiRStore *rstore,
			 SfiNum    *offset_p,
			 SfiNum    *length_p)
{
  GTokenType token = sfi_rstore_ensure_bin_offset (rstore);
  if (token != G_TOKEN_NONE)
    return token;
  token = sfi_rstore_parse_zbinary (rstore, offset_p, length_p);
  if (token != G_TOKEN_NONE)
    return token;
  *offset_p += rstore->bin_offset;
  return G_TOKEN_NONE;
}

GTokenType
sfi_rstore_parse_until (SfiRStore     *rstore,
                        GTokenType     closing_token,
                        gpointer       context_data,
                        SfiStoreParser try_statement,
                        gpointer       user_data)
{
  GScanner *scanner;

  assert_return (rstore != NULL, G_TOKEN_ERROR);
  assert_return (try_statement != NULL, G_TOKEN_ERROR);
  assert_return (closing_token == G_TOKEN_EOF || closing_token == ')', G_TOKEN_ERROR);

  scanner = rstore->scanner;

  /* we catch all SFI_TOKEN_UNMATCHED at this level. it is merely
   * a "magic" token value to implement the try_statement() semantics
   */
  while (!sfi_rstore_eof (rstore) && g_scanner_get_next_token (scanner) == '(')
    {
      GTokenType expected_token;
      uint saved_line, saved_position;

      /* it is only useful to feature statements which
       * start out with an identifier (syntactically)
       */
      if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
        {
          /* eat token and bail out */
          g_scanner_get_next_token (scanner);
          return G_TOKEN_IDENTIFIER;
        }
      /* parse a statement (may return SFI_TOKEN_UNMATCHED) */
      saved_line = scanner->line;
      saved_position = scanner->position;
      expected_token = (GTokenType) try_statement (context_data, (SfiRStore*) rstore->parser_this, scanner, user_data);
      /* if there are no matches, skip statement */
      if (expected_token == GTokenType (SFI_TOKEN_UNMATCHED))
        {
          if (saved_line != scanner->line || saved_position != scanner->position ||
              scanner->next_token != G_TOKEN_IDENTIFIER)
            {
              Bse::warning ("((SfiStoreParser)%p) advanced scanner for unmatched token", try_statement);
              return G_TOKEN_ERROR;
            }
          expected_token = sfi_rstore_warn_skip (rstore,
                                                 Bse::string_format ("unknown identifier: %s",
                                                                     scanner->next_value.v_identifier));
        }
      /* bail out on errors */
      if (expected_token != G_TOKEN_NONE)
        return expected_token;
    }

  return scanner->token == closing_token ? G_TOKEN_NONE : closing_token;
}

guint
sfi_rstore_parse_all (SfiRStore     *rstore,
		      gpointer       context_data,
		      SfiStoreParser try_statement,
		      gpointer       user_data)
{
  GTokenType expected_token = G_TOKEN_NONE;

  assert_return (rstore != NULL, 1);
  assert_return (try_statement != NULL, 1);

  /* parse all statements */
  expected_token = sfi_rstore_parse_until (rstore, G_TOKEN_EOF, context_data, try_statement, user_data);

  /* report error if any */
  if (expected_token != G_TOKEN_NONE)
    sfi_rstore_unexp_token (rstore, expected_token);

  return rstore->scanner->parse_errors;
}

// == Testing ==
#include "testing.hh"
namespace { // Anon
using namespace Bse;

#define SCANNER_ASSERT64(scanner, needprint, token, text, svalue) { \
  g_scanner_input_text (scanner, text, strlen (text)); \
  TASSERT (g_scanner_get_next_token (scanner) == token); \
  if (needprint) printout ("{scanner.v_int64:%llu}", (long long unsigned int) (scanner->value.v_int64)); \
  TASSERT (scanner->value.v_int64 == svalue); \
  TASSERT (g_scanner_get_next_token (scanner) == '#'); \
}
#define SCANNER_ASSERTf(scanner, needprint, vtoken, text, svalue) { \
  g_scanner_input_text (scanner, text, strlen (text)); \
  if (g_scanner_get_next_token (scanner) != vtoken) \
    g_scanner_unexp_token (scanner, vtoken, NULL, NULL, NULL, NULL, TRUE); \
  TASSERT (scanner->token == vtoken); \
  if (needprint) printout ("{scanner.v_float:%17g}", scanner->value.v_float); \
  TASSERT (scanner->value.v_float == svalue); \
  TASSERT (g_scanner_get_next_token (scanner) == '#'); \
}

BSE_INTEGRITY_TEST (bse_test_scanner64);
static void
bse_test_scanner64 (void)
{
  GScanner *scanner = g_scanner_new64 (sfi_storage_scanner_config);
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
}

} // Anon
