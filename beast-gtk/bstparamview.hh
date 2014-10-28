// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PARAM_VIEW_H__
#define __BST_PARAM_VIEW_H__

#include	"bstutils.hh"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_PARAM_VIEW              (bst_param_view_get_type ())
#define BST_PARAM_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_PARAM_VIEW, BstParamView))
#define BST_PARAM_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_PARAM_VIEW, BstParamViewClass))
#define BST_IS_PARAM_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_PARAM_VIEW))
#define BST_IS_PARAM_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_PARAM_VIEW))
#define BST_PARAM_VIEW_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_PARAM_VIEW, BstParamViewClass))


/* --- structures & typedefs --- */
typedef	struct	_BstParamView		BstParamView;
typedef	struct	_BstParamViewClass	BstParamViewClass;
struct _BstParamView
{
  GtkVBox	 parent_object;

  SfiProxy	 item;

  GSList	*params;        /* GxkParam* */

  gchar         *first_base_type;
  gchar         *last_base_type;
  GPatternSpec  *reject_pattern;
  GPatternSpec  *match_pattern;
};
struct _BstParamViewClass
{
  GtkVBoxClass parent_class;
};


/* --- prototypes --- */
GType		bst_param_view_get_type		(void);
GtkWidget*	bst_param_view_new		(SfiProxy	 item);
void		bst_param_view_rebuild		(BstParamView	*param_view);
void		bst_param_view_apply_defaults	(BstParamView	*param_view);
void		bst_param_view_set_item		(BstParamView	*param_view,
						 SfiProxy	 item);
void		bst_param_view_set_mask		(BstParamView	*param_view,
						 const gchar    *first_base_type,
						 const gchar    *last_base_type,
						 const gchar	*reject_pattern,
						 const gchar	*match_pattern);



G_END_DECLS

#endif /* __BST_PARAM_VIEW_H__ */
