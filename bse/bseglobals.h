/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_GLOBALS_H__
#define __BSE_GLOBALS_H__

#include <bse/bsedefs.h>
#include <bse/bsemath.h>
#include <bse/bsenote.h>
#include <bse/bseconstvalues.h>

G_BEGIN_DECLS

/* --- time ranges --- */ // FIXME: BSE_TIME_RANGE is deprecated
typedef enum
{
  BSE_TIME_RANGE_SHORT	= 1,
  BSE_TIME_RANGE_MEDIUM,
  BSE_TIME_RANGE_LONG
} BseTimeRangeType;
#define	BSE_TIME_RANGE_SHORT_ms		(1000.0 *   0.5)
#define	BSE_TIME_RANGE_MEDIUM_ms	(1000.0 *  10.0)
#define	BSE_TIME_RANGE_LONG_ms		(1000.0 * 200.0)
glong	bse_time_range_to_ms		(BseTimeRangeType	time_range);


/* --- async handlers --- */
/* most important, used for immediate async execution */
#define	BSE_PRIORITY_NOW		(-G_MAXINT / 2)
/* very important, used for io/engine handlers */
#define	BSE_PRIORITY_HIGH		(G_PRIORITY_HIGH - 10)
/* still very important, used for need-to-be-async operations */
#define	BSE_PRIORITY_NEXT		(G_PRIORITY_HIGH - 5)
/* important, delivers async signals */
#define	BSE_PRIORITY_NOTIFY		(G_PRIORITY_DEFAULT - 1)
/* normal importantance, interfaces to glue layer */
#define	BSE_PRIORITY_NORMAL		(G_PRIORITY_DEFAULT)
#define	BSE_PRIORITY_GLUE		(BSE_PRIORITY_NORMAL)
/* mildly important, used for GUI updates or user information */
#define	BSE_PRIORITY_UPDATE		(G_PRIORITY_HIGH_IDLE + 5)
/* unimportant, used when everything else done */
#define BSE_PRIORITY_BACKGROUND		(G_PRIORITY_LOW + 500)
guint	  bse_idle_now		(GSourceFunc    function,
				 gpointer       data);
guint	  bse_idle_next		(GSourceFunc    function,
				 gpointer       data);
guint	  bse_idle_notify	(GSourceFunc    function,
				 gpointer       data);
guint	  bse_idle_normal	(GSourceFunc    function,
				 gpointer       data);
guint	  bse_idle_update	(GSourceFunc    function,
				 gpointer       data);
guint	  bse_idle_background	(GSourceFunc    function,
				 gpointer       data);
gboolean  bse_idle_remove	(guint		id);
guint	  bse_idle_timed	(guint64	usec_delay,
				 GSourceFunc    function,
				 gpointer       data);


/* semitone factorization tables, i.e.
 * Index                     Factor
 * (SFI_KAMMER_NOTE - 12) -> 0.5
 * SFI_KAMMER_NOTE	  -> 1.0
 * (SFI_KAMMER_NOTE + 12) -> 2.0
 * etc...
 */
#define	BSE_SEMITONE_FACTOR(st)		(bse_transpose_factor (CLAMP (st, SFI_MIN_NOTE, SFI_MAX_NOTE) - SFI_KAMMER_NOTE))
#define	BSE_FINE_TUNE_FACTOR(ft)	(bse_cent_factor (CLAMP ((ft), BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE)))
#define	BSE_TRANSPOSE_FACTOR(st)	(bse_transpose_factor (CLAMP (st, -132, +132)))	/* BSE_MAX_TRANSPOSE */
#define	BSE_FREQ_FROM_LINEAR_VALUE(v)	(SFI_KAMMER_FREQ_d * BSE_SEMITONE_FACTOR (BSE_NOTE_FROM_VALUE (v) - SFI_KAMMER_NOTE))


/* --- prototypes --- */
void		bse_globals_init	(void);

/* --- decibel conversion --- */
gdouble	bse_db_to_factor	(gdouble	dB);
gdouble	bse_db_from_factor	(gdouble	factor,
                                 gdouble	min_dB);
#define	BSE_MINDB               (-96)   /* 32bit:-192 24bit:-144 16bit:-96 */

G_END_DECLS

#endif /* __BSE_GLOBALS_H__ */
