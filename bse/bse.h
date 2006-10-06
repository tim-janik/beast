/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
#ifndef __BSE_H__
#define __BSE_H__

#include <sfi/sfi.h>
#include <bse/bseconfig.h>

G_BEGIN_DECLS

/* initialize BSE and start the core thread */
void		bse_init_async		(gint		 *argc,
					 gchar	       ***argv,
					 const char     *app_name,
					 SfiInitValue    values[]);
/* provide SFI glue layer context for BSE */
SfiGlueContext*	bse_init_glue_context	(const gchar	*client);

/* library versioning */
extern const guint   bse_major_version;
extern const guint   bse_minor_version;
extern const guint   bse_micro_version;
extern const guint   bse_interface_age;
extern const guint   bse_binary_age;
extern const gchar  *bse_version;
gchar*               bse_check_version	(guint           required_major,
					 guint           required_minor,
					 guint           required_micro);

G_END_DECLS

#endif /* __BSE_H__ */
