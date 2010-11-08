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

#ifndef __BST_SOUND_FONT_PRESET_VIEW_H__
#define __BST_SOUND_FONT_PRESET_VIEW_H__

#include "bstitemview.h"

G_BEGIN_DECLS

/* --- Gtk+ type macros --- */
#define	BST_TYPE_SOUND_FONT_PRESET_VIEW		     (bst_sound_font_preset_view_get_type ())
#define	BST_SOUND_FONT_PRESET_VIEW(object)	     (GTK_CHECK_CAST ((object), BST_TYPE_SOUND_FONT_PRESET_VIEW, BstSoundFontPresetView))
#define	BST_SOUND_FONT_PRESET_VIEW_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_SOUND_FONT_PRESET_VIEW, BstSoundFontPresetViewClass))
#define	BST_IS_SOUND_FONT_PRESET_VIEW(object)	     (GTK_CHECK_TYPE ((object), BST_TYPE_SOUND_FONT_PRESET_VIEW))
#define	BST_IS_SOUND_FONT_PRESET_VIEW_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_SOUND_FONT_PRESET_VIEW))
#define BST_SOUND_FONT_PRESET_VIEW_GET_CLASS(obj)    (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_SOUND_FONT_PRESET_VIEW, BstSoundFontPresetViewClass))


typedef	struct	_BstSoundFontPresetView	      BstSoundFontPresetView;
typedef	struct	_BstSoundFontPresetViewClass  BstSoundFontPresetViewClass;
struct _BstSoundFontPresetView
{
  BstItemView	 parent_object;
};

struct _BstSoundFontPresetViewClass
{
  BstItemViewClass parent_class;
};

/* --- prototypes --- */

GType		bst_sound_font_preset_view_get_type          (void);
GtkWidget*	bst_sound_font_preset_view_new               (void);

#endif /* __BST_SOUND_FONT_PRESET_VIEW_H__ */
