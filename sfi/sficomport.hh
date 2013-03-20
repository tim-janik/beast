// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_COM_PORT_H__
#define __SFI_COM_PORT_H__

#include <sfi/sfitypes.hh>
#include <sfi/sfiring.hh>

G_BEGIN_DECLS

#define	SFI_COM_PORT_MAGIC	(0x42534500 /* "BSE\0" */)

struct SfiComPort;
struct SfiComPortLink;
typedef void (*SfiComPortClosedFunc) (SfiComPort *port, void *close_data);

struct SfiComPort {
  gchar    *ident;
  guint     ref_count;
  GPollFD   pfd[2];	/* 0 = remote in, 1 = remote out */
  guint	    connected : 1;
  guint	    reaped : 1;
  guint	    sigterm_sent : 1;
  guint	    sigkill_sent : 1;
  guint	    exit_signal_sent : 1;
  guint	    dumped_core : 1;
  SfiComPortLink *link;
  struct {
    guint   n;
    guint8 *data;
    guint   allocated;
  }         wbuffer;	/* output buffer */
  struct {
    guint   hlen;
    guint8  header[8];
    guint   dlen;
    guint   n;
    guint8 *data;
    guint   allocated;
  }         rbuffer;	/* input buffer */
  SfiRing  *rvalues;
  GScanner *scanner;
  SfiComPortClosedFunc close_func;
  gpointer	       close_data;
  gint	    remote_pid;
  gint	    exit_code;
  gint	    exit_signal;
};

struct SfiComPortLink
{
  Bse::Mutex            mutex;
  guint                 ref_count;
  SfiComPort           *port1;
  std::function<void()> wakeup1;
  SfiComPort           *port2;
  std::function<void()> wakeup2;
  SfiRing              *p1queue;
  SfiRing              *p2queue;
  Bse::Cond             wcond;
  bool                  waiting;
};

/* create ports */
SfiComPort*	sfi_com_port_from_pipe		(const gchar	*ident,
						 gint		 remote_input,
						 gint		 remote_output);
SfiComPort*	sfi_com_port_from_child		(const gchar	*ident,
						 gint		 remote_input,
						 gint		 remote_output,
						 gint		 remote_pid);
/* create linked ports */
void		sfi_com_port_create_linked	(const gchar	*ident1,
                                                 std::function<void()> wakeup1,
						 SfiComPort    **port1,
						 const gchar	*ident2,
                                                 std::function<void()> wakeup2,
						 SfiComPort    **port2);
SfiComPort*	sfi_com_port_ref		(SfiComPort	*port);
void		sfi_com_port_unref		(SfiComPort	*port);
/* remote I/O */
void		sfi_com_port_send		(SfiComPort	*port,
						 const GValue	*value);
void		sfi_com_port_send_bulk		(SfiComPort	*port,
						 SfiRing        *value_ring);
GValue*		sfi_com_port_recv		(SfiComPort	*port);
GValue*		sfi_com_port_recv_blocking	(SfiComPort	*port);
/* I/O handling */
GPollFD*	sfi_com_port_get_poll_fds	(SfiComPort	*port,
						 guint		*n_pfds);
gboolean	sfi_com_port_io_pending		(SfiComPort	*port);
void		sfi_com_port_process_io		(SfiComPort	*port);
/* shutdown */
void		sfi_com_port_set_close_func	(SfiComPort	*port,
						 SfiComPortClosedFunc func,
						 gpointer	 close_data);
void		sfi_com_port_close_remote	(SfiComPort	*port,
						 gboolean	 terminate_child);
void		sfi_com_port_reap_child 	(SfiComPort	*port,
						 gboolean	 kill_child);
gboolean	sfi_com_port_test_reap_child 	(SfiComPort	*port);

G_END_DECLS

#endif /* __SFI_COM_PORT_H__ */
