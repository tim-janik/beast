/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999 Tim Janik
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
#ifndef __BST_FREE_RADIO_BUTTON_H__
#define __BST_FREE_RADIO_BUTTON_H__


#include <gtk/gtkradiobutton.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define BST_TYPE_FREE_RADIO_BUTTON	      (bst_free_radio_button_get_type ())
#define BST_FREE_RADIO_BUTTON(obj)	      (BST_CHECK_CAST ((obj), BST_TYPE_FREE_RADIO_BUTTON, BstFreeRadioButton))
#define BST_FREE_RADIO_BUTTON_CLASS(klass)    (BST_CHECK_CLASS_CAST ((klass), BST_TYPE_FREE_RADIO_BUTTON, BstFreeRadioButtonClass))
#define BST_IS_FREE_RADIO_BUTTON(obj)	      (BST_CHECK_TYPE ((obj), BST_TYPE_FREE_RADIO_BUTTON))
#define BST_IS_FREE_RADIO_BUTTON_CLASS(klass) (BST_CHECK_CLASS_TYPE ((klass), BST_TYPE_FREE_RADIO_BUTTON))
#define BST_FREE_RADIO_BUTTON_GET_CLASS(obj)  ((BstFreeRadioButtonClass*) (((GtkObject*) (obj))->klass))


typedef GtkRadioButton      BstFreeRadioButton;
typedef GtkRadioButtonClass BstFreeRadioButtonClass;


GtkType	   bst_free_radio_button_get_type (void);
GtkWidget* bst_free_radio_button_new	  (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __BST_FREE_RADIO_BUTTON_H__ */
