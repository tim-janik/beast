/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


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
  BseItem      parent_instance;

  guint	       n_ids;
  guint	      *ids;
  guint	       head_id, tail_id;	/* free id list */

  guint	       n_nodes;
  BsePartNode *nodes;
  guint	       ppqn;

  guint	       range_tick;
  guint	       range_bound;
  gint         range_min_note;
  gint         range_max_note;
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


/* --- functions --- */
guint		 bse_part_insert_note		(BsePart	*self,
						 guint		 tick,
						 guint		 duration,
						 gint		 note,
						 gint		 fine_tune,
						 gfloat		 velocity);
gboolean	 bse_part_change_note		(BsePart	*self,
						 guint		 id,
						 guint		 tick,
						 guint		 duration,
						 gint		 note,
						 gint		 fine_tune,
						 gfloat		 velocity);
gboolean	 bse_part_delete_event		(BsePart	*self,
						 guint		 id);
gboolean	 bse_part_is_selected_event	(BsePart	*self,
						 guint		 id);
BswIterPartNote* bse_part_list_notes_around	(BsePart	*self,
						 guint		 tick,
						 guint		 duration,
						 gint		 min_note,
						 gint		 max_note);
void		 bse_part_queue_notes_within	(BsePart	*self,
						 guint		 tick,
						 guint		 duration,
						 gint		 min_note,
						 gint		 max_note);
BswIterPartNote* bse_part_list_selected_notes	(BsePart	*self);
BswIterPartNote* bse_part_list_notes_at		(BsePart	*self,
						 guint		 tick,
						 gint		 note);
void		 bse_part_select_rectangle	(BsePart	*self,
						 guint		 tick,
						 guint		 duration,
						 gint		 min_note,
						 gint		 max_note);
void		 bse_part_deselect_rectangle	(BsePart	*self,
						 guint		 tick,
						 guint		 duration,
						 gint		 min_note,
						 gint		 max_note);
void		 bse_part_select_rectangle_ex	(BsePart	*self,
						 guint		 tick,
						 guint		 duration,
						 gint		 min_note,
						 gint		 max_note);
gboolean	 bse_part_select_event		(BsePart	*self,
						 guint		 id);
gboolean	 bse_part_deselect_event	(BsePart	*self,
						 guint		 id);
guint		 bse_part_node_lookup_SL	(BsePart	*self,
						 guint		 tick);


/* --- implementation details --- */
typedef enum	/*< skip >*/
{
  BSE_PART_EVENT_NONE,
  BSE_PART_EVENT_NOTE,
  BSE_PART_EVENT_CONTROL
} BsePartEventType;

#define	BSE_PART_MAX_TICK		(0x7fffffff)
#define	BSE_PART_INVAL_TICK_FLAG	(0x80000000)
#define	BSE_PART_NOTE_EVENT_FREQ(nev)	(BSE_KAMMER_FREQUENCY_f * \
                                         BSE_SEMITONE_FACTOR ((nev)->note) * \
                                         BSE_FINE_TUNE_FACTOR ((nev)->fine_tune))
typedef union  _BsePartEvent BsePartEvent;
typedef struct
{
  BsePartEventType type;
  BsePartEvent    *next;
  guint		   id : 31;
  guint		   selected : 1; // FIXME
} BsePartEventAny;
typedef struct
{
  BsePartEventType type;	/* BSE_PART_EVENT_NOTE */
  BsePartEvent    *next;
  guint		   id : 31;
  guint		   selected : 1; // FIXME
  guint		   duration;	/* in ticks */
  gint		   note;
  gint		   fine_tune;
  gfloat	   velocity;	/* 0 .. 1 */
} BsePartEventNote;
typedef struct
{
  BsePartEventType type;	/* BSE_PART_EVENT_CONTROL */
  guint		   id : 31;
  guint		   selected : 1; // FIXME
  BsePartEvent	  *next;
  guint		   control;	/* BsePartControlType */
  gfloat	   value;	/* 0 .. 1 */
} BsePartEventControl;
union _BsePartEvent
{
  BsePartEventType    type;
  BsePartEventAny     any;
  BsePartEventNote    note;
  BsePartEventControl control;
};
struct _BsePartNode
{
  guint	        tick;
  BsePartEvent *events;
};


/* --- proposed --- */
typedef enum	/*< skip >*/
{
  BSE_PART_CONTROL_NONE		= 0x00,
  BSE_PART_CONTROL_VOLUME	= 0x07,
  BSE_PART_CONTROL_BALANCE	= 0x08,
} BsePartControlType;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PART_H__ */
