/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
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
#ifndef __BST_PART_DIALOG_H__
#define __BST_PART_DIALOG_H__

#include	"bstpianoroll.h"
#include	"bstpianorollctrl.h"
#include	"bstradiotools.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define BST_TYPE_PART_DIALOG              (bst_part_dialog_get_type ())
#define BST_PART_DIALOG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_PART_DIALOG, BstPartDialog))
#define BST_PART_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_PART_DIALOG, BstPartDialogClass))
#define BST_IS_PART_DIALOG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_PART_DIALOG))
#define BST_IS_PART_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_PART_DIALOG))
#define BST_PART_DIALOG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_PART_DIALOG, BstPartDialogClass))


/* --- structures & typedefs --- */
typedef	struct	_BstPartDialog		BstPartDialog;
typedef	struct	_BstPartDialogClass	BstPartDialogClass;
struct _BstPartDialog
{
  GxkDialog	parent_object;

  GtkWidget	*main_vbox;
  GtkWidget	*scrolled_window;
  GtkWidget	*proll;
  BstPianoRollController *proll_ctrl;
  GxkToolbar	*toolbar;
  BstRadioTools	*rtools;
};
struct _BstPartDialogClass
{
  GxkDialogClass parent_class;

  GtkItemFactory *popup_factory;
};


/* --- prototypes --- */
GType		bst_part_dialog_get_type	(void);
void		bst_part_dialog_set_proxy	(BstPartDialog	*self,
						 BswProxy	 part);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PART_DIALOG_H__ */
