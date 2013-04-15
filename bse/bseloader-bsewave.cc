// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseloader.hh"
#include "bsemain.hh"
#include "gsldatahandle.hh"
#include "gsldatahandle-vorbis.hh"
#include "bsemath.hh"
#include <sfi/sfistore.hh>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define parse_or_return(scanner, token) { GslWaveTokenType _t = GslWaveTokenType (token); \
                                          if (GslWaveTokenType (g_scanner_get_next_token (scanner)) != _t) \
                                            return _t; \
                                        }

/* --- token types --- */
typedef enum
{
  /* keyword tokens */
  BSEWAVE_TOKEN_WAVE           = 512,
  BSEWAVE_TOKEN_CHUNK,
  BSEWAVE_TOKEN_NAME,
  BSEWAVE_TOKEN_N_CHANNELS,
  BSEWAVE_TOKEN_MIDI_NOTE,
  BSEWAVE_TOKEN_OSC_FREQ,
  BSEWAVE_TOKEN_XINFO,
  BSEWAVE_TOKEN_VORBIS_LINK,
  BSEWAVE_TOKEN_FILE,
  BSEWAVE_TOKEN_INDEX,          /* file (auto detect loader) */
  BSEWAVE_TOKEN_RAW_FILE,
  BSEWAVE_TOKEN_BOFFSET,        /* raw-file */
  BSEWAVE_TOKEN_N_VALUES,       /* raw-file */
  BSEWAVE_TOKEN_RAW_LINK,
  BSEWAVE_TOKEN_BYTE_ORDER,     /* raw-file, raw-link */
  BSEWAVE_TOKEN_FORMAT,         /* raw-file, raw-link */
  BSEWAVE_TOKEN_MIX_FREQ,       /* raw-file, raw-link */
  /* data tokens */
  BSEWAVE_TOKEN_BIG_ENDIAN,
  BSEWAVE_TOKEN_BIG,
  BSEWAVE_TOKEN_LITTLE_ENDIAN,
  BSEWAVE_TOKEN_LITTLE,
  BSEWAVE_TOKEN_SIGNED_8,
  BSEWAVE_TOKEN_SIGNED_12,
  BSEWAVE_TOKEN_SIGNED_16,
  BSEWAVE_TOKEN_UNSIGNED_8,
  BSEWAVE_TOKEN_UNSIGNED_12,
  BSEWAVE_TOKEN_UNSIGNED_16,
  BSEWAVE_TOKEN_ALAW,
  BSEWAVE_TOKEN_ULAW,
  BSEWAVE_TOKEN_FLOAT,
  BSEWAVE_TOKEN_NONE,
  BSEWAVE_TOKEN_JUMP,
  BSEWAVE_TOKEN_PINGPONG,
  BSEWAVE_TOKEN_LAST,
} GslWaveTokenType;
/* --- tokens --- */
static const char *bsewave_tokens[] = {
  /* keyword tokens */
  "wave",       "chunk",        "name",         "n-channels",
  "midi-note",  "osc-freq",     "xinfo",        "vorbis-link",
  "file",       "index",        "raw-file",     "boffset",
  "n-values",   "raw-link",     "byte-order",   "format",
  "mix-freq",
  /* data tokens */
  "big-endian", "big",          "little-endian", "little",
  "signed-8",   "signed-12",    "signed-16",
  "unsigned-8", "unsigned-12",  "unsigned-16",
  "alaw",       "ulaw",         "float",
  "none",	"jump",		"pingpong",
};
/* --- structures --- */
typedef struct
{
  BseWaveFileInfo wfi;
  char           *cwd;
} FileInfo;
typedef struct
{
  BseWaveDsc        wdsc;
  GslWaveFormatType dfl_format;
  uint		    dfl_byte_order;
  float	            dfl_mix_freq;
} WaveDsc;
/* BseWaveChunkDsc accessors */
#define LOADER_TYPE(wcd)        ((wcd)->loader_data[0].uint)
#define LOADER_FILE(wcd)        ((wcd)->loader_data[1].ptr)
#define LOADER_INDEX(wcd)       ((wcd)->loader_data[2].ptr)
#define LOADER_FORMAT(wcd)      ((wcd)->loader_data[4].uint)
#define LOADER_BYTE_ORDER(wcd)  ((wcd)->loader_data[5].uint)
#define LOADER_BOFFSET(wcd)     ((wcd)->loader_data[6].uint)
#define LOADER_LENGTH(wcd)      ((wcd)->loader_data[7].uint)
/* loader types */
#define AUTO_FILE_MAGIC         (('A' << 24) | ('u' << 16) | ('t' << 8) | 'F')
#define RAW_FILE_MAGIC          (('R' << 24) | ('a' << 16) | ('w' << 8) | 'F')
#define RAW_LINK_MAGIC          (('R' << 24) | ('a' << 16) | ('w' << 8) | 'L')
#define VORBIS_LINK_MAGIC       (('O' << 24) | ('/' << 16) | ('V' << 8) | '1')
/* --- functions --- */
static GTokenType
bsewave_skip_rest_statement (GScanner *scanner,
			     uint      level)
{
  g_return_val_if_fail (scanner != NULL, G_TOKEN_ERROR);
  while (level)
    {
      g_scanner_get_next_token (scanner);
      switch (scanner->token)
	{
	case G_TOKEN_EOF: case G_TOKEN_ERROR:                   return GTokenType ('}');
	case '(': case '{': case '[':           level++;        break;
	case ')': case '}': case ']':           level--;        break;
	default:                                                break;
	}
    }
  return G_TOKEN_NONE;
}
static BseWaveFileInfo*
bsewave_load_file_info (void         *data,
			const char   *_file_name,
			BseErrorType *error_p)
{
  FileInfo *fi = NULL;
  gboolean in_wave = FALSE, abort = FALSE;
  SfiRing *wave_names = NULL;
  GScanner *scanner;
  char *cwd, *file_name;
  int fd;
  uint i;
  if (g_path_is_absolute (_file_name))
    {
      const char *p = strrchr (_file_name, G_DIR_SEPARATOR);
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
  scanner->config->cpair_comment_single = (char*) "#\n";
  g_scanner_scope_add_symbol (scanner, 0, "wave", GUINT_TO_POINTER (BSEWAVE_TOKEN_WAVE));
  g_scanner_scope_add_symbol (scanner, 0, "name", GUINT_TO_POINTER (BSEWAVE_TOKEN_NAME));
  g_scanner_input_file (scanner, fd);
  while (!abort)
    {
      g_scanner_get_next_token (scanner);
      switch (GslWaveTokenType (scanner->token))
	{
	case BSEWAVE_TOKEN_WAVE:
	  if (g_scanner_peek_next_token (scanner) == '{')
	    {
	      g_scanner_get_next_token (scanner); /* eat '{' */
	      in_wave = TRUE;
	    }
	  break;
	case '{':
	  if (bsewave_skip_rest_statement (scanner, 1) != G_TOKEN_NONE)
	    abort = TRUE;
	  break;
	case BSEWAVE_TOKEN_NAME:
	  if (in_wave && g_scanner_peek_next_token (scanner) == '=')
	    {
	      g_scanner_get_next_token (scanner); /* eat '=' */
	      if (g_scanner_peek_next_token (scanner) == G_TOKEN_STRING)
		{
		  char *wave_name;
		  g_scanner_get_next_token (scanner); /* eat string */
		  wave_name = g_strdup (scanner->value.v_string);
		  if (bsewave_skip_rest_statement (scanner, 1) == G_TOKEN_NONE)
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
      fi->wfi.waves = (BseWaveFileInfo::Wave*) g_malloc0 (sizeof (fi->wfi.waves[0]) * fi->wfi.n_waves);
      for (i = 0, ring = wave_names; i < fi->wfi.n_waves; i++, ring = ring->next)
	fi->wfi.waves[i].name = (char*) ring->data;
      sfi_ring_free (wave_names);
      fi->cwd = cwd;
    }
  else
    g_free (cwd);
  g_free (file_name);
  return fi ? &fi->wfi : NULL;
}
static void
bsewave_free_file_info (void            *data,
			BseWaveFileInfo *file_info)
{
  FileInfo *fi = (FileInfo*) file_info;
  uint i;
  for (i = 0; i < fi->wfi.n_waves; i++)
    g_free (fi->wfi.waves[i].name);
  g_free (fi->wfi.waves);
  g_free (fi->cwd);
  sfi_delete_struct (FileInfo, fi);
}
static uint
bsewave_parse_chunk_dsc (GScanner        *scanner,
			 BseWaveChunkDsc *chunk)
{
  parse_or_return (scanner, '{');
  do
    switch (GslWaveTokenType (g_scanner_get_next_token (scanner)))
      {
        char *key;
        float vfloat;
      case '}':
	return G_TOKEN_NONE;
      default:
	return '}';
      case BSEWAVE_TOKEN_MIDI_NOTE:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	chunk->osc_freq = bse_temp_freq (BSE_CONFIG (kammer_freq),
					 ((int) scanner->value.v_int64) - BSE_CONFIG (midi_kammer_note));
        chunk->xinfos = bse_xinfos_add_num (chunk->xinfos, "midi-note", scanner->value.v_int64);
	break;
      case BSEWAVE_TOKEN_OSC_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:	vfloat = scanner->value.v_float;	break;
	  case G_TOKEN_INT:	vfloat = scanner->value.v_int64;	break;
	  default:		return G_TOKEN_FLOAT;
	  }
        chunk->osc_freq = vfloat;
        chunk->xinfos = bse_xinfos_add_float (chunk->xinfos, "osc-freq", vfloat);
	break;
      case BSEWAVE_TOKEN_XINFO:
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
      case BSEWAVE_TOKEN_VORBIS_LINK:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, '(');
	parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	if (strcmp (scanner->value.v_identifier, "binary-appendix") != 0)
	  return G_TOKEN_IDENTIFIER;
	parse_or_return (scanner, G_TOKEN_INT);
	LOADER_BOFFSET (chunk) = scanner->value.v_int64;        /* byte offset */
	parse_or_return (scanner, G_TOKEN_INT);
	LOADER_LENGTH (chunk) = scanner->value.v_int64;         /* byte length */
	parse_or_return (scanner, ')');
	g_free (LOADER_FILE (chunk));
	LOADER_FILE (chunk) = NULL;
        LOADER_TYPE (chunk) = VORBIS_LINK_MAGIC;
	break;
      case BSEWAVE_TOKEN_FILE:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
        g_free (LOADER_FILE (chunk));
	LOADER_FILE (chunk) = g_strdup (scanner->value.v_string);
        LOADER_TYPE (chunk) = AUTO_FILE_MAGIC;
	break;
      case BSEWAVE_TOKEN_INDEX:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
	g_free (LOADER_INDEX (chunk));                          /* wave name */
	LOADER_INDEX (chunk) = g_strdup (scanner->value.v_string);
	break;
      case BSEWAVE_TOKEN_RAW_FILE:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
        g_free (LOADER_FILE (chunk));
	LOADER_FILE (chunk) = g_strdup (scanner->value.v_string);
        LOADER_TYPE (chunk) = RAW_FILE_MAGIC;
	break;
      case BSEWAVE_TOKEN_BOFFSET:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
        LOADER_BOFFSET (chunk) = scanner->value.v_int64;	/* byte offset */
	break;
      case BSEWAVE_TOKEN_N_VALUES:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
        LOADER_LENGTH (chunk) = scanner->value.v_int64;	        /* n_values */
	break;
      case BSEWAVE_TOKEN_RAW_LINK:
	parse_or_return (scanner, '=');
	parse_or_return (scanner, '(');
	parse_or_return (scanner, G_TOKEN_IDENTIFIER);
	if (strcmp (scanner->value.v_identifier, "binary-appendix") != 0)
	  return G_TOKEN_IDENTIFIER;
	parse_or_return (scanner, G_TOKEN_INT);
	LOADER_BOFFSET (chunk) = scanner->value.v_int64;        /* byte offset */
	parse_or_return (scanner, G_TOKEN_INT);
	LOADER_LENGTH (chunk) = scanner->value.v_int64;         /* byte length */
	parse_or_return (scanner, ')');
        LOADER_TYPE (chunk) = RAW_LINK_MAGIC;
	break;
      case BSEWAVE_TOKEN_BYTE_ORDER:
        parse_or_return (scanner, '=');
        g_scanner_get_next_token (scanner);
        switch (GslWaveTokenType (scanner->token))
          {
          case BSEWAVE_TOKEN_LITTLE_ENDIAN:
          case BSEWAVE_TOKEN_LITTLE:        LOADER_BYTE_ORDER (chunk) = G_LITTLE_ENDIAN; break;
          case BSEWAVE_TOKEN_BIG_ENDIAN:
          case BSEWAVE_TOKEN_BIG:           LOADER_BYTE_ORDER (chunk) = G_BIG_ENDIAN;    break;
          default:                          return BSEWAVE_TOKEN_LITTLE_ENDIAN;
          }
        break;
      case BSEWAVE_TOKEN_FORMAT:
        parse_or_return (scanner, '=');
        g_scanner_get_next_token (scanner);
        switch (GslWaveTokenType (scanner->token))
          {
          case BSEWAVE_TOKEN_SIGNED_8:      LOADER_FORMAT (chunk) = GSL_WAVE_FORMAT_SIGNED_8;    break;
          case BSEWAVE_TOKEN_SIGNED_12:     LOADER_FORMAT (chunk) = GSL_WAVE_FORMAT_SIGNED_12;   break;
          case BSEWAVE_TOKEN_SIGNED_16:     LOADER_FORMAT (chunk) = GSL_WAVE_FORMAT_SIGNED_16;   break;
          case BSEWAVE_TOKEN_UNSIGNED_8:    LOADER_FORMAT (chunk) = GSL_WAVE_FORMAT_UNSIGNED_8;  break;
          case BSEWAVE_TOKEN_UNSIGNED_12:   LOADER_FORMAT (chunk) = GSL_WAVE_FORMAT_UNSIGNED_12; break;
          case BSEWAVE_TOKEN_UNSIGNED_16:   LOADER_FORMAT (chunk) = GSL_WAVE_FORMAT_UNSIGNED_16; break;
          case BSEWAVE_TOKEN_ALAW:          LOADER_FORMAT (chunk) = GSL_WAVE_FORMAT_ALAW;        break;
          case BSEWAVE_TOKEN_ULAW:          LOADER_FORMAT (chunk) = GSL_WAVE_FORMAT_ULAW;        break;
          case BSEWAVE_TOKEN_FLOAT:         LOADER_FORMAT (chunk) = GSL_WAVE_FORMAT_FLOAT;       break;
          default:                          return BSEWAVE_TOKEN_SIGNED_16;
          }
        break;
      case BSEWAVE_TOKEN_MIX_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:	chunk->mix_freq = scanner->value.v_float;	break;
	  case G_TOKEN_INT:	chunk->mix_freq = scanner->value.v_int64;	break;
	  default:		return G_TOKEN_FLOAT;
	  }
	break;
      }
  while (TRUE);
}
static uint
bsewave_parse_wave_dsc (GScanner   *scanner,
			WaveDsc    *dsc,
			const char *wave_name)
{
  parse_or_return (scanner, '{');
  do
    switch (GslWaveTokenType (g_scanner_get_next_token (scanner)))
      {
	uint i;
        int token;
        char *key;
      case '}':
	return G_TOKEN_NONE;
      default:
	return '}';
      case BSEWAVE_TOKEN_N_CHANNELS:
        if (dsc->wdsc.n_channels != 0)
          return '}';   /* may specify n_channels only once */
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_INT);
	dsc->wdsc.n_channels = scanner->value.v_int64;
	break;
      case BSEWAVE_TOKEN_NAME:
	if (dsc->wdsc.name)
	  return '}';
	parse_or_return (scanner, '=');
	parse_or_return (scanner, G_TOKEN_STRING);
	if (wave_name)
	  {
	    if (strcmp (wave_name, scanner->value.v_string) == 0)
	      dsc->wdsc.name = g_strdup (scanner->value.v_string);
	    else
	      return bsewave_skip_rest_statement (scanner, 1);
	  }
	else
	  dsc->wdsc.name = g_strdup (scanner->value.v_string);
	break;
      case BSEWAVE_TOKEN_BYTE_ORDER:
	parse_or_return (scanner, '=');
	token = g_scanner_get_next_token (scanner);
	switch (token)
	  {
	  case BSEWAVE_TOKEN_LITTLE_ENDIAN:
	  case BSEWAVE_TOKEN_LITTLE:        dsc->dfl_byte_order = G_LITTLE_ENDIAN; break;
	  case BSEWAVE_TOKEN_BIG_ENDIAN:
	  case BSEWAVE_TOKEN_BIG:	    dsc->dfl_byte_order = G_BIG_ENDIAN;    break;
	  default:			    return BSEWAVE_TOKEN_LITTLE_ENDIAN;
	  }
	break;
      case BSEWAVE_TOKEN_FORMAT:
	parse_or_return (scanner, '=');
	token = g_scanner_get_next_token (scanner);
	switch (token)
	  {
	  case BSEWAVE_TOKEN_SIGNED_8:	    dsc->dfl_format = GSL_WAVE_FORMAT_SIGNED_8;    break;
	  case BSEWAVE_TOKEN_SIGNED_12:     dsc->dfl_format = GSL_WAVE_FORMAT_SIGNED_12;   break;
	  case BSEWAVE_TOKEN_SIGNED_16:     dsc->dfl_format = GSL_WAVE_FORMAT_SIGNED_16;   break;
	  case BSEWAVE_TOKEN_UNSIGNED_8:    dsc->dfl_format = GSL_WAVE_FORMAT_UNSIGNED_8;  break;
	  case BSEWAVE_TOKEN_UNSIGNED_12:   dsc->dfl_format = GSL_WAVE_FORMAT_UNSIGNED_12; break;
	  case BSEWAVE_TOKEN_UNSIGNED_16:   dsc->dfl_format = GSL_WAVE_FORMAT_UNSIGNED_16; break;
	  case BSEWAVE_TOKEN_ALAW:	    dsc->dfl_format = GSL_WAVE_FORMAT_ALAW;	   break;
	  case BSEWAVE_TOKEN_ULAW:	    dsc->dfl_format = GSL_WAVE_FORMAT_ULAW;	   break;
	  case BSEWAVE_TOKEN_FLOAT:	    dsc->dfl_format = GSL_WAVE_FORMAT_FLOAT;       break;
	  default:			    return BSEWAVE_TOKEN_SIGNED_16;
	  }
	break;
      case BSEWAVE_TOKEN_MIX_FREQ:
	parse_or_return (scanner, '=');
	switch (g_scanner_get_next_token (scanner))
	  {
	  case G_TOKEN_FLOAT:   dsc->dfl_mix_freq = scanner->value.v_float;	break;
	  case G_TOKEN_INT:     dsc->dfl_mix_freq = scanner->value.v_int64;	break;
	  default:		return G_TOKEN_FLOAT;
	  }
	break;
      case BSEWAVE_TOKEN_XINFO:
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
        dsc->wdsc.xinfos = bse_xinfos_add_value (dsc->wdsc.xinfos, key, scanner->value.v_string);
        g_free (key);
        break;
      case BSEWAVE_TOKEN_CHUNK:
        if (dsc->wdsc.n_channels < 1)   /* must have n_channels specification */
          {
            g_scanner_warn (scanner, "wave with unspecified number of channels, assuming 1 (mono)");
            dsc->wdsc.n_channels = 1;
          }
	if (g_scanner_peek_next_token (scanner) != '{')
	  parse_or_return (scanner, '{');
	i = dsc->wdsc.n_chunks++;
	dsc->wdsc.chunks = (BseWaveChunkDsc*) g_realloc (dsc->wdsc.chunks, sizeof (dsc->wdsc.chunks[0]) * dsc->wdsc.n_chunks);
	memset (dsc->wdsc.chunks + i, 0, sizeof (dsc->wdsc.chunks[0]) * 1);
	dsc->wdsc.chunks[i].mix_freq = 0;
	dsc->wdsc.chunks[i].osc_freq = -1;
	token = bsewave_parse_chunk_dsc (scanner, dsc->wdsc.chunks + i);
	if (token != G_TOKEN_NONE)
	  return token;
        if (dsc->wdsc.chunks[i].mix_freq <= 0)
          dsc->wdsc.chunks[i].mix_freq = dsc->dfl_mix_freq;
        if (dsc->wdsc.chunks[i].osc_freq <= 0)
          {
            /* try to set osc-freq from xinfos */
            float osc_freq = bse_xinfos_get_float (dsc->wdsc.chunks[i].xinfos, "osc-freq");
            if (osc_freq == 0 && bse_xinfos_get_value (dsc->wdsc.chunks[i].xinfos, "midi-note")) /* also matches midi-note=0 */
              {
                SfiNum midi_note = bse_xinfos_get_num (dsc->wdsc.chunks[i].xinfos, "midi-note");
                osc_freq = 440.0 /* MIDI standard pitch */ * pow (BSE_2_POW_1_DIV_12, midi_note - 69 /* MIDI kammer note */);
              }
            if (osc_freq > 0)
              dsc->wdsc.chunks[i].osc_freq = osc_freq;
          }
	if (dsc->wdsc.chunks[i].osc_freq <= 0)
	  g_scanner_error (scanner, "wave chunk \"%s\" without oscilator frequency: mix_freq=%f osc_freq=%f",
			   LOADER_FILE (&dsc->wdsc.chunks[i]) ? (char*) LOADER_FILE (&dsc->wdsc.chunks[i]) : "",
			   dsc->wdsc.chunks[i].mix_freq, dsc->wdsc.chunks[i].osc_freq);
        if (dsc->wdsc.chunks[i].osc_freq >= dsc->wdsc.chunks[i].mix_freq / 2.)
          g_scanner_error (scanner, "wave chunk \"%s\" with invalid mixing/oscilator frequency: mix_freq=%f osc_freq=%f",
			   LOADER_FILE (&dsc->wdsc.chunks[i]) ? (char*) LOADER_FILE (&dsc->wdsc.chunks[i]) : "",
                           dsc->wdsc.chunks[i].mix_freq, dsc->wdsc.chunks[i].osc_freq);
        break;
      }
  while (TRUE);
}
static void
bsewave_wave_dsc_free (WaveDsc *dsc)
{
  uint i;
  for (i = 0; i < dsc->wdsc.n_chunks; i++)
    {
      g_strfreev (dsc->wdsc.chunks[i].xinfos);
      g_free (LOADER_FILE (&dsc->wdsc.chunks[i]));
      g_free (LOADER_INDEX (&dsc->wdsc.chunks[i]));
    }
  g_free (dsc->wdsc.chunks);
  g_free (dsc->wdsc.name);
  sfi_delete_struct (WaveDsc, dsc);
}
static BseWaveDsc*
bsewave_load_wave_dsc (void            *data,
		       BseWaveFileInfo *file_info,
		       uint             nth_wave,
		       BseErrorType    *error_p)
{
  uint token, i;
  int fd = open (file_info->file_name, O_RDONLY);
  if (fd < 0)
    {
      *error_p = gsl_error_from_errno (errno, BSE_ERROR_FILE_OPEN_FAILED);
      return NULL;
    }
  GScanner *scanner = g_scanner_new64 (sfi_storage_scanner_config);
  scanner->config->cpair_comment_single = (char*) "#\n";
  scanner->input_name = file_info->file_name;
  g_scanner_input_file (scanner, fd);
  for (i = BSEWAVE_TOKEN_WAVE; i < BSEWAVE_TOKEN_LAST; i++)
    g_scanner_scope_add_symbol (scanner, 0, bsewave_tokens[i - BSEWAVE_TOKEN_WAVE], GUINT_TO_POINTER (i));
  WaveDsc *dsc = sfi_new_struct0 (WaveDsc, 1);
  dsc->wdsc.name = NULL;
  dsc->wdsc.n_chunks = 0;
  dsc->wdsc.chunks = NULL;
  dsc->wdsc.n_channels = 0;
  dsc->wdsc.xinfos = NULL;
  dsc->dfl_format = GSL_WAVE_FORMAT_SIGNED_16;
  dsc->dfl_byte_order = G_LITTLE_ENDIAN;
  dsc->dfl_mix_freq = 44100;
  if (GslWaveTokenType (g_scanner_get_next_token (scanner)) != BSEWAVE_TOKEN_WAVE)
    token = BSEWAVE_TOKEN_WAVE;
  else
    token = bsewave_parse_wave_dsc (scanner, dsc, file_info->waves[nth_wave].name);
  if (token != G_TOKEN_NONE || scanner->parse_errors) // FIXME: untested/broken branch?
    {
      bsewave_wave_dsc_free (dsc);
      dsc = NULL;
      if (!scanner->parse_errors)
	g_scanner_unexp_token (scanner, GTokenType (token), "identifier", "keyword", NULL, "discarding wave", TRUE);
    }
  else
    {
      if (dsc->wdsc.name)       /* found and parsed the correctly named wave */
	{
          if (dsc->wdsc.n_channels > 2)
            {
              g_scanner_error (scanner, "waves with n-channels > 2 (%d) are not currently supported", dsc->wdsc.n_channels);
              bsewave_wave_dsc_free (dsc);
              dsc = NULL;
            }
	}
      else
	{
	  /* got invalid/wrong wave */
	  bsewave_wave_dsc_free (dsc);
	  dsc = NULL;
        }
    }
  g_scanner_destroy (scanner);
  close (fd);
  return dsc ? &dsc->wdsc : NULL;
}
static void
bsewave_free_wave_dsc (void       *data,
		       BseWaveDsc *wave_dsc)
{
  WaveDsc *dsc = (WaveDsc*) wave_dsc;
  bsewave_wave_dsc_free (dsc);
}
static GslDataHandle*
bsewave_load_singlechunk_wave (BseWaveFileInfo *fi,
			       const char      *wave_name,
                               float            osc_freq,
			       BseErrorType    *error_p,
                               uint            *n_channelsp)
{
  BseWaveDsc *wdsc;
  uint i;
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
  wdsc = bse_wave_dsc_load (fi, i, FALSE, error_p);
  if (!wdsc)
    return NULL;
  if (wdsc->n_chunks == 1)
    {
      *n_channelsp = wdsc->n_channels;
      GslDataHandle *dhandle = bse_wave_handle_create (wdsc, 0, error_p);
      if (dhandle && osc_freq > 0)
        {
          char **xinfos = NULL;
          xinfos = bse_xinfos_add_float (xinfos, "osc-freq", osc_freq);
          GslDataHandle *tmp_handle = gsl_data_handle_new_add_xinfos (dhandle, xinfos);
          g_strfreev (xinfos);
          gsl_data_handle_unref (dhandle);
          dhandle = tmp_handle;
        }
      bse_wave_dsc_free (wdsc);
      return dhandle;
    }
  /* this is ridiculous, letting the chunk of a wave
   * point to a wave with multiple chunks...
   */
  bse_wave_dsc_free (wdsc);
  *error_p = BSE_ERROR_FORMAT_INVALID;
  return NULL;
}
static GslDataHandle*
bsewave_create_chunk_handle (void         *data,
			     BseWaveDsc   *wave_dsc,
			     uint          nth_chunk,
			     BseErrorType *error_p)
{
  WaveDsc *dsc = (WaveDsc*) wave_dsc;
  FileInfo *fi = (FileInfo*) dsc->wdsc.file_info;
  BseWaveChunkDsc *chunk = wave_dsc->chunks + nth_chunk;
  GslDataHandle *dhandle = NULL;
  switch (LOADER_TYPE (chunk))
    {
      char *string;
    case AUTO_FILE_MAGIC:
      {
        *error_p = BSE_ERROR_IO;
        /* construct chunk file name from (hopefully) relative path */
        if (g_path_is_absolute ((char*) LOADER_FILE (chunk)))
          string = g_strdup ((char*) LOADER_FILE (chunk));
        else
          string = g_strdup_printf ("%s%c%s", fi->cwd, G_DIR_SEPARATOR, (char*) LOADER_FILE (chunk));
        /* try to load the chunk via registered loaders */
        BseWaveFileInfo *cfi = bse_wave_file_info_load (string, error_p);
        if (cfi)
          {
            uint nch = 0;
            /* FIXME: there's a potential attack here, in letting a single chunk
             * wave's chunk point to its own wave. this'll trigger recursions until
             * stack overflow
             */
            dhandle = bsewave_load_singlechunk_wave (cfi, (char*) LOADER_INDEX (chunk), chunk->osc_freq, error_p, &nch);
            if (dhandle && chunk->xinfos)
              {
                GslDataHandle *tmp_handle = dhandle;
                dhandle = gsl_data_handle_new_add_xinfos (dhandle, chunk->xinfos);
                gsl_data_handle_unref (tmp_handle);
              }
            if (dhandle && nch != dsc->wdsc.n_channels)
              {
                *error_p = BSE_ERROR_WRONG_N_CHANNELS;
                gsl_data_handle_unref (dhandle);
                dhandle = NULL;
              }
            bse_wave_file_info_unref (cfi);
          }
        g_free (string);
      }
      break;
    case RAW_FILE_MAGIC:
      /* construct chunk file name from (hopefully) relative path */
      if (g_path_is_absolute ((char*) LOADER_FILE (chunk)))
        string = g_strdup ((char*) LOADER_FILE (chunk));
      else
        string = g_strdup_printf ("%s%c%s", fi->cwd, G_DIR_SEPARATOR, (char*) LOADER_FILE (chunk));
      /* try to load a raw sample */
      dhandle = gsl_wave_handle_new (string,			/* file name */
				     dsc->wdsc.n_channels,
				     GslWaveFormatType (LOADER_FORMAT (chunk) ? LOADER_FORMAT (chunk) : dsc->dfl_format),
				     LOADER_BYTE_ORDER (chunk) ? LOADER_BYTE_ORDER (chunk) : dsc->dfl_byte_order,
                                     chunk->mix_freq > 0 ? chunk->mix_freq : dsc->dfl_mix_freq,
                                     chunk->osc_freq,
				     LOADER_BOFFSET (chunk),    /* byte offset */
                                     LOADER_LENGTH (chunk) ? LOADER_LENGTH (chunk) : -1,        /* n_values */
                                     chunk->xinfos);
      *error_p = dhandle ? BSE_ERROR_NONE : BSE_ERROR_IO;
      g_free (string);
      break;
    case RAW_LINK_MAGIC:
      if (LOADER_LENGTH (chunk))        /* inlined binary data */
	{
	  dhandle = gsl_wave_handle_new_zoffset (fi->wfi.file_name,
						 dsc->wdsc.n_channels,
                                                 GslWaveFormatType (LOADER_FORMAT (chunk) ? LOADER_FORMAT (chunk) : dsc->dfl_format),
                                                 LOADER_BYTE_ORDER (chunk) ? LOADER_BYTE_ORDER (chunk) : dsc->dfl_byte_order,
                                                 chunk->mix_freq > 0 ? chunk->mix_freq : dsc->dfl_mix_freq,
                                                 chunk->osc_freq,
                                                 LOADER_BOFFSET (chunk),        /* byte offset */
						 LOADER_LENGTH (chunk),         /* byte length */
                                                 chunk->xinfos);
	  *error_p = dhandle ? BSE_ERROR_NONE : BSE_ERROR_IO;
	}
      else
        *error_p = BSE_ERROR_WAVE_NOT_FOUND;
      break;
    case VORBIS_LINK_MAGIC:
      if (LOADER_LENGTH (chunk))        /* inlined binary data */
	{
          *error_p = BSE_ERROR_IO;
          uint vnch = 0;
	  dhandle = gsl_data_handle_new_ogg_vorbis_zoffset (fi->wfi.file_name,
                                                            chunk->osc_freq,
                                                            LOADER_BOFFSET (chunk),     /* byte offset */
                                                            LOADER_LENGTH (chunk),      /* byte length */
                                                            &vnch, NULL);
          if (dhandle && vnch != dsc->wdsc.n_channels)
            {
              *error_p = BSE_ERROR_WRONG_N_CHANNELS;
              gsl_data_handle_unref (dhandle);
              dhandle = NULL;
            }
          if (dhandle && chunk->xinfos)
            {
              GslDataHandle *tmp_handle = dhandle;
              dhandle = gsl_data_handle_new_add_xinfos (dhandle, chunk->xinfos);
              gsl_data_handle_unref (tmp_handle);
            }
	}
      else
        *error_p = BSE_ERROR_WAVE_NOT_FOUND;
      break;
    default:    /* no file_name and no loader specified */
      *error_p = BSE_ERROR_FORMAT_UNKNOWN;
      break;
    }
  if (dhandle)
    *error_p = BSE_ERROR_NONE;
  return dhandle;
}
void
_gsl_init_loader_gslwave (void)
{
  static const char *file_exts[] = { "bsewave", NULL, };
  static const char *mime_types[] = { "audio/x-bsewave", NULL, };
  static const char *magics[] = { "0 string #BseWave1", NULL, };
  static BseLoader loader = {
    "BseWave",
    file_exts,
    mime_types,
    BseLoaderFlags (0),	/* flags */
    magics,
    0,  /* priority */
    NULL,
    bsewave_load_file_info,
    bsewave_free_file_info,
    bsewave_load_wave_dsc,
    bsewave_free_wave_dsc,
    bsewave_create_chunk_handle,
  };
  static gboolean initialized = FALSE;
  g_assert (initialized == FALSE);
  initialized = TRUE;
  bse_loader_register (&loader);
}
