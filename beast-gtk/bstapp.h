/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_APP_H__
#define __BST_APP_H__


#include        "bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BST_TYPE_APP            (bst_app_get_type ())
#define BST_APP(object)         (GTK_CHECK_CAST ((object), BST_TYPE_APP, BstApp))
#define BST_APP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_APP, BstAppClass))
#define BST_IS_APP(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_APP))
#define BST_IS_APP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_APP))


/* --- object & class member/convenience macros --- */
#define	BST_APP_PROJECT(app)	(BST_APP (app)->project)


/* --- typedefs --- */
typedef struct  _BstApp       BstApp;
typedef struct  _BstAppClass  BstAppClass;


/* --- structures --- */
struct _BstApp
{
  GtkWindow      window;

  BseProject	*project;

  GtkWidget	*main_vbox;
  GtkNotebook	*notebook;

  GtkWidget	*quit_dialog;
  BseSuper	*default_super;
};
struct _BstAppClass
{
  GtkWindowClass        parent_class;
  GSList               *apps;
};


/* --- prototypes --- */
void		bst_app_register		(void);
GtkType		bst_app_get_type		(void);
BstApp*		bst_app_new			(BseProject	*project);
gboolean	bst_app_can_operate		(BstApp         *app,
						 BstOps		 op);
void		bst_app_operate			(BstApp         *app,
						 BstOps		 op);
void		bst_app_reload_supers		(BstApp		*app);
void		bst_app_create_default		(BstApp		*app);
GtkWidget*	bst_app_get_current_shell	(BstApp		*app);
BseSuper*	bst_app_get_current_super	(BstApp		*app);
GtkItemFactory*	bst_app_menu_factory		(BstApp		*app);
void		bst_app_update_can_operate	(BstApp		*app);
     




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* __BST_APP_H__ */
