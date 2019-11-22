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
#include "bseblockutils.hh" /* bse_block_impl_name() */
#include "serializable.hh"
#include "bse/internal.hh"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <bse/testing.hh>

using namespace Bse;

/* --- prototypes --- */
static void	init_parse_args	(int *argc_p, char **argv_p, BseMainArgs *margs, const Bse::StringVector &args);
namespace Bse {
static void     run_registered_driver_loaders();
} // Bse

/* --- variables --- */
/* from bse.hh */
static volatile gboolean bse_initialization_stage = 0;
static BseMainArgs       default_main_args = {
  "auto",               // pcm_driver
  "auto",               // midi_driver;
  NULL,                 // override_plugin_globs
  NULL,			// override_sample_path
  1,                    // n_processors
  64,                   // wave_chunk_padding
  256,                  // wave_chunk_big_pad
  4000,                 // dcache_block_size
  10 * 1024 * 1024,     // dcache_cache_memory
  BSE_KAMMER_NOTE,      // midi_kammer_note (69)
  BSE_KAMMER_FREQUENCY, // kammer_freq (440Hz, historically 435Hz)
  false,                // stand_alone
  true,                 // allow_randomization
  false,                // force_fpu
  true,                 // load_drivers
  false,                // debug_extensions
  false,                // dump_driver_list
};
BseMainArgs             *bse_main_args = NULL;

// == BSE Initialization ==
static int initialized_for_unit_testing = -1;
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
initialize_with_argv (int *argc, char **argv, const char *app_name, const Bse::StringVector &args)
{
  assert_return (_bse_initialized() == false);
  assert_return (bse_main_context == NULL);

  // setup GLib's prgname for error messages
  if (argc && argv && *argc && !g_get_prgname ())
    g_set_prgname (*argv);

  // argument handling
  bse_main_args = &default_main_args;
  init_parse_args (argc, argv, bse_main_args, args);

  // initialize SFI
  if (initialized_for_unit_testing > 0)
    Bse::Test::init (argc, argv);
  else
    sfi_init (argc, argv);

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
  if (bse_main_args->load_drivers)
    run_registered_driver_loaders();

  // dump device list
  if (bse_main_args->dump_driver_list)
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

  // unit testing message
  if (initialized_for_unit_testing > 0)
    {
      StringVector sv = Bse::string_split (Bse::cpu_info(), " ");
      String machine = sv.size() >= 2 ? sv[1] : "Unknown";
      TNOTE ("Running on: %s+%s", machine.c_str(), bse_block_impl_name());
    }

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
_bse_init_async (int *argc, char **argv, const char *app_name, const Bse::StringVector &args)
{
  assert_return (argc && argv);
  initialize_with_argv (argc, argv, app_name, args);

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

int
bse_init_and_test (int *argc, char **argv, const std::function<int()> &bsetester, const Bse::StringVector &args)
{
  // initialize
  assert_return (initialized_for_unit_testing < 0, -128);
  initialized_for_unit_testing = 1;
  _bse_init_async (argc, argv, NULL, args);
  // run tests
  Aida::ScopedSemaphore sem;
  int retval = -128;
  std::function<void()> wrapper = [&sem, &bsetester, &retval] () {
    retval = bsetester();
    sem.post();
  };
  bse_main_enqueue (wrapper);
  sem.wait();
  return retval;
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
static String argv_bse_rcfile;

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

static bool
parse_bool_option (const String &s, const char *arg, bool *boolp)
{
  const size_t length = strlen (arg);
  if (s.size() > length && s[length] == '=' && strncmp (&s[0], arg, length) == 0)
    {
      *boolp = string_to_bool (s.substr (length + 1));
      return true;
    }
  return false;
}

static bool
parse_int_option (const String &s, const char *arg, int64 *ip)
{
  const size_t length = strlen (arg);
  if (s.size() > length && s[length] == '=' && strncmp (&s[0], arg, length) == 0)
    {
      *ip = string_to_int (s.substr (length + 1));
      return true;
    }
  return false;
}

static bool
parse_float_option (const String &s, const char *arg, double *fp)
{
  const size_t length = strlen (arg);
  if (s.size() > length && s[length] == '=' && strncmp (&s[0], arg, length) == 0)
    {
      *fp = string_to_float (s.substr (length + 1));
      return true;
    }
  return false;
}

static void
init_parse_args (int *argc_p, char **argv_p, BseMainArgs *margs, const Bse::StringVector &args)
{
  uint argc = *argc_p;
  char **argv = argv_p;
  /* this function is called before the main BSE thread is started,
   * so we can't use any BSE functions yet.
   */
  uint i;
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
	{
	  GLogLevelFlags fatal_mask = (GLogLevelFlags) g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK);
	  fatal_mask = (GLogLevelFlags) (fatal_mask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
	  g_log_set_always_fatal (fatal_mask);
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-driver-list", argv[i]) == 0)
	{
          margs->load_drivers = true;
          margs->dump_driver_list = true;
	  argv[i] = nullptr;
	}
      else if (strcmp ("--bse-pcm-driver", argv[i]) == 0)
	{
          if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
              margs->pcm_driver = argv[i];
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-midi-driver", argv[i]) == 0)
	{
          if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
              margs->midi_driver = argv[i];
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-override-plugin-globs", argv[i]) == 0 && i + 1 < argc)
	{
          argv[i++] = NULL;
          margs->override_plugin_globs = argv[i];
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-override-sample-path", argv[i]) == 0 && i + 1 < argc)
	{
	  argv[i++] = NULL;
	  margs->override_sample_path = argv[i];
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-rcfile", argv[i]) == 0 && i + 1 < argc)
	{
          argv[i++] = NULL;
          argv_bse_rcfile = argv[i];
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-force-fpu", argv[i]) == 0)
	{
          margs->force_fpu = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-disable-randomization", argv[i]) == 0)
	{
          margs->allow_randomization = FALSE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-enable-randomization", argv[i]) == 0)
	{
          margs->allow_randomization = TRUE;
	  argv[i] = NULL;
	}
    }

  if (*argc_p > 1)
    {
      uint e = 1;
      for (i = 1; i < argc; i++)
        if (argv[i])
          {
            argv[e++] = argv[i];
            if (i >= e)
              argv[i] = NULL;
          }
      *argc_p = e;
    }
  for (auto arg : args)
    {
      bool b; double d; int64 i;
      if      (parse_bool_option (arg, "stand-alone", &b))
        margs->stand_alone |= b;
      else if (parse_bool_option (arg, "allow-randomization", &b))
        margs->allow_randomization |= b;
      else if (parse_bool_option (arg, "force-fpu", &b))
        margs->force_fpu |= b;
      else if (parse_bool_option (arg, "load-drivers", &b))
        margs->load_drivers = b;
      else if (parse_bool_option (arg, "debug-extensions", &b))
        margs->debug_extensions |= b;
      else if (parse_int_option (arg, "wave-chunk-padding", &i))
        margs->wave_chunk_padding = i;
      else if (parse_int_option (arg, "wave-chunk-big-pad", &i))
        margs->wave_chunk_big_pad = i;
      else if (parse_int_option (arg, "dcache-cache-memory", &i))
        margs->dcache_cache_memory = i;
      else if (parse_int_option (arg, "dcache-block-size", &i))
        margs->dcache_block_size = i;
      else if (parse_int_option (arg, "midi-kammer-note", &i))
        margs->midi_kammer_note = i;
      else if (parse_float_option (arg, "kammer-freq", &d))
        margs->kammer_freq = d;
    }

  /* constrain (user) config */
  margs->wave_chunk_padding = MAX (1, margs->wave_chunk_padding);
  margs->wave_chunk_big_pad = MAX (2 * margs->wave_chunk_padding, margs->wave_chunk_big_pad);
  margs->dcache_block_size = MAX (2 * margs->wave_chunk_big_pad + sizeof (((GslDataCacheNode*) NULL)->data[0]), margs->dcache_block_size);
  margs->dcache_block_size = sfi_alloc_upper_power2 (margs->dcache_block_size - 1);
  /* margs->dcache_cache_memory = sfi_alloc_upper_power2 (margs->dcache_cache_memory); */

  /* non-configurable config updates */
  margs->n_processors = get_n_processors ();
}

namespace Bse {

// == Bse::GlobalConfig ==
static Configuration global_config_rcsettings;
static bool          global_config_dirty = false;
static size_t        global_config_stamp = 1;
static std::atomic<size_t> global_config_lockcount { 0 };

Configuration
GlobalConfig::defaults ()
{
  Configuration config;
  // static defaults
  config.pcm_driver = bse_main_args->pcm_driver;
  config.synth_latency = 50;
  config.synth_mixing_freq = 48000;
  config.synth_control_freq = 1000;
  config.midi_driver = bse_main_args->midi_driver;
  config.invert_sustain = false;
  config.license_default = "Creative Commons Attribution-ShareAlike 4.0 (https://creativecommons.org/licenses/by-sa/4.0/)";
  // dynamic defaults
  const String default_user_path = Path::join (Path::user_home(), "Beast");
  config.effect_path     = default_user_path + "/Effects";
  config.instrument_path = default_user_path + "/Instruments";
  config.plugin_path     = default_user_path + "/Plugins";
  config.sample_path     = default_user_path + "/Samples";
  const char *user = g_get_user_name();
  if (user)
    {
      const char *name = g_get_real_name();
      if (name && name[0] && strcmp (user, name) != 0)
        config.author_default = name;
      else
        config.author_default = user;
    }
  return config;
}

static String
global_config_beastrc()
{
  return argv_bse_rcfile.empty() ? Path::join (Path::config_home(), "beast", "bserc.xml") : argv_bse_rcfile;
}

struct BseRc : public virtual Xms::SerializableInterface {
  Configuration config;
  String        config_tag = "Configuration";
  void
  xml_serialize (Xms::SerializationNode &xs) override
  {
    xs[config_tag] & config;
  }
};

static const Configuration&
global_config_load ()
{
  static bool loaded_once = false;
  if (!loaded_once)
    {
      Configuration config = GlobalConfig::defaults();
      // load from rcfile
      const String xmltext = Path::stringread (global_config_beastrc());
      if (!xmltext.empty())
        {
          Configuration tmp = config;
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
      global_config_rcsettings = config;
      global_config_stamp++;
    }
  return global_config_rcsettings;
}

void
GlobalConfig::assign (const Configuration &configuration)
{
  if (global_config_rcsettings == configuration ||
      GlobalConfig::locked ())
    return;
  global_config_rcsettings = configuration;
  global_config_stamp++;
  if (!global_config_dirty)
    {
      exec_timeout (GlobalConfig::flush, 500);
      global_config_dirty = true;
    }
}

void
GlobalConfig::flush ()
{
  if (global_config_dirty)
    {
      BseRc rc;
      rc.config = global_config_rcsettings;
      Xms::SerializationNode xs;
      xs.save (rc);
      Path::stringwrite (global_config_beastrc(), xs.write_xml ("BseRc"), true);
      global_config_dirty = false;
    }
}

void
GlobalConfig::lock ()
{
  global_config_lockcount += 1;
}

void
GlobalConfig::unlock ()
{
  assert_return (global_config_lockcount > 0);
  global_config_lockcount -= 1;
}

bool
GlobalConfig::locked ()
{
  return global_config_lockcount > 0;
}

const GlobalConfig*
GlobalConfigPtr::operator-> () const
{
  const Configuration &config = global_config_load();
  return static_cast<const GlobalConfig*> (&config);
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
