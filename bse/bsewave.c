/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bsewave.h"

#include	"bsestorage.h"
#include	"bseprocedure.h"
#include	"gslwavechunk.h"
#include	"gsldatahandle.h"
#include	"bseserver.h"
#include        <gsl/gslloader.h>


enum {
  PARAM_0,
  PARAM_LOCATOR_SET,
  PARAM_FILE_NAME,
  PARAM_WAVE_NAME
};

typedef struct
{
  GslWaveChunk	*wchunk;
  gchar		*file_name;
  gchar		*wave_name;
  gboolean	 locator_overrides;
} WaveChunkUrl;

typedef struct
{
  guint          n_channels;
  gfloat         osc_freq;
  gfloat         mix_freq;
  guint          jump_loop : 1;
  guint          ping_pong_loop : 1;
  GslLong        loop_count;
  GslLong        loop_start;
  GslLong        loop_end;
  GslDataHandle *wave_handle;
} ParsedWaveChunk;


/* --- prototypes --- */
static void	    bse_wave_class_init		(BseWaveClass	*class);
static void	    bse_wave_init		(BseWave		*wave);
static void	    bse_wave_dispose		(GObject		*object);
static void	    bse_wave_set_property	(BseWave		*wave,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec);
static void	    bse_wave_get_property	(BseWave		*wave,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec);
static void         bse_wave_do_store_private	(BseObject		*object,
						 BseStorage		*storage);
static GTokenType   bse_wave_do_restore		(BseObject		*object,
						 BseStorage		*storage);
static BseTokenType bse_wave_do_restore_private	(BseObject		*object,
						 BseStorage		*storage);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static GQuark      quark_n_channels = 0;
static GQuark      quark_loop = 0;
static GQuark      quark_ping_pong_loop = 0;
static GQuark      quark_wave_handle = 0;
static GQuark      quark_load_wave = 0;
static GQuark      quark_set_locator = 0;
static GQuark      quark_wave_chunk = 0;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseWave)
{
  static const GTypeInfo wave_info = {
    sizeof (BseWaveClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_wave_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseWave),
    0  /* n_preallocs */,
    (GInstanceInitFunc) bse_wave_init,
  };
  
  return bse_type_register_static (BSE_TYPE_SOURCE,
				   "BseWave",
				   "BSE wave type",
				   &wave_info);
}

static void
bse_wave_class_init (BseWaveClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_wave_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_wave_get_property;
  gobject_class->dispose = bse_wave_dispose;

  object_class->store_private = bse_wave_do_store_private;
  object_class->restore = bse_wave_do_restore;
  object_class->restore_private = bse_wave_do_restore_private;
  
  quark_n_channels = g_quark_from_static_string ("n-channels");
  quark_loop = g_quark_from_static_string ("loop");
  quark_ping_pong_loop = g_quark_from_static_string ("ping-pong-loop");
  quark_wave_handle = g_quark_from_static_string ("wave-handle");
  quark_load_wave = g_quark_from_static_string ("load-wave");
  quark_set_locator = g_quark_from_static_string ("set-locator");
  quark_wave_chunk = g_quark_from_static_string ("wave-chunk");

  bse_object_class_add_param (object_class, "Locator",
			      PARAM_LOCATOR_SET,
			      g_param_spec_boolean ("locator_set", "Locator Set", NULL,
						    FALSE,
						    BSE_PARAM_GUI & ~G_PARAM_WRITABLE));
  bse_object_class_add_param (object_class, "Locator",
			      PARAM_FILE_NAME,
			      g_param_spec_string ("file_name", "File Name", NULL,
						   NULL,
						   BSE_PARAM_GUI & ~G_PARAM_WRITABLE));
  bse_object_class_add_param (object_class, "Locator",
			      PARAM_WAVE_NAME,
			      g_param_spec_string ("wave_name", "Wave Name", NULL,
						   NULL,
						   BSE_PARAM_GUI & ~G_PARAM_WRITABLE));
}

static void
bse_wave_init (BseWave *wave)
{
  wave->locator_set = FALSE;
  wave->file_name = NULL;
  wave->wave_name = NULL;
  wave->wave_chunk_urls = NULL;
  wave->n_wchunks = 0;
  wave->wave_chunks = NULL;
  wave->request_count = 0;
  wave->index_dirty = FALSE;
  wave->index_list = NULL;
  BSE_OBJECT_SET_FLAGS (wave, BSE_ITEM_FLAG_STORAGE_IGNORE);
}

static void
bse_wave_set_property (BseWave	  *wave,
		       guint       param_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wave, param_id, pspec);
      break;
    }
}

static void
bse_wave_get_property (BseWave	  *wave,
		       guint	   param_id,
		       GValue	  *value,
		       GParamSpec *pspec)
{
  switch (param_id)
    {
    case PARAM_LOCATOR_SET:
      g_value_set_boolean (value, wave->locator_set);
      break;
    case PARAM_FILE_NAME:
      g_value_set_string (value, wave->file_name);
      break;
    case PARAM_WAVE_NAME:
      g_value_set_string (value, wave->wave_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wave, param_id, pspec);
      break;
    }
}

static void
nuke_wave_urls (BseWave *wave)
{
  GSList *slist;

  for (slist = wave->wave_chunk_urls; slist; slist = slist->next)
    {
      WaveChunkUrl *url = slist->data;

      g_free (url->file_name);
      g_free (url->wave_name);
      g_free (url);
    }
  g_slist_free (wave->wave_chunk_urls);
  wave->wave_chunk_urls = NULL;
}

static void
bse_wave_dispose (GObject *object)
{
  BseWave *wave = BSE_WAVE (object);

  nuke_wave_urls (wave);
  
  while (wave->wave_chunks)
    bse_wave_remove_chunk (wave, wave->wave_chunks->data);
  g_return_if_fail (wave->index_list == NULL);

  g_free (wave->file_name);
  g_free (wave->wave_name);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GslWaveChunk*
bse_wave_lookup_chunk (BseWave *wave,
		       gfloat   osc_freq,
		       gfloat   mix_freq)
{
  BseWaveIndex *index;
  GslWaveChunk *wchunk;

  g_return_val_if_fail (BSE_IS_WAVE (wave), NULL);

  bse_wave_request_index (wave);
  index = bse_wave_get_index_for_modules (wave);
  wchunk = index ? bse_wave_index_lookup_best (index, osc_freq) : NULL;
  bse_wave_drop_index (wave);

  return wchunk;
}

void
bse_wave_remove_chunk (BseWave      *wave,
		       GslWaveChunk *wchunk)
{
  GSList *slist;

  g_return_if_fail (BSE_IS_WAVE (wave));
  g_return_if_fail (wchunk != NULL);

  wave->wave_chunks = g_slist_remove (wave->wave_chunks, wchunk);
  wave->n_wchunks--;

  for (slist = wave->wave_chunk_urls; slist; slist = slist->next)
    {
      WaveChunkUrl *url = slist->data;

      if (url->wchunk == wchunk)
	{
	  g_free (url->file_name);
	  g_free (url->wave_name);
	  g_free (url);
	  wave->wave_chunk_urls = g_slist_remove (wave->wave_chunk_urls, url);
	  break;
	}
    }

  gsl_wave_chunk_unref (wchunk);
  wave->index_dirty = TRUE;

  if (wave->n_wchunks)
    BSE_OBJECT_UNSET_FLAGS (wave, BSE_ITEM_FLAG_STORAGE_IGNORE);
  else
    BSE_OBJECT_SET_FLAGS (wave, BSE_ITEM_FLAG_STORAGE_IGNORE);
}

static gint
wchunk_cmp (gconstpointer a,
	    gconstpointer b)
{
  const GslWaveChunk *w1 = a;
  const GslWaveChunk *w2 = b;

  return w1->osc_freq < w2->osc_freq ? -1 : w1->osc_freq > w2->osc_freq;
}

void
bse_wave_add_chunk (BseWave      *wave,
		    GslWaveChunk *wchunk)
{
  WaveChunkUrl *url;

  g_return_if_fail (BSE_IS_WAVE (wave));
  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (wchunk->dcache != NULL);

  wave->wave_chunks = g_slist_insert_sorted (wave->wave_chunks, gsl_wave_chunk_ref (wchunk), wchunk_cmp);
  wave->n_wchunks++;

  url = g_new0 (WaveChunkUrl, 1);
  url->wchunk = wchunk;
  url->file_name = NULL;
  url->wave_name = NULL;
  url->locator_overrides = FALSE;
  wave->wave_chunk_urls = g_slist_prepend (wave->wave_chunk_urls, url);
  wave->index_dirty = TRUE;
  BSE_OBJECT_UNSET_FLAGS (wave, BSE_ITEM_FLAG_STORAGE_IGNORE);
}

void
bse_wave_add_chunk_with_locator (BseWave      *wave,
				 GslWaveChunk *wchunk,
				 const gchar  *file_name,
				 const gchar  *wave_name)
{
  WaveChunkUrl *url;

  g_return_if_fail (BSE_IS_WAVE (wave));
  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (wchunk->dcache != NULL);
  g_return_if_fail (file_name != NULL);
  g_return_if_fail (wave_name != NULL);

  wave->wave_chunks = g_slist_insert_sorted (wave->wave_chunks, gsl_wave_chunk_ref (wchunk), wchunk_cmp);
  wave->n_wchunks++;
  url = g_new0 (WaveChunkUrl, 1);
  url->wchunk = wchunk;
  url->file_name = g_strdup (file_name);
  url->wave_name = g_strdup (wave_name);
  url->locator_overrides = FALSE;
  wave->wave_chunk_urls = g_slist_prepend (wave->wave_chunk_urls, url);
  wave->index_dirty = TRUE;
  BSE_OBJECT_UNSET_FLAGS (wave, BSE_ITEM_FLAG_STORAGE_IGNORE);
}

void
bse_wave_set_locator (BseWave     *wave,
		      const gchar *file_name,
		      const gchar *wave_name)
{
  GSList *slist;

  g_return_if_fail (BSE_IS_WAVE (wave));
  g_return_if_fail (file_name != NULL);
  g_return_if_fail (wave_name != NULL);
  g_return_if_fail (wave->locator_set == FALSE);

  wave->locator_set = TRUE;
  wave->file_name = g_strdup (file_name);
  wave->wave_name = g_strdup (wave_name);
  for (slist = wave->wave_chunk_urls; slist; slist = slist->next)
    {
      WaveChunkUrl *url = slist->data;

      url->locator_overrides = TRUE;
    }

  g_object_freeze_notify (G_OBJECT (wave));
  g_object_notify (G_OBJECT (wave), "locator_set");
  g_object_notify (G_OBJECT (wave), "file_name");
  g_object_notify (G_OBJECT (wave), "wave_name");
  g_object_thaw_notify (G_OBJECT (wave));
}

#define parse_or_return(scanner, token) { guint _t = (token); \
                                          if (g_scanner_get_next_token (scanner) != _t) \
                                            return _t; \
                                        }

static void
bse_wave_do_store_private (BseObject  *object,
			   BseStorage *storage)
{
  BseWave *wave = BSE_WAVE (object);
  GSList *slist;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  if (wave->locator_set && !BSE_STORAGE_SELF_CONTAINED (storage))
    {
      bse_storage_break (storage);
      bse_storage_printf (storage, "(load-wave \"%s\" \"%s\"", wave->file_name, wave->wave_name);

      if (wave->wave_chunk_urls)
	{
	  GSList *slist;
	  guint i = 0;

	  for (slist = wave->wave_chunk_urls; slist; slist = slist->next)
	    {
	      WaveChunkUrl *url = slist->data;

	      if (url->locator_overrides)
		continue;

	      if (i == 0)
		{
		  bse_storage_printf (storage, " skip");
		  bse_storage_push_level (storage);
		}

	      if (i++ % 3 == 0)
		bse_storage_break (storage);
	      else
		bse_storage_putc (storage, ' ');
	      bse_storage_printf (storage, "%g", url->wchunk->osc_freq);
	    }
	  if (i != 0)
	    bse_storage_pop_level (storage);
	}
      bse_storage_putc (storage, ')');
      bse_storage_break (storage);
      bse_storage_printf (storage, "(set-locator \"%s\" \"%s\")", wave->file_name, wave->wave_name);
    }

  for (slist = wave->wave_chunk_urls; slist; slist = slist->next)
    {
      WaveChunkUrl *url = slist->data;

      if (url->locator_overrides && wave->locator_set && !BSE_STORAGE_SELF_CONTAINED (storage))
	continue;

      if (url->file_name && url->wave_name && !BSE_STORAGE_SELF_CONTAINED (storage))
	{
	  bse_storage_break (storage);
	  bse_storage_printf (storage, "(load-wave \"%s\" \"%s\" list %g)",
			      url->file_name, url->wave_name,
			      url->wchunk->osc_freq);
	}
      else	/* self-contained wave storage */
	{
	  GslErrorType error = gsl_data_handle_open (url->wchunk->dcache->dhandle);
	  if (error)
	    {
	      bse_storage_warn (storage, "failed to open data handle (%s): %s",
				gsl_data_handle_name (url->wchunk->dcache->dhandle),
				gsl_strerror (error));
	      continue;
	    }
          bse_storage_break (storage);
	  bse_storage_printf (storage, "(wave-chunk %g %g", url->wchunk->osc_freq, url->wchunk->mix_freq);
          bse_storage_push_level (storage);
	  if (gsl_data_handle_n_channels (url->wchunk->dcache->dhandle) > 1)
	    {
	      bse_storage_break (storage);
	      bse_storage_printf (storage, "(n-channels %u)", gsl_data_handle_n_channels (url->wchunk->dcache->dhandle));
	    }
	  switch (url->wchunk->loop_type)
	    {
	    case GSL_WAVE_LOOP_JUMP:
	      bse_storage_break (storage);
	      bse_storage_printf (storage, "(loop %lu %lu %lu)",
				  (GslLong) url->wchunk->loop_count,
				  url->wchunk->loop_first,
				  url->wchunk->loop_last);
	      break;
	    case GSL_WAVE_LOOP_PINGPONG:
	      bse_storage_break (storage);
	      bse_storage_printf (storage, "(ping-pong-loop %lu %lu %lu)",
				  (GslLong) url->wchunk->loop_count,
				  url->wchunk->loop_first,
				  url->wchunk->loop_last);
	      break;
	    case GSL_WAVE_LOOP_NONE:
	      break;
	    }
	  bse_storage_break (storage);
	  bse_storage_printf (storage, "(wave-handle ");
	  bse_storage_put_data_handle (storage,
				       gsl_data_handle_bit_depth (url->wchunk->dcache->dhandle),
				       url->wchunk->dcache->dhandle,
				       gsl_data_handle_length (url->wchunk->dcache->dhandle));
	  gsl_data_handle_close (url->wchunk->dcache->dhandle);
	  bse_storage_handle_break (storage);
	  bse_storage_putc (storage, ')');
          bse_storage_pop_level (storage);
	  bse_storage_putc (storage, ')');
	}
    }
}

static BseTokenType
parse_wave_chunk (BseWave         *wave,
		  BseStorage      *storage,
		  ParsedWaveChunk *pwchunk)

{
  GScanner *scanner = storage->scanner;
  GQuark quark;
  
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return BSE_TOKEN_UNMATCHED;

  quark = g_quark_try_string (scanner->next_value.v_string);
  if (quark == quark_n_channels)
    {
      g_scanner_get_next_token (scanner); /* eat quark identifier */
      parse_or_return (scanner, G_TOKEN_INT);
      pwchunk->n_channels = scanner->value.v_int;
      if (pwchunk->n_channels < 1)
	return G_TOKEN_INT;
    }
  else if (quark == quark_loop || quark == quark_ping_pong_loop)
    {
      g_scanner_get_next_token (scanner); /* eat quark identifier */
      pwchunk->jump_loop = quark == quark_loop;
      pwchunk->ping_pong_loop = quark == quark_ping_pong_loop;
      parse_or_return (scanner, G_TOKEN_INT);
      pwchunk->loop_count = scanner->value.v_int;
      parse_or_return (scanner, G_TOKEN_INT);
      pwchunk->loop_start = scanner->value.v_int;
      parse_or_return (scanner, G_TOKEN_INT);
      pwchunk->loop_end = scanner->value.v_int;
    }
  else if (quark == quark_wave_handle)
    {
      guint expected_token;
      
      g_scanner_get_next_token (scanner); /* eat quark identifier */
      if (pwchunk->wave_handle)
	return bse_storage_warn_skip (storage, "duplicate wave data reference");
      expected_token = bse_storage_parse_data_handle (storage,
						      pwchunk->n_channels,
						      pwchunk->osc_freq,
						      pwchunk->mix_freq,
						      &pwchunk->wave_handle);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;
      if (!pwchunk->wave_handle)
	return bse_storage_warn_skip (storage, "invalid wave data reference");
    }
  else
    return BSE_TOKEN_UNMATCHED;
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
}

BseErrorType
bse_wave_load_wave_file (BseWave     *wave,
			 const gchar *file_name,
			 const gchar *wave_name,
			 GDArray     *list_array,
			 GDArray     *skip_array)
{
  BseErrorType error = BSE_ERROR_NONE;
  GslWaveFileInfo *fi;

  g_return_val_if_fail (BSE_IS_WAVE (wave), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (wave_name != NULL, BSE_ERROR_INTERNAL);

  fi = gsl_wave_file_info_load (file_name, &error);
  if (fi)
    {
      guint i;

      for (i = 0; i < fi->n_waves; i++)
	if (strcmp (wave_name, fi->waves[i].name) == 0)
	  break;
      if (i < fi->n_waves)
	{
	  GslWaveDsc *wdsc = gsl_wave_dsc_load (fi, i, &error);

	  if (wdsc)
	    {
	      for (i = 0; i < wdsc->n_chunks; i++)
		if (bse_darrays_match_freq (wdsc->chunks[i].osc_freq, list_array, skip_array))
		  {
		    BseErrorType tmp_error;
		    GslWaveChunk *wchunk = gsl_wave_chunk_create (wdsc, i, &tmp_error);

		    if (!wchunk)
		      {
			error = tmp_error;
			g_message ("wave \"%s\": failed to load wave chunk (%f/%f): %s", // FIXME
				   wdsc->name, wdsc->chunks[i].osc_freq, wdsc->chunks[i].mix_freq,
				   bse_error_blurb (error));
		      }
		    else
		      bse_wave_add_chunk (wave, wchunk);
		  }
	      gsl_wave_dsc_free (wdsc);
	    }
	}
      else
	error = GSL_ERROR_NOT_FOUND;
    }
  else if (!g_file_test (file_name, G_FILE_TEST_IS_REGULAR))
    error = BSE_ERROR_FILE_NOT_FOUND;
  else if (!g_file_test (file_name, G_FILE_TEST_IS_REGULAR)) /* FIXME: READABLE */
    error = BSE_ERROR_PERMS;
  else
    error = BSE_ERROR_IO;

  return error;
}

static BseTokenType
bse_wave_do_restore_private (BseObject  *object,
			     BseStorage *storage)
{
  BseWave *wave = BSE_WAVE (object);
  GScanner *scanner = storage->scanner;
  GTokenType expected_token = BSE_TOKEN_UNMATCHED;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore_private)
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage);

  /* support storage commands */
  if (expected_token == BSE_TOKEN_UNMATCHED &&
      g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER)
    {
      GQuark quark = g_quark_try_string (scanner->next_value.v_identifier);

      if (quark == quark_load_wave)
	{
	  GDArray *skip_list, *load_list, *array;
	  gchar *file_name, *wave_name;
	  BseErrorType error;
	  
	  g_scanner_get_next_token (scanner); /* eat quark identifier */
	  parse_or_return (scanner, G_TOKEN_STRING);
	  file_name = g_strdup (scanner->value.v_string);
	  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
	    {
	      g_free (file_name);
	      return G_TOKEN_STRING;
	    }
	  wave_name = g_strdup (scanner->value.v_string);
	  skip_list = g_darray_new (1024);
	  load_list = g_darray_new (1024);
	  while (g_scanner_get_next_token (scanner) != ')')
	    {
	      if (scanner->token == G_TOKEN_IDENTIFIER)
		{
		  if (strcmp (scanner->value.v_identifier, "list") == 0)
		    array = load_list;
		  else if (strcmp (scanner->value.v_identifier, "skip") == 0)
		    array = skip_list;
		  else
		    {
		      expected_token = G_TOKEN_IDENTIFIER; /* want _valid_ identifier */
		      goto out_of_load_wave;
		    }
		  g_scanner_peek_next_token (scanner);
		  if (scanner->next_token != G_TOKEN_INT && scanner->next_token != G_TOKEN_FLOAT)
		    {
		      g_scanner_get_next_token (scanner); /* eat invalid token */
		      expected_token = G_TOKEN_FLOAT;
		      goto out_of_load_wave;
		    }
		  while (g_scanner_peek_next_token (scanner) == G_TOKEN_INT ||
			 g_scanner_peek_next_token (scanner) == G_TOKEN_FLOAT)
		    {
		      g_scanner_get_next_token (scanner); /* int or float */
		      g_darray_append (array, scanner->token == G_TOKEN_FLOAT ? scanner->value.v_float : scanner->value.v_int);
		    }
		}
	      else
		{
		  expected_token = ')';
		  goto out_of_load_wave;
		}
	    }
	  error = bse_wave_load_wave_file (wave, file_name, wave_name, load_list->n_values ? load_list : 0, skip_list);
	  if (error)
	    bse_storage_warn (storage, "failed to load wave \"%s\" from \"%s\": %s",
			      wave_name, file_name, bse_error_blurb (error));
	  expected_token = G_TOKEN_NONE; /* got ')' */
	out_of_load_wave:
	  g_free (file_name);
	  g_free (wave_name);
	  g_darray_free (skip_list);
	  g_darray_free (load_list);
	}
      else if (quark == quark_set_locator)
	{
	  gchar *file_name, *wave_name;

	  g_scanner_get_next_token (scanner); /* eat quark identifier */
	  parse_or_return (scanner, G_TOKEN_STRING);
	  file_name = g_strdup (scanner->value.v_string);
	  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
	    {
	      g_free (file_name);
	      return G_TOKEN_STRING;
	    }
	  wave_name = g_strdup (scanner->value.v_string);
	  if (g_scanner_get_next_token (scanner) != ')')
	    {
	      g_free (file_name);
	      g_free (wave_name);
	      return ')';
	    }
	  // g_print ("set-locator \"%s\" \"%s\"\n", file_name, wave_name);
	  bse_wave_set_locator (wave, file_name, wave_name);
	  expected_token = G_TOKEN_NONE; /* got ')' */
	}
      else if (quark == quark_wave_chunk)
	{
	  ParsedWaveChunk parsed_wchunk = { 1, 0, 0, 0, 0, 0, 0, 0, NULL };

	  g_scanner_get_next_token (scanner); /* eat quark identifier */
	  if (g_scanner_get_next_token (scanner) != G_TOKEN_FLOAT && scanner->token != G_TOKEN_INT)
	    return G_TOKEN_FLOAT;
	  parsed_wchunk.osc_freq = scanner->token == G_TOKEN_FLOAT ? scanner->value.v_float : scanner->value.v_int;
	  if (g_scanner_get_next_token (scanner) != G_TOKEN_FLOAT && scanner->token != G_TOKEN_INT)
	    return G_TOKEN_FLOAT;
	  parsed_wchunk.mix_freq = scanner->token == G_TOKEN_FLOAT ? scanner->value.v_float : scanner->value.v_int;
	  expected_token = bse_storage_parse_rest (storage,
						   (BseTryStatement) parse_wave_chunk,
						   wave, &parsed_wchunk);
	  if (expected_token == G_TOKEN_NONE && parsed_wchunk.wave_handle)
	    {
	      GslDataCache *dcache;
	      GslWaveChunk *wchunk;

	      if (0)
		g_print ("add_wave_chunk %u %f %f jl%u pl%u c%lu s%lu e%lu w%p\n",
			 parsed_wchunk.n_channels,
			 parsed_wchunk.osc_freq, parsed_wchunk.mix_freq,
			 parsed_wchunk.jump_loop, parsed_wchunk.ping_pong_loop,
			 parsed_wchunk.loop_count, parsed_wchunk.loop_start, parsed_wchunk.loop_end,
			 parsed_wchunk.wave_handle);
	      dcache = gsl_data_cache_from_dhandle (parsed_wchunk.wave_handle,
						    gsl_get_config ()->wave_chunk_padding * parsed_wchunk.n_channels);
	      wchunk = gsl_wave_chunk_new (dcache,
					   parsed_wchunk.osc_freq,
					   parsed_wchunk.mix_freq,
					   parsed_wchunk.jump_loop ? GSL_WAVE_LOOP_JUMP :
					   parsed_wchunk.ping_pong_loop ? GSL_WAVE_LOOP_PINGPONG :
					   GSL_WAVE_LOOP_NONE,
					   parsed_wchunk.loop_start,
					   parsed_wchunk.loop_end,
					   parsed_wchunk.loop_count);
	      gsl_data_cache_unref (dcache);
	      bse_wave_add_chunk (wave, wchunk);
	    }
	  if (parsed_wchunk.wave_handle)
	    gsl_data_handle_unref (parsed_wchunk.wave_handle);
	}
      else
	expected_token = BSE_TOKEN_UNMATCHED;
    }

  return expected_token;
}

static GTokenType
bse_wave_do_restore (BseObject  *object,
		     BseStorage *storage)
{
  BseWave *wave = BSE_WAVE (object);
  GTokenType expected_token = G_TOKEN_NONE;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore)
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore (object, storage);

  if (0)
    g_printerr ("BseWave: post parsing: %u wave chunks locator_set=%u\n",
		wave->n_wchunks, wave->locator_set);

  return expected_token;
}

void
bse_wave_request_index (BseWave *wave)
{
  g_return_if_fail (BSE_IS_WAVE (wave));

  if (!wave->request_count)
    g_object_ref (wave);
  wave->request_count++;
}

BseWaveIndex*
bse_wave_get_index_for_modules (BseWave *wave)
{
  g_return_val_if_fail (BSE_IS_WAVE (wave), NULL);
  g_return_val_if_fail (wave->request_count > 0, NULL);

  if (!wave->n_wchunks)
    return NULL;
  if (wave->index_dirty || !wave->index_list)
    {
      BseWaveIndex *index = g_malloc (sizeof (BseWaveIndex) + sizeof (index->wchunks) * wave->n_wchunks);
      GSList *slist;

      index->n_wchunks = 0;
      index->wchunks = (gpointer) (index + 1);
      for (slist = wave->wave_chunks; slist; slist = slist->next)
	{
	  GslErrorType error = gsl_wave_chunk_open (slist->data);
	  if (!error)
	    index->wchunks[index->n_wchunks++] = slist->data;
	}
      wave->index_list = g_slist_prepend (wave->index_list, index);
      // FIXME: add dummy wave chunk if none was opened succesfully
      wave->index_dirty = FALSE;
    }
  return wave->index_list->data;
}

void
bse_wave_drop_index (BseWave *wave)
{
  g_return_if_fail (BSE_IS_WAVE (wave));
  g_return_if_fail (wave->request_count > 0);

  wave->request_count--;
  if (!wave->request_count)
    {
      while (wave->index_list)
	{
	  GSList *tmp = wave->index_list->next;
	  BseWaveIndex *index = wave->index_list->data;
	  guint i;
	  
	  for (i = 0; i < index->n_wchunks; i++)
	    gsl_wave_chunk_close (index->wchunks[i]);
	  g_free (index);
	  g_slist_free_1 (wave->index_list);
	  wave->index_list = tmp;
	}
      g_object_unref (wave);
    }
}

GslWaveChunk*
bse_wave_index_lookup_best (BseWaveIndex *windex,
			    gfloat        osc_freq)
{
  gfloat best_diff = 1e+9;
  GslWaveChunk *best_chunk = NULL;

  g_return_val_if_fail (windex != NULL, NULL);

  if (windex->n_wchunks > 0)
    {
      GslWaveChunk **check, **nodes = windex->wchunks;
      guint n_nodes = windex->n_wchunks;

      nodes -= 1;
      do
	{
	  register gfloat cmp;
	  register guint i;

	  i = (n_nodes + 1) >> 1;
	  check = nodes + i;
	  cmp = osc_freq - (*check)->osc_freq;
	  if (cmp > 0)
	    {
	      if (cmp < best_diff)
		{
		  best_diff = cmp;
		  best_chunk = *check;
		}
	      n_nodes -= i;
	      nodes = check;
	    }
	  else if (cmp < 0)
	    {
	      cmp = -cmp;
	      if (cmp < best_diff)
		{
		  best_diff = cmp;
		  best_chunk = *check;
		}
	      n_nodes = i - 1;
	    }
	  else if (cmp == 0)
	    return *check;      /* exact match, prolly seldom for floats */
	}
      while (n_nodes);
    }
  return best_chunk;
}
