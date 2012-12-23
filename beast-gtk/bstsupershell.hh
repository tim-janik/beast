// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_SUPER_SHELL_H__
#define __BST_SUPER_SHELL_H__
#include	"bstutils.hh"
G_BEGIN_DECLS
/* --- Gtk+ type macros --- */
#define	BST_TYPE_SUPER_SHELL		(bst_super_shell_get_type ())
#define	BST_SUPER_SHELL(object)		(GTK_CHECK_CAST ((object), BST_TYPE_SUPER_SHELL, BstSuperShell))
#define	BST_SUPER_SHELL_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_SUPER_SHELL, BstSuperShellClass))
#define	BST_IS_SUPER_SHELL(object)	(GTK_CHECK_TYPE ((object), BST_TYPE_SUPER_SHELL))
#define	BST_IS_SUPER_SHELL_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_SUPER_SHELL))
#define BST_SUPER_SHELL_GET_CLASS(obj)	(GTK_CHECK_GET_CLASS ((obj), BST_TYPE_SUPER_SHELL, BstSuperShellClass))
/* --- structures & typedefs --- */
typedef	struct	_BstSuperShell		BstSuperShell;
typedef	struct	_BstSuperShellClass	BstSuperShellClass;
struct _BstSuperShell
{
  GtkVBox	 parent_object;
  SfiProxy	 super;
  guint		 name_set_id;
};
struct _BstSuperShellClass
{
  GtkVBoxClass	parent_class;
};
/* --- prototypes --- */
GType		bst_super_shell_get_type	(void);
void		bst_super_shell_set_super	(BstSuperShell	*super_shell,
						 SfiProxy	 super);
GtkWidget*      bst_super_shell_create_label    (BstSuperShell  *super_shell);
G_END_DECLS
#endif /* __BST_SUPER_SHELL_H__ */
