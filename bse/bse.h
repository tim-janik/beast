/* BSE - Better Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
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
const char*          bse_check_version	(guint           required_major,
					 guint           required_minor,
					 guint           required_micro);

G_END_DECLS

#endif /* __BSE_H__ */
