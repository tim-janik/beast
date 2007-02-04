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
#undef  G_LOG_DOMAIN
#define	G_LOG_DOMAIN	"SFI-GLUE"
#include "sfigluecodec.h"
#include "sfiglueproxy.h"


/* --- prototypes --- */
static SfiGlueIFace*  encoder_describe_iface		(SfiGlueContext *context,
							 const gchar    *iface);
static SfiGlueProc*   encoder_describe_proc		(SfiGlueContext *context,
							 const gchar    *proc_name);
static gchar**	      encoder_list_proc_names		(SfiGlueContext *context);
static gchar**	      encoder_list_method_names		(SfiGlueContext *context,
							 const gchar    *iface_name);
static gchar*	      encoder_base_iface		(SfiGlueContext *context);
static gchar**	      encoder_iface_children		(SfiGlueContext *context,
							 const gchar    *iface_name);
static GValue*	      encoder_exec_proc			(SfiGlueContext *context,
							 const gchar    *proc_name,
							 SfiSeq         *params);
static gchar*	      encoder_proxy_iface		(SfiGlueContext *context,
							 SfiProxy        proxy);
static gboolean	      encoder_proxy_is_a		(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *iface);
static gchar**	      encoder_proxy_list_properties	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *first_ancestor,
							 const gchar    *last_ancestor);
static GParamSpec*    encoder_proxy_get_pspec		(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *prop_name);
static SfiSCategory   encoder_proxy_get_pspec_scategory	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *prop_name);
static void	      encoder_proxy_set_property	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *prop,
							 const GValue   *value);
static GValue*	      encoder_proxy_get_property	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *prop);
static gboolean	      encoder_proxy_watch_release	(SfiGlueContext *context,
							 SfiProxy        proxy);
static gboolean	      encoder_proxy_request_notify	(SfiGlueContext *context,
							 SfiProxy        proxy,
							 const gchar    *signal,
							 gboolean        enable_notify);
static void	      encoder_proxy_processed_notify	(SfiGlueContext *context,
							 guint           notify_id);
static GValue*	      encoder_client_msg		(SfiGlueContext *context,
							 const gchar    *msg,
							 GValue         *value);
static SfiRing*	      encoder_fetch_events		(SfiGlueContext *context);
static SfiRing*	      encoder_list_poll_fds		(SfiGlueContext *context);
static void	      encoder_destroy                    (SfiGlueContext *context);


/* --- functions --- */
SfiGlueContext*
sfi_glue_encoder_context (SfiComPort *port)
{
  static const SfiGlueContextTable encoder_vtable = {
    encoder_describe_iface,
    encoder_describe_proc,
    encoder_list_proc_names,
    encoder_list_method_names,
    encoder_base_iface,
    encoder_iface_children,
    encoder_exec_proc,
    encoder_proxy_iface,
    encoder_proxy_is_a,
    encoder_proxy_list_properties,
    encoder_proxy_get_pspec,
    encoder_proxy_get_pspec_scategory,
    encoder_proxy_set_property,
    encoder_proxy_get_property,
    encoder_proxy_watch_release,
    encoder_proxy_request_notify,
    encoder_proxy_processed_notify,
    encoder_client_msg,
    encoder_fetch_events,
    encoder_list_poll_fds,
    encoder_destroy,
  };
  SfiGlueEncoder *encoder;
  
  g_return_val_if_fail (port != NULL, NULL);
  
  encoder = g_new0 (SfiGlueEncoder, 1);
  sfi_glue_context_common_init (&encoder->context, &encoder_vtable);
  encoder->port = sfi_com_port_ref (port);
  g_value_init (&encoder->svalue, SFI_TYPE_SEQ);
  encoder->events = NULL;
  
  return &encoder->context;
}

#if 0
GValue* /* needs sfi_value_free() */
sfi_glue_encode_message (guint        log_level,
			 const gchar *format,
			 ...)
{
  gchar *buffer;
  GValue *value;
  SfiSeq *seq;
  va_list args;
  
  g_return_val_if_fail (format != NULL, NULL);
  
  va_start (args, format);
  buffer = g_strdup_vprintf (format, args);
  va_end (args);
  
  switch (log_level)
    {
    case SFI_LOG_ERROR:
    case SFI_LOG_WARN:
    case SFI_LOG_INFO:
    case SFI_LOG_DIAG:
      break;
    default:
      log_level = SFI_LOG_DEBUG;
    }
  
  seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_ASYNC_MESSAGE);
  sfi_seq_append_string (seq, G_LOG_DOMAIN);
  sfi_seq_append_int (seq, log_level);
  sfi_seq_append_string (seq, buffer);		/* message */
  value = sfi_value_seq (seq);
  sfi_seq_unref (seq);
  g_free (buffer);
  return value;
}
#endif

static gboolean
encoder_process_message (SfiGlueEncoder *encoder,
			 GValue		*value,
			 GValue	       **rvalue)
{
  if (SFI_VALUE_HOLDS_SEQ (value))
    {
      SfiSeq *seq = sfi_value_get_seq (value);
      SfiGlueCodecCommands cmd = seq && seq->n_elements >= 1 ? sfi_seq_get_int (seq, 0) : 0;
      switch (cmd)
	{
	case SFI_GLUE_CODEC_ASYNC_RETURN:
	  if (rvalue)
	    {
	      *rvalue = NULL;
	      if (seq->n_elements >= 2)
		*rvalue = sfi_value_clone_shallow (sfi_seq_get (seq, 1));
	      sfi_value_free (value);
	      return TRUE;
	    }
	  else
	    sfi_diag ("ignoring message with spurious return value");
	  break;
	case SFI_GLUE_CODEC_ASYNC_MESSAGE:
#if 0
	  if (seq->n_elements >= 4)
	    sfi_log_printf (sfi_seq_get_string (seq, 1),
                            sfi_seq_get_int (seq, 2),
                            SfiLogger (NULL, NULL, NULL),
                            sfi_seq_get_string (seq, 3));
	  else
#endif
	    sfi_diag ("ignoring message with invalid message contents");
	  break;
	case SFI_GLUE_CODEC_ASYNC_EVENT:
	  seq = seq->n_elements >= 2 ? sfi_seq_get_seq (seq, 1) : NULL;
	  if (seq)
	    encoder->events = sfi_ring_append (encoder->events, sfi_seq_ref (seq));
	  else
	    sfi_diag ("ignoring message with NULL event");
	  break;
	default:
	  sfi_diag ("ignoring message with invalid ID: %u", cmd);
	  break;
	}
    }
  else
    sfi_diag ("ignoring message of invalid type: %s", G_VALUE_TYPE_NAME (value));
  sfi_value_free (value);
  
  return FALSE;
}

static void
encoder_exec_one_way (SfiGlueContext *context,
		      SfiSeq         *seq)
{
  SfiGlueEncoder *encoder = (SfiGlueEncoder*) context;
  GValue *value;
  
  /* send request off to remote */
  sfi_value_set_seq (&encoder->svalue, seq);
  sfi_com_port_send (encoder->port, &encoder->svalue);
  sfi_value_set_seq (&encoder->svalue, NULL);
  sfi_seq_unref (seq);

  /* handle incoming messages */
  do
    {
      value = sfi_com_port_recv (encoder->port);
      if (value)
	encoder_process_message (encoder, value, NULL);
    }
  while (value);
}

static SfiSeq*
encoder_exec_round_trip (SfiGlueContext *context,
			 SfiSeq         *seq)
{
  SfiGlueEncoder *encoder = (SfiGlueEncoder*) context;
  GValue *rvalue = NULL;
  
  /* send request off to remote */
  sfi_value_set_seq (&encoder->svalue, seq);
  sfi_com_port_send (encoder->port, &encoder->svalue);
  sfi_value_set_seq (&encoder->svalue, NULL);
  
  /* spin until we receive result */
  while (!rvalue)
    {
      GValue *value = sfi_com_port_recv_blocking (encoder->port);
      if (value && encoder_process_message (encoder, value, &rvalue))
	break;
    }
  
  sfi_seq_clear (seq);
  if (rvalue)
    {
      sfi_seq_append (seq, rvalue);
      sfi_value_free (rvalue);
    }
  return seq;
}

static SfiGlueIFace*
encoder_describe_iface (SfiGlueContext *context,
			const gchar    *iface_name)
{
  SfiGlueIFace *iface = NULL;
  SfiRec *rec;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_DESCRIBE_IFACE);
  sfi_seq_append_string (seq, iface_name);
  
  seq = encoder_exec_round_trip (context, seq);
  
  rec = sfi_seq_get_rec (seq, 0);
  if (rec)
    {
      iface = sfi_glue_iface_new (sfi_rec_get_string (rec, "type_name"));
      iface->ifaces = sfi_seq_to_strv (sfi_rec_get_seq (rec, "ifaces"));
      iface->n_ifaces = g_strlenv (iface->ifaces);
      iface->props = sfi_seq_to_strv (sfi_rec_get_seq (rec, "props"));
      iface->n_props = g_strlenv (iface->props);
    }
  sfi_seq_unref (seq);
  return iface;
}

static GValue*
decoder_describe_iface (SfiGlueDecoder *decoder,
			SfiSeq         *seq)
{
  SfiGlueIFace *iface = sfi_glue_describe_iface (sfi_seq_get_string (seq, 1));
  GValue *rvalue = NULL;
  SfiRec *rec = NULL;
  if (iface)
    {
      SfiSeq *seq;
      rec = sfi_rec_new ();
      sfi_rec_set_string (rec, "type_name", iface->type_name);
      seq = sfi_seq_from_strv (iface->ifaces);
      sfi_rec_set_seq (rec, "ifaces", seq);
      sfi_seq_unref (seq);
      seq = sfi_seq_from_strv (iface->props);
      sfi_rec_set_seq (rec, "props", seq);
      sfi_seq_unref (seq);
    }
  rvalue = sfi_value_rec (rec);
  sfi_glue_gc_free_now (iface, sfi_glue_iface_unref);
  return rvalue;
}

static SfiGlueProc*
encoder_describe_proc (SfiGlueContext *context,
		       const gchar    *proc_name)
{
  SfiGlueProc *proc = NULL;
  SfiRec *rec;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_DESCRIBE_PROC);
  sfi_seq_append_string (seq, proc_name);
  
  seq = encoder_exec_round_trip (context, seq);
  
  rec = sfi_seq_get_rec (seq, 0);
  if (rec)
    {
      SfiSeq *pseq;
      GParamSpec *pspec;
      proc = sfi_glue_proc_new (sfi_rec_get_string (rec, "name"));
      proc->help = g_strdup (sfi_rec_get_string (rec, "help"));
      proc->authors = g_strdup (sfi_rec_get_string (rec, "authors"));
      proc->license = g_strdup (sfi_rec_get_string (rec, "license"));
      pseq = sfi_rec_get_seq (rec, "params");
      if (pseq)
	{
	  guint i;
	  for (i = 0; i < pseq->n_elements; i++)
	    sfi_glue_proc_add_param (proc, sfi_seq_get_pspec (pseq, i));
	}
      pspec = sfi_rec_get_pspec (rec, "ret_param");
      if (pspec)
	sfi_glue_proc_add_ret_param (proc, pspec);
    }
  sfi_seq_unref (seq);
  return proc;
}

static GValue*
decoder_describe_proc (SfiGlueDecoder *decoder,
		       SfiSeq         *seq)
{
  SfiGlueProc *proc = sfi_glue_describe_proc (sfi_seq_get_string (seq, 1));
  GValue *rvalue = NULL;
  SfiRec *rec = NULL;
  if (proc)
    {
      rec = sfi_rec_new ();
      sfi_rec_set_string (rec, "name", proc->name);
      sfi_rec_set_string (rec, "help", proc->help);
      sfi_rec_set_string (rec, "authors", proc->authors);
      sfi_rec_set_string (rec, "license", proc->license);
      if (proc->ret_param)
	sfi_rec_set_pspec (rec, "ret_param", proc->ret_param);
      if (proc->params)
	{
	  SfiSeq *seq = sfi_seq_new ();
	  guint i;
	  for (i = 0; i < proc->n_params; i++)
	    sfi_seq_append_pspec (seq, proc->params[i]);
	  sfi_rec_set_seq (rec, "params", seq);
	  sfi_seq_unref (seq);
	}
    }
  rvalue = sfi_value_rec (rec);
  sfi_glue_gc_free_now (proc, sfi_glue_proc_unref);
  return rvalue;
}

static inline GValue*
seq_value_from_strv (const gchar **strv)
{
  SfiSeq *seq = sfi_seq_from_cstrv (strv);
  GValue *rvalue = sfi_value_seq (seq);
  sfi_seq_unref (seq);
  return rvalue;
}

static gchar**
encoder_list_proc_names (SfiGlueContext *context)
{
  gchar **strv = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_LIST_PROC_NAMES);
  
  seq = encoder_exec_round_trip (context, seq);
  
  strv = sfi_seq_to_strv (sfi_seq_get_seq (seq, 0));
  sfi_seq_unref (seq);
  return strv;
}

static GValue*
decoder_list_proc_names (SfiGlueDecoder *decoder,
			 SfiSeq         *seq)
{
  const gchar **names = sfi_glue_list_proc_names ();
  GValue *rvalue = seq_value_from_strv (names);
  sfi_glue_gc_free_now (names, g_strfreev);
  return rvalue;
}

static gchar**
encoder_list_method_names (SfiGlueContext *context,
			   const gchar    *iface_name)
{
  gchar **strv = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_LIST_METHOD_NAMES);
  sfi_seq_append_string (seq, iface_name);
  
  seq = encoder_exec_round_trip (context, seq);
  
  strv = sfi_seq_to_strv (sfi_seq_get_seq (seq, 0));
  sfi_seq_unref (seq);
  return strv;
}

static GValue*
decoder_list_method_names (SfiGlueDecoder *decoder,
			   SfiSeq         *seq)
{
  const gchar **names = sfi_glue_list_method_names (sfi_seq_get_string (seq, 1));
  GValue *rvalue = seq_value_from_strv (names);
  sfi_glue_gc_free_now (names, g_strfreev);
  return rvalue;
}

static gchar*
encoder_base_iface (SfiGlueContext *context)
{
  gchar *string = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_BASE_IFACE);
  
  seq = encoder_exec_round_trip (context, seq);
  
  string = g_strdup (sfi_seq_get_string (seq, 0));
  sfi_seq_unref (seq);
  return string;
}

static GValue*
decoder_base_iface (SfiGlueDecoder *decoder,
		    SfiSeq         *seq)
{
  gchar *name = sfi_glue_base_iface ();
  GValue *rvalue = NULL;
  rvalue = sfi_value_string (name);
  sfi_glue_gc_free_now (name, g_free);
  return rvalue;
}

static gchar**
encoder_iface_children (SfiGlueContext *context,
			const gchar    *iface_name)
{
  gchar **strv = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_IFACE_CHILDREN);
  sfi_seq_append_string (seq, iface_name);
  
  seq = encoder_exec_round_trip (context, seq);
  
  strv = sfi_seq_to_strv (sfi_seq_get_seq (seq, 0));
  sfi_seq_unref (seq);
  return strv;
}

static GValue*
decoder_iface_children (SfiGlueDecoder *decoder,
			SfiSeq         *seq)
{
  const gchar **names = sfi_glue_iface_children (sfi_seq_get_string (seq, 1));
  GValue *rvalue = seq_value_from_strv (names);
  sfi_glue_gc_free_now (names, g_strfreev);
  return rvalue;
}

static GValue*
encoder_exec_proc (SfiGlueContext *context,
		   const gchar    *proc_name,
		   SfiSeq         *params)
{
  GValue *rvalue = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_EXEC_PROC);
  sfi_seq_append_string (seq, proc_name);
  sfi_seq_append_seq (seq, params);
  
  seq = encoder_exec_round_trip (context, seq);
  
  if (seq->n_elements)
    rvalue = sfi_value_clone_shallow (sfi_seq_get (seq, 0));
  sfi_seq_unref (seq);
  return rvalue;
}

static GValue*
decoder_exec_proc (SfiGlueDecoder *decoder,
		   SfiSeq         *seq)
{
  GValue *pvalue = sfi_glue_call_seq (sfi_seq_get_string (seq, 1),
				      sfi_seq_get_seq (seq, 2));
  if (pvalue)
    sfi_glue_gc_remove (pvalue, sfi_value_free);
  return pvalue;
}

static gchar*
encoder_proxy_iface (SfiGlueContext *context,
		     SfiProxy        proxy)
{
  gchar *string = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_IFACE);
  sfi_seq_append_proxy (seq, proxy);
  
  seq = encoder_exec_round_trip (context, seq);
  
  string = g_strdup (sfi_seq_get_string (seq, 0));
  sfi_seq_unref (seq);
  return string;
}

static GValue*
decoder_proxy_iface (SfiGlueDecoder *decoder,
		     SfiSeq         *seq)
{
  const gchar *name = sfi_glue_proxy_iface (sfi_seq_get_proxy (seq, 1));
  GValue *rvalue = NULL;
  rvalue = sfi_value_string (name);
  sfi_glue_gc_free_now ((char*) name, g_free);
  return rvalue;
}

static gboolean
encoder_proxy_is_a (SfiGlueContext *context,
		    SfiProxy        proxy,
		    const gchar    *iface)
{
  gboolean vbool;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_IS_A);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, iface);
  
  seq = encoder_exec_round_trip (context, seq);
  
  vbool = sfi_seq_get_bool (seq, 0);
  sfi_seq_unref (seq);
  return vbool;
}

static GValue*
decoder_proxy_is_a (SfiGlueDecoder *decoder,
		    SfiSeq         *seq)
{
  gboolean vbool = sfi_glue_proxy_is_a (sfi_seq_get_proxy (seq, 1),
					sfi_seq_get_string (seq, 2));
  return sfi_value_bool (vbool);
}

static gchar**
encoder_proxy_list_properties (SfiGlueContext *context,
			       SfiProxy        proxy,
			       const gchar    *first_ancestor,
			       const gchar    *last_ancestor)
{
  gchar **strv = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_LIST_PROPERTIES);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, first_ancestor);
  sfi_seq_append_string (seq, last_ancestor);
  
  seq = encoder_exec_round_trip (context, seq);
  
  strv = sfi_seq_to_strv (sfi_seq_get_seq (seq, 0));
  sfi_seq_unref (seq);
  return strv;
}

static GValue*
decoder_proxy_list_properties (SfiGlueDecoder *decoder,
			       SfiSeq         *seq)
{
  const gchar **names = sfi_glue_proxy_list_properties (sfi_seq_get_proxy (seq, 1),
							sfi_seq_get_string (seq, 2),
							sfi_seq_get_string (seq, 3),
							NULL);
  GValue *rvalue = seq_value_from_strv (names);
  sfi_glue_gc_free_now (names, g_strfreev);
  return rvalue;
}

static GParamSpec*
encoder_proxy_get_pspec (SfiGlueContext *context,
			 SfiProxy        proxy,
			 const gchar    *prop_name)
{
  GParamSpec *pspec = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_GET_PSPEC);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, prop_name);
  
  seq = encoder_exec_round_trip (context, seq);
  
  pspec = sfi_seq_get_pspec (seq, 0);
  if (pspec)
    g_param_spec_ref (pspec);
  sfi_seq_unref (seq);
  return pspec;
}

static GValue*
decoder_proxy_get_pspec (SfiGlueDecoder *decoder,
			 SfiSeq         *seq)
{
  GParamSpec *pspec = sfi_glue_proxy_get_pspec (sfi_seq_get_proxy (seq, 1),
						sfi_seq_get_string (seq, 2));
  GValue *rvalue = NULL;
  rvalue = sfi_value_pspec (pspec);
  if (pspec)
    sfi_glue_gc_free_now (pspec, g_param_spec_unref);
  return rvalue;
}

static SfiSCategory
encoder_proxy_get_pspec_scategory (SfiGlueContext *context,
				   SfiProxy        proxy,
				   const gchar    *prop_name)
{
  SfiSCategory scat;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_GET_PSPEC_SCATEGORY);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, prop_name);
  
  seq = encoder_exec_round_trip (context, seq);
  
  scat = sfi_seq_get_int (seq, 0);
  sfi_seq_unref (seq);
  return scat;
}

static GValue*
decoder_proxy_get_pspec_scategory (SfiGlueDecoder *decoder,
				   SfiSeq         *seq)
{
  SfiSCategory scat = sfi_glue_proxy_get_pspec_scategory (sfi_seq_get_proxy (seq, 1),
							  sfi_seq_get_string (seq, 2));
  return sfi_value_int (scat);
}

static void
encoder_proxy_set_property (SfiGlueContext *context,
			    SfiProxy        proxy,
			    const gchar    *prop,
			    const GValue   *value)
{
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_SET_PROPERTY);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, prop);
  sfi_seq_append (seq, value);
  
  encoder_exec_one_way (context, seq);
}

static void
decoder_proxy_set_property (SfiGlueDecoder *decoder,
			    SfiSeq         *seq)
{
  if (seq->n_elements >= 4)
    sfi_glue_proxy_set_property (sfi_seq_get_proxy (seq, 1),
				 sfi_seq_get_string (seq, 2),
				 sfi_seq_get (seq, 3));
}

static GValue*
encoder_proxy_get_property (SfiGlueContext *context,
			    SfiProxy        proxy,
			    const gchar    *prop)
{
  GValue *rvalue = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_GET_PROPERTY);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, prop);
  
  seq = encoder_exec_round_trip (context, seq);
  
  if (seq->n_elements)
    rvalue = sfi_value_clone_shallow (sfi_seq_get (seq, 0));
  sfi_seq_unref (seq);
  return rvalue;
}

static GValue*
decoder_proxy_get_property (SfiGlueDecoder *decoder,
			    SfiSeq         *seq)
{
  GValue *pvalue = (GValue*) sfi_glue_proxy_get_property (sfi_seq_get_proxy (seq, 1),
							  sfi_seq_get_string (seq, 2));
  if (pvalue)
    sfi_glue_gc_remove (pvalue, sfi_value_free);
  return pvalue;
}

static gboolean
encoder_proxy_watch_release (SfiGlueContext *context,
			     SfiProxy        proxy)
{
  gboolean vbool;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_WATCH_RELEASE);
  sfi_seq_append_proxy (seq, proxy);
  
  seq = encoder_exec_round_trip (context, seq);
  
  vbool = sfi_seq_get_bool (seq, 0);
  sfi_seq_unref (seq);
  return vbool;
}

static GValue*
decoder_proxy_watch_release (SfiGlueDecoder *decoder,
			     SfiSeq         *seq)
{
  gboolean vbool;
  vbool = _sfi_glue_proxy_watch_release (sfi_seq_get_proxy (seq, 1));
  return sfi_value_bool (vbool);
}

static gboolean
encoder_proxy_request_notify (SfiGlueContext *context,
			      SfiProxy        proxy,
			      const gchar    *signal,
			      gboolean        enable_notify)
{
  gboolean vbool;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_REQUEST_NOTIFY);
  sfi_seq_append_proxy (seq, proxy);
  sfi_seq_append_string (seq, signal);
  sfi_seq_append_bool (seq, enable_notify);
  
  seq = encoder_exec_round_trip (context, seq);
  
  vbool = sfi_seq_get_bool (seq, 0);
  sfi_seq_unref (seq);
  return vbool;
}

static GValue*
decoder_proxy_request_notify (SfiGlueDecoder *decoder,
			      SfiSeq         *seq)
{
  gboolean vbool = _sfi_glue_proxy_request_notify (sfi_seq_get_proxy (seq, 1),
						   sfi_seq_get_string (seq, 2),
						   sfi_seq_get_bool (seq, 3));
  return sfi_value_bool (vbool);
}

static void
encoder_proxy_processed_notify (SfiGlueContext *context,
				guint           notify_id)
{
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_PROXY_PROCESSED_NOTIFY);
  sfi_seq_append_int (seq, notify_id);
  encoder_exec_one_way (context, seq);
}

static void
decoder_proxy_processed_notify (SfiGlueDecoder *decoder,
				SfiSeq         *seq)
{
  if (seq->n_elements >= 2)
    _sfi_glue_proxy_processed_notify (sfi_seq_get_int (seq, 1));
  else
    sfi_diag ("ignoring invalid \"processed notify\" receipt");
}

static GValue*
encoder_client_msg (SfiGlueContext *context,
		    const gchar    *msg,
		    GValue         *value)
{
  GValue *rvalue = NULL;
  SfiSeq *seq = sfi_seq_new ();
  sfi_seq_append_int (seq, SFI_GLUE_CODEC_CLIENT_MSG);
  sfi_seq_append_string (seq, msg);
  if (value)
    sfi_seq_append (seq, value);
  
  seq = encoder_exec_round_trip (context, seq);
  
  if (seq->n_elements)
    rvalue = sfi_value_clone_shallow (sfi_seq_get (seq, 0));
  sfi_seq_unref (seq);
  return rvalue;
}

static GValue*
decoder_client_msg (SfiGlueDecoder *decoder,
		    SfiSeq         *seq)
{
  const gchar *msg = sfi_seq_get_string (seq, 1);
  GValue dummy = { 0, }, *rvalue = NULL;
  GValue *cvalue = seq->n_elements > 2 ? sfi_seq_get (seq, 2) : &dummy;
  guint i;
  
  /* interception handler */
  for (i = 0; i < decoder->n_chandler && !rvalue; i++)
    rvalue = decoder->chandler[i].client_msg (decoder,
					      decoder->chandler[i].user_data,
					      msg, cvalue);
  /* regular handling */
  if (!rvalue)
    {
      rvalue = sfi_glue_client_msg (msg, cvalue);
      if (rvalue)
	sfi_glue_gc_remove (rvalue, sfi_value_free);
    }
  
  return rvalue;
}

static SfiRing*
encoder_fetch_events (SfiGlueContext *context)
{
  SfiGlueEncoder *encoder = (SfiGlueEncoder*) context;
  SfiRing *events;
  GValue *value = sfi_com_port_recv (encoder->port);
  while (value)		/* spin until queue is empty */
    {
      encoder_process_message (encoder, value, NULL);
      value = sfi_com_port_recv (encoder->port);
    }
  events = encoder->events;
  encoder->events = NULL;
  return events;
}

static SfiRing*
encoder_list_poll_fds (SfiGlueContext *context)
{
  SfiGlueEncoder *encoder = (SfiGlueEncoder*) context;
  guint n;
  GPollFD *pfd = sfi_com_port_get_poll_fds (encoder->port, &n);
  SfiRing *ring = NULL;
  while (n--)
    ring = sfi_ring_prepend (ring, &pfd[n]);
  return ring;
}

static void
encoder_destroy (SfiGlueContext *context)
{
  SfiGlueEncoder *encoder = (SfiGlueEncoder*) context;
  SfiSeq *seq;
  sfi_com_port_unref (encoder->port);
  g_value_unset (&encoder->svalue);
  seq = sfi_ring_pop_head (&encoder->events);
  while (seq)
    {
      sfi_seq_unref (seq);
      seq = sfi_ring_pop_head (&encoder->events);
    }
  g_free (encoder);
}

static GValue*
decoder_process_request (SfiGlueDecoder *decoder,
			 const GValue   *value,
			 gboolean       *one_way)
{
  SfiSeq *seq = SFI_VALUE_HOLDS_SEQ (value) ? sfi_value_get_seq (value) : NULL;
  SfiInt cmd;

  if (!seq || seq->n_elements < 1)
    {
      *one_way = FALSE;
      sfi_diag ("discarding invalid empty request");
      return NULL;
    }
  *one_way = FALSE;

  /* here, we are processing incoming requests from remote.
   * after decoding the request, we invoke actual glue layer
   * functions and encode the return values to pass them back
   * to the remote client.
   */
  cmd = sfi_seq_get_int (seq, 0);
  
  switch (cmd)
    {
    case SFI_GLUE_CODEC_DESCRIBE_IFACE:
      return decoder_describe_iface (decoder, seq);
    case SFI_GLUE_CODEC_DESCRIBE_PROC:
      return decoder_describe_proc (decoder, seq);
    case SFI_GLUE_CODEC_LIST_PROC_NAMES:
      return decoder_list_proc_names (decoder, seq);
    case SFI_GLUE_CODEC_LIST_METHOD_NAMES:
      return decoder_list_method_names (decoder, seq);
    case SFI_GLUE_CODEC_BASE_IFACE:
      return decoder_base_iface (decoder, seq);
    case SFI_GLUE_CODEC_IFACE_CHILDREN:
      return decoder_iface_children (decoder, seq);
    case SFI_GLUE_CODEC_EXEC_PROC:
      return decoder_exec_proc (decoder, seq);
    case SFI_GLUE_CODEC_PROXY_IFACE:
      return decoder_proxy_iface (decoder, seq);
    case SFI_GLUE_CODEC_PROXY_IS_A:
      return decoder_proxy_is_a (decoder, seq);
    case SFI_GLUE_CODEC_PROXY_LIST_PROPERTIES:
      return decoder_proxy_list_properties (decoder, seq);
    case SFI_GLUE_CODEC_PROXY_GET_PSPEC:
      return decoder_proxy_get_pspec (decoder, seq);
    case SFI_GLUE_CODEC_PROXY_GET_PSPEC_SCATEGORY:
      return decoder_proxy_get_pspec_scategory (decoder, seq);
    case SFI_GLUE_CODEC_PROXY_SET_PROPERTY:
      *one_way = TRUE;
      decoder_proxy_set_property (decoder, seq);
      return NULL;
    case SFI_GLUE_CODEC_PROXY_GET_PROPERTY:
      return decoder_proxy_get_property (decoder, seq);
    case SFI_GLUE_CODEC_PROXY_WATCH_RELEASE:
      return decoder_proxy_watch_release (decoder, seq);
    case SFI_GLUE_CODEC_PROXY_REQUEST_NOTIFY:
      return decoder_proxy_request_notify (decoder, seq);
    case SFI_GLUE_CODEC_PROXY_PROCESSED_NOTIFY:
      *one_way = TRUE;
      decoder_proxy_processed_notify (decoder, seq);
      return NULL;
    case SFI_GLUE_CODEC_CLIENT_MSG:
      return decoder_client_msg (decoder, seq);
    }
  sfi_diag ("ignoring request with invalid ID: %d", cmd);
  return NULL;
}

SfiGlueDecoder*
sfi_glue_context_decoder (SfiComPort     *port,
			  SfiGlueContext *context)
{
  SfiGlueDecoder *decoder;
  
  g_return_val_if_fail (port != NULL, NULL);
  g_return_val_if_fail (context != NULL, NULL);
  
  decoder = g_new0 (SfiGlueDecoder, 1);
  decoder->context = context;
  decoder->port = sfi_com_port_ref (port);
  decoder->incoming = NULL;
  decoder->outgoing = NULL;
  
  return decoder;
}

void
sfi_glue_decoder_add_handler (SfiGlueDecoder         *decoder,
			      SfiGlueDecoderClientMsg func,
			      gpointer                user_data)
{
  guint i;
  
  g_return_if_fail (decoder != NULL);
  g_return_if_fail (func != NULL);
  
  i = decoder->n_chandler++;
  decoder->chandler = g_realloc (decoder->chandler, sizeof (decoder->chandler[0]) * decoder->n_chandler);
  decoder->chandler[i].client_msg = func;
  decoder->chandler[i].user_data = user_data;
}

gboolean
sfi_glue_decoder_pending (SfiGlueDecoder *decoder)
{
  gboolean pending;
  
  g_return_val_if_fail (decoder != NULL, FALSE);
  
  pending = decoder->outgoing || decoder->incoming;
  if (!pending)
    {
      decoder->incoming = sfi_com_port_recv (decoder->port);
      pending |= decoder->incoming != NULL;
    }
  if (!pending)
    pending |= sfi_com_port_io_pending (decoder->port);
  if (!pending)
    {
      sfi_glue_context_push (decoder->context);
      pending |= sfi_glue_context_pending ();
      sfi_glue_context_pop ();
    }
  
  return pending;
}

void
sfi_glue_decoder_dispatch (SfiGlueDecoder *decoder)
{
  SfiSeq *seq;
  
  g_return_if_fail (decoder != NULL);
  
  sfi_glue_context_push (decoder->context);
  
  /* queue emitted signals */
  seq = sfi_glue_context_fetch_event (); /* instead of sfi_glue_context_dispatch() */
  while (seq)
    {
      SfiSeq *tmp = sfi_seq_new ();
      sfi_seq_append_int (tmp, SFI_GLUE_CODEC_ASYNC_EVENT);
      sfi_seq_append_seq (tmp, seq);
      sfi_seq_unref (seq);
      decoder->outgoing = sfi_ring_append (decoder->outgoing, sfi_value_seq (tmp));
      sfi_seq_unref (tmp);
      seq = sfi_glue_context_fetch_event ();
    }
  
  /* send away queued signals */
  sfi_com_port_send_bulk (decoder->port, decoder->outgoing);
  while (decoder->outgoing)
    sfi_value_free (sfi_ring_pop_head (&decoder->outgoing));
  
  /* FIXME: catch messages */
  
  /* process incoming request */
  if (!decoder->incoming)
    decoder->incoming = sfi_com_port_recv (decoder->port);
  if (decoder->incoming)
    {
      GValue *rvalue, *value = decoder->incoming;
      gboolean one_way;
      decoder->incoming = NULL;
      rvalue = decoder_process_request (decoder, value, &one_way);
      sfi_value_free (value);
      if (!one_way)
	{
	  seq = sfi_seq_new ();
	  sfi_seq_append_int (seq, SFI_GLUE_CODEC_ASYNC_RETURN);
	  if (rvalue)
	    {
	      sfi_seq_append (seq, rvalue);
	      sfi_value_free (rvalue);
	    }
	  decoder->outgoing = sfi_ring_append (decoder->outgoing, sfi_value_seq (seq));
	  sfi_seq_unref (seq);
	}
      else
	g_assert (rvalue == NULL);
    }

  /* queue emitted signals */
  seq = sfi_glue_context_fetch_event (); /* instead of sfi_glue_context_dispatch() */
  while (seq)
    {
      SfiSeq *tmp = sfi_seq_new ();
      sfi_seq_append_int (tmp, SFI_GLUE_CODEC_ASYNC_EVENT);
      sfi_seq_append_seq (tmp, seq);
      sfi_seq_unref (seq);
      decoder->outgoing = sfi_ring_append (decoder->outgoing, sfi_value_seq (tmp));
      sfi_seq_unref (tmp);
      seq = sfi_glue_context_fetch_event ();
    }
  
  /* send away new signals and result */
  sfi_com_port_send_bulk (decoder->port, decoder->outgoing);
  while (decoder->outgoing)
    sfi_value_free (sfi_ring_pop_head (&decoder->outgoing));
  sfi_com_port_process_io (decoder->port);
  
  sfi_glue_gc_run ();
  sfi_glue_context_pop ();
}

SfiRing* /* sfi_ring_free() result */
sfi_glue_decoder_list_poll_fds (SfiGlueDecoder *decoder)
{
  GPollFD *pfd;
  SfiRing *ring;
  guint n;
  
  g_return_val_if_fail (decoder != NULL, NULL);
  
  sfi_glue_context_push (decoder->context);
  ring = sfi_ring_copy (sfi_glue_context_list_poll_fds ());
  sfi_glue_context_pop ();
  pfd = sfi_com_port_get_poll_fds (decoder->port, &n);
  while (n--)
    ring = sfi_ring_prepend (ring, &pfd[n]);
  return ring;
}

void
sfi_glue_decoder_destroy (SfiGlueDecoder *decoder)
{
  SfiRing *ring;
  
  g_return_if_fail (decoder != NULL);
  
  sfi_com_port_unref (decoder->port);
  for (ring = decoder->outgoing; ring; ring = sfi_ring_walk (ring, decoder->outgoing))
    sfi_value_free (ring->data);
  sfi_ring_free (decoder->outgoing);
  if (decoder->incoming)
    sfi_value_free (decoder->incoming);
  g_free (decoder->chandler);
  g_free (decoder);
}

/* vim:set ts=8 sts=2 sw=2: */
