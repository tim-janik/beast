/* BstClueHunter: Completion popup with pattern matching for GtkEntry
 * Copyright (C) 1999-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BST_CLUE_HUNTER_H__
#define __BST_CLUE_HUNTER_H__

#include	<gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define	BST_TYPE_CLUE_HUNTER		(bst_clue_hunter_get_type ())
#define	BST_CLUE_HUNTER(object)	        (GTK_CHECK_CAST ((object), BST_TYPE_CLUE_HUNTER, BstClueHunter))
#define	BST_CLUE_HUNTER_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_CLUE_HUNTER, BstClueHunterClass))
#define	BST_IS_CLUE_HUNTER(object)	(GTK_CHECK_TYPE ((object), BST_TYPE_CLUE_HUNTER))
#define BST_IS_CLUE_HUNTER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_CLUE_HUNTER))
#define BST_CLUE_HUNTER_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_CLUE_HUNTER, BstClueHunterClass))


/* --- typedefs --- */
typedef	struct	_BstClueHunterClass BstClueHunterClass;
typedef	struct	_BstClueHunter	    BstClueHunter;


/* --- structures --- */
struct	_BstClueHunter
{
  GtkWindow	 window;

  guint		 popped_up : 1;
  guint		 completion_tag : 1;
  guint		 pattern_matching : 1;
  guint		 align_width : 1;
  guint		 keep_history : 1;
  guint		 clist_column : 16;

  gchar		*cstring;

  GtkWidget	*scw;
  GtkCList	*clist;
  GtkEntry	*entry;
};
struct	_BstClueHunterClass
{
  GtkWindowClass	parent_class;

  void	(*activate)	(BstClueHunter	*clue_hunter);
  void	(*popup)	(BstClueHunter	*clue_hunter);
  void	(*popdown)	(BstClueHunter	*clue_hunter);
  void	(*select_on)	(BstClueHunter	*clue_hunter,
			 const gchar	*string);
  void	(*poll_refresh)	(BstClueHunter	*clue_hunter);
};


/* --- prototypes --- */
GtkType	   bst_clue_hunter_get_type	        (void);
void	   bst_clue_hunter_popup	        (BstClueHunter	*clue_hunter);
void	   bst_clue_hunter_set_clist	        (BstClueHunter	*clue_hunter,
						 GtkCList	*clist,
						 guint16	 column);
void	   bst_clue_hunter_set_entry	        (BstClueHunter	*clue_hunter,
						 GtkEntry	*entry);
void	   bst_clue_hunter_add_string	        (BstClueHunter	*clue_hunter,
						 const gchar	*string);
void	   bst_clue_hunter_remove_string	(BstClueHunter	*clue_hunter,
						 const gchar	*string);
void	   bst_clue_hunter_remove_matches	(BstClueHunter	*clue_hunter,
						 const gchar	*pattern);
void	   bst_clue_hunter_select_on	        (BstClueHunter	*clue_hunter,
						 const gchar	*string);
void	   bst_clue_hunter_poll_refresh	        (BstClueHunter	*clue_hunter);
gchar*	   bst_clue_hunter_try_complete	        (BstClueHunter	*clue_hunter);
GtkWidget* bst_clue_hunter_create_arrow		(BstClueHunter	*clue_hunter);
BstClueHunter* bst_clue_hunter_from_entry	(GtkEntry	*entry);





#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif	/* __BST_CLUE_HUNTER_H__ */
