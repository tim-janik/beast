// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemidireceiver.hh"
#include "bsemain.hh"
#include "gslcommon.hh"
#include "bseengine.hh"
#include "bsemathsignal.hh"
#include "bsecxxutils.hh"
#include "bse/internal.hh"
#include <string.h>
#include <bse/gbsearcharray.hh>
#include <map>
#include <set>

namespace {
using namespace Bse;
using namespace std;

#define EDUMP(...)      Bse::debug ("midi-events", __VA_ARGS__)
#define VDUMP(...)      Bse::debug ("midi-voice", __VA_ARGS__)

/* --- variables --- */
static std::mutex global_midi_mutex;
#define	BSE_MIDI_RECEIVER_LOCK()        global_midi_mutex.lock()
#define	BSE_MIDI_RECEIVER_UNLOCK()      global_midi_mutex.unlock()

/********************************************************************************
 *
 * Busy/idle states of a voice input and correspondance with table entries:
 *      Events          QS      Communication           VS      Table
 *                      Idle                            Idle    0
 *                      |                               |       0
 * a) Voice activation at Note On:
 *      NoteOn          |                               |       0
 *                      Busy                            |       1 (+Voice)
 *                      |       -> Job:activate,ON      |       1
 *                      |                       ->      Busy    1
 *                      |                               |       1
 *                      Busy                            Busy    1
 *
 * b) Note synthesis finishes:
 *      Done=1          |                               |       1
 *                      |                               Idle    1
 *                      |       GC:enter_idle   <-      |       1
 *                      Idle    <-                      |       0 (-Voice)
 *                      |                               |       0
 *
 * c) External Note Off Event:
 *      NoteOff         |                               |       1
 *                      Idle                            |       1
 *                      |       -> Job:OFF              |       1
 *                      |                       ->      Idle    1
 *                      |       GC:enter_idle   <-      |       1
 *                      |       <-                      |       0 (-Voice)
 *                      |                               |       0
 *
 * Cases (b) and (c) can occour simultaneously, and sustain is handled
 * similarly to note off events.
 *
 * QS   - queued voice state (anticipated voice state)
 * VS   - voice state
 *
 * Note that voice switch modules are connected upon Job:activate, but will
 * *only* disconnect upon Done=1. Thus a Done=const(0) mesh can block voices
 * forever.
 *******************************************************************************/


/* --- midi controls --- */
struct ControlKey {
  guint                  midi_channel;
  Bse::MidiSignal      type;
  explicit
  ControlKey (guint             _mc,
              Bse::MidiSignal _tp) :
    midi_channel (_mc),
    type (_tp) {}
  bool
  operator< (const ControlKey &k) const
  {
    if (type == k.type)
      return midi_channel < k.midi_channel;
    return type < k.type;
  }
};
class ControlHandler final {
  ControlHandler (const ControlHandler&) = delete;
  ControlHandler& operator= (const ControlHandler&) = delete;
public:
  BseMidiControlHandler           handler_func = NULL;
  gpointer                        handler_data = NULL;
  gpointer                        user_data = NULL;
  BseFreeFunc                     user_free = NULL;
  std::vector<BseModule*>         modules;
  explicit
  ControlHandler (BseMidiControlHandler hfunc, void *hdata, float vmin = 0, float vmax = 0) :
    handler_func (hfunc), handler_data (hdata)
  {}
  ControlHandler (ControlHandler &&other)
  {
    std::swap (handler_func, other.handler_func);
    std::swap (handler_data, other.handler_data);
    std::swap (user_data, other.user_data);
    std::swap (user_free, other.user_free);
    std::swap (modules, other.modules);
  }
  void modify() {}
  bool
  operator== (const ControlHandler &that) const
  {
    return handler_func == that.handler_func && handler_data == that.handler_data;
  }
  bool
  operator< (const ControlHandler &that) const
  {
    return handler_func < that.handler_func || (handler_func == that.handler_func && handler_data < that.handler_data);
  }
  void
  add_module (BseModule *module)
  {
    modules.push_back (module);
  }
  void
  set_data (gpointer              extra_data,
            BseFreeFunc           extra_free)
  {
    if (user_free)
      bse_engine_add_user_callback (user_data, user_free);
    user_data = extra_data;
    user_free = extra_free;
  }
  void
  remove_module (BseModule *module)
  {
    for (std::vector<BseModule*>::iterator it = modules.begin(); it != modules.end(); it++)
      if (*it == module)
        {
          modules.erase (it);
          return;
        }
    Bse::warning ("%s: no such module: %p", G_STRLOC, module);
  }
  ~ControlHandler()
  {
    assert_return (modules.size() == 0);
    if (user_free)
      bse_engine_add_user_callback (user_data, user_free);
    user_free = NULL;
  }
};
class ControlValue final {
  ControlValue (const ControlValue&) = delete;
  ControlValue& operator= (const ControlValue&) = delete;
public:
  float                            value = 0;
  GSList                          *cmodules = NULL;
  typedef std::set<ControlHandler> HandlerList;
  HandlerList                      handlers;
  explicit
  ControlValue (gfloat v) :
    value (v)
  {}
  ControlValue (ControlValue &&other) :
    value (other.value), cmodules (NULL)
  {
    std::swap (cmodules, other.cmodules);
    std::swap (handlers, other.handlers);
  }
  ControlValue&
  operator= (ControlValue &&other)
  {
    if (this != &other)
      {
        std::swap (value, other.value);
        std::swap (cmodules, other.cmodules);
        std::swap (handlers, other.handlers);
      }
    return *this;
  }
  bool
  add_handler (BseMidiControlHandler handler_func,
               gpointer              handler_data,
               BseModule            *module)
  {
    HandlerList::iterator it = handlers.find (ControlHandler (handler_func, handler_data));
    if (it == handlers.end())
      {
        ControlHandler chandler (handler_func, handler_data);
        it = handlers.insert (std::move (chandler)).first;
      }
    ControlHandler *ch = const_cast<ControlHandler*> (&(*it));
    ch->add_module (module);
    return ch->user_data != NULL;
  }
  void
  set_handler_data (BseMidiControlHandler handler_func,
                    gpointer              handler_data,
                    gpointer              extra_data,
                    BseFreeFunc           extra_free)
  {
    HandlerList::iterator it = handlers.find (ControlHandler (handler_func, handler_data));
    if (it == handlers.end())
      {
        if (extra_free)
          bse_engine_add_user_callback (extra_data, extra_free);
      }
    else
      {
        ControlHandler *ch = const_cast<ControlHandler*> (&(*it));
        ch->set_data (extra_data, extra_free);
      }
  }
  void
  remove_handler (BseMidiControlHandler handler_func,
                  gpointer              handler_data,
                  BseModule            *module)
  {
    HandlerList::iterator it = handlers.find (ControlHandler (handler_func, handler_data));
    assert_return (it != handlers.end());
    ControlHandler *ch = const_cast<ControlHandler*> (&(*it));
    ch->remove_module (module);
    if (ch->modules.size() == 0)
      handlers.erase (it);
  }
  void
  notify_handlers (guint64           tick_stamp,
                   Bse::MidiSignal signal_type,
                   BseTrans         *trans)
  {
    for (HandlerList::iterator it = handlers.begin(); it != handlers.end(); it++)
      it->handler_func (it->handler_data, tick_stamp, signal_type, value,
                        it->modules.size(), &it->modules[0], it->user_data, trans);
  }
  ~ControlValue()
  {
    assert_return (cmodules == NULL);
  }
};

struct EventHandler
{
  guint               midi_channel;
  BseMidiEventHandler handler_func;
  gpointer            handler_data;
  BseModule          *module;

  EventHandler (guint               midi_channel,
                BseMidiEventHandler handler_func,
                gpointer            handler_data,
                BseModule          *module) :
    midi_channel (midi_channel),
    handler_func (handler_func),
    handler_data (handler_data),
    module (module)
  {
  }
  bool operator == (const EventHandler& other)
  {
    return (midi_channel == other.midi_channel &&
            handler_func == other.handler_func &&
            handler_data == other.handler_data &&
            module       == other.module);
  }
};

/* --- voice prototypes --- */
typedef struct VoiceSwitch          VoiceSwitch;
typedef struct VoiceInput           VoiceInput;
typedef std::map<float,VoiceInput*> VoiceInputTable;


/* --- midi channel --- */
struct MidiChannel {
  guint           midi_channel;
  guint           poly_enabled;
  VoiceInput     *vinput;
  guint           n_voices;
  VoiceSwitch   **voices;
  VoiceInputTable voice_input_table;
  std::vector<EventHandler> event_handlers;
  MidiChannel (guint mc) :
    midi_channel (mc),
    poly_enabled (0)
  {
    vinput = NULL;
    n_voices = 0;
    voices = NULL;
  }
  void
  enable_poly (void)
  {
    poly_enabled++;
  }
  void
  disable_poly (void)
  {
    if (poly_enabled)
      poly_enabled--;
  }
  void
  add_event_handler (const EventHandler& handler)
  {
    event_handlers.push_back (handler);
  }
  void
  remove_event_handler (const EventHandler& handler)
  {
    vector<EventHandler>::iterator hi = find (event_handlers.begin(), event_handlers.end(), handler);
    assert_return (hi != event_handlers.end());
    event_handlers.erase (hi);
  }
  bool
  call_event_handlers (BseMidiEvent *event,
                       BseTrans     *trans);
  ~MidiChannel()
  {
    if (vinput)
      Bse::warning ("destroying MIDI channel (%u) with active mono synth", midi_channel);
    for (guint j = 0; j < n_voices; j++)
      if (voices[j])
        Bse::warning ("destroying MIDI channel (%u) with active voices", midi_channel);
    g_free (voices);
  }
  void  start_note      (guint64         tick_stamp,
                         gfloat          freq,
                         gfloat          velocity,
                         BseTrans       *trans);
  void  adjust_note     (guint64          tick_stamp,
                         gfloat           freq,
                         BseMidiEventType etype,
                         gfloat           velocity,
                         gboolean         sustain_note,
                         BseTrans        *trans);
  void  kill_notes      (guint64          tick_stamp,
                         gboolean         sustained_only,
                         BseTrans        *trans);
  void  no_poly_voice   (bool             noteon,
                         const gchar     *event_name,
                         gfloat           freq);
  void  debug_notes     (guint64          tick_stamp,
                         BseTrans        *trans);
};
static inline int
midi_channel_compare (const guint        midi_channel,
                      const MidiChannel *c2)
{
  return midi_channel < c2->midi_channel ? -1 : midi_channel > c2->midi_channel;
}


/* --- midi receiver --- */
struct MidiReceiver
{
  typedef std::map<ControlKey,ControlValue>     Controls;
  typedef std::vector<MidiChannel*>             Channels;
  Controls         controls;
  uint	           n_cmodules;
  BseModule      **cmodules;            // control signals
  Channels         midi_channels;
  SfiRing         *events;              // contains BseMidiEvent*
  uint		   ref_count;
  BseMidiNotifier *notifier;
  SfiRing	  *notifier_events;
public:
  explicit MidiReceiver ()
  {
    n_cmodules = 0;
    cmodules = NULL;
    events = NULL;
    ref_count = 1;
    notifier = NULL;
    notifier_events = NULL;
  }
  ~MidiReceiver()
  {
    assert_return (ref_count == 0);
    for (Channels::iterator it = midi_channels.begin(); it != midi_channels.end(); it++)
      delete *it;
    while (events)
      {
        BseMidiEvent *event = (BseMidiEvent*) sfi_ring_pop_head (&events);
        bse_midi_free_event (event);
      }
    while (notifier_events)
      {
        BseMidiEvent *event = (BseMidiEvent*) sfi_ring_pop_head (&notifier_events);
        bse_midi_free_event (event);
      }
    if (notifier)
      g_object_unref (notifier);
    if (n_cmodules)
      Bse::warning ("destroying MIDI receiver (%p) with active control modules (%u)", this, n_cmodules);
    g_free (cmodules);
  }
  MidiChannel*
  peek_channel (guint midi_channel)
  {
    Channels::iterator iter =
      binary_lookup (midi_channels.begin(), midi_channels.end(), midi_channel_compare, midi_channel);
    return iter == midi_channels.end() ? NULL : *iter;
  }
  MidiChannel*
  get_channel (guint midi_channel)
  {
    std::pair<Channels::iterator,bool> result =
      binary_lookup_insertion_pos (midi_channels.begin(), midi_channels.end(), midi_channel_compare, midi_channel);
    if (!result.second)
      result.first = midi_channels.insert (result.first, new MidiChannel (midi_channel));
    return *result.first;
  }
private:
  ControlValue*
  get_control_value (guint             midi_channel,
                     Bse::MidiSignal type)
  {
    Controls::iterator it = controls.find (ControlKey (midi_channel, type));
    if (it == controls.end())
      {
        ControlKey ckey (midi_channel, type);
        ControlValue cvalue (bse_midi_signal_default (type));
        it = controls.emplace (ckey, std::move (cvalue)).first;
      }
    return &it->second;
  }
public:
  gfloat
  get_control (guint             midi_channel,
               Bse::MidiSignal type)
  {
    Controls::iterator it = controls.find (ControlKey (midi_channel, type));
    return it != controls.end() ? it->second.value : bse_midi_signal_default (type);
  }
  GSList*
  set_control (guint             midi_channel,
               guint64           tick_stamp,
               Bse::MidiSignal signal_type,
               gfloat            value,
               BseTrans         *trans)
  {
    ControlValue *cv = get_control_value (midi_channel, signal_type);
    if (cv->value != value)
      {
        cv->value = value;
        cv->notify_handlers (tick_stamp, signal_type, trans);
        return cv->cmodules;
      }
    else
      return NULL;
  }
  void
  add_control (guint             midi_channel,
               Bse::MidiSignal type,
               BseModule        *module)
  {
    ControlValue *cv = get_control_value (midi_channel, type);
    cv->cmodules = g_slist_prepend (cv->cmodules, module);
  }
  void
  remove_control (guint             midi_channel,
                  Bse::MidiSignal type,
                  BseModule        *module)
  {
    ControlValue *cv = get_control_value (midi_channel, type);
    cv->cmodules = g_slist_remove (cv->cmodules, module);
  }
  bool
  add_control_handler (guint                 midi_channel,
                       Bse::MidiSignal     signal_type,
                       BseMidiControlHandler handler_func,
                       gpointer              handler_data,
                       BseModule            *module)
  {
    ControlValue *cv = get_control_value (midi_channel, signal_type);
    return cv->add_handler (handler_func, handler_data, module);
  }
  void
  set_control_handler_data (guint                 midi_channel,
                            Bse::MidiSignal     signal_type,
                            BseMidiControlHandler handler_func,
                            gpointer              handler_data,
                            gpointer              extra_data,
                            BseFreeFunc           extra_free)
  {
    ControlValue *cv = get_control_value (midi_channel, signal_type);
    cv->set_handler_data (handler_func, handler_data, extra_data, extra_free);
  }
  void
  remove_control_handler (guint                 midi_channel,
                          Bse::MidiSignal     signal_type,
                          BseMidiControlHandler handler_func,
                          gpointer              handler_data,
                          BseModule            *module)
  {
    ControlValue *cv = get_control_value (midi_channel, signal_type);
    cv->remove_handler (handler_func, handler_data, module);
  }
  void
  add_event_handler (guint              midi_channel,
		     BseMidiEventHandler handler_func,
		     gpointer           handler_data,
		     BseModule         *module)
  {
    MidiChannel *channel = get_channel (midi_channel);
    channel->add_event_handler (EventHandler (midi_channel, handler_func, handler_data, module));
  }
  void
  remove_event_handler (guint              midi_channel,
			BseMidiEventHandler handler_func,
			gpointer           handler_data,
			BseModule         *module)
  {
    MidiChannel *channel = get_channel (midi_channel);
    channel->remove_event_handler (EventHandler (midi_channel, handler_func, handler_data, module));
  }
};


/* --- MIDI Control Module --- */
typedef struct
{
  guint             midi_channel;
  gfloat            values[BSE_MIDI_CONTROL_MODULE_N_CHANNELS];
  Bse::MidiSignal signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS];
  guint             ref_count;
} MidiCModuleData;

static void
midi_control_module_process_U (BseModule *module, /* vswitch->smodule */
                               guint      n_values)
{
  MidiCModuleData *cdata = (MidiCModuleData *) module->user_data;
  guint i;

  for (i = 0; i < BSE_MODULE_N_OSTREAMS (module); i++)
    if (module->ostreams[i].connected)
      module->ostreams[i].values = bse_engine_const_values (cdata->values[i]);
}

static BseModule*
create_midi_control_module_L (MidiReceiver      *self,
                              guint              midi_channel,
                              Bse::MidiSignal  signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS])
{
  static const BseModuleClass midi_cmodule_class = {
    0,                                  /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_CONTROL_MODULE_N_CHANNELS, /* n_ostreams */
    midi_control_module_process_U,      /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    (BseModuleFreeFunc) g_free,         /* free */
    Bse::ModuleFlag::CHEAP
  };
  MidiCModuleData *cdata;
  BseModule *module;
  guint i;

  assert_return (signals != NULL, NULL);

  cdata = g_new0 (MidiCModuleData, 1);
  cdata->midi_channel = midi_channel;
  for (i = 0; i < BSE_MIDI_CONTROL_MODULE_N_CHANNELS; i++)
    {
      cdata->signals[i] = signals[i];
      cdata->values[i] = self->get_control (midi_channel, cdata->signals[i]);
    }
  cdata->ref_count = 1;
  module = bse_module_new (&midi_cmodule_class, cdata);

  return module;
}

typedef struct {
  Bse::MidiSignal signal;
  gfloat            value;
} MidiCModuleAccessData;

static void
midi_control_module_access_U (BseModule *module,
                              gpointer   data)
{
  MidiCModuleData *cdata = (MidiCModuleData *) module->user_data;
  MidiCModuleAccessData *adata = (MidiCModuleAccessData *) data;
  guint i;

  for (i = 0; i < BSE_MIDI_CONTROL_MODULE_N_CHANNELS; i++)
    if (cdata->signals[i] == adata->signal)
      cdata->values[i] = adata->value;
}

static void
change_midi_control_modules_L (GSList           *modules,
                               guint64           tick_stamp,
                               Bse::MidiSignal signal,
                               gfloat            value,
                               BseTrans         *trans)
{
  MidiCModuleAccessData *adata;
  GSList *slist = modules;

  if (!modules)
    return;
  adata = g_new0 (MidiCModuleAccessData, 1);
  adata->signal = signal;
  adata->value = value;
  for (slist = modules; slist; slist = slist->next)
    bse_trans_add (trans, bse_job_flow_access ((BseModule *) slist->data,
					       tick_stamp,
					       midi_control_module_access_U,
					       adata,
					       slist->next ? NULL : g_free));
}

static gboolean
match_midi_control_module_L (BseModule         *cmodule,
                             guint              midi_channel,
                             Bse::MidiSignal  signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS])
{
  MidiCModuleData *cdata = (MidiCModuleData *) cmodule->user_data;
  gboolean match = TRUE;
  guint i;

  for (i = 0; i < BSE_MIDI_CONTROL_MODULE_N_CHANNELS; i++)
    match &= cdata->signals[i] == signals[i];
  match &= cdata->midi_channel == midi_channel;

  return match;
}


/* --- VoiceInput module --- */
typedef enum {
  VOICE_ON = 1,
  VOICE_PRESSURE,
  VOICE_SUSTAIN,
  VOICE_OFF,
  VOICE_KILL_SUSTAIN,
  VOICE_KILL
} VoiceChangeType;
static const char*
voice_change_to_string (VoiceChangeType t)
{
  switch (t)
    {
    case VOICE_ON:              return "voice-on";
    case VOICE_PRESSURE:        return "pressure";
    case VOICE_SUSTAIN:         return "sustain";
    case VOICE_OFF:             return "voice-off";
    case VOICE_KILL_SUSTAIN:    return "kill-sustain";
    case VOICE_KILL:            return "voice-kill";
    }
  return "<invalid>";
}

typedef enum {
  VSTATE_IDLE,
  VSTATE_BUSY,      /* got note-on, waiting for note-off */
  VSTATE_SUSTAINED, /* holding due to sustain, after note-off */
} VoiceState;
static const char*
voice_state_to_string (VoiceState s)
{
  switch (s)
    {
    case VSTATE_IDLE:           return "idle";
    case VSTATE_BUSY:           return "busy";
    case VSTATE_SUSTAINED:      return "sustained";
    }
  return "<invalid>";
}

struct VoiceInput
{
  /* module state */
  gfloat           freq_value;
  gfloat           gate;
  gfloat           velocity;
  gfloat           aftertouch;           /* mutable while within_note */
  VoiceState       vstate;
  /* mono synth */
  guint            ref_count;
  BseModule       *fmodule;	        /* freq module */
  guint64          tick_stamp;           /* time of last event change */
  VoiceState       queue_state;          /* vstate according to jobs queued so far */
  VoiceInputTable *table;
  VoiceInput      *next;
  VoiceInputTable::iterator iter;
};

static void
voice_input_module_process_U (BseModule *module,
                              guint      n_values)
{
  VoiceInput *vinput = (VoiceInput*) module->user_data;

  if (BSE_MODULE_OSTREAM (module, 0).connected)
    BSE_MODULE_OSTREAM (module, 0).values = bse_engine_const_values (vinput->freq_value);
  if (BSE_MODULE_OSTREAM (module, 1).connected)
    BSE_MODULE_OSTREAM (module, 1).values = bse_engine_const_values (vinput->gate);
  if (BSE_MODULE_OSTREAM (module, 2).connected)
    BSE_MODULE_OSTREAM (module, 2).values = bse_engine_const_values (vinput->velocity);
  if (BSE_MODULE_OSTREAM (module, 3).connected)
    BSE_MODULE_OSTREAM (module, 3).values = bse_engine_const_values (vinput->aftertouch);
}

typedef struct {
  VoiceChangeType vtype;
  gfloat          freq_value;
  gfloat          velocity;
} VoiceInputData;

static void
voice_input_module_reset_U (BseModule *module)
{
  VoiceInput *vinput = (VoiceInput*) module->user_data;
  vinput->freq_value = 0;
  vinput->gate = 0;
  vinput->velocity = 0.5;
  vinput->aftertouch = 0.5;
}

static void
voice_input_remove_from_table_L (VoiceInput *vinput)    /* UserThread */
{
  if (vinput->table && vinput->iter != vinput->table->end())
    {
      VoiceInputTable::iterator iter = vinput->iter;
      VoiceInput *last = NULL, *cur;
      for (cur = iter->second; cur; last = cur, cur = last->next)
        if (cur == vinput)
          {
            if (last)
              last->next = cur->next;
            else
              iter->second = cur->next;
            vinput->next = NULL;
            vinput->iter = vinput->table->end();
            vinput->queue_state = VSTATE_IDLE;
            return;
          }
      assert_return_unreached ();
    }
}

static void
voice_input_enter_sustain_U (gpointer data)     /* UserThread */
{
  VoiceInput *vinput = (VoiceInput*) data;
  BSE_MIDI_RECEIVER_LOCK ();
  voice_input_remove_from_table_L (vinput);
  vinput->queue_state = VSTATE_SUSTAINED;
  BSE_MIDI_RECEIVER_UNLOCK ();
}

static void
voice_input_enter_idle_U (gpointer data)        /* UserThread */
{
  VoiceInput *vinput = (VoiceInput*) data;
  BSE_MIDI_RECEIVER_LOCK ();
  voice_input_remove_from_table_L (vinput);
  vinput->queue_state = VSTATE_IDLE;
  BSE_MIDI_RECEIVER_UNLOCK ();
}
static void
voice_input_module_access_U (BseModule *module,
                             gpointer   data)
{
  VoiceInput *vinput = (VoiceInput*) module->user_data;
  VoiceInputData *mdata = (VoiceInputData *) data;
  VDUMP ("Synth<%p:%08llx>: ProcessEvent=%s Freq=%.2fHz",
         vinput, bse_module_tick_stamp (module),
         voice_change_to_string (mdata->vtype),
         BSE_FREQ_FROM_VALUE (mdata->freq_value));
  switch (mdata->vtype)
    {
    case VOICE_ON:
      if (vinput->vstate == VSTATE_BUSY && /*LOCK()*/ vinput->table /*UNLOCK()*/)
        Bse::warning ("%s: VOICE_ON: vinput->vstate == VSTATE_BUSY", G_STRLOC);
      vinput->vstate = VSTATE_BUSY;
      vinput->freq_value = mdata->freq_value;
      vinput->gate = 1.0;
      vinput->velocity = mdata->velocity;
      vinput->aftertouch = mdata->velocity;
      break;
    case VOICE_PRESSURE:
      if (vinput->vstate == VSTATE_BUSY &&
          BSE_SIGNAL_FREQ_EQUALS (vinput->freq_value, mdata->freq_value))
        vinput->aftertouch = mdata->velocity;
      break;
    case VOICE_SUSTAIN:
      if (vinput->vstate == VSTATE_BUSY &&
          BSE_SIGNAL_FREQ_EQUALS (vinput->freq_value, mdata->freq_value))
        {
          vinput->vstate = VSTATE_SUSTAINED;
          bse_engine_add_user_callback (vinput, voice_input_enter_sustain_U);
        }
      break;
    case VOICE_OFF:
      if (vinput->vstate == VSTATE_BUSY &&
          BSE_SIGNAL_FREQ_EQUALS (vinput->freq_value, mdata->freq_value))       /* monophonic synths get spurious note-off events */
        goto kill_voice;
      break;
    case VOICE_KILL_SUSTAIN:
      if (vinput->vstate == VSTATE_SUSTAINED)
        goto kill_voice;
      break;
    case VOICE_KILL:
    kill_voice:
      vinput->vstate = VSTATE_IDLE;
      vinput->gate = 0.0;
      bse_engine_add_user_callback (vinput, voice_input_enter_idle_U);
      break;
    }
}

static void
change_voice_input_L (VoiceInput      *vinput,
                      guint64          tick_stamp,
                      VoiceChangeType  vtype,
                      gfloat           freq_value,
                      gfloat           velocity,
                      BseTrans        *trans)
{
  switch (vtype)
    {
    case VOICE_ON:
      if (vinput->queue_state == VSTATE_BUSY && vinput->table)
        Bse::warning ("%s: VOICE_ON: vinput->queue_state == VSTATE_BUSY", G_STRLOC);
      if (vinput->table)
        {
          assert_return (vinput->iter == vinput->table->end());
          vinput->next = (*vinput->table)[freq_value];
          vinput->iter = vinput->table->find (freq_value);
          assert_return (vinput->iter != vinput->table->end());
          vinput->iter->second = vinput;
        }
      vinput->queue_state = VSTATE_BUSY;
      break;
    case VOICE_PRESSURE:
      if (vinput->table)
        assert_return (vinput->iter != vinput->table->end());
      break;
    case VOICE_SUSTAIN:
      if (vinput->table)
        assert_return (vinput->iter != vinput->table->end());
      vinput->queue_state = VSTATE_SUSTAINED;
      break;
    case VOICE_OFF:
      if (vinput->table)
        assert_return (vinput->iter != vinput->table->end());
      vinput->queue_state = VSTATE_IDLE;
      break;
    case VOICE_KILL_SUSTAIN:
      assert_return (vinput->queue_state == VSTATE_SUSTAINED);
      vinput->queue_state = VSTATE_IDLE;
      break;
    case VOICE_KILL:
      assert_return (vinput->queue_state != VSTATE_IDLE);
      vinput->queue_state = VSTATE_IDLE;
      break;
    }
  VoiceInputData mdata;
  mdata.vtype = vtype;
  mdata.freq_value = freq_value;
  mdata.velocity = velocity;
  bse_trans_add (trans, bse_job_flow_access (vinput->fmodule, tick_stamp, voice_input_module_access_U, g_memdup (&mdata, sizeof (mdata)), g_free));
  vinput->tick_stamp = tick_stamp;
  VDUMP ("Synth<%p:%08llx>: QueueEvent=%s Freq=%.2fHz%s",
         vinput, tick_stamp,
         voice_change_to_string (vtype),
         BSE_FREQ_FROM_VALUE (freq_value),
         Bse::TickStamp::current() >= tick_stamp ? " (late)" : "");
}
static void
voice_input_module_free_U (gpointer        data,
                           const BseModuleClass *klass)
{
  VoiceInput *vinput = (VoiceInput*) data;
  assert_return (vinput->next == NULL);
  delete vinput;
}

static VoiceInput*
create_voice_input_L (VoiceInputTable *table,
                      gboolean         ismono,
                      BseTrans        *trans)
{
  static const BseModuleClass mono_synth_module_class = {
    0,                                  /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_MODULE_N_CHANNELS,   /* n_ostreams */
    voice_input_module_process_U,       /* process */
    NULL,                               /* process_defer */
    voice_input_module_reset_U,         /* reset */
    voice_input_module_free_U,		/* free */
    Bse::ModuleFlag::CHEAP
  };
  VoiceInput *vinput = new VoiceInput;

  vinput->fmodule = bse_module_new (&mono_synth_module_class, vinput);
  vinput->freq_value = 0;
  vinput->gate = 0;
  vinput->velocity = 0.5;
  vinput->aftertouch = 0.5;
  vinput->vstate = VSTATE_IDLE;
  vinput->ref_count = 1;
  vinput->tick_stamp = 0;
  vinput->queue_state = VSTATE_IDLE;
  vinput->table = ismono ? NULL : table;
  vinput->next = NULL;
  vinput->iter = table->end();
  bse_trans_add (trans, bse_job_integrate (vinput->fmodule));

  return vinput;
}

static void
destroy_voice_input_L (VoiceInput      *vinput,
                       BseTrans        *trans)
{
  assert_return (vinput->ref_count == 0);

  if (vinput->table && vinput->iter != vinput->table->end())
    voice_input_remove_from_table_L (vinput);
  bse_trans_add (trans, bse_job_boundary_discard (vinput->fmodule));
}


/* --- VoiceSwitch module --- */
struct VoiceSwitch
{
  /* module state: */
  volatile gboolean disconnected;       /* a hint towards module currently being idle */
  /* switchable midi voice */
  guint             n_vinputs;
  VoiceInput      **vinputs;
  guint             ref_count;
  BseModule        *smodule;            /* input module (switches and suspends) */
  BseModule        *vmodule;            /* output module (virtual) */
};

static void
voice_switch_module_reuse_U (gpointer data)     /* UserThread */
{
  VoiceSwitch *vswitch = (VoiceSwitch*) data;
  BSE_MIDI_RECEIVER_LOCK ();
  vswitch->disconnected = TRUE;         /* reuse possible */
  BSE_MIDI_RECEIVER_UNLOCK ();
}

static void
voice_switch_module_process_U (BseModule *module,
                               guint      n_values)
{
  VoiceSwitch *vswitch = (VoiceSwitch*) module->user_data;
  guint i;

  /* dumb pass-through task */
  for (i = 0; i < BSE_MODULE_N_OSTREAMS (module); i++)
    if (BSE_MODULE_OSTREAM (module, i).connected)
      BSE_MODULE_OSTREAM (module, i).values = (gfloat*) BSE_MODULE_IBUFFER (module, i);

  /* check Done state on last stream */
  if (BSE_MODULE_IBUFFER (module, BSE_MODULE_N_ISTREAMS (module) - 1)[n_values - 1] >= 1.0)
    {
      BseTrans *trans = bse_trans_open ();
      /* disconnect all inputs */
      bse_trans_add (trans, bse_job_suspend_now (module));
      bse_trans_add (trans, bse_job_kill_inputs (vswitch->vmodule));
      bse_trans_commit (trans);
      for (i = 0; i < vswitch->n_vinputs; i++)
        if (vswitch->vinputs[i]->vstate == VSTATE_BUSY)
          {
            vswitch->vinputs[i]->vstate = VSTATE_IDLE;
            bse_engine_add_user_callback (vswitch->vinputs[i], voice_input_enter_idle_U);
          }
      bse_engine_add_user_callback (vswitch, voice_switch_module_reuse_U);
    }
}

static void
voice_switch_module_boundary_check_U (BseModule *module,
                                      gpointer   data)
{
  VoiceSwitch *vswitch = (VoiceSwitch*) module->user_data;
  if (vswitch->ref_count &&     /* don't queue jobs post-discard */
      !bse_module_has_source (vswitch->vmodule, 0))
    {
      BseTrans *trans = bse_trans_open ();
      guint i;
      for (i = 0; i < BSE_MODULE_N_ISTREAMS (vswitch->vmodule); i++)
        bse_trans_add (trans, bse_job_connect (vswitch->smodule, i, vswitch->vmodule, i));
      bse_trans_commit (trans);
      vswitch->disconnected = FALSE;      /* reset hint */
    }
}

static void
activate_voice_switch_L (VoiceSwitch *vswitch,
                         guint64      tick_stamp,
                         BseTrans    *trans)
{
  assert_return (vswitch->disconnected == TRUE);
  /* make sure the module is connected before tick_stamp */
  bse_trans_add (trans, bse_job_boundary_access (vswitch->smodule, tick_stamp, voice_switch_module_boundary_check_U, NULL, NULL));
  /* make sure the module is not suspended at tick_stamp */
  bse_trans_add (trans, bse_job_resume_at (vswitch->smodule, tick_stamp));
  vswitch->disconnected = FALSE;        /* reset hint ahead of time */
}

static void
voice_switch_module_free_U (gpointer        data,
                            const BseModuleClass *klass)
{
  VoiceSwitch *vswitch = (VoiceSwitch*) data;
  g_free (vswitch->vinputs);
  g_free (vswitch);
}

static VoiceSwitch*
create_voice_switch_module_L (BseTrans *trans)
{
  static const BseModuleClass switch_module_class = {
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_istreams */
    0,                                  /* n_jstreams */
    BSE_MIDI_VOICE_N_CHANNELS,          /* n_ostreams */
    voice_switch_module_process_U,      /* process */
    NULL,                               /* process_defer */
    NULL,                               /* reset */
    voice_switch_module_free_U,         /* free */
    Bse::ModuleFlag::CHEAP
  };
  VoiceSwitch *vswitch = g_new0 (VoiceSwitch, 1);

  vswitch->disconnected = TRUE;
  vswitch->ref_count = 1;
  vswitch->smodule = bse_module_new (&switch_module_class, vswitch);
  vswitch->vmodule = bse_module_new_virtual (BSE_MIDI_VOICE_N_CHANNELS, NULL, NULL);
  bse_trans_add (trans, bse_job_integrate (vswitch->smodule));
  bse_trans_add (trans, bse_job_integrate (vswitch->vmodule));
  bse_trans_add (trans, bse_job_suspend_now (vswitch->smodule));

  return vswitch;
}

static inline gboolean
check_voice_switch_available_L (VoiceSwitch *vswitch)
{
  return vswitch->disconnected;
}

static void
voice_switch_module_commit_accessor_U (BseModule *module,
                                       gpointer   data)
{
  BseTrans *trans = (BseTrans*) data;
  bse_trans_commit (trans);
}

static void
destroy_voice_switch_L (VoiceSwitch *vswitch,
                        BseTrans     *trans)
{
  BseTrans *tmp_trans;

  assert_return (vswitch->ref_count == 0);
  assert_return (vswitch->n_vinputs == 0);

  tmp_trans = bse_trans_open ();
  bse_trans_add (tmp_trans, bse_job_boundary_discard (vswitch->smodule));
  bse_trans_add (tmp_trans, bse_job_boundary_discard (vswitch->vmodule));
  /* we can't commit the transaction right away, because the switch
   * module might currently be processing and is about to queue
   * disconnection jobs on the modules we're just discarding.
   * so we use a normal accessor to defer destruction which makes
   * sure that pending disconnect jobs have been processed already.
   */
  bse_trans_add (trans, bse_job_access (vswitch->smodule, voice_switch_module_commit_accessor_U, tmp_trans, NULL));
}


/* --- MidiChannel --- */
static inline gboolean
check_voice_input_improvement_L (VoiceInput *vinput1, /* vinput1 better than vinput2? */
                                 VoiceInput *vinput2)
{
  if (vinput1->queue_state == vinput2->queue_state)
    return vinput1->tick_stamp < vinput2->tick_stamp;
  if (vinput1->queue_state == VSTATE_IDLE)
    return TRUE;
  if (vinput1->queue_state == VSTATE_SUSTAINED)
    return vinput2->queue_state == VSTATE_IDLE ? FALSE : TRUE;
  return FALSE; /* vinput1->queue_state == VSTATE_BUSY && vinput1->queue_state != vinput2->queue_state */
}

void
MidiChannel::no_poly_voice (bool noteon, const char *event_name, float freq)
{
  MidiChannel *mchannel = this;
  assert_return (mchannel->poly_enabled);

  if (!noteon)
    warning ("MidiChannel(%u): failed to find voice for '%s' (%fHz)",
             mchannel->midi_channel, event_name, freq);
  else
    {
      size_t scheduled = 0, disconnected = 0;
      for (size_t i = 0; i < mchannel->n_voices; i++)
        {
          if (bse_module_is_scheduled (mchannel->voices[i]->vmodule))
            scheduled++;
          if (check_voice_switch_available_L (mchannel->voices[i]))
            disconnected++;
        }
      if (noteon)
        warning ("MidiChannel(%u): failed to allocate voice for '%s' (%fHz), %d/%d voices disconnected (%d scheduled)",
                 mchannel->midi_channel, event_name, freq, disconnected, mchannel->n_voices, scheduled);
    }
}

bool
MidiChannel::call_event_handlers (BseMidiEvent *event,
                                  BseTrans     *trans)
{
  bool success = false;
  for (vector<EventHandler>::const_iterator hi = event_handlers.begin(); hi != event_handlers.end(); hi++)
    {
      int activated = 0;
      for (guint i = 0; i < n_voices; i++)
	{
	  if (voices[i] && voices[i]->n_vinputs)
	    {
	      if (check_voice_switch_available_L (voices[i]))
		{
		  activated++;
		  VoiceSwitch *vswitch = voices[i];
		  activate_voice_switch_L (vswitch, event->delta_time, trans);
		}
	    }
	}
      if (!(activated <= 1))
	Bse::warning (G_STRLOC ": midi event handling: assertion (activated <= 1) failed, activated = %d", activated);
      hi->handler_func (hi->handler_data, hi->module, event, trans);
      success = true;
    }
  return success;
}

void
MidiChannel::start_note (guint64         tick_stamp,
                         gfloat          freq,
                         gfloat          velocity,
                         BseTrans       *trans)
{
  MidiChannel *mchannel = this;
  gfloat freq_val = BSE_VALUE_FROM_FREQ (freq);
  VoiceSwitch *vswitch, *override_candidate = NULL;
  guint i;

  assert_return (freq > 0);

  /* adjust channel global mono synth */
  if (mchannel->vinput)
    change_voice_input_L (mchannel->vinput, tick_stamp, VOICE_ON, freq_val, velocity, trans);

  if (!mchannel->poly_enabled)
    return;

  /* figure voice from event */
  vswitch = NULL; // voice numbers on events not currently supported
  /* find free poly voice */
  if (!vswitch)
    for (i = 0; i < mchannel->n_voices; i++)
      if (mchannel->voices[i] && mchannel->voices[i]->n_vinputs)
        {
          override_candidate = mchannel->voices[i];
          if (check_voice_switch_available_L (mchannel->voices[i]))
            {
              vswitch = mchannel->voices[i];
              break;
            }
        }
  /* grab voice to override */
  if (!vswitch)
    (void) override_candidate; // FIXME: voice = override_candidate;

  if (vswitch && vswitch->n_vinputs)
    {
      /* start note */
      VoiceInput *vinput = vswitch->vinputs[0];
      /* figure mono synth */
      for (i = 1; i < vswitch->n_vinputs; i++)
        if (check_voice_input_improvement_L (vswitch->vinputs[i], vinput))
          vinput = vswitch->vinputs[i];
      /* setup voice */
      activate_voice_switch_L (vswitch, tick_stamp, trans);
      change_voice_input_L (vinput, tick_stamp, VOICE_ON, freq_val, velocity, trans);
    }
  else
    no_poly_voice (true, "note-on", freq);
}

void
MidiChannel::adjust_note (guint64         tick_stamp,
                          gfloat           freq,
                          BseMidiEventType etype, /* BSE_MIDI_KEY_PRESSURE or BSE_MIDI_NOTE_OFF */
                          gfloat           velocity,
                          gboolean         sustain_note, /* may be TRUE for BSE_MIDI_NOTE_OFF */
                          BseTrans        *trans)
{
  MidiChannel *mchannel = this;
  VoiceChangeType vctype = etype == BSE_MIDI_KEY_PRESSURE ? VOICE_PRESSURE : (sustain_note ? VOICE_SUSTAIN : VOICE_OFF);
  gfloat freq_val = BSE_VALUE_FROM_FREQ (freq);
  VoiceInput *vinput = NULL;

  assert_return (freq > 0 && velocity >= 0);

  /* adjust channel global mono synth */
  if (mchannel->vinput)
    change_voice_input_L (mchannel->vinput, tick_stamp, vctype, freq_val, velocity, trans);

  if (!mchannel->poly_enabled)
    return;

  /* find corresponding vinput */
  vinput = mchannel->voice_input_table[freq_val];
  while (vinput && vinput->queue_state != VSTATE_BUSY)
    vinput = vinput->next;

  /* adjust note */
  if (vinput)
    change_voice_input_L (vinput, tick_stamp, vctype, freq_val, velocity, trans);
  else
    no_poly_voice (false, etype == BSE_MIDI_NOTE_OFF ? "note-off" : "velocity", freq);
}

void
MidiChannel::kill_notes (guint64       tick_stamp,
                         gboolean      sustained_only,
                         BseTrans     *trans)
{
  MidiChannel *mchannel = this;
  guint i, j;

  /* adjust channel global voice inputs */
  if (mchannel->vinput && sustained_only && mchannel->vinput->queue_state == VSTATE_SUSTAINED)
    change_voice_input_L (mchannel->vinput, tick_stamp, VOICE_KILL_SUSTAIN, 0, 0, trans);
  else if (mchannel->vinput && !sustained_only && mchannel->vinput->queue_state != VSTATE_IDLE)
    change_voice_input_L (mchannel->vinput, tick_stamp, VOICE_KILL, 0, 0, trans);

  /* adjust poly voice inputs */
  for (i = 0; i < mchannel->n_voices; i++)
    {
      VoiceSwitch *vswitch = mchannel->voices[i];
      if (vswitch)
        {
          for (j = 0; j < vswitch->n_vinputs; j++)
            if (sustained_only && vswitch->vinputs[j]->queue_state == VSTATE_SUSTAINED)
              change_voice_input_L (vswitch->vinputs[j], tick_stamp, VOICE_KILL_SUSTAIN, 0, 0, trans);
            else if (!sustained_only && vswitch->vinputs[j]->queue_state != VSTATE_BUSY)
              change_voice_input_L (vswitch->vinputs[j], tick_stamp, VOICE_KILL, 0, 0, trans);
        }
    }
}

void
MidiChannel::debug_notes (guint64 tick_stamp, BseTrans *trans)
{
  MidiChannel *mchannel = this;
  guint i, j;
  for (i = 0; i < mchannel->n_voices; i++)
    {
      VoiceSwitch *vswitch = mchannel->voices[i];
      if (vswitch)
        for (j = 0; j < vswitch->n_vinputs; j++)
          Bse::info ("MidiChannel(%u):Voice<%p>=%c: Synth<%p:%08llx>: State=%s Queued=%s Freq=%.2fHz",
                     mchannel->midi_channel, vswitch, vswitch->disconnected ? 'd' : 'C',
                     vswitch->vinputs[j], bse_module_tick_stamp (vswitch->vinputs[j]->fmodule),
                     voice_state_to_string (vswitch->vinputs[j]->vstate),
                     voice_state_to_string (vswitch->vinputs[j]->queue_state),
                     BSE_FREQ_FROM_VALUE (vswitch->vinputs[j]->freq_value));
    }
}
} // namespace anon

struct BseMidiReceiver : public MidiReceiver {
  explicit BseMidiReceiver () :
    MidiReceiver () {}
};
/* --- prototypes --- */
static gint	midi_receiver_process_event_L  (BseMidiReceiver        *self,
						guint64                 max_tick_stamp);

/* --- variables --- */
static vector<BseMidiReceiver*> farm_residents;

/* --- function --- */
static gint
events_cmp (gconstpointer a,
            gconstpointer b,
            gpointer      data)
{
  const BseMidiEvent *e1 = (const BseMidiEvent *) a;
  const BseMidiEvent *e2 = (const BseMidiEvent *) b;

  return e1->delta_time < e2->delta_time ? -1 : e1->delta_time != e2->delta_time;
}

void
bse_midi_receiver_enter_farm (BseMidiReceiver *self)
{
  assert_return (self != NULL);
  assert_return (find (farm_residents.begin(), farm_residents.end(), self) == farm_residents.end());

  BSE_MIDI_RECEIVER_LOCK ();
  farm_residents.push_back (self);
  BSE_MIDI_RECEIVER_UNLOCK ();
}

void
bse_midi_receiver_farm_distribute_event (BseMidiEvent *event)
{
  assert_return (event != NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  for (vector<BseMidiReceiver*>::iterator it = farm_residents.begin(); it != farm_residents.end(); it++)
    (*it)->events = sfi_ring_insert_sorted ((*it)->events, bse_midi_copy_event (event), events_cmp, NULL);
  BSE_MIDI_RECEIVER_UNLOCK ();
}

void
bse_midi_receiver_farm_process_events (guint64 max_tick_stamp)
{
  gboolean seen_event;
  do
    {
      seen_event = FALSE;
      BSE_MIDI_RECEIVER_LOCK ();
      for (vector<BseMidiReceiver*>::iterator it = farm_residents.begin(); it != farm_residents.end(); it++)
        seen_event |= midi_receiver_process_event_L (*it, max_tick_stamp);
      BSE_MIDI_RECEIVER_UNLOCK ();
    }
  while (seen_event);
}

void
bse_midi_receiver_leave_farm (BseMidiReceiver *self)
{
  assert_return (self != NULL);
  assert_return (find (farm_residents.begin(), farm_residents.end(), self) != farm_residents.end());

  BSE_MIDI_RECEIVER_LOCK ();
  farm_residents.erase (find (farm_residents.begin(), farm_residents.end(), self));
  BSE_MIDI_RECEIVER_UNLOCK ();
}

void
bse_midi_receiver_push_event (BseMidiReceiver *self,
			      BseMidiEvent    *event)
{
  assert_return (self != NULL);
  assert_return (event != NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  self->events = sfi_ring_insert_sorted (self->events, event, events_cmp, NULL);
  BSE_MIDI_RECEIVER_UNLOCK ();
}

void
bse_midi_receiver_process_events (BseMidiReceiver *self,
				  guint64          max_tick_stamp)
{
  gboolean seen_event;

  assert_return (self != NULL);

  do
    {
      BSE_MIDI_RECEIVER_LOCK ();
      seen_event = midi_receiver_process_event_L (self, max_tick_stamp);
      BSE_MIDI_RECEIVER_UNLOCK ();
    }
  while (seen_event);
}


BseMidiReceiver*
bse_midi_receiver_new (const gchar *receiver_name)  // FIXME
{
  BseMidiReceiver *self;

  self = new BseMidiReceiver ();

  return self;
}

BseMidiReceiver*
bse_midi_receiver_ref (BseMidiReceiver *self)
{
  assert_return (self != NULL, NULL);
  assert_return (self->ref_count > 0, NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  self->ref_count++;
  BSE_MIDI_RECEIVER_UNLOCK ();

  return self;
}

void
bse_midi_receiver_unref (BseMidiReceiver *self)
{
  gboolean need_destroy, leave_farm;

  assert_return (self != NULL);
  assert_return (self->ref_count > 0);

  BSE_MIDI_RECEIVER_LOCK ();
  self->ref_count--;
  need_destroy = self->ref_count == 0;
  leave_farm = need_destroy && find (farm_residents.begin(),
                                     farm_residents.end(), self) != farm_residents.end();
  BSE_MIDI_RECEIVER_UNLOCK ();

  if (need_destroy)
    {
      if (leave_farm)
        bse_midi_receiver_leave_farm (self);
      delete self;
    }
}

void
bse_midi_receiver_set_notifier (BseMidiReceiver *self,
				BseMidiNotifier *notifier)
{
  BseMidiNotifier *old_notifier;

  assert_return (self != NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  old_notifier = self->notifier;
  self->notifier = notifier;
  if (self->notifier)
    g_object_ref (notifier);
  if (old_notifier)
    g_object_unref (old_notifier);
  if (!self->notifier)
    while (self->notifier_events)
      {
	BseMidiEvent *event = (BseMidiEvent *) sfi_ring_pop_head (&self->notifier_events);
	bse_midi_free_event (event);
      }
  BSE_MIDI_RECEIVER_UNLOCK ();
}

gboolean
bse_midi_receiver_has_notify_events (BseMidiReceiver *self)
{
  /* prolly don't need a lock */
  return self->notifier && self->notifier_events;
}

SfiRing*
bse_midi_receiver_fetch_notify_events (BseMidiReceiver *self)
{
  SfiRing *ring;

  assert_return (self != NULL, NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  ring = self->notifier_events;
  self->notifier_events = NULL;
  BSE_MIDI_RECEIVER_UNLOCK ();

  return ring;
}

BseModule*
bse_midi_receiver_retrieve_control_module (BseMidiReceiver  *self,
					   guint             midi_channel,
					   Bse::MidiSignal signals[BSE_MIDI_CONTROL_MODULE_N_CHANNELS],
					   BseTrans         *trans)
{
  BseModule *cmodule;
  guint i;

  assert_return (self != NULL, NULL);
  assert_return (midi_channel > 0, NULL);
  assert_return (signals != NULL, NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  for (i = 0; i < self->n_cmodules; i++)
    {
      cmodule = self->cmodules[i];
      if (match_midi_control_module_L (cmodule, midi_channel, signals))
        {
          MidiCModuleData *cdata = (MidiCModuleData *) cmodule->user_data;
          cdata->ref_count++;
	  BSE_MIDI_RECEIVER_UNLOCK ();
          return cmodule;
        }
    }
  cmodule = create_midi_control_module_L (self, midi_channel, signals);
  i = self->n_cmodules++;
  self->cmodules = g_renew (BseModule*, self->cmodules, self->n_cmodules);
  self->cmodules[i] = cmodule;
  bse_trans_add (trans, bse_job_integrate (cmodule));
  self->add_control (midi_channel, signals[0], cmodule);
  if (signals[1] != signals[0])
    self->add_control (midi_channel, signals[1], cmodule);
  if (signals[2] != signals[1] && signals[2] != signals[0])
    self->add_control (midi_channel, signals[2], cmodule);
  if (signals[3] != signals[2] && signals[3] != signals[1] && signals[3] != signals[0])
    self->add_control (midi_channel, signals[3], cmodule);
  BSE_MIDI_RECEIVER_UNLOCK ();
  return cmodule;
}

void
bse_midi_receiver_discard_control_module (BseMidiReceiver *self,
                                          BseModule       *module,
					  BseTrans        *trans)
{
  guint i;

  assert_return (self != NULL);
  assert_return (module != NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  for (i = 0; i < self->n_cmodules; i++)
    {
      BseModule *cmodule = self->cmodules[i];
      if (cmodule == module)
        {
          MidiCModuleData *cdata = (MidiCModuleData *) cmodule->user_data;
          assert_return (cdata->ref_count > 0);
          cdata->ref_count--;
          if (!cdata->ref_count)
            {
	      Bse::MidiSignal *signals = cdata->signals;
	      guint midi_channel = cdata->midi_channel;
              self->n_cmodules--;
              self->cmodules[i] = self->cmodules[self->n_cmodules];
              bse_trans_add (trans, bse_job_boundary_discard (cmodule));
	      self->remove_control (midi_channel, signals[0], cmodule);
	      if (signals[1] != signals[0])
		self->remove_control (midi_channel, signals[1], cmodule);
	      if (signals[2] != signals[1] && signals[2] != signals[0])
		self->remove_control (midi_channel, signals[2], cmodule);
	      if (signals[3] != signals[2] && signals[3] != signals[1] && signals[3] != signals[0])
		self->remove_control (midi_channel, signals[3], cmodule);
	    }
	  BSE_MIDI_RECEIVER_UNLOCK ();
          return;
        }
    }
  BSE_MIDI_RECEIVER_UNLOCK ();
  Bse::warning ("no such control module: %p", module);
}

gboolean
bse_midi_receiver_add_control_handler (BseMidiReceiver      *self,
                                       guint                 midi_channel,
                                       Bse::MidiSignal     signal_type,
                                       BseMidiControlHandler handler_func,
                                       gpointer              handler_data,
                                       BseModule            *module)
{
  assert_return (self != NULL, FALSE);
  assert_return (midi_channel > 0, FALSE);
  assert_return (handler_func != NULL, FALSE);
  assert_return (module != NULL, FALSE);

  BSE_MIDI_RECEIVER_LOCK ();
  gboolean has_data = self->add_control_handler (midi_channel, signal_type, handler_func, handler_data, module);
  BSE_MIDI_RECEIVER_UNLOCK ();
  return has_data;
}

void
bse_midi_receiver_set_control_handler_data (BseMidiReceiver      *self,
                                            guint                 midi_channel,
                                            Bse::MidiSignal     signal_type,
                                            BseMidiControlHandler handler_func,
                                            gpointer              handler_data,
                                            gpointer              extra_data,
                                            BseFreeFunc           extra_free)
{
  assert_return (self != NULL);
  assert_return (midi_channel > 0);
  assert_return (handler_func != NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  self->set_control_handler_data (midi_channel, signal_type, handler_func, handler_data, extra_data, extra_free);
  BSE_MIDI_RECEIVER_UNLOCK ();
}

void
bse_midi_receiver_remove_control_handler (BseMidiReceiver      *self,
                                          guint                 midi_channel,
                                          Bse::MidiSignal     signal_type,
                                          BseMidiControlHandler handler_func,
                                          gpointer              handler_data,
                                          BseModule            *module)
{
  assert_return (self != NULL);
  assert_return (midi_channel > 0);
  assert_return (handler_func != NULL);
  assert_return (module != NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  self->remove_control_handler (midi_channel, signal_type, handler_func, handler_data, module);
  BSE_MIDI_RECEIVER_UNLOCK ();
}

void
bse_midi_receiver_add_event_handler (BseMidiReceiver   *self,
                                     guint              midi_channel,
                                     BseMidiEventHandler handler_func,
                                     gpointer           handler_data,
                                     BseModule         *module)
{
  assert_return (self != NULL);
  assert_return (midi_channel > 0);
  assert_return (handler_func != NULL);
  assert_return (module != NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  self->add_event_handler (midi_channel, handler_func, handler_data, module);
  BSE_MIDI_RECEIVER_UNLOCK ();
}

void
bse_midi_receiver_remove_event_handler (BseMidiReceiver   *self,
                                        guint              midi_channel,
                                        BseMidiEventHandler handler_func,
                                        gpointer           handler_data,
                                        BseModule         *module)
{
  assert_return (self != NULL);
  assert_return (midi_channel > 0);
  assert_return (handler_func != NULL);
  assert_return (module != NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  self->remove_event_handler (midi_channel, handler_func, handler_data, module);
  BSE_MIDI_RECEIVER_UNLOCK ();
}

void
bse_midi_receiver_channel_enable_poly (BseMidiReceiver *self,
                                       guint            midi_channel)
{
  assert_return (self != NULL);
  assert_return (midi_channel > 0);

  BSE_MIDI_RECEIVER_LOCK ();
  MidiChannel *mchannel = self->get_channel (midi_channel);
  mchannel->enable_poly();
  BSE_MIDI_RECEIVER_UNLOCK ();
}

void
bse_midi_receiver_channel_disable_poly (BseMidiReceiver *self,
                                        guint            midi_channel)
{
  assert_return (self != NULL);
  assert_return (midi_channel > 0);

  BSE_MIDI_RECEIVER_LOCK ();
  MidiChannel *mchannel = self->get_channel (midi_channel);
  mchannel->disable_poly();
  BSE_MIDI_RECEIVER_UNLOCK ();
}

BseModule*
bse_midi_receiver_retrieve_mono_voice (BseMidiReceiver *self,
                                       guint            midi_channel,
                                       BseTrans        *trans)
{
  MidiChannel *mchannel;

  assert_return (self != NULL, NULL);
  assert_return (midi_channel > 0, NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  mchannel = self->get_channel (midi_channel);
  if (mchannel->vinput)
    mchannel->vinput->ref_count++;
  else
    mchannel->vinput = create_voice_input_L (&mchannel->voice_input_table, TRUE, trans);
  BSE_MIDI_RECEIVER_UNLOCK ();
  return mchannel->vinput->fmodule;
}

void
bse_midi_receiver_discard_mono_voice (BseMidiReceiver *self,
                                      guint            midi_channel,
                                      BseModule       *fmodule,
                                      BseTrans        *trans)
{
  MidiChannel *mchannel;

  assert_return (self != NULL);
  assert_return (fmodule != NULL);

  BSE_MIDI_RECEIVER_LOCK ();
  mchannel = self->get_channel (midi_channel);
  if (mchannel->vinput && mchannel->vinput->fmodule == fmodule)
    {
      mchannel->vinput->ref_count--;
      if (!mchannel->vinput->ref_count)
        {
          destroy_voice_input_L (mchannel->vinput, trans);
          mchannel->vinput = NULL;
        }
      BSE_MIDI_RECEIVER_UNLOCK ();
      return;
    }
  BSE_MIDI_RECEIVER_UNLOCK ();
  Bse::warning ("no such mono synth module: %p", fmodule);
}

guint
bse_midi_receiver_create_poly_voice (BseMidiReceiver *self,
                                     guint            midi_channel,
                                     BseTrans        *trans)
{
  MidiChannel *mchannel;
  guint i;

  assert_return (self != NULL, 0);
  assert_return (midi_channel > 0, 0);

  BSE_MIDI_RECEIVER_LOCK ();
  mchannel = self->get_channel (midi_channel);
  /* find free voice slot */
  for (i = 0; i < mchannel->n_voices; i++)
    if (mchannel->voices[i] == NULL)
      break;
  /* alloc voice slot */
  if (i >= mchannel->n_voices)
    {
      i = mchannel->n_voices++;
      mchannel->voices = g_renew (VoiceSwitch*, mchannel->voices, mchannel->n_voices);
    }
  mchannel->voices[i] = create_voice_switch_module_L (trans);
  BSE_MIDI_RECEIVER_UNLOCK ();

  return i + 1;
}

void
bse_midi_receiver_discard_poly_voice (BseMidiReceiver *self,
                                      guint            midi_channel,
                                      guint            voice_id,
                                      BseTrans        *trans)
{
  MidiChannel *mchannel;
  VoiceSwitch *vswitch;

  assert_return (self != NULL);
  assert_return (midi_channel > 0);
  assert_return (voice_id > 0);
  voice_id -= 1;

  BSE_MIDI_RECEIVER_LOCK ();
  mchannel = self->get_channel (midi_channel);
  vswitch = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  if (vswitch)
    {
      assert_return (vswitch->ref_count > 0);
      vswitch->ref_count--;
      if (!vswitch->ref_count)
        {
          destroy_voice_switch_L (vswitch, trans);
          mchannel->voices[voice_id] = NULL;
        }
    }
  BSE_MIDI_RECEIVER_UNLOCK ();
  if (!vswitch)
    Bse::warning ("MIDI channel %u has no voice %u", midi_channel, voice_id + 1);
}

BseModule*
bse_midi_receiver_get_poly_voice_input (BseMidiReceiver   *self,
                                        guint              midi_channel,
                                        guint              voice_id)
{
  MidiChannel *mchannel;
  VoiceSwitch *vswitch;
  BseModule *module;

  assert_return (self != NULL, NULL);
  assert_return (midi_channel > 0, NULL);
  assert_return (voice_id > 0, NULL);
  voice_id -= 1;

  BSE_MIDI_RECEIVER_LOCK ();
  mchannel = self->get_channel (midi_channel);
  vswitch = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  module = vswitch ? vswitch->smodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK ();
  return module;
}

BseModule*
bse_midi_receiver_get_poly_voice_output (BseMidiReceiver   *self,
                                         guint              midi_channel,
                                         guint              voice_id)
{
  MidiChannel *mchannel;
  VoiceSwitch *vswitch;
  BseModule *module;

  assert_return (self != NULL, NULL);
  assert_return (midi_channel > 0, NULL);
  assert_return (voice_id > 0, NULL);
  voice_id -= 1;

  BSE_MIDI_RECEIVER_LOCK ();
  mchannel = self->get_channel (midi_channel);
  vswitch = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  module = vswitch ? vswitch->vmodule : NULL;
  BSE_MIDI_RECEIVER_UNLOCK ();
  return module;
}

BseModule*
bse_midi_receiver_create_sub_voice (BseMidiReceiver   *self,
                                    guint              midi_channel,
                                    guint              voice_id,
                                    BseTrans          *trans)
{
  MidiChannel *mchannel;
  VoiceSwitch *vswitch;
  BseModule *module = NULL;

  assert_return (self != NULL, NULL);
  assert_return (midi_channel > 0, NULL);
  assert_return (voice_id > 0, NULL);
  voice_id -= 1;

  BSE_MIDI_RECEIVER_LOCK ();
  mchannel = self->get_channel (midi_channel);
  vswitch = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  uint n = 0;
  if (vswitch)
    {
      guint i = vswitch->n_vinputs++;
      vswitch->vinputs = g_renew (VoiceInput*, vswitch->vinputs, vswitch->n_vinputs);
      vswitch->vinputs[i] = create_voice_input_L (&mchannel->voice_input_table, FALSE, trans);
      vswitch->ref_count++;
      module = vswitch->vinputs[i]->fmodule;
      n = vswitch->n_vinputs;
    }
  BSE_MIDI_RECEIVER_UNLOCK ();
  assert_return (n <= 1, module); // we don't actually ever create more than one vinput per vswitch
  return module;
}

void
bse_midi_receiver_discard_sub_voice (BseMidiReceiver   *self,
                                     guint              midi_channel,
                                     guint              voice_id,
                                     BseModule         *fmodule,
                                     BseTrans          *trans)
{
  MidiChannel *mchannel;
  VoiceSwitch *vswitch;
  guint i, need_unref = FALSE;

  assert_return (self != NULL);
  assert_return (midi_channel > 0);
  assert_return (fmodule != NULL);
  assert_return (voice_id > 0);
  voice_id -= 1;

  BSE_MIDI_RECEIVER_LOCK ();
  mchannel = self->get_channel (midi_channel);
  vswitch = voice_id < mchannel->n_voices ? mchannel->voices[voice_id] : NULL;
  if (vswitch)
    for (i = 0; i < vswitch->n_vinputs; i++)
      if (vswitch->vinputs[i]->fmodule == fmodule)
        {
          vswitch->vinputs[i]->ref_count--;
          /* first, unset ref_count */
          if (!vswitch->vinputs[i]->ref_count)
            {
              VoiceInput *vinput = vswitch->vinputs[i];
              /* second, unlist vinput */
              vswitch->vinputs[i] = vswitch->vinputs[--vswitch->n_vinputs];
              BSE_MFENCE;
              /* last, queue vinput destruction */
              destroy_voice_input_L (vinput, trans);
              /* the order of the above steps is important to prevent DSP-threads
               * from queueing jobs on vinput after destruction has been queued on it
               */
              need_unref = TRUE;
            }
          fmodule = NULL;
          break;
        }
  BSE_MIDI_RECEIVER_UNLOCK ();
  if (need_unref)
    bse_midi_receiver_discard_poly_voice (self, midi_channel, voice_id + 1, trans);
  if (fmodule)
    Bse::warning ("MIDI channel %u, poly voice %u, no such sub voice: %p", midi_channel, voice_id, fmodule);
}

gboolean
bse_midi_receiver_voices_pending (BseMidiReceiver *self,
                                  guint            midi_channel)
{
  MidiChannel *mchannel;
  SfiRing *ring = NULL;
  guint i, active = 0;

  assert_return (self != NULL, FALSE);
  assert_return (midi_channel > 0, FALSE);

  if (self->events)
    return TRUE;

  BSE_MIDI_RECEIVER_LOCK ();
  mchannel = self->get_channel (midi_channel);
  if (mchannel)
    {
      active = active || (mchannel->vinput && (mchannel->vinput->vstate != VSTATE_IDLE ||
                                               mchannel->vinput->queue_state != VSTATE_IDLE));
      /* find busy poly voice */
      for (i = 0; i < mchannel->n_voices && !active; i++)
        active = active || (mchannel->voices[i] && !check_voice_switch_available_L (mchannel->voices[i]));
    }
  /* find pending events */
  for (ring = self->events; ring && !active; ring = sfi_ring_next (ring, self->events))
    {
      BseMidiEvent *event = (BseMidiEvent *) ring->data;
      active += event->channel == midi_channel;
    }
  BSE_MIDI_RECEIVER_UNLOCK ();

  return active > 0;
}


/* --- event processing --- */
static inline void
update_midi_signal_L (BseMidiReceiver  *self,
		      guint             channel,
		      guint64           tick_stamp,
		      Bse::MidiSignal signal,
		      gfloat            value,
		      BseTrans         *trans)
{
  GSList *signal_modules;

  signal_modules = self->set_control (channel, tick_stamp, signal, value, trans);
  change_midi_control_modules_L (signal_modules, tick_stamp,
                                 signal, value, trans);
#if 0
  MDEBUG ("MidiChannel[%u]: Signal %3u Value=%f (%s)", channel,
          signal, value, bse_midi_signal_name (signal));
#endif
}

static inline void
update_midi_signal_continuous_msb_L (BseMidiReceiver  *self,
				     guint             channel,
				     guint64           tick_stamp,
				     Bse::MidiSignal continuous_signal,
				     gfloat            value,
				     Bse::MidiSignal lsb_signal,
				     BseTrans         *trans)
{
  gint ival;
  /* LSB part */
  ival = bse_ftoi (self->get_control (channel, lsb_signal) * 0x7f);
  /* add MSB part */
  ival |= bse_ftoi (value * 0x7f) << 7;
  /* set continuous */
  value = ival / (gfloat) 0x3fff;
  update_midi_signal_L (self, channel, tick_stamp, continuous_signal, value, trans);
}

static inline void
update_midi_signal_continuous_lsb_L (BseMidiReceiver  *self,
				     guint             channel,
				     guint64           tick_stamp,
				     Bse::MidiSignal continuous_signal,
				     Bse::MidiSignal msb_signal,
				     gfloat            value,
				     BseTrans         *trans)
{
  gint ival;
  /* LSB part */
  ival = bse_ftoi (value * 0x7f);
  /* add MSB part */
  ival |= bse_ftoi (self->get_control (channel, msb_signal) * 0x7f) << 7;
  value = ival / (gfloat) 0x3fff;
  update_midi_signal_L (self, channel, tick_stamp, continuous_signal, value, trans);
}

static inline void
process_midi_control_L (BseMidiReceiver *self,
			guint            channel,
			guint64          tick_stamp,
			guint            control,
			gfloat           value,
                        gboolean         extra_continuous,
			BseTrans        *trans)
{
  /* here, we need to translate midi control numbers
   * into BSE MIDI signals. some control numbers affect
   * multiple MIDI signals. extra_continuous are used
   * internally to update only continuous signals.
   */

  if (extra_continuous)
    {
      /* internal Bse::MIDI_SIGNAL_CONTINUOUS_* change */
      update_midi_signal_L (self, channel, tick_stamp, static_cast<Bse::MidiSignal> (64 + control), value, trans);
      return;
    }

  /* all MIDI controls are passed literally as Bse::MIDI_SIGNAL_CONTROL_* */
  update_midi_signal_L (self, channel, tick_stamp, static_cast<Bse::MidiSignal> (128 + control), value, trans);

  if (control < 32)		/* MSB part of continuous 14bit signal */
    update_midi_signal_continuous_msb_L (self, channel, tick_stamp,
					 static_cast<Bse::MidiSignal> (control + 64),		/* continuous signal */
					 value,							/* MSB value */
					 static_cast<Bse::MidiSignal> (128 + control + 32),	/* LSB signal */
					 trans);
  else if (control < 64)	/* LSB part of continuous 14bit signal */
    update_midi_signal_continuous_lsb_L (self, channel, tick_stamp,
					 static_cast<Bse::MidiSignal> (control + 32),		/* continuous signal */
					 static_cast<Bse::MidiSignal> (128 + control - 32),	/* MSB signal */
					 value,							/* LSB value */
					 trans);
  else switch (control)
    {
      MidiChannel *mchannel;
    case 64:			/* Damper Pedal Switch (Sustain) */
      mchannel = self->peek_channel (channel);
      if (mchannel && (uint (Bse::global_config->invert_sustain) ^ (value < 0.5)))
	mchannel->kill_notes (tick_stamp, TRUE, trans);
      break;
    case 98:			/* Non-Registered Parameter MSB */
      update_midi_signal_continuous_msb_L (self, channel, tick_stamp,
					   Bse::MidiSignal::NON_PARAMETER,	/* continuous signal */
					   value,                 		/* MSB value */
					   Bse::MidiSignal::CONTROL_99,		/* LSB signal */
					   trans);
      break;
    case 99:			/* Non-Registered Parameter LSB */
      update_midi_signal_continuous_lsb_L (self, channel, tick_stamp,
					   Bse::MidiSignal::NON_PARAMETER,	/* continuous signal */
					   Bse::MidiSignal::CONTROL_98,		/* MSB signal */
					   value,                 		/* LSB value */
					   trans);
      break;
    case 100:			/* Registered Parameter MSB */
      update_midi_signal_continuous_msb_L (self, channel, tick_stamp,
					   Bse::MidiSignal::PARAMETER,		/* continuous signal */
					   value,                 		/* MSB value */
					   Bse::MidiSignal::CONTROL_101,		/* LSB signal */
					   trans);
      break;
    case 101:			/* Registered Parameter LSB */
      update_midi_signal_continuous_lsb_L (self, channel, tick_stamp,
					   Bse::MidiSignal::PARAMETER,		/* continuous signal */
					   Bse::MidiSignal::CONTROL_100,		/* MSB signal */
					   value,                 		/* LSB value */
					   trans);
      break;
    case 120:			/* All Sound Off ITrigger */
    case 123:			/* All Notes Off ITrigger */
      mchannel = self->peek_channel (channel);
      if (mchannel)
        mchannel->kill_notes (tick_stamp, FALSE, trans);
      break;
    case 122:			/* Local Control Switch */
      mchannel = self->peek_channel (channel);
      if (mchannel && value < 0.00006)
        mchannel->debug_notes (tick_stamp, trans);
      break;
    }
}

static gint
midi_receiver_process_event_L (BseMidiReceiver *self,
			       guint64          max_tick_stamp)
{
  BseMidiEvent *event;
  gboolean need_wakeup = FALSE;

  if (!self->events)
    return FALSE;

  event = (BseMidiEvent *) self->events->data;
  if (event->delta_time <= max_tick_stamp)
    {
      BseTrans *trans = bse_trans_open ();
      MidiChannel *mchannel = self->peek_channel (event->channel);
      self->events = sfi_ring_remove_node (self->events, self->events);
      uint event_status = event->status;
      if (mchannel && mchannel->call_event_handlers (event, trans))
        event_status = 0; // already handled
      switch (event_status)
        {
        case 0:
          // already handled by call_event_handlers()
          break;
        case BSE_MIDI_NOTE_ON:
          EDUMP ("MidiChannel[%u]: NoteOn  %fHz Velo=%f channel=%s (stamp:%llu)", event->channel,
                 event->data.note.frequency, event->data.note.velocity, mchannel ? string_from_int (event->channel) : "<unknown>", event->delta_time);
          if (mchannel)
            mchannel->start_note (event->delta_time,
                                  event->data.note.frequency,
                                  event->data.note.velocity,
                                  trans);
          else
            Bse::info ("ignoring note-on (%fHz) for foreign midi channel: %u", event->data.note.frequency, event->channel);
          break;
        case BSE_MIDI_KEY_PRESSURE:
        case BSE_MIDI_NOTE_OFF:
          mchannel = self->peek_channel (event->channel);
          EDUMP ("MidiChannel[%u]: %s %fHz channel=%s (stamp:%llu)", event->channel,
                 event->status == BSE_MIDI_NOTE_OFF ? "NoteOff" : "NotePressure",
                 event->data.note.frequency, mchannel ? string_from_int (event->channel) : "<unknown>", event->delta_time);
          if (mchannel)
            {
              bool sustained_note = event->status == BSE_MIDI_NOTE_OFF &&
                                    (uint (Bse::global_config->invert_sustain) ^
                                     (self->get_control (event->channel, Bse::MidiSignal::CONTROL_64) >= 0.5));
              mchannel->adjust_note (event->delta_time,
                                     event->data.note.frequency, event->status,
                                     event->data.note.velocity, sustained_note, trans);
            }
          break;
        case BSE_MIDI_CONTROL_CHANGE:
          EDUMP ("MidiChannel[%u]: Control %2u Value=%f (stamp:%llu)", event->channel,
                 event->data.control.control, event->data.control.value, event->delta_time);
          process_midi_control_L (self, event->channel, event->delta_time,
                                  event->data.control.control, event->data.control.value,
                                  FALSE,
                                  trans);
          break;
        case BSE_MIDI_X_CONTINUOUS_CHANGE:
          EDUMP ("MidiChannel[%u]: X Continuous Control %2u Value=%f (stamp:%llu)", event->channel,
                 event->data.control.control, event->data.control.value, event->delta_time);
          process_midi_control_L (self, event->channel, event->delta_time,
                                  event->data.control.control, event->data.control.value,
                                  TRUE,
                                  trans);
          break;
        case BSE_MIDI_PROGRAM_CHANGE:
          EDUMP ("MidiChannel[%u]: Program %u (Value=%f) (stamp:%llu)", event->channel,
                 event->data.program, event->data.program / (gfloat) 0x7f, event->delta_time);
          update_midi_signal_L (self, event->channel, event->delta_time,
                                Bse::MidiSignal::PROGRAM, event->data.program / (gfloat) 0x7f,
                                trans);
          break;
        case BSE_MIDI_CHANNEL_PRESSURE:
          EDUMP ("MidiChannel[%u]: Channel Pressure Value=%f (stamp:%llu)", event->channel,
                 event->data.intensity, event->delta_time);
          update_midi_signal_L (self, event->channel, event->delta_time,
                                Bse::MidiSignal::PRESSURE, event->data.intensity,
                                trans);
          break;
        case BSE_MIDI_PITCH_BEND:
          EDUMP ("MidiChannel[%u]: Pitch Bend Value=%f (stamp:%llu)", event->channel,
                 event->data.pitch_bend, event->delta_time);
          update_midi_signal_L (self, event->channel, event->delta_time,
                                Bse::MidiSignal::PITCH_BEND, event->data.pitch_bend,
                                trans);
          break;
        default:
          EDUMP ("MidiChannel[%u]: Ignoring Event %u (stamp:%llu)", event->channel,
                 event->status, event->delta_time);
          break;
        }
      if (self->notifier)
        {
          self->notifier_events = sfi_ring_append (self->notifier_events, event);
          need_wakeup = TRUE;
        }
      else
        bse_midi_free_event (event);
      bse_trans_commit (trans);
    }
  else
    return FALSE;

  return TRUE;
}
