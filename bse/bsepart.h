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
#define BSE_TYPE_PART			(BSE_TYPE_ID (BsePart))
#define BSE_PART(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PART, BsePart))
#define BSE_PART_CLASS(class)		(G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PART, BsePartClass))
#define BSE_IS_PART(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PART))
#define BSE_IS_PART_CLASS(class)	(G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PART))
#define BSE_PART_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PART, BsePartClass))


/* --- typedefs & structures --- */
typedef struct _BsePartNode  BsePartNode;
struct _BsePart
{
  BseItem	parent_instance;

  guint		n_ids;
  guint	       *ids;
  guint		last_id;	/* freed id list */

  guint		n_nodes;
  BsePartNode  *nodes;
  guint		last_tick_SL;

  guint		ltu_queued : 1;
  guint		ltu_recalc : 1;
  guint		range_queued : 1;

  /* queued updates */
  guint		range_tick;
  guint		range_bound;
  gint		range_min_note;
  gint		range_max_note;
};
struct _BsePartClass
{
  BseItemClass parent_class;

  void	(*range_changed)	(BsePart	*part,
				 guint		 tick,
				 guint		 duration,
				 gint		 range_min_note,
				 gint		 range_max_note);
};
typedef enum	/*< skip >*/
{
  BSE_PART_EVENT_NONE,
  BSE_PART_EVENT_NOTE,
  BSE_PART_EVENT_CONTROL
} BsePartEventType;


/* --- functions --- */
gboolean           bse_part_delete_event              (BsePart           *self,
                                                       guint              id);
guint              bse_part_insert_note               (BsePart           *self,
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
gboolean           bse_part_is_selected_event         (BsePart           *self,
                                                       guint              id);
BsePartNoteSeq*    bse_part_list_notes_crossing       (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               min_note,
                                                       gint               max_note);
BsePartControlSeq* bse_part_list_controls             (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       BseMidiSignalType  ctype);
void               bse_part_queue_notes_within        (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               min_note,
                                                       gint               max_note);
void               bse_part_queue_controls            (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration);
BsePartNoteSeq*    bse_part_list_selected_notes       (BsePart           *self);
BsePartControlSeq* bse_part_list_selected_controls    (BsePart           *self,
                                                       BseMidiSignalType  ctype);
void               bse_part_select_notes              (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               min_note,
                                                       gint               max_note);
void               bse_part_select_controls           (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       BseMidiSignalType  ctype);
void               bse_part_select_notes_exclusive    (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               min_note,
                                                       gint               max_note);
void               bse_part_select_controls_exclusive (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       BseMidiSignalType  ctype);
void               bse_part_deselect_notes            (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       gint               min_note,
                                                       gint               max_note);
void               bse_part_deselect_controls         (BsePart           *self,
                                                       guint              tick,
                                                       guint              duration,
                                                       BseMidiSignalType  ctype);
gboolean           bse_part_select_event              (BsePart           *self,
                                                       guint              id);
gboolean           bse_part_deselect_event            (BsePart           *self,
                                                       guint              id);
typedef struct {
  guint             id;
  BsePartEventType  event_type;
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
BsePartEventType   bse_part_query_event               (BsePart           *self,
                                                       guint              id,
                                                       BsePartQueryEvent *equery);
guint              bse_part_node_lookup_SL            (BsePart           *self,
                                                       guint              tick);





/* --- implementation details --- */
#define	BSE_PART_MAX_TICK		(0x7fffffff)
#define	BSE_PART_INVAL_TICK_FLAG	(0x80000000)
#define	BSE_PART_NOTE_EVENT_FREQ(nev)	(BSE_KAMMER_FREQUENCY_f * \
                                         BSE_SEMITONE_FACTOR ((nev)->note) * \
                                         BSE_FINE_TUNE_FACTOR ((nev)->fine_tune))
#define BSE_PART_NOTE_CONTROL(ctype)    ((ctype) == BSE_MIDI_SIGNAL_VELOCITY || \
                                         (ctype) == BSE_MIDI_SIGNAL_FINE_TUNE)
typedef union  _BsePartEventUnion BsePartEventUnion;
typedef struct
{
  BsePartEventType   type;
  BsePartEventUnion *next;
  guint		     id : 31;
  guint		     selected : 1; // FIXME
} BsePartEventAny;
typedef struct
{
  BsePartEventAny    any;       /* BSE_PART_EVENT_NOTE */
  guint		     duration;	/* in ticks */
  gint		     note;
  gint		     fine_tune;
  gfloat	     velocity;	/* 0 .. 1 */
} BsePartEventNote;
typedef struct
{
  BsePartEventAny    any;       /* BSE_PART_EVENT_CONTROL */
  guint		     ctype;	/* BseMidiSignalType */
  gfloat	     value;	/* 0 .. 1 */
} BsePartEventControl;
union _BsePartEventUnion
{
  BsePartEventType    type;
  BsePartEventAny     any;
  BsePartEventNote    note;
  BsePartEventControl control;
};
struct _BsePartNode
{
  guint	             tick;
  BsePartEventUnion *events;
};


G_END_DECLS

#endif /* __BSE_PART_H__ */
