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
#ifndef __SFI_GLUE_PROXY_H__
#define __SFI_GLUE_PROXY_H__

#include <sfi/sfiglue.h>

G_BEGIN_DECLS


/* --- typedefs --- */
typedef enum /*< skip >*/
{
  SFI_GLUE_EVENT_RELEASE	= ('G' << 16) | ('e' << 8) | 'R',
  SFI_GLUE_EVENT_NOTIFY		= ('G' << 16) | ('e' << 8) | 'N',
  SFI_GLUE_EVENT_NOTIFY_CANCEL	= ('G' << 16) | ('e' << 8) | 'C'
} SfiGlueEventType;
typedef void (*SfiProxyDestroy)	(gpointer	data,
				 SfiProxy	destroyed_proxy);


/* --- functions --- */
const gchar*	sfi_glue_proxy_iface		(SfiProxy	 proxy);
gboolean	sfi_glue_proxy_is_a		(SfiProxy	 proxy,
						 const gchar	*type);
void		sfi_glue_proxy_connect		(SfiProxy	 proxy,
						 const gchar	*signal,
						 ...);
void		sfi_glue_proxy_disconnect	(SfiProxy	 proxy,
						 const gchar	*signal,
						 ...);
gboolean	sfi_glue_proxy_pending		(SfiProxy	 proxy,
						 const gchar	*signal,
						 GCallback	 callback,
						 gpointer	 data);
void		sfi_glue_proxy_set_qdata_full	(SfiProxy	 proxy,
						 GQuark		 quark,
						 gpointer	 data,
						 GDestroyNotify	 destroy);
gpointer	sfi_glue_proxy_get_qdata	(SfiProxy	 proxy,
						 GQuark		 quark);
gpointer	sfi_glue_proxy_steal_qdata	(SfiProxy	 proxy,
						 GQuark		 quark);
void		sfi_glue_proxy_weak_ref		(SfiProxy	 proxy,
						 SfiProxyDestroy weak_notify,
						 gpointer	 data);
void		sfi_glue_proxy_weak_unref	(SfiProxy	 proxy,
						 SfiProxyDestroy weak_notify,
						 gpointer	 data);
void		sfi_glue_proxy_set		(SfiProxy	 proxy,
						 const gchar	*prop,
						 ...);
void		sfi_glue_proxy_get		(SfiProxy	 proxy,
						 const gchar	*prop,
						 ...);
void		sfi_glue_proxy_set_property	(SfiProxy	 proxy,
						 const gchar	*prop,
						 const GValue	*value);
const GValue*	sfi_glue_proxy_get_property	(SfiProxy	 proxy,
						 const gchar	*prop);
GParamSpec*	sfi_glue_proxy_get_pspec	(SfiProxy	 proxy,
						 const gchar	*name);
SfiSCategory sfi_glue_proxy_get_pspec_scategory (SfiProxy	 proxy,
						 const gchar	*name);
const gchar**	sfi_glue_proxy_list_properties	(SfiProxy	 proxy,
						 const gchar	*first_ancestor,
						 const gchar	*last_ancestor,
						 guint		*n_props);
gulong		sfi_glue_signal_connect_data	(SfiProxy	 proxy,
						 const gchar	*signal,
						 gpointer	 sig_func,
						 gpointer	 sig_data,
						 GClosureNotify  sig_data_destroy,
						 GConnectFlags   connect_flags);
#define sfi_glue_signal_connect(p,s,f,d)         sfi_glue_signal_connect_data ((p), (s), (f), (d), NULL, 0)
#define sfi_glue_signal_connect_swapped(p,s,f,d) sfi_glue_signal_connect_data ((p), (s), (f), (d), NULL, G_CONNECT_SWAPPED)
gulong		sfi_glue_signal_connect_closure	(SfiProxy	 proxy,
						 const gchar	*signal,
						 GClosure	*closure,
						 gpointer        search_data);
void		sfi_glue_signal_disconnect	(SfiProxy	 proxy,
						 gulong		 connection_id);


/* --- internal --- */
gboolean    _sfi_glue_proxy_watch_release	(SfiProxy	 proxy);
void	    _sfi_glue_proxy_processed_notify	(guint		 notify_id);
void	    _sfi_glue_context_clear_proxies	(SfiGlueContext	*context);
void	    _sfi_glue_proxy_dispatch_event	(SfiSeq		*event);
GQuark	    sfi_glue_proxy_get_signal_quark	(const gchar	*signal);
void	    sfi_glue_proxy_cancel_matched_event	(SfiSeq		*event,
						 SfiProxy        proxy,
						 GQuark		 signal_quark);


G_END_DECLS

#endif /* __SFI_GLUE_PROXY_H__ */

/* vim:set ts=8 sts=2 sw=2: */
