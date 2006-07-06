/* Birnet
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BIRNET_UTILS_H__
#define __BIRNET_UTILS_H__

#include <birnet/birnetcore.h>

BIRNET_EXTERN_C_BEGIN();

/* --- url handling --- */
void birnet_url_show                  (const char           *url);
void birnet_url_show_with_cookie      (const char           *url,
				       const char           *url_title,
				       const char           *cookie);
bool birnet_url_test_show             (const char           *url);
bool birnet_url_test_show_with_cookie (const char	    *url,
				       const char           *url_title,
				       const char           *cookie);
/* --- cleanup registration --- */
uint birnet_cleanup_add               (uint                  timeout_ms,
				       GDestroyNotify        handler,
				       void                 *data);
void birnet_cleanup_force_handlers    (void);

/* --- zintern support --- */
guint8* birnet_zintern_decompress     (unsigned int          decompressed_size,
				       const unsigned char  *cdata,
				       unsigned int          cdata_size);

BIRNET_EXTERN_C_END();

#endif /* __BIRNET_UTILS_H__ */
/* vim:set ts=8 sts=2 sw=2: */
