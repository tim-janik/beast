/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
 *
 * bseproject.h: Coordinator for distinct super objects
 */
#ifndef __BSE_PROJECT_H__
#define __BSE_PROJECT_H__

#include        <bse/bsecontainer.h>


/* --- object type macros --- */
#define	BSE_TYPE_PROJECT	      (BSE_TYPE_ID (BseProject))
#define BSE_PROJECT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PROJECT, BseProject))
#define BSE_PROJECT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PROJECT, BseProjectClass))
#define BSE_IS_PROJECT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PROJECT))
#define BSE_IS_PROJECT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PROJECT))
#define BSE_PROJECT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BseProjectClass))


/* --- BseProject object --- */
struct _BseProject
{
  BseContainer	 parent_object;

  GSList	     *supers;
};
struct _BseProjectClass
{
  BseContainerClass parent_class;
};


/* --- prototypes --- */
BseProject*	bse_project_new			(const gchar	*name);
void		bse_project_start_playback	(BseProject	*project);
void		bse_project_stop_playback	(BseProject	*project);
void		bse_project_add_super		(BseProject	*project,
						 BseSuper	*super);
void		bse_project_remove_super	(BseProject	*project,
						 BseSuper	*super);
GList*		bse_project_list_supers		(BseProject	*project,
						 GType  	 super_type);
GList* /*fr*/	bse_project_list_nick_paths	(BseProject	*project,
						 GType  	 item_type);
BseItem*	bse_project_item_from_nick_path	(BseProject	*project,
						 const gchar	*nick_path);
BseErrorType	bse_project_restore		(BseProject	*project,
						 BseStorage	*storage);
BseErrorType	bse_project_store_bse		(BseProject	*project,
						 const gchar	*bse_file);
BseObject*	bse_project_path_resolver	(gpointer        project /* func_data */,
						 BseStorage     *storage,
						 GType           required_type,
						 const gchar    *path);
     

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PROJECT_H__ */
