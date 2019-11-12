// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SERVER_H__
#define __BSE_SERVER_H__
#include <bse/bsesuper.hh>
#include <bse/driver.hh>

/* --- BSE type macros --- */
#define BSE_TYPE_SERVER              (BSE_TYPE_ID (BseServer))
#define BSE_SERVER_CAST(object)      (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SERVER, BseServer))
#define BSE_SERVER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SERVER, BseServerClass))
#define BSE_IS_SERVER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SERVER))
#define BSE_IS_SERVER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SERVER))
#define BSE_SERVER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SERVER, BseServerClass))

struct BseServer : BseContainer {
  GSource	  *engine_source;
  GSList	  *children;
  gchar		  *wave_file;
  double           wave_seconds;
  guint		   dev_use_count;
  BseModule       *pcm_imodule;
  BseModule       *pcm_omodule;
  BsePcmWriter	  *pcm_writer;
  GSList	  *watch_list;
};
struct BseServerClass : BseContainerClass
{};

BseServer*  bse_server_get			  (void);
void        bse_server_stop_recording             (BseServer *server);
void        bse_server_start_recording            (BseServer *server, const char *wave_file, double n_seconds);
Bse::Error  bse_server_open_devices		  (BseServer *server);
void	    bse_server_close_devices              (BseServer *server);
void	    bse_server_shutdown                   (BseServer *server);
BseModule*  bse_server_retrieve_pcm_output_module (BseServer *server, BseSource *source, const char *uplink_name);
void	    bse_server_discard_pcm_output_module  (BseServer *server, BseModule *module);
BseModule*  bse_server_retrieve_pcm_input_module  (BseServer *server, BseSource *source, const char *uplink_name);
void	    bse_server_discard_pcm_input_module   (BseServer *server, BseModule *module);
void	    bse_server_add_io_watch		  (BseServer *server, int fd, GIOCondition events, BseIOWatch watch_func, void *data);
void	    bse_server_remove_io_watch            (BseServer *server, BseIOWatch watch_func, void *data);

#define BSE_SERVER      (Bse::ServerImpl::instance())

namespace Bse {

class IpcHandler {
protected:
  virtual ~IpcHandler() = 0;
public:
  using BinarySender = std::function<bool(const std::string&)>;
  virtual ptrdiff_t    current_connection_id () = 0;
  virtual BinarySender create_binary_sender  () = 0;
};

class ServerImpl : public virtual ServerIface, public virtual ContainerImpl {
  int32              tc_ = 0;
  bool               log_messages_ = true;
  bool               pcm_input_checked_ = false;
  PcmDriverP         pcm_driver_;
  MidiDriverP        midi_driver_;
protected:
  virtual            ~ServerImpl            ();
public:
  void                enginechange          (bool active);
  SharedBlock         allocate_shared_block (int64 length);
  void                release_shared_block  (const SharedBlock &block);
  void                set_ipc_handler       (IpcHandler *ipch);
  IpcHandler*         get_ipc_handler       ();
  Error               open_midi_driver      ();
  void                close_midi_driver     ();
  PcmDriverP          pcm_driver            () const { return pcm_driver_; }
  Error               open_pcm_driver       (uint mix_freq, uint latency, uint block_size);
  void                require_pcm_input     ();
  void                close_pcm_driver      ();
  explicit                 ServerImpl       (BseObject*);
  virtual bool             log_messages     () const override;
  virtual void             log_messages     (bool val) override;
  virtual String           wave_file        () const override;
  virtual void             wave_file        (const String& val) override;
  virtual bool             engine_active    () override;
  virtual LegacyObjectIfaceP    from_proxy       (int64_t proxyid) override;
  virtual SharedMemory  get_shared_memory   () override;
  virtual void    broadcast_shm_fragments   (const ShmFragmentSeq &plan, int interval_ms) override;
  virtual String        get_mp3_version     () override;
  virtual String        get_vorbis_version  () override;
  virtual String        get_ladspa_path     () override;
  virtual String        get_plugin_path     () override;
  virtual String        get_instrument_path () override;
  virtual String        get_sample_path     () override;
  virtual String        get_effect_path     () override;
  virtual String        get_demo_path       () override;
  virtual String        get_version         () override;
  virtual String        get_custom_effect_dir () override;
  virtual String        get_custom_instrument_dir () override;
  virtual void   purge_stale_cachedirs   () override;
  virtual void   start_recording         (const String &wave_file, double n_seconds) override;
  virtual void   load_assets             () override;
  virtual void   load_ladspa             () override;
  virtual bool   can_load                (const String &file_name) override;
  virtual ProjectIfaceP create_project   (const String &project_name) override;
  virtual ProjectIfaceP last_project     () override;
  virtual AuxDataSeq list_module_types      () override;
  virtual AuxData    find_module_type       (const String &module_type) override;
  virtual Icon       module_type_icon       (const String &module_type) override;
  virtual SampleFileInfo sample_file_info   (const String &filename) override;
  virtual Configuration  get_config_defaults () override;
  virtual Configuration  get_config         () override;
  virtual void           set_config         (const Configuration &configuration) override;
  virtual bool           locked_config      () override;
  virtual NoteDescription note_describe_from_freq (MusicalTuning musical_tuning, double freq) override;
  virtual NoteDescription note_describe    (MusicalTuning musical_tuning, int note, int fine_tune) override;
  virtual NoteDescription note_construct   (MusicalTuning musical_tuning, int semitone, int octave, int fine_tune) override;
  virtual NoteDescription note_from_string (MusicalTuning musical_tuning, const String &name) override;
  virtual int             note_from_freq   (MusicalTuning musical_tuning, double frequency) override;
  virtual double          note_to_freq     (MusicalTuning musical_tuning, int note, int fine_tune) override;
  virtual CategorySeq     category_match_typed    (const String &pattern, const String &type_name) override;
  virtual CategorySeq     category_match          (const String &pattern) override;
  virtual int64           tick_stamp_from_systime (int64 systime_usecs) override;
  virtual void            send_user_message       (const UserMessage &umsg) override;
  virtual void            test_counter_set        (int val) override;
  virtual int             test_counter_inc_fetch  () override;
  virtual int             test_counter_get        () override;
  virtual DriverEntrySeq  list_pcm_drivers        () override;
  virtual DriverEntrySeq  list_midi_drivers       () override;
  virtual DeviceCrawlerIfaceP get_device_crawler  () override;
  static void        register_source_module (const String &type, const String &title, const String &tags, const uint8 *pixstream);
  static ServerImpl& instance               ();
};

} // Bse

#endif /* __BSE_SERVER_H__ */
