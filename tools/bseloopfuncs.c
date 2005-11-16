/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2001, 2003 Tim Janik and Stefan Westerfeld
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
#include "bseloopfuncs.h"
#include <bse/gsldatacache.h>
#include <string.h>

typedef struct {
  gdouble score;
  GslLong loop_start;
  GslLong loop_length;
} LoopEntry;
typedef struct {
  guint      max_entries;
  guint      n_entries;
  gdouble    worst_score;
  LoopEntry  entries[1];        /* flexible array */
} LoopStack;

static gdouble
score_headloop (GslDataHandle *dhandle,
                const gfloat  *ls, /* loop start */
                GslLong        ll, /* loop length */
                GslLong        cl, /* compare area length */
                gdouble        max_score)
{
  GslLong nl = cl / ll;         /* number of full loop comparisons */
  GslLong rl = cl - nl * ll;    /* fraction of last comparison */
  const gfloat *c = ls + ll;    /* compare area start */
  const gfloat *p, *le = ls + ll;
  gdouble score = 0;

  /* compute score for loop repeated over comparison area
   * .......|##########|-------------------------|......
   *        ls         c                        c+cl
   */

  while (nl--)
    {
      p = ls;
      while (p < le)
        {
          gdouble tmp = *p++ - *c++;
          GSL_GCC_PREFETCH (p);
          GSL_GCC_PREFETCH (c);
          score += tmp * tmp;
        }
      if (score > max_score)
        return score;
    }
  le = ls + rl;
  p = ls;
  while (p < le)
    {
      gdouble tmp = *p++ - *c++;
      GSL_GCC_PREFETCH (p);
      GSL_GCC_PREFETCH (c);
      score += tmp * tmp;
    }
  return score;
}

static gdouble
score_tailloop (GslDataHandle *dhandle,
                const gfloat  *cs, /* compare area start */
                GslLong        cl, /* compare area length */
                GslLong        ll, /* loop length */
                gdouble        max_score)
{
  GslLong nl = cl / ll;         /* number of full loop comparisons */
  GslLong rl = cl - nl * ll;    /* fraction of last comparison */
  const gfloat *ls = cs + cl;   /* loop start */
  const gfloat *le = ls + ll;
  const gfloat *p, *c = cs;
  gdouble score = 0;

  /* compute score for loop repeated over comparison area
   * .......|-------------------------|##########|......
   *        c                         ls         le
   */

  p = ls + ll - rl;
  while (p < le)
    {
      gdouble tmp = *p++ - *c++;
      GSL_GCC_PREFETCH (p);
      GSL_GCC_PREFETCH (c);
      score += tmp * tmp;
    }
  while (nl--)
    {
      if (score > max_score)
        return score;
      p = ls;
      while (p < le)
        {
          gdouble tmp = *p++ - *c++;
          GSL_GCC_PREFETCH (p);
          GSL_GCC_PREFETCH (c);
          score += tmp * tmp;
        }
    }
  return score;
}

gboolean
gsl_data_find_loop5 (GslDataHandle     *dhandle,
                     GslDataLoopConfig *config,
                     gpointer           pdata,
                     GslProgressFunc    pfunc)
{
  GslLong bstart, blength, min_llength, max_llength, i, dhandle_n_values, clength, pcount, score_pcount = 0;
  GslLong frame = 441; // FIXME: need assertion for frame vs. block length
  GslProgressState pstate = gsl_progress_state (pdata, pfunc, 1);
  GslDataPeekBuffer pbuf = { +1, };
  gdouble pdist, fcenter, bfrac;
  guint apoints;
  gfloat *block;
  gboolean found_loop = FALSE;

  g_return_val_if_fail (dhandle != NULL, FALSE);
  g_return_val_if_fail (config != NULL, FALSE);
  g_return_val_if_fail (frame <= config->block_start, FALSE);
  config->n_details = 0;

  /* check out data handle */
  if (gsl_data_handle_open (dhandle) != BSE_ERROR_NONE)
    return FALSE;
  dhandle_n_values = gsl_data_handle_n_values (dhandle);

  /* confine parameters */
  bstart = CLAMP (config->block_start, 0, dhandle_n_values - 1);
  if (config->block_length < 0)
    blength = dhandle_n_values - bstart - frame;
  else
    blength = MIN (dhandle_n_values - bstart - frame, config->block_length);
  if (blength < 4)
    return FALSE;

  /* determine boundaries */
  max_llength = blength / CLAMP (config->repetitions, 2, blength);
  min_llength = MAX (frame + 1, config->min_loop);
  if (min_llength > max_llength)
    return FALSE;
  /* determine frame distances */
  apoints = CLAMP (config->analysis_points, 1, blength / 2 - 1);
  bfrac = blength / (apoints + 1.0);
  /* length of comparison area */
  clength = blength / 2;

  /* provide fully cached area for comparisons */
  block = g_new (gfloat, dhandle_n_values);
  for (i = 0; i < dhandle_n_values; i++)
    block[i] = gsl_data_handle_peek_value (dhandle, i, &pbuf);

  /* upper boundary for amount of comparisons */
  pdist = apoints * (max_llength + 1 - min_llength);
  pcount = 0;
  config->score = G_MAXDOUBLE;

  /* loop over the centers of all loops */
  for (fcenter = bstart + bfrac; fcenter + 1.0 < bstart + blength; fcenter += bfrac)
    {
      GslLong ipp; // llength;
      /* loop over all loop lengths */
      // for (llength = min_llength; llength <= max_llength; llength++)
      for (ipp = min_llength; ipp <= max_llength - min_llength; ipp++)
        {
          /* for better load balancing, we do simple size alterations and don't loop
           * from min to max loop length but loop from both ends in turn (ipp -> llength)
           */
          GslLong llength = ipp & 1 ? max_llength - ipp / 2 : min_llength + ipp / 2;
          GslLong hstart, hlength, tstart, tlength;
          gdouble weight;
          /* determine loop center as 0-relative position */
          GslLong lstart = fcenter - 0.5;
          /* offset loop around center */
          lstart -= (llength - 1) >> 1;
          /* update progress (inner loop) counter */
          pcount++;
          /* confine to block boundaries */
          if (lstart < bstart || lstart + llength > bstart + blength)
            continue;
          /* center head/tail comparison areas around loop */
          hstart = lstart - clength / 2;
          hlength = clength / 2;
          tstart = lstart + llength;
          tlength = clength / 2;
          /* shift head/tail if either exceeds boundaries */
          if (hstart < bstart)
            {
              GslLong diff = bstart - hstart;
              hstart += diff;
              hlength -= diff;
              tlength += diff;
            }
          else if (tstart + tlength > bstart + blength)
            {
              GslLong diff = tstart + tlength - bstart - blength;
              tlength -= diff;
              hstart -= diff;
              hlength += diff;
            }
          /* accumulate score */
          weight = 1.0 * (tlength + hlength) / (gdouble) frame;
          double score1 = weight * score_tailloop (dhandle, block + lstart - frame, frame, llength, config->score);
          score1       += weight * score_headloop (dhandle, block + lstart, llength, frame, config->score - score1);
          double score2 = score_headloop (dhandle, block + lstart, llength, tlength, config->score - score1);
          score2       += score_tailloop (dhandle, block + hstart, hlength, llength, config->score - score1 - score2);
	  double score  = score1 + score2;
          /* apply score */
          if (score < config->score)
            {
              config->loop_start = lstart;
              config->loop_length = llength;
              config->score = score;
	      config->n_details = 2;
	      config->detail_names[0] = "score1 (proximity score)";
	      config->detail_names[1] = "score2 (loop comparision)";
	      config->detail_scores[0] = score1;
	      config->detail_scores[1] = score2;
              score_pcount = pcount;
              found_loop = TRUE;
            }
          gsl_progress_notify (&pstate, pcount * 100.0 / pdist, "score:%+g len:%ld (pos:%6lu len=%ld)",
                               config->score, config->loop_length, lstart, llength);
        }
    }
  gsl_progress_wipe (&pstate);
  g_printerr ("  LOOP: %6lu - %6lu [%6lu] (block: %6lu - %6lu [%6lu]) (score:%+g at:%5.1f%%)\n",
              config->loop_start, config->loop_start + config->loop_length, config->loop_length,
              bstart, bstart + blength, blength,
              config->score, score_pcount * 100.0 / pdist);

  /* cleanups */
  g_free (block);
  gsl_data_handle_close (dhandle);

  return found_loop;
}

gboolean
gsl_data_find_loop4 (GslDataHandle     *dhandle,
                     GslDataLoopConfig *config,
                     gpointer           pdata,
                     GslProgressFunc    pfunc)
{
  GslLong bstart, blength, min_llength, max_llength, i, dhandle_n_values, clength, llength, pcount, score_pcount = 0;
  GslProgressState pstate = gsl_progress_state (pdata, pfunc, 1);
  GslDataPeekBuffer pbuf = { +1, };
  gdouble pdist, fcenter, bfrac;
  gfloat *bstart_block;
  const gfloat *block;
  gboolean found_loop = FALSE;

  g_return_val_if_fail (dhandle != NULL, FALSE);
  g_return_val_if_fail (config != NULL, FALSE);
  config->n_details = 0;

  /* check out data handle */
  if (gsl_data_handle_open (dhandle) != BSE_ERROR_NONE)
    return FALSE;
  dhandle_n_values = gsl_data_handle_n_values (dhandle);

  /* confine parameters */
  bstart = CLAMP (config->block_start, 0, dhandle_n_values - 1);
  if (config->block_length < 0)
    blength = dhandle_n_values - bstart;
  else
    blength = MIN (dhandle_n_values - bstart, config->block_length);
  if (blength < 4)
    return FALSE;

  /* determine boundaries */
  max_llength = blength / CLAMP (config->repetitions, 2, blength);
  min_llength = MAX (1, config->min_loop);
  if (min_llength > max_llength)
    return FALSE;
  /* determine frame distances */
  bfrac = blength / (CLAMP (config->analysis_points, 1, blength / 2 - 1) + 1.0);
  /* length of comparison area */
  clength = blength / 2;

  /* provide fully cached area for comparisons */
  bstart_block = g_new (gfloat, blength);
  for (i = 0; i < blength; i++)
    bstart_block[i] = gsl_data_handle_peek_value (dhandle, bstart + i, &pbuf);
  block = bstart_block - bstart;

  /* upper boundary for amount of comparisons */
  pdist = (blength / bfrac + 0.5) * (max_llength + 1 - min_llength);
  pcount = 0;
  config->score = G_MAXDOUBLE;

  /* loop over the centers of all loops */
  for (fcenter = bstart + bfrac; fcenter + 1.0 < bstart + blength; fcenter += bfrac)
    /* loop over all loop lengths */
    for (llength = min_llength; llength <= max_llength; llength += 1)
      {
        GslLong hstart, hlength, tstart, tlength;
        gdouble score = 0;
        /* determine loop center as 0-relative position */
        GslLong lstart = fcenter - 0.5;
        /* offset loop around center */
        lstart -= (llength - 1) >> 1;
        /* update progress (inner loop) counter */
        pcount++;
        /* confine to block boundaries */
        if (lstart < bstart || lstart + llength > bstart + blength)
          break;
        /* center head/tail comparison areas around loop */
        hstart = lstart - clength / 2;
        hlength = clength / 2;
        tstart = lstart + llength;
        tlength = clength / 2;
        /* shift head/tail if either exceeds boundaries */
        if (hstart < bstart)
          {
            GslLong diff = bstart - hstart;
            hstart += diff;
            hlength -= diff;
            tlength += diff;
          }
        else if (tstart + tlength > bstart + blength)
          {
            GslLong diff = tstart + tlength - bstart - blength;
            tlength -= diff;
            hstart -= diff;
            hlength += diff;
          }
        /* accumulate score */
        score += score_headloop (dhandle, block + lstart, llength, tlength, config->score);
        score += score_tailloop (dhandle, block + hstart, hlength, llength, config->score - score);
        /* apply score */
        if (score < config->score)
          {
            config->loop_start = lstart;
            config->loop_length = llength;
            config->score = score;
            score_pcount = pcount;
            found_loop = TRUE;
          }
        gsl_progress_notify (&pstate, pcount * 100.0 / pdist, "score:%+g len:%ld (pos:%6lu len=%ld)",
                             config->score, config->loop_length, lstart, llength);
      }
  gsl_progress_wipe (&pstate);
  g_printerr ("  LOOP: %6lu - %6lu [%6lu] (block: %6lu - %6lu [%6lu]) (score:%+g at:%5.1f%%)\n",
              config->loop_start, config->loop_start + config->loop_length, config->loop_length,
              bstart, bstart + blength, blength,
              config->score, score_pcount * 100.0 / pdist);

  /* cleanups */
  g_free (bstart_block);
  gsl_data_handle_close (dhandle);

  return found_loop;
}

gboolean
gsl_data_find_loop3 (GslDataHandle     *dhandle,
                     GslDataLoopConfig *config,
                     gpointer           pdata,
                     GslProgressFunc    pfunc)
{
  GslProgressState pstate = gsl_progress_state (pdata, pfunc, 4);
  GslDataPeekBuffer pbuf = { +1, };
  GslLong minll, maxll, i, dhandle_n_values, pcount;
  gfloat *sp, *ep, *cstart, *block;
  gdouble pdist;
  gboolean found_loop = FALSE;

  g_return_val_if_fail (dhandle != NULL, FALSE);
  g_return_val_if_fail (config != NULL, FALSE);
  config->n_details = 0;

  if (gsl_data_handle_open (dhandle) != BSE_ERROR_NONE)
    return FALSE;
  dhandle_n_values = gsl_data_handle_n_values (dhandle);
  config->block_start = CLAMP (config->block_start, 0, dhandle_n_values - 1);
  if (config->block_length < 0)
    config->block_length = dhandle_n_values - config->block_start;
  else
    config->block_length = MIN (config->block_length, dhandle_n_values - config->block_start);
  if (config->block_length < 2)
    return FALSE;
  if (config->repetitions != CLAMP (config->repetitions, 2, config->block_length))
    return FALSE;
  /* current implementation supports just repetitions == 2 */
  g_return_val_if_fail (config->repetitions == 2, FALSE);

  /* provide fully cached area for comparisons */
  block = g_new (gfloat, config->block_length);
  for (i = 0; i < config->block_length; i++)
    block[i] = gsl_data_handle_peek_value (dhandle, config->block_start + i, &pbuf);

  /* test every possible loop size at every possible position */
  maxll = config->block_length / 2;
  minll = maxll * 0.91;
  g_return_val_if_fail (maxll > minll, FALSE); // FIXME
  cstart = block + config->block_length / 2;
  config->score = G_MAXDOUBLE;
  pcount = 0, pdist = (maxll * 1.0 - minll * 1.0 + 2.0) * (maxll * 1.0 - minll * 1.0 + 1.0) / 2.;
  g_printerr ("pdist: %f\n", pdist);
  for (sp = block; sp < cstart - minll; sp++)
    for (ep = sp + minll; ep < cstart; ep++)
      {
        gdouble score = score_headloop (dhandle, sp, ep - sp, config->block_length / 2, config->score);
        if (score <= config->score)
          {
            config->loop_start = sp - block;
            config->loop_length = ep - sp;
            config->score = score;
            found_loop = TRUE;
          }
        gsl_progress_notify (&pstate, ++pcount * 100.0 / pdist, "score:%+g (pos:%6lu)", config->score, sp - block);
        // g_printerr ("processed: %7.3f (score:%+g)            \r", ++pcount * 100.0 / pdist, config->score);
      }

  g_free (block);
  gsl_data_handle_close (dhandle);
  return found_loop;
}

gboolean
gsl_data_find_loop2 (GslDataHandle     *dhandle,
                     GslDataLoopConfig *config,
                     gpointer           pdata,
                     GslProgressFunc    pfunc)
{
  GslProgressState pstate = gsl_progress_state (pdata, pfunc, 4);
  GslDataPeekBuffer pbuf = { +1, };
  GslLong minll, maxll, i, dhandle_n_values, pcount, ll;
  gfloat *sp, *ep, *cstart, *block;
  gdouble pdist;
  gboolean found_loop = FALSE;

  g_return_val_if_fail (dhandle != NULL, FALSE);
  g_return_val_if_fail (config != NULL, FALSE);
  config->n_details = 0;

  if (gsl_data_handle_open (dhandle) != BSE_ERROR_NONE)
    return FALSE;
  dhandle_n_values = gsl_data_handle_n_values (dhandle);
  config->block_start = CLAMP (config->block_start, 0, dhandle_n_values - 1);
  if (config->block_length < 0)
    config->block_length = dhandle_n_values - config->block_start;
  else
    config->block_length = MIN (config->block_length, dhandle_n_values - config->block_start);
  if (config->block_length < 2)
    return FALSE;
  if (config->repetitions != CLAMP (config->repetitions, 2, config->block_length))
    return FALSE;
  /* current implementation supports just repetitions == 2 */
  g_return_val_if_fail (config->repetitions == 2, FALSE);

  /* provide fully cached area for comparisons */
  block = g_new (gfloat, config->block_length);
  for (i = 0; i < config->block_length; i++)
    block[i] = gsl_data_handle_peek_value (dhandle, config->block_start + i, &pbuf);

  goto print_sizes;

  /* find best loop size at one position */
  maxll = config->block_length / 2;
  minll = 1;
  g_return_val_if_fail (maxll > minll, FALSE); // FIXME
  cstart = block + config->block_length / 2;
  config->score = G_MAXDOUBLE;
  pcount = 0, pdist = (maxll * 1.0 - minll * 1.0 + 2.0) * (maxll * 1.0 - minll * 1.0 + 1.0) / 2.;
  sp = block;
  for (ep = sp + minll; ep < cstart; ep++)
    {
      gdouble score = score_headloop (dhandle, sp, ep - sp, config->block_length / 2, config->score);
      if (score <= config->score)
        {
          config->loop_start = sp - block;
          config->loop_length = ep - sp;
          config->score = score;
          found_loop = TRUE;
        }
      gsl_progress_notify (&pstate, ++pcount * 100.0 / pdist, "score:%+g (pos:%6lu)", config->score, sp - block);
    }
  gsl_progress_wipe (&pstate);
  ll = config->loop_length;
  g_printerr ("loop size: %lu\n", ll);

  /* test every possible position */
  minll = ll;
  maxll = ll + 1;
  g_return_val_if_fail (maxll > minll, FALSE); // FIXME
  cstart = block + config->block_length / 2;
  config->score = G_MAXDOUBLE;
  pcount = 0, pdist = (maxll * 1.0 - minll * 1.0 + 2.0) * (maxll * 1.0 - minll * 1.0 + 1.0) / 2.;
  g_printerr ("pdist: %f\n", pdist);
  for (sp = block; sp < cstart - minll; sp++)
    {
      ep = sp + minll;
      {
        gdouble score = score_headloop (dhandle, sp, ep - sp, config->block_length / 2, G_MAXDOUBLE);
        g_print ("%u %.17g\n", sp - block, score);
        continue;
        if (score <= config->score)
          {
            config->loop_start = sp - block;
            config->loop_length = ep - sp;
            config->score = score;
            found_loop = TRUE;
          }
        gsl_progress_notify (&pstate, ++pcount * 100.0 / pdist, "score:%+g (pos:%6lu)", config->score, sp - block);
      }
    }

  G_BREAKPOINT ();
 print_sizes:

  /* test every possible loop size */
  maxll = config->block_length / 2;
  minll = 1;
  g_return_val_if_fail (maxll > minll, FALSE); // FIXME
  cstart = block + config->block_length / 2;
  config->score = G_MAXDOUBLE;
  pcount = 0, pdist = (maxll * 1.0 - minll * 1.0 + 2.0) * (maxll * 1.0 - minll * 1.0 + 1.0) / 2.;
  g_printerr ("pdist: %f\n", pdist);
  for (sp = block + 99999; sp < cstart - minll; sp++)
    {
      for (ep = sp + minll; ep < cstart; ep++)
        {
          gdouble score = score_headloop (dhandle, sp, ep - sp, config->block_length / 2, config->score);
          g_print ("%u %.17g\n", ep - sp, score);
          continue;
          if (score <= config->score)
            {
              config->loop_start = sp - block;
              config->loop_length = ep - sp;
              config->score = score;
              found_loop = TRUE;
            }
          gsl_progress_notify (&pstate, ++pcount * 100.0 / pdist, "score:%+g (pos:%6lu)", config->score, sp - block);
        }
      G_BREAKPOINT ();
    }

  g_free (block);
  gsl_data_handle_close (dhandle);
  return found_loop;
}

static inline gdouble
dcache_headloop_score (GslDataCache     *dcache,
                       GslLong           lstart,        /* loop start (reaches till bstart) */
                       GslLong           cstart,        /* compare area start sample */
                       GslLong           clength,       /* compare area length */
                       gdouble           max_score,     /* no need to score beyond this */
                       GslDataCacheNode **lnp,
                       GslDataCacheNode **cnp,
                       gboolean           weighted)
{
  gsize node_size = GSL_DATA_CACHE_NODE_SIZE (dcache);
  GslDataCacheNode *lnode = *lnp, *cnode = *cnp;
  gdouble wmax = clength, score = 0.0;
  GslLong i = 0, loop_length = cstart - lstart;

  /* compute score for loop repeated over comparison area
   * .......|##########|-------------------------|......
   *    loopstart   cstart                (cstart+clength)
   */

  while (i < clength)
    {
      GslLong cdiff, ldiff, clen, llen, k, l = i % loop_length;
      gfloat *cb, *lb; /* base pointer */
      if (lnode->offset > lstart + l || lstart + l >= lnode->offset + node_size)
	{
	  gsl_data_cache_unref_node (dcache, lnode);
	  lnode = *lnp = gsl_data_cache_ref_node (dcache, lstart + l, TRUE);
	}
      if (cnode->offset > cstart + i || cstart + i >= cnode->offset + node_size)
	{
	  gsl_data_cache_unref_node (dcache, cnode);
	  cnode = *cnp = gsl_data_cache_ref_node (dcache, cstart + i, TRUE);
	}
      cdiff = cstart + i - cnode->offset;
      ldiff = lstart + l - lnode->offset;
      clen = MIN (node_size - cdiff, clength - i);
      llen = MIN (node_size - ldiff, loop_length - l);
      cb = cnode->data + cdiff - i;
      lb = lnode->data + ldiff - i;
      if (weighted)
        for (k = i + MIN (llen, clen); i < k; i++)
          {
            gdouble ed = lb[i] - cb[i];
            score += ed * ed * (1.0 - i / wmax);
          }
      else
        for (k = i + MIN (llen, clen); i < k; i++)
          {
            gdouble ed = lb[i] - cb[i];
            score += ed * ed;
          }
      if (score > max_score)
        break;
    }

  return score;
}

gboolean
gsl_data_find_loop1 (GslDataHandle    *dhandle,
                    GslDataLoopConfig *config,
                    gpointer           pdata,
                    GslProgressFunc    pfunc)
{
  GslProgressState pstate = gsl_progress_state (pdata, pfunc, 1);
  guint frame = 4410;   // FIXME: adjustable value?
  GslLong cstart, clength, j, dhandle_n_values, ls, ll, minll, pcount, maxll, min_loop = frame;
  GslDataCache *dcache;
  GslDataCacheNode *dnode1, *dnode2;
  gdouble pdist;
  gboolean found_loop = FALSE;

  g_return_val_if_fail (dhandle != NULL, FALSE);
  g_return_val_if_fail (config != NULL, FALSE);
  config->n_details = 0;

  if (gsl_data_handle_open (dhandle) != BSE_ERROR_NONE)
    return FALSE;
  dhandle_n_values = gsl_data_handle_n_values (dhandle);
  config->block_start = CLAMP (config->block_start, 0, dhandle_n_values - 1);
  if (config->block_length < 0)
    config->block_length = dhandle_n_values - config->block_start;
  else
    config->block_length = MIN (config->block_length, dhandle_n_values - config->block_start);
  if (config->block_length < 2)
    return FALSE;
  if (config->repetitions != CLAMP (config->repetitions, 2, config->block_length))
    return FALSE;

  dcache = gsl_data_cache_new (dhandle, 1);
  gsl_data_cache_open (dcache);
  gsl_data_handle_close (dhandle);
  gsl_data_cache_unref (dcache);

  dnode1 = gsl_data_cache_ref_node (dcache, config->block_start, TRUE);
  dnode2 = gsl_data_cache_ref_node (dcache, config->block_start, TRUE);

  /* widen loop, keeping it end-aligned to cstart
   *      |------------##########|--------------------|......
   * block_start   loopstart   cstart          (cstart+clength)
   */

  /* find a good loop length */
  config->score = G_MAXDOUBLE;
  cstart = config->block_start + config->block_length / config->repetitions;
  clength = config->block_length - (cstart - config->block_start);
  pcount = 0, pdist = (cstart - min_loop - config->block_start);
  for (j = config->block_start; j < cstart - min_loop; j++)
    {
      gdouble score = dcache_headloop_score (dcache, j, cstart, clength, config->score, &dnode1, &dnode2, FALSE);
      if (score <= config->score)
        {
          config->loop_start = j;
          config->loop_length = cstart - j;
          config->score = score;
          found_loop = TRUE;
        }
      if (pcount++ % 16 == 0)
        gsl_progress_notify (&pstate, pcount * 100.0 / pdist, "score:%+g", config->score);
    }
  gsl_progress_wipe (&pstate);
  if (!found_loop)
    goto seek_loop_done;
  g_printerr ("  lLOOP: %6lu - %6lu [%6lu] (block: %6lu - %6lu [%6lu]) (score:%+g)\n",
              config->loop_start, config->loop_start + config->loop_length, config->loop_length,
              config->block_start, config->block_start + config->block_length, config->block_length,
              config->score);
  
  /* find best loop position */
  ll = config->loop_length;
  minll = ll;
  maxll = ll + 1;
  config->score = G_MAXDOUBLE;
  pcount = 0, pdist = (config->block_length - maxll - frame) * (maxll - minll);
  for (ls = config->block_start; ls + maxll + frame <= config->block_start + config->block_length; ls++)
    {
      /* find best loop length */
      for (ll = minll; ll < maxll; ll++)
        {
          gdouble score = dcache_headloop_score (dcache, ls, ls + ll, frame, config->score, &dnode1, &dnode2, TRUE);
          if (score <= config->score)
            {
              config->loop_start = ls;
              config->loop_length = ll;
              config->score = score;
              found_loop = TRUE;
            }
          gsl_progress_notify (&pstate, ++pcount * 100.0 / pdist, "weighted-frame-score:%+g", config->score);
        }
    }
  gsl_progress_wipe (&pstate);
  g_printerr ("  pLOOP: %6lu - %6lu [%6lu] (block: %6lu - %6lu [%6lu]) (weighted-frame-score:%+g)\n",
              config->loop_start, config->loop_start + config->loop_length, config->loop_length,
              config->block_start, config->block_start + config->block_length, config->block_length,
              config->score);

  /* find best loop length */
  frame = MIN (config->loop_length / 5, 4410 * 7);
  while (frame >= 32) // FIXME
    {
      /* jitter loop length */
      minll = config->loop_length - frame;
      minll = MAX (minll, 1);
      maxll = config->loop_length + frame;
      maxll = MIN (config->block_start + config->block_length - frame,
                   config->loop_start + maxll) -
              config->loop_start;

      /* find best loop length */
      config->score = G_MAXDOUBLE;
      ls = config->loop_start;
      pcount = 0, pdist = maxll - minll;
      for (ll = minll; ll < maxll; ll++)
        {
          gdouble score = dcache_headloop_score (dcache, ls, ls + ll, frame, config->score, &dnode1, &dnode2, FALSE);
          if (score <= config->score)
            {
              config->loop_start = ls;
              config->loop_length = ll;
              config->score = score;
              found_loop = TRUE;
            }
          gsl_progress_notify (&pstate, ++pcount * 100.0 / pdist, "frame-score:%+g", config->score);
        }
      gsl_progress_wipe (&pstate);
      g_printerr ("  sLOOP: %6lu - %6lu [%6lu] (block: %6lu - %6lu [%6lu]) (frame[%d]-score:%+g)\n",
                  config->loop_start, config->loop_start + config->loop_length, config->loop_length,
                  config->block_start, config->block_start + config->block_length, config->block_length,
                  frame, config->score);
      frame /= 2;
    }

 seek_loop_done:

  gsl_data_cache_unref_node (dcache, dnode1);
  gsl_data_cache_unref_node (dcache, dnode2);

  gsl_data_cache_close (dcache);
  return found_loop;
}

static inline float
tailloop_score (GslDataCache          *dcache,
		const GslDataTailLoop *cfg,
		GslLong                loopstart,
		GslLong	               loopsize,
		gfloat                 worstscore)
{
  gsize node_size = GSL_DATA_CACHE_NODE_SIZE (dcache);
  GslLong looppos, i, compare = cfg->pre_loop_compare;
  gfloat score = 0.0;
  GslDataCacheNode *snode, *lnode;

  /* compute score for loopsize with compare samples before loop */
  /* -----|-------------------------|-----------------
   *  loopstart-compare          loopstart
   */

  looppos = loopstart - compare;
  while (looppos < loopstart)
    looppos += loopsize;

  snode = gsl_data_cache_ref_node (dcache, loopstart - compare, TRUE);
  lnode = gsl_data_cache_ref_node (dcache, looppos, TRUE);
  for (i = loopstart - compare; i < loopstart;)
    {
      GslLong sdiff, ldiff, slen, llen, loop_len, compare_len, j;
      gfloat *sb, *lb;

      if (snode->offset > i || i >= snode->offset + node_size)
	{
	  gsl_data_cache_unref_node (dcache, snode);
	  snode = gsl_data_cache_ref_node (dcache, i, TRUE);
	}
      if (lnode->offset > looppos || looppos >= lnode->offset + node_size)
	{
	  gsl_data_cache_unref_node (dcache, lnode);
	  lnode = gsl_data_cache_ref_node (dcache, looppos, TRUE);
	}
      sdiff = i - snode->offset;
      ldiff = looppos - lnode->offset;
      slen = node_size - sdiff;
      llen = node_size - ldiff;
      sb = snode->data + sdiff;
      lb = lnode->data + ldiff;
      compare_len = loopstart - i;
      loop_len = loopsize - (looppos - loopstart);
      
      slen = MIN (slen, compare_len);
      llen = MIN (llen, loop_len);
      slen = MIN (slen, llen);
      
      j = slen;
      if (cfg->cmp_strategy == GSL_DATA_TAIL_LOOP_CMP_CORRELATION)
	{
	  while (j--)
	    score += sb[j] * lb[j];
	}
      else
	{
	  while (j--)
	    {
	      gfloat tmp = lb[j] - sb[j];
	      score += tmp * tmp;
	    }
	  if (score > worstscore)
	    break;
	}
      
      i += slen;
      looppos += slen;
      if (looppos >= loopstart + loopsize)
	looppos -= loopsize;
    }
  gsl_data_cache_unref_node (dcache, snode);
  gsl_data_cache_unref_node (dcache, lnode);

  return score;
}

gdouble
gsl_data_find_loop0 (GslDataHandle         *dhandle,
                     const GslDataTailLoop *cfg,
                     GslLong               *loop_start_p,
                     GslLong               *loop_end_p)
{
  GslDataCache *dcache;
  GslLong perc_bound, cfg_max_loop, dhandle_n_values;
  GslLong perc_count = 0, perc_val = 0;
  GslLong loopsize, bestloopsize = 0;
  gdouble bestscore;

  g_return_val_if_fail (dhandle != NULL, 0);
  g_return_val_if_fail (cfg != NULL, 0);
  g_return_val_if_fail (loop_start_p != NULL, 0);
  g_return_val_if_fail (loop_end_p != NULL, 0);
  g_return_val_if_fail (cfg->min_loop >= 1, 0);

  if (gsl_data_handle_open (dhandle) != BSE_ERROR_NONE)
    return 0;
  dhandle_n_values = gsl_data_handle_n_values (dhandle);

  g_return_val_if_fail (cfg->pre_loop_compare < dhandle_n_values - 1, 0);
  cfg_max_loop = cfg->max_loop < 0 ? dhandle_n_values - 1 - cfg->pre_loop_compare : cfg->max_loop;
  g_return_val_if_fail (cfg_max_loop >= cfg->min_loop, 0);
  g_return_val_if_fail (cfg->pre_loop_compare + cfg_max_loop < dhandle_n_values, 0);
  g_return_val_if_fail (cfg->cmp_strategy == GSL_DATA_TAIL_LOOP_CMP_LEAST_SQUARE ||
			cfg->cmp_strategy == GSL_DATA_TAIL_LOOP_CMP_CORRELATION, 0);

  dcache = gsl_data_cache_new (dhandle, 1);
  gsl_data_cache_open (dcache);
  gsl_data_handle_close (dhandle);
  gsl_data_cache_unref (dcache);

  perc_bound = (cfg_max_loop - cfg->min_loop) / 100.0;

  /* we try to maximize correlation, but to minimize the error */
  if (cfg->cmp_strategy == GSL_DATA_TAIL_LOOP_CMP_CORRELATION)
    bestscore = 0.0;
  else
    bestscore = 1e+14;

  for (loopsize = cfg->min_loop; loopsize < cfg_max_loop; loopsize++)
    {
      gdouble score = tailloop_score (dcache, cfg,
				      dhandle_n_values - loopsize, loopsize,
				      bestscore);

      /* we try to maximize correlation, but to minimize the error */
      if ((cfg->cmp_strategy == GSL_DATA_TAIL_LOOP_CMP_CORRELATION && score > bestscore) ||
	  (cfg->cmp_strategy == GSL_DATA_TAIL_LOOP_CMP_LEAST_SQUARE && score < bestscore))
	{
	  bestloopsize = loopsize;
	  bestscore = score;
	}
      if (++perc_count >= perc_bound)
	{
	  perc_count = 0;
	  perc_val++;
	  g_printerr ("processed %lu%%       \r", perc_val);
	}
    }
  g_printerr ("\nbest match (%s): len in samples=%ld, len=%ld, score=%f\n",
	      (cfg->cmp_strategy == GSL_DATA_TAIL_LOOP_CMP_CORRELATION) ? "correlation" : "least squares",
	      bestloopsize, bestloopsize, bestscore);

  *loop_start_p = dhandle_n_values - bestloopsize;
  *loop_end_p = dhandle_n_values - 1;

  gsl_data_cache_close (dcache);

  /* FIXME: statistics: scan for other extreme points in
   *                    score[cfg->min_loop..cfg_max_loop]
   */

  return bestscore;
}
