/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* these are BSE wide defines to set certain limits or
 * give hard default values. most of them may not be adjusted
 * without corresponding code adjustments.
 * eventually tunable parameters can be found in the BseGlobals
 * structure.
 */



/* --- notes & note based frequencies --- */
#define	BSE_KAMMER_FREQ			(440)
#define	BSE_MIN_NOTE			(0)
#define	BSE_MAX_NOTE			(127)
#define	BSE_NOTE_VOID			(1024)
#define	BSE_NOTE_UNPARSABLE		(BSE_NOTE_VOID + 1)
#define	BSE_KAMMER_NOTE			((gint) (69) /* A! */)
#define	BSE_KAMMER_OCTAVE		((gint) (-1))
#define	BSE_NOTE_C(o)			(CLAMP (BSE_KAMMER_NOTE - 9 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_Cis(o)			(CLAMP (BSE_KAMMER_NOTE - 8 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_Des(o)			(BSE_NOTE_Cis (o))
#define	BSE_NOTE_D(o)			(CLAMP (BSE_KAMMER_NOTE - 7 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_Dis(o)			(CLAMP (BSE_KAMMER_NOTE - 6 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_Es(o)			(BSE_NOTE_Dis (o))
#define	BSE_NOTE_E(o)			(CLAMP (BSE_KAMMER_NOTE - 5 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_F(o)			(CLAMP (BSE_KAMMER_NOTE - 4 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_Fis(o)			(CLAMP (BSE_KAMMER_NOTE - 3 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_Ges(o)			(BSE_NOTE_Fis (o))
#define	BSE_NOTE_G(o)			(CLAMP (BSE_KAMMER_NOTE - 2 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_Gis(o)			(CLAMP (BSE_KAMMER_NOTE - 1 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_As(o)			(BSE_NOTE_Gis (o))
#define	BSE_NOTE_A(o)			(CLAMP (BSE_KAMMER_NOTE + 0 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_Ais(o)			(CLAMP (BSE_KAMMER_NOTE + 1 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_Bes(o)			(BSE_NOTE_Ais (o))
#define	BSE_NOTE_B(o)			(CLAMP (BSE_KAMMER_NOTE + 2 + ((o) - BSE_KAMMER_OCTAVE) * 12, BSE_MIN_NOTE, BSE_MAX_NOTE))
#define	BSE_NOTE_GENERIC(ht_i,o)	(BSE_NOTE_C(o)+(ht_i)-1 >= BSE_MIN_NOTE && BSE_NOTE_C(o)+(ht_i)-1 <= BSE_MAX_NOTE ? BSE_NOTE_C(o)+(ht_i)-1 : BSE_NOTE_VOID)
#define	BSE_NOTE_OCTAVE_UP(n)		((n)+12 <= BSE_MAX_NOTE && (n)+12 >= BSE_MIN_NOTE ? (n)+12 : (n))
#define	BSE_NOTE_OCTAVE_DOWN(n)		((n) >= BSE_MIN_NOTE+12 && (n)-12 <= BSE_MAX_NOTE ? (n)-12 : (n))


/* --- limits & defaults --- */
#define	BSE_MIN_VOLUME_dB		(-40) /* theoretically: -48.165 */
#define	BSE_MAX_VOLUME_dB		(+10)
#define	BSE_MIN_BPM			(1)
#define	BSE_MAX_BPM			(1024)
#define	BSE_MIN_PATTERN_LENGTH		(4)
#define	BSE_MAX_PATTERN_LENGTH		(256)
#define	BSE_MAX_N_CHANNELS		(256)
#define	BSE_MAX_N_ROWS			(BSE_MAX_PATTERN_LENGTH)
#define	BSE_MIN_BALANCE			(- BSE_MAX_BALANCE)
#define	BSE_MAX_BALANCE			(50.0)
#define	BSE_MIN_TRANSPOSE		(-12)
#define	BSE_MAX_TRANSPOSE		(12)	/* 1 octave */
#define	BSE_MIN_FINE_TUNE		(- BSE_MAX_FINE_TUNE)
#define	BSE_MAX_FINE_TUNE		(6)	/* inter-note divisor */
#define	BSE_MAX_ENV_TIME		(10000) /* ms */
#define	BSE_MIN_MIX_FREQ		(1378)
#define	BSE_MAX_MIX_FREQ		(96000)
#define BSE_MIN_TIME			(631148400)	/* 1990-01-01 00:00:00 */
#define	BSE_MAX_TIME			(2147483647)	/* 2038-01-19 04:14:07 */
#define	BSE_MAX_SAMPLE_CHANNELS		(2)
#define	BSE_MAX_SAMPLE_MUNKS		(BSE_MAX_NOTE + 1)
#define	BSE_MIN_BIT_SIZE		(8)
#define	BSE_MAX_BIT_SIZE		(16)
#define	BSE_MIN_N_VALUES		(1)
#define	BSE_MAX_N_VALUES		(128 * 1024 * 1024)
#define	BSE_MAX_SEQ_ID			(65535)
#define	BSE_BBUFFER_SIZE		(128)

/* --- BseSource & BseMaster limits --- */
#define	BSE_MAX_HISTORY			(128)
#define	BSE_MAX_N_ICHANNELS		(128)
#define	BSE_MAX_N_OCHANNELS		(128)
#define	BSE_MAX_N_TRACKS		(8)


/* --- hard defaults (used for saving) --- */
#define	BSE_DFL_SONG_VOLUME_dB		(0)
#define	BSE_DFL_SONG_BPM		(160)
#define	BSE_DFL_SONG_N_CHANNELS		(16)
#define	BSE_DFL_SONG_PATTERN_LENGTH	(64)
#define	BSE_DFL_INSTRUMENT_VOLUME_dB	(- 0.969)
#define BSE_DFL_INSTRUMENT_BALANCE	(0)
#define BSE_DFL_INSTRUMENT_TRANSPOSE	(0)
#define BSE_DFL_INSTRUMENT_FINE_TUNE	(0)
#define BSE_DFL_BIN_DATA_BITS		(16)
#define BSE_DFL_SAMPLE_REC_FREQ		(44100)
#define BSE_DFL_SAMPLE_REC_NOTE		(BSE_KAMMER_NOTE)


/* --- miscellaneous --- */
#define BSE_MAGIC                       (('B' << 24) | ('S' << 16) | \
	    /* 1112753441 0x42534521 */	 ('E' <<  8) | ('!' <<  0))
#define BSE_FADE_OUT_TIME_ms            (30)
#define BSE_VOICES_POLY_OVER_FIXED	(1)


/* --- BseGlobals - configurable defaults --- */
#define	BSE_STP_VOLUME_dB		(bse_globals->step_volume_dB)
#define	BSE_STP_BPM			(bse_globals->step_bpm)
#define	BSE_STP_N_CHANNELS		(bse_globals->step_n_channels)
#define	BSE_STP_PATTERN_LENGTH		(bse_globals->step_pattern_length)
#define	BSE_STP_BALANCE			(bse_globals->step_balance)
#define	BSE_STP_TRANSPOSE		(bse_globals->step_transpose)
#define	BSE_STP_FINE_TUNE		(bse_globals->step_fine_tune)
#define	BSE_STP_ENV_TIME		(bse_globals->step_env_time)
#define	BSE_TRACK_LENGTH		(bse_globals->track_length)
#define	BSE_MIX_FREQ			(bse_globals->mixing_frequency)


/* --- FIXME --- */
#define	BSE_VALUE_BLOCK_N_PAD_VALUES	(BSE_MAX_N_MIXER_VALUES * BSE_MAX_N_MIXER_CHANNELS)
#define	BSE_DFL_LOAD_BUFFER_SIZE	(65536)


/* halftone factorization tables, i.e.
 * Index                     Result
 * (BSE_KAMMER_NOTE - 12) -> 0.5
 * BSE_KAMMER_NOTE	  -> 1.0
 * (BSE_KAMMER_NOTE + 12) -> 2.0
 * etc...
 */
extern const gdouble* _bse_halftone_factor_table;
extern const guint*   _bse_halftone_factor_table_fixed;
#define	BSE_HALFTONE_FACTOR(ht)		((ht) > BSE_MAX_NOTE ? \
				         _bse_halftone_factor_table[BSE_MAX_NOTE] : \
				         (ht) < BSE_MIN_NOTE ? \
				         _bse_halftone_factor_table[BSE_MIN_NOTE] : \
				         _bse_halftone_factor_table[(ht)])
#define	BSE_HALFTONE_FACTOR_FIXED(ht)	((ht) > BSE_MAX_NOTE ? \
				         _bse_halftone_factor_table_fixed[BSE_MAX_NOTE] : \
				         (ht) < BSE_MIN_NOTE ? \
				         _bse_halftone_factor_table_fixed[BSE_MIN_NOTE] : \
				         _bse_halftone_factor_table_fixed[(ht)])
extern const gdouble* _bse_fine_tune_factor_table;
#define	BSE_FINE_TUNE_FACTOR(ft)	((ft) > BSE_MAX_FINE_TUNE ? \
                                         _bse_fine_tune_factor_table[BSE_MAX_FINE_TUNE] : \
                                         (ft) < BSE_MIN_FINE_TUNE ? \
                                         _bse_fine_tune_factor_table[BSE_MIN_FINE_TUNE] : \
                                         _bse_fine_tune_factor_table[(ft)])


/* --- BseGlobals --- */
struct _BseGlobals
{
  /* stepping rates
   */
  gfloat step_volume_dB;
  guint	 step_bpm;
  guint	 step_n_channels;
  guint	 step_pattern_length;
  guint	 step_balance;
  guint	 step_transpose;
  guint	 step_fine_tune;
  guint	 step_env_time;
  
  /* initial default values for PCM devices */
  guint pcm_buffer_size; /* FIXME */
  
  /* BSE pool parameters
   */
  guint	track_length;
  guint mixing_frequency;
};
extern const BseGlobals	*bse_globals;


/* --- version numbers --- */
extern	const guint	 bse_major_version;
extern	const guint	 bse_minor_version;
extern	const guint	 bse_micro_version;
extern	const guint	 bse_interface_age;
extern	const guint	 bse_binary_age;
extern	const gchar	*bse_version;


/* --- prototypes --- */
gchar*		bse_check_version	(guint	required_major,
					 guint	required_minor,
					 guint	required_micro);
void		bse_globals_init	(void);
void		bse_globals_lock	(void);
void		bse_globals_unlock	(void);
gboolean	bse_globals_locked	(void);

/* conversion */
gdouble		bse_dB_to_factor	(gfloat		dB);
gfloat		bse_dB_from_factor	(gdouble	factor,
					 gfloat		min_dB);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_GLOBALS_H__ */
