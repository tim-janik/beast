/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_PROCEDURE_H__
#define __BST_PROCEDURE_H__

#include	"bstparamview.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_PROCEDURE_DIALOG	     (bst_procedure_dialog_get_type ())
#define	BST_PROCEDURE_DIALOG(object)	     (GTK_CHECK_CAST ((object), BST_TYPE_PROCEDURE_DIALOG, BstProcedureDialog))
#define	BST_PROCEDURE_DIALOG_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_PROCEDURE_DIALOG, BstProcedureDialogClass))
#define	BST_IS_PROCEDURE_DIALOG(object)	     (GTK_CHECK_TYPE ((object), BST_TYPE_PROCEDURE_DIALOG))
#define	BST_IS_PROCEDURE_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_PROCEDURE_DIALOG))
#define BST_PROCEDURE_DIALOG_GET_CLASS(obj)  ((BstProcedureDialogClass*) (((GtkObject*) (obj))->klass))


/* --- structures & typedefs --- */
typedef	struct	_BstProcedureDialog		BstProcedureDialog;
typedef	struct	_BstProcedureDialogClass	BstProcedureDialogClass;
struct _BstProcedureDialog
{
  GtkWindow	     parent_object;

  BseProcedureClass *proc;

  guint		     n_in_params;
  guint		     n_out_params;
  GSList	    *bparams; /* n_in_params + n_out_params bparams */
  GSList	    *_params; /* first out param */

  GtkTooltips	    *tooltips;
};
struct _BstProcedureDialogClass
{
  GtkWindowClass     parent_class;
};


/* --- prototypes --- */
GtkType		bst_procedure_dialog_get_type	(void);
GtkWidget*	bst_procedure_dialog_new	(BseProcedureClass  *proc);
void		bst_procedure_dialog_update	(BstProcedureDialog *procedure_dialog);
void		bst_procedure_dialog_rebuild	(BstProcedureDialog *procedure_dialog);
void		bst_procedure_dialog_execute	(BstProcedureDialog *procedure_dialog);
void		bst_procedure_dialog_set_proc	(BstProcedureDialog *procedure_dialog,
						 BseProcedureClass  *proc);
guint		bst_procedure_dialog_preset	(BstProcedureDialog *procedure_dialog,
						 gboolean	     lock_presets,
						 GSList		    *preset_params);
GtkWidget*	bst_procedure_dialog_get_global	(void);


/* --- BST procedure stuff --- */
void		bst_procedure_void_execpl	(BseProcedureClass  *proc,
						 GSList		    *preset_params);





#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_PARAM_VIEW_H__ */
