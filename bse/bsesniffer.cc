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
#include "gslcommon.h" /* for gsl_tick_stamp() */
#include <stdexcept>

namespace Bse {

struct SnifferData {
  Sniffer    *sniffer;
  guint64     tick_stamp;
  guint       n_filled;
  SfiFBlock  *fblock1;
  SfiFBlock  *fblock2;
  SnifferType stype;
};

class Sniffer : public SnifferBase {
  class SingleSniff : public SynthesisModule {
    static const guint JCHANNEL_AUDIO_IN1 = 0, JCHANNEL_AUDIO_IN2 = 1;
    float *buffer;
  public:
    SingleSniff()
    {
      buffer = new float[block_size()];
    }
    ~SingleSniff()
    {
      delete[] buffer;
    }
    void
    reset ()
    {
    }
    void
    process (unsigned int n_values)
    {
      if (jstream (JCHANNEL_AUDIO_IN1).n_connections !=
          jstream (JCHANNEL_AUDIO_IN2).n_connections)
        return;
      g_printerr ("SNIFFER: %u connections\n", jstream (JCHANNEL_AUDIO_IN1).n_connections);

      SnifferData *data = static_cast<SnifferData*> (gsl_module_peek_reply (gslmodule()));
      if (!data || data->tick_stamp >= tick_stamp() + n_values)
        return;

      guint64 offset = data->tick_stamp > tick_stamp() ? data->tick_stamp - tick_stamp() : 0;
      guint left = MIN (data->fblock1->n_values - data->n_filled, n_values - offset);

      if (!data->n_filled && data->tick_stamp < tick_stamp())
        data->tick_stamp = tick_stamp();        /* returned tick_stamp */

      const float *values1, *values2;
      switch (data->stype)
        {
        case REQUIRE_SINGLE_INPUT:
          if (jstream (JCHANNEL_AUDIO_IN1).n_connections > 1)
            {
              data->fblock1->n_values = 0;
              data->fblock2->n_values = 0;
              gsl_module_process_reply (gslmodule());
              return;
            }
          /* fall through */
        default: 
        case PICK_FIRST_INPUT:
          if (jstream (JCHANNEL_AUDIO_IN1).n_connections)
            {
              values1 = jstream (JCHANNEL_AUDIO_IN1).values[0];
              values2 = jstream (JCHANNEL_AUDIO_IN2).values[0];
            }
          else
            values1 = values2 = const_values (0);
          memcpy (data->fblock1->values + data->n_filled, values1, left * sizeof (values1[0]));
          memcpy (data->fblock2->values + data->n_filled, values2, left * sizeof (values2[0]));
          break;
        case AVERAGE_INPUTS:
          if (jstream (JCHANNEL_AUDIO_IN1).n_connections > 1)
            {
              double nf = 1.0 / (double) jstream (JCHANNEL_AUDIO_IN1).n_connections;
              guint j;

              memcpy (buffer, jstream (JCHANNEL_AUDIO_IN1).values[0], left * sizeof (buffer[0]));
              for (j = 1; j + 1 < jstream (JCHANNEL_AUDIO_IN1).n_connections; j++)
                for (guint i = 0; i < left; i++)
                  buffer[i] += jstream (JCHANNEL_AUDIO_IN1).values[j][offset + i];
              for (guint i = 0; i < left; i++) /* j == n_connections */
                buffer[i] = nf * (buffer[i] + jstream (JCHANNEL_AUDIO_IN1).values[j][offset + i]);
              memcpy (data->fblock1->values + data->n_filled, buffer, left * sizeof (buffer[0]));

              memcpy (buffer, jstream (JCHANNEL_AUDIO_IN2).values[0], left * sizeof (buffer[0]));
              for (j = 1; j + 1 < jstream (JCHANNEL_AUDIO_IN2).n_connections; j++)
                for (guint i = 0; i < left; i++)
                  buffer[i] += jstream (JCHANNEL_AUDIO_IN2).values[j][offset + i];
              for (guint i = 0; i < left; i++) /* j == n_connections */
                buffer[i] = nf * (buffer[i] + jstream (JCHANNEL_AUDIO_IN2).values[j][offset + i]);
              memcpy (data->fblock2->values + data->n_filled, buffer, left * sizeof (buffer[0]));
            }
          else
            {
              if (jstream (JCHANNEL_AUDIO_IN1).n_connections) /* == 1 */
                {
                  values1 = jstream (JCHANNEL_AUDIO_IN1).values[0] + offset;
                  values2 = jstream (JCHANNEL_AUDIO_IN2).values[0] + offset;
                }
              else
                values1 = values2 = const_values (0);
              memcpy (data->fblock1->values + data->n_filled, values1, left * sizeof (values1[0]));
              memcpy (data->fblock2->values + data->n_filled, values2, left * sizeof (values2[0]));
            }
          break;
        }
      data->n_filled += left;

      if (data->n_filled >= data->fblock1->n_values)
        gsl_module_process_reply (gslmodule());
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
        single_sniff = gsl_module_new (create_gsl_class (m, CHEAP, 0, 2, 0), m);
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
  GslModule* get_sniffer_module () { return is_prepared() ? single_sniff : NULL; }
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
    gsl_trans_add (trans, gsl_job_jconnect (vmodule, 1, sniffer, 1));
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

namespace Procedure {

static void
sniffer_reply (gpointer cdata,
               gboolean processed)
{
  SnifferData *data = static_cast<SnifferData*> (cdata);
  if (processed)
    data->sniffer->emit_notify_pcm_data (data->tick_stamp, data->fblock1, data->fblock2);
  sfi_fblock_unref (data->fblock1);
  sfi_fblock_unref (data->fblock2);
  delete data;
}

void
sniffer_request_samples::exec (Sniffer *self,
                               Num      tick_stamp,
                               Int      n_samples,
                               SnifferType stype)
{
  if (!self || n_samples < 1)
    throw std::runtime_error ("invalid arguments");
  GslModule *sniffer = self->get_sniffer_module ();
  if (sniffer)
    {
      SnifferData *data = new SnifferData();
      data->sniffer = self;
      data->tick_stamp = tick_stamp;
      data->n_filled = 0;
      data->fblock1 = sfi_fblock_new_sized (n_samples);
      data->fblock2 = sfi_fblock_new_sized (n_samples);
      data->stype = stype;
      gsl_transact (gsl_job_request_reply (sniffer, data, sniffer_reply),
                    NULL);
    }
  g_printerr ("sniffer_start_shooting()\n");
}

Num
sniffer_get_tick_stamp::exec (Sniffer *obj)     // FIXME there should be a global procedure for this
{
  return gsl_tick_stamp ();
}

} // Procedure

BSE_CXX_DEFINE_EXPORTS ();
BSE_CXX_REGISTER_ENUM (SnifferType);
BSE_CXX_REGISTER_EFFECT (Sniffer);
BSE_CXX_REGISTER_PROCEDURE (sniffer_request_samples);
BSE_CXX_REGISTER_PROCEDURE (sniffer_get_tick_stamp);

} // Bse
