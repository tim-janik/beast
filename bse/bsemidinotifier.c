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
#include "bsemidinotifier.h"

#include "bsemarshal.h"
#include "bsemain.h"
#include "gslcommon.h"


/* --- prototypes --- */
static void	   bse_midi_notifier_class_init		(BseMidiNotifierClass *class);
static void	   bse_midi_notifier_init		(BseMidiNotifier      *self);
static void	   bse_midi_notifier_finalize		(GObject	      *object);


/* --- variables --- */
static gpointer parent_class = NULL;
static guint    signal_midi_event = 0;
static GQuark   number_quarks[BSE_MIDI_MAX_CHANNELS] = { 0, };


/* --- functions --- */
BSE_BUILTIN_TYPE (BseMidiNotifier)
{
  static const GTypeInfo midi_notifier_info = {
    sizeof (BseMidiNotifierClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_notifier_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseMidiNotifier),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_midi_notifier_init,
  };
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseMidiNotifier",
				   "MIDI Event Notifier",
				   &midi_notifier_info);
}

static void
bse_midi_notifier_class_init (BseMidiNotifierClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  guint i;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->finalize = bse_midi_notifier_finalize;
  
  for (i = 0; i < BSE_MIDI_MAX_CHANNELS; i++)
    {
      gchar buffer[32];
      
      g_snprintf (buffer, 32, "%u", i);
      number_quarks[i] = g_quark_from_string (buffer);
    }
  
  signal_midi_event = bse_object_class_add_dsignal (object_class, "midi-event",
						    bse_marshal_VOID__BOXED, NULL,
						    G_TYPE_NONE, 1,
						    BSE_TYPE_MIDI_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
bse_midi_notifier_init (BseMidiNotifier *self)
{
}

static void
bse_midi_notifier_finalize (GObject *object)
{
  /* BseMidiNotifier *self = BSE_MIDI_NOTIFIER (object); */
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
bse_midi_notifier_dispatch (BseMidiNotifier *self,
			    BseMidiReceiver *midi_receiver)
{
  GslRing *ring;
  gboolean need_emission;

  g_return_if_fail (BSE_IS_MIDI_NOTIFIER (self));
  g_return_if_fail (midi_receiver != NULL);

  need_emission = 0 != g_signal_handler_find (self, G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_UNBLOCKED,
					      signal_midi_event, 0, NULL, NULL, NULL);
  ring = bse_midi_receiver_fetch_notify_events (midi_receiver);
  while (ring)
    {
      BseMidiEvent *event = gsl_ring_pop_head (&ring);

      if (event->channel < BSE_MIDI_MAX_CHANNELS && need_emission)
	g_signal_emit (self, signal_midi_event, number_quarks[event->channel], event);
      bse_midi_free_event (event);
    }
}
