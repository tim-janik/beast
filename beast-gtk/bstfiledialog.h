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
#ifndef __BST_FILE_DIALOG_H__
#define __BST_FILE_DIALOG_H__


#include        "bstutils.h"
#include        "bstapp.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BST_TYPE_FILE_DIALOG              (bst_file_dialog_get_type ())
#define BST_FILE_DIALOG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_FILE_DIALOG, BstFileDialog))
#define BST_FILE_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_FILE_DIALOG, BstFileDialogClass))
#define BST_IS_FILE_DIALOG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_FILE_DIALOG))
#define BST_IS_FILE_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_FILE_DIALOG))
#define BST_FILE_DIALOG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_FILE_DIALOG, BstFileDialogClass))


/* --- typedefs --- */
typedef struct  _BstFileDialog		BstFileDialog;
typedef struct  _BstFileDialogClass	BstFileDialogClass;


/* --- structures --- */
struct _BstFileDialog
{
  GtkFileSelection file_selection;
};

struct _BstFileDialogClass
{
  GtkFileSelectionClass	parent_class;
};


/* --- prototypes --- */
GtkType		bst_file_dialog_get_type	(void);
GtkWidget*	bst_file_dialog_new_open	(BstApp		*app);
GtkWidget*	bst_file_dialog_new_save	(BstApp		*app);





#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* __BST_FILE_DIALOG_H__ */
