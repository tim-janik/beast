/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2003 Tim Janik
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
#ifndef __BST_SONG_BUS_VIEW_H__
#define __BST_SONG_BUS_VIEW_H__

#include	"bstitemview.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_SONG_BUS_VIEW              (bst_song_bus_view_get_type ())
#define BST_SONG_BUS_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_SONG_BUS_VIEW, BstSongBusView))
#define BST_SONG_BUS_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_SONG_BUS_VIEW, BstSongBusViewClass))
#define BST_IS_SONG_BUS_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_SONG_BUS_VIEW))
#define BST_IS_SONG_BUS_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_SONG_BUS_VIEW))
#define BST_SONG_BUS_VIEW_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_SONG_BUS_VIEW, BstSongBusViewClass))


/* --- structures & typedefs --- */
typedef	struct	_BstSongBusView		BstSongBusView;
typedef	struct	_BstSongBusViewClass	BstSongBusViewClass;
struct _BstSongBusView
{
  BstItemView	 parent_object;
};
struct _BstSongBusViewClass
{
  BstItemViewClass parent_class;
};


/* --- prototypes --- */
GType		bst_song_bus_view_get_type  (void);
GtkWidget*      bst_song_bus_view_new       (SfiProxy song);

G_END_DECLS

#endif /* __BST_SONG_BUS_VIEW_H__ */
