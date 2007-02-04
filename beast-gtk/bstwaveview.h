/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
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
#ifndef __BST_WAVE_VIEW_H__
#define __BST_WAVE_VIEW_H__

#include	"bstitemview.h"

G_BEGIN_DECLS


/* --- Gtk+ type macros --- */
#define	BST_TYPE_WAVE_VIEW	      (bst_wave_view_get_type ())
#define	BST_WAVE_VIEW(object)	      (GTK_CHECK_CAST ((object), BST_TYPE_WAVE_VIEW, BstWaveView))
#define	BST_WAVE_VIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_WAVE_VIEW, BstWaveViewClass))
#define	BST_IS_WAVE_VIEW(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_WAVE_VIEW))
#define	BST_IS_WAVE_VIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_WAVE_VIEW))
#define BST_WAVE_VIEW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_WAVE_VIEW, BstWaveViewClass))


/* --- structures & typedefs --- */
typedef	struct	_BstWaveView		BstWaveView;
typedef	struct	_BstWaveViewClass	BstWaveViewClass;
struct _BstWaveView
{
  BstItemView	 parent_object;
  guint          editable : 1;
};
struct _BstWaveViewClass
{
  BstItemViewClass parent_class;
};


/* --- prototypes --- */
GType		bst_wave_view_get_type          (void);
GtkWidget*	bst_wave_view_new               (SfiProxy     wrepo);
void            bst_wave_view_set_editable      (BstWaveView *self,
                                                 gboolean     enabled);

G_END_DECLS

#endif /* __BST_WAVE_VIEW_H__ */
