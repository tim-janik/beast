/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2003 Tim Janik
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
#ifndef __BST_PART_DIALOG_H__
#define __BST_PART_DIALOG_H__

#include	"bstpianoroll.h"
#include	"bsteventroll.h"
#include	"bstpatternview.h"
#include	"bstpatternctrl.h"
#include	"bstpianorollctrl.h"
#include	"bsteventrollctrl.h"

G_BEGIN_DECLS

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
  GxkDialog	          parent_object;

  BstPianoRoll           *proll;
  BstPianoRollController *pctrl;
  BstEventRoll           *eroll;
  BstEventRollController *ectrl;
  BstPatternView         *pview;
  BstPatternController   *pvctrl;
  SfiProxy                project;
};
struct _BstPartDialogClass
{
  GxkDialogClass parent_class;
};


/* --- prototypes --- */
GType		bst_part_dialog_get_type	(void);
void		bst_part_dialog_set_proxy	(BstPartDialog	*self,
						 SfiProxy	 part);

G_END_DECLS

#endif /* __BST_PART_DIALOG_H__ */
