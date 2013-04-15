// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_LIST_WRAPPER_H__
#define __GXK_LIST_WRAPPER_H__

#include <gtk/gtktreemodel.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_LIST_WRAPPER              (gxk_list_wrapper_get_type ())
#define GXK_LIST_WRAPPER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_LIST_WRAPPER, GxkListWrapper))
#define GXK_LIST_WRAPPER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_LIST_WRAPPER, GxkListWrapperClass))
#define GXK_IS_LIST_WRAPPER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_LIST_WRAPPER))
#define GXK_IS_LIST_WRAPPER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_LIST_WRAPPER))
#define GXK_LIST_WRAPPER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_LIST_WRAPPER, GxkListWrapperClass))


/* --- structures & typedefs --- */
typedef struct _GxkListWrapper      GxkListWrapper;
typedef struct _GxkListWrapperClass GxkListWrapperClass;
struct _GxkListWrapper
{
  GObject parent_instance;

  guint  n_rows;
  guint  n_cols;
  GType *column_types;

  /*< private >*/
  guint	 stamp;
};
struct _GxkListWrapperClass
{
  GObjectClass parent_class;

  void	(*fill_value)	(GxkListWrapper	*self,
			 guint		 column,
			 guint		 row,
			 GValue		*value);
  void	(*row_change)	(GxkListWrapper	*self,
			 gint		 row);
};


/* --- prototypes --- */
GType		gxk_list_wrapper_get_type	(void);
GxkListWrapper*	gxk_list_wrapper_new		(guint		 n_cols,
						 GType		 first_column_type,
						 ...);
GxkListWrapper*	gxk_list_wrapper_newv		(guint		 n_cols,
						 GType		*column_types);
void		gxk_list_wrapper_notify_insert	(GxkListWrapper	*self,
						 guint		 nth_row);
void		gxk_list_wrapper_notify_change	(GxkListWrapper	*self,
						 guint		 nth_row);
void		gxk_list_wrapper_notify_delete	(GxkListWrapper	*self,
						 guint		 nth_row);
void		gxk_list_wrapper_notify_prepend	(GxkListWrapper	*self,
						 guint		 n_rows);
void		gxk_list_wrapper_notify_append	(GxkListWrapper	*self,
						 guint		 n_rows);
void		gxk_list_wrapper_notify_clear	(GxkListWrapper	*self);
guint		gxk_list_wrapper_get_index	(GxkListWrapper	*self,
						 GtkTreeIter	*iter);
void		gxk_list_wrapper_get_iter_at	(GxkListWrapper	*self,
						 GtkTreeIter	*iter,
						 guint		 index);

G_END_DECLS

#endif /* __GXK_LIST_WRAPPER_H__ */
