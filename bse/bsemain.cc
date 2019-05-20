// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsemain.hh"
#include "bsestartup.hh"
#include "bseserver.hh"
#include "bsesequencer.hh"
#include "bseplugin.hh"
#include "bsecategories.hh"
#include "bsemidireceiver.hh"
#include "bsemathsignal.hh"
#include "gsldatacache.hh"
#include "bsepcmdevice.hh"
#include "bsemididevice.hh"
#include "bseengine.hh"
#include "bseblockutils.hh" /* bse_block_impl_name() */
#include "bseglue.hh"
#include "serialize.hh"
#include "bse/internal.hh"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <bse/testing.hh>

using namespace Bse;

/* --- prototypes --- */
static void	init_parse_args	(int *argc_p, char **argv_p, BseMainArgs *margs, const Bse::StringVector &args);
static void     attach_execution_context (GMainContext *gmaincontext, Aida::ExecutionContext *execution_context);

/* --- variables --- */
/* from bse.hh */
static volatile gboolean bse_initialization_stage = 0;
static BseMainArgs       default_main_args = {
  1,                    // n_processors
  64,                   // wave_chunk_padding
  256,                  // wave_chunk_big_pad
  4000,                 // dcache_block_size
  10 * 1024 * 1024,     // dcache_cache_memory
  BSE_KAMMER_NOTE,      // midi_kammer_note (69)
  BSE_KAMMER_FREQUENCY, // kammer_freq (440Hz, historically 435Hz)
  NULL,                 // override_plugin_globs
  NULL,			// override_sample_path
  false,                // stand_alone
  true,                 // allow_randomization
  false,                // force_fpu
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
initialize_with_argv (int *argc, char **argv, const char *app_name, const Bse::StringVector &args)
{
  assert_return (_bse_initialized() == false);
  assert_return (bse_main_context == NULL);

  // setup GLib's prgname for error messages
  if (argc && argv && *argc && !g_get_prgname ())
    g_set_prgname (*argv);

  // argument handling
  bse_main_args = &default_main_args;
  if (argc && argv)
    init_parse_args (argc, argv, bse_main_args, args);

  // initialize SFI
  if (initialized_for_unit_testing > 0)
    Bse::Test::init (argc, argv);
  else
    sfi_init (argc, argv);
}

static_assert (G_BYTE_ORDER == G_LITTLE_ENDIAN || G_BYTE_ORDER == G_BIG_ENDIAN, "");

static Aida::ExecutionContext *bse_execution_context = NULL;
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
  _bse_init_signal();
  bse_type_init ();
  bse_cxx_init ();
  // FIXME: global spawn dir is evil
  {
    gchar *dir = g_get_current_dir ();
    sfi_com_set_spawn_dir (dir);
    g_free (dir);
  }
  // initialize GSL components
  gsl_init ();
  // remaining BSE components
  bse_plugin_init_builtins ();
  // initialize C wrappers around C++ generated types
  _bse_init_c_wrappers ();

  // make sure the server object is alive
  bse_server_get ();

  // load drivers
  if (bse_main_args->load_drivers_early)
    {
      SfiRing *ring = bse_plugin_path_list_files (TRUE, FALSE);
      while (ring)
        {
          gchar *name = (char*) sfi_ring_pop_head (&ring);
          const char *error = bse_plugin_check_load (name);
          if (error)
            Bse::info ("while loading \"%s\": %s", name, error);
          g_free (name);
        }
    }

  // dump device list
  if (bse_main_args->dump_driver_list)
    {
      printerr ("%s", _("\nAvailable PCM drivers:\n"));
      bse_device_dump_list (BSE_TYPE_PCM_DEVICE, "  ", TRUE, NULL, NULL);
      printerr ("%s", _("\nAvailable MIDI drivers:\n"));
      bse_device_dump_list (BSE_TYPE_MIDI_DEVICE, "  ", TRUE, NULL, NULL);
    }

  // initialize core plugins
  if (bse_main_args->load_core_plugins)
    {
      auto server_registration = [] (SfiProxy server, BseRegistrationType rtype, const char *what, const char *error, void *data) {
        if (rtype != BSE_REGISTER_DONE && error && error[0])
          Bse::info ("failed to register \"%s\": %s", what, error);
      };
      g_object_connect (bse_server_get(), "signal::registration", server_registration, NULL, NULL);
      SfiRing *ring = bse_plugin_path_list_files (!bse_main_args->load_drivers_early, TRUE);
      while (ring)
        {
          gchar *name = (char*) sfi_ring_pop_head (&ring);
          const char *error = bse_plugin_check_load (name);
          if (error)
            Bse::info ("while loading \"%s\": %s", name, error);
          g_free (name);
        }
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

  // enable handling of remote calls
  assert_return (bse_execution_context == NULL);
  bse_execution_context = Aida::ExecutionContext::new_context();
  attach_execution_context (bse_main_context, bse_execution_context);
  bse_execution_context->push_thread_current();

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
  // process pending cleanups if needed
  while (g_main_context_pending (bse_main_context))
    g_main_context_iteration (bse_main_context, false);

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

/// Attach execution_context to a GLib main loop context.
static void
attach_execution_context (GMainContext *gmaincontext, Aida::ExecutionContext *execution_context)
{
  GSource *gsource = execution_context->create_gsource ("BSE::ExecutionContext", BSE_PRIORITY_GLUE);
  g_source_attach (gsource, gmaincontext);
  g_source_unref (gsource);
}

struct AsyncData {
  const gchar *client;
  const std::function<void()> &caller_wakeup;
  Bse::AsyncBlockingQueue<SfiGlueContext*> result_queue;
};

static gboolean
async_create_context (gpointer data)
{
  AsyncData *adata = (AsyncData*) data;
  SfiComPort *port1, *port2;
  sfi_com_port_create_linked ("Client", adata->caller_wakeup, &port1,
			      "Server", bse_main_wakeup, &port2);
  SfiGlueContext *context = sfi_glue_encoder_context (port1);
  bse_glue_setup_dispatcher (port2);
  adata->result_queue.push (context);
  return false; // run-once
}

SfiGlueContext*
_bse_glue_context_create (const char *client, const std::function<void()> &caller_wakeup)
{
  assert_return (client && caller_wakeup, NULL);
  AsyncData adata = { client, caller_wakeup };
  // function runs in user threads and queues handler in BSE thread to create context
  if (bse_initialization_stage < 2)
    {
      Bse::warning ("%s: called without prior %s()", __func__, "Bse::init_async");
      return NULL;
    }
  // queue handler to create context
  GSource *source = g_idle_source_new ();
  g_source_set_priority (source, G_PRIORITY_HIGH);
  adata.client = client;
  g_source_set_callback (source, async_create_context, &adata, NULL);
  g_source_attach (source, bse_main_context);
  g_source_unref (source);
  // wake up BSE thread
  g_main_context_wakeup (bse_main_context);
  // receive result asynchronously
  SfiGlueContext *context = adata.result_queue.pop();
  return context;
}

void
bse_main_wakeup ()
{
  assert_return (bse_main_context != NULL);
  g_main_context_wakeup (bse_main_context);
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
  bse_execution_context->enqueue_mt (wrapper);
  sem.wait();
  return retval;
}

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
  guint i;
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
          margs->load_drivers_early = TRUE;
          margs->dump_driver_list = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-pcm-driver", argv[i]) == 0)
	{
          if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
              margs->pcm_drivers = sfi_ring_append (margs->pcm_drivers, argv[i]);
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--bse-midi-driver", argv[i]) == 0)
	{
          if (i + 1 < argc)
	    {
	      argv[i++] = NULL;
              margs->midi_drivers = sfi_ring_append (margs->midi_drivers, argv[i]);
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

  guint e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
  for (auto arg : args)
    {
      bool b; double d; int64 i;
      if      (parse_bool_option (arg, "stand-alone", &b))
        margs->stand_alone |= b;
      else if (parse_bool_option (arg, "allow-randomization", &b))
        margs->allow_randomization |= b;
      else if (parse_bool_option (arg, "force-fpu", &b))
        margs->force_fpu |= b;
      else if (parse_bool_option (arg, "load-core-plugins", &b))
        margs->load_core_plugins |= b;
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

Aida::ExecutionContext&
execution_context () // bseutils.hh
{
  assert_return (bse_execution_context != NULL, *bse_execution_context);
  return *bse_execution_context;
}

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
  config.synth_latency = 50;
  config.synth_mixing_freq = 48000;
  config.synth_control_freq = 1000;
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

static Configuration
global_config_load ()
{
  static bool loaded_once = false;
  if (!loaded_once)
    {
      Configuration config = GlobalConfig::defaults();
      // load from rcfile
      String fileconfig = Path::stringread (global_config_beastrc());
      if (!fileconfig.empty())
        {
          Configuration tmp;
          SerializeFromXML si (fileconfig);
          if (si.load (tmp))
            config = tmp;
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
  if (global_config_rcsettings == configuration)
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
      SerializeToXML so ("configuration", version());
      so.save (global_config_rcsettings);
      Path::stringwrite (global_config_beastrc(), so.to_xml(), true);
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
  static Configuration static_config = GlobalConfig::defaults();
  if (!GlobalConfig::locked())
    {
      static size_t last_stamp = 0;
      if (last_stamp != global_config_stamp)
        {
          static_config = global_config_load();
          last_stamp = global_config_stamp;
        }
    }
  return static_cast<GlobalConfig*> (&static_config);
}

} // Bse
