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
#ifndef __SFI_VCALL_H__
#define __SFI_VCALL_H__

#include <sfi/sfitypes.h>

G_BEGIN_DECLS

/* --- hard limit --- */
#define	SFI_VCALL_MAX_ARGS	5


/* --- invocations --- */
void	sfi_vcall_void	(gpointer	 func,
			 gpointer	 arg0,
			 guint		 n_args,
			 GValue		*args,  /* 1..n */
			 gpointer	 data); /* n+1 */


/* --- internal --- */
#if GLIB_SIZEOF_VOID_P == 4
#define SFI_VCALL_PTR_ID  1
#else
#define SFI_VCALL_PTR_ID  2
#endif

G_END_DECLS

#endif /* __SFI_VCALL_H__ */

/* vim:set ts=8 sts=2 sw=2: */
