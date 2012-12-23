// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_EFFECT_VIEW_H__
#define __BST_EFFECT_VIEW_H__
#include	"bstitemview.hh"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* --- Gtk+ type macros --- */
#define	BST_TYPE_EFFECT_VIEW	        (bst_effect_view_get_type ())
#define	BST_EFFECT_VIEW(object)	        (GTK_CHECK_CAST ((object), BST_TYPE_EFFECT_VIEW, BstEffectView))
#define	BST_EFFECT_VIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_EFFECT_VIEW, BstEffectViewClass))
#define	BST_IS_EFFECT_VIEW(object)	(GTK_CHECK_TYPE ((object), BST_TYPE_EFFECT_VIEW))
#define	BST_IS_EFFECT_VIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_EFFECT_VIEW))
#define BST_EFFECT_VIEW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_EFFECT_VIEW, BstEffectViewClass))
/* --- structures & typedefs --- */
typedef	struct	_BstEffectView	    BstEffectView;
typedef	struct	_BstEffectViewClass BstEffectViewClass;
struct _BstEffectView
{
  GtkAlignment	 parent_object;
  GtkWidget	*paned;
  GtkWidget	*clist_aeffects;	/* available effects */
  GtkWidget	*clist_peffects;	/* present effects */
  GtkWidget	*param_view;
  GtkWidget	*add_button;
  GtkWidget	*remove_button;
  BsePattern	*pattern;
  guint		 channel;
  guint		 row;
};
struct _BstEffectViewClass
{
  GtkAlignmentClass parent_class;
  guint		    default_param_view_height;
};
/* --- prototypes --- */
GtkType		bst_effect_view_get_type	(void);
GtkWidget*	bst_effect_view_new		(BseSong	*song);
void		bst_effect_view_set_note	(BstEffectView	*effect_view,
						 BsePattern	*pattern,
						 guint		 channel,
						 guint		 row);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __BST_EFFECT_VIEW_H__ */
