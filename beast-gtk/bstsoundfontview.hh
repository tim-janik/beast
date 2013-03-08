// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_SOUND_FONT_VIEW_HH__
#define __BST_SOUND_FONT_VIEW_HH__

#include "bstitemview.hh"
#include "bstsoundfontpresetview.hh"

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

G_END_DECLS

#endif /* __BST_SOUND_FONT_VIEW_HH__ */
