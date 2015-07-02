// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SONG_H__
#define __BSE_SONG_H__

#include        <bse/bsesnet.hh>


G_BEGIN_DECLS


/* --- BSE type macros --- */
#define BSE_TYPE_SONG              (BSE_TYPE_ID (BseSong))
#define BSE_SONG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SONG, BseSong))
#define BSE_SONG_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SONG, BseSongClass))
#define BSE_IS_SONG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SONG))
#define BSE_IS_SONG_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SONG))
#define BSE_SONG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SONG, BseSongClass))


/* --- BseSong object --- */
struct BseSongVoice {
  BseSource *constant;
  BseSource *sub_synth;
};
struct BseSong : BseSNet {
  guint		    tpqn;		/* ticks per querter note */
  guint		    numerator;
  guint		    denominator;
  gfloat            bpm;
  BseMusicalTuningType musical_tuning;
  SfiRing          *parts;              /* of type BsePart* */
  SfiRing          *busses;             /* of type BseBus* */
  BseBus           *solo_bus;
  BseSource	   *postprocess;
  BseSource	   *output;
  BseSNet          *pnet;
  /* song position pointer */
  SfiInt	    last_position;
  guint		    position_handler;
  BseMidiReceiver  *midi_receiver_SL;
  /* fields protected by sequencer mutex */
  gdouble	    tpsi_SL;		/* ticks per stamp increment (sample) */
  SfiRing	   *tracks_SL;		/* of type BseTrack* */
  /* sequencer stuff */
  guint64           sequencer_start_request_SL;
  guint64           sequencer_start_SL; /* playback start */
  guint64           sequencer_done_SL;
  gdouble	    delta_stamp_SL;	/* start + delta_stamp => tick */
  guint		    tick_SL;		/* tick at stamp_SL */
  guint             sequencer_owns_refcount_SL : 1;
  guint             sequencer_underrun_detected_SL : 1;
  guint		    loop_enabled_SL : 1;
  SfiInt	    loop_left_SL;	/* left loop tick */
  SfiInt	    loop_right_SL;	/* left loop tick */
};
struct BseSongClass : BseSNetClass
{};

BseSong*	bse_song_lookup			(BseProject	*project,
						 const gchar	*name);
void		bse_song_stop_sequencing_SL	(BseSong	*self);
void		bse_song_get_timing		(BseSong	*self,
						 guint		 tick,
						 BseSongTiming	*timing);
void		bse_song_timing_get_default	(BseSongTiming	*timing);
BseSource*      bse_song_create_summation       (BseSong        *self);
BseBus*         bse_song_find_master            (BseSong        *self);
BseSource*      bse_song_ensure_master          (BseSong        *self);
void            bse_song_set_solo_bus           (BseSong *self, BseBus *bus);
BseTrack*       bse_song_find_first_track       (BseSong *self, BsePart *part);

G_END_DECLS

namespace Bse {

class SongImpl : public SNetImpl, public virtual SongIface {
protected:
  virtual    ~SongImpl                ();
public:
  explicit    SongImpl                (BseObject*);
  TrackIfaceP find_any_track_for_part (PartIface &part) override;
  BusIfaceP   create_bus              () override;
  void        remove_bus              (BusIface &bus) override;
  PartIfaceP  create_part             () override;
  void        remove_part             (PartIface &part) override;
};

} // Bse

#endif /* __BSE_SONG_H__ */
