/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_GCONFIG_H__
#define __BSE_GCONFIG_H__

#include	<bse/bseobject.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Global Config --- */
/* extern BseGConfig *bse_global_config; bsetype.h */
void               _bse_gconfig_init	(void);
void               bse_gconfig_apply	(SfiRec *rec);
GParamSpec*        bse_gconfig_pspec	(void);
void		   bse_gconfig_lock	(void);
void		   bse_gconfig_unlock	(void);
gboolean	   bse_gconfig_locked	(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_GCONFIG_H__ */
