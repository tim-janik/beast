/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999, 2000 Olaf Hoehmann and Tim Janik
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
#include        "bsehunkmixer.h"



/* why is there no truely generic bse_hunk_mix (loads_of_arguments) function?
 *
 * assuming for a moment that BSE_MAX_N_TRACKS would be 4,
 * we'd have the following basic cases to cover:
 * [concatenation operator for two words a, b is (a + b) / 2]
 *
 * i 1->1 := (a) -> (a)                   !v=memcpy
 * i 1->2 := (a) -> (a,a)
 * - 1->3 := (a) -> (0,0,a)
 * - 1->4 := (a) -> (a,a,0,0)
 * i 2->1 := (a,b) -> (ab)
 * i 2->2 := (a,b) -> (a,b)               !v=memcpy
 * - 2->3 := (a,b) -> (a,b,0)
 * - 2->4 := (a,b) -> (a,b,0,0)
 *   3->1 := (a,b,c) -> (abc)
 * - 3->2 := (a,b,c) -> (ac,bc)
 * o 3->3 := (a,b,c) -> (a,b,c)           !v=memcpy
 * - 3->4 := (a,b,c) -> (a,b,c,c)
 *   4->1 := (a,b,c,d) -> (abcd)
 *   4->2 := (a,b,c,d) -> (ac,bd)
 * - 4->3 := (a,b,c,d) -> (a,b,cd)
 * o 4->4 := (a,b,c,d) -> (a,b,c,d)       !v=memcpy
 *
 * and each one would need a distinction for v (volume) being 1.0 or different
 * from 1.0, which makes up 32 cases total (the ones marked with "!v" can be
 * reduced to a memcpy if the volume is close to 1.0).
 * also, the way to combine/split tracks is pretty ambiguous for the cases
 * marked with '-', and the above specified behaviour could at best get
 * interpreted as a "friendly attempt to make a good guess".
 *
 * so instead of implementing a half-baked all-doing monster table of mixing
 * functions (32 functions for BSE_MAX_N_TRACKS=4, 64 for BSE_MAX_N_TRACKS=8
 * and so on), we will currently just provide the variants marked with a 'i'
 * and maybe implement a more generic function to cover track mixing in
 * combination with a track mapping table at a later point (or provide certain
 * variants of the above as the need arises).
 * (we do also feature the 'o' functions for 1.0 volumes.)
 */


/* --- typedefs --- */
typedef void (*MixNvFunc) (BseSampleValue       *hunk,
			   BseSampleValue       *bound,
			   const BseSampleValue *src_hunk);
typedef void (*MixWvFunc) (BseSampleValue       *hunk,
			   BseSampleValue       *bound,
			   const gfloat         *volume_factors,
			   const BseSampleValue *src_hunk);


/* --- functions --- */
static void
_bse_hunk_mix_nv_memcpy (BseSampleValue       *hunk,
			 BseSampleValue       *bound,
			 const BseSampleValue *src_hunk)
{
  memcpy (hunk, src_hunk, (bound - hunk) * sizeof (BseSampleValue));
}
static void
_bse_hunk_mix_nv_1_2 (BseSampleValue       *d,
		      BseSampleValue       *bound,
		      const BseSampleValue *s)
{
  do
    {
      *(d++) = *s;
      *(d++) = *(s++);
    }
  while (d < bound);
}
static void
_bse_hunk_mix_nv_2_1 (BseSampleValue       *d,
		      BseSampleValue       *bound,
		      const BseSampleValue *s)
{
  do
    {
      BseMixValue v = *(s++);

      v += *(s++);
      *(d++) = v >> 2;
    }
  while (d < bound);
}

static void
_bse_hunk_mix_wv_1_1 (BseSampleValue       *d,
		      BseSampleValue       *bound,
		      const gfloat         *f,
		      const BseSampleValue *s)
{
  do
    {
      BseMixValue v = *(s++);

      v *= f[0];
      *(d++) = BSE_CLIP_SAMPLE_VALUE (v);
    }
  while (d < bound);
}
static void
_bse_hunk_mix_wv_1_2 (BseSampleValue       *d,
		      BseSampleValue       *bound,
		      const gfloat         *f,
		      const BseSampleValue *s)
{
  do
    {
      BseMixValue v1 = *(s++), v2 = v1;

      v1 *= f[0];
      *(d++) = BSE_CLIP_SAMPLE_VALUE (v1);
      v2 *= f[1];
      *(d++) = BSE_CLIP_SAMPLE_VALUE (v2);
    }
  while (d < bound);
}
static void
_bse_hunk_mix_wv_2_1 (BseSampleValue       *d,
		      BseSampleValue       *bound,
		      const gfloat         *f,
		      const BseSampleValue *s)
{
  do
    {
      BseMixValue v = *(s++);

      v += *(s++);
      v *= f[0];
      v >>= 2;
      *(d++) = BSE_CLIP_SAMPLE_VALUE (v);
    }
  while (d < bound);
}
static void
_bse_hunk_mix_wv_2_2 (BseSampleValue       *d,
		      BseSampleValue       *bound,
		      const gfloat         *f,
		      const BseSampleValue *s)
{
  do
    {
      BseMixValue v1 = *(s++), v2 = *(s++);

      v1 *= f[0];
      *(d++) = BSE_CLIP_SAMPLE_VALUE (v1);
      v2 *= f[1];
      *(d++) = BSE_CLIP_SAMPLE_VALUE (v2);
    }
  while (d < bound);
}

void
bse_hunk_mix (guint                 n_dest_tracks,
	      BseSampleValue       *dest_hunk,
	      const gfloat         *dest_volumes,
	      guint                 n_src_tracks,
	      const BseSampleValue *src_hunk)
{
#define MAX_N_MIX_TRACKS 4
  static const MixNvFunc mix_nv_funcs[MAX_N_MIX_TRACKS][MAX_N_MIX_TRACKS] = {
    { /* 1->1 */ _bse_hunk_mix_nv_memcpy,
      /* 1->2 */ _bse_hunk_mix_nv_1_2,
      /* 1->3 */ NULL,
      /* implicit NULLs */ },
    { /* 2->1 */ _bse_hunk_mix_nv_2_1,
      /* 2->2 */ _bse_hunk_mix_nv_memcpy,
      /* 2->3 */ NULL,
      /* implicit NULLs */ },
    { /* 3->1 */ NULL,
      /* 3->2 */ NULL,
      /* 3->3 */ _bse_hunk_mix_nv_memcpy,
      /* 3->4 */ NULL,
      /* implicit NULLs */ },
    { /* 4->1 */ NULL,
      /* 4->2 */ NULL,
      /* 4->3 */ NULL,
      /* 4->4 */ _bse_hunk_mix_nv_memcpy,
      /* implicit NULLs */ },
    /* implicit NULLs */
  };
  static const MixWvFunc mix_wv_funcs[MAX_N_MIX_TRACKS][MAX_N_MIX_TRACKS] = {
    { /* 1->1 */ _bse_hunk_mix_wv_1_1,
      /* 1->2 */ _bse_hunk_mix_wv_1_2,
      /* 1->3 */ NULL,
      /* implicit NULLs */ },
    { /* 2->1 */ _bse_hunk_mix_wv_2_1,
      /* 2->2 */ _bse_hunk_mix_wv_2_2,
      /* 2->3 */ NULL,
      /* implicit NULLs */ },
    /* implicit NULLs */
  };
  gboolean use_volume = FALSE;
  MixNvFunc mix_nv_func = NULL;
  MixWvFunc mix_wv_func = NULL;

  g_return_if_fail (n_dest_tracks >= 1 && n_dest_tracks <= MAX_N_MIX_TRACKS);
  g_return_if_fail (dest_hunk != NULL);
  g_return_if_fail (n_src_tracks >= 1 && n_src_tracks <= MAX_N_MIX_TRACKS);
  g_return_if_fail (src_hunk != NULL);

  if (dest_volumes)
    {
      guint i;

      for (i = 0; i < n_dest_tracks; i++)
	{
	  use_volume = BSE_EPSILON_CMP (1.0, dest_volumes[i]) != 0;
	  break;
	}
      if (!use_volume)
	dest_volumes = NULL;
    }

  /* FIXME: need to check for overlaps as well? */
  if (!use_volume)
    {
      if (src_hunk == dest_hunk)
	return;
      mix_nv_func = mix_nv_funcs[n_src_tracks - 1][n_dest_tracks - 1];
    }
  else
    mix_wv_func = mix_wv_funcs[n_src_tracks - 1][n_dest_tracks - 1];
  
  if (mix_wv_func)
    mix_wv_func (dest_hunk,
		 dest_hunk + BSE_TRACK_LENGTH * n_dest_tracks,
		 dest_volumes,
		 src_hunk);
  else if (mix_nv_func)
    mix_nv_func (dest_hunk,
		 dest_hunk + BSE_TRACK_LENGTH * n_dest_tracks,
		 src_hunk);
  else
    g_warning (G_STRLOC ": mixing from %u source tracks to "
	       "%u destination tracks is not imlemented (probably underspecified)",
	       n_src_tracks,
	       n_dest_tracks);
}

void
bse_hunk_fill (guint	       n_tracks,
	       BseSampleValue *hunk,
	       BseSampleValue  value)
{
  g_return_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS);
  g_return_if_fail (hunk != NULL);

  if (value >> 8 == (value & 0xff))
    memset (hunk, value, n_tracks * BSE_TRACK_LENGTH * sizeof (BseSampleValue));
  else
    {
      BseSampleValue *bound = hunk + n_tracks * BSE_TRACK_LENGTH;

      do
	*(hunk++) = value;
      while (hunk < bound);
    }
}

void
bse_hunk_clip_from_mix_buffer (guint           n_tracks,
			       BseSampleValue *dest_hunk,
			       gfloat          master_volume,
			       BseMixValue    *src_mix_buffer)
{
  BseMixValue *bound;
  
  g_return_if_fail (n_tracks >= 1 && n_tracks <= BSE_MAX_N_TRACKS);
  g_return_if_fail (dest_hunk != NULL);
  g_return_if_fail (src_mix_buffer != NULL);

  bound = src_mix_buffer + n_tracks * BSE_TRACK_LENGTH;
  if (BSE_EPSILON_CMP (1.0, master_volume) != 0)
    do
      *(dest_hunk++) = BSE_CLIP_SAMPLE_VALUE (*src_mix_buffer * master_volume);
    while (++src_mix_buffer < bound);
  else
    do
      *(dest_hunk++) = BSE_CLIP_SAMPLE_VALUE (*src_mix_buffer);
    while (++src_mix_buffer < bound);
}
