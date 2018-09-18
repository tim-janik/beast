// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_GLUE_CODEC_H__
#define __SFI_GLUE_CODEC_H__

#include <bse/sfiglue.hh>
#include <bse/sficomport.hh>


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
  struct ClientMsg {
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
  SFI_GLUE_CODEC_UNUSED_2,
  SFI_GLUE_CODEC_LIST_PROC_NAMES,
  SFI_GLUE_CODEC_LIST_METHOD_NAMES,
  SFI_GLUE_CODEC_BASE_IFACE,
  SFI_GLUE_CODEC_IFACE_CHILDREN,
  SFI_GLUE_CODEC_UNUSED_1,
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



#endif /* __SFI_GLUE_CODEC_H__ */

/* vim:set ts=8 sts=2 sw=2: */
