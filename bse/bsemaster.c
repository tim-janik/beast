/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include        "bsemaster.h"

#include        "bsestream.h"
#include        "bsechunk.h"


#define	MASTER_MAX_N_TRACKS	(2)


/* --- prototypes --- */
static void     bse_master_class_init           (BseMasterClass *class);
static void     bse_master_init                 (BseMaster      *master);
static void     bse_master_destroy              (BseObject      *object);
static void	bse_master_do_cycle		(BseSource	*source);
static void     bse_master_do_add_input         (BseSource      *source,
						 guint           ichannel_id,
						 BseSource      *input,
						 guint           ochannel_id,
						 guint           history);
static void     bse_master_do_remove_input      (BseSource      *source,
						 guint           input_index);


/* --- variables --- */
static BseTypeClass     *parent_class = NULL;
static GSList		*bse_masters = NULL;
/*FIXME: static */ BseIndex		 bse_index = -1;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseMaster)
{
  static const BseTypeInfo master_info = {
    sizeof (BseMasterClass),

    (BseBaseInitFunc) NULL,
    (BseBaseDestroyFunc) NULL,
    (BseClassInitFunc) bse_master_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    sizeof (BseMaster),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_master_init,
  };

  return bse_type_register_static (BSE_TYPE_SOURCE,
				   "BseMaster",
				   "Master drain for streamed output",
				   &master_info);
}

static void
bse_master_class_init (BseMasterClass *class)
{
  BseObjectClass *object_class;
  BseItemClass *item_class;
  BseSourceClass *source_class;

  parent_class = bse_type_class_peek (BSE_TYPE_SOURCE);
  object_class = BSE_OBJECT_CLASS (class);
  item_class = BSE_ITEM_CLASS (class);
  source_class = BSE_SOURCE_CLASS (class);

  object_class->destroy = bse_master_destroy;

  source_class->cycle = bse_master_do_cycle;
  source_class->add_input = bse_master_do_add_input;
  source_class->remove_input = bse_master_do_remove_input;

  bse_source_class_add_ichannel (source_class, "Master input", NULL, 1);
  /* bad hack for multiple inputs */
  source_class->ichannels[0].min_n_tracks = 0;
}

static void
bse_master_init (BseMaster *master)
{
  master->n_tracks = 0;
  master->stream = NULL;
  master->chunks = NULL;
  
  bse_masters = g_slist_prepend (bse_masters, master);
}

static void
bse_master_destroy (BseObject *object)
{
  BseMaster *master;
  BseChunk **chunks;
  
  master = BSE_MASTER (object);
  
  bse_masters = g_slist_remove (bse_masters, master);
  
  if (master->stream)
    bse_master_set_output (master, NULL, 0);

  chunks = master->chunks;

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);

  g_free (chunks);
}

BseMaster*
bse_master_new (BseStream *output_stream,
		guint      n_output_tracks)
{
  BseMaster *master;

  g_return_val_if_fail (n_output_tracks <= MASTER_MAX_N_TRACKS, NULL);

  master = bse_object_new (BSE_TYPE_MASTER, NULL);

  if (output_stream && n_output_tracks)
    bse_master_set_output (master, output_stream, n_output_tracks);

  return master;
}

void
bse_master_set_output (BseMaster *master,
		       BseStream *output_stream,
		       guint      n_output_tracks)
{
  g_return_if_fail (BSE_IS_MASTER (master));
  g_return_if_fail (n_output_tracks <= MASTER_MAX_N_TRACKS);

  if (master->stream)
    bse_object_unref (BSE_OBJECT (master->stream));

  master->stream = output_stream;
  if (master->stream)
    bse_object_ref (BSE_OBJECT (master->stream));

  master->n_tracks = n_output_tracks;

  if (master->stream && !BSE_SOURCE_PREPARED (master))
    {
      BSE_OBJECT_SET_FLAGS (master, BSE_SOURCE_FLAG_PREPARED);
      BSE_SOURCE_GET_CLASS (master)->prepare (BSE_SOURCE (master), bse_index);
    }
  else if (!master->stream && BSE_SOURCE_PREPARED (master))
    {
      GSList *slist;
      gboolean can_nuke = TRUE;

      BSE_OBJECT_UNSET_FLAGS (master, BSE_SOURCE_FLAG_PREPARED);
      BSE_SOURCE_GET_CLASS (master)->reset (BSE_SOURCE (master));

      for (slist = bse_masters; slist; slist = slist->next)
	if (BSE_SOURCE_PREPARED (slist->data))
	  can_nuke = FALSE;

      if (can_nuke)
	bse_chunks_nuke ();
    }
}

BseIndex
bse_masters_cycle (void)
{
  GSList *slist;
  
  bse_index++;
  
  for (slist = bse_masters; slist; slist = slist->next)
    {
      BseSource *source = BSE_SOURCE (slist->data);
      
      while (source->index < bse_index)
	bse_source_cycle (source);
      // bse_chunk_debug ();
    }

  return bse_index;
}

static void
bse_master_do_cycle (BseSource *source)
{
  BseMaster *master;

  master = BSE_MASTER (source);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->cycle (source);

  if (source->n_inputs && master->n_tracks && master->stream)
    {
      guint i;

      /* retrive hunk-completed chunks from input sources */
      for (i = 0; i < source->n_inputs; i++)
	master->chunks[i] = bse_source_ref_chunk (source->inputs[i].osource,
						  source->inputs[i].ochannel_id,
						  source->index);

      /* check if we really need source + track mixing */
      if (source->n_inputs == 1)
	{
	  if (master->chunks[0]->n_tracks == master->n_tracks)
	    bse_stream_write_sv (master->stream,
				 BSE_TRACK_LENGTH * master->n_tracks,
				 master->chunks[0]->hunk);
	  else
	    FIXME (need buffer mixing - 3 cases (mono, stereo, n-channel));
	}
      else
	{
	  BseMixValue *buffer;
	  BseSampleValue *sbuffer;
	  guint n_values, track_length = BSE_TRACK_LENGTH;
	  
	  n_values = track_length * master->n_tracks;
	  
	  /* FIXME: we should use bse_mix_buffer_alloc() here */
	  buffer = g_new0 (BseMixValue, n_values);
	  sbuffer = (BseSampleValue*) buffer;
	  
	  for (i = 0; i < source->n_inputs; i++)
	    {
	      BseChunk *chunk = master->chunks[i];
	      
	      if (chunk->n_tracks == master->n_tracks)
		{
		  BseMixValue *mbuf_end, *mbuf = buffer;
		  BseSampleValue *hunk = chunk->hunk;
		  
		  for (mbuf_end = mbuf + n_values; mbuf < mbuf_end; mbuf++)
		    *mbuf += *(hunk++);
		}
	      else
		FIXME (need buffer mixing - 3 cases (mono, stereo, n-channel));
	    }
	  
	  for (i = 0; i < n_values; i++)
	    {
	      register BseMixValue value;
	      
	      value = buffer[i];
	      
	      if (value > 32767)
		value = 32767;
	      else if (value < -32768)
		value = -32768;
	      sbuffer[i] = value;
	    }
	  
	  bse_stream_write_sv (master->stream, n_values, sbuffer);

	  g_free (buffer);
	}

      for (i = 0; i < source->n_inputs; i++)
	{
	  bse_chunk_unref (master->chunks[i]);
	  master->chunks[i] = NULL;
	}
    }
}

void
bse_master_add_source (BseMaster *master,
		       BseSource *source,
		       guint      ochannel_id)
{
  BseErrorType error;
  
  g_return_if_fail (BSE_IS_MASTER (master));
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_HAS_OUTPUT (source));
  g_return_if_fail (ochannel_id >= 1 && ochannel_id <= BSE_SOURCE_N_OCHANNELS (source));

  error = bse_source_set_input (BSE_SOURCE (master), 1, source, ochannel_id, 0);
  g_return_if_fail (error == BSE_ERROR_NONE); /* this should never fail */
}

gboolean
bse_master_remove_source (BseMaster *master,
			  BseSource *source)
{
  gboolean retry, retval = 0;

  g_return_val_if_fail (BSE_IS_MASTER (master), FALSE);
  g_return_val_if_fail (BSE_IS_SOURCE (source), FALSE);

  do
    {
      retry = bse_source_remove_input (BSE_SOURCE (master), source);
      retval |= retry;
    }
  while (retry);

  return retval;
}

void
bse_masters_reset (void)
{
  GSList *slist;
  
  slist = bse_masters;
  if (slist)
    bse_object_ref (slist->data);
  while (slist)
    {
      BseSource *source = slist->data;

      while (source->n_inputs)
	bse_source_remove_input (source, source->inputs[0].osource);

      slist = slist->next;
      if (slist)
	bse_object_ref (slist->data);

      bse_object_unref (BSE_OBJECT (source));
    }
}

static void
bse_master_do_add_input (BseSource *source,
			 guint      ichannel_id,
			 BseSource *input,
			 guint      ochannel_id,
			 guint      history)
{
  BseMaster *master;
  
  master = BSE_MASTER (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->add_input (source, ichannel_id,
					      input, ochannel_id,
					      history);
  
  master->chunks = g_renew (BseChunk*, master->chunks, source->n_inputs);
  master->chunks[source->n_inputs - 1] = NULL;
}

static void
bse_master_do_remove_input (BseSource *source,
			    guint      input_index)
{
  BseMaster *master;
  
  master = BSE_MASTER (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->remove_input (source, input_index);
  
  if (input_index < source->n_inputs)
    master->chunks[input_index] = master->chunks[source->n_inputs];
}
