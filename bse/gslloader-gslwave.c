/* GSL - Generic Sound Layer
 * Copyright (C) 2001, 2002 Tim Janik
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
#include "gslmath.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#define	GSL_DEBUG_LOADER	g_message

#define parse_or_return(scanner, token) { guint _t = (token); \
                                          if (g_scanner_get_next_token (scanner) != _t) \
                                            return _t; \
                                        }


/* --- token types --- */
typedef enum
{
  /* wave tokens */
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
  GSL_WAVE_TOKEN_INDEX,
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
  "wave",       "chunk",        "name",         "byte_order",
  "format",     "n_channels",   "mix_freq",     "osc_freq",
  "midi_note",  "file",         "index",	"boffset",
  "n_values",	"loop_type",	"loop_start",	"loop_end",
  "loop_count",
};
static const char *wave_tokens_768[] = {
  "big_endian", "big",          "little_endian", "little",
  "signed_8",   "signed_12",    "signed_16",
  "unsigned_8", "unsigned_12",  "unsigned_16",
  "float",	"none",		"jump",		 "pingpong",
};


/* --- functions --- */
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
	case G_TOKEN_EOF: case G_TOKEN_ERROR:                   return '}';
	case '(': case '{': case '[':           level++;        break;
	case ')': case '}': case ']':           level--;        break;
	default:                                                break;
	}
    }

  return G_TOKEN_NONE;
}

static GslWaveFileInfo*
load_file_info (gpointer      data,
		const gchar  *file_name,
		GslErrorType *error_p)
{
  FileInfo *fi = NULL;
  gboolean in_wave = FALSE, abort = FALSE;
  GslRing *wave_names = NULL;
  GScanner *scanner;
  gint fd;
  guint i;

  fd = open (file_name, O_RDONLY);
  if (fd < 0)
    {
      *error_p = GSL_ERROR_OPEN_FAILED;
      return NULL;
    }

  scanner = g_scanner_new (NULL);
  scanner->config->symbol_2_token = TRUE;
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
		      wave_names = gsl_ring_append (wave_names, wave_name);
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
      GslRing *ring;

      fi = gsl_new_struct0 (FileInfo, 1);
      fi->wfi.n_waves = gsl_ring_length (wave_names);
      fi->wfi.waves = g_malloc0 (sizeof (fi->wfi.waves[0]) * fi->wfi.n_waves);
      for (i = 0, ring = wave_names; i < fi->wfi.n_waves; i++, ring = ring->next)
	fi->wfi.waves[i].name = ring->data;
      gsl_ring_free (wave_names);
    }

  /* FIXME: empty wave error? */

  return fi ? &fi->wfi : NULL;
}

static void
free_file_info (gpointer         data,
		GslWaveFileInfo *file_info)
{
  FileInfo *fi = (FileInfo*) file_info;
  guint i;

  for (i = 0; i < fi->wfi.n_waves; i++)
    g_free (fi->wfi.waves[i].name);
  g_free (fi->wfi.waves);
  gsl_delete_struct (FileInfo, fi);
}

static guint
parse_chunk_dsc (GScanner        *scanner,
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
	g_free (chunk->loader_data1);	/* file_name */
	chunk->loader_data1 = g_strdup (scanner->value.v_string);
	break;
      case GSL_WAVE_TOKEN_INDEX:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
	g_free (chunk->loader_data2);	/* wave_name */
	chunk->loader_data2 = g_strdup (scanner->value.v_string);
	break;
      case GSL_WAVE_TOKEN_MIX_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:	chunk->mix_freq = scanner->value.v_float;	break;
	  case G_TOKEN_INT:	chunk->mix_freq = scanner->value.v_int;		break;
	  default:		return G_TOKEN_FLOAT;
	  }
	break;
      case GSL_WAVE_TOKEN_OSC_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:	chunk->osc_freq = scanner->value.v_float;	break;
	  case G_TOKEN_INT:	chunk->osc_freq = scanner->value.v_int;		break;
	  default:		return G_TOKEN_FLOAT;
	  }
	break;
      case GSL_WAVE_TOKEN_MIDI_NOTE:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->osc_freq = gsl_temp_freq (gsl_get_config ()->kammer_freq,
					 scanner->value.v_int - gsl_get_config ()->midi_kammer_note);
	break;
      case GSL_WAVE_TOKEN_BOFFSET:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->loader_offset = scanner->value.v_int;	/* byte_offset */
	break;
      case GSL_WAVE_TOKEN_N_VALUES:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->loader_length = scanner->value.v_int;	/* n_values */
	break;
      case GSL_WAVE_TOKEN_LOOP_TYPE:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case GSL_WAVE_TOKEN_NONE:	chunk->loop_type = GSL_WAVE_LOOP_NONE;		break;
	  case GSL_WAVE_TOKEN_JUMP:	chunk->loop_type = GSL_WAVE_LOOP_JUMP;		break;
	  case GSL_WAVE_TOKEN_PINGPONG:	chunk->loop_type = GSL_WAVE_LOOP_PINGPONG;	break;
	  default:			return GSL_WAVE_TOKEN_JUMP;
	  }
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

static guint
parse_wave_dsc (GScanner    *scanner,
		WaveDsc     *dsc,
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
	      return skip_rest_statement (scanner, 1);
	  }
	else
	  dsc->wdsc.name = g_strdup (scanner->value.v_string);
	break;
      case GSL_WAVE_TOKEN_CHUNK:
	if (g_scanner_peek_next_token (scanner) != '{')
	  parse_or_return (scanner, '{');
	i = dsc->wdsc.n_chunks++;
	dsc->wdsc.chunks = g_realloc (dsc->wdsc.chunks, sizeof (dsc->wdsc.chunks[0]) * dsc->wdsc.n_chunks);
	memset (dsc->wdsc.chunks + i, 0, sizeof (dsc->wdsc.chunks[0]) * 1);
	dsc->wdsc.chunks[i].mix_freq = dsc->dfl_mix_freq;
	dsc->wdsc.chunks[i].osc_freq = dsc->dfl_mix_freq;	/* we check this later */
	dsc->wdsc.chunks[i].loop_type = GSL_WAVE_LOOP_JUMP;
	dsc->wdsc.chunks[i].loop_start = ~0;
	dsc->wdsc.chunks[i].loop_end = 0;
	dsc->wdsc.chunks[i].loop_count = 1000000; /* FIXME */
	dsc->wdsc.chunks[i].loader_offset = 0;			/* offset in bytes */
	dsc->wdsc.chunks[i].loader_length = 0;			/* length in n_values */
	dsc->wdsc.chunks[i].loader_data1 = NULL;		/* file_name */
	dsc->wdsc.chunks[i].loader_data2 = NULL;		/* wave_name */
	token = parse_chunk_dsc (scanner, dsc->wdsc.chunks + i);
	if (token != G_TOKEN_NONE)
	  return token;
	if (dsc->wdsc.chunks[i].loop_end <= dsc->wdsc.chunks[i].loop_start)
	  {
	    dsc->wdsc.chunks[i].loop_type = GSL_WAVE_LOOP_NONE;
	    dsc->wdsc.chunks[i].loop_start = 0;
	    dsc->wdsc.chunks[i].loop_end = 0;
	    dsc->wdsc.chunks[i].loop_count = 0;
	  }
	if (dsc->wdsc.chunks[i].osc_freq >= dsc->wdsc.chunks[i].mix_freq / 2.)
	  g_scanner_error (scanner, "wave chunk \"%s\" mixing frequency is invalid: mix_freq=%f osc_freq=%f",
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
	  case GSL_WAVE_TOKEN_FLOAT:		dsc->format = GSL_WAVE_FORMAT_FLOAT;	   break;
	  default:				return GSL_WAVE_TOKEN_SIGNED_16;
	  }
	break;
      case GSL_WAVE_TOKEN_N_CHANNELS:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	dsc->wdsc.n_channels = scanner->value.v_int;
	if (dsc->wdsc.n_channels < 1)
	  return G_TOKEN_INT;
	break;
      case GSL_WAVE_TOKEN_MIX_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:   dsc->dfl_mix_freq = scanner->value.v_float;    break;
	  case G_TOKEN_INT:     dsc->dfl_mix_freq = scanner->value.v_int;      break;
	  default:		return G_TOKEN_FLOAT;
	  }
	break;
      }
  while (TRUE);
}

static void
wave_dsc_free (WaveDsc *dsc)
{
  guint i;

  for (i = 0; i < dsc->wdsc.n_chunks; i++)
    {
      g_free (dsc->wdsc.chunks[i].loader_data1); /* file_name */
      g_free (dsc->wdsc.chunks[i].loader_data2); /* wave_name */
    }
  g_free (dsc->wdsc.chunks);
  g_free (dsc->wdsc.name);
  gsl_delete_struct (WaveDsc, dsc);
}

static GslWaveDsc*
load_wave_dsc (gpointer         data,
	       GslWaveFileInfo *file_info,
	       guint            nth_wave,
	       GslErrorType    *error_p)
{
  GScanner *scanner;
  WaveDsc *dsc;
  guint token, i;
  gint fd;

  fd = open (file_info->file_name, O_RDONLY);
  if (fd < 0)
    {
      *error_p = GSL_ERROR_OPEN_FAILED;
      return NULL;
    }

  scanner = g_scanner_new (NULL);
  scanner->config->symbol_2_token = TRUE;
  scanner->input_name = file_info->file_name;
  g_scanner_input_file (scanner, fd);
  for (i = GSL_WAVE_TOKEN_WAVE; i < GSL_WAVE_TOKEN_LAST_FIELD; i++)
    g_scanner_scope_add_symbol (scanner, 0, gsl_wave_token (i), GUINT_TO_POINTER (i));
  for (i = GSL_WAVE_TOKEN_BIG_ENDIAN; i < GSL_WAVE_TOKEN_LAST_DATA; i++)
    g_scanner_scope_add_symbol (scanner, 0, gsl_wave_token (i), GUINT_TO_POINTER (i));

 continue_scanning:
  dsc = gsl_new_struct0 (WaveDsc, 1);
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
    token = parse_wave_dsc (scanner, dsc, file_info->waves[nth_wave].name);
  if (token != G_TOKEN_NONE || scanner->parse_errors)
    {
      wave_dsc_free (dsc);
      dsc = NULL;
      if (!scanner->parse_errors)
	g_scanner_unexp_token (scanner, token, "identifier", "keyword", NULL, "discarding wave", TRUE); // FIXME
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
	  wave_dsc_free (dsc);
	  dsc = NULL;
	  goto continue_scanning;	/* next attempt */
	}
    }
  g_scanner_destroy (scanner);
  close (fd);

  return dsc ? &dsc->wdsc : NULL;
}

static void
free_wave_dsc (gpointer    data,
	       GslWaveDsc *wave_dsc)
{
  WaveDsc *dsc = (WaveDsc*) wave_dsc;

  wave_dsc_free (dsc);
}

static GslDataHandle*
load_singlechunk_wave (GslWaveFileInfo *fi,
		       const gchar     *wave_name,
		       GslErrorType    *error_p)
{
  GslWaveDsc *wdsc;
  guint i;

  if (fi->n_waves == 1 && !wave_name)
    i = 0;
  else if (!wave_name)
    {
      /* don't know which wave to pick */
      *error_p = GSL_ERROR_FORMAT_INVALID;
      return NULL;
    }
  else /* find named wave */
    for (i = 0; i < fi->n_waves; i++)
      if (strcmp (fi->waves[i].name, wave_name) == 0)
	break;
  if (i >= fi->n_waves)
    {
      *error_p = GSL_ERROR_NOT_FOUND;
      return NULL;
    }

  wdsc = gsl_wave_dsc_load (fi, i, error_p);
  if (!wdsc)
    return NULL;

  if (wdsc->n_chunks == 1)
    {
      GslDataHandle *dhandle = gsl_wave_handle_create (wdsc, 0, error_p);

      gsl_wave_dsc_free (wdsc);
      return dhandle;
    }

  /* this is ridiculous, letting the chunk of a wave
   * point to a wave with multiple chunks...
   */
  gsl_wave_dsc_free (wdsc);
  *error_p = GSL_ERROR_FORMAT_INVALID;
  return NULL;
}

static GslDataHandle*
create_chunk_handle (gpointer      data,
		     GslWaveDsc   *wave_dsc,
		     guint         nth_chunk,
		     GslErrorType *error_p)
{
  WaveDsc *dsc = (WaveDsc*) wave_dsc;
  GslWaveChunkDsc *chunk = wave_dsc->chunks + nth_chunk;

  if (chunk->loader_data1)	/* file_name */
    {
      GslDataHandle *dhandle;
      GslWaveFileInfo *fi;

      /* first, try to load the chunk via registered loaders
       */
      fi = gsl_wave_file_info_load (chunk->loader_data1, error_p);
      if (fi)
	{
	  /* FIXME: there's a potential attack here, in letting a single chunk
	   * wave's chunk point to its own wave. this'll trigger recursions until
	   * stack overflow
	   */
	  dhandle = load_singlechunk_wave (fi,
					   chunk->loader_data2,	/* wave_name */
					   error_p);
	  gsl_wave_file_info_free (fi);
	  return dhandle;
	}

      /* didn't work, assume it's a raw sample
       */
      if (chunk->loader_data2)	/* wave_name */
	{
	  /* raw samples don't give names to their data */
	  *error_p = GSL_ERROR_NOT_FOUND;
	  return NULL;
	}
      dhandle = gsl_wave_handle_new (chunk->loader_data1,	/* file_name */
				     0,
				     dsc->format,
				     dsc->byte_order,
				     chunk->loader_offset,	/* byte_offset */
				     chunk->loader_length > 0	/* n_values */
				     ? chunk->loader_length
				     : -1);
      *error_p = dhandle ? GSL_ERROR_NONE : GSL_ERROR_IO;
      return dhandle;
    }	
  else
    {
      /* no file_name specified */
      *error_p = GSL_ERROR_NOT_FOUND;
      return NULL;
    }
}

void
_gsl_init_loader_gslwave (void)
{
  static gboolean initialized = FALSE;
  static GslLoader gslwave_loader = {
    "GslWave",
    "gslwave",
    "audio/x-gslwave",
    "0 string #GslWave",
    0,
    NULL,
    load_file_info,
    free_file_info,
    load_wave_dsc,
    free_wave_dsc,
    create_chunk_handle,
  };

  g_assert (initialized == FALSE);
  initialized = TRUE;

  gsl_loader_register (&gslwave_loader);
}
