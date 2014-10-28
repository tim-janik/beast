// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_COM_WIRE_H__
#define __SFI_COM_WIRE_H__

#include <sfi/sfitypes.hh>
#include <sfi/sfiring.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct _SfiComWire SfiComWire;
typedef gboolean (*SfiComDispatch)	(gpointer	 data,
					 guint		 request,
					 const gchar	*request_msg,
					 SfiComWire	*wire);
struct _SfiComWire
{
  gchar		*ident;		/* debugging identifier for this connection */
  gpointer	 owner;		/* ScriptControl object */
  guint		 connected : 1;
  guint		 remote_input_broke : 1;
  guint		 remote_output_broke : 1;
  guint		 standard_input_broke : 1;
  guint		 standard_output_broke : 1;
  guint		 standard_error_broke : 1;

  SfiComDispatch dispatch_func;
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
  SFI_COM_MSG_INVALID		= 0,
  SFI_COM_MSG_RESERVED1		= 1,
  SFI_COM_MSG_RESERVED2		= 2,
  SFI_COM_MSG_RESERVED3		= 3,
  SFI_COM_MSG_RESERVED4		= 4,
  SFI_COM_MSG_REQUEST		= 5,
  SFI_COM_MSG_RESULT		= 6
} SfiComMsgType;
#define	BSE_MAGIC_BSEm		(0x4253456d)	/* "BSEm" */
typedef struct
{
  guint32	magic;		/* "BSEm" 0x4253456d */
  guint32	mlength;	/* total length, including magic */
  guint32	type;
  guint32	request;
  gchar	       *message;
} SfiComMsg;


/* create wires */
SfiComWire*	sfi_com_wire_from_pipe		(const gchar	*ident,
						 gint		 remote_input,
						 gint		 remote_output);
SfiComWire*	sfi_com_wire_from_child		(const gchar	*ident,
						 gint		 remote_input,
						 gint		 remote_output,
						 gint		 standard_input,
						 gint		 standard_output,
						 gint		 standard_error,
						 gint		 remote_pid);

/* handle outgoing */
guint		sfi_com_wire_send_request	(SfiComWire	*wire,
						 const gchar	*request_msg);
gchar*		sfi_com_wire_receive_result	(SfiComWire	*wire,
						 guint		 request);
void		sfi_com_wire_forget_request	(SfiComWire	*wire,
						 guint		 request);
guint		sfi_com_wire_peek_first_result	(SfiComWire	*wire);

/* handle incomming */
const gchar*	sfi_com_wire_receive_request	(SfiComWire	*wire,
						 guint		*request);
void		sfi_com_wire_send_result	(SfiComWire	*wire,
						 guint		 request,
						 const gchar	*result_msg);
void		sfi_com_wire_discard_request	(SfiComWire	*wire,
						 guint		 request);

/* dispatching */
void		sfi_com_wire_set_dispatcher	(SfiComWire	*wire,
						 SfiComDispatch	 dispatch_func,
						 gpointer	 dispatch_data,
						 GDestroyNotify  destroy_data);
void		sfi_com_wire_dispatch		(SfiComWire	*wire,
						 guint		 request);
gboolean	sfi_com_wire_need_dispatch	(SfiComWire	*wire);

/* wire I/O */
gint*		sfi_com_wire_get_read_fds	(SfiComWire	*wire,
						 guint		*n_fds);
gint*		sfi_com_wire_get_write_fds	(SfiComWire	*wire,
						 guint		*n_fds);
GPollFD*	sfi_com_wire_get_poll_fds	(SfiComWire	*wire,
						 guint		*n_pfds);
void		sfi_com_wire_process_io		(SfiComWire	*wire);
gchar*		sfi_com_wire_collect_stdout	(SfiComWire	*wire,
						 guint		*n_chars);
gchar*		sfi_com_wire_collect_stderr	(SfiComWire	*wire,
						 guint		*n_chars);

/* shutdown */
void		sfi_com_wire_close_remote	(SfiComWire	*wire,
						 gboolean	 terminate);
void		sfi_com_wire_destroy		(SfiComWire	*wire);


/* convenience */
gboolean	sfi_com_wire_receive_dispatch	(SfiComWire	*wire);
void		sfi_com_wire_select		(SfiComWire	*wire,
						 guint		 timeout);
gchar*		sfi_com_wire_ping_pong		(SfiComWire	*wire,
						 const gchar	*ping,
						 guint		 timeout);


/* --- fork/exec --- */
void		sfi_com_set_spawn_dir		(const gchar	*cwd);
const char*	sfi_com_spawn_async		(const gchar	*executable,
						 gint		*child_pid,
						 gint		*standard_input,
						 gint		*standard_output,
						 gint		*standard_error,
						 const gchar	*command_fd_option,
						 gint		*command_input,
						 gint		*command_output,
						 SfiRing	*args);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SFI_COM_WIRE_H__ */
