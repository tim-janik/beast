/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2003 Tim Janik
 * Copyright (C) 2009 Stefan Westerfeld
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

#ifndef __BST_SOUND_FONT_VIEW_H__
#define __BST_SOUND_FONT_VIEW_H__

#include "bstitemview.h"
#include "bstsoundfontpresetview.h"

G_BEGIN_DECLS

/* --- Gtk+ type macros --- */
#define	BST_TYPE_SOUND_FONT_VIEW	      (bst_sound_font_view_get_type ())
#define	BST_SOUND_FONT_VIEW(object)	      (GTK_CHECK_CAST ((object), BST_TYPE_SOUND_FONT_VIEW, BstSoundFontView))
#define	BST_SOUND_FONT_VIEW_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_SOUND_FONT_VIEW, BstSoundFontViewClass))
#define	BST_IS_SOUND_FONT_VIEW(object)	      (GTK_CHECK_TYPE ((object), BST_TYPE_SOUND_FONT_VIEW))
#define	BST_IS_SOUND_FONT_VIEW_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_SOUND_FONT_VIEW))
#define BST_SOUND_FONT_VIEW_GET_CLASS(obj)    (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_SOUND_FONT_VIEW, BstSoundFontViewClass))


typedef	struct	_BstSoundFontView	      BstSoundFontView;
typedef	struct	_BstSoundFontViewClass	      BstSoundFontViewClass;
struct _BstSoundFontView
{
  BstItemView		    parent_object;
  BstSoundFontPresetView   *preset_view;
};

struct _BstSoundFontViewClass
{
  BstItemViewClass parent_class;
};

/* --- prototypes --- */

GType		bst_sound_font_view_get_type          (void);
GtkWidget*	bst_sound_font_view_new               (SfiProxy		   sfont_repo);
SfiProxy	bst_sound_font_view_get_preset	      (BstSoundFontView	  *self);

#endif /* __BST_SOUND_FONT_VIEW_H__ */
