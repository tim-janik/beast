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
#include <sfi/sficomport.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- encoder API --- */
typedef struct
{
  SfiGlueContext  context;
  SfiComPort	 *port;
  /*< private >*/
  GValue	  svalue;
  SfiRing        *events;
} SfiGlueEncoder;
/* encode glue layer API calls and pass them on to remote server */
SfiGlueContext*	sfi_glue_encoder_context	(SfiComPort	*port);


#if 0
GValue*		sfi_glue_encode_message		(guint		 log_level,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2,3);
#endif


/* --- decoder API --- */
typedef struct _SfiGlueDecoder SfiGlueDecoder;
typedef GValue*	(*SfiGlueDecoderClientMsg)	(SfiGlueDecoder	*decoder,
						 gpointer	 user_data,
						 const gchar	*message,
						 const GValue	*value);
struct _SfiGlueDecoder
{
  /*< private >*/
  SfiGlueContext *context;
  SfiComPort	 *port;
  GValue	 *incoming;
  SfiRing	 *outgoing;
  guint           n_chandler;
  struct {
    SfiGlueDecoderClientMsg client_msg;
    gpointer                user_data;
  }		 *chandler;
};
/* receive encoded requests and dispatch them onto a given context */
SfiGlueDecoder*	sfi_glue_context_decoder	(SfiComPort	*port,
						 SfiGlueContext	*context);
void		sfi_glue_decoder_add_handler	(SfiGlueDecoder	*decoder,
						 SfiGlueDecoderClientMsg func,
						 gpointer	 user_data);
SfiRing*	sfi_glue_decoder_list_poll_fds	(SfiGlueDecoder	*decoder);
gboolean	sfi_glue_decoder_pending	(SfiGlueDecoder	*decoder);
void		sfi_glue_decoder_dispatch	(SfiGlueDecoder	*decoder);
void		sfi_glue_decoder_destroy	(SfiGlueDecoder	*decoder);


/* --- implementation details --- */
typedef enum /*< skip >*/
{
  SFI_GLUE_CODEC_ASYNC_RETURN			=  1,
  SFI_GLUE_CODEC_ASYNC_MESSAGE,
  SFI_GLUE_CODEC_ASYNC_EVENT,
  SFI_GLUE_CODEC_DESCRIBE_IFACE			= 129,
  SFI_GLUE_CODEC_DESCRIBE_PROC,
  SFI_GLUE_CODEC_LIST_PROC_NAMES,
  SFI_GLUE_CODEC_LIST_METHOD_NAMES,
  SFI_GLUE_CODEC_BASE_IFACE,
  SFI_GLUE_CODEC_IFACE_CHILDREN,
  SFI_GLUE_CODEC_EXEC_PROC,
  SFI_GLUE_CODEC_PROXY_IFACE,
  SFI_GLUE_CODEC_PROXY_IS_A,
  SFI_GLUE_CODEC_PROXY_LIST_PROPERTIES,
  SFI_GLUE_CODEC_PROXY_GET_PSPEC,
  SFI_GLUE_CODEC_PROXY_GET_PSPEC_SCATEGORY,
  SFI_GLUE_CODEC_PROXY_SET_PROPERTY,		/* one-way */
  SFI_GLUE_CODEC_PROXY_GET_PROPERTY,
  SFI_GLUE_CODEC_PROXY_WATCH_RELEASE,
  SFI_GLUE_CODEC_PROXY_REQUEST_NOTIFY,
  SFI_GLUE_CODEC_PROXY_PROCESSED_NOTIFY,	/* one-way */
  SFI_GLUE_CODEC_CLIENT_MSG
} SfiGlueCodecCommands;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SFI_GLUE_CODEC_H__ */

/* vim:set ts=8 sts=2 sw=2: */
