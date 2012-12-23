// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_BIQUAD_FILTER_H__
#define __BSE_BIQUAD_FILTER_H__

#include <bse/bsesource.hh>
#include <bse/gslfilter.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BSE_TYPE_BIQUAD_FILTER              (BSE_TYPE_ID (BseBiquadFilter))
#define BSE_BIQUAD_FILTER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_BIQUAD_FILTER, BseBiquadFilter))
#define BSE_BIQUAD_FILTER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_BIQUAD_FILTER, BseBiquadFilterClass))
#define BSE_IS_BIQUAD_FILTER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_BIQUAD_FILTER))
#define BSE_IS_BIQUAD_FILTER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_BIQUAD_FILTER))
#define BSE_BIQUAD_FILTER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_BIQUAD_FILTER, BseBiquadFilterClass))


/* --- enums --- */
typedef enum
{
  BSE_BIQUAD_FILTER_RESONANT_LOWPASS  = GSL_BIQUAD_RESONANT_LOWPASS,
  BSE_BIQUAD_FILTER_RESONANT_HIGHPASS = GSL_BIQUAD_RESONANT_HIGHPASS,
} BseBiquadFilterType;
typedef enum    /* skip */
{
  BSE_BIQUAD_FILTER_NORM_PASSBAND	= GSL_BIQUAD_NORMALIZE_PASSBAND,
  BSE_BIQUAD_FILTER_NORM_RESONANCE_GAIN	= GSL_BIQUAD_NORMALIZE_RESONANCE_GAIN,
  BSE_BIQUAD_FILTER_NORM_PEAK_GAIN	= GSL_BIQUAD_NORMALIZE_PEAK_GAIN
} BseBiquadFilterNorm;


/* --- BseBiquadFilter source --- */
typedef struct _BseBiquadFilter      BseBiquadFilter;
typedef struct _BseBiquadFilterClass BseBiquadFilterClass;
struct _BseBiquadFilter
{
  BseSource	parent_object;

  BseBiquadFilterType filter_type;
  guint		      type_change : 1;
  guint		      exponential_fm : 1;
  gfloat	      freq;
  gfloat	      fm_strength;
  gfloat	      fm_n_octaves;
  BseBiquadFilterNorm norm_type;
  gfloat	      gain;
  gfloat	      gain_strength;
};
struct _BseBiquadFilterClass
{
  BseSourceClass parent_class;
};


/* --- channels --- */
enum
{
  BSE_BIQUAD_FILTER_ICHANNEL_AUDIO,
  BSE_BIQUAD_FILTER_ICHANNEL_FREQ,
  BSE_BIQUAD_FILTER_ICHANNEL_FREQ_MOD,
  BSE_BIQUAD_FILTER_ICHANNEL_GAIN_MOD,
  BSE_BIQUAD_FILTER_N_ICHANNELS
};
enum
{
  BSE_BIQUAD_FILTER_OCHANNEL_AUDIO,
  BSE_BIQUAD_FILTER_N_OCHANNELS
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_BIQUAD_FILTER_H__ */
