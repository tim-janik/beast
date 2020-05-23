// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemain.hh"
#include "bsestartup.hh"
#include "bseserver.hh"
#include "bsesequencer.hh"
#include "bseplugin.hh"
#include "bsecategories.hh"
#include "bsemidireceiver.hh"
#include "bsemathsignal.hh"
#include "driver.hh"
#include "gsldatacache.hh"
#include "bseengine.hh"
#include "serializable.hh"
#include "bse/internal.hh"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

using namespace Bse;

/* --- prototypes --- */
namespace Bse {
static void     run_registered_driver_loaders();
static void     config_init (const StringVector &args);
} // Bse

/* --- variables --- */
static volatile int bse_initialization_stage = 0;

// == BSE Initialization ==
static std::thread async_bse_thread;

bool
_bse_initialized ()
{
  return async_bse_thread.get_id() != std::thread::id(); // has async_bse_thread started?
}

static void
init_sigpipe()
{
  // don't die if we write() data to a process and that process dies (i.e. jackd)
  sigset_t signal_mask;
  sigemptyset (&signal_mask);
  sigaddset (&signal_mask, SIGPIPE);

  int rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
  if (rc != 0)
    Bse::warning ("BSE: pthread_sigmask for SIGPIPE failed: %s\n", strerror (errno));
}

static void
initialize_with_args (const char *app_name, const Bse::StringVector &args)
{
  assert_return (_bse_initialized() == false);
  assert_return (bse_main_context == NULL);

  // argument handling
  config_init (args);

  // setup GLib's prgname for error messages
  if (auto exe = config_string ("exe"); !exe.empty() && !g_get_prgname())
    g_set_prgname (exe.c_str());
  if (!g_get_prgname() && app_name)
    g_set_prgname (app_name);

  // initialize SFI
  sfi_init();

  // SIGPIPE init: needs to be done before any child thread is created
  init_sigpipe();
}

static_assert (G_BYTE_ORDER == G_LITTLE_ENDIAN || G_BYTE_ORDER == G_BIG_ENDIAN, "");

static std::atomic<bool> main_loop_thread_running { true };

static void
bse_main_loop_thread (Bse::AsyncBlockingQueue<int> *init_queue)
{
  if (bse_initialization_stage != 0 || ++bse_initialization_stage != 1)
    {
      Bse::warning ("%s() may only be called once", __func__);
      return;
    }
  // register new thread
  const char *const myid = "BseMain";
  Bse::this_thread_set_name (myid);
  Bse::TaskRegistry::add (myid, Bse::this_thread_getpid(), Bse::this_thread_gettid());

  // main loop
  assert_return (bse_main_context == NULL);
  bse_main_context = g_main_context_new ();

  // basic components
  bse_globals_init ();
  bse_type_init ();
  bse_cxx_init ();
  // initialize GSL components
  gsl_init ();
  // remaining BSE components
  bse_plugin_init_builtins ();
  // initialize C wrappers around C++ generated types
  _bse_init_c_wrappers ();

  // make sure the server object is alive
  bse_server_get ();

  // load drivers
  run_registered_driver_loaders();

  // dump device list
  if (false) // dump_driver_list
    {
      Bse::Driver::EntryVec entries;
      printerr ("%s", _("\nAvailable PCM drivers:\n"));
      entries = Bse::PcmDriver::list_drivers();
      for (const auto &entry : entries)
        printerr ("  %-30s (%s, %08x)\n\t%s\n%s%s%s", entry.devid + ":",
                  entry.readonly ? "Input" : entry.writeonly ? "Output" : "Duplex",
                  entry.priority, entry.device_name,
                  entry.capabilities.empty() ? "" : "\t" + entry.capabilities + "\n",
                  entry.device_info.empty() ? "" : "\t" + entry.device_info + "\n",
                  entry.notice.empty() ? "" : "\t" + entry.notice + "\n");
      printerr ("%s", _("\nAvailable MIDI drivers:\n"));
      entries = Bse::MidiDriver::list_drivers();
      for (const auto &entry : entries)
        printerr ("  %-30s (%s, %08x)\n\t%s\n%s%s%s", entry.devid + ":",
                  entry.readonly ? "Input" : entry.writeonly ? "Output" : "Duplex",
                  entry.priority, entry.device_name,
                  entry.capabilities.empty() ? "" : "\t" + entry.capabilities + "\n",
                  entry.device_info.empty() ? "" : "\t" + entry.device_info + "\n",
                  entry.notice.empty() ? "" : "\t" + entry.notice + "\n");
    }

  // start other threads
  struct Internal : Bse::Sequencer { using Bse::Sequencer::_init_threaded; };
  Internal::_init_threaded();

  // complete initialization
  bse_initialization_stage++;   // = 2
  init_queue->push ('B');       // signal completion to caller
  init_queue = NULL;            // completion invalidates init_queue

  // main BSE thread event loop
  while (main_loop_thread_running)
    {
      g_main_context_pending (bse_main_context);
      if (main_loop_thread_running)
        g_main_context_iteration (bse_main_context, TRUE);
    }

  // close devices and shutdown engine threads
  bse_server_shutdown (bse_server_get());
  // process pending cleanups if needed, but avoid endless loops
  for (size_t i = 0; i < 1000; i++)
    if (g_main_context_pending (bse_main_context))
      g_main_context_iteration (bse_main_context, false);
    else
      break;

  // unregister new thread
  Bse::TaskRegistry::remove (Bse::this_thread_gettid());
}

static void
reap_main_loop_thread ()
{
  assert_return (main_loop_thread_running == true);
  main_loop_thread_running = false;
  bse_main_wakeup();
  async_bse_thread.join();
}

void
_bse_init_async (const char *app_name, const Bse::StringVector &args)
{
  initialize_with_args (app_name, args);

  // start main BSE thread
  if (std::atexit (reap_main_loop_thread) != 0)
    Bse::warning ("BSE: failed to install main thread reaper");
  auto *init_queue = new Bse::AsyncBlockingQueue<int>();
  async_bse_thread = std::thread (bse_main_loop_thread, init_queue);
  // wait for initialization completion of the core thread
  int msg = init_queue->pop();
  assert_return (msg == 'B');
  delete init_queue;
}

/// Wake up the event loop in the BSE thread.
void
bse_main_wakeup ()
{
  assert_return (bse_main_context != NULL);
  g_main_context_wakeup (bse_main_context);
}

static void
bse_main_enqueue (const std::function<void()> &func)
{
  assert_return (bse_main_context != NULL);
  using VFunc = std::function<void()>;
  struct CxxSource : GSource {
    VFunc func_;
  };
  auto prepare  = [] (GSource*, int*) -> gboolean { return true; };
  auto check    = [] (GSource*)       -> gboolean { return true; };
  auto dispatch = [] (GSource *source, GSourceFunc, void*) -> gboolean {
    CxxSource &cs = *(CxxSource*) source;
    cs.func_();
    return false; // run once
  };
  auto finalize = [] (GSource *source) {
    // this handler can be called from g_source_remove, or dispatch(), with or w/o main loop mutex...
    CxxSource &cs = *(CxxSource*) source;
    cs.func_.~VFunc();
  };
  static GSourceFuncs cxx_source_funcs = { prepare, check, dispatch, finalize };
  GSource *source = g_source_new (&cxx_source_funcs, sizeof (CxxSource));
  CxxSource &cs = *(CxxSource*) source;
  new (&cs.func_) VFunc (func);
  // g_source_set_priority (source, BSE_PRIORITY_GLUE); // g_source_new assigns G_PRIORITY_DEFAULT
  static_assert (BSE_PRIORITY_GLUE == G_PRIORITY_DEFAULT);
  g_source_attach (source, bse_main_context);
  g_source_unref (source);
}

namespace Bse {
void
JobQueue::call_remote (const std::function<void()> &job)
{
  Aida::ScopedSemaphore sem;
  std::function<void()> wrapper = [&sem, &job] () {
    job();
    sem.post();
  };
  bse_main_enqueue (wrapper);
  sem.wait();
}
JobQueue jobs;
} // Bse

// == parse args ==
static guint
get_n_processors (void)
{
#ifdef _SC_NPROCESSORS_ONLN
    gint n = sysconf (_SC_NPROCESSORS_ONLN);
    if (n > 0)
      return n;
#endif
  return 1;
}

namespace Bse {

// == Bse Configuration ==
using StringMap = std::unordered_map<String,String>;

static String
kv_split (const String &kvpair, String *valuep = nullptr)
{
  const char *const s = kvpair.c_str();
  const char *const eq = strchr (s, '=');
  if (eq)
    {
      if (valuep)
        *valuep = eq + 1;
      return std::string (s, eq - s);
    }
  if (valuep)
    *valuep = "";
  return "";
}

static StringMap *volatile global_config = nullptr;

/// Apply configuration upon BSE initialization.
static void
config_init (const StringVector &args)
{
  assert_return (global_config == nullptr);
  // map args to config_*()
  StringMap gconfig;
  for (const auto &kv : args)
    {
      String value;
      if (kv == "--") // should not occour
        break;
      else if (kv_split (kv, &value) == "fatal-warnings")
        gconfig["fatal-warnings"] = value;
      else if (kv_split (kv, &value) == "pcm-driver")
        gconfig["pcm-driver"] = value;
      else if (kv_split (kv, &value) == "midi-driver")
        gconfig["midi-driver"] = value;
      else if (kv_split (kv, &value) == "override-plugin-globs")
        gconfig["override-plugin-globs"] = value;
      else if (kv_split (kv, &value) == "override-sample-path")
        gconfig["override-sample-path"] = value;
      else if (kv_split (kv, &value) == "rcfile")
        gconfig["rcfile"] = value;
      else if (kv_split (kv, &value) == "allow-randomization")
        gconfig["allow-randomization"] = string_to_bool (value);
      else if (kv_split (kv, &value) == "stand-alone")
        gconfig["stand-alone"] = string_to_bool (value) ? "1" : "0";
      else if (kv_split (kv, &value) == "jobs")
        gconfig["jobs"] = string_from_int (string_to_int (value));
    }
  // apply config
  if (string_to_bool (gconfig["fatal-warnings"]))
    {
      Bse::set_debug_flags (Bse::DebugFlags::FATAL_WARNINGS);
      unsigned int flags = g_log_set_always_fatal (GLogLevelFlags (G_LOG_FATAL_MASK));
      g_log_set_always_fatal (GLogLevelFlags (flags | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL));
    }
  // sanitize settings
  if (string_to_int (gconfig["jobs"]) <= 0)
    gconfig["jobs"] = string_from_int (get_n_processors());
  if (const String r = gconfig["allow-randomization"]; r.empty())
    gconfig["allow-randomization"] = "1";
  // assign
  assert_return (global_config == nullptr);
  global_config = new StringMap (gconfig);
}

/// Retrive BSE configuration setting as string.
String
config_string (const String &key, const String &fallback)
{
  auto it = global_config->find (key);
  if (it != global_config->end())
    return it->second;
  return fallback;
}

/// Retrive BSE configuration setting as boolean.
bool
config_bool (const String &key, bool fallback)
{
  auto it = global_config->find (key);
  if (it != global_config->end())
    return string_to_bool (it->second, fallback);
  return fallback;
}

/// Retrive BSE configuration setting as integer.
int64
config_int (const String &key, int64 fallback)
{
  auto it = global_config->find (key);
  if (it != global_config->end())
    {
      size_t consumed = 0;
      const int64 i = string_to_int (it->second, &consumed);
      return consumed ? i : fallback;
    }
  return fallback;
}

// == Bse::GlobalPreferences ==
static Preferences   global_prefs_rcsettings;
static bool          global_prefs_dirty = false;
static size_t        global_prefs_stamp = 1;
static std::atomic<size_t> global_prefs_lockcount { 0 };

Preferences
GlobalPreferences::defaults ()
{
  Preferences prefs;
  // static defaults
  prefs.pcm_driver = config_string ("pcm-driver", "auto");
  prefs.synth_latency = 22;
  prefs.synth_mixing_freq = 48000;
  prefs.synth_control_freq = 1500;
  prefs.midi_driver = config_string ("midi-driver", "auto");
  prefs.invert_sustain = false;
  prefs.license_default = "Creative Commons Attribution-ShareAlike 4.0 (https://creativecommons.org/licenses/by-sa/4.0/)";
  // dynamic defaults
  const String default_user_path = Path::join (Path::user_home(), "Beast");
  prefs.effect_path     = default_user_path + "/Effects";
  prefs.instrument_path = default_user_path + "/Instruments";
  prefs.plugin_path     = default_user_path + "/Plugins";
  prefs.sample_path     = default_user_path + "/Samples";
  const char *user = g_get_user_name();
  if (user)
    {
      const char *name = g_get_real_name();
      if (name && name[0] && strcmp (user, name) != 0)
        prefs.author_default = name;
      else
        prefs.author_default = user;
    }
  return prefs;
}

static String
global_prefs_beastrc()
{
  const String defaultrc = Path::join (Path::config_home(), "beast", "bserc.xml");
  return config_string ("rcfile", defaultrc);
}

struct BseRc : public virtual Xms::SerializableInterface {
  Preferences config;
  String      config_tag = "Configuration";
  void
  xml_serialize (Xms::SerializationNode &xs) override
  {
    xs[config_tag] & config;
  }
};

static const Preferences&
global_prefs_load ()
{
  static bool loaded_once = false;
  if (!loaded_once)
    {
      Preferences config = GlobalPreferences::defaults();
      // load from rcfile
      const String xmltext = Path::stringread (global_prefs_beastrc());
      if (!xmltext.empty())
        {
          Preferences tmp = config;
          Xms::SerializationNode xs;
          if (Bse::Error::NONE == xs.parse_xml ("", xmltext)) // "BseRc" but allow "configuration"
            {
              BseRc rc;
              rc.config = config; // pre-load with defaults
              if (xs.name() == "BseRc")
                {
                  xs.load (rc);
                  config = rc.config;
                }
              else if (xs.name() == "configuration" && xs.has ("record")) // serialize.hh compat
                {
                  rc.config_tag = "record";
                  xs.load (rc);
                  config = rc.config;
                }
            }
        }
      loaded_once = true;
      global_prefs_rcsettings = config;
      global_prefs_stamp++;
    }
  return global_prefs_rcsettings;
}

void
GlobalPreferences::assign (const Preferences &preferences)
{
  if (global_prefs_rcsettings == preferences ||
      GlobalPreferences::locked ())
    return;
  global_prefs_rcsettings = preferences;
  global_prefs_stamp++;
  if (!global_prefs_dirty)
    {
      exec_timeout (GlobalPreferences::flush, 500);
      global_prefs_dirty = true;
    }
}

void
GlobalPreferences::flush ()
{
  if (global_prefs_dirty)
    {
      BseRc rc;
      rc.config = global_prefs_rcsettings;
      Xms::SerializationNode xs;
      xs.save (rc);
      Path::stringwrite (global_prefs_beastrc(), xs.write_xml ("BseRc"), true);
      global_prefs_dirty = false;
    }
}

void
GlobalPreferences::lock ()
{
  global_prefs_lockcount += 1;
}

void
GlobalPreferences::unlock ()
{
  assert_return (global_prefs_lockcount > 0);
  global_prefs_lockcount -= 1;
}

bool
GlobalPreferences::locked ()
{
  return global_prefs_lockcount > 0;
}

const GlobalPreferences*
GlobalPreferencesPtr::operator-> () const
{
  const Preferences &config = global_prefs_load();
  return static_cast<const GlobalPreferences*> (&config);
}

// == loaders ==
using RegisteredLoaderFunc = Error (*) ();
struct RegisteredLoader {
  const char *const what;
  const RegisteredLoaderFunc func;
};
using RegisteredLoaderVector = std::vector<RegisteredLoader>;
static RegisteredLoaderVector& registered_loaders() { static RegisteredLoaderVector lv; return lv; }
static bool registered_loaders_executed = false;

///< Register loader callbacks at static constructor time.
bool*
register_driver_loader (const char *staticwhat, Error (*loader) ())
{
  assert_return (loader != NULL, nullptr);
  assert_return (registered_loaders_executed == false, nullptr);
  RegisteredLoaderVector &lv = registered_loaders();
  lv.push_back ({ staticwhat, loader });
  return &registered_loaders_executed;
}

static void
run_registered_driver_loaders()
{
  assert_return (registered_loaders_executed == false);
  registered_loaders_executed = true;
  for (auto &loader : registered_loaders())
    {
      Error error = loader.func ();
      if (error != Error::NONE)
        printerr ("BSE: %s: loading failed: %s\n", loader.what, bse_error_blurb (error));
    }
}

} // Bse
