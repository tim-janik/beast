/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2003 Tim Janik
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
#include "bsewave.h"

#include "bsestorage.h"
#include "bseprocedure.h"
#include "gslwavechunk.h"
#include "gsldatahandle.h"
#include "bseserver.h"
#include <bse/gslloader.h>

#include <string.h>

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
  guint          jump_loop : 1;
  guint          ping_pong_loop : 1;
  GslLong        loop_count;
  GslLong        loop_start;
  GslLong        loop_end;
  GslDataHandle *wave_handle;
  guint          wh_n_channels;
  gfloat         wh_mix_freq;
  gfloat         wh_osc_freq;
} ParsedWaveChunk;


/* --- prototypes --- */
static void	    bse_wave_class_init		(BseWaveClass	*class);
static void	    bse_wave_init		(BseWave		*wave);
static void	    bse_wave_dispose		(GObject		*object);
static void	    bse_wave_set_property	(GObject                *object,
						 guint                   param_id,
						 const GValue           *value,
						 GParamSpec             *pspec);
static void	    bse_wave_get_property	(GObject                *object,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec);
static gboolean     bse_wave_needs_storage      (BseItem                *item,
                                                 BseStorage             *storage);
static void         bse_wave_store_private	(BseObject		*object,
						 BseStorage		*storage);
static SfiTokenType bse_wave_restore_private	(BseObject		*object,
						 BseStorage		*storage,
                                                 GScanner               *scanner);


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
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_wave_set_property;
  gobject_class->get_property = bse_wave_get_property;
  gobject_class->dispose = bse_wave_dispose;
  
  object_class->store_private = bse_wave_store_private;
  object_class->restore_private = bse_wave_restore_private;

  item_class->needs_storage = bse_wave_needs_storage;
  
  quark_n_channels = g_quark_from_static_string ("n-channels");
  quark_loop = g_quark_from_static_string ("loop");
  quark_ping_pong_loop = g_quark_from_static_string ("ping-pong-loop");
  quark_wave_handle = g_quark_from_static_string ("wave-handle");
  quark_load_wave = g_quark_from_static_string ("load-wave");
  quark_set_locator = g_quark_from_static_string ("set-locator");
  quark_wave_chunk = g_quark_from_static_string ("wave-chunk");
  
  bse_object_class_add_param (object_class, "Locator",
			      PARAM_LOCATOR_SET,
			      sfi_pspec_bool ("locator_set", "Locator Set", NULL,
					      FALSE, "G:r"));
  bse_object_class_add_param (object_class, "Locator",
			      PARAM_FILE_NAME,
			      sfi_pspec_string ("file_name", "File Name", NULL,
						NULL, "G:r"));
  bse_object_class_add_param (object_class, "Locator",
			      PARAM_WAVE_NAME,
			      sfi_pspec_string ("wave_name", "Wave Name", NULL,
						NULL, "G:r"));
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
}

static void
bse_wave_set_property (GObject      *object,
		       guint         param_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_wave_get_property (GObject    *object,
		       guint	   param_id,
		       GValue	  *value,
		       GParamSpec *pspec)
{
  BseWave *wave = BSE_WAVE (object);
  switch (param_id)
    {
    case PARAM_LOCATOR_SET:
      sfi_value_set_bool (value, wave->locator_set);
      break;
    case PARAM_FILE_NAME:
      sfi_value_set_string (value, wave->file_name);
      break;
    case PARAM_WAVE_NAME:
      sfi_value_set_string (value, wave->wave_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static gboolean
bse_wave_needs_storage (BseItem    *item,
                        BseStorage *storage)
{
  BseWave *self = BSE_WAVE (item);
  return self->n_wchunks > 0;
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
  wave->file_name = NULL;
  g_free (wave->wave_name);
  wave->wave_name = NULL;

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GslWaveChunk*
bse_wave_lookup_chunk (BseWave *wave,
		       gfloat   mix_freq,
		       gfloat   osc_freq)
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
bse_wave_set_description_bits (BseWave        *self,
                               GslWaveDsc     *wdsc,
                               gboolean        honour_name)
{
  g_return_if_fail (BSE_IS_WAVE (self));
  if (wdsc->name && honour_name)
    bse_item_set (self, "uname", wdsc->name, NULL);
  /* FIXME: set authors, license, comment
   * if (wdsc->comment)
   * bse_item_set (self, "blurb", wdsc->comment, NULL);
   */
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
bse_wave_store_private (BseObject  *object,
			BseStorage *storage)
{
  BseWave *wave = BSE_WAVE (object);
  GSList *slist;
  
  /* chain parent class' handler */
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
	      bse_storage_putf (storage, url->wchunk->osc_freq);
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
	  bse_storage_printf (storage, "(load-wave \"%s\" \"%s\" list ",
			      url->file_name, url->wave_name);
	  bse_storage_putf (storage, url->wchunk->osc_freq);
	  bse_storage_putc (storage, ')');
	}
      else	/* self-contained wave storage */
	{
	  BseErrorType error = gsl_data_handle_open (url->wchunk->dcache->dhandle);
	  if (error)
	    {
	      bse_storage_warn (storage, "failed to open data handle (%s): %s",
				gsl_data_handle_name (url->wchunk->dcache->dhandle),
				bse_error_blurb (error));
	      continue;
	    }
          bse_storage_break (storage);
	  bse_storage_puts (storage, "(wave-chunk ");
          bse_storage_push_level (storage);
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
	  bse_storage_put_data_handle (storage, 0, url->wchunk->dcache->dhandle);
	  gsl_data_handle_close (url->wchunk->dcache->dhandle);
          bse_storage_pop_level (storage);
	  bse_storage_putc (storage, ')');
	}
    }
}

static SfiTokenType
parse_wave_chunk (BseWave         *wave,
		  BseStorage      *storage,
                  GScanner        *scanner,
		  ParsedWaveChunk *pwchunk)
     
{
  GQuark quark;
  
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return SFI_TOKEN_UNMATCHED;

  quark = g_quark_try_string (scanner->next_value.v_string);
  if (quark == quark_n_channels)
    {
      g_scanner_get_next_token (scanner); /* eat quark identifier */
      parse_or_return (scanner, G_TOKEN_INT);
      bse_storage_compat_dhchannels (storage, scanner->value.v_int64);
    }
  else if (quark == quark_loop || quark == quark_ping_pong_loop)
    {
      g_scanner_get_next_token (scanner); /* eat quark identifier */
      pwchunk->jump_loop = quark == quark_loop;
      pwchunk->ping_pong_loop = quark == quark_ping_pong_loop;
      parse_or_return (scanner, G_TOKEN_INT);
      pwchunk->loop_count = scanner->value.v_int64;
      parse_or_return (scanner, G_TOKEN_INT);
      pwchunk->loop_start = scanner->value.v_int64;
      parse_or_return (scanner, G_TOKEN_INT);
      pwchunk->loop_end = scanner->value.v_int64;
    }
  else if (bse_storage_match_data_handle (storage, quark) ||
           (quark == quark_wave_handle && BSE_STORAGE_COMPAT (storage, 0, 5, 1))) /* VERSION-FIXME: 0.5.1 compat */
    {
      guint expected_token;

      if (quark == quark_wave_handle)   /* VERSION-FIXME: 0.5.1 compat */
        g_scanner_get_next_token (scanner); /* eat quark identifier */
      if (pwchunk->wave_handle)
	return bse_storage_warn_skip (storage, "duplicate wave data reference");
      if (quark == quark_wave_handle)   /* VERSION-FIXME: 0.5.1 compat */
        expected_token = bse_storage_parse_data_handle (storage,
                                                        &pwchunk->wave_handle,
                                                        &pwchunk->wh_n_channels,
                                                        &pwchunk->wh_mix_freq,
                                                        &pwchunk->wh_osc_freq);
      else
        expected_token = bse_storage_parse_data_handle_rest (storage,
                                                             &pwchunk->wave_handle,
                                                             &pwchunk->wh_n_channels,
                                                             &pwchunk->wh_mix_freq,
                                                             &pwchunk->wh_osc_freq);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;
      if (!pwchunk->wave_handle)
        bse_storage_warn (storage, "invalid wave data reference");
      /* don't eat closing brace in non-compat case */
      if (quark != quark_wave_handle) /* VERSION-FIXME: 0.5.1 compat */
        return G_TOKEN_NONE;
    }
  else
    return SFI_TOKEN_UNMATCHED;
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
}

BseErrorType
bse_wave_load_wave_file (BseWave      *wave,
			 const gchar  *file_name,
			 const gchar  *wave_name,
			 BseFreqArray *list_array,
			 BseFreqArray *skip_array,
                         gboolean      honour_description)
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
	  GslWaveDsc *wdsc = gsl_wave_dsc_load (fi, i, FALSE, &error);
	  
	  if (wdsc)
	    {
	      for (i = 0; i < wdsc->n_chunks; i++)
		if (bse_freq_arrays_match_freq (wdsc->chunks[i].osc_freq, list_array, skip_array))
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
                      {
                        bse_wave_add_chunk (wave, wchunk);
                        if (honour_description)
                          bse_wave_set_description_bits (wave, wdsc, TRUE);
                      }
		  }
	      gsl_wave_dsc_free (wdsc);
	    }
	}
      else
	error = BSE_ERROR_FILE_NOT_FOUND;
    }
  else
    {
      error = gsl_file_check (file_name, "fr");
      if (!error)
	error = BSE_ERROR_FILE_OPEN_FAILED;
    }
  return error;
}

static SfiTokenType
bse_wave_restore_private (BseObject  *object,
			  BseStorage *storage,
                          GScanner   *scanner)
{
  BseWave *wave = BSE_WAVE (object);
  GTokenType expected_token;
  GQuark quark;

  /* chain parent class' handler */
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);

  /* parse storage commands */
  quark = g_quark_try_string (scanner->next_value.v_identifier);
  if (quark == quark_load_wave)
    {
      BseFreqArray *skip_list, *load_list, *array;
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
      skip_list = bse_freq_array_new (1024);
      load_list = bse_freq_array_new (1024);
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
		  bse_freq_array_append (array, scanner->token == G_TOKEN_FLOAT ? scanner->value.v_float : scanner->value.v_int64);
		}
	    }
	  else
	    {
	      expected_token = ')';
	      goto out_of_load_wave;
	    }
	}
      error = bse_wave_load_wave_file (wave, file_name, wave_name,
                                       bse_freq_array_n_values (load_list) ? load_list : 0, skip_list,
                                       FALSE);
      if (error)
	bse_storage_warn (storage, "failed to load wave \"%s\" from \"%s\": %s",
			  wave_name, file_name, bse_error_blurb (error));
      expected_token = G_TOKEN_NONE; /* got ')' */
    out_of_load_wave:
      g_free (file_name);
      g_free (wave_name);
      bse_freq_array_free (skip_list);
      bse_freq_array_free (load_list);
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
      ParsedWaveChunk parsed_wchunk = { 0, 0, 0, 0, 0, NULL, 0, 0, 0 };

      g_scanner_get_next_token (scanner); /* eat quark identifier */

      g_scanner_peek_next_token (scanner);

      bse_storage_compat_dhreset (storage);
      if (scanner->next_token == G_TOKEN_FLOAT || scanner->next_token == G_TOKEN_INT)
        {
          g_scanner_get_next_token (scanner);
          bse_storage_compat_dhoscf (storage, (scanner->token == G_TOKEN_INT ?
                                               scanner->value.v_int64 : scanner->value.v_float));
          g_scanner_peek_next_token (scanner);

          if (scanner->next_token == G_TOKEN_FLOAT || scanner->next_token == G_TOKEN_INT)
            {
              g_scanner_get_next_token (scanner);
              bse_storage_compat_dhmixf (storage, (scanner->token == G_TOKEN_INT ?
                                                   scanner->value.v_int64 : scanner->value.v_float));
            }
        }

      expected_token = bse_storage_parse_rest (storage, wave,
					       (BseTryStatement) parse_wave_chunk,
                                               &parsed_wchunk);
      bse_storage_compat_dhreset (storage);

      if (expected_token == G_TOKEN_NONE && parsed_wchunk.wave_handle)
	{
	  GslDataCache *dcache;
	  GslWaveChunk *wchunk;
	  
	  if (0)
	    g_print ("add_wave_chunk %u %f %f jl%u pl%u c%lu s%lu e%lu w%p\n",
		     parsed_wchunk.wh_n_channels,
		     parsed_wchunk.wh_osc_freq, parsed_wchunk.wh_mix_freq,
		     parsed_wchunk.jump_loop, parsed_wchunk.ping_pong_loop,
		     parsed_wchunk.loop_count, parsed_wchunk.loop_start, parsed_wchunk.loop_end,
		     parsed_wchunk.wave_handle);
	  dcache = gsl_data_cache_from_dhandle (parsed_wchunk.wave_handle,
						gsl_get_config ()->wave_chunk_padding * parsed_wchunk.wh_n_channels);
	  wchunk = gsl_wave_chunk_new (dcache,
				       parsed_wchunk.wh_mix_freq,
				       parsed_wchunk.wh_osc_freq,
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
  else /* chain parent class' handler */
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage, scanner);
  
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
	  BseErrorType error = gsl_wave_chunk_open (slist->data);
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
