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
#ifndef __BST_DIALOG_H__
#define __BST_DIALOG_H__


#include        "bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BST_TYPE_DIALOG              (bst_dialog_get_type ())
#define BST_DIALOG(object)           (GTK_CHECK_CAST ((object), BST_TYPE_DIALOG, BstDialog))
#define BST_DIALOG_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_DIALOG, BstDialogClass))
#define BST_IS_DIALOG(object)        (GTK_CHECK_TYPE ((object), BST_TYPE_DIALOG))
#define BST_IS_DIALOG_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_DIALOG))
#define BST_DIALOG_GET_CLASS(dialog) (G_TYPE_INSTANCE_GET_CLASS ((dialog), BST_TYPE_DIALOG, BstDialogClass))


/* --- typedefs & enums --- */
typedef struct  _BstDialog       BstDialog;
typedef struct  _BstDialogClass  BstDialogClass;
typedef enum
{
  BST_DIALOG_HIDE_ON_DELETE	= 1 << 0,	/* skips destroy upon delete event */
  BST_DIALOG_STATUS		= 1 << 1,
  BST_DIALOG_MODAL		= 1 << 2,
  BST_DIALOG_POPUP_POS		= 1 << 3,
  BST_DIALOG_DELETE_BUTTON	= 1 << 4	/* auto-add "Close" button */
} BstDialogFlags;


/* --- structures --- */
struct _BstDialog
{
  GtkWindow      window;

  GtkWidget	*vbox;

  BswProxy	 proxy;
  gchar		*title1;
  gchar		*title2;

  /*< private >*/
  GtkObject	*alive_object;	/* dialog is destroyed with this object */
  BstDialogFlags flags;
  gpointer	*pointer_loc;	/* nullified on destroy */
  GtkWidget	*status_bar;
  GtkWidget	*default_widget;
  GtkWidget	*sep;
  GtkWidget	*hbox;
  GtkWidget	*mbox;
};
struct _BstDialogClass
{
  GtkWindowClass        parent_class;
};


/* --- prototypes --- */
GtkType		bst_dialog_get_type		  (void);
gpointer	bst_dialog_new			  (gpointer	   pointer_loc,
						   GtkObject	  *alive_object,
						   BstDialogFlags  flags,
						   const gchar    *title,
						   GtkWidget	  *child);
void		bst_dialog_set_title		  (BstDialog	  *dialog,
						   const gchar	  *title);
void		bst_dialog_set_child		  (BstDialog	  *dialog,
						   GtkWidget	  *child);
GtkWidget*	bst_dialog_get_child		  (BstDialog	  *dialog);
BstDialog*	bst_dialog_get_status_window	  (void);
void		bst_dialog_sync_title_to_proxy	  (BstDialog	  *dialog,
						   BswProxy	   proxy,
						   const gchar	  *title_format);
void		bst_dialog_add_flags		  (BstDialog	  *dialog,
						   BstDialogFlags  flags);
void		bst_dialog_clear_flags		  (BstDialog	  *dialog,
						   BstDialogFlags  flags);
void		bst_dialog_remove_actions	  (BstDialog	  *dialog);
#define		bst_dialog_action(		   dialog, action, callback, data)	\
                                                  bst_dialog_action_multi ((dialog), (action), (callback), (data), 0, 0)
#define		bst_dialog_default_action(	   dialog, action, callback, data)	\
                                                  bst_dialog_action_multi ((dialog), (action), (callback), (data), 0, BST_DIALOG_MULTI_DEFAULT)
#define		bst_dialog_action_swapped(	   dialog, action, callback, data)	\
                                                  bst_dialog_action_multi ((dialog), (action), (callback), (data), 0, BST_DIALOG_MULTI_SWAPPED)
#define		bst_dialog_default_action_swapped( dialog, action, callback, data)	\
                                                  bst_dialog_action_multi ((dialog), (action), (callback), (data), 0, BST_DIALOG_MULTI_DEFAULT | BST_DIALOG_MULTI_SWAPPED)


/* --- internal --- */
typedef enum /*< skip >*/
{
  BST_DIALOG_MULTI_DEFAULT = 1,
  BST_DIALOG_MULTI_SWAPPED = 2
} BstDialogMultiFlags;
GtkWidget*	bst_dialog_action_multi		(BstDialog	    *dialog,
						 const gchar	    *action,
						 gpointer	     callback,
						 gpointer	     data,
						 const gchar	    *icon_stock_id,
						 BstDialogMultiFlags multi_mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif  /* __BST_DIALOG_H__ */
