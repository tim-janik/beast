// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ENUMS_H__
#define __BSE_ENUMS_H__

#include <bse/bsetype.hh>
#include <bse/gsldefs.hh>


G_BEGIN_DECLS


/* --- enum definitions --- */
typedef enum
{
  BSE_IIR_FILTER_BUTTERWORTH = 1,
  BSE_IIR_FILTER_CHEBYCHEFF1,
  BSE_IIR_FILTER_CHEBYCHEFF2
} BseIIRFilterAlgorithm;
typedef enum
{
  BSE_IIR_FILTER_LOW_PASS = 1,
  BSE_IIR_FILTER_HIGH_PASS,
  BSE_IIR_FILTER_BAND_PASS,
  BSE_IIR_FILTER_BAND_STOP
} BseIIRFilterType;
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
  BSE_ERROR_NONE		= 0,
  BSE_ERROR_INTERNAL,
  BSE_ERROR_UNKNOWN,
  /* general errors */
  BSE_ERROR_IO,
  BSE_ERROR_PERMS,
  /* file errors */
  BSE_ERROR_FILE_BUSY,
  BSE_ERROR_FILE_EXISTS,
  BSE_ERROR_FILE_EOF,
  BSE_ERROR_FILE_EMPTY,
  BSE_ERROR_FILE_NOT_FOUND,
  BSE_ERROR_FILE_IS_DIR,
  BSE_ERROR_FILE_OPEN_FAILED,
  BSE_ERROR_FILE_SEEK_FAILED,
  BSE_ERROR_FILE_READ_FAILED,
  BSE_ERROR_FILE_WRITE_FAILED,
  /* out of resource conditions */
  BSE_ERROR_MANY_FILES,
  BSE_ERROR_NO_FILES,
  BSE_ERROR_NO_SPACE,
  BSE_ERROR_NO_MEMORY,
  /* content errors */
  BSE_ERROR_NO_HEADER,
  BSE_ERROR_NO_SEEK_INFO,
  BSE_ERROR_NO_DATA,
  BSE_ERROR_DATA_CORRUPT,
  BSE_ERROR_WRONG_N_CHANNELS,
  BSE_ERROR_FORMAT_INVALID,
  BSE_ERROR_FORMAT_UNKNOWN,
  BSE_ERROR_DATA_UNMATCHED,
  /* miscellaneous errors */
  BSE_ERROR_TEMP,
  BSE_ERROR_WAVE_NOT_FOUND,
  BSE_ERROR_CODEC_FAILURE,
  BSE_ERROR_UNIMPLEMENTED,
  BSE_ERROR_INVALID_PROPERTY,
  BSE_ERROR_INVALID_MIDI_CONTROL,
  BSE_ERROR_PARSE_ERROR,
  BSE_ERROR_SPAWN,
  /* Device errors */
  BSE_ERROR_DEVICE_NOT_AVAILABLE,
  BSE_ERROR_DEVICE_ASYNC,
  BSE_ERROR_DEVICE_BUSY,
  BSE_ERROR_DEVICE_FORMAT,
  BSE_ERROR_DEVICE_BUFFER,
  BSE_ERROR_DEVICE_LATENCY,
  BSE_ERROR_DEVICE_CHANNELS,
  BSE_ERROR_DEVICE_FREQUENCY,
  BSE_ERROR_DEVICES_MISMATCH,
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
