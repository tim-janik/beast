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
#include <queue>
using namespace std;
using namespace Sfi;

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

      SnifferData *data = static_cast<SnifferData*> (gsl_module_peek_reply (gslmodule()));
      while (data && data->tick_stamp < tick_stamp() + n_values)
        {
          guint64 offset = data->tick_stamp > tick_stamp() ? data->tick_stamp - tick_stamp() : 0;
          if (!data->n_filled && data->tick_stamp < tick_stamp())
            data->tick_stamp = tick_stamp();        /* returned tick_stamp */
          guint left = MIN (data->fblock1->n_values - data->n_filled, n_values - offset);

          // FIXME: don't return overlapping blocks of sample data

          const float *values1, *values2;
          switch (data->stype)
            {
            case SNIFFER_REQUIRE_SINGLE_INPUT:
              if (jstream (JCHANNEL_AUDIO_IN1).n_connections > 1)
                {
                  data->fblock1->n_values = 0;
                  data->fblock2->n_values = 0;
                  data->n_filled = 0;
                  left = 0;
                  break;
                }
              /* fall through */
            default: 
            case SNIFFER_PICK_FIRST_INPUT:
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
            case SNIFFER_AVERAGE_INPUTS:
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
            {
              gsl_module_process_reply (gslmodule());
              data = static_cast<SnifferData*> (gsl_module_peek_reply (gslmodule()));
            }
          else
            data = NULL;
        }
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
        commit_queue (trans);
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
      {
        gsl_trans_add (trans, gsl_job_discard (single_sniff));
        single_sniff = NULL;
      }
  }
  struct SRequest {
    Num      tick_stamp;
    Int      n_samples;
    SnifferType stype;
  };
  queue<SRequest> queued_jobs;
  static void
  sniffer_reply (gpointer cdata,
                 gboolean processed)
  {
    SnifferData *data = static_cast<SnifferData*> (cdata);
    if (!processed)
      {
        data->fblock1->n_values = 0;
        data->fblock2->n_values = 0;
      }
    data->sniffer->emit_notify_pcm_data (data->tick_stamp, *data->fblock1, *data->fblock2);
    sfi_fblock_unref (data->fblock1);
    sfi_fblock_unref (data->fblock2);
    delete data;
  }
  void
  commit_queue (GslTrans *trans = NULL)
  {
    if (!single_sniff)
      return;
    GslTrans *tmptrans = NULL;
    if (!trans)
      trans = tmptrans = gsl_trans_open();
    while (!queued_jobs.empty())
      {
        SRequest sr = queued_jobs.front();
        queued_jobs.pop();
        SnifferData *data = new SnifferData();
        data->sniffer = this;
        data->tick_stamp = sr.tick_stamp;
        data->n_filled = 0;
        data->fblock1 = sfi_fblock_new_sized (sr.n_samples);
        data->fblock2 = sfi_fblock_new_sized (sr.n_samples);
        data->stype = sr.stype;
        gsl_trans_add (trans, gsl_job_request_reply (single_sniff, data, sniffer_reply));
      }
    if (tmptrans)
      gsl_trans_commit (tmptrans);
  }
public:
  void
  queue_job (Num      tick_stamp,
             Int      n_samples,
             SnifferType stype)
  {
    SRequest sr;
    sr.tick_stamp = tick_stamp;
    sr.n_samples = n_samples;
    sr.stype = stype;
    queued_jobs.push (sr);
    commit_queue();
  }
  GslModule* get_sniffer_module () { return is_prepared() ? single_sniff : NULL; }
  Sniffer() :
    single_sniff (NULL),
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

void
sniffer_request_samples::exec (Sniffer *self,
                               Num      tick_stamp,
                               Int      n_samples,
                               SnifferType stype)
{
  if (!self || n_samples < 1)
    throw std::runtime_error ("invalid arguments");
  self->queue_job (tick_stamp, n_samples, stype);
  // g_printerr ("sniffer_request_samples(%llu, %u)\n", tick_stamp, n_samples);
}

Num
sniffer_get_tick_stamp::exec (Sniffer *obj)     // FIXME there should be a global procedure for this
{
  return gsl_tick_stamp ();
}

Int
sniffer_get_mix_freq::exec (Sniffer *self)
{
  if (!self)
    throw std::runtime_error ("invalid arguments");
  GslModule *module = self->get_sniffer_module ();
  return module ? gsl_engine_sample_freq() : 0;
}

void
sniffer_request_combined::exec (const SnifferRequestSeq &srs)
{
  for (guint i = 0; i < srs.length(); i++)
    {
      const SnifferRequestHandle sr = srs[i];
      Sniffer *self = sr->sniffer;
      if (!BSE_IS_SNIFFER (self))
        throw InvalidArgument (self);
      Int   n_samples = sr->n_samples;
      SnifferType stype = sr->sniffer_type;
      Num   tick_stamp = sr->variable_time;
      switch (sr->time_type)
        {
        case SNIFFER_TIME_ABSOLUTE_TICK_STAMP:  /* handled in initialization */
          break;
        case SNIFFER_TIME_RELATIVE_USECS:
          tick_stamp = Num (sr->variable_time * (gsl_engine_sample_freq() * 0.000001) + gsl_tick_stamp ());
          break;
        case SNIFFER_TIME_RELATIVE_TICK_STAMP:
          tick_stamp = sr->variable_time + gsl_tick_stamp ();
          break;
        }
      self->queue_job (tick_stamp, n_samples, stype);
    }
}

} // Procedure

BSE_CXX_DEFINE_EXPORTS ();
BSE_CXX_REGISTER_ENUM (SnifferType);
BSE_CXX_REGISTER_EFFECT (Sniffer);
BSE_CXX_REGISTER_PROCEDURE (sniffer_request_samples);
BSE_CXX_REGISTER_PROCEDURE (sniffer_get_tick_stamp);
BSE_CXX_REGISTER_PROCEDURE (sniffer_get_mix_freq);
BSE_CXX_REGISTER_ENUM (SnifferTimeType);
BSE_CXX_REGISTER_RECORD (SnifferRequest);
BSE_CXX_REGISTER_SEQUENCE (SnifferRequestSeq);
BSE_CXX_REGISTER_PROCEDURE (sniffer_request_combined);

} // Bse
