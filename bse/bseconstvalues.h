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
 */
#ifndef __BSE_CONST_VALUES_H__
#define __BSE_CONST_VALUES_H__

#include	<bse/bsedefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* BSE wide constant values */


/* --- frequencies --- */
/* kammer frequency, frequency of the note A of the standard piano
 * octave.slightly different values are used in different countries,
 * however, 440Hz is the official value for germany and much of europe.
 */
#define	BSE_KAMMER_FREQUENCY_f		(440.0)

/* maximum (audible) frequency represented in synthesis signals.
 * this value shouldn't be changed, to maintain frequency
 * representation in signals.
 */
#define	BSE_MAX_FREQUENCY_f		(24000.0)
#define	BSE_MAX_FREQUENCY_d		((gdouble) BSE_MAX_FREQUENCY_f)

/* minimum (needs to be > 0) and maximum audible frequencies supported
 * by oscillators (outer limits for user supplied frequency values)
 */
#define	BSE_MAX_OSC_FREQUENCY_d		((gdouble) 20000.0)
#define	BSE_MIN_OSC_FREQUENCY_d		((gdouble) 1.0 / BSE_MAX_OSC_FREQUENCY_d)
#define	BSE_MAX_OSC_FREQUENCY_f		((gfloat) BSE_MAX_OSC_FREQUENCY_d)
#define	BSE_MIN_OSC_FREQUENCY_f		((gfloat) BSE_MIN_OSC_FREQUENCY_d)

/* epsilon used to compare audible frequencies and check for equality */
#define BSE_FREQUENCY_EPSILON		((gdouble) 0.001)


/* --- notes --- */
/* notes are generally kept in signed integers, though they are zero
 * bounded, they are often used within expression that shouldn't be
 * promoted into unsigned expressions.
 */
#define	BSE_MIN_NOTE			(0)	/* assumed to be 0 in various places */
#define	BSE_MAX_NOTE			(131 /* 123 */)

/* special note values to represent
 * 1) no value specified
 * 2) note couldn't be parsed from external (string) representation
 */
#define	BSE_NOTE_VOID			(BSE_MAX_NOTE + 1)
#define	BSE_NOTE_UNPARSABLE		(BSE_NOTE_VOID + 1)

/* kammer note, representing the kammer frequency's note value */
#define	BSE_KAMMER_NOTE			((gint) (69) /* A' */)
#define	BSE_KAMMER_OCTAVE		((gint) (+1))

/* resulting minimum and maximum octaves */
#define	BSE_MIN_OCTAVE			(BSE_NOTE_OCTAVE (BSE_MIN_NOTE))
#define	BSE_MAX_OCTAVE			(BSE_NOTE_OCTAVE (BSE_MAX_NOTE))

/* macro to retrive a valid note. simply defaults to kammer note
 * for invalid note values
 */
#define	BSE_NOTE_MAKE_VALID(n)		((n) > BSE_MAX_NOTE || (n) < BSE_MIN_NOTE ? BSE_KAMMER_NOTE : ((gint) (n)))
#define	BSE_NOTE_IS_VALID(n)		((n) >= BSE_MIN_NOTE && (n) <= BSE_MAX_NOTE)

/* clamp note against boundaries in cases of underflow or overflow */
#define	BSE_NOTE_CLAMP(n)	(CLAMP (((gint) (n)), BSE_MIN_NOTE, BSE_MAX_NOTE))


/* macros to compose and decompose note values into semitones and octaves */
#define	BSE_NOTE_OCTAVE(n)		((((gint) (n)) - BSE_NOTE_SEMITONE (n) - (BSE_KAMMER_NOTE - 9)) / 12 + BSE_KAMMER_OCTAVE)
#define	BSE_NOTE_SEMITONE(n)		(((gint) (n)) % 12 + (9 - (BSE_KAMMER_NOTE % 12)))
#define	BSE_NOTE_GENERIC(o,ht_i)	(BSE_KAMMER_NOTE - 9 + ((gint) (ht_i)) + (((gint) (o)) - BSE_KAMMER_OCTAVE) * 12)
#define	BSE_NOTE_C(o)			(BSE_NOTE_GENERIC ((o), 0))
#define	BSE_NOTE_Cis(o)			(BSE_NOTE_GENERIC ((o), 1))
#define	BSE_NOTE_Des(o)			(BSE_NOTE_Cis (o))
#define	BSE_NOTE_D(o)			(BSE_NOTE_GENERIC ((o), 2))
#define	BSE_NOTE_Dis(o)			(BSE_NOTE_GENERIC ((o), 3))
#define	BSE_NOTE_Es(o)			(BSE_NOTE_Dis (o))
#define	BSE_NOTE_E(o)			(BSE_NOTE_GENERIC ((o), 4))
#define	BSE_NOTE_F(o)			(BSE_NOTE_GENERIC ((o), 5))
#define	BSE_NOTE_Fis(o)			(BSE_NOTE_GENERIC ((o), 6))
#define	BSE_NOTE_Ges(o)			(BSE_NOTE_Fis (o))
#define	BSE_NOTE_G(o)			(BSE_NOTE_GENERIC ((o), 7))
#define	BSE_NOTE_Gis(o)			(BSE_NOTE_GENERIC ((o), 8))
#define	BSE_NOTE_As(o)			(BSE_NOTE_Gis (o))
#define	BSE_NOTE_A(o)			(BSE_NOTE_GENERIC ((o), 9))
#define	BSE_NOTE_Ais(o)			(BSE_NOTE_GENERIC ((o), 10))
#define	BSE_NOTE_Bes(o)			(BSE_NOTE_Ais (o))
#define	BSE_NOTE_B(o)			(BSE_NOTE_GENERIC ((o), 11))
#define	_BSE_NOTE_SHIFT_AUX(n,ht,dfl)	(n + ht >= BSE_MIN_NOTE && n + ht <= BSE_MAX_NOTE ? n + ht : dfl)
#define	BSE_NOTE_SHIFT(n,ht_i)		(_BSE_NOTE_SHIFT_AUX ((gint) (n), (gint) (ht), (gint) (n)))
#define	BSE_NOTE_OCTAVE_UP(n)		(BSE_NOTE_SHIFT ((n), +12))
#define	BSE_NOTE_OCTAVE_DOWN(n)		(BSE_NOTE_SHIFT ((n), -12))


/* --- fine tune --- */
/* fine tune in cents of a semitone */
#define	BSE_MIN_FINE_TUNE		(-100)
#define	BSE_MAX_FINE_TUNE		(+100)
#define BSE_FINE_TUNE_IS_VALID(n)	((n) >= BSE_MIN_FINE_TUNE && (n) <= BSE_MAX_FINE_TUNE)


/* --- signal ranges --- */
/* min..max sample value: -1.0 .. 1.0
 * notes<->sample value: 0 .. 127 (BSE_VALUE_FROM_NOTE)
 * freq<->sample value: 0 .. 24000 (BSE_FREQ_FROM_VALUE)
 */
#define	BSE_VALUE_FROM_BOUNDED_UINT(k,b) ((gfloat) ((k) * (1.0 / ((gfloat) (b)))))
#define	BSE_FREQ_FROM_VALUE(value)	 (((gfloat) (value)) * BSE_MAX_FREQUENCY_f)
#define	BSE_VALUE_FROM_FREQ(freq)	 ((gfloat) ((freq) * (1.0 / BSE_MAX_FREQUENCY_f)))
#define	BSE_FREQUENCY_EPSILON		 ((gdouble) 0.001)


/* --- volume --- */
#define	BSE_MIN_VOLUME_dB		(-40) /* theoretically: -48.165 */
#define	BSE_MAX_VOLUME_dB		(+10)


/* --- balance (left/right volume) --- */
#define	BSE_MIN_BALANCE_f		(-100.0)
#define	BSE_MAX_BALANCE_f		(+100.0)

/* --- bpm --- */
#define	BSE_MIN_BPM			(1)
#define	BSE_MAX_BPM			(1024)


/* --- time (unix seconds) --- */
#define BSE_MIN_TIME			(631148400)	/* 1990-01-01 00:00:00 */
#define	BSE_MAX_TIME			(2147483647)	/* 2038-01-19 04:14:07 */


/* --- BseSource limits --- */
#define	BSE_MAX_N_ICHANNELS		(32)
#define	BSE_MAX_N_OCHANNELS		(32)


/* --- miscellaneous --- */
#define BSE_MAGIC                       (('B' << 24) | ('S' << 16) | \
	    /* 1112753441 0x42534521 */	 ('E' <<  8) | ('!' <<  0))
/* driver rating */
#define	BSE_RATING_DEFAULT		(500)
#define	BSE_RATING_FALLBACK		(1)



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_CONST_VALUES_H__ */
