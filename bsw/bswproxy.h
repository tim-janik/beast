/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSW_PROXY_H__
#define __BSW_PROXY_H__

extern char *bsw_log_domain_bsw;
#include        <bse/bswcommon.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define	BSW_SERVER		(bsw_proxy_get_server ())

typedef struct
{
  guint  n_in_params;
  GValue in_params[16];
  GValue out_param;
  gchar *proc_name;
} BswProxyProcedureCall;
typedef struct
{
  gpointer        lock_data;
  void          (*lock)         (gpointer lock_data);
  void          (*unlock)       (gpointer lock_data);
} BswLockFuncs;

void		bsw_init			(gint			*argc,
						 gchar		       **argv[],
						 const BswLockFuncs	*lock_funcs);
void		bsw_proxy_call_procedure	(BswProxyProcedureCall	*closure);
BswProxy	bsw_proxy_get_server		(void)	G_GNUC_CONST;
void		bsw_proxy_set			(BswProxy		 proxy,
						 const gchar		*prop,
						 ...);
GParamSpec*	bsw_proxy_get_pspec		(BswProxy		 proxy,
						 const gchar		*name);
GType		bsw_proxy_type			(BswProxy		 proxy);


/* --- garbage collection --- */
gchar*		bsw_collector_get_string	(GValue			*value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_PROXY_H__ */
