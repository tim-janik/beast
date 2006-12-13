/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2003-2006 Tim Janik
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
#include "bseprobe.genidl.hh"
#include "bseengine.h"
#include "bseblockutils.hh"
#include "gslcommon.h" /* for gsl_tick_stamp() */
#include "gslfft.h"
#include <stdexcept>
#include <set>
using namespace std;
using namespace Sfi;

namespace { // Anon
using namespace Bse;

/* --- variables --- */
static guint    MAX_QUEUE_LENGTH = 3; // or, for 20ms: (int) (bse_engine_sample_freq() * 0.020 / bse_engine_block_size() + 0.5)
static guint    bse_source_signal_probes = 0;

/* --- functions --- */
static inline double
blackman_window (double x)
{       // FIXE: parameterize, move somewhere else
  /* returns a blackman window: x is supposed to be in the interval [0..1] */
  if (x < 0)
    return 0;
  if (x > 1)
    return 0;
  return 0.42 - 0.5 * cos (PI * x * 2) + 0.08 * cos (4 * PI * x);
}


/* --- ProbeQueue --- */
class SourceProbes;
class ProbeQueue {
  /* this class is part of the BseSource probing logic. each source can have a set of these
   * classes, one for each (output-channel, block_size) combination. the sum of the probing
   * data from all of this source's engine modules is fed into the ProbeQueue objects which
   * then compute the requested probe features from the probe data.
   * the blocks of probe data used to compute features from are deliberately aligned to the
   * individual ProbeQueue's block_size. enforcing aligned feature computation results in
   * all ProbeQueue objects in the BSE core to emit new features simultaneously, which in
   * turn allowes aggregation of new probe feature messages so they can be sent to the
   * requesting clients in batches.
   */
  SourceProbes &probes;
  const uint    block_size;
  guint64       first_stamp;
  uint          n_computed;
  struct {
    uint8 range;
    uint8 energie;
    uint8 samples;
    uint8 fft;
  } requests;
  gfloat        range_min, range_max;
  gfloat        energie_accu;
  gfloat       *raw_floats;
  bool          seen_raw_floats;
  bool          probe_xrun;
  /* --- methods --- */
  static inline bool
  is_power2 (guint32 v)
  {
    guint64 n = 1 << (g_bit_storage (v) - 1);
    return n == v;
  }
  inline void
  reset_probe_state()
  {
    first_stamp = 0;
    n_computed = 0;
    range_min = +3.40282347e+38;
    range_max = -3.40282347e+38;
    energie_accu = 0;
    seen_raw_floats = false;
    probe_xrun = false;
  }
  void
  compute_probe_state (guint         n,
                       const gfloat *oblock,
                       bool          connected)
  {
    g_assert (n <= block_size - n_computed);
    /* store samples, possibly for fft */
    if (requests.samples || requests.fft)
      {
        if (!raw_floats)
          raw_floats = g_new (gfloat, block_size);
        if (connected && !probe_xrun)
          {
            seen_raw_floats |= true;    /* seen non-0.0 raw floats */
            bse_block_copy_float (n, raw_floats + n_computed, oblock);
          }
        else
          bse_block_fill_float (n, raw_floats + n_computed, 0.0);
      }
    if (connected && !probe_xrun)
      {
        /* calc min/max and/or energie */
        if (requests.range && requests.energie)
          {
            float rmin = range_min, rmax = range_max;
            energie_accu += bse_block_calc_float_range_and_square_sum (n, oblock, &rmin, &rmax);
            range_min = MIN (rmin, range_min);
            range_max = MAX (rmax, range_max);
          }
        else if (requests.energie)
          energie_accu += bse_block_calc_float_square_sum (n, oblock);
        else if (requests.range)
          {
            float rmin = range_min, rmax = range_max;
            bse_block_calc_float_range (n, oblock, &rmin, &rmax);
            range_min = MIN (rmin, range_min);
            range_max = MAX (rmax, range_max);
          }
      }
    else /* !connected */
      {
        /* calc min/max and energie for silence */
        range_min = MIN (range_min, 0);
        range_max = MAX (range_max, 0);
        // energie_accu += 0;
      }
    /* adjust counter */
    if (requests.range || requests.energie || requests.samples || requests.fft)
      n_computed += n;
  }
  void
  complete_probe (Probe  &probe,
                  guint64 channel_id)
  {
    if (n_computed != block_size)
      return;
    /* fill probe */
    probe.channel_id = channel_id;
    probe.block_stamp = first_stamp;
    probe.mix_freq = bse_engine_sample_freq();
    ProbeFeatures probe_features;
    probe_features.probe_range = requests.range > 0;
    /* fill probe range */
    if (probe_features.probe_range)
      {
        probe.min = range_min;
        probe.max = range_max;
        if (probe_xrun)
          probe.min = probe.max = 0.0;
        requests.range -= 1;
      }
    /* fill probe energie */
    probe_features.probe_energie = requests.energie > 0;
    if (probe_features.probe_energie)
      {
        /* silently ignore xruns on energie calculation */
        probe.energie = energie_accu > 0 ? 10 * log10 (energie_accu / n_computed) : -999;
        requests.energie -= 1;
      }
    /* fill probe fft */
    probe_features.probe_fft = requests.fft > 0;
    if (probe_features.probe_fft)
      {
        guint fft_size = n_computed;
        bool valid_fft_size = is_power2 (fft_size) && fft_size >= 4 && fft_size <= 65536;
        if (raw_floats && valid_fft_size && seen_raw_floats && !probe_xrun)
          {
            /* perform fft */
            double *rv = g_newa (double, fft_size * 2);
            double *cv = rv + fft_size;
            const float *ivalues = raw_floats;
            double reci_fft_size = 1.0 / (fft_size - 1);
            uint i = fft_size;
            while (i--) /* convert to double */
              rv[i] = ivalues[i] * blackman_window (i * reci_fft_size);
            gsl_power2_fftar (fft_size, rv, cv);
            probe.fft_data.resize (fft_size);
            float *fvalues = probe.fft_data.fblock()->values;
            reci_fft_size = 1.0 / fft_size;
            i = fft_size;
            while (i--) /* convert to float */
              fvalues[i] = cv[i] * reci_fft_size;
          }
        else if (raw_floats && valid_fft_size)
          {
            /* all raw floats are 0.0 and so will be the resulting fft */
            probe.fft_data.resize (fft_size);
            bse_block_fill_float (fft_size, probe.fft_data.fblock()->values, 0.0);
          }
        else
          probe_features.probe_fft = false;
        requests.fft -= 1;
      }
    /* fill probe samples */
    probe_features.probe_samples = requests.samples > 0;
    if (probe_features.probe_samples)
      {
        if (raw_floats)
          {
            /* if (probe_xrun) bse_block_fill_float (n_computed, raw_floats, 0.0); */
            SfiFBlock *fblock = sfi_fblock_new_foreign (block_size, raw_floats, g_free);
            raw_floats = NULL;
            probe.sample_data.take (fblock);
          }
        else
          probe_features.probe_samples = false;
        requests.samples -= 1;
      }
    /* complete and clean up */
    probe.probe_features = probe_features;
    if (raw_floats)
      {
        g_free (raw_floats);
        raw_floats = NULL;
      }
    reset_probe_state();
  }
  void queue_probes_update (uint probe_queue_length);
public:
  void
  handle_probe_values (ProbeSeq     &probe_seq,
                       guint         n_values,
                       guint64       tick_stamp,
                       guint         channel_id,
                       const gfloat *oblock,
                       bool          connected)
  {
    while (n_values)
      {
        /* only start computing at block_size-aligned positions in the stream */
        if (n_computed == 0)
          {
            uint misaligned = tick_stamp % block_size;
            if (UNLIKELY (misaligned > 0))
              {
                uint offset = block_size - misaligned;
                /* skip ahead */
                offset = MIN (offset, n_values);        /* don't skip too far */
                tick_stamp += offset;
                oblock += offset;
                n_values -= offset;
                continue;                               /* check n_values again */
              }
            first_stamp = tick_stamp;                   /* record probe stamp */
          }
        else if (tick_stamp != first_stamp + n_computed)
          probe_xrun = true; // g_printerr ("%s: gap in probe block: %lld\n", G_STRFUNC, tick_stamp - first_stamp - n_computed);
        /* compute features */
        uint n = block_size - n_computed;
        if (n)
          {
            n = MIN (n, n_values);
            compute_probe_state (n, oblock, connected);
            tick_stamp += n;
            oblock += n;
            n_values -= n;
          }
        /* are the features complete yet? */
        if (n_computed == block_size)
          {
            Probe probe;
            complete_probe (probe, channel_id);
            probe_seq += probe;
          }
      }
  }
  void
  queue_probe_request (const ProbeFeatures &channel_features)
  {
    /* update probe ages */
    int max_age = MAX (MAX (requests.range, requests.energie), MAX (requests.samples, requests.fft));
    int old_age = max_age;
    if (channel_features.probe_range)
      max_age = requests.range = MAX_QUEUE_LENGTH;
    if (channel_features.probe_energie)
      max_age = requests.energie = MAX_QUEUE_LENGTH;
    if (channel_features.probe_samples)
      max_age = requests.samples = MAX_QUEUE_LENGTH;
    if (channel_features.probe_fft)
      max_age = requests.fft = MAX_QUEUE_LENGTH;
    /* request new probes */
    if (max_age != old_age && bse_engine_block_size())
      {
        uint n_probes_per_block = MAX (1, (block_size + bse_engine_block_size() - 1) / bse_engine_block_size());
        uint n_requests = max_age + 1;  /* add head room for alignment adjustments */
        queue_probes_update (n_probes_per_block * n_requests);
      }
  }
  ~ProbeQueue()
  {
    g_free (raw_floats);
  }
  ProbeQueue (SourceProbes &_probes,
              uint          _block_size) :
    probes (_probes), block_size (_block_size),
    first_stamp (0), n_computed (0), raw_floats (NULL)
  {
    g_assert (block_size > 0);
    memset (&requests, 0, sizeof (requests));
    reset_probe_state();
  }
  /* SourceProbe keying by block_size */
  struct Key {
    uint block_size;
    Key (uint bs) : block_size (bs) {}
  };
  ProbeQueue (Key key) :
    probes (*(SourceProbes*) NULL), block_size (key.block_size), raw_floats (NULL)
  {}    /* just set up the bare minimum for the destructor and KeyLesser() to work */
  struct KeyLesser {
    inline bool
    operator() (const ProbeQueue *a,
                const ProbeQueue *b) const
    {
      return a->block_size < b->block_size;
    }
  };
private:
  BIRNET_PRIVATE_CLASS_COPY (ProbeQueue);
};


/* --- SourceProbes --- */
class SourceProbes {
  typedef std::set<ProbeQueue*, ProbeQueue::KeyLesser> ProbeQueueSet;
  BseSource            *source;
  vector<ProbeQueueSet> channel_sets;
  SfiRing              *omodules;
  uint                  queued_jobs;
  uint                  required_jobs;
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
  ProbeQueueSet::iterator
  probe_queue_set_begin (guint channel)
  {
    return channel_sets[channel].begin();
  }
  ProbeQueueSet::iterator
  probe_queue_set_end (guint channel)
  {
    return channel_sets[channel].end();
  }
  ProbeQueue*
  get_probe_queue (uint channel,
                   uint block_size)
  {
    ProbeQueue::Key k (block_size);
    ProbeQueue key (k);
    ProbeQueueSet::iterator it = channel_sets[channel].find (&key);
    if (it == probe_queue_set_end (channel))
      {
        std::pair<ProbeQueueSet::iterator,bool> result;
        result = channel_sets[channel].insert (new ProbeQueue (*this, block_size));
        it = result.first;
        g_assert (result.second == true);
      }
    return *it;
  }
public:
  void
  reset_omodules ()
  {
    sfi_ring_free (omodules);
    omodules = NULL;
  }
  SourceProbes (BseSource *src) :
    source (src), channel_sets (BSE_SOURCE_N_OCHANNELS (src)),
    omodules (NULL), queued_jobs (0), required_jobs (0)
  {}
  ~SourceProbes ()
  {
    g_assert (queued_jobs == 0);
    reset_omodules();
    for (uint j = 0; j < channel_sets.size(); j++)
      {
        set<ProbeQueue*>::iterator end = probe_queue_set_end (j), it = probe_queue_set_begin (j);
        while (it != end)
          {
            set<ProbeQueue*>::iterator current = it++;
            ProbeQueue *probe_queue = *current;
            channel_sets[j].erase (current);
            delete probe_queue;
          }
      }
  }
private:
  struct ProbeData {
    BseSource  *source;
    BseOStream *ostreams;
    uint64      debug_stamp;
    uint        debug_n_values;
    uint        n_pending;
    ProbeData (BseSource *_source,
               guint      n_streams) :
      source (_source), ostreams (NULL), debug_stamp (0), debug_n_values (0), n_pending (0)
    {}
    ~ProbeData()
    {
      g_assert (ostreams == NULL);
    }
    BIRNET_PRIVATE_CLASS_COPY (ProbeData);
  };
  void
  handle_probe (ProbeData   &pdata,
                guint        n_values,   /* bse_engine_block_size() */
                guint64      tick_stamp,
                guint        n_ostreams, /* ENGINE_NODE_N_OSTREAMS() */
                BseOStream **ostreams_p)
  {
    g_assert (pdata.n_pending > 0);
    g_assert (n_ostreams == channel_sets.size());
    /* preprocess data from multiple modules */
    if (!pdata.ostreams)                        /* first module */
      {
        /* upon first block, "steal" ostreams */
        g_assert (pdata.ostreams == NULL);
        g_assert (pdata.debug_stamp == 0);
        pdata.ostreams = *ostreams_p;
        *ostreams_p = NULL;
        pdata.debug_n_values = n_values;
        pdata.debug_stamp = tick_stamp;
      }
    else                                        /* successive modules */
      {
        /* add up all successive blocks */
        g_assert (pdata.debug_stamp == tick_stamp);
        g_assert (pdata.debug_n_values == n_values);
        BseOStream *ostreams = *ostreams_p;
        for (uint j = 0; j < n_ostreams; j++)
          if (ostreams[j].connected && channel_sets[j].size() > 0)
            {
              if (pdata.ostreams[j].connected)  /* add to first block */
                bse_block_add_floats (n_values, pdata.ostreams[j].values, ostreams[j].values);
              else                              /* overwrite first block */
                bse_block_copy_float (n_values, pdata.ostreams[j].values, ostreams[j].values);
              pdata.ostreams[j].connected = true;
            }
      }
    /* adjust counter */
    pdata.n_pending--;
    /* hand over data to the probe queues */
    if (!pdata.n_pending)                       /* last module */
      {
        ProbeSeq probe_seq;
        for (uint j = 0; j < n_ostreams; j++)
          if (channel_sets[j].size() > 0)
            {
              set<ProbeQueue*>::iterator it, end = probe_queue_set_end (j);
              for (it = probe_queue_set_begin (j); it != end; it++)
                (*it)->handle_probe_values (probe_seq, n_values, tick_stamp, j,
                                            pdata.ostreams[j].connected ? pdata.ostreams[j].values : bse_engine_const_zeros (n_values),
                                            pdata.ostreams[j].connected);
            }
        /* free "stolen" ostreams */
        bse_engine_free_ostreams (n_ostreams, pdata.ostreams);
        pdata.ostreams = NULL;
        /* notify clients */
        if (probe_seq.length())
          g_signal_emit (source, bse_source_signal_probes, 0, probe_seq.c_ptr());
      }
    if (0)
      g_printerr ("BseProbe: got probe: %d %d (left=%d)\n", n_values, n_ostreams, pdata.n_pending);
  }
  static void
  source_probe_callback (gpointer     data,
                         guint        n_values,
                         guint64      tick_stamp,
                         guint        n_ostreams,
                         BseOStream **ostreams_p)
  {
    ProbeData *pdata = reinterpret_cast<ProbeData*> (data);
    SourceProbes *probes = peek_from_source (pdata->source);
    g_assert (probes != NULL);
    g_assert (probes->queued_jobs > 0);
    g_assert (pdata->n_pending > 0);
    probes->handle_probe (*pdata, n_values, tick_stamp, n_ostreams, ostreams_p);
    if (!pdata->n_pending)
      {
        probes->queued_jobs--;
        delete pdata;
      }
  }
  void
  add_requests (BseTrans *trans)
  {
    SfiRing *ring = get_omodules();
    if (BSE_SOURCE_PREPARED (source) && ring)
      while (queued_jobs < required_jobs)
        {
          ProbeData *pdata = new ProbeData (source, channel_sets.size());
          for (SfiRing *node = ring; node; node = sfi_ring_walk (node, ring))
            {
              bse_trans_add (trans, bse_job_probe_request ((BseModule*) node->data, source_probe_callback, pdata));
              pdata->n_pending++;
            }
          queued_jobs++;
        }
    required_jobs = 0;
  }
  static SfiRing *bse_probe_sources;
  static guint    bse_idle_handler_id;
  static gboolean
  bse_probe_sources_start_assembly (gpointer data)
  {
    /* probe jobs of multiple sources are requested in one transaction
     * to ensure that syncronized probe requests also yield syncronized
     * probe results.
     */
    BseTrans *trans = bse_trans_open();
    while (bse_probe_sources)
      {
        data = sfi_ring_pop_head (&bse_probe_sources);
        BseSource *source = BSE_SOURCE (data);
        SourceProbes *probes = peek_from_source (source);
        if (probes)
          probes->add_requests (trans);
        g_object_unref (source);
      }
    bse_trans_commit (trans);
    bse_idle_handler_id = 0;
    return FALSE;
  }
public:
  void
  queue_probes_update (uint probe_queue_length)
  {
    uint n_jobs = MAX (required_jobs, probe_queue_length);
    if (n_jobs != required_jobs)
      {
        required_jobs = n_jobs;
        if (!sfi_ring_find (bse_probe_sources, source))
          bse_probe_sources = sfi_ring_prepend (bse_probe_sources, g_object_ref (source));
        if (!bse_idle_handler_id)
          bse_idle_handler_id = bse_idle_now (bse_probe_sources_start_assembly, NULL);
      }
  }
  void
  queue_probe_request (guint                 n_channels,
                       const ProbeFeatures **channel_features,
                       guint                 requested_block_size);
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
SfiRing *SourceProbes::bse_probe_sources = NULL;
guint    SourceProbes::bse_idle_handler_id = 0;

void
ProbeQueue::queue_probes_update (uint probe_queue_length)
{
  probes.queue_probes_update (probe_queue_length);
}

/* --- unprepared probing --- */
static SfiRing *bse_dummy_sources = NULL;
static guint    bse_dummy_prober_id = 0;

static gboolean
bse_dummy_prober (gpointer data)
{
  while (bse_dummy_sources)
    {
      data = sfi_ring_pop_head (&bse_dummy_sources);
      BseSource *source = BSE_SOURCE (data);
      ProbeSeq probe_seq;
      g_signal_emit (source, bse_source_signal_probes, 0, probe_seq.c_ptr());
      g_object_unref (source);
    }
  bse_dummy_prober_id = 0;
  return FALSE;
}

void
SourceProbes::queue_probe_request (guint                 n_channels,
                                   const ProbeFeatures **channel_features,
                                   guint                 requested_block_size)
{
  if (!BSE_SOURCE_PREPARED (source) || !get_omodules())
    {
      /* queue for emission of dummy probe signals */
      if (!sfi_ring_find (bse_dummy_sources, source))
        bse_dummy_sources = sfi_ring_append (bse_dummy_sources, g_object_ref (source));
      if (!bse_dummy_prober_id)
        bse_dummy_prober_id = bse_idle_timed (250 * 1000, bse_dummy_prober, NULL);
      return;
    }
  /* queue probes */
  n_channels = MIN (n_channels, channel_sets.size());
  /* update probe ages */
  for (guint i = 0; i < n_channels; i++)
    if (channel_features[i])
      {
        ProbeQueue *pqueue = get_probe_queue (i, requested_block_size);
        pqueue->queue_probe_request (*channel_features[i]);
      }
}

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
  rq.frequency = 1;     // FIXME: remove source_request_probes()
  rq.probe_features = probe_features;
  ProbeRequestSeq prs;
  prs += rq;
  source_mass_request::exec (prs);
}

static guint
fft_align (guint bsize)
{
  bsize = 1 << (g_bit_storage (bsize) - 1);
  return CLAMP (bsize, 4, 65536);
}

void
source_mass_request::exec (const ProbeRequestSeq &cprseq)
{
  struct Sub {
    static bool
    probe_requests_lesser (const ProbeRequestHandle &h1,
                           const ProbeRequestHandle &h2)
    {
      return h2->source < h1->source || h2->frequency < h1->frequency;
    }
  };
  ProbeRequestSeq prs (cprseq);
  /* sort probe-requests, so requests for a single source are adjacent */
  stable_sort (prs.begin(), prs.end(), Sub::probe_requests_lesser);
  BseSource *current = NULL;
  guint current_size = 0;
  bool seen_fft = false;
  const ProbeFeatures **channel_features = NULL;
  for (ProbeRequestSeq::iterator it = prs.begin(); it != prs.end(); it++)
    {
      guint block_size = bse_engine_sample_freq() / CLAMP ((*it)->frequency, 1, 1000) + 0.5;
      if (!(*it)->source)
        continue;       /* can happen due to sources getting destroyed before asyncronous delivery */
      else if ((*it)->source != current || block_size != current_size)
        {               /* open new request list */
          if (current)
            {           /* commit old request list */
              SourceProbes *probes = SourceProbes::create_from_source (current);
              if (seen_fft)
                current_size = fft_align (current_size);
              probes->queue_probe_request (BSE_SOURCE_N_OCHANNELS (current), channel_features, current_size);
              g_free (channel_features);
              channel_features = NULL;
            }
          current = (*it)->source;
          current_size = block_size;
          channel_features = g_new0 (const ProbeFeatures*, BSE_SOURCE_N_OCHANNELS (current));
          seen_fft = false;
        }
      guint channel_id = (*it)->channel_id;
      if (channel_id < BSE_SOURCE_N_OCHANNELS (current))        /* add request */
        {
          channel_features[channel_id] = (*it)->probe_features.c_ptr();
          seen_fft |= channel_features[channel_id]->probe_fft;
        }
    }
  if (current)
    {                   /* commit last request list */
      SourceProbes *probes = SourceProbes::create_from_source (current);
      if (seen_fft)
        current_size = fft_align (current_size);
      probes->queue_probe_request (BSE_SOURCE_N_OCHANNELS (current), channel_features, current_size);
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
  // FIXME: remove: probes->queue_probes_update (1);
}

void
bse_source_class_add_probe_signals (BseSourceClass *klass)
{
  g_assert (bse_source_signal_probes == 0);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  bse_source_signal_probes = bse_object_class_add_signal (object_class, "probes", G_TYPE_NONE, 1, BSE_TYPE_PROBE_SEQ);
}

};
