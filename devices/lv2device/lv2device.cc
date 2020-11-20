// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bse/processor.hh"
#include "bse/signalmath.hh"
#include "bse/internal.hh"
#include "bse/bseengine.hh"
#include "aidacc/aida.hh"

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/parameters/parameters.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/presets/presets.h"

#include "lv2_evbuf.h"
#include "ringbuffer.hh"

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
  map<LV2_URID, String> m_urid_unmap;

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

    m_urid_unmap[id] = str;
    printf ("map %s -> %d\n", str, id);
    return id;
  }
  const char *
  urid_unmap (LV2_URID id)
  {
    auto it = m_urid_unmap.find (id);
    if (it != m_urid_unmap.end())
      return it->second.c_str();
    else
      return nullptr;
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

  const LV2_Worker_Interface *worker_interface = nullptr;
  LV2_Handle                  instance = nullptr;
  RingBuffer<uint8>           work_buffer_;
  RingBuffer<uint8>           response_buffer_;
  std::thread                 thread_;
  std::atomic<int>            quit_;
  Aida::ScopedSemaphore       sem_;
public:
  Worker() :
    lv2_worker_sched { this, schedule },
    lv2_worker_feature { LV2_WORKER__schedule, &lv2_worker_sched },
    work_buffer_ (4096),
    response_buffer_ (4096),
    quit_ (0)
  {
    thread_ = std::thread (&Worker::run, this);
  }
  void
  stop()
  {
    quit_ = 1;
    sem_.post();
    thread_.join();
    printf ("worker thread joined\n");
  }

  void
  set_instance (LilvInstance *lilv_instance)
  {
    instance = lilv_instance_get_handle (lilv_instance);

    const LV2_Descriptor *descriptor = lilv_instance_get_descriptor (lilv_instance);
    if (descriptor && descriptor->extension_data)
       worker_interface = (const LV2_Worker_Interface *) (*descriptor->extension_data) (LV2_WORKER__interface);
  }

  void
  run()
  {
    while (!quit_)
      {
        sem_.wait();
        while (work_buffer_.get_readable_values())
          {
            uint32 size;
            work_buffer_.read (sizeof (size), (uint8 *) &size);
            uint8 data[size];
            work_buffer_.read (size, data);

            printf ("got work %d bytes\n", size);
            worker_interface->work (instance, respond, this, size, data);
          }
      }
  }

  LV2_Worker_Status
  send_data (RingBuffer<uint8>& ring_buffer, uint32_t size, const void *data)
  {
    const uint32 n_values = sizeof (size) + size;
    if (n_values <= ring_buffer.get_writable_values())
      {
        uint8 to_write[n_values];
        memcpy (to_write, &size, sizeof (size));
        memcpy (to_write + sizeof (size), data, size);

        ring_buffer.write (n_values, to_write);
        return LV2_WORKER_SUCCESS;
      }
    else
      {
        return LV2_WORKER_ERR_NO_SPACE;
      }
  }
  LV2_Worker_Status
  schedule (uint32_t size, const void *data)
  {
    if (!worker_interface)
      return LV2_WORKER_ERR_UNKNOWN;

    auto rc = send_data (work_buffer_, size, data);
    sem_.post();
    return rc;
  }
  LV2_Worker_Status
  respond (uint32_t size, const void *data)
  {
    if (!worker_interface)
      return LV2_WORKER_ERR_UNKNOWN;

    printf ("queue work response\n");
    return send_data (response_buffer_, size, data);
  }
  void
  handle_responses()
  {
    while (response_buffer_.get_readable_values())
      {
        uint32 size;
        response_buffer_.read (sizeof (size), (uint8 *) &size);
        uint8 data[size];
        response_buffer_.read (size, data);

        printf ("got work response %d bytes\n", size);
        worker_interface->work_response (instance, size, data);
      }
  }
  void
  end_run()
  {
    /* to be called after each run cycle */
    if (worker_interface && worker_interface->end_run)
      worker_interface->end_run (instance);
  }
  static LV2_Worker_Status
  schedule (LV2_Worker_Schedule_Handle handle,
            uint32_t                   size,
            const void*                data)
  {
    Worker *worker = static_cast<Worker *> (handle);
    return worker->schedule (size, data);
  }
  static LV2_Worker_Status
  respond  (LV2_Worker_Respond_Handle handle,
            uint32_t                  size,
            const void*               data)
  {
    Worker *worker = static_cast<Worker *> (handle);
    return worker->respond (size, data);
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

  Features features;

  Worker   worker;

  PluginInstance (PluginHost& plugin_host);
  ~PluginInstance();

  const LilvPlugin             *plugin = nullptr;
  LilvInstance                 *instance = nullptr;
  const LV2_Worker_Interface   *worker_interface = nullptr;
  std::vector<Port>             plugin_ports;
  std::vector<int>              atom_out_ports;
  std::vector<int>              atom_in_ports;
  std::vector<int>              audio_in_ports;
  std::vector<int>              audio_out_ports;
  std::vector<PresetInfo>       presets;
  bool                          active = false;

  void init_ports();
  void init_presets();
  void reset_event_buffers();
  void write_midi (uint32_t time, size_t size, const uint8_t *data);
  void connect_audio_port (uint32_t port, float *buffer);
  void run (uint32_t nframes);
  void activate();
  void deactivate();
};

struct PluginHost
{
  LilvWorld  *world;
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

  Options  options;

  PluginHost() :
    world (nullptr),
    urids (urid_map),
    options (*this)
  {
    world = lilv_world_new();
    lilv_world_load_all (world);

    nodes.init (world);
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

  PluginInstance *plugin_instance = new PluginInstance (*this);

  LilvInstance *instance = lilv_plugin_instantiate (plugin, mix_freq, plugin_instance->features.get_features());
  if (!instance)
    {
      fprintf (stderr, "plugin instantiate failed\n");
      delete plugin_instance;

      return nullptr;
    }

  plugin_instance->instance = instance;
  plugin_instance->plugin = plugin;
  plugin_instance->init_ports();
  plugin_instance->init_presets();
  plugin_instance->worker.set_instance (instance);

  return plugin_instance;
}

PluginInstance::PluginInstance (PluginHost& plugin_host) :
  plugin_host (plugin_host)
{
  features.add (plugin_host.urid_map.feature());
  features.add (worker.feature());
  features.add (plugin_host.options.feature()); /* TODO: maybe make a local version */
}

PluginInstance::~PluginInstance()
{
  worker.stop();

  if (instance)
    {
      if (active)
        deactivate();

      lilv_instance_free (instance);
      instance = nullptr;
    }
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
  if (!active)
    {
      printf ("activate\n");
      lilv_instance_activate (instance);

      active = true;
    }
}

void
PluginInstance::deactivate()
{
  if (active)
    {
      printf ("deactivate\n");
      lilv_instance_deactivate (instance);

      active = false;
    }
}

void
PluginInstance::connect_audio_port (uint32_t port, float *buffer)
{
  lilv_instance_connect_port (instance, port, buffer);
}

void
PluginInstance::run (uint32_t nframes)
{
  lilv_instance_run (instance, nframes);

  worker.handle_responses();
  worker.end_run();
}

}

class LV2Device : public AudioSignal::Processor {
  IBusId stereo_in_;
  OBusId stereo_out_;
  vector<IBusId> mono_ins_;
  vector<OBusId> mono_outs_;

  PluginInstance *plugin_instance;
  PluginHost plugin_host; // TODO: should be only one instance for all lv2 devices

  vector<Port *> param_id_port;
  int current_preset = 0;

  enum
    {
      PID_PRESET         = 1,
      PID_DELETE         = 2,
      PID_CONTROL_OFFSET = 10
    };

  void
  initialize () override
  {
    plugin_host.options.set (sample_rate(), BSE_ENGINE_MAX_BLOCK_SIZE);
    const char *uri = getenv ("LV2URI");
    if (!uri)
      uri = "http://zynaddsubfx.sourceforge.net";

    plugin_instance = plugin_host.instantiate (uri, sample_rate());
    if (!plugin_instance)
      return;

    if (plugin_instance->presets.size()) /* choice with 1 entry will crash */
      {
        ChoiceEntries centries;
        centries += { "-none-" };
        for (auto preset : plugin_instance->presets)
          centries += { preset.name };
        add_param (PID_PRESET, "Device Preset", "Preset", std::move (centries), 0, "", "Device Preset to be used");
      }
    current_preset = 0;

    add_param (PID_DELETE, "Test Delete", "TestDel", false);

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
  configure (uint n_ibusses, const SpeakerArrangement *ibusses, uint n_obusses, const SpeakerArrangement *obusses) override {
    if (!plugin_instance)
      return;

    remove_all_buses();
    prepare_event_input();

    /* map audio inputs/outputs to busses;
     *
     *   channels == 1 -> one mono bus
     *   channels == 2 -> one stereo bus
     *   channels >= 3 -> N mono busses (TODO: is this the best mapping for all plugins?)
     */
    mono_ins_.clear();
    mono_outs_.clear();
    if (plugin_instance->audio_in_ports.size() == 2)
      {
        stereo_in_ = add_input_bus ("Stereo In", SpeakerArrangement::STEREO);
        assert_return (bus_info (stereo_in_).ident == "stereo-in");
      }
    else
      {
        for (size_t i = 0; i < plugin_instance->audio_in_ports.size(); i++)
          mono_ins_.push_back (add_input_bus (string_format ("Mono In %zd", i + 1), SpeakerArrangement::MONO));
      }

    if (plugin_instance->audio_out_ports.size() == 2)
      {
        stereo_out_ = add_output_bus ("Stereo Out", SpeakerArrangement::STEREO);
        assert_return (bus_info (stereo_out_).ident == "stereo-out");
      }
    else
      {
        for (size_t i = 0; i < plugin_instance->audio_out_ports.size(); i++)
          mono_outs_.push_back (add_output_bus (string_format ("Mono Out %zd", i + 1), SpeakerArrangement::MONO));
      }
  }
  void
  reset () override
  {
    if (!plugin_instance)
      return;

    adjust_params (true);
  }
  void
  adjust_param (Id32 tag) override
  {
    if (!plugin_instance)
      return;

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
    if (int (tag) == PID_DELETE && get_param (tag) > 0.5) // this is just test code
      {
        delete plugin_instance;
        plugin_instance = nullptr;
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

    if (!plugin_instance)
      {
        if (plugin_instance->audio_out_ports.size() == 2)
          {
            bse_block_fill_0 (n_frames, oblock (stereo_out_, 0));
            bse_block_fill_0 (n_frames, oblock (stereo_out_, 1));
          }
        else
          {
            for (size_t i = 0; i < plugin_instance->audio_out_ports.size(); i++)
              bse_block_fill_0 (n_frames, oblock (mono_outs_[i], 0));
          }
        return;
      }

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

    if (plugin_instance->audio_in_ports.size() == 2)
      {
        plugin_instance->connect_audio_port (plugin_instance->audio_in_ports[0], const_cast<float *> (ifloats (stereo_in_, 0)));
        plugin_instance->connect_audio_port (plugin_instance->audio_in_ports[1], const_cast<float *> (ifloats (stereo_in_, 1)));
      }
    else
      {
        for (size_t i = 0; i < plugin_instance->audio_in_ports.size(); i++)
          plugin_instance->connect_audio_port (plugin_instance->audio_in_ports[i], const_cast<float *> (ifloats (mono_ins_[i], 0)));
      }

    if (plugin_instance->audio_out_ports.size() == 2)
      {
        plugin_instance->connect_audio_port (plugin_instance->audio_out_ports[0], oblock (stereo_out_, 0));
        plugin_instance->connect_audio_port (plugin_instance->audio_out_ports[1], oblock (stereo_out_, 1));
      }
    else
      {
        for (size_t i = 0; i < plugin_instance->audio_out_ports.size(); i++)
          plugin_instance->connect_audio_port (plugin_instance->audio_out_ports[i], oblock (mono_outs_[i], 0));
      }
    plugin_instance->run (n_frames);
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
        fprintf (stderr, "error: Preset `%s' value has bad type <%s>\n",
                          port_symbol, plugin_instance->plugin_host.urid_map.urid_unmap (type));
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
