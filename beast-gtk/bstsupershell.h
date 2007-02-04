/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
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
