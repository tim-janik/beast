/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __SFI_GLUE_CODEC_H__
#define __SFI_GLUE_CODEC_H__

#include <sfi/sfiglue.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- server side API --- */
/* incomming messages (requests) are decoded and
 * dispatched through the glue layer, and the glue layer results
 * are encoded and returned via sfi_glue_codec_process().
 * server side events are encoded and need to be handled via
 * SfiGlueCodecSendEvent() (these usually result from signal
 * emissions of the glue layer). the events are decoded and
 * processed via the client side API.
 * client messages are, before being passed on to the glue
 * layer, filtered through SfiGlueCodecClientMsg() and may
 * be intercepted.
 */
typedef struct _SfiGlueCodec                    SfiGlueCodec;
typedef void         (*SfiGlueCodecSendEvent)  (SfiGlueCodec *code,
					        gpointer     user_data,
					        const gchar  *message);
typedef GValue*      (*SfiGlueCodecClientMsg)  (SfiGlueCodec *code,
					        gpointer      user_data,
					        const gchar  *message,
					        GValue       *value,
					        gboolean     *handled);
struct _SfiGlueCodec
{
  gpointer		user_data;
  SfiGlueContext       *context;
  SfiGlueCodecSendEvent	send_event;
  SfiGlueCodecClientMsg client_msg;
  GDestroyNotify	destroy;
  GScanner	       *scanner;
  SfiRing	       *signals;
};
SfiGlueCodec*	sfi_glue_codec_new	(SfiGlueContext		*context,
					 SfiGlueCodecSendEvent	 send_event,
					 SfiGlueCodecClientMsg   client_msg);
void		sfi_glue_codec_destroy	(SfiGlueCodec		*codec);
gchar*		sfi_glue_codec_process	(SfiGlueCodec		*codec,
					 const gchar		*message);

void		sfi_glue_codec_set_user_data (SfiGlueCodec	*codec,
					      gpointer		 user_data,
					      GDestroyNotify	 destroy);


/* --- client side API --- */
/* after pushing the codec context as current glue layer context,
 * glue layer requests are encoded, the encoded request is passed
 * in to SfiGlueCodecHandleIO(), and the response is then decdoded
 * and returned by the glue layer.
 * server side event messages are processed within the codec
 * context through sfi_glue_codec_enqueue_event().
 */
typedef gchar* (*SfiGlueCodecHandleIO)	(gpointer		 user_data,
					 const gchar		*message);
SfiGlueContext*	sfi_glue_codec_context	(SfiGlueCodecHandleIO	 handle_io,
					 gpointer		 user_data,
					 GDestroyNotify		 destroy);
/* void	 sfi_glue_context_destroy	(SfiGlueContext		*context); */
gboolean sfi_glue_codec_enqueue_event	(SfiGlueContext		*context,
					 const gchar		*message);


/* --- implementation details --- */
typedef enum /*< skip >*/
{
  SFI_GLUE_CODEC_DESCRIBE_ENUM         =  1,
  SFI_GLUE_CODEC_DESCRIBE_IFACE        =  2,
  SFI_GLUE_CODEC_DESCRIBE_PROP         =  3,
  SFI_GLUE_CODEC_DESCRIBE_PROC         =  4,
  SFI_GLUE_CODEC_LIST_PROC_NAMES       =  5,
  SFI_GLUE_CODEC_LIST_METHOD_NAMES     =  6,
  SFI_GLUE_CODEC_BASE_IFACE            =  7,
  SFI_GLUE_CODEC_IFACE_CHILDREN        =  8,
  SFI_GLUE_CODEC_PROXY_IFACE           =  9,
  SFI_GLUE_CODEC_EXEC                  = 10,
  SFI_GLUE_CODEC_SIGNAL_CONNECTION     = 11,
  SFI_GLUE_CODEC_CLIENT_MSG            = 12,
  SFI_GLUE_CODEC_EVENT_SIGNAL          = 13
} SfiGlueCodecCommands;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SFI_GLUE_CODEC_H__ */

/* vim:set ts=8 sts=2 sw=2: */
