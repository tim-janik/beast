// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bse/processor.hh"
#include "bse/signalmath.hh"
#include "bse/internal.hh"

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/parameters/parameters.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/presets/presets.h"

#include "lv2_evbuf.h"

#include <lilv/lilv.h>

namespace Bse {

using namespace AudioSignal;

namespace
{

#define NS_EXT "http://lv2plug.in/ns/ext/"

using std::vector;
using std::string;
using std::map;
using std::max;
using std::min;

class Map
{
  LV2_URID              next_id;
  map<string, LV2_URID> m_urid_map;

  LV2_URID_Map       lv2_urid_map;
  const LV2_Feature  lv2_urid_map_feature;
public:
  Map() :
    next_id (1),
    lv2_urid_map { this, urid_map },
    lv2_urid_map_feature { LV2_URID_MAP_URI, &lv2_urid_map }
  {
  }

  static LV2_URID
  urid_map (LV2_URID_Map_Handle handle, const char *str)
  {
    return static_cast<Map *> (handle)->urid_map (str);
  }

  LV2_URID
  urid_map (const char *str)
  {
    LV2_URID& id = m_urid_map[str];
    if (id == 0)
      id = next_id++;

    printf ("map %s -> %d\n", str, id);
    return id;
  }

  const LV2_Feature *
  feature() const
  {
    return &lv2_urid_map_feature;
  }
  LV2_URID_Map *
  lv2_map()
  {
    return &lv2_urid_map;
  }
};

class PluginHost;

class Options
{
  PluginHost& plugin_host;
  float       m_sample_rate;
  uint32_t    m_block_length;

  vector<LV2_Options_Option> const_opts;

  LV2_Feature  lv2_options_feature;
public:
  Options (PluginHost& plugin_host);
  void
  set (float sample_rate, int32_t block_length)
  {
    m_sample_rate   = sample_rate;
    m_block_length  = block_length;
  }
  const LV2_Feature *
  feature() const
  {
    return &lv2_options_feature;
  }
};

class Worker
{
  LV2_Worker_Schedule lv2_worker_sched;
  const LV2_Feature   lv2_worker_feature;
public:
  Worker() :
    lv2_worker_sched { this, schedule },
    lv2_worker_feature { LV2_WORKER__schedule, &lv2_worker_sched }
  {
  }

  static LV2_Worker_Status
  schedule (LV2_Worker_Schedule_Handle handle,
            uint32_t                   size,
            const void*                data)
  {
    printf ("schedule %p %d %p\n", handle, size, data);
    return LV2_WORKER_ERR_UNKNOWN; // TODO
  }

  const LV2_Feature *
  feature() const
  {
    return &lv2_worker_feature;
  }
};


class Features
{
  std::vector<const LV2_Feature *> features;
public:
  Features()
  {
    features.push_back (nullptr);
  }
  const LV2_Feature * const*
  get_features()
  {
    return &features[0];
  }
  void
  add (const LV2_Feature *lv2_feature)
  {
    // preserve nullptr termination
    assert_return (!features.empty());

    features.back() = lv2_feature;
    features.push_back (nullptr);
  }
};

struct Port
{
  LV2_Evbuf  *evbuf;
  float       control;    /* for control ports */
  float       min_value;  /* min control */
  float       max_value;  /* max control */
  String      name;
  String      symbol;

  enum {
    UNKNOWN,
    CONTROL_IN,
    CONTROL_OUT
  }           type;

  Port() :
    evbuf (nullptr),
    control (0.0),
    type (UNKNOWN)
  {
  }
};

struct PresetInfo
{
  String          name;
  const LilvNode *preset = nullptr;
};

struct PluginInstance
{
  PluginHost& plugin_host;

  PluginInstance (PluginHost& plugin_host) :
    plugin_host (plugin_host)
  {
  }

  const LilvPlugin             *plugin;
  LilvInstance                 *instance;
  std::vector<Port>             plugin_ports;
  std::vector<int>              atom_out_ports;
  std::vector<int>              atom_in_ports;
  std::vector<int>              audio_in_ports;
  std::vector<int>              audio_out_ports;
  std::vector<PresetInfo>       presets;

  void init_ports();
  void init_presets();
  void reset_event_buffers();
  void write_midi (uint32_t time, size_t size, const uint8_t *data);
  void run (float *audio_out_l, float *audio_out_r, uint32_t nframes);
  void activate();
  void deactivate();
};

struct PluginHost
{
  LilvWorld  *world;
  Features    features;
  Map         urid_map;

  struct URIDs {
    LV2_URID param_sampleRate;
    LV2_URID atom_Double;
    LV2_URID atom_Float;
    LV2_URID atom_Int;
    LV2_URID atom_Long;
    LV2_URID atom_eventTransfer;
    LV2_URID bufsz_maxBlockLength;
    LV2_URID bufsz_minBlockLength;
    LV2_URID midi_MidiEvent;

    URIDs (Map& map) :
      param_sampleRate          (map.urid_map (LV2_PARAMETERS__sampleRate)),
      atom_Double               (map.urid_map (LV2_ATOM__Double)),
      atom_Float                (map.urid_map (LV2_ATOM__Float)),
      atom_Int                  (map.urid_map (LV2_ATOM__Int)),
      atom_Long                 (map.urid_map (LV2_ATOM__Long)),
      atom_eventTransfer        (map.urid_map (LV2_ATOM__eventTransfer)),
      bufsz_maxBlockLength      (map.urid_map (LV2_BUF_SIZE__maxBlockLength)),
      bufsz_minBlockLength      (map.urid_map (LV2_BUF_SIZE__minBlockLength)),
      midi_MidiEvent            (map.urid_map (LV2_MIDI__MidiEvent))
    {
    }
  } urids;

  struct Nodes {
    LilvNode *lv2_audio_class;
    LilvNode *lv2_atom_class;
    LilvNode *lv2_input_class;
    LilvNode *lv2_output_class;
    LilvNode *lv2_control_class;

    LilvNode *lv2_atom_Chunk;
    LilvNode *lv2_atom_Sequence;
    LilvNode *lv2_presets_Preset;

    LilvNode *rdfs_label;

    void init (LilvWorld *world)
    {
      lv2_audio_class   = lilv_new_uri (world, LILV_URI_AUDIO_PORT);
      lv2_atom_class    = lilv_new_uri (world, LILV_URI_ATOM_PORT);
      lv2_input_class   = lilv_new_uri (world, LILV_URI_INPUT_PORT);
      lv2_output_class  = lilv_new_uri (world, LILV_URI_OUTPUT_PORT);
      lv2_control_class = lilv_new_uri (world, LILV_URI_CONTROL_PORT);

      lv2_atom_Chunk    = lilv_new_uri (world, LV2_ATOM__Chunk);
      lv2_atom_Sequence = lilv_new_uri (world, LV2_ATOM__Sequence);

      lv2_presets_Preset = lilv_new_uri(world, LV2_PRESETS__Preset);
      rdfs_label         = lilv_new_uri(world, LILV_NS_RDFS "label");
    }
  } nodes;

  Worker   worker;
  Options  options;

  PluginHost() :
    world (nullptr),
    urids (urid_map),
    options (*this)
  {
    world = lilv_world_new();
    lilv_world_load_all (world);

    nodes.init (world);

    features.add (urid_map.feature());
    features.add (worker.feature());
    features.add (options.feature());
  }
  PluginInstance *instantiate (const char *plugin_uri, float mix_freq);
};

Options::Options (PluginHost& plugin_host) :
  plugin_host (plugin_host),
  lv2_options_feature { LV2_OPTIONS__options, nullptr }
{
  const_opts.push_back ({ LV2_OPTIONS_INSTANCE, 0, plugin_host.urids.param_sampleRate,
                          sizeof(float), plugin_host.urids.atom_Float, &m_sample_rate });
  const_opts.push_back ({ LV2_OPTIONS_INSTANCE, 0, plugin_host.urids.bufsz_minBlockLength,
                          sizeof(int32_t), plugin_host.urids.atom_Int, &m_block_length });
  const_opts.push_back ({ LV2_OPTIONS_INSTANCE, 0, plugin_host.urids.bufsz_maxBlockLength,
                          sizeof(int32_t), plugin_host.urids.atom_Int, &m_block_length });
  const_opts.push_back ({ LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr });

  lv2_options_feature.data = &const_opts[0];
}

PluginInstance *
PluginHost::instantiate (const char *plugin_uri, float mix_freq)
{
  LilvNode* uri = lilv_new_uri (world, plugin_uri);
  if (!uri)
    {
      fprintf (stderr, "Invalid plugin URI <%s>\n", plugin_uri);
      return nullptr;
    }

  const LilvPlugins* plugins = lilv_world_get_all_plugins (world);

  const LilvPlugin*  plugin  = lilv_plugins_get_by_uri (plugins, uri);

  if (!plugin)
    {
      fprintf (stderr, "plugin is nil\n");
      return nullptr;
    }
  lilv_node_free (uri);

  LilvInstance *instance = lilv_plugin_instantiate (plugin, mix_freq, features.get_features());
  if (!instance)
    {
      fprintf (stderr, "plugin instantiate failed\n");
      exit (1);
    }

  PluginInstance *plugin_instance = new PluginInstance (*this);
  plugin_instance->instance = instance;
  plugin_instance->plugin = plugin;
  plugin_instance->init_ports();
  plugin_instance->init_presets();

  return plugin_instance;
}

void
PluginInstance::init_ports()
{
  const int n_ports = lilv_plugin_get_num_ports (plugin);

  // don't resize later, otherwise control connections get lost
  plugin_ports.resize (n_ports);

  vector<float> defaults (n_ports);
  vector<float> min_values (n_ports);
  vector<float> max_values (n_ports);

  size_t n_control_ports = 0;

  lilv_plugin_get_port_ranges_float (plugin, &min_values[0], &max_values[0], &defaults[0]);
  for (int i = 0; i < n_ports; i++)
    {
      const LilvPort *port = lilv_plugin_get_port_by_index (plugin, i);
      if (port)
        {
          if (lilv_port_is_a (plugin, port, plugin_host.nodes.lv2_input_class))
            {
              if (lilv_port_is_a (plugin, port, plugin_host.nodes.lv2_audio_class))
                {
                  audio_in_ports.push_back (i);
                }
              else if (lilv_port_is_a (plugin, port, plugin_host.nodes.lv2_atom_class))
                {
                  printf ("found atom input port\n");
                  const int buf_size = 4096;
                  plugin_ports[i].evbuf = lv2_evbuf_new (buf_size, LV2_EVBUF_ATOM, plugin_host.urid_map.urid_map (lilv_node_as_string (plugin_host.nodes.lv2_atom_Chunk)),
                                                                                   plugin_host.urid_map.urid_map (lilv_node_as_string (plugin_host.nodes.lv2_atom_Sequence)));
                  lilv_instance_connect_port (instance, i, lv2_evbuf_get_buffer (plugin_ports[i].evbuf));

                  atom_in_ports.push_back (i);
                }
              else if (lilv_port_is_a (plugin, port, plugin_host.nodes.lv2_control_class))
                {
                  plugin_ports[i].control = defaults[i];      // start with default value
                  plugin_ports[i].type = Port::CONTROL_IN;
                  plugin_ports[i].min_value = min_values[i];
                  plugin_ports[i].max_value = max_values[i];

                  LilvNode *nname = lilv_port_get_name (plugin, port);
                  plugin_ports[i].name = lilv_node_as_string (nname);
                  lilv_node_free (nname);

                  const LilvNode *nsymbol = lilv_port_get_symbol (plugin, port);
                  plugin_ports[i].symbol = lilv_node_as_string (nsymbol);

                  lilv_instance_connect_port (instance, i, &plugin_ports[i].control);

                  n_control_ports++;
                }
              else
                {
                  printf ("found unknown input port\n");
                }
            }
          if (lilv_port_is_a (plugin, port, plugin_host.nodes.lv2_output_class))
            {
              if (lilv_port_is_a (plugin, port, plugin_host.nodes.lv2_audio_class))
                {
                  audio_out_ports.push_back (i);
                }
              else if (lilv_port_is_a (plugin, port, plugin_host.nodes.lv2_atom_class))
                {
                  atom_out_ports.push_back (i);

                  printf ("found atom output port\n");
                  const int buf_size = 4096;
                  plugin_ports[i].evbuf = lv2_evbuf_new (buf_size, LV2_EVBUF_ATOM, plugin_host.urid_map.urid_map (lilv_node_as_string (plugin_host.nodes.lv2_atom_Chunk)),
                                                                                   plugin_host.urid_map.urid_map (lilv_node_as_string (plugin_host.nodes.lv2_atom_Sequence)));
                  lilv_instance_connect_port (instance, i, lv2_evbuf_get_buffer (plugin_ports[i].evbuf));

                }
              else if (lilv_port_is_a (plugin, port, plugin_host.nodes.lv2_control_class))
                {
                  plugin_ports[i].control = defaults[i];      // start with default value
                  plugin_ports[i].type = Port::CONTROL_OUT;

                  lilv_instance_connect_port (instance, i, &plugin_ports[i].control);
                }
              else
                {
                  printf ("found unknown output port\n");
                }
            }
        }
    }

  printf ("--------------------------------------------------\n");
  printf ("audio IN:%zd OUT:%zd\n", audio_in_ports.size(), audio_out_ports.size());
  printf ("control IN:%zd\n", n_control_ports);
  printf ("--------------------------------------------------\n");
}

void
PluginInstance::init_presets()
{
  LilvNodes* lilv_presets = lilv_plugin_get_related (plugin, plugin_host.nodes.lv2_presets_Preset);
  LILV_FOREACH (nodes, i, lilv_presets)
    {
      const LilvNode* preset = lilv_nodes_get (lilv_presets, i);
      lilv_world_load_resource (plugin_host.world, preset);
      LilvNodes* labels = lilv_world_find_nodes (plugin_host.world, preset, plugin_host.nodes.rdfs_label, NULL);
      if (labels)
        {
          const LilvNode* label = lilv_nodes_get_first (labels);
          presets.push_back ({lilv_node_as_string (label), lilv_node_duplicate (preset)}); // TODO: preset leak
          lilv_nodes_free (labels);
        }
    }
  lilv_nodes_free (lilv_presets);
}

void
PluginInstance::write_midi (uint32_t time, size_t size, const uint8_t *data)
{
  if (!atom_in_ports.empty())
    {
      /* we use the first atom in port for midi, is there a better strategy? */
      int p = atom_in_ports[0];

      LV2_Evbuf           *evbuf = plugin_ports[p].evbuf;
      LV2_Evbuf_Iterator    iter = lv2_evbuf_end (evbuf);

      lv2_evbuf_write (&iter, time, 0, plugin_host.urids.midi_MidiEvent, size, data);
    }
}

void
PluginInstance::reset_event_buffers()
{
  for (int p : atom_out_ports)
    {
      /* Clear event output for plugin to write to */
      LV2_Evbuf *evbuf = plugin_ports[p].evbuf;

      lv2_evbuf_reset (evbuf, false);
    }
  for (int p : atom_in_ports)
    {
      LV2_Evbuf *evbuf = plugin_ports[p].evbuf;

      lv2_evbuf_reset (evbuf, true);
    }
}

void
PluginInstance::activate()
{
  lilv_instance_activate (instance);
}

void
PluginInstance::deactivate()
{
  lilv_instance_deactivate (instance);
}

void
PluginInstance::run (float *audio_out_l, float *audio_out_r, uint32_t nframes)
{
  float fake_in[nframes];

  // fake input: use on-stack buffer with zeros
  if (!audio_in_ports.empty())
    {
      std::fill_n (fake_in, nframes, 0.0);

      for (size_t i = 0; i < audio_in_ports.size(); i++)
        lilv_instance_connect_port (instance, audio_in_ports[i], fake_in);
    }
  const bool mono_plugin = audio_out_ports.size() == 1;
  const bool stereo_plugin = audio_out_ports.size() == 2;

  assert_return (mono_plugin || stereo_plugin);
  lilv_instance_connect_port (instance, audio_out_ports[0], audio_out_l);

  if (stereo_plugin)
    lilv_instance_connect_port (instance, audio_out_ports[1], audio_out_r);

  assert_return (atom_in_ports.size() == 1);

  lilv_instance_run (instance, nframes);

  // automatically convert mono -> stereo
  if (mono_plugin)
    std::copy (audio_out_l, audio_out_l + nframes, audio_out_r);
}

}

class LV2Device : public AudioSignal::Processor {
  OBusId stereo_out_;
  PluginInstance *plugin_instance;
  PluginHost plugin_host; // TODO: should be only one instance for all lv2 devices

  vector<Port *> param_id_port;
  int current_preset = 0;

  enum
    {
      PID_PRESET         = 1,
      PID_CONTROL_OFFSET = 10
    };

  void
  initialize () override
  {
    plugin_host.options.set (sample_rate(), /* FIXME: buffer size */ 128);
    const char *uri = getenv ("LV2URI");
    if (!uri)
      uri = "http://zynaddsubfx.sourceforge.net";
    plugin_instance = plugin_host.instantiate (uri, sample_rate());

    ChoiceEntries centries;
    centries += { "-none-" };
    for (auto preset : plugin_instance->presets)
      centries += { preset.name };
    add_param (PID_PRESET, "Device Preset", "Preset", std::move (centries), 0, "", "Device Preset to be used");
    current_preset = 0;

    int pid = PID_CONTROL_OFFSET;
    for (auto& port : plugin_instance->plugin_ports)
      if (port.type == Port::CONTROL_IN)
        {
          add_param (pid++, port.name, port.name, port.min_value, port.max_value, port.control);
          param_id_port.push_back (&port);
        }

    // TODO: deactivate?
    // TODO: is this the right place?
    plugin_instance->activate();
  }
  void
  query_info (ProcessorInfo &info) const override
  {
    info.uri = "Bse.LV2Device";
    // info.version = "0";
    info.label = "LV2Device";
    info.category = "Synth";
  }
  void
  configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override
  {
    remove_all_buses();
    prepare_event_input();
    stereo_out_ = add_output_bus ("Stereo Out", SpeakerArrangement::STEREO);
    assert_return (bus_info (stereo_out_).ident == "stereo-out");
  }
  void
  reset () override
  {
    adjust_params (true);
  }
  void
  adjust_param (Id32 tag) override
  {
    // controls for LV2Device
    if (int (tag) == PID_PRESET)
      {
        int want_preset = bse_ftoi (get_param (tag));
        if (current_preset != want_preset)
          {
            current_preset = want_preset;

            if (want_preset > 0 && want_preset <= int (plugin_instance->presets.size()))
              {
                // TODO: this should not be done in audio thread

                auto preset_info = plugin_instance->presets[want_preset - 1];
                printf ("load preset %s\n", preset_info.name.c_str());
                LilvState *state = lilv_state_new_from_world (plugin_host.world, plugin_host.urid_map.lv2_map(), preset_info.preset);
                const LV2_Feature* state_features[] = { // TODO: more features
                  plugin_host.urid_map.feature(),
                  NULL
                };
                lilv_state_restore (state, plugin_instance->instance, set_port_value, this, 0, state_features);
              }
          }
      }

    // real LV2 controls start at PID_CONTROL_OFFSET
    auto control_id = tag - PID_CONTROL_OFFSET;
    if (control_id >= 0 && control_id < param_id_port.size())
      param_id_port[control_id]->control = get_param (tag);
  }
  void
  render (uint n_frames) override
  {
    adjust_params (false);

    // reset event buffers and write midi events
    plugin_instance->reset_event_buffers();
    EventRange erange = get_event_input();

    for (const auto &ev : erange)
      {
        const int time_stamp = std::max<int> (ev.frame, 0);
        uint8_t midi_data[3] = { 0, };

        switch (ev.message())
          {
          case Message::NOTE_OFF:
            midi_data[0] = 0x80 | ev.channel;
            midi_data[1] = ev.key;
            plugin_instance->write_midi (time_stamp, 3, midi_data);
            break;
          case Message::NOTE_ON:
            midi_data[0] = 0x90 | ev.channel;
            midi_data[1] = ev.key;
            midi_data[2] = std::clamp (bse_ftoi (ev.velocity * 127), 0, 127);
            plugin_instance->write_midi (time_stamp, 3, midi_data);
            break;
#if 0
          case Message::ALL_NOTES_OFF:
          case Message::ALL_SOUND_OFF:
            synth_.all_sound_off();    // NOTE: there is no extra "all notes off" in liquidsfz
            break;
#endif
          default: ;
          }
      }

    float *output[2] = {
      oblock (stereo_out_, 0),
      oblock (stereo_out_, 1)
    };
    plugin_instance->run (output[0], output[1], n_frames);
  }
  void
  set_port_value (const char*         port_symbol,
                  const void*         value,
                  uint32_t            size,
                  uint32_t            type)
  {
    double dvalue = 0;
    if (type == plugin_host.urids.atom_Float)
      {
        dvalue = *(const float*)value;
      }
    else if (type == plugin_host.urids.atom_Double)
      {
        dvalue = *(const double*)value;
      }
    else if (type == plugin_host.urids.atom_Int)
      {
        dvalue = *(const int32_t*)value;
      }
    else if (type == plugin_host.urids.atom_Long)
      {
        dvalue = *(const int64_t*)value;
      }
    else
      {
        // TODO: unmap
#if 0
                fprintf(stderr, "error: Preset `%s' value has bad type <%s>\n",
                        port_symbol, jalv->unmap.unmap(jalv->unmap.handle, type));
#endif
        return;
      }
    printf ("%s = %f\n", port_symbol, dvalue);
    for (int i = 0; i < (int) param_id_port.size(); i++)
      {
        if (param_id_port[i]->symbol == port_symbol)
          {
            set_param (i + PID_CONTROL_OFFSET, dvalue);
          }
      }
  }

  static void
  set_port_value (const char*         port_symbol,
                  void*               user_data,
                  const void*         value,
                  uint32_t            size,
                  uint32_t            type)
  {
    LV2Device *dev = (LV2Device *) user_data;
    dev->set_port_value (port_symbol, value, size, type);
  }
};

static auto lv2device = Bse::enroll_asp<LV2Device>();

} // Bse
