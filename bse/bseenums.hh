// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_ENUMS_H__
#define __BSE_ENUMS_H__

#include <bse/gsldefs.hh>
#include <bse/bsetype.hh>
#include <bse/bseserverapi.hh>


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

#ifdef  BSE_IDL_PSEUDOS
typedef enum
{
  BSE_ERROR_NONE = Bse::ERROR_NONE,
} BseErrorType;
#endif // BSE_IDL_PSEUDOS


/* --- convenience functions --- */
const gchar*	bse_error_blurb			(Bse::ErrorType	 error_value);
Bse::ErrorType	bse_error_from_errno		(gint v_errno, Bse::ErrorType fallback);

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
