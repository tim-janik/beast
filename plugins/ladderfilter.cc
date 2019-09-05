// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "ladderfilter.genidl.hh"
#include "laddervcf.hh"

namespace Bse {

class LadderFilter : public LadderFilterBase {
  class Module : public SynthesisModule {
    LadderVCFNonLinear vcf;

    double cutoff    = 0;
    double resonance = 0;
  public:
    void
    reset()
    {
      vcf.reset();
    }
    void
    config (LadderFilterProperties *params)
    {
      cutoff    = params->cutoff / (mix_freq() * 0.5);
      resonance = params->resonance / 100; /* percent */

      LadderVCFMode m = LadderVCFMode::LP4;
      switch (params->filter_type)
      {
        case LADDER_FILTER_LP4: m = LadderVCFMode::LP4; break;
        case LADDER_FILTER_LP3: m = LadderVCFMode::LP3; break;
        case LADDER_FILTER_LP2: m = LadderVCFMode::LP2; break;
        case LADDER_FILTER_LP1: m = LadderVCFMode::LP1; break;
      }
      vcf.set_mode (m);
      vcf.set_drive (params->drive_db);
      vcf.set_rate (mix_freq());
      vcf.set_freq_mod_octaves (params->freq_mod_octaves);
      vcf.set_key_tracking (params->key_tracking / 100);
      vcf.set_resonance_mod (params->resonance_mod / 100);
    }
    const float *
    istream_ptr (uint index)
    {
      const IStream& is = istream (index);
      return is.connected ? is.values : nullptr;
    }
    void
    process (unsigned int n_values)
    {
      const float *inputs[2]  = { istream (ICHANNEL_AUDIO_IN1).values,  istream (ICHANNEL_AUDIO_IN2).values };
      float       *outputs[2] = { ostream (OCHANNEL_AUDIO_OUT1).values, ostream (OCHANNEL_AUDIO_OUT2).values };

      vcf.run_block (n_values, cutoff, resonance, inputs, outputs,
                     ostream (OCHANNEL_AUDIO_OUT1).connected, // need left channel?
                     ostream (OCHANNEL_AUDIO_OUT2).connected, // need right channel?
                     istream_ptr (ICHANNEL_FREQ_IN),
                     istream_ptr (ICHANNEL_FREQ_MOD_IN),
                     istream_ptr (ICHANNEL_KEY_FREQ_IN),
                     istream_ptr (ICHANNEL_RESO_MOD_IN));
    }
  };
  BSE_EFFECT_INTEGRATE_MODULE (LadderFilter, Module, LadderFilterProperties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_ALL_TYPES_FROM_LADDERFILTER_IDL();

}
