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

  guint	       n_nodes;
  BsePartNode *nodes;
  guint	       ppqn;

  guint	       range_tick;
  guint	       range_bound;
  gfloat       range_min_freq;
  gfloat       range_max_freq;
};
struct _BsePartClass
{
  BseItemClass parent_class;

  void	(*range_changed)	(BsePart	*part,
				 guint		 tick,
				 guint		 duration,
				 gfloat		 min_freq,
				 gfloat		 max_freq);
};


/* --- functions --- */
BseErrorType	bse_part_insert_note	(BsePart	*part,
					 guint		 tick,
					 guint		 duration,
					 gfloat		 freq,
					 gfloat		 velocity);
void		bse_part_delete_note	(BsePart	*part,
					 guint		 tick,
					 gfloat		 freq);
BswIterPartNote* bse_part_list_notes	(BsePart	*part,
					 guint		 tick,
					 guint		 duration,
					 gfloat		 min_freq,
					 gfloat		 max_freq);
BswIterPartNote* bse_part_get_note_at	(BsePart	*part,
					 guint		 tick,
					 gfloat		 freq);
guint		bse_part_node_lookup_SL	(BsePart	*part,
					 guint		 tick);


/* --- implementation details --- */
typedef enum	/*< skip >*/
{
  BSE_PART_EVENT_NONE,
  BSE_PART_EVENT_NOTE,
  BSE_PART_EVENT_CONTROL
} BsePartEventType;

/* to speed up comparisons, we store frequencies as integers.
 * for forward conversion int = BSE_PART_FREQ_FACTOR * float + 0.5,
 * and for backward conversion float = int * BSE_PART_FREQ_IFACTOR_f
 * are used.
 */
#define	BSE_PART_FREQ_FACTOR		(65536)	/* preserve 16bit of fraction */
#define	BSE_PART_FREQ_IFACTOR_f		(1.0 / (gfloat) BSE_PART_FREQ_FACTOR)
#define	BSE_PART_IFREQ(float_freq)	((guint) (BSE_PART_FREQ_FACTOR * (float_freq) + 0.5))
#define	BSE_PART_FREQ(ifreq)		(BSE_PART_FREQ_IFACTOR_f * (ifreq))
#define	BSE_PART_MAX_TICK		(0x7fffffff)

typedef union  _BsePartEvent BsePartEvent;
typedef struct
{
  BsePartEventType type;
  BsePartEvent    *next;
} BsePartEventAny;
typedef struct
{
  BsePartEventType type;	/* BSE_PART_EVENT_NOTE */
  BsePartEvent    *next;
  guint		   ifreq;
  guint		   duration;	/* in ticks */
  gfloat	   velocity;	/* 0 .. 1 */
} BsePartEventNote;
typedef struct
{
  BsePartEventType type;	/* BSE_PART_EVENT_CONTROL */
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
