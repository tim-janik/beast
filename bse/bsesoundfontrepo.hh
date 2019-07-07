// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef	__BSE_SOUND_FONT_REPO_HH__
#define	__BSE_SOUND_FONT_REPO_HH__

#include	<bse/bsesuper.hh>
#include        <fluidsynth.h>
#include        <bse/bsesoundfontosc.hh>
#include        <bse/bseengine.hh>

#define BSE_TYPE_SOUND_FONT_REPO	        (BSE_TYPE_ID (BseSoundFontRepo))
#define BSE_SOUND_FONT_REPO(object)	        (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SOUND_FONT_REPO, BseSoundFontRepo))
#define BSE_SOUND_FONT_REPO_CLASS(class)	(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SOUND_FONT_REPO, BseSoundFontRepoClass))
#define BSE_IS_SOUND_FONT_REPO(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SOUND_FONT_REPO))
#define BSE_IS_SOUND_FONT_REPO_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SOUND_FONT_REPO))
#define BSE_SOUND_FONT_REPO_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SOUND_FONT_REPO, BseSoundFontRepoClass))

#define BSE_FLUID_SYNTH_PROGRAM_SELECT -1

struct BseFluidEvent {
  guint64            tick_stamp;
  guint              channel;
  int		     command;
  int		     arg1;
  int		     arg2;
  int                sfont_id;	  /* required for program selection only */
};

struct BseSoundFontRepo : BseSuper {
};

struct BseSoundFontRepoClass : BseSuperClass
{};


/* --- prototypes --- */
void	       bse_sound_font_repo_list_all_presets   (BseSoundFontRepo *sfrepo,
						       Bse::ItemSeq     &items);
fluid_synth_t* bse_sound_font_repo_fluid_synth        (BseSoundFontRepo *sfrepo);

namespace Bse {

class SoundFontRepoImpl : public SuperImpl, public virtual SoundFontRepoIface {
public:
  std::vector<BseSoundFont *> sound_fonts;

  fluid_settings_t           *fluid_settings;
  fluid_synth_t              *fluid_synth;

protected:
  virtual  ~SoundFontRepoImpl ();

public:
  explicit      SoundFontRepoImpl (BseObject*);
  virtual Error load_file         (const String &file_name) override;
  virtual Error remove_sound_font (SoundFontIface &wave) override;
};

} // Bse


#endif /* __BSE_SOUND_FONT_REPO_HH__ */
