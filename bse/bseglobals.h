/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-2002 Tim Janik
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
 *
 * bseglobals.h: globals definitions and values for BSE
 */
#ifndef __BSE_GLOBALS_H__
#define __BSE_GLOBALS_H__

#include	<bse/bsedefs.h>
#include	<bse/bsemath.h>
#include	<bse/bseconstvalues.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- signal ranges --- */
/* min..max sample value: -1.0 .. 1.0
 * notes<->sample value: 0 .. 127 (BSE_VALUE_FROM_NOTE)
 * freq<->sample value: 0 .. 24000 (BSE_FREQ_FROM_VALUE)
 */
#define	BSE_FREQ_FROM_VALUE(value)	 (((gfloat) (value)) * BSE_MAX_FREQUENCY_f)
#define	BSE_VALUE_FROM_FREQ(freq)	 ((gfloat) ((freq) * (1.0 / BSE_MAX_FREQUENCY_f)))


/* --- time ranges --- */
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


/* --- FIXME: deprecated constants --- */
#define	BSE_MAX_N_TRACKS		(256)
#define	BSE_MAX_ENV_TIME		(10000) /* ms */
#define	BSE_BBUFFER_SIZE		(128)	// FIXME
#define	BSE_MAX_BLOCK_PADDING		(64) /* Gsl wave_chunk_padding */
#define BSE_MIN_BIT_SIZE                (8)
#define BSE_MAX_BIT_SIZE                (16)
#define BSE_DFL_BIN_DATA_BITS		(16)	// FIXME


/* --- hard defaults (used for saving) --- */
#define	BSE_DFL_MASTER_VOLUME_dB	(0.0)
#define	BSE_DFL_SONG_BPM		(160)
#define	BSE_DFL_INSTRUMENT_VOLUME_dB	(- 0.969)
#define BSE_DFL_INSTRUMENT_BALANCE	(0)
#define BSE_DFL_INSTRUMENT_TRANSPOSE	(0)
#define BSE_DFL_INSTRUMENT_FINE_TUNE	(0)


/* --- Convenience --- */
#define	BSE_MIX_FREQ	(gsl_engine_sample_freq ())
#define	BSE_MIX_FREQ_f	((gfloat) BSE_MIX_FREQ)
#define	BSE_MIX_FREQ_d	((gdouble) BSE_MIX_FREQ)


/* --- async handlers --- */
/* very important, used for io/engine handlers */
#define	BSE_PRIORITY_HIGH		(G_PRIORITY_HIGH - 10)
/* still very important, used for need-to-be-async operations */
#define	BSE_PRIORITY_NOW		(G_PRIORITY_HIGH - 5)
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
extern const gdouble* _bse_semitone_factor_table;
#define	BSE_SEMITONE_FACTOR(ht)		((ht) > SFI_MAX_NOTE ? \
				         _bse_semitone_factor_table[SFI_MAX_NOTE] : \
				         (ht) < SFI_MIN_NOTE ? \
				         _bse_semitone_factor_table[SFI_MIN_NOTE] : \
				         _bse_semitone_factor_table[(ht)])
extern const gdouble* _bse_fine_tune_factor_table;
#define	BSE_FINE_TUNE_FACTOR(ft)	((ft) > BSE_MAX_FINE_TUNE ? \
                                         _bse_fine_tune_factor_table[BSE_MAX_FINE_TUNE] : \
                                         (ft) < BSE_MIN_FINE_TUNE ? \
                                         _bse_fine_tune_factor_table[BSE_MIN_FINE_TUNE] : \
                                         _bse_fine_tune_factor_table[(ft)])
#define	BSE_FREQ_FROM_LINEAR_VALUE(v)	(SFI_KAMMER_FREQ_d * BSE_SEMITONE_FACTOR (BSE_NOTE_FROM_VALUE (v) - SFI_KAMMER_NOTE))


/* --- prototypes --- */
void		bse_globals_init	(void);

/* conversion */
gdouble		bse_dB_to_factor	(gfloat		dB);
gfloat		bse_dB_from_factor	(gdouble	factor,
					 gfloat		min_dB);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_GLOBALS_H__ */
