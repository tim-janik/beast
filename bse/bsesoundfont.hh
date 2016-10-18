// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SOUND_FONT_HH__
#define __BSE_SOUND_FONT_HH__

#include	<bse/bsecontainer.hh>
#include        <bse/bsestorage.hh>

G_BEGIN_DECLS

/* --- BSE type macros --- */
#define BSE_TYPE_SOUND_FONT		  (BSE_TYPE_ID (BseSoundFont))
#define BSE_SOUND_FONT(object)		  (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SOUND_FONT, BseSoundFont))
#define BSE_SOUND_FONT_CLASS(class)	  (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SOUND_FONT, BseSoundFontClass))
#define BSE_IS_SOUND_FONT(object)	  (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SOUND_FONT))
#define BSE_IS_SOUND_FONT_CLASS(class)	  (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SOUND_FONT))
#define BSE_SOUND_FONT_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SOUND_FONT, BseSoundFontClass))

struct BseSoundFont : BseContainer {
  BseStorageBlob    *blob;
  int                sfont_id;
  BseSoundFontRepo  *sfrepo;
  GList             *presets;
};
struct BseSoundFontClass : BseContainerClass
{};

Bse::Error      bse_sound_font_load_blob	(BseSoundFont       *sound_font,
						 BseStorageBlob     *blob,
						 gboolean            init_presets);
void		bse_sound_font_unload           (BseSoundFont       *sound_font);
Bse::Error      bse_sound_font_reload           (BseSoundFont       *sound_font);

G_END_DECLS

namespace Bse {

class SoundFontImpl : public ContainerImpl, public virtual SoundFontIface {
protected:
  virtual  ~SoundFontImpl ();
public:
  explicit  SoundFontImpl (BseObject*);
};

} // Bse


#endif /* __BSE_SOUND_FONT_HH__ */
