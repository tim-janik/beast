/* GSL - Generic Sound Layer
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
#ifndef __GSL_GLUE_CODEC_H__
#define __GSL_GLUE_CODEC_H__

#include <gsl/gslglue.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- server side API --- */
/* incomming messages (requests) are decoded and
 * dispatched through the glue layer, and the glue layer results
 * are encoded and returned via gsl_glue_codec_process().
 * server side events are encoded and need to be handled via
 * GslGlueCodecSendEvent() (these usually result from signal
 * emissions of the glue layer). the events are decoded and
 * processed via the client side API.
 * client messages are, before being passed on to the glue
 * layer, filtered through GslGlueCodecClientMsg() and may
 * be intercepted.
 */
typedef struct _GslGlueCodec                   GslGlueCodec;
typedef void         (*GslGlueCodecSendEvent) (GslGlueCodec *code,
					       gpointer	     user_data,
					       const gchar  *message);
typedef GslGlueValue (*GslGlueCodecClientMsg) (GslGlueCodec *code,
					       gpointer      user_data,
					       const gchar  *message,
					       GslGlueValue  value,
					       gboolean     *handled);
struct _GslGlueCodec
{
  gpointer		user_data;
  GslGlueContext       *context;
  GslGlueCodecSendEvent	send_event;
  GslGlueCodecClientMsg client_msg;
  GDestroyNotify	destroy;
  GScanner	       *scanner;
  GslRing	       *signals;
};
GslGlueCodec*	gsl_glue_codec_new	(GslGlueContext		*context,
					 GslGlueCodecSendEvent	 send_event,
					 GslGlueCodecClientMsg   client_msg);
void		gsl_glue_codec_destroy	(GslGlueCodec		*codec);
gchar*		gsl_glue_codec_process	(GslGlueCodec		*codec,
					 const gchar		*message);

void		gsl_glue_codec_set_user_data (GslGlueCodec	*codec,
					      gpointer		 user_data,
					      GDestroyNotify	 destroy);


/* --- client side API --- */
/* after pushing the codec context as current glue layer context,
 * glue layer requests are encoded, the encoded request is passed
 * in to GslGlueCodecHandleIO(), and the response is then decdoded
 * and returned by the glue layer.
 * server side event messages are processed within the codec
 * context through gsl_glue_codec_enqueue_event().
 */
typedef gchar* (*GslGlueCodecHandleIO)	(gpointer		 user_data,
					 const gchar		*message);
GslGlueContext*	gsl_glue_codec_context	(GslGlueCodecHandleIO	 handle_io,
					 gpointer		 user_data,
					 GDestroyNotify		 destroy);
/* void	 gsl_glue_context_destroy	(GslGlueContext		*context); */
gboolean gsl_glue_codec_enqueue_event	(GslGlueContext		*context,
					 const gchar		*message);


/* --- implementation details --- */
typedef enum /*< skip >*/
{
  GSL_GLUE_CODEC_DESCRIBE_ENUM         =  1,
  GSL_GLUE_CODEC_DESCRIBE_IFACE        =  2,
  GSL_GLUE_CODEC_DESCRIBE_PROP         =  3,
  GSL_GLUE_CODEC_DESCRIBE_PROC         =  4,
  GSL_GLUE_CODEC_LIST_PROC_NAMES       =  5,
  GSL_GLUE_CODEC_LIST_METHOD_NAMES     =  6,
  GSL_GLUE_CODEC_BASE_IFACE            =  7,
  GSL_GLUE_CODEC_IFACE_CHILDREN        =  8,
  GSL_GLUE_CODEC_PROXY_IFACE           =  9,
  GSL_GLUE_CODEC_EXEC                  = 10,
  GSL_GLUE_CODEC_SIGNAL_CONNECTION     = 11,
  GSL_GLUE_CODEC_CLIENT_MSG            = 12,
  GSL_GLUE_CODEC_EVENT_SIGNAL          = 13
} GslGlueCodecCommands;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_GLUE_CODEC_H__ */
