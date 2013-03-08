// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstsoundfontpresetview.hh"

/* --- functions --- */

G_DEFINE_TYPE (BstSoundFontPresetView, bst_sound_font_preset_view, BST_TYPE_ITEM_VIEW);


static void
bst_sound_font_preset_view_class_init (BstSoundFontPresetViewClass *klass)
{
  BstItemViewClass *item_view_class = BST_ITEM_VIEW_CLASS (klass);

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
