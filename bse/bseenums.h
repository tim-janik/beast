/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
 *
 * bseenums.h: main portion of enum definitions in BSE and convenience functions
 */
#ifndef __BSE_ENUMS_H__
#define __BSE_ENUMS_H__

#include	<bse/bsetype.h>
#include	<gsl/gsldefs.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- enum definitions --- */
typedef enum
{
  BSE_LITTLE_ENDIAN	= G_LITTLE_ENDIAN	/* 1234 */,
  BSE_BIG_ENDIAN	= G_BIG_ENDIAN		/* 4321 */
} BseEndianType;
#define	BSE_BYTE_ORDER	G_BYTE_ORDER
typedef enum
{
  BSE_INTERPOL_NONE,		/*< nick=None >*/
  BSE_INTERPOL_LINEAR,		/*< nick=Linear >*/
  BSE_INTERPOL_CUBIC		/*< nick=Cubic >*/
} BseInterpolType;
typedef enum
{
  BSE_LOOP_NONE,
  BSE_LOOP_PATTERN,
  BSE_LOOP_PATTERN_ROWS,
  BSE_LOOP_SONG,
  BSE_LOOP_LAST				/*< skip >*/
} BseLoopType;
typedef enum
{
  BSE_MAGIC_BSE_BIN_EXTENSION   = 1 << 0,
  BSE_MAGIC_BSE_SAMPLE          = 1 << 1,
  BSE_MAGIC_BSE_SONG            = 1 << 2
} BseMagicFlags;
typedef enum
{
  /* GSL errors mirrored into BSE */
  BSE_ERROR_NONE		= GSL_ERROR_NONE,	/* 0 */
  BSE_ERROR_INTERNAL		= GSL_ERROR_INTERNAL,
  BSE_ERROR_UNKNOWN		= GSL_ERROR_UNKNOWN,
  /* I/O errors */
  BSE_ERROR_IO			= GSL_ERROR_IO,
  BSE_ERROR_EOF			= GSL_ERROR_EOF,
  BSE_ERROR_NOT_FOUND		= GSL_ERROR_NOT_FOUND,
  BSE_ERROR_OPEN_FAILED		= GSL_ERROR_OPEN_FAILED,
  BSE_ERROR_SEEK_FAILED		= GSL_ERROR_SEEK_FAILED,
  BSE_ERROR_READ_FAILED		= GSL_ERROR_READ_FAILED,
  BSE_ERROR_WRITE_FAILED	= GSL_ERROR_WRITE_FAILED,
  /* content errors */
  BSE_ERROR_FORMAT_INVALID	= GSL_ERROR_FORMAT_INVALID,
  BSE_ERROR_FORMAT_UNKNOWN	= GSL_ERROR_FORMAT_UNKNOWN,
  BSE_ERROR_DATA_CORRUPT	= GSL_ERROR_DATA_CORRUPT,
  /* miscellaneous errors */
  BSE_ERROR_CODEC_FAILURE	= GSL_ERROR_CODEC_FAILURE,

  /* BSE errors */
  BSE_ERROR_UNIMPLEMENTED	= GSL_ERROR_LAST,
  BSE_ERROR_PERMS,
  BSE_ERROR_NOT_OWNER,
  /* File, Loading/Saving errors */
#define BSE_ERROR_FILE_IO		BSE_ERROR_IO
#define BSE_ERROR_FILE_NOT_FOUND	BSE_ERROR_NOT_FOUND
  BSE_ERROR_FILE_EXISTS,
  BSE_ERROR_FILE_TOO_SHORT,
  BSE_ERROR_FILE_TOO_LONG,
#define BSE_ERROR_FORMAT_MISMATCH	BSE_ERROR_FORMAT_INVALID
#define	BSE_ERROR_FORMAT_TOO_NEW	BSE_ERROR_FORMAT_UNKNOWN
#define	BSE_ERROR_FORMAT_TOO_OLD	BSE_ERROR_FORMAT_UNKNOWN
  BSE_ERROR_HEADER_CORRUPT,
  BSE_ERROR_SUB_HEADER_CORRUPT,
  BSE_ERROR_BINARY_DATA_CORRUPT,
  BSE_ERROR_PARSE_ERROR,
  /* Device errors */
#define BSE_ERROR_DEVICE_IO	BSE_ERROR_IO
#define BSE_ERROR_DEVICE_PERMS	BSE_ERROR_PERMS
  BSE_ERROR_DEVICE_ASYNC,
  BSE_ERROR_DEVICE_BUSY,
  BSE_ERROR_DEVICE_GET_CAPS,
  BSE_ERROR_DEVICE_CAPS_MISMATCH,
  BSE_ERROR_DEVICE_SET_CAPS,
  /* Date parsing errors */
  BSE_ERROR_DATE_INVALID,
  BSE_ERROR_DATE_CLUTTERED,
  BSE_ERROR_DATE_YEAR_BOUNDS,
  BSE_ERROR_DATE_MONTH_BOUNDS,
  BSE_ERROR_DATE_DAY_BOUNDS,
  BSE_ERROR_DATE_HOUR_BOUNDS,
  BSE_ERROR_DATE_MINUTE_BOUNDS,
  BSE_ERROR_DATE_SECOND_BOUNDS,
  BSE_ERROR_DATE_TOO_OLD,
  /* BseSource errors */
  BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL,
  BSE_ERROR_SOURCE_NO_SUCH_OCHANNEL,
  BSE_ERROR_SOURCE_NO_SUCH_CONNECTION,
  BSE_ERROR_SOURCE_ICHANNEL_IN_USE,
  BSE_ERROR_SOURCE_CHANNELS_CONNECTED,
  BSE_ERROR_SOURCE_BAD_LOOPBACK,
  BSE_ERROR_SOURCE_BUSY,
  BSE_ERROR_SOURCE_TYPE_INVALID,
  /* BseProcedure errors */
  BSE_ERROR_PROC_BUSY,
  BSE_ERROR_PROC_PARAM_INVAL,
  BSE_ERROR_PROC_EXECUTION,
  BSE_ERROR_PROC_ABORT,
  /* BseServer errors */
  BSE_ERROR_NO_PCM_DEVICE,
  BSE_ERROR_PCM_DEVICE_ACTIVE,
  BSE_ERROR_LAST			/*< skip >*/
} BseErrorType;
/* --- MIDI signals --- */
typedef enum
{
  BSE_MIDI_SIGNAL_CONTROL,      /* + 0..127 */
  BSE_MIDI_SIGNAL_PROGRAM       = 128,
  BSE_MIDI_SIGNAL_PRESSURE,
  BSE_MIDI_SIGNAL_PITCH_BEND,
  BSE_MIDI_SIGNAL_FREQUENCY,    /* of note */
  BSE_MIDI_SIGNAL_GATE,         /* of note */
  BSE_MIDI_SIGNAL_VELOCITY,     /* of note */
  BSE_MIDI_SIGNAL_AFTERTOUCH,   /* of note */
  BSE_MIDI_SIGNAL_LAST
} BseMidiSignalType;


/* --- convenience functions --- */
gchar*		bse_error_name			(BseErrorType	 error_value);
gchar*		bse_error_nick			(BseErrorType	 error_value);
gchar*		bse_error_blurb			(BseErrorType	 error_value);
BseErrorType	bse_error_from_errno		(gint		 v_errno,
						 BseErrorType    fallback);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_ENUMS_H__ */
