/* GtkListWrapper - GtkListModel implementation as a simple list wrapper
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GTK_LIST_WRAPPER_H__
#define __GTK_LIST_WRAPPER_H__

#include <gtk/gtktreemodel.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* --- type macros --- */
#define GTK_TYPE_LIST_WRAPPER              (gtk_list_wrapper_get_type ())
#define GTK_LIST_WRAPPER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GTK_TYPE_LIST_WRAPPER, GtkListWrapper))
#define GTK_LIST_WRAPPER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_LIST_WRAPPER, GtkListWrapperClass))
#define GTK_IS_LIST_WRAPPER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GTK_TYPE_LIST_WRAPPER))
#define GTK_IS_LIST_WRAPPER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_LIST_WRAPPER))
#define GTK_LIST_WRAPPER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GTK_TYPE_LIST_WRAPPER, GtkListWrapperClass))


/* --- structures & typedefs --- */
typedef struct _GtkListWrapper      GtkListWrapper;
typedef struct _GtkListWrapperClass GtkListWrapperClass;
struct _GtkListWrapper
{
  GObject parent_instance;

  guint  n_rows;
  guint  n_cols;
  GType *column_types;

  /*< private >*/
  guint	 stamp;
};
struct _GtkListWrapperClass
{
  GObjectClass parent_class;

  void	(*fill_value)	(GtkListWrapper	*self,
			 guint		 column,
			 guint		 row,
			 GValue		*value);
};


/* --- prototypes --- */
GType		gtk_list_wrapper_get_type	(void);
GtkListWrapper*	gtk_list_wrapper_new		(guint		 n_cols,
						 GType		 first_column_type,
						 ...);
GtkListWrapper*	gtk_list_wrapper_newv		(guint		 n_cols,
						 GType		*column_types);
void		gtk_list_wrapper_notify_insert	(GtkListWrapper	*self,
						 guint		 nth_row);
void		gtk_list_wrapper_notify_change	(GtkListWrapper	*self,
						 guint		 nth_row);
void		gtk_list_wrapper_notify_delete	(GtkListWrapper	*self,
						 guint		 nth_row);
void		gtk_list_wrapper_notify_prepend	(GtkListWrapper	*self,
						 guint		 n_rows);
void		gtk_list_wrapper_notify_append	(GtkListWrapper	*self,
						 guint		 n_rows);
void		gtk_list_wrapper_notify_clear	(GtkListWrapper	*self);
guint		gtk_list_wrapper_get_index	(GtkListWrapper	*self,
						 GtkTreeIter	*iter);
void		gtk_list_wrapper_get_iter_at	(GtkListWrapper	*self,
						 GtkTreeIter	*iter,
						 guint		 index);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_LIST_WRAPPER_H__ */
