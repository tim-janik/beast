/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <string.h>
#include <sfi/gbsearcharray.h>
#include "gslosctable.h"

#include "gslcommon.h"
#include "gslmath.h"
#include "gslfft.h"


#define OSC_DEBUG		GSL_DEBUG_FUNCTION (GSL_MSG_OSC, G_STRLOC)

#define	OSC_FREQ_EPSILON	(1e-3)	/* range within which frequencies are "equal" */

/* compare mfreqs against each other, use an arbitrary sample rate
 * for which OSC_FREQ_EPSILON makes sense
 */
#define	CACHE_MATCH_FREQ(usr_mfreq, cache_mfreq) \
  (fabs ((cache_mfreq) * 44107 - (usr_mfreq) * 44107) < OSC_FREQ_EPSILON)


/* --- structures --- */
typedef struct
{
  /* main key (osc and cache tables) */
  gfloat         mfreq;			/* [0..0.5], mix_freq relative */
  /* secondary key (cache tables) */
  GslOscWaveForm wave_form;
  guint8	*filter_func;		/* just here for key indexing */
  /* data */
  guint		 ref_count;
  guint		 min_pos, max_pos;	/* pulse extension */
  guint          n_values;
  const gfloat   values[1];		/* flexible array */
} OscTableEntry;


/* --- prototypes --- */
static gint	cache_table_entry_locs_cmp	(gconstpointer	bsearch_node1, /* key */
						 gconstpointer	bsearch_node2);
static gint	osc_table_entry_locs_cmp	(gconstpointer	bsearch_node1, /* key */
						 gconstpointer	bsearch_node2);
static void	osc_wave_extrema_pos		(guint		n_values,
						 const gfloat *values,
						 guint        *minp_p,
						 guint        *maxp_p);
void		gsl_osc_cache_debug_dump	(void);


/* --- variables --- */
static GBSearchArray       *cache_entries = NULL;
static const GBSearchConfig cache_taconfig = {
  sizeof (OscTableEntry*),
  cache_table_entry_locs_cmp,
  0
};
static const GBSearchConfig osc_taconfig = {
  sizeof (OscTableEntry*),
  osc_table_entry_locs_cmp,
  0
};


/* --- functions --- */
static gint
cache_table_entry_locs_cmp (gconstpointer bsearch_node1, /* key */
			    gconstpointer bsearch_node2)
{
  const OscTableEntry * const *ep1 = bsearch_node1;
  const OscTableEntry * const *ep2 = bsearch_node2;
  const OscTableEntry *e1 = *ep1;
  const OscTableEntry *e2 = *ep2;
  
  if (e1->wave_form == e2->wave_form)
    {
      if (e1->filter_func == e2->filter_func)
	return G_BSEARCH_ARRAY_CMP (e1->mfreq, e2->mfreq);
      else
	return e1->filter_func > e2->filter_func ? 1 : -1;
    }
  else
    return e1->wave_form > e2->wave_form ? 1 : -1;
}

static gint
osc_table_entry_locs_cmp (gconstpointer bsearch_node1, /* key */
			  gconstpointer bsearch_node2)
{
  const OscTableEntry * const *ep1 = bsearch_node1;
  const OscTableEntry * const *ep2 = bsearch_node2;
  const OscTableEntry *e1 = *ep1;
  const OscTableEntry *e2 = *ep2;
  
  return G_BSEARCH_ARRAY_CMP (e1->mfreq, e2->mfreq);
}

static OscTableEntry*
cache_table_entry_lookup_best (GslOscWaveForm wave_form,
			       guint8*        filter_func,
			       gfloat         mfreq)
{
  OscTableEntry key, *k = &key, **ep1 = NULL, **ep2, **ep3 = NULL;
  
  key.mfreq = mfreq;
  key.wave_form = wave_form;
  key.filter_func = filter_func;
  
  /* get exact match or a match which is one off into either direction */
  ep2 = g_bsearch_array_lookup_sibling (cache_entries, &cache_taconfig, &k);
  if (ep2)
    {
      guint i = g_bsearch_array_get_index (cache_entries, &cache_taconfig, ep2);
      
      /* get siblings */
      if (i > 0)
	ep1 = g_bsearch_array_get_nth (cache_entries, &cache_taconfig, i - 1);
      if (i + 1 < g_bsearch_array_get_n_nodes (cache_entries))
	ep3 = g_bsearch_array_get_nth (cache_entries, &cache_taconfig, i + 1);
      
      /* get rid of invalid matches, i.e. ones with:
       * - a different wave
       * - a different filter
       * - a filter wider than required
       */
      if (ep1 && ((*ep1)->wave_form != wave_form ||
		  (*ep1)->filter_func != filter_func ||
		  (*ep1)->mfreq < mfreq))
	ep1 = NULL;
      if (ep3 && ((*ep3)->wave_form != wave_form ||
		  (*ep3)->filter_func != filter_func ||
		  (*ep3)->mfreq < mfreq))
	ep3 = NULL;
      if ((*ep2)->wave_form != wave_form ||
	  (*ep2)->filter_func != filter_func ||
	  (*ep2)->mfreq < mfreq)
	{
	  /* collapse siblings, so that, if we have valid matches, ep2 is amongst them */
	  if (ep1)
	    {
	      ep2 = ep1;
	      ep1 = NULL;
	    }
	  else if (ep3)
	    {
	      ep2 = ep3;
	      ep3 = NULL;
	    }
	  else
	    ep2 = NULL;	/* no valid match at all */
	}
    }
  
  /* now figure best out of valid siblings */
  if (ep2)
    {
      if (ep1 && fabs ((*ep1)->mfreq - mfreq) < fabs ((*ep2)->mfreq - mfreq))
	ep2 = ep1;
      if (ep3 && fabs ((*ep3)->mfreq - mfreq) < fabs ((*ep2)->mfreq - mfreq))
	ep2 = ep3;
    }
  return ep2 ? *ep2 : NULL;
}

static OscTableEntry*
osc_table_entry_lookup_best (const GslOscTable *table,
			     gfloat             mfreq,
			     gfloat	       *min_mfreq)
{
  OscTableEntry key, *k = &key, **ep;
  guint i;
  
  /* get exact match or a match which is one off into either direction */
  key.mfreq = mfreq;
  ep = g_bsearch_array_lookup_sibling (table->entry_array, &osc_taconfig, &k);
  if_reject (!ep)
    return NULL;	/* ugh, bad */
  
  if (mfreq > (*ep)->mfreq)	/* need better filter */
    {
      i = g_bsearch_array_get_index (table->entry_array, &osc_taconfig, ep);
      if (i + 1 < g_bsearch_array_get_n_nodes (table->entry_array))
	ep = g_bsearch_array_get_nth (table->entry_array, &osc_taconfig, i + 1);
      else	/* bad, might cause aliasing */
	OSC_DEBUG ("lookup mismatch, aliasing possible: want_freq=%f got_freq=%f",
		   mfreq * table->mix_freq, (*ep)->mfreq * table->mix_freq);
    }
  
  if (min_mfreq)
    {
      /* fetch mfreq from previous */
      i = g_bsearch_array_get_index (table->entry_array, &osc_taconfig, ep);
      if (i > 0)
	{
	  OscTableEntry **tp = g_bsearch_array_get_nth (table->entry_array, &osc_taconfig, i - 1);
	  
	  *min_mfreq = (*tp)->mfreq;
	}
      else
	*min_mfreq = 0;
    }
  
  return *ep;
}

static guint
wave_table_size (GslOscWaveForm wave_form,
		 gfloat         mfreq)
{
  /* have to return power of 2, and honour 8 <= size */
  
  /* FIXME: decide on other table sizes
  10000: 256
    5000: 512
    2500: 1024
    1250: 2048
    GSL_OSC_WAVE_SAW_FALL always huge buffers to guarantee pulse width stepping granularity
  */

  if (wave_form == GSL_OSC_WAVE_SAW_FALL)
    return 8192;

  return 2048;
}

static void
fft_filter (guint    n_values,
	    gfloat  *values,	/* [0..n_values], n_values/2 complex values */
	    gdouble  scale_window,
	    double (*window) (double))
{
  guint i;

  n_values >>= 1;
  scale_window /= (gdouble) n_values;
  for (i = 0; i <= n_values; i++)
    {
      gdouble w = window (i * scale_window);
      values[i * 2] *= w;
      values[i * 2 + 1] *= w;
    }
}

static OscTableEntry*
cache_table_ref_entry (GslOscWaveForm wave_form,
		       double       (*filter_func) (double),
		       gfloat         mfreq)
{
  OscTableEntry *e = cache_table_entry_lookup_best (wave_form, (guint8*) filter_func, mfreq);
  
  if (e && !CACHE_MATCH_FREQ (mfreq, e->mfreq))
    e = NULL;
  if (!e)
    {
      guint size = wave_table_size (wave_form, mfreq);
      gfloat *values, *fft, step, min, max;

      /* size:
       * - OscTableEntry already contains the first float values
       * - we need n_values+1 adressable floats to provide values[0] == values[n_values]
       */
      e = g_malloc (sizeof (OscTableEntry) + sizeof (gfloat) * size);
      values = (gfloat*) &e->values[0];
      e->wave_form = wave_form;
      e->filter_func = (guint8*) filter_func;
      e->mfreq = mfreq;
      e->ref_count = 1;
      e->n_values = size;
      gsl_osc_wave_fill_buffer (e->wave_form, e->n_values, values);
      
      /* filter wave accordingly */
      gsl_osc_wave_extrema (e->n_values, values, &min, &max);
      fft = g_new (gfloat, e->n_values + 2);	/* [0..n_values] for n_values/2 complex freqs */
      gsl_power2_fftar_simple (e->n_values, values, fft);
      step = e->mfreq * (gdouble) e->n_values;
      fft_filter (e->n_values, fft, step, filter_func);
      gsl_power2_fftsr_simple (e->n_values, fft, values);
      g_free (fft);
      gsl_osc_wave_normalize (e->n_values, values, (min + max) / 2, max);

      /* provide values[0]==values[n_values] */
      values[e->n_values] = values[0];

      /* pulse min/max pos extension */
      osc_wave_extrema_pos (e->n_values, values, &e->min_pos, &e->max_pos);

      /* insert into cache */
      cache_entries = g_bsearch_array_insert (cache_entries, &cache_taconfig, &e);
    }
  else
    e->ref_count++;
  return e;
}

static void
cache_table_unref_entry (OscTableEntry *e)
{
  g_return_if_fail (e->ref_count > 0);
  
  e->ref_count -= 1;
  if (e->ref_count == 0)
    {
      OscTableEntry **ep;
      guint i;
      
      ep = g_bsearch_array_lookup (cache_entries, &cache_taconfig, &e);
      i = g_bsearch_array_get_index (cache_entries, &cache_taconfig, ep);
      cache_entries = g_bsearch_array_remove (cache_entries, &cache_taconfig, i);
    }
}

GslOscTable*
gsl_osc_table_create (gfloat         mix_freq,
		      GslOscWaveForm wave_form,
		      double       (*filter_func) (double),
		      guint          n_freqs,
		      const gfloat  *freqs)
{
  GslOscTable *table;
  gfloat nyquist;
  guint i;
  
  g_return_val_if_fail (mix_freq > 0, NULL);
  g_return_val_if_fail (n_freqs > 0, NULL);
  g_return_val_if_fail (freqs != NULL, NULL);
  
  if (!cache_entries)
    cache_entries = g_bsearch_array_create (&cache_taconfig);
  
  table = sfi_new_struct (GslOscTable, 1);
  table->mix_freq = mix_freq;
  table->wave_form = wave_form;
  table->entry_array = g_bsearch_array_create (&osc_taconfig);
  nyquist = table->mix_freq * 0.5;
  if (wave_form == GSL_OSC_WAVE_PULSE_SAW)
    wave_form = GSL_OSC_WAVE_SAW_FALL;
  for (i = 0; i < n_freqs; i++)
    {
      OscTableEntry *e;
      gdouble mfreq = MIN (nyquist, freqs[i]);
      
      mfreq /= table->mix_freq;
      e = osc_table_entry_lookup_best (table, mfreq, NULL);
      if (!e || fabs (e->mfreq * table->mix_freq - mfreq * table->mix_freq) > OSC_FREQ_EPSILON)
	{
	  e = cache_table_ref_entry (wave_form, filter_func, mfreq);
	  table->entry_array = g_bsearch_array_insert (table->entry_array, &osc_taconfig, &e);
	}
      else if (e)
	OSC_DEBUG ("not inserting existing entry (freq=%f) for freq %f (nyquist=%f)",
		   e->mfreq * table->mix_freq, mfreq * table->mix_freq, nyquist);
    }
  
  return table;
}

void
gsl_osc_table_lookup (const GslOscTable	*table,
		      gfloat		 freq,
		      GslOscWave	*wave)
{
  OscTableEntry *e;
  gfloat mfreq, min_mfreq;

  g_return_if_fail (table != NULL);
  g_return_if_fail (wave != NULL);
  
  mfreq = freq / table->mix_freq;
  e = osc_table_entry_lookup_best (table, mfreq, &min_mfreq);
  if (e)
    {
      guint32 int_one;
      gfloat float_one;

      wave->min_freq = min_mfreq * table->mix_freq;
      wave->max_freq = e->mfreq * table->mix_freq;
      wave->n_values = e->n_values;
      wave->values = e->values;
      wave->n_frac_bits = g_bit_storage (wave->n_values - 1);
      wave->n_frac_bits = 32 - wave->n_frac_bits;
      int_one = 1 << wave->n_frac_bits;
      wave->frac_bitmask = int_one - 1;
      float_one = int_one;
      wave->freq_to_step = float_one * wave->n_values / table->mix_freq;
      wave->phase_to_pos = wave->n_values * float_one;
      wave->ifrac_to_float = 1.0 / float_one;
      /* pulse min/max pos extension */
      wave->min_pos = e->min_pos;
      wave->max_pos = e->max_pos;
    }
  else
    {
      /* shouldn't happen */
      OSC_DEBUG ("table lookup revealed NULL, empty table?");
      memset (wave, 0, sizeof (*wave));
    }
}

void
gsl_osc_table_free (GslOscTable *table)
{
  guint n;
  
  g_return_if_fail (table != NULL);
  
  n = g_bsearch_array_get_n_nodes (table->entry_array);
  while (n--)
    {
      OscTableEntry **ep;
      
      ep = g_bsearch_array_get_nth (table->entry_array, &osc_taconfig, n);
      cache_table_unref_entry (*ep);
      table->entry_array = g_bsearch_array_remove (table->entry_array, &osc_taconfig, n);
    }
  g_bsearch_array_free (table->entry_array, &osc_taconfig);
  sfi_delete_struct (GslOscTable, table);
}

void
gsl_osc_cache_debug_dump (void)
{
  OSC_DEBUG ("left in cache: %u", g_bsearch_array_get_n_nodes (cache_entries));
}

void
gsl_osc_wave_fill_buffer (GslOscWaveForm type,
			  guint	         n_values,
			  gfloat	*values)
{
  gdouble max = n_values, hmax = max * 0.5, qmax = n_values * 0.25;
  gint i, half = n_values / 2, quarter = half / 2;
  
  switch (type)
    {
      gdouble frac, pos;
    case GSL_OSC_WAVE_SINE:
      for (i = 0; i < n_values; i++)
	{
	  frac = ((gdouble) i) / max; /* [0..1[ */
	  pos = frac * 2. * GSL_PI;
	  values[i] = sin (pos);
	}
      break;
    case GSL_OSC_WAVE_SAW_RISE:
      for (i = 0; i < n_values; i++)
	{
	  frac = ((gdouble) i) / max; /* [0..1[ */
	  values[i] = 2.0 * frac - 1.0;
	}
      break;
    case GSL_OSC_WAVE_SAW_FALL:
      for (i = 0; i < n_values; i++)
	{
	  frac = ((gdouble) i) / max; /* [0..1[ */
	  values[i] = 1.0 - 2.0 * frac;
	}
      break;
    case GSL_OSC_WAVE_PEAK_RISE:	/* spaced saw */
      for (i = 0; i < half; i++)
	{
	  frac = ((gdouble) i) / hmax;
          values[i] = 2.0 * frac - 1.0;
	}
      for (; i < n_values; i++)
	values[i] = -1.0;
      break;
    case GSL_OSC_WAVE_PEAK_FALL:	/* spaced saw */
      for (i = 0; i < half; i++)
	{
	  frac = ((gdouble) i) / hmax;
          values[i] = 1.0 - 2.0 * frac;
	}
      for (; i < n_values; i++)
	values[i] = -1.0;
      break;
    case GSL_OSC_WAVE_TRIANGLE:
      for (i = 0; i < quarter; i++)
	{
	  frac = ((gdouble) i) / qmax;
	  values[i] = frac;
	}
      for (; i < half + quarter; i++)
	{
	  frac = ((gdouble) i - quarter) / hmax;
          values[i] = 1.0 - 2.0 * frac;
	}
      for (; i < n_values; i++)
	{
	  frac = ((gdouble) i - half - quarter) / qmax;
	  values[i] = frac - 1.0;
	}
      break;
    case GSL_OSC_WAVE_MOOG_SAW:
      for (i = 0; i < half; i++)
	{
	  frac = ((gdouble) i) / hmax;
          values[i] = 2.0 * frac - 1.0;
	}
      for (; i < n_values; i++)
	{
	  frac = ((gdouble) i) / max;
          values[i] = 1.0 - 2.0 * frac;
	}
      break;
    case GSL_OSC_WAVE_SQUARE:
      for (i = 0; i < half; i++)
	values[i] = 1.0;
      for (; i < n_values; i++)
	values[i] = -1.0;
      break;
    default:
      g_critical ("%s: invalid wave form id (%u)", G_STRLOC, type);
    case GSL_OSC_WAVE_NONE:
      for (i = 0; i < n_values; i++)
	values[i] = 0;
      break;
    }
}

static void
osc_wave_extrema_pos (guint          n_values,
		      const gfloat *values,
		      guint        *minp_p,
		      guint        *maxp_p)
{
  guint i, minp = 0, maxp = 0;
  gfloat min = values[0], max = min;

  for (i = 1; i < n_values; i++)
    {
      if (values[i] > max)
	{
	  max = values[i];
	  maxp = i;
	}
      else if (values[i] < min)
	{
	  min = values[i];
	  minp = i;
	}
    }
  *minp_p = minp;
  *maxp_p = maxp;
}

void
gsl_osc_wave_extrema (guint         n_values,
		      const gfloat *values,
		      gfloat       *min_p,
		      gfloat       *max_p)
{
  guint minp, maxp;

  g_return_if_fail (n_values > 0 && values != NULL && min_p != NULL && max_p != NULL);

  osc_wave_extrema_pos (n_values, values, &minp, &maxp);
  *min_p = values[minp];
  *max_p = values[maxp];
}

void
gsl_osc_wave_adjust_range (guint   n_values,
			   gfloat *values,
			   gfloat  min,
			   gfloat  max,
			   gfloat  new_center,
			   gfloat  new_max)
{
  gfloat center;
  guint i;
  
  g_return_if_fail (n_values > 0 && values != NULL);

  center = (min + max) / 2;
  center = new_center - center;
  min = fabs (min + center);
  max = fabs (max + center);
  if (min > max)
    max = min;
  if (max > GSL_FLOAT_MIN_NORMAL)
    max = new_max / max;
  else
    max = 0;
  for (i = 0; i < n_values; i++)
    values[i] = (values[i] + center) * max;
}

void
gsl_osc_wave_normalize (guint   n_values,
			gfloat *values,
			gfloat  new_center,
			gfloat  new_max)
{
  gfloat min, max;
  guint i;

  g_return_if_fail (n_values > 0 && values != NULL);

  min = values[0];
  max = min;
  for (i = 1; i < n_values; i++)
    {
      register gfloat v = values[i];

      max = MAX (max, v);
      min = MIN (min, v);
    }

  gsl_osc_wave_adjust_range (n_values, values, min, max, new_center, new_max);
}
