/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
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
#ifndef __BST_MSG_ABSORB_H__
#define __BST_MSG_ABSORB_H__

#include "bstutils.h"

G_BEGIN_DECLS

/* --- access config file --- */
#define BST_STRDUP_ABSORBRC_FILE()  (g_strconcat (g_get_home_dir (), "/.beast/absorbrc", NULL))

/* --- prototypes --- */
void		       _bst_msg_absorb_config_init	(void);
void		       bst_msg_absorb_config_apply	(SfiSeq         *seq);
GParamSpec*	       bst_msg_absorb_config_pspec      (void);
BstMsgAbsorbStringSeq* bst_msg_absorb_config_get_global (void);
gboolean               bst_msg_absorb_config_adjust     (const gchar    *config_blurb,
                                                         gboolean        enabled,
                                                         gboolean        update_version);
gboolean               bst_msg_absorb_config_match      (const gchar    *config_blurb);
void                   bst_msg_absorb_config_update     (const gchar    *config_blurb);
GtkWidget*	       bst_msg_absorb_config_box	(void);
void                   bst_msg_absorb_config_box_set    (GtkWidget      *box,
                                                         BstMsgAbsorbStringSeq *mass);
BstMsgAbsorbStringSeq* bst_msg_absorb_config_box_get    (GtkWidget      *box);

/* --- config file --- */
void                   bst_msg_absorb_config_save       (void);
void                   bst_msg_absorb_config_load       (void);

G_END_DECLS

#endif /* __BST_MSG_ABSORB_H__ */
