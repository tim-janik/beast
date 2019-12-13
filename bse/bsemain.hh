// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MAIN_H__
#define __BSE_MAIN_H__

#include <bse/bseenums.hh>

// == BSE Initialization ==
void _bse_init_async	(int *argc, char **argv, const char *app_name, const Bse::StringVector &args);
bool _bse_initialized	();
int  bse_init_and_test 	(int *argc, char **argv, const std::function<int()> &bsetester, const Bse::StringVector &args = Bse::StringVector());
void bse_main_wakeup    ();

namespace Bse {

// == Bse Configuration ==
struct GlobalConfig : Configuration {
  static Configuration defaults ();
  static void          assign   (const Configuration &configuration);
  static void          flush    ();
  static void          lock     ();
  static void          unlock   ();
  static bool          locked   ();
};
struct GlobalConfigPtr {
  const GlobalConfig* operator-> () const;
  const GlobalConfig& operator*  () const { return *operator->(); }
};
inline GlobalConfigPtr global_config;

bool* register_driver_loader (const char *staticwhat, Error (*loader) ());

class JobQueue {
  static void call_remote (const std::function<void()>&);
  template<typename T, typename = void>
  struct LambdaTraits;
  template<typename R> struct LambdaTraits<R (*)()> { using ReturnType = R; };
  template<typename R, typename C>
  struct LambdaTraits<R (C::*)() const> { using ReturnType = R; };
  template<typename T> struct LambdaTraits<T, void_t< decltype (&T::operator()) > > :
    public LambdaTraits<decltype (&T::operator())> {};
public:
  template<typename F> typename LambdaTraits<F>::ReturnType
  operator+= (const F &job)
  {
    using R = typename LambdaTraits<F>::ReturnType;
    if constexpr (std::is_same<R, void>::value)
      {
        call_remote (job);
        return;
      }
    else // R != void
      {
        R result;
        call_remote ([&job, &result] () {
            result = job();
          });
        return result;
      }
  }
};
/// Execute a lambda job in the Bse main loop and wait for its result.
extern JobQueue jobs;

} // Bse


/* --- global macros --- */
#define	BSE_THREADS_ENTER()			// bse_main_global_lock ()
#define	BSE_THREADS_LEAVE()			// bse_main_global_unlock ()
#define	BSE_DBG_EXT     			(bse_main_args->debug_extensions != FALSE)
#define	BSE_CONFIG(field)			(bse_main_args->field)

/* --- argc/argv overide settings --- */
struct BseMainArgs {
  const char           *pcm_driver;
  const char           *midi_driver;
  const char           *override_plugin_globs;
  const char 	       *override_sample_path;
  uint   	        n_processors;
  /* # values to pad around wave chunk blocks per channel */
  uint   	        wave_chunk_padding;
  uint   	        wave_chunk_big_pad;
  /* data (file) cache block size (aligned to power of 2) */
  uint   	        dcache_block_size;
  /* amount of bytes to spare for memory cache */
  uint   	        dcache_cache_memory;
  uint   	        midi_kammer_note;
  /* kammer frequency, normally 440Hz, historically 435Hz */
  double 	        kammer_freq;
  bool                  stand_alone;            ///< Initialization argument "stand-alone" - no rcfiles, etc.
  bool                  allow_randomization;	///< Initialization argument "allow-randomization" - enables non-deterministic behavior
  bool                  force_fpu;		///< Initialization argument "force-fpu" - avoid vectorized optimizations
  bool		        load_drivers;   	///< Load Audio/MIDI drivers at bootup
  bool                  debug_extensions;       ///< Initialization argument "debug-extensions" - enable debugging extensions
  bool                  dump_driver_list;
};

/* --- internal --- */
void    _bse_init_c_wrappers    ();
extern BseMainArgs     *bse_main_args;

#endif /* __BSE_MAIN_H__ */
