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
#ifndef __BST_TRACK_VIEW_H__
#define __BST_TRACK_VIEW_H__

#include "bstitemview.h"
#include "bsttrackroll.h"
#include "bsttrackrollctrl.h"

G_BEGIN_DECLS

/* --- Gtk+ type macros --- */
#define	BST_TYPE_TRACK_VIEW	       (bst_track_view_get_type ())
#define	BST_TRACK_VIEW(object)	       (GTK_CHECK_CAST ((object), BST_TYPE_TRACK_VIEW, BstTrackView))
#define	BST_TRACK_VIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_TRACK_VIEW, BstTrackViewClass))
#define	BST_IS_TRACK_VIEW(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_TRACK_VIEW))
#define	BST_IS_TRACK_VIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_TRACK_VIEW))
#define BST_TRACK_VIEW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_TRACK_VIEW, BstTrackViewClass))


/* --- structures & typedefs --- */
typedef	struct	_BstTrackView	   BstTrackView;
typedef	struct	_BstTrackViewClass BstTrackViewClass;
struct _BstTrackView
{
  BstItemView	          parent_object;
  BstTrackRoll	         *troll;
  BstTrackRollController *tctrl;
  GtkWidget		 *repeat_toggle;
};
struct _BstTrackViewClass
{
  BstItemViewClass parent_class;
};


/* --- prototypes --- */
GType		bst_track_view_get_type	(void);
GtkWidget*	bst_track_view_new	(SfiProxy	song);



G_END_DECLS

#endif /* __BST_TRACK_VIEW_H__ */
