/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002-2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_PART_H__
#define __BSE_PART_H__

#include        <bse/bseitem.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_PART                   (BSE_TYPE_ID (BsePart))
#define BSE_PART(object)                (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PART, BsePart))
#define BSE_PART_CLASS(class)           (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PART, BsePartClass))
#define BSE_IS_PART(object)             (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PART))
#define BSE_IS_PART_CLASS(class)        (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PART))
#define BSE_PART_GET_CLASS(object)      (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PART, BsePartClass))


/* --- typedefs & structures --- */
typedef struct {
  gpointer bsa;
} BsePartControls;
typedef struct {
  gpointer bsa;
} BsePartNoteChannel;
struct _BsePart
{
  BseItem             parent_instance;

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
struct _BsePartClass
{
  BseItemClass parent_class;

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


/* --- functions --- */
void               bse_part_links_changed             (BsePart           *self);
BsePartLinkSeq*    bse_part_list_links                (BsePart           *self);
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
                                                       BseMidiSignalType  ctype,
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
                                                       BseMidiSignalType  ctype,
                                                       gfloat             value);
BsePartNoteSeq*    bse_part_list_notes                (BsePart           *self,
                                                       guint              channel,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               min_note,
                                                       gint               max_note,
                                                       gboolean           include_crossings);
BsePartControlSeq* bse_part_list_controls             (BsePart           *self,
                                                       guint              channel, /* for note events */
                                                       guint              tick,
                                                       guint              duration,
                                                       BseMidiSignalType  ctype);
void               bse_part_queue_notes_within        (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               min_note,
                                                       gint               max_note);
#define            bse_part_queue_controls(p,t,d)          bse_part_queue_notes_within (p, t, d, BSE_MIN_NOTE, BSE_MAX_NOTE)
BsePartNoteSeq*    bse_part_list_selected_notes       (BsePart           *self);
BsePartControlSeq* bse_part_list_selected_controls    (BsePart           *self,
                                                       BseMidiSignalType  ctype);
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
                                                       BseMidiSignalType  ctype,
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
                                                       BseMidiSignalType  ctype);
gboolean           bse_part_set_note_selected         (BsePart           *self,
                                                       guint              id,
                                                       guint              channel,
                                                       gboolean           selected);
gboolean           bse_part_set_control_selected      (BsePart           *self,
                                                       guint              id,
                                                       gboolean           selected);
typedef struct {
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
  BseMidiSignalType control_type;
  gfloat            control_value;
} BsePartQueryEvent;
BsePartEventType   bse_part_query_event         (BsePart           *self,
                                                 guint              id,
                                                 BsePartQueryEvent *equery);


/* --- implementation details --- */
#define BSE_PART_MAX_CHANNELS           (0x1024)
#define BSE_PART_MAX_TICK               (0x7fffffff)
#define BSE_PART_INVAL_TICK_FLAG        (0x80000000)
#define BSE_PART_NOTE_CONTROL(ctype)    ((ctype) == BSE_MIDI_SIGNAL_VELOCITY || \
                                         (ctype) == BSE_MIDI_SIGNAL_FINE_TUNE)

/* --- BsePartControlChannel --- */
typedef struct _BsePartEventControl BsePartEventControl;
typedef struct
{
  guint                tick;
  BsePartEventControl *events;
} BsePartTickNode;
struct _BsePartEventControl
{
  BsePartEventControl   *next;
  guint                  id : 31;
  guint                  selected : 1;
  guint                  ctype; /* BseMidiSignalType */
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


/* --- BsePartNoteChannel --- */
typedef struct _BsePartEventNote BsePartEventNote;
struct _BsePartEventNote
{
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
#define BSE_PART_NOTE_FREQ(note)                (BSE_KAMMER_FREQUENCY *                 \
                                                 BSE_SEMITONE_FACTOR ((note)->note) *   \
                                                 BSE_FINE_TUNE_FACTOR ((note)->fine_tune))

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

G_END_DECLS

#endif /* __BSE_PART_H__ */
