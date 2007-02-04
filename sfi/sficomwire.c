/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "sficomwire.h"
#include "sfiprimitives.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>


/* --- prototypes --- */
static GList*	wire_find_link	(GList	*list,
				 guint	 request);


/* --- functions --- */
static void
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
}

SfiComWire*
sfi_com_wire_from_child (const gchar *ident,
			 gint         remote_input,
			 gint         remote_output,
			 gint         standard_input,
			 gint         standard_output,
			 gint         standard_error,
			 gint         remote_pid)
{
  SfiComWire *wire;
  
  g_return_val_if_fail (ident != NULL, NULL);
  
  wire = g_new0 (SfiComWire, 1);
  if (remote_pid > 1)
    wire->ident = g_strdup_printf ("%s[%u]", ident, remote_pid);
  else
    wire->ident = g_strdup (ident);
  wire->remote_input = remote_input;
  wire->remote_output = remote_output;
  wire->standard_input = standard_input;
  wire->standard_output = standard_output;
  wire->standard_error = standard_error;
  wire->remote_pid = remote_pid > 1 ? remote_pid : -1;
  wire->gstring_stdout = g_string_new ("");
  wire->gstring_stderr = g_string_new ("");
  wire->connected = (wire->remote_input >= 0 ||
		     wire->remote_output >= 0 ||
		     wire->standard_input >= 0 ||
		     wire->standard_output >= 0 ||
		     wire->standard_error >= 0);
  sfi_com_wire_set_dispatcher (wire, NULL, NULL, NULL);
  nonblock_fd (wire->remote_input);
  nonblock_fd (wire->remote_output);
  nonblock_fd (wire->standard_input);
  nonblock_fd (wire->standard_output);
  nonblock_fd (wire->standard_error);
  
  return wire;
}

SfiComWire*
sfi_com_wire_from_pipe (const gchar *ident,
			gint         remote_input,
			gint         remote_output)
{
  g_return_val_if_fail (ident != NULL, NULL);
  
  return sfi_com_wire_from_child (ident,
				  remote_input,
				  remote_output,
				  -1, -1, -1, -1);
}

static SfiComMsg*
alloc_msg (SfiComMsgType type)
{
  SfiComMsg *msg = g_new (SfiComMsg, 1);
  
  msg->magic = BSE_MAGIC_BSEm;
  msg->mlength = 0;
  msg->type = type;
  
  return msg;
}

static gchar*
free_msg_skel (SfiComMsg *msg)
{
  gchar *content = msg->message;
  
  g_free (msg);
  return content;
}

static void
free_msg (SfiComMsg *msg)
{
  g_free (free_msg_skel (msg));
}

static void
wire_write_remote (SfiComWire *wire)
{
  guint8 *buf = wire->obuffer;
  
  if (wire->obp - buf && wire->remote_output >= 0)
    {
      gint n;
      
      do
	{
	  n = write (wire->remote_output, buf, wire->obp - buf);
	  buf += MAX (n, 0);
	}
      while (n < 0 && errno == EINTR);
      
      if (n == 0 || (n < 0 && errno != EINTR && errno != EAGAIN))
	wire->remote_output_broke = TRUE;
      
      n = wire->obp - buf;
      g_memmove (wire->obuffer, buf, n);
      wire->obp = wire->obuffer + n;
    }
}

static inline gpointer
put_uint32 (gpointer p,
	    guint32  val)
{
  guint32 *ip = p;
  
  *ip++ = GUINT32_TO_BE (val);
  return ip;
}

static void
wire_send (SfiComWire *wire,
	   SfiComMsg  *msg)
{
  guint strl;
  
  g_return_if_fail (msg->mlength == 0);
  
  strl = strlen (msg->message) + 1;	/* include trailing 0 */
  msg->mlength = (4 +	/* magic */
		  4 + 	/* mlength */
		  4 +	/* type */
		  4 +	/* request */
		  strl);
  if (wire->obp + msg->mlength >= wire->obound)
    {
      guint l = wire->obp - wire->obuffer;
      wire->obuffer = g_renew (guint8, wire->obuffer, l + msg->mlength);
      wire->obp = wire->obuffer + l;
      wire->obound = wire->obp + msg->mlength;
    }
  wire->obp = put_uint32 (wire->obp, msg->magic);
  wire->obp = put_uint32 (wire->obp, msg->mlength);
  wire->obp = put_uint32 (wire->obp, msg->type);
  wire->obp = put_uint32 (wire->obp, msg->request);
  memcpy (wire->obp, msg->message, strl);
  wire->obp += strl;
  wire_write_remote (wire);
}

static void
wire_read_remote (SfiComWire *wire)
{
  if (wire->remote_input >= 0)
    {
      guint read_size = 8192;
      gint n;
      
      if (wire->ibound - wire->ibp < read_size)
	{
	  guint l = wire->ibp - wire->ibuffer;
	  wire->ibuffer = g_renew (guint8, wire->ibuffer, l + read_size);
	  wire->ibp = wire->ibuffer + l;
	  wire->ibound = wire->ibp + read_size;
	}
      
      do
	{
	  n = read (wire->remote_input, wire->ibp, wire->ibound - wire->ibp);
	  wire->ibp += MAX (n, 0);
	}
      while (n < 0 && errno == EINTR);
      
      /* n==0 on pipes/fifos means remote closed the connection (end-of-file) */
      if (n == 0 || (n < 0 && errno != EINTR && errno != EAGAIN))
	wire->remote_input_broke = TRUE;
    }
}

static inline gpointer
get_uint32 (gpointer p,
	    guint32 *val)
{
  guint32 *ip = p, v;
  
  v = *ip++;
  *val = GUINT32_FROM_BE (v);
  return ip;
}

static void
wire_receive (SfiComWire *wire)
{
  wire_read_remote (wire);
  
  if (wire->ibp >= wire->ibuffer + 4 + 4 + 4)	/* magic + mlength + type */
    {
      guint8 *p = wire->ibuffer;
      guint32 magic, mlength, type;
      guint mheader_length = 4 + 4 + 4 + 4, max_mlength = 4 * 1024 * 1024;
      
      p = get_uint32 (p, &magic);
      p = get_uint32 (p, &mlength);
      p = get_uint32 (p, &type);
      if (magic != BSE_MAGIC_BSEm)
	{
	  g_printerr ("%s: message with invalid magic: 0x%08x\n", wire->ident, magic);
	  wire->remote_input_broke = TRUE;
	  wire->ibp = wire->ibuffer;
	}
      else if (mlength <= mheader_length || mlength >= max_mlength)
	{
	  g_printerr ("%s: message (type=%u) with invalid length: %u < %u < %u\n",
		      wire->ident, type, mheader_length, mlength, max_mlength);
	  wire->remote_input_broke = TRUE;
	  wire->ibp = wire->ibuffer;
	}
      else if (mlength <= wire->ibp - wire->ibuffer)
	{
	  guint strl = mlength - mheader_length;	/* >= 1 */
	  
	  switch (type)
	    {
	      SfiComMsg *msg;
	      guint n;
	    case SFI_COM_MSG_REQUEST:
	    case SFI_COM_MSG_RESULT:
	      msg = alloc_msg (type);
	      msg->mlength = mlength;
	      p = get_uint32 (p, &msg->request);
	      msg->message = g_new (gchar, strl);
	      memcpy (msg->message, p, strl - 1);	/* ignoring trailing 0 */
	      msg->message[strl - 1] = 0;
	      p += strl;
	      if (type == SFI_COM_MSG_REQUEST)
		wire->irequests = g_list_append (wire->irequests, msg);
	      else
		{
		  if (wire_find_link (wire->orequests, msg->request))
		    wire->iresults = g_list_prepend (wire->iresults, msg);
		  else
		    {
		      g_printerr ("%s: ignoring spurious result (request=%u): %s\n", wire->ident, msg->request, msg->message);
		      free_msg (msg);
		    }
		}
	      n = wire->ibp - p;
	      g_memmove (wire->ibuffer, p, n);
	      wire->ibp = wire->ibuffer + n;
	      break;
	    case SFI_COM_MSG_RESERVED1:
	    case SFI_COM_MSG_RESERVED2:
	    case SFI_COM_MSG_RESERVED3:
	    case SFI_COM_MSG_RESERVED4:
	      g_printerr ("%s: ignoring message with unknown type: %u\n",
			  wire->ident, type);
	      p += 4;	/* request */
	      p += strl;
	      n = wire->ibp - p;
	      g_memmove (wire->ibuffer, p, n);
	      wire->ibp = wire->ibuffer + n;
	      break;
	    default:
	      g_printerr ("%s: message with invalid type: %u\n",
			  wire->ident, type);
	      wire->remote_input_broke = TRUE;
	      wire->ibp = wire->ibuffer;
	      break;
	    }
	}
    }
}

static inline gboolean	/* returns: connection_alive */
wire_read_gstring (SfiComWire *wire,
		   gint        fd,
		   GString    *gstring)
{
  guint l = gstring->len;
  guint8 *pos, *bound;
  gint n;
  
  g_string_set_size (gstring, l + 8192);
  pos = gstring->str + l;
  bound = gstring->str + gstring->len;
  do
    {
      n = read (fd, pos, bound - pos);
      pos += MAX (n, 0);
    }
  while (n < 0 && errno == EINTR);
  g_string_set_size (gstring, pos - (guint8*) gstring->str);
  
  /* n==0 on pipes/fifos means remote closed the connection (end-of-file) */
  return n > 0 || (n < 0 && (errno == EINTR || errno == EAGAIN));
}

static void
wire_capture (SfiComWire *wire)
{
  if (wire->standard_output >= 0)
    if (!wire_read_gstring (wire, wire->standard_output, wire->gstring_stdout))
      wire->standard_output_broke = TRUE;
  if (wire->standard_error >= 0)
    if (!wire_read_gstring (wire, wire->standard_error, wire->gstring_stderr))
      wire->standard_error_broke = TRUE;
}

static inline void
wire_update_alive (SfiComWire *wire)
{
  if (wire->remote_input_broke ||
      wire->remote_output_broke ||
      wire->standard_input_broke ||
      wire->standard_output_broke ||
      wire->standard_error_broke)
    wire->connected = FALSE;
}

static GList*
wire_find_link (GList *list,
		guint  request)
{
  for (; list; list = list->next)
    {
      SfiComMsg *msg = list->data;
      
      if (msg->request == request)
	return list;
    }
  return NULL;
}

static guint
wire_alloc_request (SfiComWire *wire)
{
  guint request = (rand () << 16) ^ rand ();
  
  while (request == 0 || wire_find_link (wire->orequests, request))
    request++;
  
  return request;
}

guint
sfi_com_wire_send_request (SfiComWire  *wire,
			   const gchar *request_msg)
{
  SfiComMsg *msg;
  guint request;
  
  g_return_val_if_fail (wire != NULL, 0);
  g_return_val_if_fail (request_msg != NULL, 0);
  
  request = wire_alloc_request (wire);
  msg = alloc_msg (SFI_COM_MSG_REQUEST);
  msg->request = request;
  msg->message = g_strdup (request_msg);
  
  wire->orequests = g_list_prepend (wire->orequests, msg);
  wire_send (wire, msg);
  
  wire_update_alive (wire);
  
  return request;
}

gchar*
sfi_com_wire_receive_result (SfiComWire *wire,
			     guint       request)
{
  GList *out_link, *in_link;
  
  g_return_val_if_fail (wire != NULL, NULL);
  g_return_val_if_fail (request > 0, NULL);
  out_link = wire_find_link (wire->orequests, request);
  g_return_val_if_fail (out_link != NULL, NULL);
  
  wire_receive (wire);
  wire_update_alive (wire);
  
  in_link = wire_find_link (wire->iresults, request);
  if (in_link)
    {
      SfiComMsg *omsg = out_link->data;
      SfiComMsg *imsg = in_link->data;
      
      wire->orequests = g_list_delete_link (wire->orequests, out_link);
      wire->iresults = g_list_delete_link (wire->iresults, in_link);
      free_msg (omsg);
      return free_msg_skel (imsg);
    }
  else
    return NULL;
}

void
sfi_com_wire_forget_request (SfiComWire *wire,
			     guint       request)
{
  GList *out_link;
  SfiComMsg *omsg;
  
  g_return_if_fail (wire != NULL);
  g_return_if_fail (request > 0);
  out_link = wire_find_link (wire->orequests, request);
  g_return_if_fail (out_link != NULL);
  
  omsg = out_link->data;
  wire->orequests = g_list_delete_link (wire->orequests, out_link);
  free_msg (omsg);
}

guint
sfi_com_wire_peek_first_result (SfiComWire *wire)
{
  SfiComMsg *msg;
  
  g_return_val_if_fail (wire != NULL, 0);
  
  msg = wire->iresults ? wire->iresults->data : NULL;
  return msg ? msg->request : 0;
}

const gchar*
sfi_com_wire_receive_request (SfiComWire *wire,
			      guint      *request_p)
{
  g_return_val_if_fail (wire != NULL, NULL);
  g_return_val_if_fail (request_p != NULL, NULL);
  
  wire_receive (wire);
  wire_update_alive (wire);
  
  if (wire->irequests)
    {
      SfiComMsg *msg = wire->irequests->data;
      
      wire->irequests = g_list_remove (wire->irequests, msg);
      if (msg->request == 0)
	{
	  /* 0-requests are low-level messages, currently unhandled */
	  g_printerr ("%s: ignoring message with request_id=0\n", wire->ident);
	  free_msg (msg);
	  return sfi_com_wire_receive_request (wire, request_p);
	}
      wire->rrequests = g_list_prepend (wire->rrequests, msg);
      *request_p = msg->request;
      
      return msg->message;
    }
  else
    {
      *request_p = 0;
      return NULL;
    }
}

void
sfi_com_wire_send_result (SfiComWire  *wire,
			  guint        request,
			  const gchar *result_msg)
{
  SfiComMsg *msg;
  GList *received_link;
  
  g_return_if_fail (wire != NULL);
  g_return_if_fail (request > 0);
  g_return_if_fail (result_msg != NULL);
  received_link = wire_find_link (wire->rrequests, request);
  g_return_if_fail (received_link != NULL);
  
  msg = alloc_msg (SFI_COM_MSG_RESULT);
  msg->request = request;
  msg->message = g_strdup (result_msg);
  wire_send (wire, msg);
  
  free_msg (received_link->data);
  wire->rrequests = g_list_delete_link (wire->rrequests, received_link);
  free_msg (msg);
  
  wire_update_alive (wire);
}

void
sfi_com_wire_discard_request (SfiComWire *wire,
			      guint       request)
{
  GList *received_link;
  
  g_return_if_fail (wire != NULL);
  g_return_if_fail (request > 0);
  received_link = wire_find_link (wire->rrequests, request);
  g_return_if_fail (received_link != NULL);
  
  free_msg (received_link->data);
  wire->rrequests = g_list_delete_link (wire->rrequests, received_link);
  
  wire_update_alive (wire);
}

static gboolean
wire_default_dispatch (gpointer     data,
		       guint        request,
		       const gchar *request_msg,
		       SfiComWire  *wire)
{
  g_printerr ("%s: unhandled request (id=%u): %s\n", wire->ident, request, request_msg);
  sfi_com_wire_discard_request (wire, request);
  return TRUE;
}

void
sfi_com_wire_set_dispatcher (SfiComWire    *wire,
			     SfiComDispatch dispatch_func,
			     gpointer       dispatch_data,
			     GDestroyNotify destroy_data)
{
  g_return_if_fail (wire != NULL);
  
  if (wire->destroy_data)
    wire->destroy_data (wire->dispatch_data);
  if (dispatch_func)
    {
      wire->dispatch_func = dispatch_func;
      wire->dispatch_data = dispatch_data;
      wire->destroy_data = destroy_data;
    }
  else
    {
      wire->dispatch_func = wire_default_dispatch;
      wire->dispatch_data = NULL;
      wire->destroy_data = NULL;
    }
}

void
sfi_com_wire_dispatch (SfiComWire  *wire,
		       guint        request)
{
  GList *received_link;
  SfiComMsg *msg;
  gboolean handled;
  
  g_return_if_fail (wire != NULL);
  g_return_if_fail (request > 0);
  received_link = wire_find_link (wire->rrequests, request);
  g_return_if_fail (received_link != NULL);
  
  msg = received_link->data;
  handled = wire->dispatch_func (wire->dispatch_data, msg->request, msg->message, wire);
  if (!handled)
    wire_default_dispatch (NULL, msg->request, msg->message, wire);
}

gboolean
sfi_com_wire_need_dispatch (SfiComWire *wire)
{
  g_return_val_if_fail (wire != NULL, FALSE);
  
  return wire->iresults || wire->irequests || wire->gstring_stdout->len || wire->gstring_stderr->len;
}

gint*
sfi_com_wire_get_read_fds (SfiComWire *wire,
			   guint      *n_fds_p)
{
  g_return_val_if_fail (wire != NULL, NULL);
  g_return_val_if_fail (n_fds_p != NULL, NULL);
  
  if (wire->remote_input >= 0 ||
      wire->standard_output >= 0 ||
      wire->standard_error >= 0)
    {
      guint n_fds = 0;
      gint *fds = g_new (gint, 3);
      
      if (wire->remote_input >= 0)
	fds[n_fds++] = wire->remote_input;
      if (wire->standard_output >= 0)
	fds[n_fds++] = wire->standard_output;
      if (wire->standard_error >= 0)
	fds[n_fds++] = wire->standard_error;
      *n_fds_p = n_fds;
      return fds;
    }
  else
    {
      *n_fds_p = 0;
      return NULL;
    }
}

gint*
sfi_com_wire_get_write_fds (SfiComWire *wire,
			    guint      *n_fds_p)
{
  g_return_val_if_fail (wire != NULL, NULL);
  g_return_val_if_fail (n_fds_p != NULL, NULL);
  
  if (wire->obp - wire->obuffer && wire->remote_output >= 0)
    {
      guint n_fds = 0;
      gint *fds = g_new (gint, 1);
      
      fds[n_fds++] = wire->remote_output;
      *n_fds_p = n_fds;
      return fds;
    }
  else
    {
      *n_fds_p = 0;
      return NULL;
    }
}

GPollFD*
sfi_com_wire_get_poll_fds (SfiComWire *wire,
			   guint      *n_pfds_p)
{
  g_return_val_if_fail (wire != NULL, NULL);
  g_return_val_if_fail (n_pfds_p != NULL, NULL);
  
  if (wire->remote_input >= 0 ||
      wire->standard_output >= 0 ||
      wire->standard_error >= 0 ||
      wire->remote_output >= 0)
    {
      guint n_pfds = 0;
      GPollFD *pfds = g_new0 (GPollFD, 3 + 1);
      
      if (wire->remote_input >= 0)
	{
	  pfds[n_pfds].fd = wire->remote_input;
	  pfds[n_pfds++].events = G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
	}
      if (wire->standard_output >= 0)
	{
	  pfds[n_pfds].fd = wire->standard_output;
	  pfds[n_pfds++].events = G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
	}
      if (wire->standard_error >= 0)
	{
	  pfds[n_pfds].fd = wire->standard_error;
	  pfds[n_pfds++].events = G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
	}
      if (wire->remote_output >= 0)
	{
	  pfds[n_pfds].fd = wire->remote_output;
	  pfds[n_pfds].events = G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL;
	  if (wire->obp - wire->obuffer)
	    pfds[n_pfds].events |= G_IO_OUT;
	  n_pfds++;
	}
      *n_pfds_p = n_pfds;
      return pfds;
    }
  else
    {
      *n_pfds_p = 0;
      return NULL;
    }
}

void
sfi_com_wire_process_io (SfiComWire *wire)
{
  g_return_if_fail (wire != NULL);
  
  wire_capture (wire);
  wire_write_remote (wire);
  wire_read_remote (wire);
  wire_capture (wire);
  
  if (wire->remote_input_broke)
    {
      if (wire->remote_input >= 0)
	close (wire->remote_input);
      wire->remote_input = -1;
    }
  if (wire->remote_output_broke)
    {
      if (wire->remote_output >= 0)
	close (wire->remote_output);
      wire->remote_output = -1;
    }
  if (wire->standard_input_broke)
    {
      if (wire->standard_input >= 0)
	close (wire->standard_input);
      wire->standard_input = -1;
    }
  if (wire->standard_output_broke)
    {
      if (wire->standard_output >= 0)
	close (wire->standard_output);
      wire->standard_output = -1;
    }
  if (wire->standard_error_broke)
    {
      if (wire->standard_error >= 0)
	close (wire->standard_error);
      wire->standard_error = -1;
    }
}

void
sfi_com_wire_close_remote (SfiComWire *wire,
			   gboolean    terminate)
{
  g_return_if_fail (wire != NULL);
  
  wire->connected = FALSE;
  if (wire->remote_input >= 0)
    close (wire->remote_input);
  wire->remote_input = -1;
  if (wire->remote_output >= 0)
    close (wire->remote_output);
  wire->remote_output = -1;
  if (wire->standard_input >= 0)
    close (wire->standard_input);
  wire->standard_input = -1;
  if (wire->standard_output >= 0)
    close (wire->standard_output);
  wire->standard_output = -1;
  if (wire->standard_error >= 0)
    close (wire->standard_error);
  wire->standard_error = -1;
  if (wire->remote_pid > 1 && terminate)
    kill (wire->remote_pid, SIGTERM);
  wire->remote_pid = -1;
}

void
sfi_com_wire_destroy (SfiComWire *wire)
{
  GList *list;
  
  g_return_if_fail (wire != NULL);
  
  sfi_com_wire_set_dispatcher (wire, NULL, NULL, NULL);
  sfi_com_wire_close_remote (wire, TRUE);
  for (list = wire->orequests; list; list = list->next)
    free_msg (list->data);
  g_list_free (wire->orequests);
  for (list = wire->iresults; list; list = list->next)
    free_msg (list->data);
  g_list_free (wire->iresults);
  for (list = wire->irequests; list; list = list->next)
    free_msg (list->data);
  g_list_free (wire->irequests);
  for (list = wire->rrequests; list; list = list->next)
    free_msg (list->data);
  g_list_free (wire->rrequests);
  g_string_free (wire->gstring_stdout, TRUE);
  g_string_free (wire->gstring_stderr, TRUE);
  g_free (wire->ibuffer);
  g_free (wire->obuffer);
  g_free (wire->ident);
  g_free (wire);
}

gboolean
sfi_com_wire_receive_dispatch (SfiComWire *wire)
{
  guint request;
  
  g_return_val_if_fail (wire != NULL, FALSE);
  
  if (sfi_com_wire_receive_request (wire, &request))
    {
      sfi_com_wire_dispatch (wire, request);
      return TRUE;
    }
  else
    return FALSE;
}

void
sfi_com_wire_select (SfiComWire *wire,
		     guint       timeout)
{
  fd_set rfds, wfds, efds;
  guint *fds, i, n, max_fd = 0;
  struct timeval tv;
  
  g_return_if_fail (wire != NULL);
  
  FD_ZERO (&rfds);
  FD_ZERO (&wfds);
  FD_ZERO (&efds);
  
  fds = sfi_com_wire_get_read_fds (wire, &n);
  for (i = 0; i < n; i++)
    {
      FD_SET (fds[i], &rfds);
      FD_SET (fds[i], &efds);
      max_fd = MAX (max_fd, fds[i]);
    }
  g_free (fds);
  
  fds = sfi_com_wire_get_write_fds (wire, &n);
  for (i = 0; i < n; i++)
    {
      FD_SET (fds[i], &wfds);
      FD_SET (fds[i], &efds);
      max_fd = MAX (max_fd, fds[i]);
    }
  g_free (fds);
  
  tv.tv_usec = (timeout % 1000) * 1000;
  tv.tv_sec = timeout / 1000;
  select (max_fd + 1, &rfds, &wfds, NULL, &tv);
}

gchar*
sfi_com_wire_ping_pong (SfiComWire  *wire,
			const gchar *ping,
			guint        timeout)
{
  guint request;
  gchar *pong;
  
  g_return_val_if_fail (wire != NULL, NULL);
  g_return_val_if_fail (ping != NULL, NULL);
  
  request = sfi_com_wire_send_request (wire, ping);
  pong = sfi_com_wire_receive_result (wire, request);
  if (pong)
    return pong;
  
  sfi_com_wire_select (wire, timeout / 4);
  sfi_com_wire_process_io (wire);
  pong = sfi_com_wire_receive_result (wire, request);
  if (pong)
    return pong;
  
  sfi_com_wire_select (wire, timeout / 4);
  sfi_com_wire_process_io (wire);
  pong = sfi_com_wire_receive_result (wire, request);
  if (pong)
    return pong;
  
  sfi_com_wire_select (wire, timeout / 4);
  sfi_com_wire_process_io (wire);
  pong = sfi_com_wire_receive_result (wire, request);
  if (pong)
    return pong;
  
  sfi_com_wire_select (wire, timeout / 4);
  sfi_com_wire_process_io (wire);
  pong = sfi_com_wire_receive_result (wire, request);
  if (pong)
    return pong;
  
  sfi_com_wire_forget_request (wire, request);
  return NULL;
}


/* --- fork/exec --- */
static gchar *spawn_current_dir = NULL;

void
sfi_com_set_spawn_dir (const gchar *cwd)
{
  g_free (spawn_current_dir);
  spawn_current_dir = g_strdup (cwd);
}

static void
unset_cloexec (gint fd)
{
  gint r;
  
  do
    r = fcntl (fd, F_SETFD, 0 /* FD_CLOEXEC */);
  while (r < 0 && errno == EINTR);
}

typedef struct {
  gint keepexec1;
  gint keepexec2;
} ChildSetupData;

static void
pre_exec_child_setup (gpointer data)
{
  ChildSetupData *cdata = data;
  
  if (cdata->keepexec1)
    unset_cloexec (cdata->keepexec1);
  if (cdata->keepexec2)
    unset_cloexec (cdata->keepexec2);
  /* drop scheduling priorities if we have any */
  setpriority (PRIO_PROCESS, getpid(), 0);
}

gchar*
sfi_com_spawn_async (const gchar *executable,
		     gint        *child_pid,
		     gint        *standard_input,	/* writable */
		     gint        *standard_output,	/* readable */
		     gint        *standard_error,	/* readable */
		     const gchar *command_fd_option,
		     gint        *command_input,	/* writable */
		     gint        *command_output,	/* readable */
		     SfiRing     *args)
{
  gint command_input_pipe[2] = { -1, -1 };
  gint command_output_pipe[2] = { -1, -1 };
  ChildSetupData setup_data = { -1, -1 };
  SfiRing *ring, *cargs = NULL;
  gchar **argv, **argp, *reterr = NULL;
  GError *error = NULL;
  guint l;
  
  g_return_val_if_fail (executable != NULL, NULL);
  if (command_fd_option)
    g_return_val_if_fail (command_fd_option && command_input && command_output, NULL);
  else
    g_return_val_if_fail (!command_fd_option && !command_input && !command_output, NULL);
  
  if (command_fd_option)
    {
      if (pipe (command_output_pipe) < 0 || pipe (command_input_pipe) < 0)
	{
	  gint e = errno;
	  
	  if (command_output_pipe[0] >= 0)
	    {
	      close (command_output_pipe[0]);
	      close (command_output_pipe[1]);
	    }
	  
	  return g_strdup_printf ("failed to create communication channels: %s", g_strerror (e));
	}
      cargs = sfi_ring_prepend (cargs, g_strdup_printf ("%u", command_output_pipe[1]));
      cargs = sfi_ring_prepend (cargs, g_strdup_printf ("%u", command_input_pipe[0]));
      if (command_fd_option[0])
	cargs = sfi_ring_prepend (cargs, g_strdup (command_fd_option));
      setup_data.keepexec1 = command_output_pipe[1];
      setup_data.keepexec2 = command_input_pipe[0];
    }
  cargs = sfi_ring_prepend (cargs, g_strdup_printf (/*"SFI-Spawn:%s"*/"%s", executable));
  cargs = sfi_ring_prepend (cargs, g_strdup (executable));
  
  l = sfi_ring_length (cargs) + sfi_ring_length (args);
  argv = g_new (gchar*, l + 1);
  argp = argv;
  for (ring = cargs; ring; ring = sfi_ring_walk (ring, cargs))
    *argp++ = ring->data;
  for (ring = args; ring; ring = sfi_ring_walk (ring, args))
    *argp++ = ring->data;
  *argp = NULL;
  
  if (!g_spawn_async_with_pipes (spawn_current_dir, argv, NULL,
                                 G_SPAWN_DO_NOT_REAP_CHILD |
                                 /* G_SPAWN_CHILD_INHERITS_STDIN | */
				 G_SPAWN_FILE_AND_ARGV_ZERO,
				 pre_exec_child_setup, &setup_data,
				 child_pid,
				 standard_input,
				 standard_output,
				 standard_error,
				 &error))
    {
      reterr = error ? error->message : "failed to spawn child process";
      reterr = g_strdup (reterr);
      g_clear_error (&error);
      close (command_output_pipe[0]);	command_output_pipe[0] = -1;
      close (command_output_pipe[1]);	command_output_pipe[1] = -1;
      close (command_input_pipe[0]);	command_input_pipe[0] = -1;
      close (command_input_pipe[1]);	command_input_pipe[1] = -1;
      if (child_pid)
	*child_pid = 0;
      if (standard_input)
	*standard_input = -1;
      if (standard_output)
	*standard_output = -1;
      if (standard_error)
	*standard_error = -1;
      goto cleanup;
    }
  
 cleanup:
  g_free (argv);
  for (ring = cargs; ring; ring = sfi_ring_walk (ring, cargs))
    g_free (ring->data);
  sfi_ring_free (cargs);
  if (command_fd_option)
    {
      if (command_output_pipe[1] >= 0)
	{
	  close (command_output_pipe[1]);
	  close (command_input_pipe[0]);
	}
      *command_input = command_input_pipe[1];
      *command_output = command_output_pipe[0];
    }
  
  return reterr;
}
