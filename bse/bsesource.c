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
#include        "bsesource.h"
#include        "bsechunk.h"



/* --- prototypes --- */
static void     bse_source_class_base_init	(BseSourceClass	*class);
static void     bse_source_class_base_destroy	(BseSourceClass	*class);
static void     bse_source_class_init		(BseSourceClass	*class);
static void     bse_source_init			(BseSource	*source,
						 BseSourceClass	*class);
static void     bse_source_do_shutdown		(BseObject	*object);
static void     bse_source_calc_history		(BseSource	*source,
						 guint		 ochannel_id);
static void     bse_source_do_prepare		(BseSource	*source,
						 BseIndex	 index);
static void     bse_source_do_reset		(BseSource	*source);
static void     bse_source_do_cycle		(BseSource	*source);
static void	bse_source_do_add_input		(BseSource	*source,
						 guint     	 ichannel_id,
						 BseSource 	*input,
						 guint     	 ochannel_id);
static void	bse_source_do_remove_input	(BseSource	*source,
						 guint		 input_index);


/* --- variables --- */
static BseTypeClass     *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSource)
{
  static const BseTypeInfo source_info = {
    sizeof (BseSourceClass),
    
    (BseBaseInitFunc) bse_source_class_base_init,
    (BseBaseDestroyFunc) bse_source_class_base_destroy,
    (BseClassInitFunc) bse_source_class_init,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSource),
    0 /* n_preallocs */,
    (BseObjectInitFunc) bse_source_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseSource",
				   "Base type for sound sources",
				   &source_info);
}

static void
bse_source_class_base_init (BseSourceClass *class)
{
  class->n_ichannels = 0;
  class->ichannel_defs = NULL;
  class->n_ochannels = 0;
  class->ochannel_defs = NULL;
}

static void
bse_source_class_base_destroy (BseSourceClass *class)
{
  guint i;
  
  for (i = 0; i < class->n_ichannels; i++)
    {
      g_free (class->ichannel_defs[i].name);
      g_free (class->ichannel_defs[i].blurb);
    }
  g_free (class->ichannel_defs);
  for (i = 0; i < class->n_ochannels; i++)
    {
      g_free (class->ochannel_defs[i].name);
      g_free (class->ochannel_defs[i].blurb);
    }
  g_free (class->ochannel_defs);
  
  class->n_ichannels = 0;
  class->ichannel_defs = NULL;
  class->n_ochannels = 0;
  class->ochannel_defs = NULL;
}

static void
bse_source_class_init (BseSourceClass *class)
{
  BseObjectClass *object_class;
  
  parent_class = bse_type_class_peek (BSE_TYPE_ITEM);
  object_class = BSE_OBJECT_CLASS (class);
  
  object_class->shutdown = bse_source_do_shutdown;

  class->prepare = bse_source_do_prepare;
  class->calc_chunk = NULL;
  class->reset = bse_source_do_reset;

  class->cycle = bse_source_do_cycle;
  class->add_input = bse_source_do_add_input;
  class->remove_input = bse_source_do_remove_input;
}

static void
bse_source_init (BseSource      *source,
		 BseSourceClass *class)
{
  guint i;

  if (class->n_ichannels)
    BSE_OBJECT_SET_FLAGS (source, BSE_SOURCE_FLAG_HAS_INPUT);
  if (class->n_ochannels)
    BSE_OBJECT_SET_FLAGS (source, BSE_SOURCE_FLAG_HAS_OUTPUT);
  
  source->n_inputs = 0;
  source->inputs = NULL;
  source->outputs = NULL;

  source->start = -1;
  source->index = 0;

  source->ochannels = g_new0 (BseSourceOChannel, class->n_ochannels);
  for (i = 0; i < class->n_ochannels; i++)
    source->ochannels[i].chunks = NULL;
}

static void
bse_source_do_shutdown (BseObject *object)
{
  BseSource *source;
  guint i;
  
  source = BSE_SOURCE (object);

  bse_source_clear_ochannels (source);

  g_return_if_fail (!BSE_SOURCE_PREPARED (source));

  if (source->n_inputs)
    {
      while (source->n_inputs)
	BSE_SOURCE_GET_CLASS (source)->remove_input (source, source->n_inputs - 1);
      BSE_NOTIFY (source, io_changed, NOTIFY (OBJECT, DATA));
    }

  g_free (source->inputs);
  
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    {
      BseSourceOChannel *oc = BSE_SOURCE_OCHANNEL (source, i + 1);
      guint n;

      for (n = 0; n < oc->history; n++)
	if (oc->chunks[n])
	  bse_chunk_unref (oc->chunks[n]);
      g_free (oc->chunks);
    }
  g_free (source->ochannels);
  source->ochannels = NULL;

  /* chain parent class' shutdown handler */
  BSE_OBJECT_CLASS (parent_class)->shutdown (object);
}

guint
bse_source_class_add_ichannel (BseSourceClass *source_class,
			       const gchar    *name,
			       const gchar    *blurb,
			       guint           min_n_tracks,
			       guint           max_n_tracks)
{
  guint index;

  g_return_val_if_fail (BSE_IS_SOURCE_CLASS (source_class), 0);
  g_return_val_if_fail (name != NULL, 0);
  if (!blurb)
    blurb = name;
  g_return_val_if_fail (min_n_tracks >= 1 && min_n_tracks <= BSE_MAX_N_TRACKS, 0);
  g_return_val_if_fail (max_n_tracks >= min_n_tracks && max_n_tracks <= BSE_MAX_N_TRACKS, 0);
  
  index = source_class->n_ichannels;
  source_class->n_ichannels++;
  source_class->ichannel_defs = g_renew (BseSourceIChannelDef,
					 source_class->ichannel_defs,
					 source_class->n_ichannels);
  source_class->ichannel_defs[index].name = g_strdup (name);
  source_class->ichannel_defs[index].blurb = g_strdup (blurb);
  source_class->ichannel_defs[index].min_n_tracks = min_n_tracks;
  source_class->ichannel_defs[index].max_n_tracks = max_n_tracks;
  source_class->ichannel_defs[index].min_history = 0;
  
  return index + 1;
}

guint
bse_source_class_add_ochannel (BseSourceClass *source_class,
			       const gchar    *name,
			       const gchar    *blurb,
			       guint           n_tracks)
{
  guint index;

  g_return_val_if_fail (BSE_IS_SOURCE_CLASS (source_class), 0);
  g_return_val_if_fail (name != NULL, 0);
  if (!blurb)
    blurb = name;
  g_return_val_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS, 0);
  
  index = source_class->n_ochannels;
  source_class->n_ochannels++;
  source_class->ochannel_defs = g_renew (BseSourceOChannelDef,
					 source_class->ochannel_defs,
					 source_class->n_ochannels);
  source_class->ochannel_defs[index].name = g_strdup (name);
  source_class->ochannel_defs[index].blurb = g_strdup (blurb);
  source_class->ochannel_defs[index].n_tracks = n_tracks;

  return index + 1;
}

static void
bse_source_calc_history (BseSource *source,
			 guint      ochannel_id)
{
  BseSourceOChannel *oc = BSE_SOURCE_OCHANNEL (source, ochannel_id);
  guint history = 0;
  BseChunk **chunks;
  GSList *slist;
  guint i, n;
  
  g_return_if_fail (oc->in_calc == FALSE); /* paranoid */
  
  for (slist = source->outputs; slist; slist = slist->next)
    {
      BseSource *output = slist->data;
      
      for (n = 0; n < output->n_inputs; n++)
	if (output->inputs[n].osource == source &&
	    output->inputs[n].ochannel_id == ochannel_id)
	  {
	    BseSourceIChannelDef *ic_def;

	    ic_def = BSE_SOURCE_ICHANNEL_DEF (output, output->inputs[n].ichannel_id);
	    history = MAX (history, MAX (ic_def->min_history, 1));
	  }
    }

  chunks = g_new (BseChunk*, history);

  if (!oc->chunks)
    {
      /* object just got inititalized */
      for (i = 0; i < history; i++)
	chunks[i] = NULL;
    }
  else
    {
      n = MIN (history, oc->history);
      for (i = 0; i < n; i++)
	chunks[i] = oc->chunks[(oc->ring_offset + i) % oc->history];
      
      for (n = i; n < history; n++)
	chunks[n] = NULL;
      
      for (; i < oc->history; i++)
	if (oc->chunks[(oc->ring_offset + i) % oc->history])
	  bse_chunk_unref (oc->chunks[(oc->ring_offset + i) % oc->history]);
    }
  
  g_free (oc->chunks);
  oc->chunks = chunks;
  oc->history = history;
  oc->ring_offset = 0;
}

static inline BseChunk*
bse_source_fetch_chunk (BseSource *source,
			guint      ochannel_id,
			BseIndex   index)
{
  BseSourceOChannel *oc;
  BseChunk *chunk;
  guint n_tracks;

  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);
  g_return_val_if_fail (BSE_SOURCE_PREPARED (source), NULL);
  g_return_val_if_fail (ochannel_id >= 1 && ochannel_id <= BSE_SOURCE_N_OCHANNELS (source), NULL);
  g_return_val_if_fail (index <= source->index, NULL);

  n_tracks = BSE_SOURCE_OCHANNEL_DEF (source, ochannel_id)->n_tracks;
  oc = BSE_SOURCE_OCHANNEL (source, ochannel_id);
  index = source->index - index;
  if (index > oc->history || index < 0)
    {
      g_warning ("%s (\"%s\"): invalid (%lld - %lld = %lld) history chunk requested on %u",
		 BSE_OBJECT_TYPE_NAME (source),
		 BSE_OBJECT_NAME (source) ? BSE_OBJECT_NAME (source) : "",
		 source->index,
		 source->index - index,
		 index,
		 ochannel_id);
      return bse_chunk_new_static_zero (n_tracks);
    }

  if (index || oc->in_calc)
    {
      chunk = oc->chunks[(index + oc->ring_offset) % oc->history];
      if (!chunk)
	chunk = bse_chunk_new_static_zero (n_tracks);
      else
	bse_chunk_ref (chunk);
    }
  else
    {
      if (!oc->chunks[oc->ring_offset])
	{
	  BseChunk* (*calc_chunk) (BseSource *source,
				   guint      ochannel_id) = BSE_SOURCE_GET_CLASS (source)->calc_chunk;

	  oc->in_calc = TRUE;
	  if (!calc_chunk) /* FIXME: paranoid */
	    {
	      g_warning ("`%s' doesn't implement ->calc_chunk() memeber function",
			 bse_type_name (BSE_OBJECT_TYPE (source)));
	      oc->chunks[oc->ring_offset] = bse_chunk_new_static_zero (n_tracks);
	    }
	  else
	    oc->chunks[oc->ring_offset] = BSE_SOURCE_GET_CLASS (source)->calc_chunk (source, ochannel_id);
	  oc->in_calc = FALSE;
	}
      chunk = oc->chunks[oc->ring_offset];
      bse_chunk_ref (chunk);
    }

  return chunk;
}

BseChunk*
bse_source_ref_chunk (BseSource *source,
		      guint      ochannel_id,
		      BseIndex   index)
{
  BseChunk *chunk;

  chunk = bse_source_fetch_chunk (source, ochannel_id, index);

  bse_chunk_complete_hunk (chunk);

  return chunk;
}

BseChunk*
bse_source_ref_state_chunk (BseSource *source,
			    guint      ochannel_id,
			    BseIndex   index)
{
  BseChunk *chunk;

  chunk = bse_source_fetch_chunk (source, ochannel_id, index);

  bse_chunk_complete_state (chunk);

  return chunk;
}

void
bse_source_set_paused (BseSource *source,
		       gboolean   paused)
{
  g_return_if_fail (BSE_IS_SOURCE (source));

  paused = paused != FALSE;

  if (BSE_SOURCE_PAUSED (source) != paused)
    {
      if (paused)
	BSE_OBJECT_SET_FLAGS (source, BSE_SOURCE_FLAG_PAUSED);
      else
	BSE_OBJECT_UNSET_FLAGS (source, BSE_SOURCE_FLAG_PAUSED);
    }
}

void
bse_source_cycle (BseSource *source)
{
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_PREPARED (source));
  g_return_if_fail (!BSE_OBJECT_DESTROYED (source));
  
  bse_object_ref (BSE_OBJECT (source));
  BSE_SOURCE_GET_CLASS (source)->cycle (source);
  bse_object_unref (BSE_OBJECT (source));
}

static void
bse_source_do_cycle (BseSource *source)
{
  guint i;
  
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    {
      BseSourceOChannel *oc = BSE_SOURCE_OCHANNEL (source, i + 1);
      
      if (oc->history)
	{
	  oc->ring_offset = (oc->ring_offset + 1) % oc->history;
	  if (oc->chunks[oc->ring_offset])
	    {
	      bse_chunk_unref (oc->chunks[oc->ring_offset]);
	      oc->chunks[oc->ring_offset] = NULL;
	    }
	}
    }
  
  source->index++;

  for (i = 0; i < source->n_inputs; i++)
    {
      BseSource *input = source->inputs[i].osource;

      while (input->index < source->index)
	bse_source_cycle (input);
    }

  if (!BSE_SOURCE_PAUSED (source))
    {
      BseChunk* (*calc_chunk) (BseSource *source,
			       guint      ochannel_id) = BSE_SOURCE_GET_CLASS (source)->calc_chunk;

      if (!calc_chunk && BSE_SOURCE_N_OCHANNELS (source)) /* FIXME: paranoid */
	{
	  g_warning ("`%s' doesn't implement ->calc_chunk() memeber function",
		     bse_type_name (BSE_OBJECT_TYPE (source)));
	  return;
	}

      for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
	{
	  BseSourceOChannel *oc = BSE_SOURCE_OCHANNEL (source, i + 1);
	  
	  if (oc->history && !oc->chunks[oc->ring_offset])
	    {
	      oc->in_calc = TRUE;
	      oc->chunks[oc->ring_offset] = calc_chunk (source, i + 1);
	      oc->in_calc = FALSE;
	    }
	}
    }
}

BseErrorType
bse_source_set_input (BseSource *source,
		      guint      ichannel_id,
		      BseSource *input,
		      guint      ochannel_id)
{
  BseSourceIChannelDef *ic_def;
  BseSourceOChannelDef *oc_def;
  guint i;
  
  g_return_val_if_fail (BSE_IS_SOURCE (source), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_SOURCE_HAS_INPUT (source), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_IS_SOURCE (input), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_SOURCE_HAS_OUTPUT (input), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_OBJECT_DESTROYED (source), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_OBJECT_DESTROYED (input), BSE_ERROR_INTERNAL);
  
  ic_def = BSE_SOURCE_ICHANNEL_DEF (source, ichannel_id);
  oc_def = BSE_SOURCE_OCHANNEL_DEF (input, ochannel_id);
  
  if (ichannel_id < 1 || ichannel_id > BSE_SOURCE_N_ICHANNELS (source))
    return BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL;
  if (ochannel_id < 1 || ochannel_id > BSE_SOURCE_N_OCHANNELS (input))
    return BSE_ERROR_SOURCE_NO_SUCH_OCHANNEL;

  if (ic_def->min_n_tracks > oc_def->n_tracks)
    return BSE_ERROR_SOURCE_TOO_MANY_ITRACKS;
  if (ic_def->max_n_tracks < oc_def->n_tracks)
    return BSE_ERROR_SOURCE_TOO_MANY_OTRACKS;
  
  if (ic_def->min_n_tracks) /* FIXME: bad hack */
    for (i = 0; i < source->n_inputs; i++)
      if (source->inputs[i].ichannel_id == ichannel_id)
	return BSE_ERROR_SOURCE_ICHANNEL_IN_USE;
  
  bse_object_ref (BSE_OBJECT (source));
  bse_object_ref (BSE_OBJECT (input));

  BSE_SOURCE_GET_CLASS (source)->add_input (source, ichannel_id, input, ochannel_id);
  bse_source_calc_history (input, ochannel_id);
  BSE_NOTIFY (source, io_changed, NOTIFY (OBJECT, DATA));
  BSE_NOTIFY (input, io_changed, NOTIFY (OBJECT, DATA));
  
  bse_object_unref (BSE_OBJECT (input));
  bse_object_unref (BSE_OBJECT (source));
  
  return BSE_ERROR_NONE;
}

static void
bse_source_do_add_input (BseSource *source,
			 guint      ichannel_id,
			 BseSource *input,
			 guint      ochannel_id)
{
  guint i = source->n_inputs;

  source->n_inputs += 1;
  source->inputs = g_renew (BseSourceInput, source->inputs, source->n_inputs);
  source->inputs[i].ichannel_id = ichannel_id;
  source->inputs[i].osource = input;
  source->inputs[i].ochannel_id = ochannel_id;
  
  input->outputs = g_slist_prepend (input->outputs, source);

  if (BSE_SOURCE_PREPARED (source) && !BSE_SOURCE_PREPARED (input))
    {
      BSE_OBJECT_SET_FLAGS (input, BSE_SOURCE_FLAG_PREPARED);
      BSE_SOURCE_GET_CLASS (input)->prepare (input, MAX (-1, source->index - 1));
    }
}

BseSourceInput*
bse_source_get_input (BseSource *source,
		      guint      ichannel_id)
{
  guint i;

  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);
  g_return_val_if_fail (ichannel_id >= 1 && ichannel_id <= BSE_SOURCE_N_ICHANNELS (source), NULL);

  for (i = 0; i < source->n_inputs; i++)
    if (source->inputs[i].ichannel_id == ichannel_id)
      return source->inputs + i;

  return NULL;
}

gboolean
bse_source_remove_input (BseSource *source,
			 BseSource *input)
{
  guint i;

  g_return_val_if_fail (BSE_IS_SOURCE (source), FALSE);
  g_return_val_if_fail (BSE_IS_SOURCE (input), FALSE);

  for (i = 0; i < source->n_inputs; i++)
    if (source->inputs[i].osource == input)
      {
	bse_object_ref (BSE_OBJECT (source));
        BSE_SOURCE_GET_CLASS (source)->remove_input (source, i);
	BSE_NOTIFY (source, io_changed, NOTIFY (OBJECT, DATA));
	bse_object_unref (BSE_OBJECT (source));

	return TRUE;
      }

  return FALSE;
}

static void
bse_source_do_remove_input (BseSource *source,
			    guint      input_index)
{
  BseSource *osource;

  osource = source->inputs[input_index].osource;

  source->n_inputs--;
  if (input_index < source->n_inputs)
    source->inputs[input_index] = source->inputs[source->n_inputs];

  osource->outputs = g_slist_remove (osource->outputs, source);

  bse_object_ref (BSE_OBJECT (osource));
  if (!osource->outputs && BSE_SOURCE_PREPARED (osource))
    {
      /* FIXME: i guess this is not right, we should also
       * un-prepare, i.e. reset the osource if all the remaining outputs
       * are un-prepared
       * FIXME: this whole concept still has issues with sources that
       * serve as outputs for themselves.
       */
      BSE_OBJECT_UNSET_FLAGS (osource, BSE_SOURCE_FLAG_PREPARED);
      BSE_SOURCE_GET_CLASS (osource)->reset (osource);
    }
  BSE_NOTIFY (osource, io_changed, NOTIFY (OBJECT, DATA));
  bse_object_unref (BSE_OBJECT (osource));
}

void
bse_source_clear_ichannel (BseSource *source,
			   guint      ichannel_id)
{
  guint i;

  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (ichannel_id >= 1 && ichannel_id <= BSE_SOURCE_N_ICHANNELS (source));

  bse_object_ref (BSE_OBJECT (source));

  for (i = 0; i < source->n_inputs; i++)
    if (source->inputs[i].ichannel_id == ichannel_id)
      {
	BSE_SOURCE_GET_CLASS (source)->remove_input (source, i);
	BSE_NOTIFY (source, io_changed, NOTIFY (OBJECT, DATA));
	break;
      }

  bse_object_unref (BSE_OBJECT (source));
}

void
bse_source_clear_ichannels (BseSource *source)
{
  g_return_if_fail (BSE_IS_SOURCE (source));

  bse_object_ref (BSE_OBJECT (source));

  if (source->n_inputs)
    {
      while (source->n_inputs)
	BSE_SOURCE_GET_CLASS (source)->remove_input (source, source->n_inputs - 1);
      BSE_NOTIFY (source, io_changed, NOTIFY (OBJECT, DATA));
    }

  bse_object_unref (BSE_OBJECT (source));
}

void
bse_source_clear_ochannels (BseSource *source)
{
  g_return_if_fail (BSE_IS_SOURCE (source));

  bse_object_ref (BSE_OBJECT (source));

  while (source->outputs)
    bse_source_remove_input (source->outputs->data, source);

  bse_object_unref (BSE_OBJECT (source));
}

static void
bse_source_do_prepare (BseSource *source,
		       BseIndex   index)
{
  guint i;

  source->start = index;
  source->index = index;
  
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    bse_source_calc_history (source, i + 1);

  for (i = 0; i < source->n_inputs; i++)
    {
      BseSource *input = source->inputs[i].osource;
      
      if (!BSE_SOURCE_PREPARED (input))
	{
	  BSE_OBJECT_SET_FLAGS (input, BSE_SOURCE_FLAG_PREPARED);
	  BSE_SOURCE_GET_CLASS (input)->prepare (input, index);
	}
    }
}

static void
bse_source_do_reset (BseSource *source)
{
  guint i;
  
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    {
      BseSourceOChannel *oc = BSE_SOURCE_OCHANNEL (source, i + 1);
      guint n;

      if (oc->in_calc)
	{
	  g_warning ("can't reset ochannel contents for %s \"%s\", in_calc flag not cleared",
		     BSE_OBJECT_TYPE_NAME (source),
		     BSE_OBJECT_NAME (source));
	  continue;
	}

      for (n = 0; n < oc->history; n++)
	if (oc->chunks[n])
	  {
	    bse_chunk_unref (oc->chunks[n]);
	    oc->chunks[n] = NULL;
	  }
      oc->ring_offset = 0;
      oc->history = 0;
    }
  
  source->start = -1;
  source->index = 0;
  
  /* reset all input sources for which we are the sole prepared output
   */
  for (i = 0; i < source->n_inputs; i++)
    {
      BseSource *input = source->inputs[i].osource;
      GSList *slist;
      
      for (slist = input->outputs; slist && input; slist = slist->next)
	if (slist->data != source && BSE_SOURCE_PREPARED (slist->data))
	  input = NULL;
      
      if (input)
	{
	  if (BSE_SOURCE_PREPARED (input))
	    {
	      bse_object_ref (BSE_OBJECT (input));
	      BSE_OBJECT_UNSET_FLAGS (input, BSE_SOURCE_FLAG_PREPARED);
	      BSE_SOURCE_GET_CLASS (input)->reset (input);
	      bse_object_unref (BSE_OBJECT (input));
	    }
	  else
	    g_warning ("source's `%s' input `%s' ought to be prepared", /* paranoid */
		       BSE_OBJECT_TYPE_NAME (source),
		       BSE_OBJECT_TYPE_NAME (input));
	}
    }
}

GList*
bse_source_list_inputs (BseSource *source)
{
  GList *list;
  guint i;
  
  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);
  
  list = NULL;
  for (i = 0; i < source->n_inputs; i++)
    list = g_list_prepend (list, source->inputs[i].osource);
  
  return list;
}
