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
#ifndef __BST_TOOLBAR_H__
#define __BST_TOOLBAR_H__

#include "bstutils.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BST_TYPE_TOOLBAR		(bst_toolbar_get_type ())
#define BST_TOOLBAR(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_TOOLBAR, BstToolbar))
#define BST_TOOLBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_TOOLBAR, BstToolbarClass))
#define BST_IS_TOOLBAR(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_TOOLBAR))
#define BST_IS_TOOLBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_TOOLBAR))
#define BST_TOOLBAR_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BST_TYPE_TOOLBAR, BstToolbarClass))


/* --- toolbar widget --- */
typedef struct _BstToolbar	     BstToolbar;
typedef struct _BstToolbarClass BstToolbarClass;
struct _BstToolbar
{
  GtkFrame       parent_object;

  GtkReliefStyle relief_style;
  GtkIconSize	 icon_size;
  gint		 icon_width;
  gint		 icon_height;
  GtkSizeGroup  *size_group;
  GtkWidget	*box;
};
struct _BstToolbarClass
{
  GtkFrameClass parent_class;
};
typedef enum {
  BST_TOOLBAR_SPACE,
  BST_TOOLBAR_SEPARATOR,
  BST_TOOLBAR_BUTTON,
  BST_TOOLBAR_TRUNC_BUTTON,
  BST_TOOLBAR_EXTRA_BUTTON,
  BST_TOOLBAR_TOGGLE,
  BST_TOOLBAR_TRUNC_TOGGLE,
  BST_TOOLBAR_EXTRA_TOGGLE,
  BST_TOOLBAR_WIDGET,
  BST_TOOLBAR_TRUNC_WIDGET,
  BST_TOOLBAR_EXTRA_WIDGET
} BstToolbarChild;
typedef void (*BstToolbarChoiceFunc)	(gpointer	data,
					 guint		choice);


/* --- API --- */
GType		bst_toolbar_get_type		(void);
BstToolbar*	bst_toolbar_new			(gpointer	 nullify_pointer);
GtkWidget*	bst_toolbar_append		(BstToolbar	*self,
						 BstToolbarChild child_type,
						 const gchar	*name,
						 const gchar	*tooltip,
						 GtkWidget	*icon);
GtkWidget*	bst_toolbar_append_stock	(BstToolbar	*self,
						 BstToolbarChild child_type,
						 const gchar	*name,
						 const gchar	*tooltip,
						 const gchar    *stock_id);
#define	bst_toolbar_append_space(self)		bst_toolbar_append ((self), BST_TOOLBAR_SPACE, 0, 0, 0)
#define	bst_toolbar_append_separator(self)	bst_toolbar_append ((self), BST_TOOLBAR_SEPARATOR, 0, 0, 0)
GtkWidget*	bst_toolbar_append_choice	(BstToolbar	*self,
						 BstToolbarChild child_type,
						 BstToolbarChoiceFunc choice_func,
						 gpointer        data,
						 GDestroyNotify  data_free);
void		bst_toolbar_choice_add		(GtkWidget	*widget,
						 const gchar	*name,
						 const gchar	*tooltip,
						 GtkWidget	*icon,
						 guint           choice);
void		bst_toolbar_choice_set		(GtkWidget	*widget,
						 const gchar	*name,
						 const gchar	*tooltip,
						 GtkWidget	*icon,
						 guint           choice);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_TOOLBAR_H__ */

