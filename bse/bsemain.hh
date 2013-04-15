// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_MAIN_H__
#define __BSE_MAIN_H__
#include	<bse/bse.hh>	/* initialization */
#include        <bse/bsetype.hh>
G_BEGIN_DECLS

// == BSE Initialization ==
void		bse_init_textdomain_only (void);
void		_bse_init_async		 (int *argc, char **argv, const char *app_name, const Bse::StringVector &args);
SfiGlueContext* _bse_glue_context_create (const char *client, const std::function<void()> &caller_wakeup);
const char*     bse_check_version	(guint		 required_major,
                                         guint		 required_minor,
                                         guint		 required_micro);       // prototyped in bse.hh

/* initialization for internal utilities */
void bse_init_inprocess	(int *argc, char **argv, const char *app_name, const Bse::StringVector &args = Bse::StringVector());
void bse_init_test 	(int *argc, char **argv, const Bse::StringVector &args = Bse::StringVector());
void bse_main_wakeup    ();

/* --- global macros --- */
#define	BSE_THREADS_ENTER()			// bse_main_global_lock ()
#define	BSE_THREADS_LEAVE()			// bse_main_global_unlock ()
#define	BSE_DBG_EXT     			(bse_main_args->debug_extensions != FALSE)
#define	BSE_CONFIG(field)			(bse_main_args->field)

/* --- argc/argv overide settings --- */
struct BseMainArgs {
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
  const char           *path_binaries;
  const char           *bse_rcfile;
  const char           *override_plugin_globs;
  const char           *override_script_path;
  const char 	       *override_sample_path;
  bool                  stand_alone;            ///< Initialization argument "stand-alone" - no rcfiles, boot scripts, etc.
  bool                  allow_randomization;	///< Initialization argument "allow-randomization" - enables non-deterministic behavior
  bool                  force_fpu;		///< Initialization argument "force-fpu" - avoid vectorized optimizations
  bool		        load_core_plugins;	///< Initialization argument "load-core-plugins" - enable core plugin bootup
  bool		        load_core_scripts;	///< Initialization argument "load-core-scripts" - enable core script bootup
  bool		        debug_extensions;	///< Initialization argument "debug-extensions" - enable debugging extensions
  bool                  load_drivers_early;
  bool                  dump_driver_list;
  int                   latency;
  int                   mixing_freq;
  int                   control_freq;
  SfiRing              *pcm_drivers;
  SfiRing              *midi_drivers;
};

/* --- internal --- */
void    _bse_init_c_wrappers    ();
extern BseMainArgs     *bse_main_args;
extern GMainContext    *bse_main_context;

G_END_DECLS

#endif /* __BSE_MAIN_H__ */
