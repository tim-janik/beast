// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PART_H__
#define __BSE_PART_H__

#include <bse/bseitem.hh>
#include <bse/gbsearcharray.hh>

/* --- object type macros --- */
#define BSE_TYPE_PART                   (BSE_TYPE_ID (BsePart))
#define BSE_PART(object)                (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PART, BsePart))
#define BSE_PART_CLASS(class)           (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PART, BsePartClass))
#define BSE_IS_PART(object)             (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PART))
#define BSE_IS_PART_CLASS(class)        (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PART))
#define BSE_PART_GET_CLASS(object)      (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PART, BsePartClass))


/* --- typedefs & structures --- */
struct BsePartControls {
  GBSearchArray *bsa;
};
struct BsePartNoteChannel {
  GBSearchArray *bsa;
};
struct BsePart : BseItem {
  const double       *semitone_table; // [-132..+132] only updated when not playing
  /* id -> tick lookups */
  guint               n_ids;
  guint              *ids;
  guint               last_id;        /* head of free id list */
  /* control events */
  BsePartControls     controls;
  /* notes */
  guint               n_channels;
  BsePartNoteChannel *channels;
  /* one after any tick used by controls or notes */
  guint               last_tick_SL;
  /* queued updates */
  guint               links_queued : 1;
  guint               range_queued : 1;
  guint               range_tick;
  guint               range_bound;
  gint                range_min_note;
  gint                range_max_note;
};
struct BsePartClass : BseItemClass {
  void  (*range_changed)        (BsePart        *part,
                                 guint           tick,
                                 guint           duration,
                                 gint            range_min_note,
                                 gint            range_max_note);
};
typedef enum    /*< skip >*/
{
  BSE_PART_EVENT_NONE,
  BSE_PART_EVENT_CONTROL,
  BSE_PART_EVENT_NOTE
} BsePartEventType;

#define            bse_part_transpose_factor(          part, index /* -132..+132*/)     ((part)->semitone_table[index])
void               bse_part_set_semitone_table        (BsePart           *self,
                                                       const double      *semitone_table);
void               bse_part_links_changed             (BsePart           *self);
Bse::PartLinkSeq   bse_part_list_links                (BsePart *self);
gboolean           bse_part_delete_control            (BsePart           *self,
                                                       guint              id);
gboolean           bse_part_delete_note               (BsePart           *self,
                                                       guint              id,
                                                       guint              channel);
guint              bse_part_insert_note               (BsePart           *self,
                                                       guint              channel,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               note,
                                                       gint               fine_tune,
                                                       gfloat             velocity);
guint              bse_part_insert_control            (BsePart           *self,
                                                       guint              tick,
                                                       Bse::MidiSignal  ctype,
                                                       gfloat             value);
gboolean           bse_part_change_note               (BsePart           *self,
                                                       guint              id,
                                                       guint              channel,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               note,
                                                       gint               fine_tune,
                                                       gfloat             velocity);
gboolean           bse_part_change_control            (BsePart           *self,
                                                       guint              id,
                                                       guint              tick,
                                                       Bse::MidiSignal  ctype,
                                                       gfloat             value);
Bse::PartNoteSeq   bse_part_list_notes                (BsePart           *self,
                                                       guint              channel,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               min_note,
                                                       gint               max_note,
                                                       gboolean           include_crossings);
Bse::PartControlSeq bse_part_list_controls           (BsePart           *self,
                                                       guint              channel, /* for note events */
                                                       guint              tick,
                                                       guint              duration,
                                                       Bse::MidiSignal  ctype);
void               bse_part_queue_notes_within        (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               min_note,
                                                       gint               max_note);
#define            bse_part_queue_controls(p,t,d)          bse_part_queue_notes_within (p, t, d, BSE_MIN_NOTE, BSE_MAX_NOTE)
Bse::PartNoteSeq    bse_part_list_selected_notes      (BsePart           *self);
Bse::PartControlSeq bse_part_list_selected_controls  (BsePart *self, Bse::MidiSignal ctype);
void               bse_part_select_notes              (BsePart           *self,
                                                       guint              channel,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               min_note,
                                                       gint               max_note,
                                                       gboolean           selected);
void               bse_part_select_controls           (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       Bse::MidiSignal  ctype,
                                                       gboolean           selected);
void               bse_part_select_notes_exclusive    (BsePart           *self,
                                                       guint              channel,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               min_note,
                                                       gint               max_note);
void               bse_part_select_controls_exclusive (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       Bse::MidiSignal  ctype);
gboolean           bse_part_set_note_selected         (BsePart           *self,
                                                       guint              id,
                                                       guint              channel,
                                                       gboolean           selected);
gboolean           bse_part_set_control_selected      (BsePart           *self,
                                                       guint              id,
                                                       gboolean           selected);
struct BsePartQueryEvent {
  guint             id;
  BsePartEventType  event_type;
  guint             channel;
  guint             tick;
  gboolean          selected;
  /* note */
  guint             duration;
  gint              note;
  gint              fine_tune;
  gfloat            velocity;
  /* note control */
  gfloat            fine_tune_value;
  gfloat            velocity_value;
  /* control */
  Bse::MidiSignal control_type;
  gfloat            control_value;
};

BsePartEventType   bse_part_query_event         (BsePart           *self,
                                                 guint              id,
                                                 BsePartQueryEvent *equery);


/* --- implementation details --- */
#define BSE_PART_MAX_CHANNELS           (0x1024)
#define BSE_PART_MAX_TICK               (0x7fffffff)
#define BSE_PART_INVAL_TICK_FLAG        (0x80000000)
#define BSE_PART_NOTE_CONTROL(ctype)    ((ctype) == Bse::MidiSignal::VELOCITY || \
                                         (ctype) == Bse::MidiSignal::FINE_TUNE)

/* --- BsePartControlChannel --- */
struct BsePartEventControl;
struct BsePartTickNode {
  guint                tick;
  BsePartEventControl *events;
};
struct BsePartEventControl {
  BsePartEventControl   *next;
  guint                  id : 31;
  guint                  selected : 1;
  guint                  ctype; /* Bse::MidiSignal */
  gfloat                 value;         /* -1 .. 1 */
};

void                 bse_part_controls_init            (BsePartControls     *self);
BsePartTickNode*     bse_part_controls_lookup          (BsePartControls     *self,
                                                        guint                tick);
BsePartEventControl* bse_part_controls_lookup_event    (BsePartControls     *self,
                                                        guint                tick,
                                                        guint                id);
BsePartTickNode*     bse_part_controls_lookup_ge       (BsePartControls     *self,
                                                        guint                tick);
BsePartTickNode*     bse_part_controls_lookup_lt       (BsePartControls     *self,
                                                        guint                tick);
BsePartTickNode*     bse_part_controls_lookup_le       (BsePartControls     *self,
                                                        guint                tick);
BsePartTickNode*     bse_part_controls_get_bound       (BsePartControls     *self);
guint                bse_part_controls_get_last_tick   (BsePartControls     *self);
BsePartTickNode*     bse_part_controls_ensure_tick     (BsePartControls     *self,
                                                        guint                tick);
void                 bse_part_controls_insert          (BsePartControls     *self,
                                                        BsePartTickNode     *node,
                                                        guint                id,
                                                        guint                selected,
                                                        guint                ctype,
                                                        gfloat               value);
void                 bse_part_controls_change          (BsePartControls     *self,
                                                        BsePartTickNode     *node,
                                                        BsePartEventControl *cev,
                                                        guint                id,
                                                        guint                selected,
                                                        guint                ctype,
                                                        gfloat               value);
void                 bse_part_controls_change_selected (BsePartEventControl *cev,
                                                        guint                selected);
void                 bse_part_controls_remove          (BsePartControls     *self,
                                                        guint                tick,
                                                        BsePartEventControl *cev);
void                 bse_part_controls_destroy         (BsePartControls     *self);

struct BsePartEventNote {
  guint                  tick;
  guint                  id : 31;
  guint                  selected : 1;
  guint                 *crossings;
  guint                  duration;      /* in ticks */
  gint                   note;
  gint                   fine_tune;
  gfloat                 velocity;      /* 0 .. 1 */
};

#define BSE_PART_NOTE_N_CROSSINGS(note)         ((note)->crossings ? (note)->crossings[0] : 0)
#define BSE_PART_NOTE_CROSSING(note,j)          ((note)->crossings[1 + (j)])
#define BSE_PART_SEMITONE_FACTOR(part,noteval)  (bse_part_transpose_factor ((part), CLAMP ((noteval), SFI_MIN_NOTE, SFI_MAX_NOTE) - SFI_KAMMER_NOTE))
#define BSE_PART_NOTE_FREQ(part,note)           (BSE_KAMMER_FREQUENCY *                                 \
                                                 BSE_PART_SEMITONE_FACTOR ((part), (note)->note) *      \
                                                 bse_cent_tune_fast ((note)->fine_tune))

void              bse_part_note_channel_init          (BsePartNoteChannel *self);
BsePartEventNote* bse_part_note_channel_lookup        (BsePartNoteChannel *self,
                                                       guint               tick);
BsePartEventNote* bse_part_note_channel_lookup_le     (BsePartNoteChannel *self,
                                                       guint               tick);
BsePartEventNote* bse_part_note_channel_lookup_lt     (BsePartNoteChannel *self,
                                                       guint               tick);
BsePartEventNote* bse_part_note_channel_lookup_ge     (BsePartNoteChannel *self,
                                                       guint               tick);
BsePartEventNote* bse_part_note_channel_get_bound     (BsePartNoteChannel *self);
guint             bse_part_note_channel_get_last_tick (BsePartNoteChannel *self);
BsePartEventNote* bse_part_note_channel_insert        (BsePartNoteChannel *self,
                                                       BsePartEventNote    key);
void              bse_part_note_channel_change_note   (BsePartNoteChannel *self,
                                                       BsePartEventNote   *note,
                                                       guint               id,
                                                       gboolean            selected,
                                                       gint                vnote,
                                                       gint                fine_tune,
                                                       gfloat              velocity);
void              bse_part_note_channel_remove        (BsePartNoteChannel *self,
                                                       guint               tick);
void              bse_part_note_channel_destroy       (BsePartNoteChannel *self);

namespace Bse {

class PartImpl : public ItemImpl, public virtual PartIface {
protected:
  virtual               ~PartImpl               ();
public:
  explicit               PartImpl               (BseObject*);
  virtual PartNoteSeq    list_notes_crossing    (int tick, int duration) override;
  virtual PartNoteSeq    list_notes_within      (int channel, int tick, int duration) override;
  virtual PartNoteSeq    list_selected_notes    () override;
  virtual PartNoteSeq    check_overlap          (int tick, int duration, int note) override;
  virtual PartNoteSeq    get_notes              (int tick, int note) override;
  virtual PartControlSeq list_selected_controls (MidiSignal control_type) override;
  virtual PartControlSeq list_controls          (int tick, int duration, MidiSignal control_type) override;
  virtual PartControlSeq get_channel_controls   (int channel, int tick, int duration, MidiSignal control_type) override;
  virtual PartControlSeq get_controls           (int tick, MidiSignal control_type) override;
  virtual PartLinkSeq    list_links             () override;
  virtual SongTiming     get_timing             (int tick) override;
  virtual int            get_max_note           () override;
  virtual int            get_min_note           () override;
  virtual int            get_last_tick          () override;
  virtual Error      change_control         (int id, int tick, MidiSignal control_type, double value) override;
  virtual Error      change_note            (int id, int tick, int duration, int note, int fine_tune, double velocity) override;
  virtual Error      delete_event           (int id) override;
  virtual void deselect_controls         (int tick, int duration, MidiSignal control_type) override;
  virtual void deselect_event            (int id) override;
  virtual void deselect_notes            (int tick, int duration, int min_note, int max_note) override;
  virtual bool is_event_selected         (int id) override;
  virtual void select_controls           (int tick, int duration, MidiSignal control_type) override;
  virtual void select_controls_exclusive (int tick, int duration, MidiSignal control_type) override;
  virtual void select_event              (int id) override;
  virtual void select_notes              (int tick, int duration, int min_note, int max_note) override;
  virtual void select_notes_exclusive    (int tick, int duration, int min_note, int max_note) override;
  virtual int  insert_control            (int tick, MidiSignal control_type, double value) override;
  virtual int  insert_note               (int channel, int tick, int duration, int note, int fine_tune, double velocity) override;
  virtual int  insert_note_auto          (int tick, int duration, int note, int fine_tune, double velocity) override;
  virtual void queue_controls            (int tick, int duration) override;
  virtual void queue_notes               (int tick, int duration, int min_note, int max_note) override;
};

} // Bse


#endif /* __BSE_PART_H__ */
