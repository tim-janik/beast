/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000-2002 Tim Janik and Red Hat, Inc.
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
#ifndef __BST_PLAY_LIST_H__
#define __BST_PLAY_LIST_H__

#include	"bstutils.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_PLAY_LIST	      (bst_play_list_get_type ())
#define	BST_PLAY_LIST(object)	      (GTK_CHECK_CAST ((object), BST_TYPE_PLAY_LIST, BstPlayList))
#define	BST_PLAY_LIST_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_PLAY_LIST, BstPlayListClass))
#define	BST_IS_PLAY_LIST(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_PLAY_LIST))
#define	BST_IS_PLAY_LIST_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_PLAY_LIST))
#define BST_PLAY_LIST_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_PLAY_LIST, BstPlayListClass))


/* --- structures & typedefs --- */
typedef	struct	_BstPlayList		BstPlayList;
typedef	struct	_BstPlayListClass	BstPlayListClass;
struct _BstPlayList
{
  GtkVPaned	 parent_object;

  BseSong	*song;

  GtkWidget	*pattern_list;
  GtkWidget	*group_list;
  GtkSizeGroup  *size_group;
};
struct _BstPlayListClass
{
  GtkVPanedClass parent_class;
};


/* --- prototypes --- */
GtkType		bst_play_list_get_type	(void);
GtkWidget*	bst_play_list_new		(BseSong	*song);
void		bst_play_list_set_song		(BstPlayList	*plist,
						 BseSong	*song);
void		bst_play_list_rebuild		(BstPlayList	*plist);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PLAY_LIST_H__ */
