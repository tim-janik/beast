/* BSE - Bedevilled Sound Engine
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
#include "bsesniffer.gen-idl.h"
#include "gslengine.h"
#include <stdexcept>

namespace Bse {

class Sniffer : public SnifferBase {
  class SingleSniff : public SynthesisModule {
    bool ctrl_mul;
    static const guint JCHANNEL_AUDIO_IN = Sniffer::ICHANNEL_AUDIO_IN;
  public:
    void
    config (SnifferProperties *params)
    {
      ctrl_mul = params->ctrl_mul;
    }
    void
    reset ()
    {
    }
    void
    process (unsigned int n_values)
    {
      g_printerr ("SNIFFER: %u connections\n", jstream (JCHANNEL_AUDIO_IN).n_connections);
    }
  };
  GslModule *single_sniff;         /* context-singleton per instance */
  guint      single_sniff_ref_count;
  GslModule*
  single_sniff_ref (GslTrans *trans)
  {
    if (!single_sniff_ref_count)
      {
        SingleSniff *m = new SingleSniff();
        single_sniff = gsl_module_new (create_gsl_class (m, CHEAP, 0, 1, 0), m);
        m->set_module (single_sniff);
        gsl_trans_add (trans, gsl_job_integrate (single_sniff));
        gsl_trans_add (trans, gsl_job_set_consumer (single_sniff, TRUE));
      }
    single_sniff_ref_count++;
    return single_sniff;
  }
  void
  single_sniff_unref (GslTrans *trans)
  {
    g_return_if_fail (single_sniff_ref_count > 0);
    single_sniff_ref_count--;
    if (!single_sniff_ref_count)
      gsl_trans_add (trans, gsl_job_discard (single_sniff));
  }
public:
  Sniffer() :
    single_sniff_ref_count (0)
  {
  }
  /* implement creation and config methods for synthesis modules */
  GslModule*
  integrate_gsl_module (unsigned int   context_handle,
                        GslTrans      *trans)
  {
    /* here, we create a virtual module for each context, connecting to the single sniffer module */
    GslModule *sniffer = single_sniff_ref (trans);
    GslModule *vmodule = gsl_module_new_virtual (sniffer->klass->n_jstreams, NULL, NULL);
    /* intergrate module into engine */
    gsl_trans_add (trans, gsl_job_integrate (vmodule));
    /* connect to virtual module */
    gsl_trans_add (trans, gsl_job_jconnect (vmodule, 0, sniffer, 0));
    return vmodule;
  }
  void
  dismiss_gsl_module (GslModule       *vmodule,
                      guint            context_handle,
                      GslTrans        *trans)
  {
    gsl_trans_add (trans, gsl_job_discard (vmodule));
    single_sniff_unref (trans);
  }
  SynthesisModule*
  create_module (unsigned int context_handle,
                 GslTrans    *trans)
  {
    return NULL; // dummy, creation taken over by integrate_gsl_module()
  }
  SynthesisModule::Accessor*
  module_configurator()
  {
    return NULL; // dummy, nothing to configure with virtual modules
  }
};

SfiInt
Procedure_sniffer_start_shooting::exec (Sniffer *self,
                                        int i)
{
  if (!self)
    throw std::runtime_error ("object pointer is NULL");
  
  g_printerr ("Procedure_sniffer_start_shooting()\n");
  return 0;
}

BSE_CXX_DEFINE_EXPORTS ();
BSE_CXX_REGISTER_EFFECT (Sniffer);
BSE_CXX_REGISTER_PROC (sniffer_start_shooting);

} // Bse

