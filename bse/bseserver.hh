// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SERVER_H__
#define __BSE_SERVER_H__
#include <bse/bsesuper.hh>
#include <bse/bsepcmdevice.hh>
#include <bse/bsemididevice.hh>
#include <bse/testobject.hh>

/* --- BSE type macros --- */
#define BSE_TYPE_SERVER              (BSE_TYPE_ID (BseServer))
#define BSE_SERVER_CAST(object)      (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SERVER, BseServer))
#define BSE_SERVER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SERVER, BseServerClass))
#define BSE_IS_SERVER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SERVER))
#define BSE_IS_SERVER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SERVER))
#define BSE_SERVER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SERVER, BseServerClass))

struct BseServer : BseContainer {
  GSource	  *engine_source;
  GList	          *projects;
  GSList	  *children;
  gchar		  *wave_file;
  double           wave_seconds;
  guint		   dev_use_count;
  guint            log_messages : 1;
  guint            pcm_input_checked : 1;
  BsePcmDevice    *pcm_device;
  BseModule       *pcm_imodule;
  BseModule       *pcm_omodule;
  BsePcmWriter	  *pcm_writer;
  BseMidiDevice	  *midi_device;
  GSList	  *watch_list;
};
struct BseServerClass : BseContainerClass
{};

BseServer*	bse_server_get				(void);
BseProject*	bse_server_find_project			(BseServer	*server,
							 const gchar	*name);
void    	bse_server_stop_recording		(BseServer	*server);
void            bse_server_start_recording              (BseServer      *server,
                                                         const char     *wave_file,
                                                         double          n_seconds);
Bse::Error	bse_server_open_devices			(BseServer	*server);
void		bse_server_close_devices		(BseServer	*server);
BseModule*	bse_server_retrieve_pcm_output_module	(BseServer	*server,
							 BseSource	*source,
							 const gchar	*uplink_name);
void		bse_server_discard_pcm_output_module	(BseServer	*server,
							 BseModule	*module);
BseModule*	bse_server_retrieve_pcm_input_module	(BseServer	*server,
							 BseSource	*source,
							 const gchar	*uplink_name);
void		bse_server_discard_pcm_input_module	(BseServer	*server,
							 BseModule	*module);
void		bse_server_require_pcm_input    	(BseServer	*server);
BseModule*	bse_server_retrieve_midi_input_module	(BseServer	*server,
							 const gchar	*downlink_name,
							 guint		 midi_channel_id,
							 guint		 nth_note,
							 guint		 signals[4]);
void		bse_server_discard_midi_input_module	(BseServer	*server,
							 BseModule	*module);
void		bse_server_add_io_watch			(BseServer	*server,
							 gint		 fd,
							 GIOCondition	 events,
							 BseIOWatch	 watch_func,
							 gpointer	 data);
void		bse_server_remove_io_watch		(BseServer	*server,
							 BseIOWatch	 watch_func,
							 gpointer	 data);

/* --- internal --- */
void		bse_server_registration			(BseServer          *server,
							 BseRegistrationType rtype,
							 const gchar	    *what,
							 const gchar	    *error);
void		bse_server_script_start			(BseServer          *server,
							 BseJanitor	    *janitor);
void		bse_server_script_error			(BseServer	    *server,
							 const gchar	    *script_name,
							 const gchar	    *proc_name,
							 const gchar        *reason);
Bse::Error	bse_server_run_remote			(BseServer	    *server,
							 const gchar	    *process_name,
							 SfiRing	    *params,
							 const gchar        *script_name,
							 const gchar        *proc_name,
							 BseJanitor	   **janitor_p);
void		bse_server_queue_kill_wire		(BseServer	    *server,
							 SfiComWire	    *wire);
void		bse_server_notify_gconfig		(BseServer	    *server);

#define BSE_SERVER      (Bse::ServerImpl::instance())

namespace Bse {

class ServerImpl : public virtual ServerIface, public virtual ObjectImpl {
  TestObjectImplP    test_object_;
protected:
  virtual                 ~ServerImpl ();
public:
  explicit                 ServerImpl       (BseObject*);
  virtual TestObjectIfaceP get_test_object  () override;
  virtual ObjectIfaceP     from_proxy       (int64_t proxyid) override;
  virtual String        get_mp3_version     () override;
  virtual String        get_vorbis_version  () override;
  virtual String        get_ladspa_path     () override;
  virtual String        get_plugin_path     () override;
  virtual String        get_script_path     () override;
  virtual String        get_instrument_path () override;
  virtual String        get_sample_path     () override;
  virtual String        get_effect_path     () override;
  virtual String        get_demo_path       () override;
  virtual String        get_version         () override;
  virtual String        get_custom_effect_dir () override;
  virtual String        get_custom_instrument_dir () override;
  virtual void   save_preferences        () override;
  virtual void   register_ladspa_plugins () override;
  virtual void   register_core_plugins   () override;
  virtual void   start_recording         (const String &wave_file, double n_seconds) override;
  virtual void   register_scripts        () override;
  virtual bool   preferences_locked      () override;
  virtual int    n_scripts               () override;
  virtual bool   can_load                (const String &file_name) override;
  virtual ProjectIfaceP create_project   (const String &project_name) override;
  virtual void          destroy_project  (ProjectIface &project) override;
  virtual AuxDataSeq list_module_types      () override;
  virtual AuxData    find_module_type       (const String &module_type) override;
  virtual Icon       module_type_icon       (const String &module_type) override;
  virtual SampleFileInfo sample_file_info   (const String &filename) override;
  virtual NoteDescription note_describe_from_freq (MusicalTuning musical_tuning, double freq) override;
  virtual NoteDescription note_describe    (MusicalTuning musical_tuning, int note, int fine_tune) override;
  virtual NoteDescription note_construct   (MusicalTuning musical_tuning, int semitone, int octave, int fine_tune) override;
  virtual NoteDescription note_from_string (MusicalTuning musical_tuning, const String &name) override;
  virtual int             note_from_freq   (MusicalTuning musical_tuning, double frequency) override;
  virtual double          note_to_freq     (MusicalTuning musical_tuning, int note, int fine_tune) override;
  virtual CategorySeq     category_match_typed (const String &pattern, const String &type_name) override;
  virtual CategorySeq     category_match       (const String &pattern) override;
  void               send_user_message      (const UserMessage &umsg);
  static void        register_source_module (const String &type, const String &title, const String &tags, const uint8 *pixstream);
  static ServerImpl& instance               ();
};

} // Bse

#endif /* __BSE_SERVER_H__ */
