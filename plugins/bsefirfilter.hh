// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_FIR_FILTER_H__
#define __BSE_FIR_FILTER_H__

#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





/* --- object type macros --- */
#define BSE_TYPE_FIR_FILTER              (type_id_fir_filter)
#define BSE_FIR_FILTER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_FIR_FILTER, BseFIRFilter))
#define BSE_FIR_FILTER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_FIR_FILTER, BseFIRFilterClass))
#define BSE_IS_FIR_FILTER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_FIR_FILTER))
#define BSE_IS_FIR_FILTER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_FIR_FILTER))
#define BSE_FIR_FILTER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_FIR_FILTER, BseFIRFilterClass))


/* --- BseFIRFilter source --- */
typedef struct _BseFIRFilter      BseFIRFilter;
typedef struct _BseFIRFilterClass BseFIRFilterClass;
struct _BseFIRFilter
{
  BseSource       parent_object;

  guint16 degree;
  guint   filter_type : 8;
  guint   lanczos_smoothing : 1;
  guint   hann_smoothing : 1;
  gfloat  cut_off_freq;

  guint           n_coeffs;
  gfloat         *coeffs;
  guint           history_pos;
  BseSampleValue *history;
};
struct _BseFIRFilterClass
{
  BseSourceClass parent_class;
};


/* --- enums --- */
typedef enum
{
  BSE_FIR_FILTER_ALLPASS,
  BSE_FIR_FILTER_LOWPASS,
  BSE_FIR_FILTER_HIGHPASS
} BseFIRFilterType;


/* --- channels --- */
enum
{
  BSE_FIR_FILTER_OCHANNEL_NONE,
  BSE_FIR_FILTER_OCHANNEL_MONO
};
enum
{
  BSE_FIR_FILTER_ICHANNEL_NONE,
  BSE_FIR_FILTER_ICHANNEL_MONO
};




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_FIR_FILTER_H__ */
