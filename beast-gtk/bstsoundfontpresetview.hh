// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_SOUND_FONT_PRESET_VIEW_HH__
#define __BST_SOUND_FONT_PRESET_VIEW_HH__

#include "bstitemview.hh"

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

G_END_DECLS
#endif /* __BST_SOUND_FONT_PRESET_VIEW_HH__ */
