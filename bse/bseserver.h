/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_SERVER_H__
#define __BSE_SERVER_H__

#include        <bse/bsesuper.h>
#include        <bse/bsepcmdevice.h>
#include        <bse/bsemididevice.h>

G_BEGIN_DECLS

/* --- BSE type macros --- */
#define BSE_TYPE_SERVER              (BSE_TYPE_ID (BseServer))
#define BSE_SERVER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SERVER, BseServer))
#define BSE_SERVER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SERVER, BseServerClass))
#define BSE_IS_SERVER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SERVER))
#define BSE_IS_SERVER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SERVER))
#define BSE_SERVER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SERVER, BseServerClass))


/* --- BseServer object --- */
struct _BseServer
{
  BseContainer     parent_object;
  
  GSource	  *engine_source;
  
  GList	          *projects;
  GSList	  *children;
  
  guint		   dev_use_count;
  guint		   pcm_latency;
  BsePcmDevice    *pcm_device;
  GslModule       *pcm_imodule;
  GslModule       *pcm_omodule;
  BseMidiDevice	  *midi_device;
  BseMidiReceiver *midi_receiver;
  
  GSList	  *watch_list;
};
struct _BseServerClass
{
  BseContainerClass parent_class;
};


/* --- prototypes --- */
BseServer*	bse_server_get				(void);
BseProject*	bse_server_create_project		(BseServer	*server,
							 const gchar	*name);
BseProject*	bse_server_find_project			(BseServer	*server,
							 const gchar	*name);
BseErrorType	bse_server_open_devices			(BseServer	*server);
void		bse_server_close_devices		(BseServer	*server);
GslModule*	bse_server_retrieve_pcm_output_module	(BseServer	*server,
							 BseSource	*source,
							 const gchar	*uplink_name);
void		bse_server_discard_pcm_output_module	(BseServer	*server,
							 GslModule	*module);
GslModule*	bse_server_retrieve_pcm_input_module	(BseServer	*server,
							 BseSource	*source,
							 const gchar	*uplink_name);
void		bse_server_discard_pcm_input_module	(BseServer	*server,
							 GslModule	*module);
BseMidiReceiver*bse_server_get_midi_receiver		(BseServer	*self,
							 const gchar	*midi_name);
GslModule*	bse_server_retrieve_midi_input_module	(BseServer	*server,
							 const gchar	*downlink_name,
							 guint		 midi_channel_id,
							 guint		 nth_note,
							 guint		 signals[4]);
void		bse_server_discard_midi_input_module	(BseServer	*server,
							 GslModule	*module);
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
void		bse_server_script_start			(BseServer       *server,
							 BseJanitor	 *janitor);
void		bse_server_script_error			(BseServer	 *server,
							 const gchar	 *script_name,
							 const gchar	 *proc_name,
							 const gchar     *reason);
void		bse_server_user_message			(BseServer	*server,
							 BseUserMsgType  msg_type,
							 const gchar    *message);
BseErrorType	bse_server_run_remote			(BseServer	   *server,
							 const gchar	   *process_name,
							 SfiRing	   *params,
							 const gchar       *script_name,
							 const gchar       *proc_name,
							 BseJanitor	  **janitor_p);
void		bse_server_queue_kill_wire		(BseServer	*server,
							 SfiComWire	*wire);
void		bse_server_notify_gconfig		(BseServer	*server);

G_END_DECLS

#endif /* __BSE_SERVER_H__ */
