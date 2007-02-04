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
#ifndef __SFI_VMARSHAL_H__
#define __SFI_VMARSHAL_H__

#include <sfi/sfitypes.h>

G_BEGIN_DECLS

/* --- hard limit --- */
#define	SFI_VMARSHAL_MAX_ARGS	5


/* --- invocations --- */
void	sfi_vmarshal_void	(gpointer	 func,
				 gpointer	 arg0,
				 guint		 n_args,
				 const GValue	*args,  /* 1..n */
				 gpointer	 data); /* n+1 */


/* --- internal --- */
#if GLIB_SIZEOF_VOID_P == 4
#define SFI_VMARSHAL_PTR_ID  1
#else
#define SFI_VMARSHAL_PTR_ID  2
#endif

G_END_DECLS

#endif /* __SFI_VMARSHAL_H__ */

/* vim:set ts=8 sts=2 sw=2: */
