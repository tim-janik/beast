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
#ifndef __BST_SPLASH_H__
#define __BST_SPLASH_H__

#include        "bstutils.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BST_TYPE_SPLASH              (bst_splash_get_type ())
#define BST_SPLASH(object)           (GTK_CHECK_CAST ((object), BST_TYPE_SPLASH, BstSplash))
#define BST_SPLASH_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_SPLASH, BstSplashClass))
#define BST_IS_SPLASH(object)        (GTK_CHECK_TYPE ((object), BST_TYPE_SPLASH))
#define BST_IS_SPLASH_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_SPLASH))
#define BST_SPLASH_GET_CLASS(splash) (G_TYPE_INSTANCE_GET_CLASS ((splash), BST_TYPE_SPLASH, BstSplashClass))


/* --- typedefs & enums --- */
typedef struct  _BstSplash       BstSplash;
typedef struct  _BstSplashClass  BstSplashClass;


/* --- structures --- */
struct _BstSplash
{
  GtkWindow       window;

  GtkWidget	 *vbox;

  GtkWidget	 *splash_box;

  GtkWidget	 *entity;
  GtkWidget	 *item;
  GtkProgressBar *pbar;
  guint		  item_count;
  guint		  max_items;
};
struct _BstSplashClass
{
  GtkWindowClass parent_class;
};


/* --- prototypes --- */
GtkType		bst_splash_get_type		(void);
GtkWidget*	bst_splash_new			(const gchar	*title,
						 guint		 splash_width,
						 guint		 splash_height,
						 guint		 max_items);
void		bst_splash_show_now		(GtkWidget	*widget);
void		bst_splash_set_text		(GtkWidget	*widget,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2, 3);
void		bst_splash_set_animation	(GtkWidget	*widget,
						 GdkPixbufAnimation *anim);
void		bst_splash_update		(void);
void		bst_splash_update_entity	(GtkWidget	*widget,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2, 3);
void		bst_splash_update_item		(GtkWidget	*widget,
						 const gchar	*format,
						 ...) G_GNUC_PRINTF (2, 3);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __BST_SPLASH_H__ */
