/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_CONTROLLERS_H__
#define __BST_CONTROLLERS_H__

#include        "bstdefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct  _BstControllerInfo      BstControllerInfo;

struct _BstControllerInfo
{
  GType           value_type;
  gchar          *name;
  gsize           controller_data;
  gboolean      (*check)        (GParamSpec     *pspec,
				 gsize           controller_data);
  GtkWidget*    (*create)       (GParamSpec     *pspec,
				 GCallback       notify,
				 gpointer        notify_data,
				 gsize           controller_data);
  void          (*update)       (GtkWidget      *widget,
				 GParamSpec     *pspec,
				 const GValue   *value,
				 gsize           controller_data);
  void          (*fetch)        (GtkWidget      *widget,
				 GParamSpec     *pspec,
				 GValue         *value,
				 gsize           controller_data);
};


/* --- controller operations --- */
guint		bst_controller_check	(BstControllerInfo	*cinfo,
					 GParamSpec		*pspec);
GtkWidget*	bst_controller_create	(BstControllerInfo	*cinfo,
					 GParamSpec		*pspec,
					 GCallback		 changed_notify,
					 gpointer		 notify_data);
void		bst_controller_update	(GtkWidget		*widget,
					 const GValue		*value);
void		bst_controller_fetch	(GtkWidget		*widget,
					 GValue			*value);

/* --- find controllers --- */
GSList*		   bst_controller_list		(void);
BstControllerInfo* bst_controller_lookup	(const gchar	*name,
						 GParamSpec	*pspec);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_CONTROLLERS_H__ */
