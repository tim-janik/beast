/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#ifndef __BST_PATTERN_DIALOG_H__
#define __BST_PATTERN_DIALOG_H__

#include	"bstpatterneditor.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_PATTERN_DIALOG		 (bst_pattern_dialog_get_type ())
#define	BST_PATTERN_DIALOG(object)	 (GTK_CHECK_CAST ((object), BST_TYPE_PATTERN_DIALOG, BstPatternDialog))
#define	BST_PATTERN_DIALOG_CLASS(klass)	 (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_PATTERN_DIALOG, BstPatternDialogClass))
#define	BST_IS_PATTERN_DIALOG(object)	 (GTK_CHECK_TYPE ((object), BST_TYPE_PATTERN_DIALOG))
#define	BST_IS_PATTERN_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_PATTERN_DIALOG))
#define BST_PATTERN_DIALOG_GET_CLASS(obj)	 ((BstPatternDialogClass*) (((GtkObject*) (obj))->klass))


/* --- structures & typedefs --- */
typedef	struct	_BstPatternDialog	BstPatternDialog;
typedef	struct	_BstPatternDialogClass	BstPatternDialogClass;
struct _BstPatternDialog
{
  GtkWindow	parent_object;

  GtkWidget	*main_vbox;
  GtkWidget	*scrolled_window;
  GtkWidget	*pattern_editor;

  GtkWidget	*popup;

  GtkWidget	*proc_dialog;
};
struct _BstPatternDialogClass
{
  GtkWindowClass parent_class;

  GSList *popup_entries; /* of type BstMenuEntry* */
};

typedef enum
{
  BST_PATTERN_OP_NONE,
  BST_PATTERN_OP_HUHU,
  BST_PATTERN_OP_LAST
} BstPatternOps;


/* --- prototypes --- */
GtkType		bst_pattern_dialog_get_type	(void);
GtkWidget*	bst_pattern_dialog_new		(BsePattern	  *pattern);
void		bst_pattern_dialog_operate	(BstPatternDialog *pattern_dialog,
						 BstPatternOps	   op);
gboolean	bst_pattern_dialog_can_operate	(BstPatternDialog *pattern_dialog,
						 BstPatternOps	   op);
void		bst_pattern_dialog_update	(BstPatternDialog *pattern_dialog);
void   bst_pattern_dialog_gtkfix_default_accels (void);


#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_PATTERN_DIALOG_H__ */
