/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
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
#ifndef __BSE_COM_WIRE_H__
#define __BSE_COM_WIRE_H__

#include <bse/bsedefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* typedef gboolean (*BseComDispatch)	(gpointer	 data,
 *					 guint		 request,
 *					 const gchar	*request_msg,
 *					 BseComWire	*wire);
 */

struct _BseComWire
{
  gchar		*ident;		/* debugging identifier for this connection */
  gpointer	 owner;		/* BseScriptControl* */
  guint		 connected : 1;
  guint		 remote_input_broke : 1;
  guint		 remote_output_broke : 1;
  guint		 standard_input_broke : 1;
  guint		 standard_output_broke : 1;
  guint		 standard_error_broke : 1;
  
  BseComDispatch dispatch_func;
  gpointer	 dispatch_data;
  GDestroyNotify destroy_data;

  /* message queues */
  GList		*orequests;	/* outgoing requests */
  GList		*iresults;	/* incoming results */
  GList		*irequests;	/* incoming requests */
  GList		*rrequests;	/* received requests */

  /* I/O channels */
  gint           remote_input;		/* readable */
  gint           remote_output;		/* writable */

  /* spawned child */
  gint           standard_input;	/* writable */
  gint           standard_output;	/* readable */
  gint           standard_error;	/* readable */
  gint           remote_pid;
  GString	*gstring_stdout;
  GString	*gstring_stderr;

  /* input buffer */
  guint8	*ibuffer;
  guint8	*ibp;
  guint8	*ibound;

  /* output buffer */
  guint8	*obuffer;
  guint8	*obp;
  guint8	*obound;
};

typedef enum /*< skip >*/
{
  BSE_COM_MSG_INVALID		= 0,
  BSE_COM_MSG_RESERVED1		= 1,
  BSE_COM_MSG_RESERVED2		= 2,
  BSE_COM_MSG_RESERVED3		= 3,
  BSE_COM_MSG_RESERVED4		= 4,
  BSE_COM_MSG_REQUEST		= 5,
  BSE_COM_MSG_RESULT		= 6
} BseComMsgType;
#define	BSE_MAGIC_BSEm		(0x4253456d)	/* "BSEm" */
typedef struct
{
  guint32	magic;		/* "BSEm" 0x4253456d */
  guint32	mlength;	/* total length, including magic */
  guint32	type;
  guint32	request;
  gchar	       *message;
} BseComMsg;


/* create wires */
BseComWire*	bse_com_wire_from_pipe		(const gchar	*ident,
						 gint		 remote_input,
						 gint		 remote_output);
BseComWire*	bse_com_wire_from_child		(const gchar	*ident,
						 gint		 remote_input,
						 gint		 remote_output,
						 gint		 standard_input,
						 gint		 standard_output,
						 gint		 standard_error,
						 gint		 remote_pid);

/* handle outgoing */
guint		bse_com_wire_send_request	(BseComWire	*wire,
						 const gchar	*request_msg);
gchar*		bse_com_wire_receive_result	(BseComWire	*wire,
						 guint		 request);
void		bse_com_wire_forget_request	(BseComWire	*wire,
						 guint		 request);
guint		bse_com_wire_peek_first_result	(BseComWire	*wire);

/* handle incomming */
const gchar*	bse_com_wire_receive_request	(BseComWire	*wire,
						 guint		*request);
void		bse_com_wire_send_result	(BseComWire	*wire,
						 guint		 request,
						 const gchar	*result_msg);
void		bse_com_wire_discard_request	(BseComWire	*wire,
						 guint		 request);

/* dispatching */
void		bse_com_wire_set_dispatcher	(BseComWire	*wire,
						 BseComDispatch	 dispatch_func,
						 gpointer	 dispatch_data,
						 GDestroyNotify  destroy_data);
void		bse_com_wire_dispatch		(BseComWire	*wire,
						 guint		 request);
gboolean	bse_com_wire_need_dispatch	(BseComWire	*wire);

/* wire I/O */
gint*		bse_com_wire_get_read_fds	(BseComWire	*wire,
						 guint		*n_fds);
gint*		bse_com_wire_get_write_fds	(BseComWire	*wire,
						 guint		*n_fds);
GPollFD*	bse_com_wire_get_poll_fds	(BseComWire	*wire,
						 guint		*n_pfds);
void		bse_com_wire_process_io		(BseComWire	*wire);
gchar*		bse_com_wire_collect_stdout	(BseComWire	*wire,
						 guint		*n_chars);
gchar*		bse_com_wire_collect_stderr	(BseComWire	*wire,
						 guint		*n_chars);

/* shutdown */
void		bse_com_wire_close_remote	(BseComWire	*wire,
						 gboolean	 terminate);
void		bse_com_wire_destroy		(BseComWire	*wire);


/* convenience */
gboolean	bse_com_wire_receive_dispatch	(BseComWire	*wire);
void		bse_com_wire_select		(BseComWire	*wire,
						 guint		 timeout);
gchar*		bse_com_wire_ping_pong		(BseComWire	*wire,
						 const gchar	*ping,
						 guint		 timeout);


/* --- fork/exec --- */
void		bse_com_set_spawn_dir		(const gchar	*cwd);
gchar*		bse_com_spawn_async		(const gchar	*executable,
						 gint		*child_pid,
						 gint		*standard_input,
						 gint		*standard_output,
						 gint		*standard_error,
						 const gchar	*command_fd_option,
						 gint		*command_input,
						 gint		*command_output,
						 GSList		*args);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_COM_WIRE_H__ */
