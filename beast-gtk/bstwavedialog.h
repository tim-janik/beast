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
#ifndef __BST_WAVE_DIALOG_H__
#define __BST_WAVE_DIALOG_H__

#include        "bstdefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BST_TYPE_WAVE_DIALOG		  (bst_wave_dialog_get_type ())
#define BST_WAVE_DIALOG(object)		  (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_WAVE_DIALOG, BstWaveDialog))
#define BST_WAVE_DIALOG_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_WAVE_DIALOG, BstWaveDialogClass))
#define BST_IS_WAVE_DIALOG(object)	  (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_WAVE_DIALOG))
#define BST_IS_WAVE_DIALOG_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_WAVE_DIALOG))
#define BST_WAVE_DIALOG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_WAVE_DIALOG, BstWaveDialogClass))


/* --- typedefs --- */
typedef struct  _BstWaveDialog		BstWaveDialog;
typedef struct  _BstWaveDialogClass	BstWaveDialogClass;


/* --- structures --- */
struct _BstWaveDialog
{
  GtkFileSelection wave_selection;

  GtkWidget	*parent_dialog;
  BswProxy	 wave_repo;
};

struct _BstWaveDialogClass
{
  GtkFileSelectionClass	parent_class;
};


/* --- prototypes --- */
GType		bst_wave_dialog_get_type	(void);
GtkWidget*	bst_wave_dialog_new_load	(BswProxy	 wave_repo,
						 GtkWidget	*parent_dialog);
void		bst_wave_dialog_set_wave_repo	(BstWaveDialog	*wave_dialog,
						 BswProxy	 wave_repo);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* __BST_WAVE_DIALOG_H__ */
