/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997, 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include	"bseglobals.h"

#include	"bseconfig.h"
#include	<math.h>


/* --- defines --- */
#define BSE_DEBUG(y,x)  G_STMT_START { /* x */ ; } G_STMT_END


/* factorization constants: 2^(1/12) and 2^(1/(12*6))
 * retrived with:
 * #include <stl.h>
 * #include <complex.h>
 * typedef long double ld;
 *
 * int main (void)
 * {
 *   ld r;
 *
 *   cout.precision(256);
 *
 *   r = pow ((ld) 2, (ld) 1 / (ld) 12);
 *   cout << "2^(1/12) =\n";
 *   cout << "2^" << (ld) 1 / (ld) 12 << " =\n";
 *   cout << r << "\n";
 *
 *   r = pow ((ld) 2, (ld) 1 / (ld) 72);
 *   cout << "2^(1/72) =\n";
 *   cout << "2^" << (ld) 1 / (ld) 72 << " =\n";
 *   cout << r << "\n";
 *
 *   return 0;
 * }
 */
#define	BSE_2_RAISED_TO_1_OVER_12_d	( /* 2^(1/12) */ \
              1.0594630943592953098431053149397484958171844482421875)
#define	BSE_2_RAISED_TO_1_OVER_72_d	( /* 2^(1/72) */ \
              1.009673533228510944326217213529162108898162841796875)


/* --- extern variables --- */
const guint	bse_major_version = BSE_MAJOR_VERSION;
const guint	bse_minor_version = BSE_MINOR_VERSION;
const guint	bse_micro_version = BSE_MICRO_VERSION;
const guint	bse_interface_age = BSE_INTERFACE_AGE;
const guint	bse_binary_age = BSE_BINARY_AGE;
const gchar    *bse_version = BSE_VERSION;
const gdouble*	_bse_halftone_factor_table = NULL;
const guint*	_bse_halftone_factor_table_fixed = NULL;
const gdouble*	_bse_fine_tune_factor_table = NULL;


/* --- variables --- */
const BseGlobals	*bse_globals = NULL;
static guint		 bse_globals_lock_count = 0;
static const BseGlobals	 bse_globals_defaults =
{
  0.1		/* step_volume_dB */,
  10		/* step_bpm */,
  4		/* step_n_channels */,
  4		/* step_pattern_length */,
  8		/* step_balance */,
  4		/* step_transpose */,
  4		/* step_fine_tune */,
  1		/* step_env_time */,
  
  sizeof (BseSampleValue) * 512	/* pcm_buffer_size */,
  
  256		/* track_length (hunk_size) */,
  44100		/* mixing_frequency */,
};


/* --- functions --- */
gchar*
bse_check_version (guint required_major,
		   guint required_minor,
		   guint required_micro)
{
  if (required_major > BSE_MAJOR_VERSION)
    return "BSE version too old (major mismatch)";
  if (required_major < BSE_MAJOR_VERSION)
    return "BSE version too new (major mismatch)";
  if (required_minor > BSE_MINOR_VERSION)
    return "BSE version too old (minor mismatch)";
  if (required_minor < BSE_MINOR_VERSION)
    return "BSE version too new (minor mismatch)";
  if (required_micro < BSE_MICRO_VERSION - BSE_BINARY_AGE)
    return "BSE version too new (micro mismatch)";
  if (required_micro > BSE_MICRO_VERSION)
    return "BSE version too old (micro mismatch)";
  return NULL;
}

void
bse_globals_init (void)
{
  static gdouble ht_factor_table_d[BSE_MAX_NOTE + 1] = { 0.0, };
  static guint	 ht_factor_table_fixed_ui[BSE_MAX_NOTE + 1] = { 0, };
  static gdouble ft_factor_table_d[BSE_MAX_FINE_TUNE * 2 + 1] = { 0.0, };
  static BseGlobals globals;
  gint i;
  
  g_return_if_fail (bse_globals == NULL);
  
  /* setup half tone factorization table
   */
  g_assert (BSE_MIN_NOTE == 0);
  for (i = 0; i <= BSE_MAX_NOTE; i++)
    {
      ht_factor_table_d[i] = pow (BSE_2_RAISED_TO_1_OVER_12_d,
				  ((gdouble) i) - BSE_KAMMER_NOTE);
      ht_factor_table_fixed_ui[i] = 0.5 + ht_factor_table_d[i] * 65536;
      BSE_DEBUG (TABLES, {
	if (i == BSE_MIN_NOTE || i == BSE_MAX_NOTE ||
	    (i >= BSE_KAMMER_NOTE - 6 && i <= BSE_KAMMER_NOTE + 12))
	  printf ("ht-table: [%d] -> %.20f (%d)\n",
		  i, ht_factor_table_d[i], ht_factor_table_fixed_ui[i]);
      });
    }
  _bse_halftone_factor_table = ht_factor_table_d;
  _bse_halftone_factor_table_fixed = ht_factor_table_fixed_ui;
  
  /* fine tune assertments, so BSE_2_RAISED_TO_1_OVER_72_d is the right
   * constant (12 * 6 = 72)
   */
  g_assert (- BSE_MIN_FINE_TUNE == BSE_MAX_FINE_TUNE &&
	    BSE_MAX_FINE_TUNE == 6);
  
  /* setup fine tune factorization table, since fine tunes are in the
   * positive and in the negative range, we allow negative indexes here.
   */
  for (i = -BSE_MAX_FINE_TUNE; i <= BSE_MAX_FINE_TUNE; i++)
    {
      ft_factor_table_d[BSE_MAX_FINE_TUNE + i] = pow (BSE_2_RAISED_TO_1_OVER_72_d, i);
      BSE_DEBUG (TABLES, {
	printf ("ft-table: [%d] -> %.20f\n",
		i, ft_factor_table_d[BSE_MAX_FINE_TUNE + i]);
      });
    }
  _bse_fine_tune_factor_table = ft_factor_table_d + BSE_MAX_FINE_TUNE;
  
  /* setup BseGlobals
   */
  globals = bse_globals_defaults;
  
  bse_globals = &globals;
  bse_globals_lock_count = 0;
}

void
bse_globals_lock (void)
{
  bse_globals_lock_count++;
}

void
bse_globals_unlock (void)
{
  if (bse_globals_lock_count)
    bse_globals_lock_count--;
}

gboolean
bse_globals_locked (void)
{
  return bse_globals_lock_count != 0;
}

gdouble
bse_dB_to_factor (gfloat dB)
{
  gdouble factor;
  
  factor = dB / 10; /* Bell */
  factor = pow (10, factor);
  
  return factor;
}

gfloat
bse_dB_from_factor (gdouble factor,
		    gfloat  min_dB)
{
  if (factor > 0)
    {
      gfloat dB;
      
      dB = log10 (factor); /* Bell */
      dB *= 10;
      
      return dB;
    }
  else
    return min_dB;
}
