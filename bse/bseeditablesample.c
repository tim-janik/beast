/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
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
#include "bseeditablesample.h"

#include "bsemain.h"
#include "bsemarshal.h"
#include "bsestorage.h"
#include "gsldatahandle.h"


/* --- structures --- */
typedef struct _Notify Notify;
struct _Notify
{
  Notify            *next;
  BseEditableSample *esample;
};


/* --- prototypes --- */
static void	    bse_editable_sample_init		(BseEditableSample	*sample);
static void	    bse_editable_sample_class_init	(BseEditableSampleClass	*class);
static void	    bse_editable_sample_destroy		(BseObject		*object);
static void	    bse_editable_sample_finalize	(GObject		*object);


/* --- variables --- */
static gpointer parent_class = NULL;
static guint	signal_changed = 0;
static Notify  *changed_notify_list = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseEditableSample)
{
  static const GTypeInfo editable_sample_info = {
    sizeof (BseEditableSampleClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_editable_sample_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_editable */,
    
    sizeof (BseEditableSample),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_editable_sample_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseEditableSample",
				   "Editable sample type",
				   &editable_sample_info);
}

static void
bse_editable_sample_class_init (BseEditableSampleClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->finalize = bse_editable_sample_finalize;
  
  object_class->destroy = bse_editable_sample_destroy;
  
  signal_changed = bse_object_class_add_signal (object_class, "changed",
						bse_marshal_VOID__NONE,
						G_TYPE_NONE, 0);
}

static void
bse_editable_sample_init (BseEditableSample *sample)
{
  sample->in_destroy = FALSE;
  sample->wchunk = NULL;
  BSE_OBJECT_SET_FLAGS (sample, BSE_ITEM_FLAG_STORAGE_IGNORE);
}

static void
bse_editable_sample_destroy (BseObject *object)
{
  BseEditableSample *sample = BSE_EDITABLE_SAMPLE (object);

  sample->in_destroy = TRUE;

  if (sample->wchunk)
    {
      _gsl_wave_chunk_destroy (sample->wchunk);
      sample->wchunk = NULL;
    }

  /* chain parent class' handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_editable_sample_finalize (GObject *object)
{
  BseEditableSample *esample = BSE_EDITABLE_SAMPLE (object);
  Notify *notify, *last = NULL;

  for (notify = changed_notify_list; notify; )
    {
      if (notify->esample == esample)
	{
	  Notify *tmp;

	  if (last)
	    last->next = notify->next;
	  else
	    changed_notify_list = notify->next;
	  tmp = notify;
	  notify = notify->next;
	  g_free (tmp);
	}
      else
	{
	  last = notify;
	  notify = last->next;
	}
    }

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);

  g_return_if_fail (esample->wchunk == NULL);
}

static gboolean
changed_notify_handler (gpointer editable)
{
  BSE_THREADS_ENTER ();

  while (changed_notify_list)
    {
      Notify *notify = changed_notify_list;

      changed_notify_list = notify->next;
      if (!notify->esample->in_destroy)
	g_signal_emit (notify->esample, signal_changed, 0);
      g_free (notify);
    }

  BSE_THREADS_LEAVE ();

  return FALSE;
}

static void
changed_notify_add (BseEditableSample *esample)
{
  Notify *notify;

  if (!changed_notify_list)
    g_idle_add_full (BSE_NOTIFY_PRIORITY, changed_notify_handler, NULL, NULL);
  for (notify = changed_notify_list; notify; notify = notify->next)
    if (notify->esample == esample)
      return;
  notify = g_new (Notify, 1);
  notify->esample = esample;
  notify->next = changed_notify_list;
  changed_notify_list = notify;
}

void
bse_editable_sample_set_wchunk (BseEditableSample *esample,
				GslWaveChunk      *wchunk)
{
  g_return_if_fail (BSE_IS_EDITABLE_SAMPLE (esample));

  if (esample->wchunk)
    _gsl_wave_chunk_destroy (esample->wchunk);
  esample->wchunk = wchunk ? gsl_wave_chunk_copy (wchunk) : NULL;
  changed_notify_add (esample);
}
