/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999, 2000, 2001 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BST_FREE_RADIO_BUTTON_H__
#define __BST_FREE_RADIO_BUTTON_H__


#include <gtk/gtkradiobutton.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define BST_TYPE_FREE_RADIO_BUTTON	      (bst_free_radio_button_get_type ())
#define BST_FREE_RADIO_BUTTON(obj)	      (GTK_CHECK_CAST ((obj), BST_TYPE_FREE_RADIO_BUTTON, BstFreeRadioButton))
#define BST_FREE_RADIO_BUTTON_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_FREE_RADIO_BUTTON, BstFreeRadioButtonClass))
#define BST_IS_FREE_RADIO_BUTTON(obj)	      (GTK_CHECK_TYPE ((obj), BST_TYPE_FREE_RADIO_BUTTON))
#define BST_IS_FREE_RADIO_BUTTON_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_FREE_RADIO_BUTTON))
#define BST_FREE_RADIO_BUTTON_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_FREE_RADIO_BUTTON, BstFreeRadioButtonClass))


typedef GtkRadioButton      BstFreeRadioButton;
typedef GtkRadioButtonClass BstFreeRadioButtonClass;


GtkType	   bst_free_radio_button_get_type (void);
GtkWidget* bst_free_radio_button_new	  (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __BST_FREE_RADIO_BUTTON_H__ */
