/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik
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

#include	"bstdialog.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_PATTERN_DIALOG		   (bst_pattern_dialog_get_type ())
#define	BST_PATTERN_DIALOG(object)	   (GTK_CHECK_CAST ((object), BST_TYPE_PATTERN_DIALOG, BstPatternDialog))
#define	BST_PATTERN_DIALOG_CLASS(klass)	   (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_PATTERN_DIALOG, BstPatternDialogClass))
#define	BST_IS_PATTERN_DIALOG(object)	   (GTK_CHECK_TYPE ((object), BST_TYPE_PATTERN_DIALOG))
#define	BST_IS_PATTERN_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_PATTERN_DIALOG))
#define BST_PATTERN_DIALOG_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_PATTERN_DIALOG, BstPatternDialogClass))


/* --- structures & typedefs --- */
typedef	struct	_BstPatternDialog	BstPatternDialog;
typedef	struct	_BstPatternDialogClass	BstPatternDialogClass;
struct _BstPatternDialog
{
  BstDialog	parent_object;

  GtkWidget	*main_vbox;
  GtkWidget	*scrolled_window;
  GtkWidget	*pattern_editor;
  GtkWidget	*effect_view;
};
struct _BstPatternDialogClass
{
  BstDialogClass parent_class;

  GtkItemFactory *popup_factory;
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
void   bst_pattern_dialog_gtkfix_default_accels (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PATTERN_DIALOG_H__ */
