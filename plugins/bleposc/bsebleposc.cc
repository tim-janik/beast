// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html

#include "bsebleposc.genidl.hh"
#include <bse/bsemathsignal.hh>
#include <vector>
#include "bleputils.hh"
#include "bleposc.hh"

using namespace std;

namespace Bse {

class BlepOsc : public BlepOscBase {
  /* FIXME: get rid of this as soon as the modules have their own current_musical_tuning() accessor */
  struct Properties : public BlepOscProperties {
    Bse::MusicalTuning current_musical_tuning;
    Properties (BlepOsc *blep_osc) :
      BlepOscProperties (blep_osc),
      current_musical_tuning (blep_osc->current_musical_tuning())
    {}
  };

  /* actual computation */
  class Module : public SynthesisModule {
    OscImpl osc;
    double  frequency;
    double  transpose_factor;
    double  fine_tune;
    double  freq_mod_octaves;
    double  sub;
    double  sub_mod;
    double  sync;
    double  sync_mod;

  public:
    void
    config (Properties *properties)
    {
      frequency         = properties->frequency;
      transpose_factor  = bse_transpose_factor (properties->current_musical_tuning, properties->transpose);
      fine_tune         = properties->fine_tune;
      freq_mod_octaves  = properties->freq_mod_octaves;

      osc.shape_base    = properties->shape / 100;
      osc.shape_mod     = properties->shape_mod / 100;

      sub               = properties->sub / 100;
      sub_mod           = properties->sub_mod / 100;
      sync              = properties->sync;
      sync_mod          = properties->sync_mod;

      osc.pulse_width_base = properties->pulse_width / 100;
      osc.pulse_width_mod  = properties->pulse_width_mod / 100;

      osc.set_unison (properties->unison_voices, properties->unison_detune, properties->unison_stereo / 100);
      osc.rate        = mix_freq();
    }
    void
    reset()
    {
    }
    Module()
    {
    }
    ~Module()
    {
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
      const float *freq_in      = istream (ICHANNEL_FREQ_IN).values;
      const float *freq_mod_in  = istream (ICHANNEL_FREQ_MOD_IN).values;
      const float *sub_mod_in   = istream (ICHANNEL_SUB_MOD_IN).values;
      const float *sync_mod_in  = istream (ICHANNEL_SYNC_MOD_IN).values;
      float *left_out           = ostream (OCHANNEL_LEFT_OUT).values;
      float *right_out          = ostream (OCHANNEL_RIGHT_OUT).values;

      /* master freq */
      double current_freq = frequency * bse_cent_tune_fast (fine_tune);
      if (istream (ICHANNEL_FREQ_IN).connected)
        {
          current_freq = BSE_SIGNAL_TO_FREQ (freq_in[0]) * bse_cent_tune_fast (fine_tune);
        }
      current_freq *= transpose_factor;

      /* freq mod */
      if (istream (ICHANNEL_FREQ_MOD_IN).connected)
        {
          current_freq *= bse_approx5_exp2 (freq_mod_in[0] * freq_mod_octaves);
        }
      osc.master_freq = current_freq;

      /* sub mod */
      double current_sub = sub;
      if (istream (ICHANNEL_SUB_MOD_IN).connected)
        {
          current_sub = CLAMP (sub + sub_mod * sub_mod_in[0], 0.0, 1.0);
        }
      osc.sub = current_sub;

      /* slave freq from sync */
      double current_sync = sync;
      if (istream (ICHANNEL_SYNC_MOD_IN).connected)
        {
          current_sync = CLAMP (sync + sync_mod * sync_mod_in[0], 0.0, 60.0);
        }
      osc.freq = osc.master_freq * bse_approx5_exp2 (current_sync / 12.);

      osc.process_sample_stereo (left_out, right_out, n_values,
                                 istream_ptr (ICHANNEL_SHAPE_MOD_IN),
                                 istream_ptr (ICHANNEL_PULSE_MOD_IN));
    }
  };
public:
  /* implement creation and config methods for synthesis Module */
  BSE_EFFECT_INTEGRATE_MODULE (BlepOsc, Module, Properties);
};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (BlepOsc);

} // Bse
