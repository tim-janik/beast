// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemidinotifier.hh"
#include "bsemain.hh"
#include "gslcommon.hh"
#include "bsecxxplugin.hh"
/* --- prototypes --- */
static void	   bse_midi_notifier_class_init		(BseMidiNotifierClass *klass);
static void	   bse_midi_notifier_init		(BseMidiNotifier      *self);
static void	   bse_midi_notifier_finalize		(GObject	      *object);
/* --- variables --- */
static gpointer parent_class = NULL;
static guint    signal_midi_event = 0;
static GQuark   number_quarks[BSE_MIDI_MAX_CHANNELS] = { 0, };
static SfiRing *midi_notifier_list = NULL;
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
                                   __FILE__, __LINE__,
                                   &midi_notifier_info);
}
static void
bse_midi_notifier_class_init (BseMidiNotifierClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  guint i;
  parent_class = g_type_class_peek_parent (klass);
  gobject_class->finalize = bse_midi_notifier_finalize;
  for (i = 0; i < BSE_MIDI_MAX_CHANNELS; i++)
    {
      gchar buffer[32];
      g_snprintf (buffer, 32, "%u", i);
      number_quarks[i] = g_quark_from_string (buffer);
    }
  signal_midi_event = bse_object_class_add_dsignal (object_class, "midi-event",
						    G_TYPE_NONE, 1,
						    BSE_TYPE_MIDI_CHANNEL_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}
static void
bse_midi_notifier_init (BseMidiNotifier *self)
{
  midi_notifier_list = sfi_ring_append (midi_notifier_list, self);
}
static void
bse_midi_notifier_finalize (GObject *object)
{
  BseMidiNotifier *self = BSE_MIDI_NOTIFIER (object);
  midi_notifier_list = sfi_ring_remove (midi_notifier_list, self);
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}
void
bse_midi_notifier_set_receiver (BseMidiNotifier *self,
                                BseMidiReceiver *midi_receiver)
{
  BseMidiReceiver *old = self->midi_receiver;
  self->midi_receiver = midi_receiver;
  if (self->midi_receiver)
    {
      bse_midi_receiver_ref (self->midi_receiver);
      bse_midi_receiver_set_notifier (self->midi_receiver, self);
    }
  if (old)
    bse_midi_receiver_unref (old);
}
static inline void
bse_midi_notifier_notify_event (BseMidiNotifier *self,
                                BseMidiEvent    *event)
{
  BseMidiChannelEvent cev = { BseMidiChannelEventType (0), };
  switch (event->status)
    {
      /* channel voice messages */
    case BSE_MIDI_NOTE_OFF:
      cev.event_type = BSE_MIDI_EVENT_NOTE_OFF;
      cev.frequency = event->data.note.frequency;
      cev.velocity = event->data.note.velocity;
      break;
    case BSE_MIDI_NOTE_ON:
      cev.event_type = BSE_MIDI_EVENT_NOTE_ON;
      cev.frequency = event->data.note.frequency;
      cev.velocity = event->data.note.velocity;
      break;
    case BSE_MIDI_KEY_PRESSURE:
      cev.event_type = BSE_MIDI_EVENT_KEY_PRESSURE;
      cev.frequency = event->data.note.frequency;
      cev.velocity = event->data.note.velocity;
      break;
    case BSE_MIDI_CONTROL_CHANGE:
      cev.event_type = BSE_MIDI_EVENT_CONTROL_CHANGE;
      cev.control = event->data.control.control;
      cev.value = event->data.control.value;
      break;
    case BSE_MIDI_PROGRAM_CHANGE:
      cev.event_type = BSE_MIDI_EVENT_PROGRAM_CHANGE;
      cev.program = event->data.program;
      break;
    case BSE_MIDI_CHANNEL_PRESSURE:
      cev.event_type = BSE_MIDI_EVENT_CHANNEL_PRESSURE;
      cev.intensity = event->data.intensity;
      break;
    case BSE_MIDI_PITCH_BEND:
      cev.event_type = BSE_MIDI_EVENT_PITCH_BEND;
      cev.pitch_bend = event->data.pitch_bend;
      break;
      /* system realtime messages */
    case BSE_MIDI_TIMING_CLOCK:
      cev.event_type = BSE_MIDI_EVENT_TIMING_CLOCK;
      /* no data */
      break;
    case BSE_MIDI_SONG_START:
      cev.event_type = BSE_MIDI_EVENT_SONG_START;
      /* no data */
      break;
    case BSE_MIDI_SONG_CONTINUE:
      cev.event_type = BSE_MIDI_EVENT_SONG_CONTINUE;
      /* no data */
      break;
    case BSE_MIDI_SONG_STOP:
      cev.event_type = BSE_MIDI_EVENT_SONG_STOP;
      /* no data */
      break;
    case BSE_MIDI_ACTIVE_SENSING:
      cev.event_type = BSE_MIDI_EVENT_ACTIVE_SENSING;
      /* no data */
      break;
    case BSE_MIDI_SYSTEM_RESET:
      cev.event_type = BSE_MIDI_EVENT_SYSTEM_RESET;
      /* no data */
      break;
      /* system common messages */
    case BSE_MIDI_SONG_POINTER:
      cev.event_type = BSE_MIDI_EVENT_SONG_POINTER;
      cev.song_pointer = event->data.song_pointer;
      break;
    case BSE_MIDI_SONG_SELECT:
      cev.event_type = BSE_MIDI_EVENT_SONG_SELECT;
      cev.song_number = event->data.song_number;
      break;
    case BSE_MIDI_TUNE:
      cev.event_type = BSE_MIDI_EVENT_TUNE;
      /* no data */
      break;
    case BSE_MIDI_END_EX:
    case BSE_MIDI_SYS_EX:
    default:
      break;
    }
  cev.channel = event->channel;
  cev.tick_stamp = event->delta_time;
  if (cev.event_type)
    g_signal_emit (self, signal_midi_event, number_quarks[event->channel], &cev);
}
void
bse_midi_notifier_dispatch (BseMidiNotifier *self)
{
  g_return_if_fail (BSE_IS_MIDI_NOTIFIER (self));
  if (!self->midi_receiver)
    return;
  SfiRing *ring = bse_midi_receiver_fetch_notify_events (self->midi_receiver);
  if (!ring)
    return;
  uint need_emission = g_signal_handler_find (self,
                                              G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_UNBLOCKED,
                                              signal_midi_event, 0, NULL, NULL, NULL);
  while (ring)
    {
      BseMidiEvent *event = (BseMidiEvent*) sfi_ring_pop_head (&ring);
      if (event->channel < BSE_MIDI_MAX_CHANNELS && need_emission)
        bse_midi_notifier_notify_event (self, event);
      bse_midi_free_event (event);
    }
}
static gboolean
midi_notifiers_need_dispatch (void)
{
  SfiRing *ring;
  for (ring = midi_notifier_list; ring; ring = sfi_ring_walk (ring, midi_notifier_list))
    {
      BseMidiNotifier *notifier = (BseMidiNotifier*) ring->data;
      if (notifier->midi_receiver && bse_midi_receiver_has_notify_events (notifier->midi_receiver))
        return TRUE;
    }
  return FALSE;
}
static gboolean
midi_notifiers_source_prepare (GSource *source,
                               gint    *timeout_p)
{
  BSE_THREADS_ENTER ();
  gboolean need_dispatch = midi_notifiers_need_dispatch();
  BSE_THREADS_LEAVE ();
  return need_dispatch;
}
static gboolean
midi_notifiers_source_check (GSource *source)
{
  BSE_THREADS_ENTER ();
  gboolean need_dispatch = midi_notifiers_need_dispatch();
  BSE_THREADS_LEAVE ();
  return need_dispatch;
}
static gboolean
midi_notifiers_source_dispatch (GSource    *source,
                                GSourceFunc callback,
                                gpointer    user_data)
{
  BSE_THREADS_ENTER ();
  SfiRing *ring = midi_notifier_list;
  while (ring)
    {
      BseMidiNotifier *notifier = (BseMidiNotifier*) ring->data;
      ring = sfi_ring_walk (ring, midi_notifier_list);
      bse_midi_notifier_dispatch (notifier);
    }
  BSE_THREADS_LEAVE ();
  return TRUE;
}
void
bse_midi_notifiers_attach_source (void)
{
  static GSourceFuncs midi_notifiers_source_funcs = {
    midi_notifiers_source_prepare,
    midi_notifiers_source_check,
    midi_notifiers_source_dispatch,
  };
  GSource *source = g_source_new (&midi_notifiers_source_funcs, sizeof (GSource));
  g_source_set_priority (source, BSE_PRIORITY_NORMAL);
  g_source_attach (source, bse_main_context);
}

void
bse_midi_notifiers_wakeup (void)
{
  bse_main_wakeup();
}
