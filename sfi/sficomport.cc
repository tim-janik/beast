// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "sficomport.hh"
#include "sfiprimitives.hh"
#include "sfiserial.hh"
#include "sfistore.hh"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CDEBUG(...)     Bse::debug ("comport", __VA_ARGS__)

/* define the io bottle neck (for writes) to a small value
 * (e.g. 20) to trigger and test blocking IO on fast systems
 */
#define IO_BOTTLE_NECK  (1024 * 1024)
/* some systems don't have ERESTART (which is what linux returns for system
 * calls on pipes which are being interrupted). most probably just use EINTR,
 * and maybe some can return both. so we check for both in the below code,
 * and alias ERESTART to EINTR if it's not present.
 */
#ifndef ERESTART
#define ERESTART        EINTR
#endif


/* --- functions --- */
static gint
nonblock_fd (gint fd)
{
  if (fd >= 0)
    {
      glong r, d_long;
      do
	d_long = fcntl (fd, F_GETFL);
      while (d_long < 0 && errno == EINTR);

      d_long |= O_NONBLOCK;

      do
	r = fcntl (fd, F_SETFL, d_long);
      while (r < 0 && errno == EINTR);
    }
  return fd;
}

SfiComPort*
sfi_com_port_from_child (const gchar *ident,
			 gint         remote_input,
			 gint         remote_output,
			 gint         remote_pid)
{
  SfiComPort *port;

  assert_return (ident != NULL, NULL);

  port = g_new0 (SfiComPort, 1);
  port->ref_count = 1;
  if (remote_pid > 1)
    port->ident = g_strdup_format ("%s[%u]", ident, remote_pid);
  else
    port->ident = g_strdup (ident);
  port->pfd[0].fd = nonblock_fd (remote_input);
  port->pfd[0].events = port->pfd[0].fd >= 0 ? G_IO_IN : 0;
  port->pfd[0].revents = 0;
  port->pfd[1].fd = nonblock_fd (remote_output);
  port->pfd[1].events = port->pfd[1].fd >= 0 ? G_IO_OUT : 0;
  port->pfd[1].revents = 0;
  if (remote_pid > 1)
    {
      port->remote_pid = remote_pid;
      port->reaped = FALSE;
    }
  else
    {
      port->remote_pid = -1;
      port->reaped = TRUE;
    }
  port->sigterm_sent = FALSE;
  port->sigkill_sent = FALSE;
  port->exit_signal_sent = FALSE;
  port->dumped_core = FALSE;
  port->exit_code = 0;
  port->exit_signal = 0;
  port->link = NULL;
  port->connected = ((remote_input < 0 || port->pfd[0].fd >= 0) &&
		     (remote_output < 0 || port->pfd[1].fd >= 0));
  return port;
}

SfiComPort*
sfi_com_port_from_pipe (const gchar *ident,
			gint         remote_input,
			gint         remote_output)
{
  assert_return (ident != NULL, NULL);

  return sfi_com_port_from_child (ident,
				  remote_input,
				  remote_output,
				  -1);
}

void
sfi_com_port_create_linked (const gchar *ident1, std::function<void()> wakeup1, SfiComPort **port1,
			    const gchar *ident2, std::function<void()> wakeup2, SfiComPort **port2)
{
  SfiComPortLink *link;
  assert_return (wakeup1 && ident1);
  assert_return (wakeup2 && ident2);
  assert_return (port1 && port2);
  link = new SfiComPortLink();
  link->port1 = sfi_com_port_from_child (ident1, -1, -1, -1);
  link->wakeup1 = wakeup1;
  link->port2 = sfi_com_port_from_child (ident2, -1, -1, -1);
  link->wakeup2 = wakeup2;
  link->ref_count = 2;
  link->port1->link = link;
  link->port1->connected = TRUE;
  link->port2->link = link;
  link->port2->connected = TRUE;
  *port1 = link->port1;
  *port2 = link->port2;
}

static void
sfi_com_port_link_destroy (SfiComPortLink *link)
{
  assert_return (link->ref_count == 0);
  assert_return (link->port1== NULL && link->port2 == NULL);

  while (link->p1queue)
    sfi_value_free ((GValue*) sfi_ring_pop_head (&link->p1queue));
  while (link->p2queue)
    sfi_value_free ((GValue*) sfi_ring_pop_head (&link->p2queue));
  delete link;
}

SfiComPort*
sfi_com_port_ref (SfiComPort *port)
{
  assert_return (port != NULL, NULL);
  assert_return (port->ref_count > 0, NULL);

  port->ref_count++;
  return port;
}

void
sfi_com_port_set_close_func (SfiComPort          *port,
			     SfiComPortClosedFunc func,
			     gpointer             close_data)
{
  assert_return (port != NULL);

  port->close_func = func;
  port->close_data = func ? close_data : NULL;
  /* provide notification right now */
  if (!port->connected)
    sfi_com_port_close_remote (port, FALSE);
}

static void
com_port_try_reap (SfiComPort *port,
                   gboolean    mayblock)
{
  if (port->remote_pid && !port->reaped)
    {
      int status = 0;
      gint ret = waitpid (port->remote_pid, &status, mayblock ? 0 : WNOHANG);
      if (ret > 0)
        {
          port->reaped = TRUE;
          port->exit_code = WEXITSTATUS (status);
          port->exit_signal = WIFSIGNALED (status) ? WTERMSIG (status) : 0;
#ifdef WCOREDUMP
          port->dumped_core = WCOREDUMP (status) != 0;
#endif
          port->exit_signal_sent = (port->exit_signal == SIGTERM && port->sigterm_sent) ||
                                   (port->exit_signal == SIGKILL && port->sigkill_sent);
        }
      else if (ret < 0 && errno == EINTR && mayblock)
        com_port_try_reap (port, mayblock);
    }
}

void
sfi_com_port_close_remote (SfiComPort *port,
			   gboolean    terminate_child)
{
  assert_return (port != NULL);

  port->connected = FALSE;
  if (port->pfd[0].fd >= 0)
    {
      close (port->pfd[0].fd);
      port->pfd[0].fd = -1;
      port->pfd[0].events = 0;
    }
  if (port->pfd[1].fd >= 0)
    {
      close (port->pfd[1].fd);
      port->pfd[1].fd = -1;
      port->pfd[1].events = 0;
    }
  com_port_try_reap (port, FALSE);
  if (terminate_child &&
      port->remote_pid > 1 &&
      !port->reaped &&
      !port->sigterm_sent)
    {
      if (kill (port->remote_pid, SIGTERM) >= 0)
        port->sigterm_sent = TRUE;
      com_port_try_reap (port, FALSE);
    }
  if (port->link)
    {
      SfiComPortLink *link = port->link;
      bool need_destroy;
      link->mutex.lock();
      if (port == link->port1)
	{
	  link->port1 = NULL;
	  link->wakeup1 = NULL;
	}
      else
	{
	  link->port2 = NULL;
	  link->wakeup2 = NULL;
	}
      link->ref_count--;
      need_destroy = link->ref_count == 0;
      link->mutex.unlock();
      port->link = NULL;
      if (need_destroy)
	sfi_com_port_link_destroy (port->link);
    }
  if (port->close_func)
    {
      SfiComPortClosedFunc close_func = port->close_func;
      gpointer close_data = port->close_data;
      port->close_func = NULL;
      port->close_data = NULL;
      close_func (port, close_data);
    }
}

static void
sfi_com_port_destroy (SfiComPort *port)
{
  assert_return (port != NULL);

  sfi_com_port_close_remote (port, FALSE);
  if (port->scanner)
    g_scanner_destroy (port->scanner);
  while (port->rvalues)
    sfi_value_free ((GValue*) sfi_ring_pop_head (&port->rvalues));
  g_free (port->ident);
  g_free (port->wbuffer.data);
  g_free (port->rbuffer.data);
  g_free (port);
}

void
sfi_com_port_unref (SfiComPort *port)
{
  assert_return (port != NULL);
  assert_return (port->ref_count > 0);

  port->ref_count--;
  if (!port->ref_count)
    sfi_com_port_destroy (port);
}

static void
com_port_grow_wbuffer (SfiComPort *port,
		       guint       delta)
{
  if (port->wbuffer.n + delta > port->wbuffer.allocated)
    {
      port->wbuffer.allocated = port->wbuffer.n + delta;
      port->wbuffer.data = g_renew (guint8, port->wbuffer.data, port->wbuffer.allocated);
    }
}

static gboolean
com_port_write_queued (SfiComPort *port)
{
  gint fd = port->pfd[1].fd;
  port->pfd[1].revents = 0;
  if (fd >= 0 && port->wbuffer.n)
    {
      gint n;
      do
	n = write (fd, port->wbuffer.data, MIN (port->wbuffer.n, IO_BOTTLE_NECK));
      while (n < 0 && errno == EINTR);
      if (n == 0 || (n < 0 && errno != EINTR && errno != EAGAIN && errno != ERESTART))
	return FALSE; /* connection broke */
      if (n > 0)
	{
	  port->wbuffer.n -= n;
	  memmove (port->wbuffer.data, port->wbuffer.data + n, port->wbuffer.n);
	}
    }
  return TRUE;	/* connection remains valid */
}

static gboolean
com_port_write (SfiComPort   *port,
		guint         n_bytes,
		const uint8  *bytes)
{
  int fd = port->pfd[1].fd;
  if (!com_port_write_queued (port))
    return FALSE; /* connection broke */
  if (fd >= 0 && !port->wbuffer.n)
    {
      int n;
      do
	n = write (fd, bytes, MIN (n_bytes, IO_BOTTLE_NECK));
      while (n < 0 && errno == EINTR);
      if (n == 0 || (n < 0 && errno != EINTR && errno != EAGAIN && errno != ERESTART))
	return FALSE;
      n = CLAMP (n, 0, int (n_bytes));
      n_bytes -= n;
      bytes += n;
    }
  if (n_bytes)
    {
      /* append to queued data */
      com_port_grow_wbuffer (port, n_bytes);
      memcpy (port->wbuffer.data + port->wbuffer.n, bytes, n_bytes);
      port->wbuffer.n += n_bytes;
    }
  return TRUE;  /* connection remains valid */
}

void
sfi_com_port_send_bulk (SfiComPort   *port,
                        SfiRing      *value_ring)
{
  SfiRing *ring;
  assert_return (port != NULL);
  if (!value_ring || !port->connected)
    return;
  assert_return (port->link || port->pfd[1].fd >= 0);

  if (port->link)
    {
      SfiComPortLink *link = port->link;
      gboolean first = port == link->port1;
      SfiRing *target = NULL;
      std::function<void()> wakeup;
      /* guard caller against receiver messing with value */
      for (ring = value_ring; ring; ring = sfi_ring_next (ring, value_ring))
        target = sfi_ring_append (target, sfi_value_clone_deep ((GValue*) ring->data));
      link->mutex.lock();
      if (first)
	link->p1queue = sfi_ring_concat (link->p1queue, target);
      else
	link->p2queue = sfi_ring_concat (link->p2queue, target);
      if (link->waiting)
        link->wcond.notify_one();
      else
	wakeup = first ? link->wakeup2 : link->wakeup1;
      link->mutex.unlock();
      CDEBUG ("[%s: sent values]", port->ident);
      if (wakeup)
        wakeup();
    }
  else
    for (ring = value_ring; ring; ring = sfi_ring_next (ring, value_ring))
      {
        const GValue *value = (GValue*) ring->data;
        /* preserve space for header */
        GString *gstring = g_string_new ("12345678");
        gchar *str;
        guint l;
        /* serialize value */
        sfi_value_store_typed (value, gstring);
        l = gstring->len - 8;
        str = g_string_free (gstring, FALSE);
        /* patch magic */
        str[0] = SFI_COM_PORT_MAGIC >> 24;
        str[1] = (SFI_COM_PORT_MAGIC >> 16) & 0xff;
        str[2] = (SFI_COM_PORT_MAGIC >> 8) & 0xff;
        str[3] = SFI_COM_PORT_MAGIC & 0xff;
        /* patch length */
        str[4] = l >> 24;
        str[5] = (l >> 16) & 0xff;
        str[6] = (l >> 8) & 0xff;
        str[7] = l & 0xff;
        /* write away */
        com_port_write (port, l + 8, (uint8*) str);
        g_free (str);
      }
}

void
sfi_com_port_send (SfiComPort   *port,
		   const GValue *value)
{
  SfiRing *ring;
  assert_return (port != NULL);
  assert_return (value != NULL);
  ring = sfi_ring_append (NULL, (GValue*) value);
  sfi_com_port_send_bulk (port, ring);
  sfi_ring_free (ring);
}

static gboolean
com_port_read_pending (SfiComPort *port)
{
  gint fd = port->pfd[0].fd;
  port->pfd[0].revents = 0;
  if (fd >= 0)
    {
      gint n;
      /* read header */
      if (port->rbuffer.hlen < 8)
	{
	  do
	    {
	      n = read (fd, port->rbuffer.header + port->rbuffer.hlen, 8 - port->rbuffer.hlen);
	      port->rbuffer.hlen += MAX (n, 0);
	    }
	  while (n < 0 && errno == EINTR);
	  /* n==0 on pipes/fifos means remote closed the connection (end-of-file) */
	  if (n == 0 || (n < 0 && errno != EINTR && errno != EAGAIN && errno != ERESTART))
	    {
	      CDEBUG ("%s: during read: remote pipe closed", port->ident);
	      return FALSE;
	    }
	  /* check completed header */
	  if (port->rbuffer.hlen == 8)
	    {
	      /* construct length */
	      port->rbuffer.dlen = port->rbuffer.header[4];
	      port->rbuffer.dlen <<= 8;
	      port->rbuffer.dlen |= port->rbuffer.header[5];
	      port->rbuffer.dlen <<= 8;
	      port->rbuffer.dlen |= port->rbuffer.header[6];
	      port->rbuffer.dlen <<= 8;
	      port->rbuffer.dlen |= port->rbuffer.header[7];
	      /* check magic */
	      if (port->rbuffer.header[0] != ((SFI_COM_PORT_MAGIC >> 24) & 0xff) ||
		  port->rbuffer.header[1] != ((SFI_COM_PORT_MAGIC >> 16) & 0xff) ||
		  port->rbuffer.header[2] != ((SFI_COM_PORT_MAGIC >> 8) & 0xff) ||
		  port->rbuffer.header[3] != (SFI_COM_PORT_MAGIC & 0xff))
		{
		  Bse::printerr ("ComPort:%s: received data with invalid magic", port->ident);
		  return FALSE;
		}
	      /* check length */
	      if (port->rbuffer.dlen < 1 || port->rbuffer.dlen > 10 * 1024 * 1024)
		{
		  Bse::printerr ("ComPort:%s: received data with excessive length", port->ident);
		  return FALSE;
		}
	    }
	}
      /* read data */
      if (port->rbuffer.hlen == 8 && port->rbuffer.n < port->rbuffer.dlen)
	{
	  /* grow buffer as necessary */
	  if (port->rbuffer.allocated < port->rbuffer.dlen)
	    {
	      port->rbuffer.allocated = port->rbuffer.dlen;
	      port->rbuffer.data = g_renew (guint8, port->rbuffer.data, port->rbuffer.allocated);
	    }
	  /* wire read */
	  do
	    {
	      n = read (fd, port->rbuffer.data + port->rbuffer.n, port->rbuffer.dlen - port->rbuffer.n);
	      port->rbuffer.n += MAX (n, 0);
	    }
	  while (n < 0 && errno == EINTR);
	  /* n==0 on pipes/fifos means remote closed the connection (end-of-file) */
	  if (n == 0 || (n < 0 && errno != EINTR && errno != EAGAIN && errno != ERESTART))
	    {
	      CDEBUG ("%s: during read: remote pipe closed", port->ident);
	      return FALSE;
	    }
	}
    }
  return TRUE;  /* connection remains valid */
}

static void
com_port_scanner_msg (GScanner *scanner,
		      gchar    *message,
		      gboolean  error)
{
  SfiComPort *port = (SfiComPort*) scanner->user_data;
  Bse::printerr ("ComPort:%s: while processing data: %s", port->ident, message);
}

static void
sfi_com_port_deserialize (SfiComPort *port)
{
  if (!port->scanner && port->rbuffer.hlen)
    {
      port->scanner = g_scanner_new64 (sfi_storage_scanner_config);
      port->scanner->input_name = NULL;
      port->scanner->parse_errors = 0;
      port->scanner->user_data = port;
      port->scanner->msg_handler = com_port_scanner_msg;
    }
  if (port->rbuffer.hlen == 8 && port->rbuffer.n >= port->rbuffer.dlen)
    {
      GValue *value = sfi_value_empty ();
      GTokenType token;
      g_scanner_input_text (port->scanner, (const char*) port->rbuffer.data, port->rbuffer.n);
      token = sfi_value_parse_typed (value, port->scanner);
      if (token == G_TOKEN_NONE)
	{
	  g_scanner_input_text (port->scanner, NULL, 0);
	  port->rvalues = sfi_ring_append (port->rvalues, value);
	}
      else
	{
	  sfi_value_free (value);
	  g_scanner_unexp_token (port->scanner, token, NULL, NULL, NULL, "aborting...", TRUE);
	  sfi_com_port_close_remote (port, FALSE);
	}
      port->rbuffer.n = 0;
      port->rbuffer.hlen = 0;
    }
}

static GValue*
sfi_com_port_recv_intern (SfiComPort *port,
			  gboolean    blocking)
{
  CDEBUG ("[%s: START receiving]", port->ident);
  if (!port->rvalues && port->link)
    {
      SfiComPortLink *link = port->link;
      std::unique_lock<std::mutex> link_lock (link->mutex);
    refetch:
      if (port == link->port1)
	port->rvalues = link->p2queue, link->p2queue = NULL;
      else
	port->rvalues = link->p1queue, link->p1queue = NULL;
      if (!port->rvalues && blocking)
	{
	  link->waiting = TRUE;
	  link->wcond.wait (link_lock);
	  link->waiting = FALSE;
	  goto refetch;
	}
    }
  else if (!port->rvalues)
    {
    loop_blocking:
      if (blocking &&   /* flush output buffer if data is pending */
          !com_port_write_queued (port))
        sfi_com_port_close_remote (port, FALSE);

      if (!port->rvalues)
        {
	  if (!com_port_read_pending (port))
            sfi_com_port_close_remote (port, FALSE);
          sfi_com_port_deserialize (port);
        }

      if (blocking && !port->rvalues && port->pfd[0].fd >= 0)
        {
          struct timeval tv = { 60, 0, };
          fd_set in_fds, out_fds, exp_fds;
          gint xfd;

          FD_ZERO (&in_fds);
          FD_ZERO (&out_fds);
          FD_ZERO (&exp_fds);
          FD_SET (port->pfd[0].fd, &in_fds);      /* select for read() */
          FD_SET (port->pfd[0].fd, &exp_fds);
          xfd = port->pfd[0].fd;
          if (port->wbuffer.n &&                  /* if data is pending... */
              port->pfd[1].fd >= 0)
            {
              FD_SET (port->pfd[1].fd, &out_fds); /* select for write() */
              FD_SET (port->pfd[1].fd, &exp_fds);
              xfd = MAX (xfd, port->pfd[1].fd);
            }
          /* do the actual blocking */
          select (xfd + 1, &in_fds, &out_fds, &exp_fds, &tv);
          /* block only once so higher layers may handle signals */
          blocking = FALSE;
          goto loop_blocking;
        }
    }
  CDEBUG ("[%s: DONE receiving]", port->ident);
  return port->connected ? (GValue*) sfi_ring_pop_head (&port->rvalues) : NULL;
}

GValue*
sfi_com_port_recv (SfiComPort *port)
{
  assert_return (port != NULL, NULL);
  if (!port->connected)
    return NULL;
  assert_return (port->link || port->pfd[0].fd >= 0, NULL);

  return sfi_com_port_recv_intern (port, FALSE);
}

GValue*
sfi_com_port_recv_blocking (SfiComPort *port)
{
  assert_return (port != NULL, NULL);
  if (!port->connected)
    return NULL;
  assert_return (port->link || port->pfd[0].fd >= 0, NULL);

  return sfi_com_port_recv_intern (port, TRUE);
}

GPollFD*
sfi_com_port_get_poll_fds (SfiComPort *port,
			   guint      *n_pfds)
{
  GPollFD *pfds = NULL;
  guint n = 0;

  assert_return (port != NULL, NULL);
  assert_return (n_pfds != NULL, NULL);

  if (port->pfd[1].fd >= 0)
    {
      n++;
      pfds = &port->pfd[1];
    }
  if (port->pfd[0].fd >= 0)
    {
      n++;
      pfds = &port->pfd[0];
    }
  *n_pfds = n;
  return n ? pfds : NULL;
}

gboolean
sfi_com_port_io_pending (SfiComPort *port)
{
  assert_return (port != NULL, FALSE);

  /* maintain poll fds */
  port->pfd[0].events = port->pfd[0].fd >= 0 ? G_IO_IN : 0;
  port->pfd[1].events = port->pfd[1].fd >= 0 && port->wbuffer.n ? G_IO_OUT : 0;

  /* check link queue */
  if (port->link &&
      ((port == port->link->port1 && port->link->p2queue) ||
       (port == port->link->port2 && port->link->p1queue)))
    return TRUE;

  /* check input channel */
  if (port->pfd[0].fd >= 0 &&
      port->pfd[0].revents & G_IO_IN)
    return TRUE;

  /* check output channel */
  if (port->pfd[1].fd >= 0 && port->wbuffer.n &&
      port->pfd[1].revents & G_IO_OUT)
    return TRUE;

  /* nothing to do */
  return FALSE;
}

void
sfi_com_port_process_io (SfiComPort *port)
{
  assert_return (port != NULL);

  if (!com_port_read_pending (port) ||
      !com_port_write_queued (port))
    sfi_com_port_close_remote (port, FALSE);
  if (port->connected)
    sfi_com_port_deserialize (port);
}

void
sfi_com_port_reap_child (SfiComPort *port,
                         gboolean    kill_child)
{
  assert_return (port != NULL);

  com_port_try_reap (port, !kill_child);
  if (kill_child &&
      port->remote_pid > 1 &&
      !port->reaped &&
      !port->sigkill_sent)
    {
      if (kill (port->remote_pid, SIGKILL) >= 0)
        port->sigkill_sent = TRUE;
    }
  com_port_try_reap (port, TRUE);
}

gboolean
sfi_com_port_test_reap_child (SfiComPort *port)
{
  assert_return (port != NULL, FALSE);
  com_port_try_reap (port, FALSE);
  return port->reaped;
}
