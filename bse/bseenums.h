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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BSE_ENUM_CLASS(class)     (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_ENUM, BseEnumClass))
#define BSE_IS_ENUM_CLASS(class)  (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_ENUM))
#define BSE_FLAGS_CLASS(class)    (BSE_CHECK_CLASS_CAST ((class), BSE_TYPE_FLAGS, BseFlagsClass))
#define BSE_IS_FLAGS_CLASS(class) (BSE_CHECK_CLASS_TYPE ((class), BSE_TYPE_FLAGS))


/* --- enum/flag values & classes --- */
struct	_BseEnumClass
{
  BseTypeClass	bse_class;
  
  gint		 minimum;
  gint		 maximum;
  guint		 n_values;
  BseEnumValue	*values;
};
struct	_BseFlagsClass
{
  BseTypeClass	bse_class;
  
  guint		 mask;
  guint		 n_values;
  BseFlagsValue	*values;
};
struct _BseEnumValue
{
  gint	 value;
  gchar *value_name;
  gchar *value_nick;
};
struct _BseFlagsValue
{
  guint  value;
  gchar *value_name;
  gchar *value_nick;
};


/* --- enum definitions --- */
typedef enum
{
  BSE_LITTLE_ENDIAN	= G_LITTLE_ENDIAN	/* 1234 */,
  BSE_BIG_ENDIAN	= G_BIG_ENDIAN		/* 4321 */
} BseEndianType;
#define	BSE_BYTE_ORDER	G_BYTE_ORDER
typedef enum
{
  BSE_LOOP_NONE,
  BSE_LOOP_PATTERN,
  BSE_LOOP_PATTERN_ROWS,
  BSE_LOOP_SONG,
  BSE_LOOP_LAST				/* <skip> */
} BseLoopType;
typedef enum
{
  BSE_INSTRUMENT_NONE,
  BSE_INSTRUMENT_SAMPLE,
  BSE_INSTRUMENT_MIDI,
  BSE_INSTRUMENT_LAST			/* <skip> */
} BseInstrumentType;
typedef enum
{
  BSE_ERROR_NONE,
  BSE_ERROR_IGNORE,
  BSE_ERROR_UNKNOWN,
  BSE_ERROR_INTERNAL,
  BSE_ERROR_UNIMPLEMENTED,
  BSE_ERROR_IO,
  BSE_ERROR_PERMS,
  /* FIle, Loading/Saving errors */
#define BSE_ERROR_FILE_IO	BSE_ERROR_IO
  BSE_ERROR_FILE_EXISTS,
  BSE_ERROR_FILE_NOT_FOUND,
  BSE_ERROR_FILE_TOO_SHORT,
  BSE_ERROR_FILE_TOO_LONG,
  BSE_ERROR_FORMAT_UNSUPPORTED,
  BSE_ERROR_FORMAT_TOO_NEW,
  BSE_ERROR_FORMAT_TOO_OLD,
  BSE_ERROR_HEADER_CORRUPT,
  BSE_ERROR_SUB_HEADER_CORRUPT,
  BSE_ERROR_DATA_CORRUPT,
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
  BSE_ERROR_SOURCE_BAD_LOOPBACK,
  BSE_ERROR_SOURCE_ICHANNEL_IN_USE,
  BSE_ERROR_SOURCE_TOO_MANY_ITRACKS,
  BSE_ERROR_SOURCE_TOO_MANY_OTRACKS,
  /* BseProcedure errors */
  BSE_ERROR_PROC_BUSY,
  BSE_ERROR_PROC_PARAM_INVAL,
  BSE_ERROR_PROC_EXECUTION,
  BSE_ERROR_PROC_NEEDLESS,
  BSE_ERROR_PROC_ABORT,
  BSE_ERROR_LAST			/* <skip> */
} BseErrorType;


/* --- prototypes --- */
BseEnumValue*	bse_enum_get_value		(BseEnumClass	*enum_class,
						 gint		 value);
BseEnumValue*	bse_enum_get_value_by_name	(BseEnumClass	*enum_class,
						 const gchar	*name);
BseEnumValue*	bse_enum_get_value_by_nick	(BseEnumClass	*enum_class,
						 const gchar	*nick);
BseFlagsValue*	bse_flags_get_first_value	(BseFlagsClass	*flags_class,
						 guint		 value);
BseFlagsValue*	bse_flags_get_value_by_name	(BseFlagsClass	*flags_class,
						 const gchar	*name);
BseFlagsValue*	bse_flags_get_value_by_nick	(BseFlagsClass	*flags_class,
						 const gchar	*nick);


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
