/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2003 Tim Janik
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

#include <bse/bsetype.h>
#include <bse/gsldefs.h>


G_BEGIN_DECLS


/* --- enum definitions --- */
typedef enum
{
  BSE_REGISTER_PLUGIN   = 1,
  BSE_REGISTER_SCRIPT   = 2,
  BSE_REGISTER_DONE	= 256
} BseRegistrationType;
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
  BSE_MAGIC_BSE_SONG            = 1 << 2
} BseMagicFlags;
typedef enum
{
  BSE_USER_MSG_UNSPECIFIED,
  BSE_USER_MSG_DEBUG,
  BSE_USER_MSG_DIAG,
  BSE_USER_MSG_INFO,
  BSE_USER_MSG_WARNING,
  BSE_USER_MSG_ERROR
} BseUserMsgType;
typedef enum
{
  /* GSL errors are mirrored into BSE */
  BSE_ERROR_NONE		= GSL_ERROR_NONE,	/* 0 */
  BSE_ERROR_INTERNAL		= GSL_ERROR_INTERNAL,
  BSE_ERROR_UNKNOWN		= GSL_ERROR_UNKNOWN,
  /* file errors */
  BSE_ERROR_FILE_IO		= GSL_ERROR_IO,
  BSE_ERROR_FILE_PERMS		= GSL_ERROR_PERMS,
  BSE_ERROR_FILE_BUSY		= GSL_ERROR_BUSY,
  BSE_ERROR_FILE_EXISTS		= GSL_ERROR_EXISTS,
  BSE_ERROR_FILE_EOF		= GSL_ERROR_EOF,
  BSE_ERROR_FILE_NOT_FOUND	= GSL_ERROR_NOT_FOUND,
  BSE_ERROR_FILE_IS_DIR 	= GSL_ERROR_IS_DIR,
  BSE_ERROR_FILE_OPEN_FAILED	= GSL_ERROR_OPEN_FAILED,
  BSE_ERROR_FILE_SEEK_FAILED	= GSL_ERROR_SEEK_FAILED,
  BSE_ERROR_FILE_READ_FAILED	= GSL_ERROR_READ_FAILED,
  BSE_ERROR_FILE_WRITE_FAILED	= GSL_ERROR_WRITE_FAILED,
  /* out of resource conditions */
  BSE_ERROR_MANY_FILES		= GSL_ERROR_MANY_FILES,
  BSE_ERROR_NO_FILES		= GSL_ERROR_NO_FILES,
  BSE_ERROR_NO_SPACE		= GSL_ERROR_NO_SPACE,
  BSE_ERROR_NO_MEMORY		= GSL_ERROR_NO_MEMORY,
  /* content errors */
  BSE_ERROR_NO_HEADER		= GSL_ERROR_NO_HEADER,
  BSE_ERROR_NO_SEEK_INFO	= GSL_ERROR_NO_SEEK_INFO,
  BSE_ERROR_NO_DATA		= GSL_ERROR_NO_DATA,
  BSE_ERROR_DATA_CORRUPT	= GSL_ERROR_DATA_CORRUPT,
  BSE_ERROR_FORMAT_INVALID	= GSL_ERROR_FORMAT_INVALID,
  BSE_ERROR_FORMAT_UNKNOWN	= GSL_ERROR_FORMAT_UNKNOWN,
  /* miscellaneous errors */
  BSE_ERROR_TEMP		= GSL_ERROR_TEMP,
  BSE_ERROR_WAVE_NOT_FOUND	= GSL_ERROR_WAVE_NOT_FOUND,
  BSE_ERROR_CODEC_FAILURE	= GSL_ERROR_CODEC_FAILURE,
  /* BSE errors */
  BSE_ERROR_UNIMPLEMENTED	= GSL_ERROR_LAST,
  /* Device errors */
  BSE_ERROR_DEVICE_NOT_AVAILABLE,
  BSE_ERROR_DEVICE_ASYNC,
  BSE_ERROR_DEVICE_BUSY,
  BSE_ERROR_DEVICE_GET_CAPS,
  BSE_ERROR_DEVICE_CAPS_MISMATCH,
  BSE_ERROR_DEVICE_SET_CAPS,
  /* BseSource errors */
  BSE_ERROR_SOURCE_NO_SUCH_MODULE,
  BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL,
  BSE_ERROR_SOURCE_NO_SUCH_OCHANNEL,
  BSE_ERROR_SOURCE_NO_SUCH_CONNECTION,
  BSE_ERROR_SOURCE_PRIVATE_ICHANNEL,
  BSE_ERROR_SOURCE_ICHANNEL_IN_USE,
  BSE_ERROR_SOURCE_CHANNELS_CONNECTED,
  BSE_ERROR_SOURCE_CONNECTION_INVALID,
  BSE_ERROR_SOURCE_PARENT_MISMATCH,
  BSE_ERROR_SOURCE_BAD_LOOPBACK,
  BSE_ERROR_SOURCE_BUSY,
  BSE_ERROR_SOURCE_TYPE_INVALID,
  /* BseProcedure errors */
  BSE_ERROR_PROC_NOT_FOUND,
  BSE_ERROR_PROC_BUSY,
  BSE_ERROR_PROC_PARAM_INVAL,
  BSE_ERROR_PROC_EXECUTION,
  BSE_ERROR_PROC_ABORT,
  /* miscellaneous errors */
  BSE_ERROR_PARSE_ERROR,
  BSE_ERROR_SPAWN,
  /* various procedure errors */
  BSE_ERROR_NO_ENTRY,
  BSE_ERROR_NO_EVENT,
  BSE_ERROR_NO_TARGET,
  BSE_ERROR_NOT_OWNER,
  BSE_ERROR_INVALID_OFFSET,
  BSE_ERROR_INVALID_DURATION,
  BSE_ERROR_INVALID_OVERLAP,
} BseErrorType;


/* --- convenience functions --- */
const gchar*	bse_error_name			(BseErrorType	 error_value);
const gchar*	bse_error_nick			(BseErrorType	 error_value);
const gchar*	bse_error_blurb			(BseErrorType	 error_value);
BseErrorType	bse_error_from_errno		(gint		 v_errno,
						 BseErrorType    fallback);

#define bse_assert_ok(error)    G_STMT_START{                           \
     if G_UNLIKELY (error)                                              \
       {                                                                \
         g_log (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,                        \
                "%s:%d: unexpected error: %s",                          \
                __FILE__, __LINE__, bse_error_blurb (error));           \
       }                                                                \
}G_STMT_END

G_END_DECLS


#endif /* __BSE_ENUMS_H__ */
