/* BseSequencer - BSE Sequencer
 * Copyright (C) 1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsesequencer.h"

#include <bse/gslengine.h>
#include <bse/gslieee754.h>


enum {
  PARAM_0,
  PARAM_NOTES,
  PARAM_LENGTH,
  PARAM_TRANSPOSE,
  PARAM_COUNTER
};


/* --- prototypes --- */
static void	bse_sequencer_init		(BseSequencer		*sequencer);
static void	bse_sequencer_class_init	(BseSequencerClass	*class);
static void	bse_sequencer_finalize		(GObject		*object);
static void	bse_sequencer_set_property	(BseSequencer           *sequencer,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec);
static void	bse_sequencer_get_property	(BseSequencer           *sequencer,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec);
static void	bse_sequencer_prepare		(BseSource		*source);
static void	bse_sequencer_context_create	(BseSource		*source,
						 guint			 context_handle,
						 GslTrans		*trans);
static void	bse_sequencer_reset		(BseSource		*source);
static void	bse_sequencer_update_modules	(BseSequencer		*seq);


/* --- Export to BSE --- */
#include "./icons/sequencer.c"
BSE_REGISTER_OBJECT (BseSequencer, BseSource, "/Modules/Other Sources/Sequencer", sequencer_icon,
                     "The Sequencer produces a frequency signal according to a sequence of notes",
                     bse_sequencer_class_init, NULL, bse_sequencer_init);
BSE_DEFINE_EXPORTS (BSE_PLUGIN_NAME);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
static void
bse_sequencer_class_init (BseSequencerClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_sequencer_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_sequencer_get_property;
  gobject_class->finalize = bse_sequencer_finalize;
  
  source_class->prepare = bse_sequencer_prepare;
  source_class->context_create = bse_sequencer_context_create;
  source_class->reset = bse_sequencer_reset;
  
  bse_object_class_add_param (object_class, "Sequence",
			      PARAM_LENGTH,
			      sfi_pspec_int ("length", "Length", NULL,
					     8, 1, 128, 4,
					     SFI_PARAM_GUI ":scale"));
  bse_object_class_add_param (object_class, "Sequence",
			      PARAM_NOTES,
			      bse_param_spec_boxed ("notes", "Notes", NULL,
						    BSE_TYPE_NOTE_SEQUENCE,
						    "note-sequence:" SFI_PARAM_STANDARD));
  bse_object_class_add_param (object_class, "Sequence",
			      PARAM_TRANSPOSE,
			      sfi_pspec_int ("transpose", "Transpose", NULL,
					     0, -36, +36, 3,
					     SFI_PARAM_STANDARD ":scale:skip-default"));
  bse_object_class_add_param (object_class, "Sequence",
			      PARAM_COUNTER,
			      sfi_pspec_real ("counter", "Timing [ms]", NULL,
					      100, 0, 1000, 5,
					      SFI_PARAM_STANDARD ":f:scale"));
  
  ochannel = bse_source_class_add_ochannel (source_class, "Freq Out", "Frequency Signal");
  g_assert (ochannel == BSE_SEQUENCER_OCHANNEL_FREQ);
  ochannel = bse_source_class_add_ochannel (source_class, "Note Sync", "Note Sync Signal");
  g_assert (ochannel == BSE_SEQUENCER_OCHANNEL_NOTE_SYNC);
}

static void
bse_sequencer_init (BseSequencer *seq)
{
  seq->sdata = bse_note_sequence_new ();
  bse_note_sequence_resize (seq->sdata, 8);
  seq->sdata->offset = SFI_NOTE_C (SFI_KAMMER_OCTAVE);
  seq->n_freq_values = 0;
  seq->freq_values = NULL;
  seq->transpose = 0;
}

static void
bse_sequencer_finalize (GObject *object)
{
  BseSequencer *seq = BSE_SEQUENCER (object);
  
  bse_note_sequence_free (seq->sdata);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_sequencer_set_property (BseSequencer *seq,
			    guint         param_id,
			    GValue       *value,
			    GParamSpec   *pspec)
{
  switch (param_id)
    {
      BseNoteSequence *sdata;
    case PARAM_LENGTH:
      if (sfi_value_get_int (value) != bse_note_sequence_length (seq->sdata))
	{
	  bse_note_sequence_resize (seq->sdata, sfi_value_get_int (value));
	  bse_sequencer_update_modules (seq);
	  g_object_notify (seq, "notes");
	}
      break;
    case PARAM_NOTES:
      bse_note_sequence_free (seq->sdata);
      sdata = bse_value_get_boxed (value);
      if (sdata)
	{
	  guint i, l, mnote = SFI_MAX_NOTE;
	  seq->sdata = bse_note_sequence_copy_shallow (sdata);
	  /* fixup offset */
	  l = bse_note_sequence_length (seq->sdata);
	  for (i = 0; i < l; i++)
	    mnote = MIN (mnote, seq->sdata->notes->notes[i]);
	  if (l && ABS (mnote - seq->sdata->offset) >= 12)
	    seq->sdata->offset = (mnote < SFI_NOTE_A (SFI_NOTE_OCTAVE (mnote)) ?
				  SFI_NOTE_C (SFI_NOTE_OCTAVE (mnote)) :
				  SFI_NOTE_A (SFI_NOTE_OCTAVE (mnote)));
	}
      else
	{
	  seq->sdata = bse_note_sequence_new ();
	  bse_note_sequence_resize (seq->sdata, 8);
	  seq->sdata->offset = SFI_NOTE_C (SFI_KAMMER_OCTAVE);
	}
      bse_sequencer_update_modules (seq);
      g_object_notify (seq, "length");
      break;
    case PARAM_COUNTER:
      seq->counter = sfi_value_get_real (value);
      bse_sequencer_update_modules (seq);
      break;
    case PARAM_TRANSPOSE:
      seq->transpose = sfi_value_get_int (value);
      bse_sequencer_update_modules (seq);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (seq, param_id, pspec);
      break;
    }
}

static void
bse_sequencer_get_property (BseSequencer *seq,
			    guint         param_id,
			    GValue       *value,
			    GParamSpec   *pspec)
{
  switch (param_id)
    {
    case PARAM_NOTES:
      bse_value_set_boxed (value, seq->sdata);
      break;
    case PARAM_LENGTH:
      sfi_value_set_int (value, bse_note_sequence_length (seq->sdata));
      break;
    case PARAM_COUNTER:
      sfi_value_set_real (value, seq->counter);
      break;
    case PARAM_TRANSPOSE:
      sfi_value_set_int (value, seq->transpose);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (seq, param_id, pspec);
      break;
    }
}

static gfloat*
freq_values_from_seq (BseNoteSequence *sdata,
		      gint             transpose)
{
  gfloat *v = g_new (gfloat, bse_note_sequence_length (sdata));
  guint i;
  
  for (i = 0; i < bse_note_sequence_length (sdata); i++)
    {
      gint note = sdata->notes->notes[i];
      
      if (note == SFI_NOTE_VOID)
	v[i] = 0;
      else
	v[i] = BSE_VALUE_FROM_FREQ (bse_note_to_freq (CLAMP (note + transpose, SFI_MIN_NOTE, SFI_MAX_NOTE)));
    }
  
  return v;
}

typedef struct {
  guint	       n_values;
  gfloat      *values;	/* notes */
  guint	       counter;
  guint        index;	/* nth_note */
  guint	       c;	/* timing */
} SeqModule;
typedef struct {
  guint   n_values;
  gfloat *new_values;
  guint   counter;
  gfloat *old_values;
} AccessData;

static void
seq_access (GslModule *module,
	    gpointer   data)
{
  SeqModule *smod = module->user_data;
  AccessData *d = data;
  
  smod->n_values = d->n_values;
  smod->values = d->new_values;
  smod->counter = d->counter;
  smod->index %= smod->n_values;
  smod->c %= smod->counter;
  if (smod->c < 1)
    smod->c = smod->counter;
}

static void
seq_access_free (gpointer data)
{
  AccessData *d = data;
  
  g_free (d->old_values);
  g_free (d);
}

static void
bse_sequencer_update_modules (BseSequencer *seq)
{
  if (BSE_SOURCE_PREPARED (seq))
    {
      AccessData *d = g_new (AccessData, 1);
      
      d->old_values = seq->freq_values;
      
      seq->n_freq_values = bse_note_sequence_length (seq->sdata);
      seq->freq_values = freq_values_from_seq (seq->sdata, seq->transpose);
      
      d->n_values = seq->n_freq_values;
      d->new_values = seq->freq_values;
      d->counter = seq->counter / 1000.0 * gsl_engine_sample_freq ();
      d->counter = MAX (d->counter, 1);
      
      bse_source_access_modules (BSE_SOURCE (seq),
				 seq_access, d, seq_access_free,
				 NULL);
    }
}

static void
sequencer_process (GslModule *module,
		   guint      n_values)
{
  SeqModule *smod = module->user_data;
  gfloat *freq_out = GSL_MODULE_OBUFFER (module, BSE_SEQUENCER_OCHANNEL_FREQ);
  gfloat *nsync_out = GSL_MODULE_OBUFFER (module, BSE_SEQUENCER_OCHANNEL_NOTE_SYNC);
  gfloat *bound = freq_out + n_values;
  
  while (freq_out < bound)
    {
      gfloat nval = smod->values[smod->index];
      
      if (smod->c == 0)
	{
	  smod->c = smod->counter;
	  smod->index++;
	  if (smod->index >= smod->n_values)
	    smod->index = 0;
	  *nsync_out = 1.0;
	}
      else
	*nsync_out = 0.0;
      *freq_out++ = nval;
      nsync_out++;
      smod->c--;
    }
}

static void
bse_sequencer_prepare (BseSource *source)
{
  BseSequencer *seq = BSE_SEQUENCER (source);
  
  seq->n_freq_values = bse_note_sequence_length (seq->sdata);
  seq->freq_values = freq_values_from_seq (seq->sdata, seq->transpose);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
bse_sequencer_context_create (BseSource *source,
			      guint      context_handle,
			      GslTrans  *trans)
{
  static const GslClass sequencer_class = {
    0,				/* n_istreams */
    0,                  	/* n_jstreams */
    BSE_SEQUENCER_N_OCHANNELS,	/* n_ostreams */
    sequencer_process,		/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    (gpointer) g_free,		/* free */
    GSL_COST_CHEAP,		/* flags */
  };
  BseSequencer *seq = BSE_SEQUENCER (source);
  SeqModule *smod = g_new0 (SeqModule, 1);
  GslModule *module;
  
  smod->n_values = seq->n_freq_values;
  smod->values = seq->freq_values;
  smod->counter = seq->counter / 1000.0 * gsl_engine_sample_freq ();
  smod->counter = MAX (smod->counter, 1);
  smod->index = 0;
  smod->c = smod->counter;
  
  module = gsl_module_new (&sequencer_class, smod);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_sequencer_reset (BseSource *source)
{
  BseSequencer *seq = BSE_SEQUENCER (source);
  
  g_free (seq->freq_values);
  seq->freq_values = NULL;
  seq->n_freq_values = 0;
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
