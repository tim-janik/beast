/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#ifndef __BST_ITEM_VIEW_H__
#define __BST_ITEM_VIEW_H__

#include	"bstutils.h"

G_BEGIN_DECLS

/* --- Gtk+ type macros --- */
#define	BST_TYPE_ITEM_VIEW	      (bst_item_view_get_type ())
#define	BST_ITEM_VIEW(object)	      (GTK_CHECK_CAST ((object), BST_TYPE_ITEM_VIEW, BstItemView))
#define	BST_ITEM_VIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_ITEM_VIEW, BstItemViewClass))
#define	BST_IS_ITEM_VIEW(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_ITEM_VIEW))
#define	BST_IS_ITEM_VIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_ITEM_VIEW))
#define BST_ITEM_VIEW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_ITEM_VIEW, BstItemViewClass))

#define	BST_ITEM_VIEW_TREE_HEIGHT     (120)


/* --- structures & typedefs --- */
typedef	struct	_BstItemView		BstItemView;
typedef	struct	_BstItemViewClass	BstItemViewClass;
struct _BstItemView
{
  GtkAlignment	  parent_object;

  GtkTreeView    *tree;
  GxkListWrapper *wlist;

  GtkWidget	 *pview;

  SfiProxy	 container;
  SfiProxy	 auto_select;
  GtkWidget    **op_widgets;
};
struct _BstItemViewClass
{
  GtkAlignmentClass parent_class;

  const gchar	   *item_type;

  void	      (*set_container)	(BstItemView	*self,
				 SfiProxy	 new_container);
  void	      (*listen_on)	(BstItemView	*self,
				 SfiProxy	 item);
  void	      (*unlisten_on)	(BstItemView	*self,
				 SfiProxy	 item);
};


/* --- prototypes --- */
GType		bst_item_view_get_type		(void);
void		bst_item_view_select		(BstItemView	*item_view,
						 SfiProxy	 item);
SfiProxy	bst_item_view_get_current	(BstItemView	*item_view);
SfiProxy	bst_item_view_get_proxy		(BstItemView	*item_view,
						 gint            row);
gint            bst_item_view_get_proxy_row     (BstItemView    *self,
                                                 SfiProxy        item);
void		bst_item_view_set_container	(BstItemView	*item_view,
						 SfiProxy	 new_container);
void		bst_item_view_set_tree  	(BstItemView	*item_view,
						 GtkTreeView    *tree);
void            bst_item_view_complete_tree     (BstItemView    *self,
						 GtkTreeView    *tree);
void            bst_item_view_build_param_view  (BstItemView    *self,
                                                 GtkContainer   *container);
void		bst_item_view_refresh   	(BstItemView    *self,
						 SfiProxy        item);
void		bst_item_view_name_edited	(BstItemView    *self,
						 const gchar    *strpath,
						 const gchar    *text);
void		bst_item_view_blurb_edited	(BstItemView    *self,
						 const gchar    *strpath,
						 const gchar    *text);
void		bst_item_view_enable_param_view	(BstItemView    *self,
                                                 gboolean        enabled);
GtkTreeModel* bst_item_view_adapt_list_wrapper	(BstItemView	*self,
						 GxkListWrapper *lwrapper);


G_END_DECLS

#endif /* __BST_ITEM_VIEW_H__ */
