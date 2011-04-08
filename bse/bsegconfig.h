/* BSE - Better Sound Engine
 * Copyright (C) 1997-2002 Tim Janik
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
#ifndef __BSE_GCONFIG_H__
#define __BSE_GCONFIG_H__

#include	<bse/bseobject.h>
#include	<bse/bsemain.h>

G_BEGIN_DECLS

/* --- Global Config --- */
/* extern BseGConfig *bse_global_config; bsetype.h */
void               _bse_gconfig_init	  (void);
void               bse_gconfig_apply	  (SfiRec *rec);
GParamSpec*        bse_gconfig_pspec	  (void);
void		   bse_gconfig_lock	  (void);
void		   bse_gconfig_unlock	  (void);
gboolean	   bse_gconfig_locked	  (void);
void               bse_gconfig_merge_args (const BseMainArgs *margs);

G_END_DECLS

#endif /* __BSE_GCONFIG_H__ */
