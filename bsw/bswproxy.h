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

#include        <bse/bse.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



SfiProxy	bsw_proxy_get_server (void);

#define	BSE_SERVER			(bsw_proxy_get_server ())

#define	bse_proxy_set_property		sfi_glue_proxy_set_property
#define	bse_proxy_get_property		sfi_glue_proxy_get_property
#define	bse_proxy_set			sfi_glue_proxy_set
#define	bse_proxy_get			sfi_glue_proxy_get
#define	bse_proxy_get_pspec		sfi_glue_proxy_get_pspec
#define	bse_proxy_list_properties	sfi_glue_proxy_list_properties
#define	bse_proxy_disconnect		sfi_glue_proxy_disconnect
#define	bse_proxy_connect		sfi_glue_proxy_connect
#define	bse_proxy_pending		sfi_glue_proxy_pending
#define	bse_proxy_is_a			sfi_glue_proxy_is_a
#define	bse_proxy_get_qdata		sfi_glue_proxy_get_qdata
#define	bse_proxy_set_qdata_full	sfi_glue_proxy_set_qdata_full
#define	bse_proxy_steal_qdata		sfi_glue_proxy_steal_qdata
#define	bse_proxy_set_qdata(p,q,d)	bse_proxy_set_qdata_full ((p), (q), (d), NULL)
#define	bse_proxy_set_data(p,n,d)	bse_proxy_set_qdata ((p), g_quark_from_string (n), (d))
#define	bse_proxy_get_data(p,n)		bse_proxy_get_qdata ((p), g_quark_try_string (n))
#define	bse_proxy_steal_data(p,n)	bse_proxy_steal_qdata ((p), g_quark_try_string (n))
#define	bse_proxy_set_data_full(p,n,d,f) bse_proxy_set_qdata_full ((p), g_quark_from_string (n), (d), (f))


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_PROXY_H__ */
