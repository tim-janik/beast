/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Tim Janik
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
#include <gsl/gslmultiwave.h>


#include <math.h>



/* --- functions --- */
static GslDataLocator*
mwave_dloc_from_cache (GslMultiWave         *mwave,
		       const GslDataLocator *key)
{
  GslDataLocator *dloc;
  GslRing *ring;
  
  for (ring = mwave->dloc_cache; ring; ring = gsl_ring_walk (mwave->dloc_cache, ring))
    {
      dloc = ring->data;
      if (key->n_values == dloc->n_values && key->mtime == dloc->mtime &&
	  strcmp (key->name, dloc->name) == 0)
	return dloc;
    }
  dloc = gsl_new_struct (GslDataLocator, 1);
  dloc->name = g_strdup (key->name);
  dloc->mtime = key->mtime;
  dloc->n_values = key->n_values;
  mwave->dloc_cache = gsl_ring_prepend (mwave->dloc_cache, dloc);

  return dloc;
}

GslMultiWave*
gsl_multi_wave_new (const gchar          *name,
		    const GslDataLocator *sloc)
{
  GslMultiWave *mwave;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (sloc != NULL, NULL);
  g_return_val_if_fail (sloc->name != NULL, NULL);
  g_return_val_if_fail (sloc->mtime > 0, NULL);

  mwave = gsl_new_struct (GslMultiWave, 1);
  mwave->name = g_strdup (name);
  mwave->ref_count = 1;
  mwave->frozen = 0;
  mwave->wave_chunks = NULL;
  mwave->dloc_cache = NULL;
  mwave->multi_wave_location = mwave_dloc_from_cache (mwave, sloc);

  return mwave;
}

static void
free_mwave (GslMultiWave *mwave)
{
  GslRing *ring;

  for (ring = mwave->wave_chunks; ring; ring = gsl_ring_walk (mwave->wave_chunks, ring))
    {
      GslWaveChunk *wchunk = ring->data;

      if (wchunk->dcache)
	_gsl_wave_chunk_close (wchunk);
      _gsl_wave_chunk_uninit (wchunk);
      gsl_delete_struct (GslWaveChunk, 1, wchunk);
    }
  gsl_ring_free (mwave->wave_chunks);
  for (ring = mwave->dloc_cache; ring; ring = gsl_ring_walk (mwave->dloc_cache, ring))
    {
      GslDataLocator *dloc = ring->data;

      g_free (dloc->name);
      gsl_delete_struct (GslDataLocator, 1, dloc);
    }
  gsl_ring_free (mwave->dloc_cache);
  g_free (mwave->name);
  gsl_delete_struct (GslMultiWave, 1, mwave);
}

GslMultiWave*
gsl_multi_wave_ref (GslMultiWave *mwave)
{
  g_return_val_if_fail (mwave != NULL, NULL);
  g_return_val_if_fail (mwave->ref_count > 0, NULL);

  mwave->ref_count++;

  return mwave;
}

void
gsl_multi_wave_unref (GslMultiWave *mwave)
{
  g_return_if_fail (mwave != NULL);
  g_return_if_fail (mwave->ref_count > 0);

  mwave->ref_count--;
  if (!mwave->ref_count)
    free_mwave (mwave);
}

GslWaveChunk*
gsl_multi_wave_add_chunk (GslMultiWave	       *mwave,
			  const GslDataLocator *wcdata_loc,
			  GslDataReader        *reader,
			  gsize                 offset,
			  gsize                 length,
			  guint			n_channels,
			  gfloat                mix_freq,
			  gfloat                osc_freq)
{
  GslWaveChunk *wchunk;

  g_return_val_if_fail (mwave != NULL, NULL);
  g_return_val_if_fail (mwave->frozen == 0, NULL);
  g_return_val_if_fail (wcdata_loc != NULL, NULL);
  g_return_val_if_fail (reader != NULL, NULL);
  g_return_val_if_fail (n_channels > 0, NULL);

  wchunk = gsl_new_struct0 (GslWaveChunk, 1);
  _gsl_wave_chunk_init (wchunk, mwave_dloc_from_cache (mwave, wcdata_loc),
			reader, offset, length, n_channels, mix_freq, osc_freq);
  mwave->wave_chunks = gsl_ring_append (mwave->wave_chunks, wchunk);

  return wchunk;
}

GslWaveChunk*
gsl_multi_wave_match_chunk (GslMultiWave *mwave,
			    guint         n_channels,
			    gfloat        mix_freq,
			    gfloat        mix_freq_epsilon,
			    gfloat        osc_freq,
			    gfloat        osc_freq_epsilon)
{
  GslRing *ring;
  GslWaveChunk *best_wchunk = NULL;
  gfloat best;

  g_return_val_if_fail (mwave != NULL, NULL);

  mix_freq_epsilon = fabs (mix_freq_epsilon);
  osc_freq_epsilon = fabs (osc_freq_epsilon);
  best = (osc_freq_epsilon + 2.0 * mix_freq_epsilon);
  best *= best;
  for (ring = mwave->wave_chunks; ring; ring = gsl_ring_walk (mwave->wave_chunks, ring))
    {
      GslWaveChunk *wchunk = ring->data;
      gfloat odiff, mdiff, goodness;

      odiff = fabs (osc_freq - wchunk->osc_freq);
      mdiff = fabs (mix_freq - wchunk->mix_freq);
      if (n_channels != wchunk->n_channels || odiff > osc_freq_epsilon || mdiff > mix_freq_epsilon)
	continue;

      goodness = (odiff + 2.0 * mdiff);
      goodness *= goodness;
      if (goodness <= best)
	{
	  best = goodness;
	  best_wchunk = wchunk;
	}
    }
  return best_wchunk;
}

void
gsl_multi_wave_set_chunk_loop   (GslMultiWave   *mwave,
				 GslWaveChunk   *wchunk,
				 GslWaveLoopType loop_type,
				 GslLong         loop_start,
				 GslLong         loop_end,
				 guint           loop_count)
{
  g_return_if_fail (mwave != NULL);
  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (mwave->frozen == 0);

  _gsl_wave_chunk_set_loop (wchunk, loop_type, loop_start, loop_end, loop_count);
}

void
gsl_multi_wave_freeze (GslMultiWave *mwave)
{
  g_return_if_fail (mwave != NULL);
  g_return_if_fail (mwave->ref_count > 0);

  if (!mwave->frozen)
    gsl_multi_wave_ref (mwave);
  mwave->frozen++;
}

void
gsl_multi_wave_thaw (GslMultiWave *mwave)
{
  g_return_if_fail (mwave != NULL);
  g_return_if_fail (mwave->frozen > 0);

  mwave->frozen--;
  if (!mwave->frozen)
    gsl_multi_wave_unref (mwave);
}

void
gsl_multi_wave_open_chunk (GslMultiWave *mwave,
			   GslWaveChunk *wchunk)
{
  g_return_if_fail (mwave != NULL);
  g_return_if_fail (wchunk != NULL);

  if (!wchunk->dcache)
    {
      gsl_multi_wave_freeze (mwave);
      _gsl_wave_chunk_open (wchunk);
    }
}

void
gsl_multi_wave_close_chunk (GslMultiWave *mwave,
			    GslWaveChunk *wchunk)
{
  g_return_if_fail (mwave != NULL);
  g_return_if_fail (wchunk != NULL);
  g_return_if_fail (mwave->frozen > 0);

  if (!wchunk->dcache)
    gsl_multi_wave_thaw (mwave);
}
