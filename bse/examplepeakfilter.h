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
#ifndef __EXAMPLE_PEAK_FILTER_H__
#define __EXAMPLE_PEAK_FILTER_H__

#include <bse/bsecxxmodule.h>

namespace Example {
using namespace Bse;

#define EXAMPLE_TYPE_PEAK_FILTER     (BSE_CXX_TYPE_GET_REGISTERED (PeakFilter))

class PeakFilter : public Effect {
  /* properties: */
  enum { PROP_FREQ = 1, PROP_GAIN, PROP_QUALITY };
  SfiReal freq; // peak frequency
  SfiReal gain; // in dB
  SfiReal quality;
  double  v;
public:
  /* input/output channel IDs */
  enum
  {
    ICHANNEL_MONO,
    N_ICHANNELS
  };
  enum
  {
    OCHANNEL_FILTERED,
    N_OCHANNELS
  };
  /* "transport" structure to configure synthesis modules from properties: */
  struct Parameters {
    SfiReal f, g, q;
    double  v;
    explicit Parameters (PeakFilter *p)
    {
      f = p->freq;
      g = p->gain;
      q = p->quality;
      v = p->v;
    }
  };
  explicit          PeakFilter          ();
  void              set_property        (guint          prop_id,
                                         const Value   &value,
                                         GParamSpec    *pspec);
  void              get_property        (guint          prop_id,
                                         Value         &value,
                                         GParamSpec    *pspec);
  SynthesisModule*  create_module       (unsigned int   context_handle,
                                         GslTrans      *trans);
  SynthesisModule::Accessor*
                    module_configurator ();
  virtual          ~PeakFilter          ();
  
  static void       class_init          (CxxBaseClass *klass);
};

}

#endif /* __EXAMPLE_PEAK_FILTER_H__ */
