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
#ifndef __SFI_GLUE_H__
#define __SFI_GLUE_H__

#include <sfi/sfiprimitives.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct _SfiGlueContext SfiGlueContext;


/* Glue proxy (object handle) description in terms of
 * supported interfaces and property names
 */
typedef struct {
  guint   ref_count;
  gchar  *type_name;	/* interface type name */
  guint   n_ifaces;
  gchar **ifaces;	/* supported interfaces */
  guint   n_props;
  gchar **props;	/* property names */
  guint   n_signals;
  gchar **signals;	/* notification slots */
} SfiGlueIFace;

gchar*		sfi_glue_base_iface	(void);
gchar**		sfi_glue_iface_children	(const gchar	*iface_name);
gchar*		sfi_glue_proxy_iface	(SfiProxy	 proxy);
SfiGlueIFace*	sfi_glue_describe_iface	(const gchar	*iface_name);
SfiGlueIFace*	sfi_glue_iface_ref	(SfiGlueIFace	*iface);
void		sfi_glue_iface_unref	(SfiGlueIFace	*iface);

GParamSpec*	sfi_glue_describe_prop	(SfiProxy	 proxy,
					 const gchar	*prop_name);

void		sfi_glue_proxy_set_prop	(SfiProxy        proxy,
					 const gchar    *prop,
					 GValue         *value);
GValue*		sfi_glue_proxy_get_prop	(SfiProxy        proxy,
					 const gchar    *prop);


/* Procedure description in terms of it's parameters
 */
typedef struct {
  guint        ref_count;
  gchar       *proc_name;
  GParamSpec  *ret_param;
  guint        n_params;
  GParamSpec **params;
} SfiGlueProc;

SfiGlueProc*	sfi_glue_describe_proc		(const gchar	*proc_name);
SfiGlueProc*	sfi_glue_proc_ref		(SfiGlueProc	*proc);
void		sfi_glue_proc_unref		(SfiGlueProc	*proc);
gchar**		sfi_glue_list_proc_names	(void);
gchar**		sfi_glue_list_method_names	(const gchar	*iface_name);

GValue*		sfi_glue_call_seq		(const gchar	*proc_name,
						 SfiSeq		*params);
GValue*		sfi_glue_call_valist		(const gchar	*proc_name,
                                                 guint8          first_arg_type,
						 va_list         var_args);
void		sfi_glue_vcall_void		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);
SfiBool		sfi_glue_vcall_bool		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);
SfiInt		sfi_glue_vcall_int		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);
SfiNum		sfi_glue_vcall_num		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);
SfiReal		sfi_glue_vcall_real		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);
const gchar*	sfi_glue_vcall_string		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);
const gchar*	sfi_glue_vcall_choice		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);
SfiProxy	sfi_glue_vcall_proxy		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);
SfiSeq*		sfi_glue_vcall_seq		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);
SfiRec*		sfi_glue_vcall_rec		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);
SfiFBlock*	sfi_glue_vcall_fblock		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);
SfiBBlock*	sfi_glue_vcall_bblock		(const gchar	*proc_name,
						 guint8		 first_arg_type,
						 ...);


/* Global glue context, captures memory pool and type/object
 * system bindings.
 */
typedef struct {
  SfiGlueIFace*         (*describe_iface)               (SfiGlueContext *context,
                                                         const gchar    *iface);
  GParamSpec*           (*describe_prop)                (SfiGlueContext *context,
                                                         SfiProxy        proxy,
                                                         const gchar    *prop_name);
  SfiGlueProc*          (*describe_proc)                (SfiGlueContext *context,
                                                         const gchar    *proc_name);
  gchar**               (*list_proc_names)              (SfiGlueContext *context);
  gchar**               (*list_method_names)            (SfiGlueContext *context,
                                                         const gchar    *iface_name);
  gchar*                (*base_iface)                   (SfiGlueContext *context);
  gchar**               (*iface_children)               (SfiGlueContext *context,
                                                         const gchar    *iface_name);
  gchar*                (*proxy_iface)                  (SfiGlueContext *context,
                                                         SfiProxy        proxy);
  GValue*               (*exec_proc)                    (SfiGlueContext *context,
							 const gchar    *proc_name,
                                                         SfiSeq         *params);
  gboolean              (*signal_connection)            (SfiGlueContext *context,
                                                         const gchar    *signal,
                                                         SfiProxy        proxy,
                                                         gboolean        enable_connection);
  void			(*proxy_set_prop)		(SfiGlueContext *context,
							 SfiProxy	 proxy,
							 const gchar	*prop,
							 GValue		*value);
  GValue*		(*proxy_get_prop)		(SfiGlueContext *context,
							 SfiProxy	 proxy,
							 const gchar	*prop);
  GValue*               (*client_msg)                   (SfiGlueContext *context,
                                                         const gchar    *msg,
                                                         GValue         *value);
} SfiGlueContextTable;
typedef void (*SfiGlueSignalFunc)  (gpointer      sig_data,
				    const gchar  *signal,
				    const SfiSeq *args);
struct _SfiGlueContext
{
  /*< private >*/
  SfiGlueContextTable    table;
  GHashTable		*sighash;
  SfiRing		*gc_signals;
  SfiRing		*sigqueue;
};
void		sfi_glue_context_push	 (SfiGlueContext	*context);
SfiGlueContext* sfi_glue_context_current (void);
void		sfi_glue_context_pop	 (void);
gulong	     sfi_glue_signal_connect	 (const gchar		*signal,
					  SfiProxy		 proxy,
					  SfiGlueSignalFunc      func,
					  gpointer		 sig_data,
					  GDestroyNotify	 sig_data_free);
void	     sfi_glue_signal_disconnect	 (const gchar		*signal,
					  SfiProxy		 proxy,
					  gulong		 connection_id);
GValue*      sfi_glue_client_msg	 (const gchar		*msg,
					  GValue		*value);
/* called from sfi_glue_receive() or vtable implementations */
void	 sfi_glue_enqueue_signal_event	 (const gchar		*signal,
					  SfiSeq		*args,
					  gboolean		 disabled);
void	 sfi_glue_context_dispatch	 (SfiGlueContext	*context);
gboolean sfi_glue_context_pending	 (SfiGlueContext	*context);


/* --- Glue utilities --- */
void		sfi_glue_gc_add		(gpointer	 data,
					 gpointer	 free_func); // void (*free_func) (gpointer data);
void		sfi_glue_gc_remove	(gpointer	 data,
					 gpointer	 free_func); // void (*free_func) (gpointer data);
void		sfi_glue_gc_free_now	(gpointer	 data,
					 gpointer	 free_func); // void (*free_func) (gpointer data);
void		sfi_glue_gc_run		(void);
/* sfi_glue_gc_free_now() shortcuts */
void	sfi_glue_gc_collect_value	(GValue		*value);
void	sfi_glue_gc_collect_iface	(SfiGlueIFace	*iface);
void	sfi_glue_gc_collect_proc	(SfiGlueProc	*proc);
void	sfi_glue_gc_collect_pspec	(GParamSpec	*pspec);


/* --- internal --- */
gboolean	_sfi_glue_gc_test		(gpointer	 data,
						 gpointer	 free_func);
SfiGlueIFace*	_sfi_glue_iface_new		(const gchar	*iface_name);
SfiGlueProc*	_sfi_glue_proc_new		(void);
void		_sfi_glue_proc_add_param	(SfiGlueProc	*proc,
						 GParamSpec	*param);
void		_sfi_glue_proc_add_ret_param	(SfiGlueProc	*proc,
						 GParamSpec	*param);


/* --- implementations --- */
void sfi_glue_context_common_init (SfiGlueContext            *context,
				   const SfiGlueContextTable *vtable);
static inline SfiGlueContext*
sfi_glue_fetch_context (const gchar *floc);
static inline SfiGlueContext*
sfi_glue_fetch_context (const gchar *floc)
{
  SfiGlueContext *context = sfi_glue_context_current ();

  if (!context)
    g_error ("%s: SfiGlue function called without context (use sfi_glue_context_push())", floc);

  return context;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SFI_GLUE_H__ */

/* vim:set ts=8 sts=2 sw=2: */
