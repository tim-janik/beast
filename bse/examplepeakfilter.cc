/* BSE - Bedevilled Sound Engine                        -*-mode: c++;-*-
 * Copyright (C) 2003 Tim Janik
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
 */
#include "examplepeakfilter.h"

// allow debugging messages
#define INFO   sfi_info_keyfunc ("peak-filter")

namespace {
using namespace Bse;
using namespace Example;

/* setup basic information about this module, such as the
 * the category (determines position in popup menus) and
 * a general description
 */
const ClassInfo cinfo ("/Modules/Examples/Peak Filter",
                       "Example::PeakFilter is a sample implementation of a peak filter in C++.");

/* register this module with the BSE type system, passing the
 * parent type and class information as extra parameters.
 * (this macro implements PeakFilter.get_type())
 */
BSE_CXX_TYPE_REGISTER (PeakFilter, "BseEffect", &cinfo);

/* Constructor */
PeakFilter::PeakFilter()
{
  INFO ("PeakFilter Constructor: %p", this);
}

/* Destructor */
PeakFilter::~PeakFilter()
{
  INFO ("PeakFilter Destructor: %p", this);
}

/* class initialization, called before PeakFilter() constructor
 * is called. here, we export the properties used by this effect,
 * and we create the required input/output channels.
 */
void
PeakFilter::class_init (CxxBaseClass *klass)
{
  INFO ("PeakFilter class-init: %p", klass);
  /* register properties */
  klass->add ("Peak Settings", PROP_FREQ,
              bse_param_spec_freq_simple ("freq", _("Frequency"), _("Frequency around which the peak is centered"),
                                          SFI_PARAM_DEFAULT "f:dial"));
  klass->add ("Peak Settings", PROP_GAIN,
              sfi_pspec_real ("gain", _("Gain [dB]"), _("Specify the peak level in decibell"),
                              3, 0, +48., 3,
                              SFI_PARAM_DEFAULT ":dial"));
  klass->add ("Filter Settings", PROP_QUALITY,
              sfi_pspec_real ("quality", _("Quality"),
                              _("The quality adjust how much neighbouring frequencies are affected "
                                "by altering the peak frequency"),
                              1, 0.01, 10.0, 1,
                              SFI_PARAM_DEFAULT ":dial"));
  klass->add_ichannel ("Audio In", "Audio input to be filtered", ICHANNEL_MONO);
  klass->add_ochannel ("Audio Out", "Filtered audio output", OCHANNEL_FILTERED);
}

void
PeakFilter::set_property (guint        prop_id,
                          const Value &value,
                          GParamSpec  *pspec)
{
  INFO ("PeakFilter::set_property");
  switch (prop_id)
    {
    case PROP_FREQ:
      freq = value.get_real();
      break;
    case PROP_GAIN:
      gain = value.get_real();
      /* calculate auxillary variables */
      v = pow (10, gain / 20.);         /* v=10^(gain[dB]/20) */
      break;
    case PROP_QUALITY:
      quality = value.get_real();
      break;
    }
  /* propagate changes to all currently running synthesis modules */
  update_modules ();
}

void
PeakFilter::get_property (guint       prop_id,
                          Value      &value,
                          GParamSpec *pspec)
{
  INFO ("PeakFilter::get_property");
  switch (prop_id)
    {
    case PROP_FREQ:
      value = freq;
      break;
    case PROP_GAIN:
      value = gain;
      break;
    case PROP_QUALITY:
      value = quality;
      break;
    }
}

/* synthesis module: */
class PeakFilterModule : public SynthesisModule {
  /* state: */
  double xd1, xd2;    // X delay elements
  double yd1, yd2;    // Y delay elements
  /* coefficients: */
  double a0, a1, a2;  // X coefficients
  double b1, b2;      // Y coefficients
public:
  void
  PeakFilterModule::process (unsigned int n_values)
  {
    /* this function runs in various synthesis threads */
    const float *x = istream (PeakFilter::ICHANNEL_MONO).values;
    float *y = ostream (PeakFilter::OCHANNEL_FILTERED).values;
    float *bound = y + n_values;
    /* Filter structure:                    +-------------------->- Y
     *                                      |
     * X ->-+-|z^-1|-+-|z^-1|-(*a2)->-(+)->-+-|z^-1|-+-|z^-1|-+
     *      |        |                 |             |        |
     *      |       (*a1)----------->-(+)-<---------(*-b1)    |
     *     (*a0)-------------------->-(+)-<------------------(*-b2)
     */
    while (y < bound)
      {
        double accu = xd2 * a2;
        accu += xd1 * a1;
        xd2 = xd1;
        xd1 = *x++;
        accu += xd1 * a0;
        accu -= yd2 * b2;
        accu -= yd1 * b1;
        yd2 = yd1;
        yd1 = accu;
        *y++ = accu;
      }
  }
  void
  PeakFilterModule::reset ()
  {
    /* this function runs in various synthesis threads */
    xd1 = xd2 = 0;
    yd1 = yd2 = 0;
  }
  void
  PeakFilterModule::config (PeakFilter::Parameters *params)
  {
    /* this function runs in the master synthesis thread */
    double k = tan (params->f / (mix_freq() / 2.) * PI / 2.0);
    double kk = k * k;
    double denominator = 1.0 + (1.0 / params->q + k) * k;
    a0 = (1.0 + (params->v / params->q + k) * k) / denominator;
    a1 = (2.0 * (kk - 1.0)) / denominator;
    a2 = (1.0 - (params->v / params->q + k) * k) / denominator;
    b1 = a1;
    b2 = (1.0 - (1.0 / params->q + k) * k) / denominator;
  }
};

SynthesisModule::Accessor*
PeakFilter::module_configurator ()
{
  /* this function creates a (re-)configuration closure for synthesis
   * modules, to be run from the master synthesis thread only.
   */
  Parameters params (this);
  /* accessor() copies params and creates a closure from config()
   * to be executed asyncronously in synthesis threads for modules
   * created by create_module()
   */
  return SynthesisModule::accessor (&PeakFilterModule::config, params);
}

SynthesisModule*
PeakFilter::create_module (unsigned int context_handle,
                           GslTrans    *trans)
{
  /* create a synthesis module */
  PeakFilterModule *m = new PeakFilterModule();
  return m;
}

} // namespace
