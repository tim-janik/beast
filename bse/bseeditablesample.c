/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
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
#include "bseeditablesample.h"

#include "bsemain.h"
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
static void	    bse_editable_sample_init		(BseEditableSample	*self);
static void	    bse_editable_sample_class_init	(BseEditableSampleClass	*class);
static void	    bse_editable_sample_dispose		(GObject		*object);
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
  
  g_assert (BSE_EDITABLE_SAMPLE_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT);
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseEditableSample",
				   "Editable sample type",
                                   __FILE__, __LINE__,
                                   &editable_sample_info);
}

static void
bse_editable_sample_class_init (BseEditableSampleClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->dispose = bse_editable_sample_dispose;
  gobject_class->finalize = bse_editable_sample_finalize;
  
  signal_changed = bse_object_class_add_signal (object_class, "changed",
						G_TYPE_NONE, 0);
}

static void
bse_editable_sample_init (BseEditableSample *self)
{
  self->wchunk = NULL;
}

static void
bse_editable_sample_dispose (GObject *object)
{
  BseEditableSample *self = BSE_EDITABLE_SAMPLE (object);
  
  bse_editable_sample_set_wchunk (self, NULL);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_editable_sample_finalize (GObject *object)
{
  BseEditableSample *self = BSE_EDITABLE_SAMPLE (object);
  Notify *notify, *last = NULL;
  
  for (notify = changed_notify_list; notify; )
    {
      if (notify->esample == self)
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
  
  g_return_if_fail (self->wchunk == NULL);
}

static gboolean
changed_notify_handler (gpointer editable)
{
  BSE_THREADS_ENTER ();
  
  while (changed_notify_list)
    {
      Notify *notify = changed_notify_list;
      
      changed_notify_list = notify->next;
      if (!BSE_OBJECT_DISPOSING (notify->esample))
	g_signal_emit (notify->esample, signal_changed, 0);
      g_free (notify);
    }
  
  BSE_THREADS_LEAVE ();
  
  return FALSE;
}

static void
changed_notify_add (BseEditableSample *self)
{
  Notify *notify;
  
  if (!changed_notify_list)
    bse_idle_notify (changed_notify_handler, NULL);
  for (notify = changed_notify_list; notify; notify = notify->next)
    if (notify->esample == self)
      return;
  notify = g_new (Notify, 1);
  notify->esample = self;
  notify->next = changed_notify_list;
  changed_notify_list = notify;
}

void
bse_editable_sample_set_wchunk (BseEditableSample *self,
				GslWaveChunk      *wchunk)
{
  g_return_if_fail (BSE_IS_EDITABLE_SAMPLE (self));
  
  if (self->wchunk)
    {
      if (self->open_count)
	gsl_wave_chunk_close (self->wchunk);
      self->open_count = 0;
      gsl_wave_chunk_unref (self->wchunk);
    }
  self->wchunk = wchunk ? gsl_wave_chunk_ref (wchunk) : NULL;
  changed_notify_add (self);
}
