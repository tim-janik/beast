// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_LOG_ADJUSTMENT_H__
#define __BST_LOG_ADJUSTMENT_H__

#include	<gtk/gtkadjustment.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_LOG_ADJUSTMENT              (bst_log_adjustment_get_type ())
#define BST_LOG_ADJUSTMENT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_LOG_ADJUSTMENT, BstLogAdjustment))
#define BST_LOG_ADJUSTMENT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_LOG_ADJUSTMENT, BstLogAdjustmentClass))
#define BST_IS_LOG_ADJUSTMENT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_LOG_ADJUSTMENT))
#define BST_IS_LOG_ADJUSTMENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_LOG_ADJUSTMENT))
#define BST_LOG_ADJUSTMENT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_LOG_ADJUSTMENT, BstLogAdjustmentClass))


/* --- structures & typedefs --- */
typedef	struct	_BstLogAdjustment	BstLogAdjustment;
typedef	struct	_BstLogAdjustmentClass	BstLogAdjustmentClass;
struct _BstLogAdjustment
{
  GtkAdjustment	parent_instance;

  /* settings */
  gdouble	 center;
  gdouble	 n_steps;
  gdouble	 base;
  GtkAdjustment *client;

  guint		 block_client;
  gdouble	 base_ln;
  gdouble	 llimit;
  gdouble	 ulimit;
};
struct _BstLogAdjustmentClass
{
  GtkAdjustmentClass parent_class;
};


/* --- prototypes --- */
GType		bst_log_adjustment_get_type	(void);
void		bst_log_adjustment_set_client	(BstLogAdjustment	*ladj,
						 GtkAdjustment		*client);
GtkAdjustment*	bst_log_adjustment_from_adj	(GtkAdjustment		*client);
void		bst_log_adjustment_setup	(BstLogAdjustment	*ladj,
						 gdouble		 center,
						 gdouble		 base,
						 gdouble		 n_steps);

G_END_DECLS

#endif /* __BST_LOG_ADJUSTMENT_H__ */
