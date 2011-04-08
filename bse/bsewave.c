/* BSE - Better Sound Engine
 * Copyright (C) 1997-1999, 2000-2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bsewave.h"
#include "bsemain.h"
#include "bsestorage.h"
#include "bseprocedure.h"
#include "gslwavechunk.h"
#include "gsldatahandle.h"
#include "bseserver.h"
#include "bseloader.h"
#include "topconfig.h"

#include <string.h>

#define parse_or_return         bse_storage_scanner_parse_or_return

enum {
  PARAM_0,
  PARAM_LOCATOR_SET,
  PARAM_FILE_NAME,
  PARAM_WAVE_NAME
};

typedef struct
{
  GslDataHandle *data_handle;
  gchar        **xinfos;
  guint          wh_n_channels;
  gfloat         wh_mix_freq;
  gfloat         wh_osc_freq;
} ParsedWaveChunk;


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static GQuark      quark_n_channels = 0;
static GQuark      quark_xinfos = 0;
static GQuark      quark_loop = 0;
static GQuark      quark_ping_pong_loop = 0;
static GQuark      quark_wave_handle = 0;
static GQuark      quark_load_wave = 0;
static GQuark      quark_set_locator = 0;
static GQuark      quark_wave_chunk = 0;


/* --- functions --- */
static void
bse_wave_init (BseWave *wave)
{
  wave->locator_set = FALSE;
  wave->file_name = NULL;
  wave->wave_name = NULL;
  wave->xinfos = NULL;
  wave->n_wchunks = 0;
  wave->wave_chunks = NULL;
  wave->open_handles = NULL;
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
bse_wave_dispose (GObject *object)
{
  BseWave *wave = BSE_WAVE (object);
  bse_wave_clear (wave);
  g_return_if_fail (wave->index_list == NULL);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_wave_finalize (GObject *object)
{
  BseWave *wave = BSE_WAVE (object);
  bse_wave_clear (wave);
  g_return_if_fail (wave->index_list == NULL);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static BseErrorType
bse_wave_add_inlined_wave_chunk (BseWave      *self,
                                 GslWaveChunk *wchunk)
{
  g_return_val_if_fail (BSE_IS_WAVE (self), BSE_ERROR_INTERNAL);
  BseErrorType error = gsl_data_handle_open (wchunk->dcache->dhandle);
  if (!error)
    self->open_handles = sfi_ring_append (self->open_handles, wchunk->dcache->dhandle);
  return error;
}


GslWaveChunk*
bse_wave_lookup_chunk (BseWave *wave,
		       gfloat   mix_freq,
		       gfloat   osc_freq,
		       gfloat   velocity)
{
  BseWaveIndex *index;
  GslWaveChunk *wchunk;
  
  g_return_val_if_fail (BSE_IS_WAVE (wave), NULL);
  
  bse_wave_request_index (wave);
  index = bse_wave_get_index_for_modules (wave);
  wchunk = index ? bse_wave_index_lookup_best (index, osc_freq, velocity) : NULL;
  bse_wave_drop_index (wave);
  
  return wchunk;
}

void
bse_wave_remove_chunk (BseWave      *wave,
		       GslWaveChunk *wchunk)
{
  g_return_if_fail (BSE_IS_WAVE (wave));
  g_return_if_fail (wchunk != NULL);
  
  wave->wave_chunks = sfi_ring_remove (wave->wave_chunks, wchunk);
  wave->n_wchunks--;

  SfiRing *ring;
  for (ring = wave->open_handles; ring; ring = sfi_ring_walk (ring, wave->open_handles))
    if (ring->data == (void*) wchunk->dcache->dhandle)
      {
        gsl_data_handle_close (wchunk->dcache->dhandle);
        wave->open_handles = sfi_ring_remove_node (wave->open_handles, ring);
        break;
      }
  
  gsl_wave_chunk_unref (wchunk);
  wave->index_dirty = TRUE;
}

static gint
wchunk_cmp (gconstpointer a,
	    gconstpointer b,
            gpointer      data)
{
  const GslWaveChunk *w1 = a;
  const GslWaveChunk *w2 = b;
  
  return w1->osc_freq < w2->osc_freq ? -1 : w1->osc_freq > w2->osc_freq;
}

void
bse_wave_add_chunk (BseWave      *wave,
		    GslWaveChunk *wchunk)
{
  g_return_if_fail (BSE_IS_WAVE (wave));
  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (wchunk->dcache != NULL);
  
  wave->wave_chunks = sfi_ring_insert_sorted (wave->wave_chunks, gsl_wave_chunk_ref (wchunk), wchunk_cmp, NULL);
  wave->n_wchunks++;
  wave->index_dirty = TRUE;
}

static void
bse_wave_set_locator (BseWave     *wave,
		      const gchar *file_name,
		      const gchar *wave_name)
{
  g_return_if_fail (BSE_IS_WAVE (wave));
  g_return_if_fail (file_name != NULL);
  g_return_if_fail (wave_name != NULL);
  g_return_if_fail (wave->locator_set == FALSE);
  
  wave->locator_set = TRUE;
  wave->file_name = g_strdup (file_name);
  wave->wave_name = g_strdup (wave_name);
  
  g_object_freeze_notify (G_OBJECT (wave));
  g_object_notify (G_OBJECT (wave), "locator_set");
  g_object_notify (G_OBJECT (wave), "file_name");
  g_object_notify (G_OBJECT (wave), "wave_name");
  g_object_thaw_notify (G_OBJECT (wave));
}

void
bse_wave_clear (BseWave *wave)
{
  g_return_if_fail (BSE_IS_WAVE (wave));

  /* delete all wave chunks */
  while (wave->wave_chunks)
    bse_wave_remove_chunk (wave, wave->wave_chunks->data);
  while (wave->open_handles)
    gsl_data_handle_close (sfi_ring_pop_head (&wave->open_handles));

  /* free fields */
  g_free (wave->file_name);
  wave->file_name = NULL;
  g_free (wave->wave_name);
  wave->wave_name = NULL;
  g_strfreev (wave->xinfos);
  wave->xinfos = NULL;
}

BseErrorType
bse_wave_load_wave_file (BseWave      *self,
			 const gchar  *file_name,
			 const gchar  *wave_name,
			 BseFreqArray *list_array,
			 BseFreqArray *skip_array,
                         gboolean      rename_wave)
{
  BseErrorType error = BSE_ERROR_NONE;
  
  g_return_val_if_fail (BSE_IS_WAVE (self), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  bse_wave_clear (self);

  BseWaveFileInfo *fi = NULL;

  if (!g_path_is_absolute (file_name))	  /* resolve relative path using search dir */
    {
      char *sample_path;
      SfiRing *files, *walk;
      if (bse_main_args->override_sample_path)
	sample_path = g_strdup (bse_main_args->override_sample_path);
      else
	sample_path = g_path_concat (BSE_PATH_SAMPLES, BSE_GCONFIG (sample_path), NULL);
      files = sfi_file_crawler_list_files (sample_path, file_name, G_FILE_TEST_IS_REGULAR);

      for (walk = files; walk; walk = sfi_ring_walk (files, walk))
	{
	  char *fname = walk->data;
	  if (!fi)
	    fi = bse_wave_file_info_load (fname, &error);
	  g_free (fname);
	}
      sfi_ring_free (files);
      g_free (sample_path);
    }
  else
    {
      fi = bse_wave_file_info_load (file_name, &error);
    }
  if (fi)
    {
      guint i = 0;
      if (wave_name)
        {
          for (i = 0; i < fi->n_waves; i++)
            if (strcmp (wave_name, fi->waves[i].name) == 0)
              break;
        }
      else if (fi->n_waves != 1)
        i = fi->n_waves;        /* behind boundary: no wave */
      if (i < fi->n_waves)
	{
	  BseWaveDsc *wdsc = bse_wave_dsc_load (fi, i, FALSE, &error);
          wave_name = fi->waves[i].name;
	  if (wdsc && wdsc->n_chunks)
	    {
	      for (i = 0; i < wdsc->n_chunks; i++)
		if (bse_freq_arrays_match_freq (wdsc->chunks[i].osc_freq, list_array, skip_array))
		  {
		    BseErrorType tmp_error;
		    GslWaveChunk *wchunk = bse_wave_chunk_create (wdsc, i, &tmp_error);
		    if (wchunk)
                      bse_wave_add_chunk (self, wchunk);
                    else
		      {
			error = tmp_error;
			sfi_warning (_("Wave \"%s\": failed to load wave chunk for frequency %f: %s"),
                                     wdsc->name, wdsc->chunks[i].osc_freq, bse_error_blurb (error));
		      }
                  }
              if (self->n_wchunks)
                {
                  if (rename_wave && wdsc->name && wdsc->name[0])
                    bse_item_set (self, "uname", wdsc->name, NULL);
                  self->xinfos = bse_xinfos_dup_consolidated (wdsc->xinfos, FALSE);
                  bse_wave_set_locator (self, file_name, wave_name);
                }
              else
                ; /* error still set from bse_wave_chunk_create() */
	      bse_wave_dsc_free (wdsc);
	    }
          else if (wdsc)
            error = BSE_ERROR_FILE_EMPTY;
	}
      else
	error = BSE_ERROR_FILE_NOT_FOUND;
      bse_wave_file_info_unref (fi);
    }
  else
    {
      error = gsl_file_check (file_name, "fr");
      if (!error)
	error = BSE_ERROR_FILE_OPEN_FAILED;
    }
  return error;
}

static void
bse_wave_store_private (BseObject  *object,
			BseStorage *storage)
{
  BseWave *wave = BSE_WAVE (object);
  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  if (wave->locator_set && !BSE_STORAGE_SELF_CONTAINED (storage))
    {
      bse_storage_break (storage);
      bse_storage_printf (storage, "(load-wave \"%s\" \"%s\")", wave->file_name, wave->wave_name);
    }

  if (!wave->locator_set || BSE_STORAGE_SELF_CONTAINED (storage))
    {
      if (wave->xinfos && wave->xinfos[0])
        {
          bse_storage_break (storage);
          bse_storage_puts (storage, "(xinfos ");
          bse_storage_put_xinfos (storage, wave->xinfos);
          bse_storage_putc (storage, ')');
        }
      bse_storage_break (storage);
      SfiRing *ring;
      for (ring = wave->wave_chunks; ring; ring = sfi_ring_walk (ring, wave->wave_chunks))
        {
          GslWaveChunk *wchunk = ring->data;
          BseErrorType error = gsl_data_handle_open (wchunk->dcache->dhandle);
          if (error)
            {
              bse_storage_warn (storage, "failed to open data handle (%s): %s",
                                gsl_data_handle_name (wchunk->dcache->dhandle),
                                bse_error_blurb (error));
              continue;
            }
          bse_storage_break (storage);
          bse_storage_puts (storage, "(wave-chunk ");
          bse_storage_push_level (storage);
          if (wchunk->dcache->dhandle->setup.xinfos)
            {
              gchar **xinfos = bse_xinfos_dup_consolidated (wchunk->dcache->dhandle->setup.xinfos, FALSE);
              xinfos = bse_xinfos_del_value (xinfos, "osc-freq");
              if (xinfos && xinfos[0])
                {
                  bse_storage_break (storage);
                  bse_storage_puts (storage, "(xinfos ");
                  bse_storage_put_xinfos (storage, xinfos);
                  bse_storage_putc (storage, ')');
                }
              g_strfreev (xinfos);
            }
          bse_storage_break (storage);
          bse_storage_put_data_handle (storage, 0, wchunk->dcache->dhandle);
          gsl_data_handle_close (wchunk->dcache->dhandle);
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
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return SFI_TOKEN_UNMATCHED;
  
  GQuark quark = g_quark_try_string (scanner->next_value.v_string);
  if (quark == quark_xinfos)
    {
      g_scanner_get_next_token (scanner); /* eat identifier */
      g_strfreev (pwchunk->xinfos);
      pwchunk->xinfos = NULL;
      GTokenType token = bse_storage_parse_xinfos (storage, &pwchunk->xinfos);
      if (token != G_TOKEN_NONE)
        return token;
    }
  else if (bse_storage_match_data_handle (storage, quark))
    {
      guint expected_token;
      if (pwchunk->data_handle)
	return bse_storage_warn_skip (storage, "duplicate wave data reference");
      expected_token = bse_storage_parse_data_handle_rest (storage,
                                                           &pwchunk->data_handle,
                                                           &pwchunk->wh_n_channels,
                                                           &pwchunk->wh_mix_freq,
                                                           &pwchunk->wh_osc_freq);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;
      if (!pwchunk->data_handle)
        bse_storage_warn (storage, "invalid wave data reference");
      /* closing brace already parsed by bse_storage_parse_data_handle_rest() */
      return G_TOKEN_NONE;
    }
  else if (BSE_STORAGE_COMPAT (storage, 0, 5, 1) && quark == quark_wave_handle)
    {
      guint expected_token;
      g_scanner_get_next_token (scanner); /* eat identifier */
      if (pwchunk->data_handle)
	return bse_storage_warn_skip (storage, "duplicate wave data reference");
      expected_token = bse_storage_parse_data_handle (storage,
                                                      &pwchunk->data_handle,
                                                      &pwchunk->wh_n_channels,
                                                      &pwchunk->wh_mix_freq,
                                                      &pwchunk->wh_osc_freq);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;
      if (!pwchunk->data_handle)
        bse_storage_warn (storage, "invalid wave data reference");
    }
  else if (BSE_STORAGE_COMPAT (storage, 0, 5, 1) && quark == quark_n_channels)
    {
      g_scanner_get_next_token (scanner); /* eat quark identifier */
      parse_or_return (scanner, G_TOKEN_INT);
      bse_storage_compat_dhchannels (storage, scanner->value.v_int64);
    }
  else if (BSE_STORAGE_COMPAT (storage, 0, 6, 4) && (quark == quark_loop || quark == quark_ping_pong_loop))
    {
      g_scanner_get_next_token (scanner); /* eat quark identifier */
      if (quark == quark_loop)
        pwchunk->xinfos = bse_xinfos_add_value (pwchunk->xinfos, "loop-type", gsl_wave_loop_type_to_string (GSL_WAVE_LOOP_JUMP));
      if (quark == quark_ping_pong_loop)
        pwchunk->xinfos = bse_xinfos_add_value (pwchunk->xinfos, "loop-type", gsl_wave_loop_type_to_string (GSL_WAVE_LOOP_PINGPONG));
      parse_or_return (scanner, G_TOKEN_INT);
      pwchunk->xinfos = bse_xinfos_add_num (pwchunk->xinfos, "loop-count", scanner->value.v_int64);
      parse_or_return (scanner, G_TOKEN_INT);
      pwchunk->xinfos = bse_xinfos_add_num (pwchunk->xinfos, "loop-start", scanner->value.v_int64);
      parse_or_return (scanner, G_TOKEN_INT);
      pwchunk->xinfos = bse_xinfos_add_num (pwchunk->xinfos, "loop-end", scanner->value.v_int64);
    }
  else
    return SFI_TOKEN_UNMATCHED;
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
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
  if (quark == quark_xinfos)
    {
      g_scanner_get_next_token (scanner); /* eat identifier */
      gchar **xinfos = NULL;
      GTokenType token = bse_storage_parse_xinfos (storage, &xinfos);
      if (token != G_TOKEN_NONE)
        return token;
      guint i = 0;
      for (i = 0; xinfos && xinfos[i]; i++)
        wave->xinfos = bse_xinfos_parse_assignment (wave->xinfos, xinfos[i]);
      g_strfreev (xinfos);
      parse_or_return (scanner, ')');
      expected_token = G_TOKEN_NONE; /* got ')' */
    }
  else if (quark == quark_load_wave)
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
      if (BSE_STORAGE_COMPAT (storage, 0, 6, 4))
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
      else
        parse_or_return (scanner, ')');
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
  else if (BSE_STORAGE_COMPAT (storage, 0, 6, 4) && quark == quark_set_locator)
    {
      g_scanner_get_next_token (scanner); /* eat quark identifier */
      parse_or_return (scanner, G_TOKEN_STRING);
      gchar *file_name = g_strdup (scanner->value.v_string);
      if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
	{
	  g_free (file_name);
	  return G_TOKEN_STRING;
	}
      gchar *wave_name = g_strdup (scanner->value.v_string);
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
      ParsedWaveChunk parsed_wchunk = { NULL, NULL, 0, 0, 0 };

      g_scanner_get_next_token (scanner); /* eat quark identifier */
      g_scanner_peek_next_token (scanner);
      bse_storage_compat_dhreset (storage); /* VERSION-FIXME: needed for <= 0.5.1 */
      if (scanner->next_token == G_TOKEN_FLOAT || scanner->next_token == G_TOKEN_INT)
        {
          g_scanner_get_next_token (scanner);
          bse_storage_compat_dhoscf (storage, /* VERSION-FIXME: needed for <= 0.5.1 */
                                     (scanner->token == G_TOKEN_INT ?
                                      scanner->value.v_int64 : scanner->value.v_float));
          g_scanner_peek_next_token (scanner);

          if (scanner->next_token == G_TOKEN_FLOAT || scanner->next_token == G_TOKEN_INT)
            {
              g_scanner_get_next_token (scanner);
              bse_storage_compat_dhmixf (storage, /* VERSION-FIXME: needed for <= 0.5.1 */
                                         (scanner->token == G_TOKEN_INT ?
                                          scanner->value.v_int64 : scanner->value.v_float));
            }
        }

      expected_token = bse_storage_parse_rest (storage, wave,
					       (BseTryStatement) parse_wave_chunk,
                                               &parsed_wchunk);
      bse_storage_compat_dhreset (storage); /* VERSION-FIXME: needed for <= 0.5.1 */

      if (expected_token == G_TOKEN_NONE && parsed_wchunk.data_handle)
	{
	  if (0)
	    g_printerr ("restore-wave-chunk: nch=%u of=%f mf=%f dh=%p\n",
                        parsed_wchunk.wh_n_channels,
                        parsed_wchunk.wh_osc_freq, parsed_wchunk.wh_mix_freq,
                        parsed_wchunk.data_handle);
          if (parsed_wchunk.data_handle && parsed_wchunk.xinfos)
            {
              GslDataHandle *tmp_handle = parsed_wchunk.data_handle;
              parsed_wchunk.data_handle = gsl_data_handle_new_add_xinfos (parsed_wchunk.data_handle, parsed_wchunk.xinfos);
              gsl_data_handle_unref (tmp_handle);
            }
	  GslDataCache *dcache = gsl_data_cache_from_dhandle (parsed_wchunk.data_handle,
                                                              BSE_CONFIG (wave_chunk_padding) * parsed_wchunk.wh_n_channels);
          const gchar *ltype = bse_xinfos_get_value (parsed_wchunk.xinfos, "loop-type");
          GslWaveLoopType loop_type = ltype ? gsl_wave_loop_type_from_string (ltype) : GSL_WAVE_LOOP_NONE;
          SfiNum loop_start = bse_xinfos_get_num (parsed_wchunk.xinfos, "loop-start");
          SfiNum loop_end = bse_xinfos_get_num (parsed_wchunk.xinfos, "loop-end");
          SfiNum loop_count = bse_xinfos_get_num (parsed_wchunk.xinfos, "loop-count");
          if (loop_end <= loop_start)
            {
              loop_start = loop_end = 0;
              loop_type = GSL_WAVE_LOOP_NONE;
              loop_count = 0;
            }
          GslWaveChunk *wchunk = gsl_wave_chunk_new (dcache, parsed_wchunk.wh_mix_freq, parsed_wchunk.wh_osc_freq,
                                                     loop_type, loop_start, loop_end, loop_count);
	  gsl_data_cache_unref (dcache);
          /* we need to keep inlined data handles open to protect against storage (.bse file) overwriting */
          BseErrorType error = bse_wave_add_inlined_wave_chunk (wave, wchunk);
          if (!error)
            bse_wave_add_chunk (wave, wchunk);
          else
            {
              bse_storage_error (storage, "failed to reopen inlined data handle (%s): %s",
                                 gsl_data_handle_name (wchunk->dcache->dhandle), bse_error_blurb (error));
              gsl_wave_chunk_unref (wchunk);
            }
	}
      if (parsed_wchunk.data_handle)
	gsl_data_handle_unref (parsed_wchunk.data_handle);
      g_strfreev (parsed_wchunk.xinfos);
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
      BseWaveIndex *index = g_malloc (sizeof (BseWaveIndex) + sizeof (index->entries[0]) * (wave->n_wchunks - 1));
      index->n_entries = 0;
      SfiRing *ring;
      for (ring = wave->wave_chunks; ring; ring = sfi_ring_walk (ring, wave->wave_chunks))
	{
	  BseErrorType error = gsl_wave_chunk_open (ring->data);
	  if (!error)
            {
              index->entries[index->n_entries].wchunk = ring->data;
              index->entries[index->n_entries].osc_freq = index->entries[index->n_entries].wchunk->osc_freq;
              index->entries[index->n_entries].velocity = 1; // FIXME: velocity=1 hardcoded
              index->n_entries++;
            }
	}
      wave->index_list = g_slist_prepend (wave->index_list, index);
      // FIXME: add dummy wave chunk if none was opened succesfully?
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
	  
	  for (i = 0; i < index->n_entries; i++)
	    gsl_wave_chunk_close (index->entries[i].wchunk);
	  g_free (index);
	  g_slist_free_1 (wave->index_list);
	  wave->index_list = tmp;
	}
      g_object_unref (wave);
    }
}

GslWaveChunk*
bse_wave_index_lookup_best (BseWaveIndex *windex,
			    gfloat        osc_freq,
                            gfloat        velocity)
{
  gfloat best_diff = 1e+9;
  const BseWaveEntry *best_chunk = NULL;
  
  g_return_val_if_fail (windex != NULL, NULL);
  
  if (windex->n_entries > 0)
    {
      const BseWaveEntry *check, *nodes = &windex->entries[0];
      guint n_nodes = windex->n_entries;
      
      nodes -= 1;
      do
	{
	  register gfloat cmp;
	  register guint i;
	  
	  i = (n_nodes + 1) >> 1;
	  check = nodes + i;
	  cmp = osc_freq - check->wchunk->osc_freq;
	  if (cmp > 0)
	    {
	      if (cmp < best_diff)
		{
		  best_diff = cmp;
		  best_chunk = check;
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
		  best_chunk = check;
		}
	      n_nodes = i - 1;
	    }
	  else if (cmp == 0)
	    return check->wchunk;       /* exact match, prolly seldom for floats */
	}
      while (n_nodes);
    }
  return best_chunk->wchunk;
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
  gobject_class->finalize = bse_wave_finalize;

  object_class->store_private = bse_wave_store_private;
  object_class->restore_private = bse_wave_restore_private;

  item_class->needs_storage = bse_wave_needs_storage;
  
  quark_n_channels = g_quark_from_static_string ("n-channels");
  quark_xinfos = g_quark_from_static_string ("xinfos");
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
                                   __FILE__, __LINE__,
                                   &wave_info);
}
