/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Tim Janik
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
#include "gslwavedsc.h"

#include "gslcommon.h"
#include "gslmath.h"
#include "gsldatacache.h"
#include "gslwavechunk.h"
#include "gsldatahandle.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


/* --- macros --- */
#define parse_or_return(scanner, token)	{ guint _t = (token); \
                                          if (g_scanner_get_next_token (scanner) != _t) \
                                            return _t; \
                                        }


/* --- token types --- */
typedef enum
{
  GSL_WAVE_TOKEN_WAVE           = 512,
  GSL_WAVE_TOKEN_CHUNK,
  GSL_WAVE_TOKEN_NAME,
  GSL_WAVE_TOKEN_BYTE_ORDER,
  GSL_WAVE_TOKEN_FORMAT,
  GSL_WAVE_TOKEN_N_CHANNELS,
  GSL_WAVE_TOKEN_MIX_FREQ,
  GSL_WAVE_TOKEN_OSC_FREQ,
  GSL_WAVE_TOKEN_MIDI_NOTE,
  GSL_WAVE_TOKEN_FILE,
  GSL_WAVE_TOKEN_BOFFSET,
  GSL_WAVE_TOKEN_N_VALUES,
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
  GSL_WAVE_TOKEN_FLOAT,
  GSL_WAVE_TOKEN_LAST_DATA
} GslWaveTokenType;


/* --- variables --- */
static const char *wave_tokens_512[] = {
  "wave",	"chunk",	"name",		"byte_order",
  "format",	"n_channels",	"mix_freq",	"osc_freq",
  "midi_note",	"file",		"boffset",	"n_values",
  "loop_start", "loop_end",	"loop_count",
};
static const char *wave_tokens_768[] = {
  "big_endian",	"big",		"little_endian", "little",
  "signed_8",	"signed_12",	"signed_16",
  "unsigned_8",	"unsigned_12",	"unsigned_16",
  "float",
};


/* --- functions --- */
void
_gsl_init_wave_dsc (void)
{
  gboolean initialized = FALSE;

  g_assert (initialized == FALSE);
  initialized = TRUE;
}

static const gchar*
gsl_wave_token (GslWaveTokenType token)
{
  if (token >= 768)
    {
      token -= 768;
      return token > sizeof (wave_tokens_768) / sizeof (wave_tokens_768[0]) ? NULL : wave_tokens_768[token];
    }
  else
    {
      token -= 512;
      return token > sizeof (wave_tokens_512) / sizeof (wave_tokens_512[0]) ? NULL : wave_tokens_512[token];
    }
}

static guint
parse_wave_chunk_dsc (GScanner        *scanner,
		      GslWaveChunkDsc *chunk)
{
  parse_or_return (scanner, '{');
  do
    switch (g_scanner_get_next_token (scanner))
      {
      case '}':
	return G_TOKEN_NONE;
      default:
	return '}';
      case GSL_WAVE_TOKEN_FILE:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
	g_free (chunk->file_name);
	chunk->file_name = g_strdup (scanner->value.v_string);
	break;
      case GSL_WAVE_TOKEN_BYTE_ORDER:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case GSL_WAVE_TOKEN_LITTLE_ENDIAN:
	  case GSL_WAVE_TOKEN_LITTLE:		chunk->byte_order = G_LITTLE_ENDIAN;	break;
	  case GSL_WAVE_TOKEN_BIG_ENDIAN:
	  case GSL_WAVE_TOKEN_BIG:		chunk->byte_order = G_BIG_ENDIAN;	break;
	  default:	return GSL_WAVE_TOKEN_LITTLE_ENDIAN;
	  }
	break;
      case GSL_WAVE_TOKEN_FORMAT:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case GSL_WAVE_TOKEN_SIGNED_8:		chunk->format = GSL_WAVE_FORMAT_SIGNED_8;	break;
	  case GSL_WAVE_TOKEN_SIGNED_12:	chunk->format = GSL_WAVE_FORMAT_SIGNED_12;	break;
	  case GSL_WAVE_TOKEN_SIGNED_16:	chunk->format = GSL_WAVE_FORMAT_SIGNED_16;	break;
	  case GSL_WAVE_TOKEN_UNSIGNED_8:	chunk->format = GSL_WAVE_FORMAT_UNSIGNED_8;	break;
	  case GSL_WAVE_TOKEN_UNSIGNED_12:	chunk->format = GSL_WAVE_FORMAT_UNSIGNED_12;	break;
	  case GSL_WAVE_TOKEN_UNSIGNED_16:	chunk->format = GSL_WAVE_FORMAT_UNSIGNED_16;	break;
	  case GSL_WAVE_TOKEN_FLOAT:		chunk->format = GSL_WAVE_FORMAT_FLOAT;		break;
	  default:	return GSL_WAVE_TOKEN_SIGNED_16;
	  }
	break;
      case GSL_WAVE_TOKEN_N_CHANNELS:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
      	chunk->n_channels = scanner->value.v_int;
	if (chunk->n_channels < 1)
	  return G_TOKEN_INT;
	break;
      case GSL_WAVE_TOKEN_MIX_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:	chunk->mix_freq = scanner->value.v_float;	break;
	  case G_TOKEN_INT:	chunk->mix_freq = scanner->value.v_int;		break;
	  default:	return G_TOKEN_FLOAT;
	  }
	break;
      case GSL_WAVE_TOKEN_OSC_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:	chunk->osc_freq = scanner->value.v_float;	break;
	  case G_TOKEN_INT:	chunk->osc_freq = scanner->value.v_int;		break;
	  default:	return G_TOKEN_FLOAT;
	  }
	break;
      case GSL_WAVE_TOKEN_MIDI_NOTE:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->osc_freq = gsl_temp_freq (440., scanner->value.v_int - gsl_get_config ()->midi_kammer_note);	/* FIXME */
	break;
      case GSL_WAVE_TOKEN_BOFFSET:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->boffset = scanner->value.v_int;
	break;
      case GSL_WAVE_TOKEN_N_VALUES:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->n_values = scanner->value.v_int;
	break;
      case GSL_WAVE_TOKEN_LOOP_START:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->loop_start = scanner->value.v_int;
	break;
      case GSL_WAVE_TOKEN_LOOP_END:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->loop_end = scanner->value.v_int;
	break;
      case GSL_WAVE_TOKEN_LOOP_COUNT:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->loop_count = scanner->value.v_int;
	break;
      }
  while (TRUE);
}

static GTokenType
skip_rest_statement (GScanner *scanner,
		     guint     level)
{
  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);

  while (level)
    {
      g_scanner_get_next_token (scanner);
      switch (scanner->token)
	{
	case G_TOKEN_EOF: case G_TOKEN_ERROR:			return '}';
	case '(': case '{': case '[':		level++;	break;
	case ')': case '}': case ']':		level--;	break;
	default:						break;
	}
    }

  return G_TOKEN_NONE;
}

static guint
parse_wave (GScanner    *scanner,
	    GslWaveDsc  *wave,
	    const gchar *wave_name)
{
  parse_or_return (scanner, '{');
  do
    switch (g_scanner_get_next_token (scanner))
      {
	guint i, token;
      case '}':
	return G_TOKEN_NONE;
      default:
	return '}';
      case GSL_WAVE_TOKEN_CHUNK:
	if (g_scanner_peek_next_token (scanner) != '{')
	  parse_or_return (scanner, '{');
	i = wave->n_chunks++;
	wave->chunks = g_renew (GslWaveChunkDsc, wave->chunks, wave->n_chunks);
	wave->chunks[i].file_name = NULL;
	wave->chunks[i].byte_order = wave->dfl_byte_order;
	wave->chunks[i].format = wave->dfl_format;
	wave->chunks[i].n_channels = wave->dfl_n_channels;
	wave->chunks[i].mix_freq = wave->dfl_mix_freq;
	wave->chunks[i].osc_freq = 0;
	wave->chunks[i].boffset = 0;
	wave->chunks[i].n_values = 0;
	wave->chunks[i].loop_start = ~0;
	wave->chunks[i].loop_end = 0;
	wave->chunks[i].loop_count = 1000000; /* FIXME */
	token = parse_wave_chunk_dsc (scanner, wave->chunks + i);
	if (token != G_TOKEN_NONE)
	  return token;
	if (wave->chunks[i].loop_end <= wave->chunks[i].loop_start)
	  {
	    wave->chunks[i].loop_start = 0;
	    wave->chunks[i].loop_end = 0;
	    wave->chunks[i].loop_count = 0;
	  }
	if (wave->chunks[i].osc_freq >= wave->chunks[i].mix_freq / 2.)
	  g_scanner_error (scanner, "wave chunk \"%s\" mixing frequency is invalid: mix_freq=%f osc_freq=%f",
			   wave->chunks[i].file_name ? wave->chunks[i].file_name : "",
			   wave->chunks[i].mix_freq,
			   wave->chunks[i].osc_freq);
	break;
      case GSL_WAVE_TOKEN_NAME:
	if (wave->name)
	  return '}';
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
	if (wave_name)
	  {
	    if (strcmp (wave_name, scanner->value.v_string) == 0)
	      wave->name = g_strdup (scanner->value.v_string);
	    else
	      return skip_rest_statement (scanner, 1);
	  }
	else
	  wave->name = g_strdup (scanner->value.v_string);
	break;
      case GSL_WAVE_TOKEN_BYTE_ORDER:
	parse_or_return (scanner, '=');
	token = g_scanner_get_next_token (scanner);
	switch (token)
	  {
	  case GSL_WAVE_TOKEN_LITTLE_ENDIAN:
	  case GSL_WAVE_TOKEN_LITTLE:		wave->dfl_byte_order = G_LITTLE_ENDIAN;	break;
	  case GSL_WAVE_TOKEN_BIG_ENDIAN:
	  case GSL_WAVE_TOKEN_BIG:		wave->dfl_byte_order = G_BIG_ENDIAN;	break;
	  default:	return GSL_WAVE_TOKEN_LITTLE_ENDIAN;
	  }
	break;
      case GSL_WAVE_TOKEN_FORMAT:
	parse_or_return (scanner, '=');
	token = g_scanner_get_next_token (scanner);
	switch (token)
	  {
	  case GSL_WAVE_TOKEN_SIGNED_8:		wave->dfl_format = GSL_WAVE_FORMAT_SIGNED_8;	break;
	  case GSL_WAVE_TOKEN_SIGNED_12:	wave->dfl_format = GSL_WAVE_FORMAT_SIGNED_12;	break;
	  case GSL_WAVE_TOKEN_SIGNED_16:	wave->dfl_format = GSL_WAVE_FORMAT_SIGNED_16;	break;
	  case GSL_WAVE_TOKEN_UNSIGNED_8:	wave->dfl_format = GSL_WAVE_FORMAT_UNSIGNED_8;	break;
	  case GSL_WAVE_TOKEN_UNSIGNED_12:	wave->dfl_format = GSL_WAVE_FORMAT_UNSIGNED_12;	break;
	  case GSL_WAVE_TOKEN_UNSIGNED_16:	wave->dfl_format = GSL_WAVE_FORMAT_UNSIGNED_16;	break;
	  case GSL_WAVE_TOKEN_FLOAT:		wave->dfl_format = GSL_WAVE_FORMAT_FLOAT;	break;
	  default:	return GSL_WAVE_TOKEN_SIGNED_16;
	  }
	break;
      case GSL_WAVE_TOKEN_N_CHANNELS:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	wave->dfl_n_channels = scanner->value.v_int;
	if (wave->dfl_n_channels < 1)
	  return G_TOKEN_INT;
	break;
      case GSL_WAVE_TOKEN_MIX_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:	wave->dfl_mix_freq = scanner->value.v_float;	break;
	  case G_TOKEN_INT:	wave->dfl_mix_freq = scanner->value.v_int;	break;
	  default:	return G_TOKEN_FLOAT;
	  }
	break;
      }
  while (TRUE);
}

static void
wave_dsc_sort_chunks (GslWaveDsc *wave)
{
  guint i, n;

  g_return_if_fail (wave != NULL);

  n = wave->n_chunks;
  while (n)
    {
      for (i = 0; i < n - 1; i++)
	{
	  GslWaveChunkDsc *chunk1 = wave->chunks + i;
	  GslWaveChunkDsc *chunk2 = chunk1 + 1;

	  if (chunk1->osc_freq > chunk2->osc_freq)
	    {
	      GslWaveChunkDsc tmp = *chunk2;

	      *chunk2 = *chunk1;
	      *chunk1 = tmp;
	    }
	}
      n--;
      wave->max_n_channels = MAX (wave->max_n_channels, wave->chunks[n].n_channels);
    }
}

GslWaveDsc*
gsl_wave_dsc_read (const gchar *file_name)
{
  GScanner *scanner;
  GslWaveDsc *wave;
  gint fd;
  guint i, token;

  g_return_val_if_fail (file_name != NULL, NULL);

  fd = open (file_name, O_RDONLY);
  if (fd < 0)
    return NULL;
  scanner = g_scanner_new (NULL);
  scanner->config->symbol_2_token = TRUE;
  scanner->input_name = file_name;
  g_scanner_input_file (scanner, fd);
  for (i = GSL_WAVE_TOKEN_WAVE; i < GSL_WAVE_TOKEN_LAST_FIELD; i++)
    g_scanner_add_symbol (scanner, gsl_wave_token (i), GUINT_TO_POINTER (i));
  for (i = GSL_WAVE_TOKEN_BIG_ENDIAN; i < GSL_WAVE_TOKEN_LAST_DATA; i++)
    g_scanner_add_symbol (scanner, gsl_wave_token (i), GUINT_TO_POINTER (i));
 continue_scanning:
  wave = gsl_new_struct0 (GslWaveDsc, 1);
  wave->name = NULL;
  wave->n_chunks = 0;
  wave->chunks = NULL;
  wave->max_n_channels = 0;
  wave->dfl_format = GSL_WAVE_FORMAT_SIGNED_16;
  wave->dfl_n_channels = 1;
  wave->dfl_byte_order = G_LITTLE_ENDIAN;
  wave->dfl_mix_freq = 44100;
  if (g_scanner_get_next_token (scanner) != GSL_WAVE_TOKEN_WAVE)
    token = GSL_WAVE_TOKEN_WAVE;
  else
    token = parse_wave (scanner, wave, NULL);
  if (token != G_TOKEN_NONE || scanner->parse_errors)
    {
      gsl_wave_dsc_free (wave);
      wave = NULL;
      if (!scanner->parse_errors)
	g_scanner_unexp_token (scanner, token, "identifier", "keyword", NULL, "discarding wave", TRUE);
    }
  else
    {
      if (wave->n_chunks && wave->name)
	wave_dsc_sort_chunks (wave);	/* sets max_n_channels */
      else
	{
	  /* got invalid/wrong wave */
	  gsl_wave_dsc_free (wave);
	  wave = NULL;
	  goto continue_scanning;	/* next attempt */
	}
    }
  g_scanner_destroy (scanner);
  close (fd);

  return wave;
}

void
gsl_wave_dsc_free (GslWaveDsc *wave)
{
  guint i;

  g_return_if_fail (wave != NULL);

  for (i = 0; i < wave->n_chunks; i++)
    g_free (wave->chunks[i].file_name);
  g_free (wave->chunks);
  g_free (wave->name);
  gsl_delete_struct (GslWaveDsc, 1, wave);
}

GslRing* /* free ring + ring ->data elements */
gsl_wave_file_scan (const gchar *file_name)
{
  GslRing *ring = NULL;
  gint fd;

  g_return_val_if_fail (file_name != NULL, NULL);

  fd = open (file_name, O_RDONLY);
  if (fd >= 0)
    {
      GScanner *scanner = g_scanner_new (NULL);
      gboolean in_wave = FALSE, abort = FALSE;

      scanner->config->symbol_2_token = TRUE;
      g_scanner_add_symbol (scanner, "wave", GUINT_TO_POINTER (GSL_WAVE_TOKEN_WAVE));
      g_scanner_add_symbol (scanner, "name", GUINT_TO_POINTER (GSL_WAVE_TOKEN_NAME));
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
	      if (skip_rest_statement (scanner, 1) != G_TOKEN_NONE)
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
		      if (skip_rest_statement (scanner, 1) == G_TOKEN_NONE)
			{
			  in_wave = FALSE;
			  ring = gsl_ring_append (ring, wave_name);
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
	      if (scanner->token == G_TOKEN_EOF ||
		  scanner->token == G_TOKEN_ERROR)
		abort = TRUE;
	      break;
	    }
	}
      g_scanner_destroy (scanner);
      close (fd);
    }

  return ring;
}

void
gsl_wave_file_scan_free (GslRing *ring)
{
  GslRing *node;

  for (node = ring; node; node = gsl_ring_walk (ring, node))
    g_free (node->data);
  gsl_ring_free (ring);
}

GslWaveChunk*
gsl_wave_chunk_from_dsc (GslWaveDsc *wave_dsc,
			 guint       nth_chunk)
{
  GslDataHandle *dhandle;
  GslDataCache *dcache;
  GslWaveChunk *wchunk;
  GslWaveChunkDsc *wchunk_dsc;
  GslLong length;

  g_return_val_if_fail (wave_dsc != NULL, NULL);
  g_return_val_if_fail (nth_chunk < wave_dsc->n_chunks, NULL);

  wchunk_dsc = wave_dsc->chunks + nth_chunk;
  dhandle = gsl_wave_handle_new_cached (wchunk_dsc->file_name, 0,
					wchunk_dsc->format,
					wchunk_dsc->byte_order,
					wchunk_dsc->boffset % 4); /* 4==sizeof (float) */
  if (!dhandle)
    return NULL;

  dcache = gsl_data_cache_from_dhandle (dhandle, gsl_get_config ()->wave_chunk_padding * wchunk_dsc->n_channels);
  gsl_data_handle_unref (dhandle);
  if (!dcache)
    return NULL;
  /* dcache keeps dhandle alive */

  if (wchunk_dsc->n_values)
    length = wchunk_dsc->n_values;
  else
    length = dhandle->n_values / wchunk_dsc->n_channels;
  wchunk = _gsl_wave_chunk_create (dcache,
				   wchunk_dsc->boffset >> 2, /* divide by 4==sizeof (float) */
				   length,
				   wchunk_dsc->n_channels,
				   wchunk_dsc->osc_freq, wchunk_dsc->mix_freq,
				   GSL_WAVE_LOOP_JUMP,
				   wchunk_dsc->loop_start,
				   wchunk_dsc->loop_end,
				   wchunk_dsc->loop_count);
  gsl_data_cache_unref (dcache);
  if (!wchunk)
    return NULL;

  return wchunk;
}
