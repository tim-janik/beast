/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
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
#ifndef __BST_MARKER_H__
#define __BST_MARKER_H__

#include        "bstutils.h"

G_BEGIN_DECLS

typedef enum
{
  BST_MARKER_NONE	= 0,
  BST_MARKER_RED,
  BST_MARKER_GREEN,
  BST_MARKER_BLUE
} BstMarkerType;
typedef struct
{
  guint		index;
  BstMarkerType type;
  gint          pixoffset;
  /* user data */
  glong		position;
  gpointer	user_data;
} BstMarker;
typedef struct
{
  guint	       n_marks : 30;
  guint	       vmarks : 1;
  BstMarker   *marks;
  GdkDrawable *drawable;
  GdkPixmap   *pixmap;
  GdkColor     red, green, blue, light, dark;
  GdkGC	      *red_gc, *green_gc, *blue_gc, *light_gc, *dark_gc;
} BstMarkerSetup;

void	    bst_marker_init_vertical	(BstMarkerSetup	*self);
void	    bst_marker_finalize		(BstMarkerSetup	*self);
void	    bst_marker_realize		(BstMarkerSetup	*self,
					 GdkDrawable	*drawable);
void	    bst_marker_resize		(BstMarkerSetup	*self);
void	    bst_marker_unrealize	(BstMarkerSetup	*self);
BstMarker*  bst_marker_add		(BstMarkerSetup	*self,
					 guint		 index);
BstMarker*  bst_marker_get		(BstMarkerSetup	*self,
					 guint		 index);
void	    bst_marker_delete		(BstMarkerSetup	*self,
					 BstMarker	*marker);
void	    bst_marker_set		(BstMarkerSetup	*self,
					 BstMarker	*mark,
					 BstMarkerType	 type,
					 gint		 pixoffset);
void	    bst_marker_save_backing	(BstMarkerSetup	*self,
					 GdkRectangle	*area);
void	    bst_marker_expose		(BstMarkerSetup	*self,
					 GdkRectangle	*area);
void	    bst_marker_scroll		(BstMarkerSetup	*self,
					 gint		 xdiff,
					 gint		 ydiff);
GdkGC*	    bst_marker_get_gc		(BstMarkerSetup	*self,
					 BstMarker	*marker);

G_END_DECLS

#endif /* __BST_MARKER_H__ */
