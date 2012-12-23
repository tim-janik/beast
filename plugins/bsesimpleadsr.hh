// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SIMPLE_ADSR_H__
#define __BSE_SIMPLE_ADSR_H__
#include <bse/bseplugin.hh>
#include <bse/bsesource.hh>
G_BEGIN_DECLS
/* --- object type macros --- */
#define BSE_TYPE_SIMPLE_ADSR              (bse_simple_adsr_get_type())
#define BSE_SIMPLE_ADSR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SIMPLE_ADSR, BseSimpleADSR))
#define BSE_SIMPLE_ADSR_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SIMPLE_ADSR, BseSimpleADSRClass))
#define BSE_IS_SIMPLE_ADSR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SIMPLE_ADSR))
#define BSE_IS_SIMPLE_ADSR_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SIMPLE_ADSR))
#define BSE_SIMPLE_ADSR_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SIMPLE_ADSR, BseSimpleADSRClass))
/* --- BseSimpleADSR source --- */
typedef struct _BseSimpleADSR      BseSimpleADSR;
typedef struct _BseSimpleADSRClass BseSimpleADSRClass;
typedef struct {
  gfloat	  attack_level;
  gfloat	  attack_inc;
  gfloat	  decay_dec;
  gfloat	  sustain_level;
  gfloat	  release_dec;
} BseSimpleADSRVars;
struct _BseSimpleADSR
{
  BseSource         parent_object;
  gfloat	    attack_time;
  gfloat	    decay_time;
  gfloat	    sustain_level;
  gfloat	    release_time;
  BseTimeRangeType  time_range;
};
struct _BseSimpleADSRClass
{
  BseSourceClass parent_class;
};
/* --- channels --- */
enum
{
  BSE_SIMPLE_ADSR_ICHANNEL_GATE,
  BSE_SIMPLE_ADSR_ICHANNEL_RETRIGGER,
  BSE_SIMPLE_ADSR_N_ICHANNELS
};
enum
{
  BSE_SIMPLE_ADSR_OCHANNEL_OUT,
  BSE_SIMPLE_ADSR_OCHANNEL_DONE,
  BSE_SIMPLE_ADSR_N_OCHANNELS
};
G_END_DECLS
#endif /* __BSE_SIMPLE_ADSR_H__ */
