/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2003 Tim Janik
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
#include "gslloader.h"

#include "gsldatahandle.h"
#include "gsldatahandle-vorbis.h"
#include "bsemath.h"
#include <sfi/sfistore.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#define DEBUG(...)      sfi_debug ("bsewave", __VA_ARGS__)

#define parse_or_return(scanner, token) { guint _t = (token); \
                                          if (g_scanner_get_next_token (scanner) != _t) \
                                            return _t; \
                                        }

/* --- inline loader types --- */
#define RAWLINK_MAGIC           (('R' << 24) | ('a' << 16) | ('w' << 8) | 'L')
#define OGGLINK_MAGIC           (('O' << 24) | ('g' << 16) | ('g' << 8) | 'L')


/* --- token types --- */
typedef enum
{
  /* wave tokens */
  GSL_WAVE_TOKEN_WAVE           = 512,
  GSL_WAVE_TOKEN_CHUNK,
  GSL_WAVE_TOKEN_NAME,
  GSL_WAVE_TOKEN_AUTHORS,
  GSL_WAVE_TOKEN_LICENSE,
  GSL_WAVE_TOKEN_COMMENT,
  GSL_WAVE_TOKEN_BYTE_ORDER,
  GSL_WAVE_TOKEN_FORMAT,
  GSL_WAVE_TOKEN_N_CHANNELS,
  GSL_WAVE_TOKEN_MIX_FREQ,
  GSL_WAVE_TOKEN_OSC_FREQ,
  GSL_WAVE_TOKEN_MIDI_NOTE,
  GSL_WAVE_TOKEN_LABEL,
  GSL_WAVE_TOKEN_VOLUME,
  GSL_WAVE_TOKEN_BALANCE,
  GSL_WAVE_TOKEN_XINFO,
  GSL_WAVE_TOKEN_FILE,
  GSL_WAVE_TOKEN_INDEX,
  GSL_WAVE_TOKEN_RAWLINK,
  GSL_WAVE_TOKEN_OGGLINK,
  GSL_WAVE_TOKEN_BOFFSET,
  GSL_WAVE_TOKEN_N_VALUES,
  GSL_WAVE_TOKEN_LOOP_TYPE,
  GSL_WAVE_TOKEN_LOOP_START,
  GSL_WAVE_TOKEN_LOOP_END,
  GSL_WAVE_TOKEN_LOOP_COUNT,
  GSL_WAVE_TOKEN_LAST_FIELD,
  /* data tokens */
  GSL_WAVE_TOKEN_BIG_ENDIAN     = 768,
  GSL_WAVE_TOKEN_BIG,
  GSL_WAVE_TOKEN_LITTLE_ENDIAN,
  GSL_WAVE_TOKEN_LITTLE,
  GSL_WAVE_TOKEN_SIGNED_8,
  GSL_WAVE_TOKEN_SIGNED_12,
  GSL_WAVE_TOKEN_SIGNED_16,
  GSL_WAVE_TOKEN_UNSIGNED_8,
  GSL_WAVE_TOKEN_UNSIGNED_12,
  GSL_WAVE_TOKEN_UNSIGNED_16,
  GSL_WAVE_TOKEN_ALAW,
  GSL_WAVE_TOKEN_ULAW,
  GSL_WAVE_TOKEN_FLOAT,
  GSL_WAVE_TOKEN_NONE,
  GSL_WAVE_TOKEN_JUMP,
  GSL_WAVE_TOKEN_PINGPONG,
  GSL_WAVE_TOKEN_LAST_DATA
} GslWaveTokenType;


/* --- structures --- */
typedef struct
{
  GslWaveFileInfo wfi;
  gchar          *cwd;
} FileInfo;

typedef struct
{
  GslWaveDsc        wdsc;
  GslWaveFormatType format;
  guint		    byte_order;
  gfloat	    dfl_mix_freq;
} WaveDsc;


/* --- tokens --- */
static const char *wave_tokens_512[] = {
  "wave",       "chunk",
  "name",       "author",       "license",      "info",
  "byte_order",
  "format",     "n_channels",   "mix_freq",     "osc_freq",
  "midi_note",  "label",        "volume",       "balance",
  "xinfo",
  "file",       "index",	"rawlink",
  "ogglink",    "boffset",	"n_values",
  "loop_type",	"loop_start",   "loop_end",	"loop_count",
};
static const char *wave_tokens_768[] = {
  "big-endian", "big",          "little-endian", "little",
  "signed-8",   "signed-12",    "signed-16",
  "unsigned-8", "unsigned-12",  "unsigned-16",
  "alaw",       "ulaw",         "float",
  "none",	"jump",		"pingpong",
};


/* --- functions --- */
static const gchar*
gsl_wave_token (GslWaveTokenType token)
{
  if (token >= 768)
    {
      token -= 768;
      return token >= sizeof (wave_tokens_768) / sizeof (wave_tokens_768[0]) ? NULL : wave_tokens_768[token];
    }
  else
    {
      token -= 512;
      return token >= sizeof (wave_tokens_512) / sizeof (wave_tokens_512[0]) ? NULL : wave_tokens_512[token];
    }
}

static GTokenType
gslwave_skip_rest_statement (GScanner *scanner,
			     guint     level)
{
  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);

  while (level)
    {
      g_scanner_get_next_token (scanner);
      switch (scanner->token)
	{
	case G_TOKEN_EOF: case G_TOKEN_ERROR:                   return '}';
	case '(': case '{': case '[':           level++;        break;
	case ')': case '}': case ']':           level--;        break;
	default:                                                break;
	}
    }

  return G_TOKEN_NONE;
}

static GslWaveFileInfo*
gslwave_load_file_info (gpointer      data,
			const gchar  *_file_name,
			BseErrorType *error_p)
{
  FileInfo *fi = NULL;
  gboolean in_wave = FALSE, abort = FALSE;
  SfiRing *wave_names = NULL;
  GScanner *scanner;
  gchar *cwd, *file_name;
  gint fd;
  guint i;

  if (g_path_is_absolute (_file_name))
    {
      gchar *p = strrchr (_file_name, G_DIR_SEPARATOR);

      g_assert (p != NULL);
      cwd = g_strndup (_file_name, p - _file_name + 1);
      file_name = g_strdup (_file_name);
    }
  else
    {
      cwd = g_get_current_dir ();
      file_name = g_strdup_printf ("%s%c%s", cwd, G_DIR_SEPARATOR, _file_name);
    }

  fd = open (file_name, O_RDONLY);
  if (fd < 0)
    {
      *error_p = gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
      g_free (cwd);
      g_free (file_name);
      return NULL;
    }

  scanner = g_scanner_new64 (sfi_storage_scanner_config);
  scanner->config->cpair_comment_single = "#\n";
  g_scanner_scope_add_symbol (scanner, 0, "wave", GUINT_TO_POINTER (GSL_WAVE_TOKEN_WAVE));
  g_scanner_scope_add_symbol (scanner, 0, "name", GUINT_TO_POINTER (GSL_WAVE_TOKEN_NAME));
  g_scanner_input_file (scanner, fd);
  while (!abort)
    {
      g_scanner_get_next_token (scanner);
      switch (scanner->token)
	{
	case GSL_WAVE_TOKEN_WAVE:
	  if (g_scanner_peek_next_token (scanner) == '{')
	    {
	      g_scanner_get_next_token (scanner); /* eat '{' */
	      in_wave = TRUE;
	    }
	  break;
	case '{':
	  if (gslwave_skip_rest_statement (scanner, 1) != G_TOKEN_NONE)
	    abort = TRUE;
	  break;
	case GSL_WAVE_TOKEN_NAME:
	  if (in_wave && g_scanner_peek_next_token (scanner) == '=')
	    {
	      g_scanner_get_next_token (scanner); /* eat '=' */
	      if (g_scanner_peek_next_token (scanner) == G_TOKEN_STRING)
		{
		  gchar *wave_name;
		  
		  g_scanner_get_next_token (scanner); /* eat string */
		  wave_name = g_strdup (scanner->value.v_string);
		  if (gslwave_skip_rest_statement (scanner, 1) == G_TOKEN_NONE)
		    {
		      in_wave = FALSE;
		      wave_names = sfi_ring_append (wave_names, wave_name);
		    }
		  else
		    {
		      g_free (wave_name);
		      abort = TRUE;
		    }
		}
	    }
	  break;
	default:
	  if (scanner->token == G_TOKEN_EOF || scanner->token == G_TOKEN_ERROR)
	    abort = TRUE;
	  break;
	}
    }
  g_scanner_destroy (scanner);
  close (fd);

  if (wave_names)
    {
      SfiRing *ring;

      fi = sfi_new_struct0 (FileInfo, 1);
      fi->wfi.n_waves = sfi_ring_length (wave_names);
      fi->wfi.waves = g_malloc0 (sizeof (fi->wfi.waves[0]) * fi->wfi.n_waves);
      for (i = 0, ring = wave_names; i < fi->wfi.n_waves; i++, ring = ring->next)
	fi->wfi.waves[i].name = ring->data;
      sfi_ring_free (wave_names);
      fi->cwd = cwd;
    }
  else
    g_free (cwd);
  g_free (file_name);

  /* FIXME: empty wave error? */

  return fi ? &fi->wfi : NULL;
}

static void
gslwave_free_file_info (gpointer         data,
			GslWaveFileInfo *file_info)
{
  FileInfo *fi = (FileInfo*) file_info;
  guint i;

  for (i = 0; i < fi->wfi.n_waves; i++)
    g_free (fi->wfi.waves[i].name);
  g_free (fi->wfi.waves);
  g_free (fi->cwd);
  sfi_delete_struct (FileInfo, fi);
}

static guint
gslwave_parse_chunk_dsc (GScanner        *scanner,
			 GslWaveChunkDsc *chunk)
{
  gboolean negate = FALSE;
  parse_or_return (scanner, '{');
  do
    switch (g_scanner_get_next_token (scanner))
      {
        double dvalue;
        gchar *key;
      case '}':
	return G_TOKEN_NONE;
      default:
	return '}';
      case GSL_WAVE_TOKEN_FILE:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
	g_free (chunk->loader_data1);	/* file_name */
	chunk->loader_data1 = g_strdup (scanner->value.v_string);
	break;
      case GSL_WAVE_TOKEN_INDEX:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
	g_free (chunk->loader_data2);	/* wave_name */
	chunk->loader_data2 = g_strdup (scanner->value.v_string);
	break;
      case GSL_WAVE_TOKEN_RAWLINK:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, '(');
	parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	if (strcmp (scanner->value.v_identifier, "binary-appendix") != 0)
	  return G_TOKEN_IDENTIFIER;
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->loader_offset = scanner->value.v_int64; /* byte offset */
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->loader_length = scanner->value.v_int64; /* byte length */
	parse_or_return (scanner, ')');
	g_free (chunk->loader_data1);	/* file_name */
	chunk->loader_data1 = NULL;
        chunk->loader_num1 = RAWLINK_MAGIC;
	break;
      case GSL_WAVE_TOKEN_OGGLINK:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, '(');
	parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	if (strcmp (scanner->value.v_identifier, "binary-appendix") != 0)
	  return G_TOKEN_IDENTIFIER;
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->loader_offset = scanner->value.v_int64; /* byte offset */
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->loader_length = scanner->value.v_int64; /* byte length */
	parse_or_return (scanner, ')');
	g_free (chunk->loader_data1);	/* file_name */
	chunk->loader_data1 = NULL;
        chunk->loader_num1 = OGGLINK_MAGIC;
	break;
      case GSL_WAVE_TOKEN_MIX_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:	chunk->mix_freq = scanner->value.v_float;	break;
	  case G_TOKEN_INT:	chunk->mix_freq = scanner->value.v_int64;	break;
	  default:		return G_TOKEN_FLOAT;
	  }
	break;
      case GSL_WAVE_TOKEN_OSC_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:	chunk->osc_freq = scanner->value.v_float;	break;
	  case G_TOKEN_INT:	chunk->osc_freq = scanner->value.v_int64;	break;
	  default:		return G_TOKEN_FLOAT;
	  }
	break;
      case GSL_WAVE_TOKEN_MIDI_NOTE:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->osc_freq = bse_temp_freq (gsl_get_config ()->kammer_freq,
					 scanner->value.v_int64 - gsl_get_config ()->midi_kammer_note);
        chunk->xinfos = bse_xinfos_add_num (chunk->xinfos, "midi-note", scanner->value.v_int64);
	break;
      case GSL_WAVE_TOKEN_LABEL:
        parse_or_return (scanner, '=');
        parse_or_return (scanner, G_TOKEN_STRING);
        DEBUG ("ignoring: label=\"%s\"", scanner->value.v_string);
        break;
      case GSL_WAVE_TOKEN_VOLUME:
        parse_or_return (scanner, '=');
        if (g_scanner_peek_next_token (scanner) == '-')
          {
            g_scanner_get_next_token (scanner);
            negate = TRUE;
          }
        switch (g_scanner_get_next_token (scanner))
          {
          case G_TOKEN_FLOAT:   dvalue = scanner->value.v_float;        break;
          case G_TOKEN_INT:     dvalue = scanner->value.v_int64;        break;
          default:              return G_TOKEN_FLOAT;
          }
        if (negate)
          dvalue = -dvalue;
        DEBUG ("ignoring: volume=%f", dvalue);
        break;
      case GSL_WAVE_TOKEN_BALANCE:
        parse_or_return (scanner, '=');
        if (g_scanner_peek_next_token (scanner) == '-')
          {
            g_scanner_get_next_token (scanner);
            negate = TRUE;
          }
        switch (g_scanner_get_next_token (scanner))
          {
          case G_TOKEN_FLOAT:   dvalue = scanner->value.v_float;        break;
          case G_TOKEN_INT:     dvalue = scanner->value.v_int64;        break;
          default:              return G_TOKEN_FLOAT;
          }
        if (negate)
          dvalue = -dvalue;
        DEBUG ("ignoring: balance=%f", dvalue);
        break;
      case GSL_WAVE_TOKEN_XINFO:
        parse_or_return (scanner, '[');
        parse_or_return (scanner, G_TOKEN_STRING);
        key = g_strdup (scanner->value.v_string);
        if (g_scanner_peek_next_token (scanner) != ']')
          g_free (key);
        parse_or_return (scanner, ']');
        if (g_scanner_peek_next_token (scanner) != '=')
          g_free (key);
        parse_or_return (scanner, '=');
        if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
          g_free (key);
        parse_or_return (scanner, G_TOKEN_STRING);
        chunk->xinfos = bse_xinfos_add_value (chunk->xinfos, key, scanner->value.v_string);
        g_free (key);
        break;
      case GSL_WAVE_TOKEN_BOFFSET:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->loader_offset = scanner->value.v_int64;	/* byte_offset */
	break;
      case GSL_WAVE_TOKEN_N_VALUES:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->loader_length = scanner->value.v_int64;	/* n_values */
	break;
      case GSL_WAVE_TOKEN_LOOP_TYPE:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case GSL_WAVE_TOKEN_PINGPONG:
            chunk->xinfos = bse_xinfos_add_value (chunk->xinfos, "loop-type", gsl_wave_loop_type_to_string (GSL_WAVE_LOOP_PINGPONG));
            break;
	  case GSL_WAVE_TOKEN_JUMP:
            chunk->xinfos = bse_xinfos_add_value (chunk->xinfos, "loop-type", gsl_wave_loop_type_to_string (GSL_WAVE_LOOP_JUMP));
            break;
	  case GSL_WAVE_TOKEN_NONE:
	  default:
            break;
	  }
	break;
      case GSL_WAVE_TOKEN_LOOP_START:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
        chunk->xinfos = bse_xinfos_add_num (chunk->xinfos, "loop-start", scanner->value.v_int64);
	break;
      case GSL_WAVE_TOKEN_LOOP_END:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
        chunk->xinfos = bse_xinfos_add_num (chunk->xinfos, "loop-end", scanner->value.v_int64);
	break;
      case GSL_WAVE_TOKEN_LOOP_COUNT:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
        chunk->xinfos = bse_xinfos_add_num (chunk->xinfos, "loop-count", scanner->value.v_int64);
	break;
      }
  while (TRUE);
}

static guint
gslwave_parse_wave_dsc (GScanner    *scanner,
			WaveDsc     *dsc,
			const gchar *wave_name)
{
  gboolean negate = FALSE;
  parse_or_return (scanner, '{');
  do
    switch (g_scanner_get_next_token (scanner))
      {
        gdouble dvalue;
	guint i, token;
      case '}':
	return G_TOKEN_NONE;
      default:
	return '}';
      case GSL_WAVE_TOKEN_NAME:
	if (dsc->wdsc.name)
	  return '}';
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
	if (wave_name)
	  {
	    if (strcmp (wave_name, scanner->value.v_string) == 0)
	      dsc->wdsc.name = g_strdup (scanner->value.v_string);
	    else
	      return gslwave_skip_rest_statement (scanner, 1);
	  }
	else
	  dsc->wdsc.name = g_strdup (scanner->value.v_string);
	break;
      case GSL_WAVE_TOKEN_AUTHORS:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
        DEBUG ("ignore: authors=\"%s\"", scanner->value.v_string);
        break;
      case GSL_WAVE_TOKEN_LICENSE:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
        DEBUG ("ignore: license=\"%s\"", scanner->value.v_string);
        break;
      case GSL_WAVE_TOKEN_COMMENT:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
        DEBUG ("ignore: comment=\"%s\"", scanner->value.v_string);
        break;
      case GSL_WAVE_TOKEN_VOLUME:
        parse_or_return (scanner, '=');
        if (g_scanner_peek_next_token (scanner) == '-')
          {
            g_scanner_get_next_token (scanner);
            negate = TRUE;
          }
        switch (g_scanner_get_next_token (scanner))
          {
          case G_TOKEN_FLOAT:   dvalue = scanner->value.v_float;        break;
          case G_TOKEN_INT:     dvalue = scanner->value.v_int64;        break;
          default:              return G_TOKEN_FLOAT;
          }
        if (negate)
          dvalue = -dvalue;
        DEBUG ("ignoring: volume=%f", dvalue);
        break;
      case GSL_WAVE_TOKEN_BALANCE:
        parse_or_return (scanner, '=');
        if (g_scanner_peek_next_token (scanner) == '-')
          {
            g_scanner_get_next_token (scanner);
            negate = TRUE;
          }
        switch (g_scanner_get_next_token (scanner))
          {
          case G_TOKEN_FLOAT:   dvalue = scanner->value.v_float;        break;
          case G_TOKEN_INT:     dvalue = scanner->value.v_int64;        break;
          default:              return G_TOKEN_FLOAT;
          }
        if (negate)
          dvalue = -dvalue;
        DEBUG ("ignoring: balance=%f", dvalue);
        break;
      case GSL_WAVE_TOKEN_CHUNK:
	if (g_scanner_peek_next_token (scanner) != '{')
	  parse_or_return (scanner, '{');
	i = dsc->wdsc.n_chunks++;
	dsc->wdsc.chunks = g_realloc (dsc->wdsc.chunks, sizeof (dsc->wdsc.chunks[0]) * dsc->wdsc.n_chunks);
	memset (dsc->wdsc.chunks + i, 0, sizeof (dsc->wdsc.chunks[0]) * 1);
	dsc->wdsc.chunks[i].mix_freq = dsc->dfl_mix_freq;
	dsc->wdsc.chunks[i].osc_freq = -1;
	dsc->wdsc.chunks[i].loader_offset = 0;			/* offset in bytes */
	dsc->wdsc.chunks[i].loader_length = 0;			/* length in n_values or bytes */
	dsc->wdsc.chunks[i].loader_data1 = NULL;		/* file_name */
	dsc->wdsc.chunks[i].loader_data2 = NULL;		/* wave_name */
	token = gslwave_parse_chunk_dsc (scanner, dsc->wdsc.chunks + i);
	if (token != G_TOKEN_NONE)
	  return token;
	if (dsc->wdsc.chunks[i].osc_freq <= 0)
	  g_scanner_error (scanner, "wave chunk \"%s\" without oscilator frequency: mix_freq=%f osc_freq=%f",
			   dsc->wdsc.chunks[i].loader_data1 ? (gchar*) dsc->wdsc.chunks[i].loader_data1 : "",
			   dsc->wdsc.chunks[i].mix_freq,
			   dsc->wdsc.chunks[i].osc_freq);
        if (dsc->wdsc.chunks[i].osc_freq >= dsc->wdsc.chunks[i].mix_freq / 2.)
          g_scanner_error (scanner, "wave chunk \"%s\" with invalid mixing/oscilator frequency: mix_freq=%f osc_freq=%f",
                           dsc->wdsc.chunks[i].loader_data1 ? (gchar*) dsc->wdsc.chunks[i].loader_data1 : "",
                           dsc->wdsc.chunks[i].mix_freq,
                           dsc->wdsc.chunks[i].osc_freq);
        break;
      case GSL_WAVE_TOKEN_BYTE_ORDER:
	parse_or_return (scanner, '=');
	token = g_scanner_get_next_token (scanner);
	switch (token)
	  {
	  case GSL_WAVE_TOKEN_LITTLE_ENDIAN:
	  case GSL_WAVE_TOKEN_LITTLE:		dsc->byte_order = G_LITTLE_ENDIAN; break;
	  case GSL_WAVE_TOKEN_BIG_ENDIAN:
	  case GSL_WAVE_TOKEN_BIG:		dsc->byte_order = G_BIG_ENDIAN;    break;
	  default:				return GSL_WAVE_TOKEN_LITTLE_ENDIAN;
	  }
	break;
      case GSL_WAVE_TOKEN_FORMAT:
	parse_or_return (scanner, '=');
	token = g_scanner_get_next_token (scanner);
	switch (token)
	  {
	  case GSL_WAVE_TOKEN_SIGNED_8:		dsc->format = GSL_WAVE_FORMAT_SIGNED_8;    break;
	  case GSL_WAVE_TOKEN_SIGNED_12:	dsc->format = GSL_WAVE_FORMAT_SIGNED_12;   break;
	  case GSL_WAVE_TOKEN_SIGNED_16:	dsc->format = GSL_WAVE_FORMAT_SIGNED_16;   break;
	  case GSL_WAVE_TOKEN_UNSIGNED_8:	dsc->format = GSL_WAVE_FORMAT_UNSIGNED_8;  break;
	  case GSL_WAVE_TOKEN_UNSIGNED_12:	dsc->format = GSL_WAVE_FORMAT_UNSIGNED_12; break;
	  case GSL_WAVE_TOKEN_UNSIGNED_16:	dsc->format = GSL_WAVE_FORMAT_UNSIGNED_16; break;
	  case GSL_WAVE_TOKEN_ALAW:		dsc->format = GSL_WAVE_FORMAT_ALAW;	   break;
	  case GSL_WAVE_TOKEN_ULAW:		dsc->format = GSL_WAVE_FORMAT_ULAW;	   break;
	  case GSL_WAVE_TOKEN_FLOAT:		dsc->format = GSL_WAVE_FORMAT_FLOAT;	   break;
	  default:				return GSL_WAVE_TOKEN_SIGNED_16;
	  }
	break;
      case GSL_WAVE_TOKEN_N_CHANNELS:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	dsc->wdsc.n_channels = scanner->value.v_int64;
	if (dsc->wdsc.n_channels < 1)
	  return G_TOKEN_INT;
	break;
      case GSL_WAVE_TOKEN_MIX_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:   dsc->dfl_mix_freq = scanner->value.v_float;	break;
	  case G_TOKEN_INT:     dsc->dfl_mix_freq = scanner->value.v_int64;	break;
	  default:		return G_TOKEN_FLOAT;
	  }
	break;
      }
  while (TRUE);
}

static void
gslwave_wave_dsc_free (WaveDsc *dsc)
{
  guint i;

  for (i = 0; i < dsc->wdsc.n_chunks; i++)
    {
      g_strfreev (dsc->wdsc.chunks[i].xinfos);
      g_free (dsc->wdsc.chunks[i].loader_data1); /* file_name */
      g_free (dsc->wdsc.chunks[i].loader_data2); /* wave_name */
    }
  g_free (dsc->wdsc.chunks);
  g_free (dsc->wdsc.name);
  sfi_delete_struct (WaveDsc, dsc);
}

static GslWaveDsc*
gslwave_load_wave_dsc (gpointer         data,
		       GslWaveFileInfo *file_info,
		       guint            nth_wave,
		       BseErrorType    *error_p)
{
  GScanner *scanner;
  WaveDsc *dsc;
  guint token, i;
  gint fd;

  fd = open (file_info->file_name, O_RDONLY);
  if (fd < 0)
    {
      *error_p = gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
      return NULL;
    }

  scanner = g_scanner_new64 (sfi_storage_scanner_config);
  scanner->config->cpair_comment_single = "#\n";
  scanner->input_name = file_info->file_name;
  g_scanner_input_file (scanner, fd);
  for (i = GSL_WAVE_TOKEN_WAVE; i < GSL_WAVE_TOKEN_LAST_FIELD; i++)
    g_scanner_scope_add_symbol (scanner, 0, gsl_wave_token (i), GUINT_TO_POINTER (i));
  for (i = GSL_WAVE_TOKEN_BIG_ENDIAN; i < GSL_WAVE_TOKEN_LAST_DATA; i++)
    g_scanner_scope_add_symbol (scanner, 0, gsl_wave_token (i), GUINT_TO_POINTER (i));

 continue_scanning:
  dsc = sfi_new_struct0 (WaveDsc, 1);
  dsc->wdsc.name = NULL;
  dsc->wdsc.n_chunks = 0;
  dsc->wdsc.chunks = NULL;
  dsc->wdsc.n_channels = 1;
  dsc->format = GSL_WAVE_FORMAT_SIGNED_16;
  dsc->byte_order = G_LITTLE_ENDIAN;
  dsc->dfl_mix_freq = 44100;
  if (g_scanner_get_next_token (scanner) != GSL_WAVE_TOKEN_WAVE)
    token = GSL_WAVE_TOKEN_WAVE;
  else
    token = gslwave_parse_wave_dsc (scanner, dsc, file_info->waves[nth_wave].name);
  if (token != G_TOKEN_NONE || scanner->parse_errors)
    {
      gslwave_wave_dsc_free (dsc);
      dsc = NULL;
      if (!scanner->parse_errors)
	g_scanner_unexp_token (scanner, token, "identifier", "keyword", NULL, "discarding wave", TRUE); /* FIXME */
    }
  else
    {
      if (dsc->wdsc.n_chunks && dsc->wdsc.name)
	{
	  /* found the correctly named wave and parsed it */
	}
      else
	{
	  /* got invalid/wrong wave */
	  gslwave_wave_dsc_free (dsc);
	  dsc = NULL;
	  goto continue_scanning;	/* next attempt */
	}
    }
  g_scanner_destroy (scanner);
  close (fd);

  return dsc ? &dsc->wdsc : NULL;
}

static void
gslwave_free_wave_dsc (gpointer    data,
		       GslWaveDsc *wave_dsc)
{
  WaveDsc *dsc = (WaveDsc*) wave_dsc;

  gslwave_wave_dsc_free (dsc);
}

static GslDataHandle*
gslwave_load_singlechunk_wave (GslWaveFileInfo *fi,
			       const gchar     *wave_name,
                               gfloat           osc_freq,
			       BseErrorType    *error_p)
{
  GslWaveDsc *wdsc;
  guint i;

  if (fi->n_waves == 1 && !wave_name)
    i = 0;
  else if (!wave_name)
    {
      /* don't know which wave to pick */
      *error_p = BSE_ERROR_FORMAT_INVALID;
      return NULL;
    }
  else /* find named wave */
    for (i = 0; i < fi->n_waves; i++)
      if (strcmp (fi->waves[i].name, wave_name) == 0)
	break;
  if (i >= fi->n_waves)
    {
      *error_p = BSE_ERROR_WAVE_NOT_FOUND;
      return NULL;
    }

  wdsc = gsl_wave_dsc_load (fi, i, error_p);
  if (!wdsc)
    return NULL;

  if (wdsc->n_chunks == 1)
    {
      GslDataHandle *dhandle = gsl_wave_handle_create (wdsc, 0, error_p);
      if (osc_freq > 0 && dhandle)
        gsl_data_handle_override (dhandle, -1, -1, osc_freq);
      gsl_wave_dsc_free (wdsc);
      return dhandle;
    }

  /* this is ridiculous, letting the chunk of a wave
   * point to a wave with multiple chunks...
   */
  gsl_wave_dsc_free (wdsc);
  *error_p = BSE_ERROR_FORMAT_INVALID;
  return NULL;
}

static GslDataHandle*
gslwave_create_chunk_handle (gpointer      data,
			     GslWaveDsc   *wave_dsc,
			     guint         nth_chunk,
			     BseErrorType *error_p)
{
  WaveDsc *dsc = (WaveDsc*) wave_dsc;
  FileInfo *fi = (FileInfo*) dsc->wdsc.file_info;
  GslWaveChunkDsc *chunk = wave_dsc->chunks + nth_chunk;

  if (chunk->loader_data1)	/* file_name */
    {
      GslDataHandle *dhandle;
      GslWaveFileInfo *cfi;
      gchar *string;

      /* construct chunk file name from (hopefully) relative path */
      if (g_path_is_absolute (chunk->loader_data1))
	string = g_strdup (chunk->loader_data1);
      else
	string = g_strdup_printf ("%s%c%s", fi->cwd, G_DIR_SEPARATOR, (char*) chunk->loader_data1);

      /* first, try to load the chunk via registered loaders */
      cfi = gsl_wave_file_info_load (string, error_p);
      if (cfi)
	{
	  /* FIXME: there's a potential attack here, in letting a single chunk
	   * wave's chunk point to its own wave. this'll trigger recursions until
	   * stack overflow
	   */
	  dhandle = gslwave_load_singlechunk_wave (cfi,
						   chunk->loader_data2,	/* wave_name */
                                                   chunk->osc_freq,
						   error_p);
          if (dhandle && chunk->xinfos)
            {
              GslDataHandle *tmp_handle = dhandle;
              dhandle = gsl_data_handle_new_add_xinfos (dhandle, chunk->xinfos);
              gsl_data_handle_unref (tmp_handle);
            }
	  gsl_wave_file_info_unref (cfi);
	  g_free (string);
	  return dhandle;
	}

      /* didn't work, assume it's a raw sample */
      if (chunk->loader_data2)	/* wave_name */
	{
	  /* raw samples don't give names to their data */
	  *error_p = BSE_ERROR_WAVE_NOT_FOUND;
	  g_free (string);
	  return NULL;
	}
      dhandle = gsl_wave_handle_new (string,			/* file_name */
				     dsc->wdsc.n_channels,
				     dsc->format,
				     dsc->byte_order,
                                     chunk->mix_freq <= 0 ? dsc->dfl_mix_freq : chunk->mix_freq,
                                     chunk->osc_freq,
				     chunk->loader_offset,	/* byte_offset */
				     (chunk->loader_length > 0	/* n_values */
                                      ? chunk->loader_length
                                      : -1),
                                     chunk->xinfos);
      *error_p = dhandle ? BSE_ERROR_NONE : BSE_ERROR_IO;
      g_free (string);
      return dhandle;
    }	
  else if (chunk->loader_num1 == RAWLINK_MAGIC)
    {
      GslDataHandle *dhandle = NULL;
      if (chunk->loader_length) /* inlined binary data */
	{
	  dhandle = gsl_wave_handle_new_zoffset (fi->wfi.file_name,
						 dsc->wdsc.n_channels,
						 dsc->format,
						 dsc->byte_order,
                                                 chunk->mix_freq <= 0 ? dsc->dfl_mix_freq : chunk->mix_freq,
                                                 chunk->osc_freq,
						 chunk->loader_offset,	/* byte_offset */
						 chunk->loader_length,	/* byte length */
                                                 chunk->xinfos);
	  *error_p = dhandle ? BSE_ERROR_NONE : BSE_ERROR_IO;
	}
      else
	*error_p = BSE_ERROR_WAVE_NOT_FOUND;
      return dhandle;
    }
  else if (chunk->loader_num1 == OGGLINK_MAGIC)
    {
      GslDataHandle *dhandle = NULL;
      if (chunk->loader_length) /* inlined binary data */
	{
	  dhandle = gsl_data_handle_new_ogg_vorbis_zoffset (fi->wfi.file_name,
                                                            chunk->osc_freq,
                                                            chunk->loader_offset,	/* byte_offset */
                                                            chunk->loader_length);	/* byte length */
          if (dhandle && chunk->xinfos)
            {
              GslDataHandle *tmp_handle = dhandle;
              dhandle = gsl_data_handle_new_add_xinfos (dhandle, chunk->xinfos);
              gsl_data_handle_unref (tmp_handle);
            }

	  *error_p = dhandle ? BSE_ERROR_NONE : BSE_ERROR_IO;
	}
      else
	*error_p = BSE_ERROR_WAVE_NOT_FOUND;
      return dhandle;
    }
  else /* no file_name and no loader specified */
    {
      *error_p = BSE_ERROR_FORMAT_UNKNOWN;
      return NULL;
    }
}

void
_gsl_init_loader_gslwave (void)
{
  static const gchar *file_exts[] = { "bsewave", NULL, };
  static const gchar *mime_types[] = { "audio/x-bsewave", NULL, };
  static const gchar *magics[] = { "0 string #BseWave", "0 string #GslWave", NULL, };
  static GslLoader loader = {
    "BseWave",
    file_exts,
    mime_types,
    0,	/* flags */
    magics,
    0,  /* priority */
    NULL,
    gslwave_load_file_info,
    gslwave_free_file_info,
    gslwave_load_wave_dsc,
    gslwave_free_wave_dsc,
    gslwave_create_chunk_handle,
  };
  static gboolean initialized = FALSE;

  g_assert (initialized == FALSE);
  initialized = TRUE;

  gsl_loader_register (&loader);
}
