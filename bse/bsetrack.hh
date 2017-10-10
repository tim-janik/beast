// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_TRACK_H__
#define __BSE_TRACK_H__

#include <bse/bseitem.hh>
#include <bse/bsesnet.hh>
#include <bse/bsecontextmerger.hh>

/* --- BSE type macros --- */
#define BSE_TYPE_TRACK		    (BSE_TYPE_ID (BseTrack))
#define BSE_TRACK(object)	    (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_TRACK, BseTrack))
#define BSE_TRACK_CLASS(class)	    (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_TRACK, BseTrackClass))
#define BSE_IS_TRACK(object)	    (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_TRACK))
#define BSE_IS_TRACK_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_TRACK))
#define BSE_TRACK_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_TRACK, BseTrackClass))

struct BseTrackEntry {
  guint    tick;
  guint    id;
  BsePart *part;
};
struct BseTrack : BseContextMerger {
  guint            channel_id;
  guint		   max_voices;
  BseSNet	  *snet;
  BseSNet         *pnet;
  /* wave synthesis */
  BseWave	  *wave;
  BseSNet	  *wnet;

  /* sound font synthesis */
  BseSoundFontPreset  *sound_font_preset;
  BseSNet         *sound_font_net;

  /* playback intergration */
  BseSource       *sub_synth;
  BseSource       *voice_input;
  BseSource       *voice_switch;
  BseSource       *postprocess;
  SfiRing         *bus_outputs; /* maintained by bsebus.[hc] */
  /* fields protected by sequencer mutex */
  guint		   n_entries_SL : 30;
  guint		   muted_SL : 1;
  BseTrackEntry	  *entries_SL;
  guint            midi_channel_SL;
  gboolean	   track_done_SL;
};
struct BseTrackClass : BseContextMergerClass
{};

/* --- prototypes -- */
void	bse_track_add_modules		(BseTrack		*self,
					 BseContainer		*container,
                                         BseMidiReceiver        *midi_receiver);
void	bse_track_remove_modules	(BseTrack		*self,
					 BseContainer		*container);
void	bse_track_clone_voices		(BseTrack		*self,
					 BseSNet		*snet,
					 guint			 context,
                                         BseMidiContext          mcontext,
					 BseTrans		*trans);
BseSource*       bse_track_get_output   (BseTrack               *self);
guint        	 bse_track_get_last_tick(BseTrack		*self);
guint        	 bse_track_insert_part	(BseTrack		*self,
					 guint			 tick,
					 BsePart		*part);
void		 bse_track_remove_tick	(BseTrack		*self,
					 guint			 tick);
Bse::TrackPartSeq bse_track_list_parts	(BseTrack		*self);
Bse::TrackPartSeq bse_track_list_part	(BseTrack		*self,
                                         BsePart                *part);
gboolean	 bse_track_find_part	(BseTrack		*self,
					 BsePart		*part,
					 guint			*start_p);
BseTrackEntry*	 bse_track_lookup_tick	(BseTrack		*self,
					 guint			 tick);
BseTrackEntry*   bse_track_find_link    (BseTrack               *self,
                                         guint                   id);
BsePart*	 bse_track_get_part_SL	(BseTrack		*self,
					 guint			 tick,
					 guint			*start,
					 guint			*next);
namespace Bse {

class TrackImpl : public ContextMergerImpl, public virtual TrackIface {
protected:
  virtual             ~TrackImpl         ();
public:
  explicit             TrackImpl         (BseObject*);
  virtual SongTiming   get_timing        (int tick) override;
  virtual int          insert_part       (int tick, PartIface &part) override;
  virtual void         remove_tick       (int tick) override;
  virtual void         remove_link       (int id) override;
  virtual PartSeq      list_parts_uniq   () override;
  virtual TrackPartSeq list_parts        () override;
  virtual PartIfaceP   get_part          (int tick) override;
  virtual int          get_last_tick     () override;
  virtual Error        ensure_output     () override;
  virtual SourceIfaceP get_output_source () override;
  virtual ItemSeq      outputs           () const override;
  virtual void         outputs           (const ItemSeq &newoutputs) override;
};

} // Bse

#endif /* __BSE_TRACK_H__ */
