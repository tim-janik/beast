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
#ifndef __BST_TRACK_VIEW_H__
#define __BST_TRACK_VIEW_H__

#include	"bstitemview.h"
#include	"bsttrackroll.h"
#include	"bstradiotools.h"
#include	"bsttrackrollctrl.h"

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
  BstTrackRollController *troll_ctrl;
  BstRadioTools		 *canvas_rtools;
  BstRadioTools		 *hpanel_rtools;
  BstRadioTools		 *quant_rtools;
  GxkToolbar		 *toolbar;
  GtkWidget		 *repeat_toggle;
};
struct _BstTrackViewClass
{
  BstItemViewClass parent_class;
};


/* --- prototypes --- */
GtkType		bst_track_view_get_type	(void);
GtkWidget*	bst_track_view_new	(SfiProxy	song);



G_END_DECLS

#endif /* __BST_TRACK_VIEW_H__ */
