/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999-2002 Tim Janik and Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BST_GCONFIG_H__
#define __BST_GCONFIG_H__

#include	"bstutils.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BstGConfig - configurable defaults --- */
#define	BST_RC_VERSION			(bst_global_config->rc_version)
#define BST_DISABLE_ALSA		(bst_global_config->disable_alsa)
#define BST_TAB_WIDTH			(bst_global_config->tab_width)
#define BST_SNET_ANTI_ALIASED		(bst_global_config->snet_anti_aliased)
#define BST_SNET_EDIT_FALLBACK		(bst_global_config->snet_edit_fallback)
#define BST_SNET_SWAP_IO_CHANNELS	(bst_global_config->snet_swap_io_channels)

extern BstGConfig *bst_global_config;


/* --- prototypes --- */
void		_bst_gconfig_init		(void);
void		bst_gconfig_set_rc_version	(const gchar	*rc_version);
void		bst_gconfig_apply		(SfiRec		*rec);
GParamSpec*	bst_gconfig_pspec		(void);


/* --- rc file --- */
BseErrorType     bst_rc_dump                    (const gchar    *file_name);
BseErrorType     bst_rc_parse                   (const gchar    *file_name);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_GCONFIG_H__ */
