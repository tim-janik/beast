/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik
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


enum {
  PARAM_0,
};

#define	ABANDONED_OWNER		(GUINT_TO_POINTER (42))

typedef struct
{
  GslWaveChunk     *wchunk;
  gchar            *file_name;
  gchar		   *wave_name;
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
static void	    bse_wave_destroy		(BseObject		*object);
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
static void	    bse_wave_prepare		(BseSource		*source);
static void	    bse_wave_reset		(BseSource		*source);
static void	    wave_update_index		(BseWave		*wave,
						 gboolean		 force);


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
    8  /* n_preallocs */,
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
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_wave_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_wave_get_property;

  object_class->store_private = bse_wave_do_store_private;
  object_class->restore = bse_wave_do_restore;
  object_class->restore_private = bse_wave_do_restore_private;
  object_class->destroy = bse_wave_destroy;
  
  source_class->prepare = bse_wave_prepare;
  source_class->reset = bse_wave_reset;

  quark_n_channels = g_quark_from_static_string ("n-channels");
  quark_loop = g_quark_from_static_string ("loop");
  quark_ping_pong_loop = g_quark_from_static_string ("ping-pong-loop");
  quark_wave_handle = g_quark_from_static_string ("wave-handle");
  quark_load_wave = g_quark_from_static_string ("load-wave");
  quark_set_locator = g_quark_from_static_string ("set-locator");
  quark_wave_chunk = g_quark_from_static_string ("wave-chunk");

  bse_server_register_loader (bse_server_get (),
			      g_type_from_name ("BseWave+gslwave-loader"),
			      ".gslwave",
			      "0 string #GslWave\n");
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
  wave->abandoned_chunks = NULL;
  wave->index_list = NULL;
  BSE_OBJECT_SET_FLAGS (wave, BSE_ITEM_FLAG_NEVER_STORE);
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
bse_wave_destroy (BseObject *object)
{
  BseWave *wave = BSE_WAVE (object);

  nuke_wave_urls (wave);
  
  while (wave->wave_chunks)
    bse_wave_remove_chunk (wave, wave->wave_chunks->data);
  g_return_if_fail (wave->abandoned_chunks == NULL);
  g_return_if_fail (wave->index_list == NULL);

  g_free (wave->file_name);
  g_free (wave->wave_name);

  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

BseWaveIndex*
bse_wave_get_index_for_modules (BseWave *wave)
{
  g_return_val_if_fail (BSE_IS_WAVE (wave), NULL);

  if (BSE_SOURCE_PREPARED (wave) && wave->n_wchunks)
    return wave->index_list->data;
  return NULL;
}

GslWaveChunk*
bse_wave_lookup_chunk (BseWave *wave,
		       gfloat   osc_freq,
		       gfloat   mix_freq)
{
  g_return_val_if_fail (BSE_IS_WAVE (wave), NULL);

  if (wave->index_list)
    return bse_wave_index_lookup_best (wave->index_list->data, osc_freq);
  else
    {
      GslWaveChunk *wchunk;

      /* _lazy_ coder */
      wave_update_index (wave, TRUE);
      wchunk = bse_wave_index_lookup_best (wave->index_list->data, osc_freq);
      g_free (wave->index_list->data);
      g_assert (wave->index_list->next == NULL);
      g_slist_free_1 (wave->index_list);
      wave->index_list = NULL;

      return wchunk;
    }
}

void
bse_wave_remove_chunk (BseWave      *wave,
		       GslWaveChunk *wchunk)
{
  GSList *slist;

  g_return_if_fail (BSE_IS_WAVE (wave));
  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (wchunk->owner_data == wave);

  wave->wave_chunks = g_slist_remove (wave->wave_chunks, wchunk);
  wave->n_wchunks--;
  wchunk->owner_data = ABANDONED_OWNER;

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

  if (BSE_SOURCE_PREPARED (wave))
    wave->abandoned_chunks = g_slist_prepend (wave->abandoned_chunks, wchunk);
  else
    _gsl_wave_chunk_destroy (wchunk);

  wave_update_index (wave, FALSE);

  if (wave->n_wchunks)
    BSE_OBJECT_UNSET_FLAGS (wave, BSE_ITEM_FLAG_NEVER_STORE);
  else
    BSE_OBJECT_SET_FLAGS (wave, BSE_ITEM_FLAG_NEVER_STORE);
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
  g_return_if_fail (BSE_IS_WAVE (wave));
  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (wchunk->dcache != NULL);
  g_return_if_fail (wchunk->owner_data == NULL);

  wchunk->owner_data = wave;
  wave->wave_chunks = g_slist_insert_sorted (wave->wave_chunks, wchunk, wchunk_cmp);
  wave->n_wchunks++;
  if (wave->locator_set)
    {
      WaveChunkUrl *url = g_new0 (WaveChunkUrl, 1);

      url->wchunk = wchunk;
      url->file_name = NULL;
      url->wave_name = NULL;
      wave->wave_chunk_urls = g_slist_prepend (wave->wave_chunk_urls, url);
    }
  wave_update_index (wave, FALSE);
  BSE_OBJECT_UNSET_FLAGS (wave, BSE_ITEM_FLAG_NEVER_STORE);
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
  g_return_if_fail (wchunk->owner_data == NULL);
  g_return_if_fail (file_name != NULL);
  g_return_if_fail (wave_name != NULL);

  wchunk->owner_data = wave;
  wave->wave_chunks = g_slist_insert_sorted (wave->wave_chunks, wchunk, wchunk_cmp);
  wave->n_wchunks++;
  url = g_new0 (WaveChunkUrl, 1);
  url->wchunk = wchunk;
  url->file_name = g_strdup (file_name);
  url->wave_name = g_strdup (wave_name);
  wave->wave_chunk_urls = g_slist_prepend (wave->wave_chunk_urls, url);
  wave_update_index (wave, FALSE);
  BSE_OBJECT_UNSET_FLAGS (wave, BSE_ITEM_FLAG_NEVER_STORE);
}

void
bse_wave_set_locator (BseWave     *wave,
		      const gchar *file_name,
		      const gchar *wave_name)
{
  g_return_if_fail (BSE_IS_WAVE (wave));
  g_return_if_fail (file_name != NULL);
  g_return_if_fail (wave_name != NULL);
  g_return_if_fail (wave->locator_set == FALSE);

  nuke_wave_urls (wave);
  wave->locator_set = TRUE;
  wave->file_name = g_strdup (file_name);
  wave->wave_name = g_strdup (wave_name);
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

  if (wave->locator_set)
    {
      bse_storage_break (storage);
      bse_storage_printf (storage, "(load-wave \"%s\" \"%s\"", wave->file_name, wave->wave_name);

      if (wave->wave_chunk_urls)
	{
	  GSList *slist;
	  guint i = 0;

	  bse_storage_printf (storage, " skip");
	  bse_storage_push_level (storage);
	  for (slist = wave->wave_chunk_urls; slist; slist = slist->next)
	    {
	      WaveChunkUrl *url = slist->data;

	      if (i++ % 3 == 0)
		bse_storage_break (storage);
	      else
		bse_storage_putc (storage, ' ');
	      bse_storage_printf (storage, "%g", url->wchunk->osc_freq);
	    }
	  bse_storage_pop_level (storage);
	}

      bse_storage_putc (storage, ')');
      bse_storage_break (storage);
      bse_storage_printf (storage, "(set-locator \"%s\" \"%s\")", wave->file_name, wave->wave_name);
    }

  for (slist = wave->wave_chunk_urls; slist; slist = slist->next)
    {
      WaveChunkUrl *url = slist->data;

      if (url->file_name && url->wave_name)
	{
	  bse_storage_break (storage);
	  bse_storage_printf (storage, "(load-wave \"%s\" \"%s\" list %g)",
			      url->file_name, url->wave_name,
			      url->wchunk->osc_freq);
	}
      else
	{
          bse_storage_break (storage);
	  bse_storage_printf (storage, "(wave-chunk %g %g", url->wchunk->osc_freq, url->wchunk->mix_freq);
          bse_storage_push_level (storage);
	  if (1||url->wchunk->n_channels > 1)
	    {
	      bse_storage_break (storage);
	      bse_storage_printf (storage, "(n-channels %u)", url->wchunk->n_channels);
	    }
	  switch (url->wchunk->loop_type)
	    {
	    case GSL_WAVE_LOOP_JUMP:
	      bse_storage_break (storage);
	      bse_storage_printf (storage, "(loop %lu %lu %lu)",
				  (GslLong) url->wchunk->loop_count,
				  url->wchunk->loop_start,
				  url->wchunk->loop_end);
	      break;
	    case GSL_WAVE_LOOP_PINGPONG:
	      bse_storage_break (storage);
	      bse_storage_printf (storage, "(ping-pong-loop %lu %lu %lu)",
				  (GslLong) url->wchunk->loop_count,
				  url->wchunk->loop_start,
				  url->wchunk->loop_end);
	      break;
	    case GSL_WAVE_LOOP_NONE:
	      break;
	    }
	  bse_storage_break (storage);
	  bse_storage_printf (storage, "(wave-handle ");
	  bse_storage_put_wave_handle (storage, 32,
				       url->wchunk->dcache->handle,
				       url->wchunk->offset,
				       url->wchunk->length);
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
      expected_token = bse_storage_parse_wave_handle (storage, &pwchunk->wave_handle);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;
      if (!pwchunk->wave_handle)
	return bse_storage_warn_skip (storage, "invalid wave data reference");
    }
  else
    return BSE_TOKEN_UNMATCHED;
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
}

static gboolean
bse_procedure_signature_is_loader (BseProcedureClass *proc)
{
  g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (proc), FALSE);

  if (proc->n_in_params == 5 && proc->n_out_params == 1)
    {
      if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (proc->out_param_specs[0]), BSE_TYPE_ERROR_TYPE) &&
	  g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (proc->in_param_specs[0]), BSE_TYPE_WAVE) &&
	  G_IS_PARAM_SPEC_STRING (proc->in_param_specs[1]) &&
	  G_IS_PARAM_SPEC_STRING (proc->in_param_specs[2]) &&
	  G_IS_PARAM_SPEC_VALUE_ARRAY (proc->in_param_specs[3]) &&
	  G_IS_PARAM_SPEC_VALUE_ARRAY (proc->in_param_specs[4]))
	{
	  GParamSpec *espec1 = G_PARAM_SPEC_VALUE_ARRAY (proc->in_param_specs[3])->element_spec;
	  GParamSpec *espec2 = G_PARAM_SPEC_VALUE_ARRAY (proc->in_param_specs[4])->element_spec;

	  if (espec1 && G_IS_PARAM_SPEC_FLOAT (espec1) &&
	      espec2 && G_IS_PARAM_SPEC_FLOAT (espec2))
	    return TRUE;
	}
    }
  return FALSE;
}

static BseErrorType
call_wave_chunk_loader (GType        proc_type,
			BseWave     *wave,
			const gchar *file_name,
			const gchar *wave_name,
			GDArray     *list_array,
			GDArray     *skip_array)
{
  GValueArray *skip_value_array = NULL, *list_value_array = NULL;
  GValue tvalue = { 0, };
  BseProcedureClass *proc;
  GDArray *array;
  BseErrorType perror, error;

  g_return_val_if_fail (BSE_TYPE_IS_PROCEDURE (proc_type), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_IS_WAVE (wave), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (file_name != NULL, BSE_ERROR_INTERNAL);

  g_print ("parse \"%s\" \"%s\" list %u skip %u\n",
	   file_name, wave_name, list_array ? list_array->n_values : 0, skip_array ? skip_array->n_values : 0);
  g_value_init (&tvalue, G_TYPE_FLOAT);
  array = skip_array;
  if (array && array->n_values)
    {
      GValueArray *value_array = g_value_array_new (array->n_values);
      guint j;

      for (j = 0; j < array->n_values; j++)
	{
	  g_value_set_float (&tvalue, array->values[j]);
	  g_value_array_append (value_array, &tvalue);
	}
      skip_value_array = value_array;
    }
  array = list_array;
  if (array && array->n_values)
    {
      GValueArray *value_array = g_value_array_new (array->n_values);
      guint j;

      for (j = 0; j < array->n_values; j++)
	{
	  g_value_set_float (&tvalue, array->values[j]);
	  g_value_array_append (value_array, &tvalue);
	}
      list_value_array = value_array;
    }
  g_value_unset (&tvalue);

  proc = g_type_class_ref (proc_type);
  if (!bse_procedure_signature_is_loader (proc))
    error = BSE_ERROR_NO_LOADER;	/* actually, this is a bad mismatch */
  else
    error = bse_procedure_exec (g_type_name (proc_type),
				wave, file_name, wave_name,
				list_value_array, skip_value_array,
				&perror);
  g_type_class_unref (proc);
  if (!error)
    error = perror;
  if (skip_value_array)
    g_value_array_free (skip_value_array);
  if (list_value_array)
    g_value_array_free (list_value_array);

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
	  GType proc_type;
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
	  proc_type = bse_server_find_loader (bse_server_get (), file_name);
	  if (proc_type)
	    error = call_wave_chunk_loader (proc_type, wave, file_name, wave_name, load_list, skip_list);
	  else
	    error = BSE_ERROR_NO_LOADER;
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
	  g_print ("set-locator \"%s\" \"%s\"\n", file_name, wave_name);
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
	  if (expected_token == G_TOKEN_NONE)
	    {
	      GslDataCache *dcache;
	      GslWaveChunk *wchunk;

	      g_print ("add_wave_chunk %u %f %f jl%u pl%u c%lu s%lu e%lu w%p\n",
		       parsed_wchunk.n_channels,
		       parsed_wchunk.osc_freq, parsed_wchunk.mix_freq,
		       parsed_wchunk.jump_loop, parsed_wchunk.ping_pong_loop,
		       parsed_wchunk.loop_count, parsed_wchunk.loop_start, parsed_wchunk.loop_end,
		       parsed_wchunk.wave_handle);
	      dcache = gsl_data_cache_from_dhandle (parsed_wchunk.wave_handle,
						    gsl_get_config ()->wave_chunk_padding * parsed_wchunk.n_channels);
	      wchunk = _gsl_wave_chunk_create (dcache,
					       0, parsed_wchunk.wave_handle->n_values / parsed_wchunk.n_channels,
					       parsed_wchunk.n_channels,
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

  g_print ("post parsing: %u wave chunks locator_set=%u\n",
	   wave->n_wchunks, wave->locator_set);

  return expected_token;
}

static void
wave_update_index (BseWave *wave,
		   gboolean force)
{
  if (BSE_SOURCE_PREPARED (wave) || force)
    {
      BseWaveIndex *index = g_malloc (sizeof (BseWaveIndex) + sizeof (index->wchunks) * wave->n_wchunks);
      GSList *slist;
      guint i;
      
      index->n_wchunks = wave->n_wchunks;
      index->wchunks = (gpointer) (index + 1);
      for (i = 0, slist = wave->wave_chunks; i < index->n_wchunks; i++, slist = slist->next)
	index->wchunks[i] = slist->data;
      wave->index_list = g_slist_prepend (wave->index_list, index);
    }
}

static void
bse_wave_prepare (BseSource *source)
{
  BseWave *wave = BSE_WAVE (source);

  wave_update_index (wave, FALSE);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
bse_wave_reset (BseSource *source)
{
  BseWave *wave = BSE_WAVE (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);

  do
    {
      GSList *tmp = wave->index_list->next;

      g_free (wave->index_list->data);
      g_slist_free_1 (wave->index_list);
      wave->index_list = tmp;
    }
  while (wave->index_list);

  while (wave->abandoned_chunks)
    {
      GSList *tmp = wave->abandoned_chunks;

      wave->abandoned_chunks = tmp->next;
      _gsl_wave_chunk_destroy (tmp->data);
      g_slist_free_1 (tmp);
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
