// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SONG_H__
#define __BSE_SONG_H__

#include        <bse/bsesnet.hh>



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
  int		    tpqn;		/* ticks per querter note */
  int		    numerator;
  int		    denominator;
  float             bpm;
  Bse::MusicalTuning musical_tuning;
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
  uint		   *tick_SL;		/* tick at stamp_SL */
  guint             sequencer_owns_refcount_SL : 1;
  guint             sequencer_underrun_detected_SL : 1;
  guint		    loop_enabled_SL : 1;
  SfiInt	    loop_left_SL;	/* left loop tick */
  SfiInt	    loop_right_SL;	/* left loop tick */
};
struct BseSongClass : BseSNetClass
{};

BseSong*   bse_song_lookup	       (BseProject *project, const char *name);
void	   bse_song_stop_sequencing_SL (BseSong	*self);
void	   bse_song_get_timing	       (BseSong	*self, uint tick, Bse::SongTiming *timing);
void	   bse_song_timing_get_default (Bse::SongTiming *timing);
BseSource* bse_song_create_summation   (BseSong *self);
BseBus*    bse_song_find_master        (BseSong *self);
BseSource* bse_song_ensure_master      (BseSong *self);
void       bse_song_set_solo_bus       (BseSong *self, BseBus *bus);
BseTrack*  bse_song_find_first_track   (BseSong *self, BsePart *part);

namespace Bse {

class SongImpl : public SNetImpl, public virtual SongIface {
  SharedBlock               shm_block_;
protected:
  virtual void              post_init               () override;
  virtual                  ~SongImpl                ();
public:
  explicit                  SongImpl                (BseObject*);
  void                      propagate_bpm           ();
  virtual int               numerator               () const override;
  virtual void              numerator               (int val) override;
  virtual int               denominator             () const override;
  virtual void              denominator             (int val) override;
  virtual double            bpm                     () const override;
  virtual void              bpm                     (double val) override;
  virtual int               tpqn                    () const override;
  virtual void              tpqn                    (int val) override;
  virtual MusicalTuning     musical_tuning          () const override;
  virtual void              musical_tuning          (MusicalTuning tuning) override;
  virtual bool              loop_enabled            () const override;
  virtual void              loop_enabled            (bool enabled) override;
  virtual int               loop_left               () const override;
  virtual void              loop_left               (int tick) override;
  virtual int               loop_right              () const override;
  virtual void              loop_right              (int tick) override;
  virtual int               tick_pointer            () const override;
  virtual void              tick_pointer            (int tick) override;
  virtual SongTiming        get_timing              (int tick) override;
  virtual TrackIfaceP       find_any_track_for_part (PartIface &part) override;
  virtual BusIfaceP         create_bus              () override;
  virtual void              remove_bus              (BusIface &bus) override;
  virtual PartIfaceP        create_part             () override;
  virtual void              remove_part             (PartIface &part) override;
  virtual TrackSeq          list_tracks             () override;
  virtual TrackIfaceP       create_track            () override;
  virtual void              remove_track            (TrackIface &track) override;
  virtual BusIfaceP         ensure_master_bus       () override;
  virtual void              ensure_track_links      () override;
  virtual TrackIfaceP       find_track_for_part     (PartIface &part) override;
  virtual BusIfaceP         get_master_bus          () override;
  virtual void              synthesize_note         (TrackIface &track, int duration, int note, int fine_tune, double velocity) override;
  virtual int64_t           get_shm_offset          (SongTelemetry fld) override;
};
using SongImplP = std::shared_ptr<SongImpl>;

} // Bse

#endif /* __BSE_SONG_H__ */
