// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PROJECT_CTRL_H__
#define __BST_PROJECT_CTRL_H__
#include "bstutils.hh"
G_BEGIN_DECLS
/* --- type macros --- */
#define BST_TYPE_PROJECT_CTRL              (bst_project_ctrl_get_type ())
#define BST_PROJECT_CTRL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_PROJECT_CTRL, BstProjectCtrl))
#define BST_PROJECT_CTRL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_PROJECT_CTRL, BstProjectCtrlClass))
#define BST_IS_PROJECT_CTRL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_PROJECT_CTRL))
#define BST_IS_PROJECT_CTRL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_PROJECT_CTRL))
#define BST_PROJECT_CTRL_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_PROJECT_CTRL, BstProjectCtrlClass))
/* --- typedefs & structures --- */
typedef struct {
  GtkHBox	 parent_instance;
  SfiProxy	 project;
  GxkLed	*led;
  GtkWidget	*stop;
  GtkWidget	*rew;
  GtkWidget	*play;
  GtkWidget	*pause;
  GtkWidget	*fwd;
} BstProjectCtrl;
typedef GtkHBoxClass BstProjectCtrlClass;
/* --- prototypes --- */
GType		bst_project_ctrl_get_type	(void);
void		bst_project_ctrl_set_project	(BstProjectCtrl	*self,
						 SfiProxy	 project);
void		bst_project_ctrl_play		(BstProjectCtrl *self);
void		bst_project_ctrl_stop		(BstProjectCtrl *self);
G_END_DECLS
#endif /* __BST_PROJECT_CTRL_H__ */
