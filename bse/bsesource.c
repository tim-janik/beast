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
#include        "bsecontainer.h"
#include        "bsestorage.h"
#include        "bseheart.h"



/* --- prototypes --- */
static void         bse_source_class_base_init		(BseSourceClass	*class);
static void         bse_source_class_base_destroy	(BseSourceClass	*class);
static void         bse_source_class_init		(BseSourceClass	*class);
static void         bse_source_init			(BseSource	*source,
							 BseSourceClass	*class);
static void         bse_source_do_shutdown		(BseObject	*object);
static void         bse_source_calc_history		(BseSource	*source,
							 guint		 ochannel_id);
static void         bse_source_do_prepare		(BseSource	*source,
							 BseIndex	 index);
static void         bse_source_do_cycle			(BseSource	*source);
static void         bse_source_do_reset			(BseSource	*source);
static BseChunk*    bse_source_default_calc_chunk	(BseSource      *source,
							 guint           ochannel_id);
static BseChunk*    bse_source_default_skip_chunk	(BseSource      *source,
							 guint           ochannel_id);
static void	    bse_source_do_add_input		(BseSource	*source,
							 guint     	 ichannel_id,
							 BseSource 	*input,
							 guint     	 ochannel_id);
static void	    bse_source_do_remove_input		(BseSource	*source,
							 guint		 input_index);
static void	    bse_source_do_store_private		(BseObject	*object,
							 BseStorage	*storage);
static BseTokenType bse_source_do_restore_private	(BseObject      *object,
							 BseStorage     *storage);


/* --- variables --- */
static BseTypeClass *parent_class = NULL;
static GQuark        quark_deferred_input = 0;


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

  g_assert (BSE_SOURCE_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT);
  
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
  
  object_class->store_private = bse_source_do_store_private;
  object_class->restore_private = bse_source_do_restore_private;
  object_class->shutdown = bse_source_do_shutdown;

  class->prepare = bse_source_do_prepare;
  class->calc_chunk = bse_source_default_calc_chunk;
  class->skip_chunk = bse_source_default_skip_chunk;
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

  if (bse_object_get_qdata (object, quark_deferred_input))
    g_warning ("bse_source_do_shutdown(): Uhh ohh, source still contains deferred_input data");

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
bse_source_get_ichannel_id (BseSource   *source,
			    const gchar *ichannel_name)
{
  guint i;

  g_return_val_if_fail (BSE_IS_SOURCE (source), 0);
  g_return_val_if_fail (ichannel_name != NULL, 0);

  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    {
      BseSourceIChannelDef *ic_def = BSE_SOURCE_ICHANNEL_DEF (source, i + 1);

      if (strcmp (ic_def->name, ichannel_name) == 0)
	return i + 1;
    }

  return 0;
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

guint
bse_source_get_ochannel_id (BseSource   *source,
			    const gchar *ochannel_name)
{
  guint i;

  g_return_val_if_fail (BSE_IS_SOURCE (source), 0);
  g_return_val_if_fail (ochannel_name != NULL, 0);

  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    {
      BseSourceOChannelDef *oc_def = BSE_SOURCE_OCHANNEL_DEF (source, i + 1);

      if (strcmp (oc_def->name, ochannel_name) == 0)
	return i + 1;
    }

  return 0;
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
	  oc->in_calc = TRUE;
	  oc->chunks[oc->ring_offset] = BSE_SOURCE_GET_CLASS (source)->calc_chunk (source,
										   ochannel_id);
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
bse_source_calc_history (BseSource *source,
			 guint      ochannel_id)
{
  BseSourceOChannel *oc = BSE_SOURCE_OCHANNEL (source, ochannel_id);
  guint history;
  BseChunk **chunks;
  GSList *slist;
  guint i, n;
  
  g_return_if_fail (oc->in_calc == FALSE); /* paranoid */

  history = 0;	/* FIXME */
  history = 1;
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

static void
bse_source_do_prepare (BseSource *source,
		       BseIndex   index)
{
  guint i;

  source->start = index;
  source->index = index;
  
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    {
      BseSourceOChannel *oc = BSE_SOURCE_OCHANNEL (source, i + 1);
      BseSourceOChannelDef *ocd = BSE_SOURCE_OCHANNEL_DEF (source, i + 1);

      bse_source_calc_history (source, i + 1);
      
      if (oc->history && !oc->chunks[oc->ring_offset])
	oc->chunks[oc->ring_offset] = bse_chunk_new_static_zero (ocd->n_tracks);
    }

  bse_heart_attach (source);

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
bse_source_do_cycle (BseSource *source)
{
  BseChunk* (*calc_chunk) (BseSource *source,
			   guint      ochannel_id) = BSE_SOURCE_GET_CLASS (source)->calc_chunk;
  BseChunk* (*skip_chunk) (BseSource *source,
			   guint      ochannel_id) = BSE_SOURCE_GET_CLASS (source)->skip_chunk;
  guint i;
  
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    {
      BseSourceOChannel *oc = BSE_SOURCE_OCHANNEL (source, i + 1);
      
      if (!oc->history)
	continue;

      if (!oc->chunks[oc->ring_offset])
	{
          oc->in_calc = TRUE;
	  oc->chunks[oc->ring_offset] = skip_chunk (source, i + 1);
	  oc->in_calc = FALSE;
	}
      if (oc->chunks[oc->ring_offset])
	{
	  bse_chunk_unref (oc->chunks[oc->ring_offset]);
	  oc->chunks[oc->ring_offset] = NULL;
	}
    }
  
  source->index++;

#if 0
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
#endif
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
  
  bse_heart_detach (source);

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
	    bse_source_reset (input);
	  else
	    g_warning ("source's `%s' input `%s' ought to be prepared", /* paranoid */
		       BSE_OBJECT_TYPE_NAME (source),
		       BSE_OBJECT_TYPE_NAME (input));
	}
    }
}

static BseChunk*
bse_source_default_calc_chunk (BseSource *source,
			       guint      ochannel_id)
{
  return bse_chunk_new_static_zero (BSE_SOURCE_OCHANNEL_DEF (source, ochannel_id)->n_tracks);
}

static BseChunk*
bse_source_default_skip_chunk (BseSource *source,
			       guint      ochannel_id)
{
  return BSE_SOURCE_GET_CLASS (source)->calc_chunk (source, ochannel_id);
}

void
bse_source_reset (BseSource *source)
{
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_PREPARED (source));
  g_return_if_fail (!BSE_OBJECT_DESTROYED (source));
  
  bse_object_ref (BSE_OBJECT (source));
  BSE_SOURCE_GET_CLASS (source)->reset (source);
  BSE_OBJECT_UNSET_FLAGS (source, BSE_SOURCE_FLAG_PREPARED);
  bse_object_unref (BSE_OBJECT (source));
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
  g_return_val_if_fail (BSE_IS_SOURCE (input), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_OBJECT_DESTROYED (source), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_OBJECT_DESTROYED (input), BSE_ERROR_INTERNAL);
  
  if (ichannel_id < 1 || ichannel_id > BSE_SOURCE_N_ICHANNELS (source))
    return BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL;
  if (ochannel_id < 1 || ochannel_id > BSE_SOURCE_N_OCHANNELS (input))
    return BSE_ERROR_SOURCE_NO_SUCH_OCHANNEL;

  ic_def = BSE_SOURCE_ICHANNEL_DEF (source, ichannel_id);
  oc_def = BSE_SOURCE_OCHANNEL_DEF (input, ochannel_id);
  
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
      bse_source_reset (osource);
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

static void
bse_source_do_store_private (BseObject	*object,
			     BseStorage *storage)
{
  BseSource *source = BSE_SOURCE (object);
  BseProject *project = bse_item_get_project (BSE_ITEM (source));
  guint i;
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);

  for (i = 0; i < source->n_inputs; i++)
    {
      BseSourceInput *input = source->inputs + i;
      gchar *path = bse_container_make_item_path (BSE_CONTAINER (project),
						  BSE_ITEM (input->osource),
						  FALSE);

      bse_storage_break (storage);
      
      bse_storage_printf (storage,
			  "(source-input \"%s\" ",
			  BSE_SOURCE_ICHANNEL_DEF (source, input->ichannel_id)->name);
      bse_storage_push_level (storage);
      bse_storage_printf (storage, "%s \"%s\"",
			  path,
			  BSE_SOURCE_OCHANNEL_DEF (input->osource, input->ochannel_id)->name);
      bse_storage_pop_level (storage);
      bse_storage_handle_break (storage);
      bse_storage_putc (storage, ')');
      g_free (path);
    }
}

typedef struct _DeferredInput DeferredInput;
struct _DeferredInput
{
  DeferredInput *next;
  guint          ichannel_id;
  gchar         *osource_path;
  gchar         *ochannel_name;
};

static void
deferred_input_free (gpointer data)
{
  do
    {
      DeferredInput *dinput = data;

      data = dinput->next;
      g_free (dinput->osource_path);
      g_free (dinput->ochannel_name);
      g_free (dinput);
    }
  while (data);
}

static void
resolve_dinput (BseSource  *source,
		BseStorage *storage,
		gboolean    aborted,
		BseProject *project)
{
  BseObject *object = BSE_OBJECT (source);
  DeferredInput *dinput = bse_object_get_qdata (object, quark_deferred_input);

  bse_object_remove_notifiers_by_func (BSE_OBJECT (project),
				       resolve_dinput,
				       source);

  for (; dinput; dinput = dinput->next)
    {
      BseItem *item;
      BseErrorType error;

      item = bse_container_item_from_path (BSE_CONTAINER (project), dinput->osource_path);
      if (!item || !BSE_IS_SOURCE (item))
	{
	  if (!aborted)
	    bse_storage_warn (storage,
			      "%s: unable to determine input source from \"%s\"",
			      BSE_OBJECT_NAME (source),
			      dinput->osource_path);
	  continue;
	}
      error = bse_source_set_input (source,
				    dinput->ichannel_id,
				    BSE_SOURCE (item),
				    bse_source_get_ochannel_id (BSE_SOURCE (item),
								dinput->ochannel_name));
      if (error && !aborted)
	bse_storage_warn (storage,
			  "%s: failed to create input link from \"%s\": %s",
			  BSE_OBJECT_NAME (source),
			  dinput->osource_path,
			  bse_error_blurb (error));
    }

  bse_object_set_qdata (object, quark_deferred_input, NULL);
}

static BseTokenType
bse_source_do_restore_private (BseObject  *object,
			       BseStorage *storage)
{
  BseSource *source = BSE_SOURCE (object);
  GScanner *scanner = storage->scanner;
  GTokenType expected_token = BSE_TOKEN_UNMATCHED;
  DeferredInput *dinput;
  BseProject *project;
  guint ichannel_id;
  gchar *osource_path;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore_private)
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage);

  /* feature source-input keywords */
  if (expected_token != BSE_TOKEN_UNMATCHED ||
      g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER ||
      !bse_string_equals ("source-input", scanner->next_value.v_identifier))
    return expected_token;

  g_scanner_get_next_token (scanner); /* eat "source-input" */
  project = bse_item_get_project (BSE_ITEM (source));

  /* parse ichannel name */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    return G_TOKEN_STRING;
  ichannel_id = bse_source_get_ichannel_id (source, scanner->value.v_string);

  /* parse osource path */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return G_TOKEN_IDENTIFIER;
  osource_path = g_strdup (scanner->value.v_identifier);

  /* parse ochannel name */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    {
      g_free (osource_path);

      return G_TOKEN_STRING;
    }

  /* ok, we got the link information, save it away into our list
   * and make sure we get notified from our project when to
   * complete restoration
   */
  if (!quark_deferred_input)
    quark_deferred_input = g_quark_from_static_string ("BseSourceDeferredInput");
  dinput = g_new (DeferredInput, 1);
  dinput->next = bse_object_get_qdata (object, quark_deferred_input);
  dinput->ichannel_id = ichannel_id;
  dinput->osource_path = osource_path;
  dinput->ochannel_name = g_strdup (scanner->value.v_string);
  if (dinput->next)
    {
      bse_object_kill_qdata_no_notify (object, quark_deferred_input);
      bse_object_set_qdata_full (object, quark_deferred_input, dinput, deferred_input_free);
    }
  else
    {
      bse_object_set_qdata_full (object, quark_deferred_input, dinput, deferred_input_free);
      bse_object_add_data_notifier (BSE_OBJECT (project),
				    "complete_restore",
				    resolve_dinput,
				    source);
    }

  /* read closing brace */
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
}
