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
#ifndef __SFI_SERIAL_H__
#define __SFI_SERIAL_H__

#include <sfi/sfivalues.h>

G_BEGIN_DECLS


/* --- functions --- */
void		sfi_value_store_typed		(const GValue	*value,
						 GString	*gstring);
void		sfi_value_store_param		(const GValue	*value,
						 GString	*gstring,
						 GParamSpec	*pspec,
						 guint		 indent);
GTokenType	sfi_value_parse_typed		(GValue		*value,
						 GScanner	*scanner);
GTokenType	sfi_value_parse_param_rest	(GValue		*value,
						 GScanner	*scanner,
						 GParamSpec	*pspec);
extern const GScannerConfig *sfi_storage_scanner_config;


G_END_DECLS

#endif /* __SFI_SERIAL_H__ */

/* vim:set ts=8 sts=2 sw=2: */
