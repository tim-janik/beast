/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#ifndef __BST_SUPER_SHELL_H__
#define __BST_SUPER_SHELL_H__

#include	"bstutils.h"

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
  GtkVBox	parent_object;

  GtkAccelGroup	*accel_group;

  SfiProxy	 super;
  guint		 name_set_id;
};
struct _BstSuperShellClass
{
  GtkVBoxClass	parent_class;
  
  gchar*        (*get_title)     (BstSuperShell  *super_shell);
  void		(*setup_super)	 (BstSuperShell  *super_shell,
				  SfiProxy	  super);
  void		(*release_super) (BstSuperShell  *super_shell,
				  SfiProxy	  super);
  void		(*operate)	 (BstSuperShell  *super_shell,
				  BstOps          op);
  gboolean	(*can_operate)	 (BstSuperShell  *super_shell,
				  BstOps          op);
  void		(*rebuild)	 (BstSuperShell  *super_shell);
};


/* --- prototypes --- */
GType		bst_super_shell_get_type	(void);
void		bst_super_shell_operate		(BstSuperShell	*super_shell,
						 BstOps		 op);
gboolean	bst_super_shell_can_operate	(BstSuperShell	*super_shell,
						 BstOps		 op);
void		bst_super_shell_set_super	(BstSuperShell	*super_shell,
						 SfiProxy	 super);
void            bst_super_shell_update_label    (BstSuperShell  *super_shell);

G_END_DECLS

#endif /* __BST_SUPER_SHELL_H__ */
