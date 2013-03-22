// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_WAVE_OSC_H__
#define __BSE_WAVE_OSC_H__
#include <bse/bsesource.hh>
#include <bse/bsewave.hh>
#include <bse/gslwaveosc.hh>
G_BEGIN_DECLS
/* --- object type macros --- */
#define BSE_TYPE_WAVE_OSC              (BSE_TYPE_ID (BseWaveOsc))
#define BSE_WAVE_OSC(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_WAVE_OSC, BseWaveOsc))
#define BSE_WAVE_OSC_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_WAVE_OSC, BseWaveOscClass))
#define BSE_IS_WAVE_OSC(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_WAVE_OSC))
#define BSE_IS_WAVE_OSC_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_WAVE_OSC))
#define BSE_WAVE_OSC_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_WAVE_OSC, BseWaveOscClass))

struct BseWaveOsc : BseSource {
  BseWave           *wave;
  GslWaveChunk      *esample_wchunk;
  GslWaveOscConfig   config;
  gfloat             fm_strength;
  gfloat             n_octaves;
};
struct BseWaveOscClass : BseSourceClass
{};

void    bse_wave_osc_request_pcm_position       (BseWaveOsc        *self);
void    bse_wave_osc_mass_seek                  (guint              n_woscs,
                                                 BseWaveOsc       **woscs,
                                                 gfloat             perc);
void    bse_wave_osc_set_from_esample           (BseWaveOsc        *self,
                                                 BseEditableSample *esample);
/* --- channels --- */
enum
{
  BSE_WAVE_OSC_ICHANNEL_FREQ,
  BSE_WAVE_OSC_ICHANNEL_SYNC,
  BSE_WAVE_OSC_ICHANNEL_MOD,
  BSE_WAVE_OSC_N_ICHANNELS
};
enum
{
  BSE_WAVE_OSC_OCHANNEL_WAVE,
  BSE_WAVE_OSC_OCHANNEL_GATE,
  BSE_WAVE_OSC_OCHANNEL_DONE,
  BSE_WAVE_OSC_N_OCHANNELS
};
G_END_DECLS
#endif /* __BSE_WAVE_OSC_H__ */
