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
#include "bstsoundfontpresetview.h"

/* --- functions --- */

G_DEFINE_TYPE (BstSoundFontPresetView, bst_sound_font_preset_view, BST_TYPE_ITEM_VIEW);


static void
bst_sound_font_preset_view_class_init (BstSoundFontPresetViewClass *class)
{
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (class);

  item_view_class->item_type = "BseSoundFontPreset";
}

static void
bst_sound_font_preset_view_init (BstSoundFontPresetView *self)
{
  BstItemView *iview = BST_ITEM_VIEW (self);
}

GtkWidget*
bst_sound_font_preset_view_new()
{
  GtkWidget *sound_font_preset_view;

  sound_font_preset_view = gtk_widget_new (BST_TYPE_SOUND_FONT_PRESET_VIEW, NULL);

  return sound_font_preset_view;
}
