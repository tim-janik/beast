// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SERVER_H__
#define __BSE_SERVER_H__
#include <bse/bsesuper.hh>
#include <bse/bsepcmdevice.hh>
#include <bse/bsemididevice.hh>
#include <bse/testobject.hh>

G_BEGIN_DECLS

/* --- BSE type macros --- */
#define BSE_TYPE_SERVER              (BSE_TYPE_ID (BseServer))
#define BSE_SERVER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SERVER, BseServer))
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
BseProject*	bse_server_create_project		(BseServer	*server,
							 const gchar	*name);
BseProject*	bse_server_find_project			(BseServer	*server,
							 const gchar	*name);
void    	bse_server_stop_recording		(BseServer	*server);
void            bse_server_start_recording              (BseServer      *server,
                                                         const char     *wave_file,
                                                         double          n_seconds);
BseErrorType	bse_server_open_devices			(BseServer	*server);
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
void		bse_server_send_message	                (BseServer	    *server,
                                                         const BseMessage   *umsg);
void		bse_server_message		        (BseServer	    *server,
                                                         const gchar        *log_domain,
                                                         BseMsgType          msg_type,
                                                         const gchar        *title,
                                                         const gchar        *primary,
                                                         const gchar        *secondary,
                                                         const gchar        *details,
                                                         const gchar        *config_blurb,
                                                         BseJanitor         *janitor,
                                                         const gchar        *process_name,
                                                         gint                pid);
BseErrorType	bse_server_run_remote			(BseServer	    *server,
							 const gchar	    *process_name,
							 SfiRing	    *params,
							 const gchar        *script_name,
							 const gchar        *proc_name,
							 BseJanitor	   **janitor_p);
void		bse_server_queue_kill_wire		(BseServer	    *server,
							 SfiComWire	    *wire);
void		bse_server_notify_gconfig		(BseServer	    *server);
G_END_DECLS


namespace Bse {

class ServerImpl : public ServerIface {
  TestObjectImplP    test_object_;
protected:
  explicit           ServerImpl ();
  virtual           ~ServerImpl ();
public:
  virtual TestObjectIface* get_test_object ();
  static ServerImpl&       instance        ();
  void                     send_user_message (const UserMessage &umsg);
};

} // Bse

#endif /* __BSE_SERVER_H__ */
