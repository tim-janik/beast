/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include	"bsesample.h"

#include	"bseproject.h"
#include	"bsebindata.h"
#include	"bsechunk.h"
#include	"bsestorage.h"
#include	<string.h>
#include	<time.h>
#include	<fcntl.h>
#include	<unistd.h>


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_N_TRACKS,
  PARAM_REC_FREQ
};


/* --- prototypes --- */
static void	    bse_sample_class_init		(BseSampleClass *class);
static void	    bse_sample_init			(BseSample	*sample);
static void	    bse_sample_do_shutdown		(BseObject	*object);
static void	    bse_sample_set_param		(BseSample	*sample,
							 BseParam	*param);
static void	    bse_sample_get_param		(BseSample	*sample,
							 BseParam	*param);
static void	    bse_sample_do_store_private		(BseObject	*object,
							 BseStorage	*storage);
static GTokenType   bse_sample_do_restore		(BseObject	*object,
							 BseStorage	*storage);
static BseTokenType bse_sample_do_restore_private	(BseObject	*object,
							 BseStorage	*storage);


/* --- variables --- */
static BseTypeClass	*parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSample)
{
  static const BseTypeInfo sample_info = {
    sizeof (BseSampleClass),
    
    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_sample_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSample),
    16 /* n_preallocs */,
    (BseObjectInitFunc) bse_sample_init,
  };
  
  return bse_type_register_static (BSE_TYPE_SUPER,
				   "BseSample",
				   "BSE Sample type",
				   &sample_info);
}

static void
bse_sample_class_init (BseSampleClass *class)
{
  BseObjectClass *object_class;
  BseContainerClass *container_class;
  BseSuperClass *super_class;
  
  parent_class = bse_type_class_peek (BSE_TYPE_SUPER);
  object_class = BSE_OBJECT_CLASS (class);
  container_class = BSE_CONTAINER_CLASS (class);
  super_class = BSE_SUPER_CLASS (class);
  
  object_class->set_param = (BseObjectSetParamFunc) bse_sample_set_param;
  object_class->get_param = (BseObjectGetParamFunc) bse_sample_get_param;
  object_class->store_private = bse_sample_do_store_private;
  object_class->restore = bse_sample_do_restore;
  object_class->restore_private = bse_sample_do_restore_private;
  object_class->shutdown = bse_sample_do_shutdown;
  
  /* add BseSample memebers as class parameters, we use the structure
   * offset of the fields as parameter identifiers.
   */
  bse_object_class_add_param (object_class, NULL,
			      PARAM_N_TRACKS,
			      bse_param_spec_uint ("n_tracks", "Number of Tracks", NULL,
						   1, BSE_MAX_N_TRACKS,
						   1, 1,
						   BSE_PARAM_READWRITE |
						   BSE_PARAM_SERVE_STORAGE));
  bse_object_class_add_param (object_class, NULL,
			      PARAM_REC_FREQ,
			      bse_param_spec_uint ("recording_frequency", "Recording Frequency", NULL,
						   BSE_MIN_MIX_FREQ, BSE_MAX_MIX_FREQ,
						   1024, BSE_DFL_SAMPLE_REC_FREQ,
						   BSE_PARAM_READWRITE |
						   BSE_PARAM_SERVE_STORAGE));
}

static void
bse_sample_init (BseSample *sample)
{
  guint i;
  
  sample->n_tracks = 1;
  sample->rec_freq = BSE_DFL_SAMPLE_REC_FREQ;
  
  for (i = 0; i < BSE_MAX_SAMPLE_MUNKS; i++)
    {
      sample->munks[i].rec_note = BSE_DFL_SAMPLE_REC_NOTE;
      sample->munks[i].loop_begin = 0;
      sample->munks[i].loop_end = 0;
      sample->munks[i].bin_data = NULL;
    }
}

static void
bse_sample_do_shutdown (BseObject *object)
{
  BseSample *sample;
  guint i;
  
  sample = BSE_SAMPLE (object);
  
  for (i = 0; i < BSE_MAX_SAMPLE_MUNKS; i++)
    {
      BseObject *object;
      
      object = (BseObject*) sample->munks[i].bin_data;
      if (object)
	{
	  sample->munks[i].bin_data = NULL;
	  bse_object_unref (object);
	}
    }
  
  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
bse_sample_set_param (BseSample *sample,
		      BseParam *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_N_TRACKS:
      sample->n_tracks = param->value.v_uint;
      break;
    case PARAM_REC_FREQ:
      sample->rec_freq = param->value.v_uint;
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to set parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (sample),
		 BSE_OBJECT_NAME (sample),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

static void
bse_sample_get_param (BseSample *sample,
		      BseParam *param)
{
  switch (param->pspec->any.param_id)
    {
    case PARAM_N_TRACKS:
      param->value.v_uint = sample->n_tracks;
      break;
    case PARAM_REC_FREQ:
      param->value.v_uint = sample->rec_freq;
      break;
    default:
      g_warning ("%s(\"%s\"): invalid attempt to get parameter \"%s\" of type `%s'",
		 BSE_OBJECT_TYPE_NAME (sample),
		 BSE_OBJECT_NAME (sample),
		 param->pspec->any.name,
		 bse_type_name (param->pspec->type));
      break;
    }
}

BseSample*
bse_sample_new (const gchar *first_param_name,
		...)
{
  BseObject *object;
  va_list var_args;
  
  va_start (var_args, first_param_name);
  object = bse_object_new_valist (BSE_TYPE_SAMPLE, first_param_name, var_args);
  va_end (var_args);
  
  return BSE_SAMPLE (object);
}

BseSample*
bse_sample_lookup (BseProject  *project,
		   const gchar *name)
{
  BseItem *item;
  
  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  
  item = bse_container_lookup_item (BSE_CONTAINER (project), name);
  
  return BSE_IS_SAMPLE (item) ? BSE_SAMPLE (item) : NULL;
}

void
bse_sample_fillup_munks (BseSample *sample)
{
  BseMunk *munk = NULL;
  guint i;
  
  g_return_if_fail (BSE_IS_SAMPLE (sample));
  
  /* find valid munks
   */
  for (i = 0; i < BSE_MAX_SAMPLE_MUNKS; i++)
    {
      if (sample->munks[i].bin_data)
	{
	  munk = sample->munks + i;
	  break;
	}
    }
  
  if (!munk)
    {
      g_warning ("BseSample: attempt to fillup \"%s\", with empty BseMunks",
		 BSE_OBJECT_NAME (sample));
      return;
    }
  
  for (i = 0; i < BSE_MAX_SAMPLE_MUNKS; i++)
    {
      if (!sample->munks[i].bin_data)
	bse_sample_set_munk (sample,
			     i,
			     munk->rec_note,
			     munk->loop_begin,
			     munk->loop_end,
			     munk->bin_data);
      else if (sample->munks[i].bin_data != munk->bin_data)
	munk = sample->munks + i;
    }
}

static inline void
bse_sample_set_munk_i (BseSample     *sample,
		       guint	      munk_index,
		       guint	      recording_note,
		       guint	      loop_begin,
		       guint	      loop_end,
		       BseBinData    *bin_data)
{
  BseMunk *munk;
  
  munk = &sample->munks[munk_index];
  
  if (munk->bin_data)
    bse_object_unref (BSE_OBJECT (munk->bin_data));
  
  munk->rec_note = recording_note;
  munk->loop_begin = loop_begin;
  munk->loop_end = loop_end;
  munk->bin_data = bin_data;
  bse_object_ref (BSE_OBJECT (munk->bin_data));
}

void
bse_sample_set_munk (BseSample	   *sample,
		     guint	    munk_index,
		     guint	    recording_note,
		     guint	    loop_begin,
		     guint	    loop_end,
		     BseBinData    *bin_data)
{
  g_return_if_fail (BSE_IS_SAMPLE (sample));
  g_return_if_fail (!BSE_OBJECT_IS_LOCKED (sample));
  g_return_if_fail (munk_index < BSE_MAX_SAMPLE_MUNKS);
  g_return_if_fail (recording_note >= BSE_MIN_NOTE && recording_note <= BSE_MAX_NOTE);
  g_return_if_fail (BSE_IS_BIN_DATA (bin_data));
  g_return_if_fail (loop_begin <= loop_end);
  g_return_if_fail (loop_end <= bin_data->n_values);
  
  bse_sample_set_munk_i (sample, munk_index, recording_note,
			 loop_begin, loop_end,
			 bin_data);
}

static void
bse_sample_do_store_private (BseObject	*object,
			     BseStorage *storage)
{
  BseSample *sample = BSE_SAMPLE (object);
  BseMunk *munk = NULL;
  guint i;
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);
  
  for (i = 0; i < BSE_MAX_SAMPLE_MUNKS; i++)
    if (sample->munks[i].bin_data &&
	(!munk || memcmp (munk, sample->munks + i, sizeof (*munk))))
      {
	gchar *string;
	gboolean need_break = FALSE;
	
	munk = &sample->munks[i];
	bse_storage_break (storage);
	string = bse_note_to_string (i);
	bse_storage_printf (storage, "(munk %s", string);
	g_free (string);
	bse_storage_push_level (storage);
	
	if (munk->rec_note != BSE_KAMMER_NOTE)
	  {
	    bse_storage_break (storage);
	    string = bse_note_to_string (i);
	    bse_storage_printf (storage, "(recording-note %s)", string);
	    g_free (string);
	    need_break = TRUE;
	  }
	if (munk->loop_begin && munk->loop_end)
	  {
	    bse_storage_break (storage);
	    bse_storage_printf (storage, "(loop %u %u)", munk->loop_begin, munk->loop_end);
	    need_break = TRUE;
	  }
	if (need_break)
	  bse_storage_break (storage);
	else
	  bse_storage_putc (storage, ' ');
	bse_storage_printf (storage, "(bin-data ");
	bse_storage_put_bin_data (storage, munk->bin_data);
	bse_storage_putc (storage, ')');
	
	bse_storage_pop_level (storage);
	bse_storage_handle_break (storage);
	bse_storage_putc (storage, ')');
      }
}

static BseTokenType
parse_munk (BseSample  *sample,
	    BseStorage *storage,
	    BseMunk    *munk)
{
  static GQuark quark_loop = 0;
  static GQuark quark_rec_note = 0;
  static GQuark quark_bin_data = 0;
  GScanner *scanner = storage->scanner;
  GQuark token_quark;
  
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return BSE_TOKEN_UNMATCHED;
  
  if (!quark_loop)
    {
      quark_loop = g_quark_from_static_string ("loop");
      quark_rec_note = g_quark_from_static_string ("recording-note");
      quark_bin_data = g_quark_from_static_string ("bin-data");
    }

  token_quark = g_quark_try_string (scanner->next_value.v_identifier);
  if (token_quark == quark_loop)
    {
      guint loop_begin, loop_end;
      
      g_scanner_get_next_token (scanner);
      
      if (g_scanner_get_next_token (scanner) != G_TOKEN_INT)
	return G_TOKEN_INT;
      loop_begin = scanner->value.v_int;
      if (g_scanner_get_next_token (scanner) != G_TOKEN_INT)
	return G_TOKEN_INT;
      loop_end = scanner->value.v_int;
      if (munk->loop_begin > munk->loop_end)
	bse_storage_warn (storage, "invalid loop interval [%u...%u]", loop_begin, loop_end);
      else
	{
	  munk->loop_begin = loop_begin;
	  munk->loop_end = loop_end;
	}
    }
  else if (token_quark == quark_rec_note)
    {
      guint rec_note;
      gchar bbuffer[BSE_BBUFFER_SIZE];
      GTokenType expected_token;
      
      g_scanner_get_next_token (scanner);
      
      expected_token = bse_storage_parse_note (storage, &rec_note, bbuffer);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;
      if (rec_note == BSE_NOTE_UNPARSABLE ||
	  rec_note == BSE_NOTE_VOID)
	return bse_storage_warn_skip (storage, "invalid note `%s'", bbuffer);
    }
  else if (token_quark == quark_bin_data)
    {
      GTokenType expected_token;

      g_scanner_get_next_token (scanner);
      
      expected_token = bse_storage_parse_bin_data (storage, &munk->bin_data);
      if (expected_token != G_TOKEN_NONE)
	return expected_token;
      if (!munk->bin_data)
	return bse_storage_warn_skip (storage, "invalid binary data reference");
    }
  else
    return BSE_TOKEN_UNMATCHED;
  
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
}

static BseTokenType
bse_sample_do_restore_private (BseObject  *object,
			       BseStorage *storage)
{
  BseSample *sample = BSE_SAMPLE (object);
  GScanner *scanner = storage->scanner;
  GTokenType expected_token = BSE_TOKEN_UNMATCHED;
  gchar bbuffer[BSE_BBUFFER_SIZE];
  guint note;
  BseMunk munk = {
    BSE_NOTE_VOID,
    0, 0,
    NULL,
  };
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore_private)
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage);
  
  /* feature munks */
  if (expected_token != BSE_TOKEN_UNMATCHED ||
      g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER ||
      !bse_string_equals ("munk", scanner->next_value.v_identifier))
    return expected_token;
  
  g_scanner_get_next_token (scanner); /* eat "munk" */
  
  /* parse note */
  expected_token = bse_storage_parse_note (storage, &note, bbuffer);
  if (expected_token != G_TOKEN_NONE)
    return expected_token;
  if (note == BSE_NOTE_UNPARSABLE ||
      note == BSE_NOTE_VOID)
    return bse_storage_warn_skip (storage, "invalid note `%s'", bbuffer);
  note = CLAMP (note, 0, BSE_MAX_SAMPLE_MUNKS);

  expected_token = bse_storage_parse_rest (storage,
					   (BseTryStatement) parse_munk,
					   sample,
					   &munk);

  if (expected_token == G_TOKEN_NONE)
    {
      if (munk.bin_data)
	{
	  if (munk.loop_begin > munk.bin_data->n_values ||
	      munk.loop_end > munk.bin_data->n_values)
	    {
	      bse_storage_warn (storage,
				"loop interval [%u...%u] exceeds sample range (%u)",
				munk.loop_begin,
				munk.loop_end,
				munk.bin_data->n_values);
	      munk.loop_begin = 0;
	      munk.loop_end = 0;
	    }
	  if (munk.rec_note == BSE_NOTE_VOID)
	    munk.rec_note = BSE_DFL_SAMPLE_REC_NOTE;
	  bse_sample_set_munk (sample,
			       note,
			       munk.rec_note,
			       munk.loop_begin,
			       munk.loop_end,
			       munk.bin_data);
	  
	}
      else if (munk.loop_begin || munk.loop_end)
	bse_storage_warn (storage,
			  "loop interval [%u...%u] specified without sample data",
			  munk.loop_begin,
			  munk.loop_end);
      else if (munk.rec_note != BSE_NOTE_VOID)
	bse_storage_warn (storage, "recording note specified without sample data");
    }

  return expected_token;
}

static GTokenType
bse_sample_do_restore (BseObject  *object,
		       BseStorage *storage)
{
  GTokenType expected_token = G_TOKEN_NONE;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore)
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore (object, storage);

  bse_sample_fillup_munks (BSE_SAMPLE (object));

  return expected_token;
 }
