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
#include "bseprobe.gen-idl.h"
#include "bseengine.h"
#include "gslcommon.h" /* for gsl_tick_stamp() */
#include "gslfft.h"
#include <stdexcept>
#include <queue>
using namespace std;
using namespace Sfi;

namespace { // Anon
using namespace Bse;

static const guint PROBE_QUEUE_LENGTH = 3;
static guint signal_probes = 0;

class SourceProbes {
  BseSource          *source;
  guint               n_channels;
  vector<ProbeHandle> channel_probes;
  vector<guint8>      range_ages, energie_ages, samples_ages, fft_ages, channel_ages;
  guint               bsize_fifo[PROBE_QUEUE_LENGTH], bsize_offset;
  SfiRing            *omodules;
  guint               queued_jobs;
  guint               idle_handler_id;
  void
  resize_ochannels (guint n_ochannels)
  {
    n_channels = n_ochannels;
    range_ages.resize (n_channels, 0);
    energie_ages.resize (n_channels, 0);
    samples_ages.resize (n_channels, 0);
    fft_ages.resize (n_channels, 0);
    channel_ages.resize (n_channels, 0);
    Probe zprobe;
    zprobe.probe_features = ProbeFeatures();
    channel_probes.resize (n_channels, zprobe);
  }
public:
  SourceProbes (BseSource *src) :
    source (src), n_channels (0), omodules (NULL)
  {
    queued_jobs = 0;
    idle_handler_id = 0;
    resize_ochannels (BSE_SOURCE_N_OCHANNELS (src));
    memset (bsize_fifo, 0, sizeof (bsize_fifo));
    bsize_offset = 0;
  }
  ~SourceProbes ()
  {
    g_assert (queued_jobs == 0);
  }
  void
  reset_omodules ()
  {
    sfi_ring_free (omodules);
    omodules = NULL;
  }
private:
  SfiRing*
  get_omodules ()
  {
    if (!omodules)
      {
        omodules = bse_source_list_omodules (source);
        /* remove dupes */
        omodules = sfi_ring_sort (omodules, sfi_pointer_cmp, NULL);
        omodules = sfi_ring_uniq (omodules, sfi_pointer_cmp, NULL);
      }
    return omodules;
  }
  struct ProbeData {
    BseSource *source;
    guint      n_modules;
    guint      n_pending;
    ProbeSeq   pseq;
    ProbeData (const SourceProbes &probes,
               gdouble             mix_freq)
    {
      source = probes.source;
      for (guint i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
        if (probes.channel_ages[i])
          {
            Probe probe;
            probe.channel_id = i;
            probe.max = SFI_MINREAL;
            probe.min = SFI_MAXREAL;
            probe.mix_freq = mix_freq;
            probe.energie = -999;
            ProbeFeatures features;
            features.probe_range = probes.range_ages[i] > 0;
            features.probe_energie = probes.energie_ages[i] > 0;
            features.probe_samples = probes.samples_ages[i] > 0;
            features.probe_fft = probes.fft_ages[i] > 0;
            probe.probe_features = features;
            pseq += probe;
          }
    }
  };
  void
  fill_probe (Probe  &probe,
              guint   n_values,
              gfloat *values,
              bool    need_summing)
  {
    if (probe.probe_features->probe_range || probe.probe_features->probe_energie)
      {
        gdouble accu = 0;
        for (gfloat *v = values; v < values + n_values; v++)
          {
            if (G_UNLIKELY (*v < probe.min))
              probe.min = *v;
            if (G_UNLIKELY (*v > probe.max))
              probe.max = *v;
            accu += *v * *v;
          }
        probe.energie = accu > 0 ? 10 * log10 (accu / n_values) : -999;
      }
    SfiFBlock *fblock = probe.sample_data.fblock();
    if (need_summing && fblock)
      {
        gfloat *v = values;
        for (FBlock::iterator it = probe.sample_data.begin(); it != probe.sample_data.end(); it++)
          *it += *v++;
      }
  }
  /* returns a blackman window: x is supposed to be in the interval [0..1] */
  inline double
  blackman_window (double x)
  {
    if (x < 0)
      return 0;
    if (x > 1)
      return 0;
    return 0.42 - 0.5 * cos (PI * x * 2) + 0.08 * cos (4 * PI * x);
  }
  void
  handle_probes (ProbeData &pdata,
                 guint64    tick_stamp,
                 guint      n_values,
                 gfloat   **oblocks) /* [ENGINE_NODE_N_OSTREAMS()] */
  {
    if (pdata.n_pending == pdata.n_modules - 1) /* setup fblocks from first module */
      for (ProbeSeq::iterator it = pdata.pseq.begin(); it != pdata.pseq.end(); it++)
        {
          Probe &probe = **it;
          gfloat *block = oblocks[probe.channel_id];
          if (probe.probe_features->probe_samples || probe.probe_features->probe_fft)
            {
              probe.sample_data.take (sfi_fblock_new_foreign (n_values, block, g_free));
              oblocks[probe.channel_id] = NULL; /* steal from engine */
              fill_probe (probe, n_values, block, FALSE);
            }
          else
            fill_probe (probe, n_values, block, FALSE);
        }
    else
      for (ProbeSeq::iterator it = pdata.pseq.begin(); it != pdata.pseq.end(); it++)
        fill_probe (**it, n_values, oblocks[(*it)->channel_id], TRUE);
    if (!pdata.n_pending)       /* last module */
      {
        /* min/max fixup */
        if (!n_values)
          for (ProbeSeq::iterator it = pdata.pseq.begin(); it != pdata.pseq.end(); it++)
            {
              Probe &probe = **it;
              if (probe.probe_features->probe_range && probe.max < probe.min)
                probe.min = probe.max = 0;
            }
        /* complete fft */
        for (ProbeSeq::iterator it = pdata.pseq.begin(); it != pdata.pseq.end(); it++)
          {
            Probe &probe = **it;
            if (probe.probe_features->probe_fft)
              {
                guint fft_size = probe.sample_data.length();
                if (fft_size)
                  {
                    fft_size = 1 << (g_bit_storage (fft_size) - 1);
                    fft_size = MIN (fft_size, 65536);
                    probe.fft_data.resize (fft_size);
                    /* perform fft */
                    double *rv = g_new (double, fft_size * 2), *cv = rv + fft_size; // FIXME: optimize
                    gfloat *fvalues = probe.sample_data.fblock()->values;
                    double reci_fft_size = 1.0 / (fft_size - 1);
                    guint i = fft_size;
                    while (i--) /* convert to double */
                      rv[i] = fvalues[i] * blackman_window (i * reci_fft_size);
                    gsl_power2_fftar (fft_size, rv, cv);
                    fvalues = probe.fft_data.fblock()->values;
                    reci_fft_size = 1.0 / fft_size;
                    i = fft_size;
                    while (i--) /* convert to float */
                      fvalues[i] = cv[i] * reci_fft_size;
                    g_free (rv);
                  }
                else
                  probe.fft_data.resize (0);
                if (!probe.probe_features->probe_samples)
                  probe.sample_data.take (NULL); /* delete fblock */
              }
          }
        g_signal_emit (source, signal_probes, 0, pdata.pseq.c_ptr());
        /* clean up counters */
        for (guint i = 0; i < n_channels; i++)
          {
            if (channel_ages[i])
              channel_ages[i]--;
            if (range_ages[i])
              range_ages[i]--;
            if (energie_ages[i])
              energie_ages[i]--;
            if (samples_ages[i])
              samples_ages[i]--;
            if (fft_ages[i])
              fft_ages[i]--;
          }
        g_assert (queued_jobs > 0);
        queued_jobs--;
      }
  }
  static void
  source_probe_callback (gpointer       data,
                         guint64        tick_stamp,
                         guint          n_values,
                         gfloat       **oblocks) /* [ENGINE_NODE_N_OSTREAMS()] */
  {
    ProbeData *pdata = reinterpret_cast<ProbeData*> (data);
    g_assert (pdata->n_pending > 0);
    pdata->n_pending--;
    SourceProbes *probes = peek_from_source (pdata->source);
    if (probes)
      probes->handle_probes (*pdata, tick_stamp, n_values, oblocks);
    if (!pdata->n_pending)
      delete pdata;
  }
public:
  void
  commit_requests ()
  {
    if (queued_jobs >= PROBE_QUEUE_LENGTH ||
        !BSE_SOURCE_PREPARED (source))
      return;
    SfiRing *ring = get_omodules();
    if (!ring)
      return;
    BseTrans *trans = bse_trans_open();
    while (queued_jobs < PROBE_QUEUE_LENGTH)
      {
        ProbeData *pdata = new ProbeData (*this, bse_engine_sample_freq());
        pdata->n_modules = 0;
        for (SfiRing *node = ring; node; node = sfi_ring_walk (node, ring))
          {
            guint block_size = 0;
            for (guint i = 0; i < PROBE_QUEUE_LENGTH; i++)
              block_size = MAX (block_size, bsize_fifo[i]);
            guint max_bsize = MIN (bse_engine_sample_freq(), 65536);
            block_size = CLAMP (block_size, 2, max_bsize);
            bse_trans_add (trans, bse_job_probe_request ((BseModule*) node->data,
                                                         0, // 3 * bse_engine_block_size(),
                                                         block_size, &channel_ages[0], source_probe_callback, pdata));
            pdata->n_modules++;
          }
        pdata->n_pending = pdata->n_modules;
        queued_jobs++;
      }
    bse_trans_commit (trans);
  }
  static gboolean
  idle_commit_requests (gpointer data)
  {
    BseSource *source = BSE_SOURCE (data);
    SourceProbes *probes = peek_from_source (source);
    if (probes)
      {
        probes->idle_handler_id = 0;
        probes->commit_requests();
      }
    g_object_unref (source);
    return FALSE;
  }
  void
  queue_update()
  {
    if (!idle_handler_id)
      idle_handler_id = bse_idle_now (idle_commit_requests, g_object_ref (source));
  }
  void
  queue_probe_request (const ProbeFeatures **channel_features, // [n_ochannels]
                       guint                 requested_block_size)
  {
    bool queue_block_size = false;
    /* update probe ages */
    for (guint i = 0; i < n_channels; i++)
      if (channel_features[i])
        {
          guint oldage = channel_ages[i];
          if (channel_features[i]->probe_range)
            channel_ages[i] = range_ages[i] = PROBE_QUEUE_LENGTH;
          if (channel_features[i]->probe_energie)
            channel_ages[i] = energie_ages[i] = PROBE_QUEUE_LENGTH;
          if (channel_features[i]->probe_samples)
            channel_ages[i] = samples_ages[i] = PROBE_QUEUE_LENGTH;
          if (channel_features[i]->probe_fft)
            channel_ages[i] = fft_ages[i] = PROBE_QUEUE_LENGTH;
          queue_block_size = oldage != channel_ages[i];
        }
    /* enqueue block size */
    if (queue_block_size)
      {
        bsize_fifo[bsize_offset++] = requested_block_size;
        bsize_offset %= PROBE_QUEUE_LENGTH;
      }
    queue_update();
  }
  static SourceProbes*
  peek_from_source (BseSource *source)
  {
    return reinterpret_cast<SourceProbes*> (source->probes);
  }
  static SourceProbes*
  create_from_source (BseSource *source)
  {
    if (!source->probes)
      source->probes = reinterpret_cast<BseSourceProbes*> (new SourceProbes (source));
    return peek_from_source (source);
  }
};    

} // Anon

namespace Bse {
namespace Procedure {

void
source_request_probes::exec (BseSource                 *source,
                             Int                        ochannel_id,
                             const ProbeFeaturesHandle &probe_features)
{
  ProbeRequest rq;
  rq.source = source;
  rq.channel_id = ochannel_id;
  rq.probe_features = probe_features;
  ProbeRequestSeq prs;
  prs += rq;
  source_mass_request::exec (prs);
}

void
source_mass_request::exec (const ProbeRequestSeq &cprseq)
{
  struct Sub {
    static bool
    probe_requests_lesser (const ProbeRequestHandle &h1,
                           const ProbeRequestHandle &h2)
    {
      return h2->source < h1->source;
    }
  };
  ProbeRequestSeq prs (cprseq);
  /* sort probe-requests, so requests for a single source are adjacent */
  stable_sort (prs.begin(), prs.end(), Sub::probe_requests_lesser);
  BseSource *current = NULL;
  guint block_size = 0;
  const ProbeFeatures **channel_features = NULL;
  for (ProbeRequestSeq::iterator it = prs.begin(); it != prs.end(); it++)
    {
      if (!(*it)->source)
        continue;       /* can happen due to sources getting destroyed before asyncronous delivery */
      else if ((*it)->source != current)
        {               /* open new request list */
          if (current)
            {           /* commit old request list */
              SourceProbes *probes = SourceProbes::create_from_source (current);
              probes->queue_probe_request (channel_features, block_size);
              probes->commit_requests();
              g_free (channel_features);
              channel_features = NULL;
            }
          current = (*it)->source;
          channel_features = g_new0 (const ProbeFeatures*, BSE_SOURCE_N_OCHANNELS (current));
          block_size = 0;
        }
      guint channel_id = (*it)->channel_id;
      if (channel_id < BSE_SOURCE_N_OCHANNELS (current))        /* add request */
        {
          channel_features[channel_id] = (*it)->probe_features.c_ptr();
          block_size = MAX ((gint) block_size, (*it)->block_size);
        }
    }
  if (current)
    {                   /* commit last request list */
      SourceProbes *probes = SourceProbes::create_from_source (current);
      probes->queue_probe_request (channel_features, block_size);
      probes->commit_requests();
      g_free (channel_features);
      channel_features = NULL;
    }
}

Num
source_get_tick_stamp::exec (BseSource *self)
{
  return gsl_tick_stamp ();
}

Int
source_get_mix_freq::exec (BseSource *self)
{
  if (!self)
    throw std::runtime_error ("invalid arguments");
  return BSE_SOURCE_PREPARED (self) ? bse_engine_sample_freq() : 0;
}

} // Procedure

/* export definitions follow */
BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_ALL_TYPES_FROM_BSEPROBE_IDL();

} // Bse

/* --- bsesource.h bits --- */
extern "C" {    // from bsesource.h
using namespace Bse;

void
bse_source_clear_probes (BseSource *source)
{
  g_return_if_fail (!BSE_SOURCE_PREPARED (source));
  SourceProbes *probes = SourceProbes::peek_from_source (source);
  source->probes = NULL;
  delete probes;
}                          

void
bse_source_probes_modules_changed (BseSource *source)
{
  SourceProbes *probes = SourceProbes::peek_from_source (source);
  probes->reset_omodules();
  probes->queue_update();
}

void
bse_source_class_add_probe_signals (BseSourceClass *klass)
{
  g_assert (signal_probes == 0);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  signal_probes = bse_object_class_add_signal (object_class, "probes", G_TYPE_NONE, 1, BSE_TYPE_PROBE_SEQ);
}

};
