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
#ifndef __BST_INSTRUMENT_VIEW_H__
#define __BST_INSTRUMENT_VIEW_H__

#include	"bstitemview.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_INSTRUMENT_VIEW	    (bst_instrument_view_get_type ())
#define	BST_INSTRUMENT_VIEW(object)	    (GTK_CHECK_CAST ((object), BST_TYPE_INSTRUMENT_VIEW, BstInstrumentView))
#define	BST_INSTRUMENT_VIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_INSTRUMENT_VIEW, BstInstrumentViewClass))
#define	BST_IS_INSTRUMENT_VIEW(object)	    (GTK_CHECK_TYPE ((object), BST_TYPE_INSTRUMENT_VIEW))
#define	BST_IS_INSTRUMENT_VIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_INSTRUMENT_VIEW))
#define BST_INSTRUMENT_VIEW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_INSTRUMENT_VIEW, BstInstrumentViewClass))


/* --- structures & typedefs --- */
typedef	struct	_BstInstrumentView	BstInstrumentView;
typedef	struct	_BstInstrumentViewClass	BstInstrumentViewClass;
struct _BstInstrumentView
{
  BstItemView	 parent_object;
};
struct _BstInstrumentViewClass
{
  BstItemViewClass parent_class;
};


/* --- prototypes --- */
GtkType		bst_instrument_view_get_type	(void);
GtkWidget*	bst_instrument_view_new		(BseSong	*song);



#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_INSTRUMENT_VIEW_H__ */
