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


/* --- defines --- */
/* factorization constants: 2^(1/12), ln(2^(1/12)) and 2^(1/(12*6))
 * retrived with:
 #include <stl.h>
 #include <complex.h>
 typedef long double ld;
 
 int main (void)
 {
 ld r, l;
 
 cout.precision(256);
 
 r = pow ((ld) 2, (ld) 1 / (ld) 12);
 cout << "2^(1/12) =\n";
 cout << "2^" << (ld) 1 / (ld) 12 << " =\n";
 cout << r << "\n";
 
 l = log (r);
 cout << "ln(2^(1/12)) =\n";
 cout << "ln(" << r << ") =\n";
 cout << l << "\n";
 
 r = pow ((ld) 2, (ld) 1 / (ld) 72);
 cout << "2^(1/72) =\n";
 cout << "2^" << (ld) 1 / (ld) 72 << " =\n";
 cout << r << "\n";

 r = pow ((ld) 2, (ld) 1 / (ld) 1200);
 cout << "2^(1/1200) =\n";
 cout << "2^" << (ld) 1 / (ld) 1200 << " =\n";
 cout << r << "\n";

 return 0;
 }
*/
/* keep these defines in sync with bseutils.c */
#define	BSE_2_RAISED_TO_1_OVER_12_d	( /* 2^(1/12) */ \
              1.0594630943592953098431053149397484958171844482421875)
#define	BSE_LN_OF_2_RAISED_TO_1_OVER_12_d	( /* ln(2^(1/12)) */ \
              0.05776226504666215344485635796445421874523162841796875)
#define	BSE_2_RAISED_TO_1_OVER_72_d	( /* 2^(1/72) */ \
              1.009673533228510944326217213529162108898162841796875)
#define BSE_2_RAISED_TO_1_OVER_1200_d     ( /* 2^(1/1200) */ \
              1.0005777895065548488418016859213821589946746826171875)


/* --- prototypes --- */
extern void        bse_gconfig_notify_lock_changed (void);			/* from bsegconfig.c */
extern void        bse_globals_copy                (const BseGlobals *globals_src,
						    BseGlobals       *globals); /* for bsegconfig.c */
extern void        bse_globals_unset               (BseGlobals       *globals); /* for bsegconfig.c */


/* --- extern variables --- */
const guint	     bse_major_version = BSE_MAJOR_VERSION;
const guint	     bse_minor_version = BSE_MINOR_VERSION;
const guint	     bse_micro_version = BSE_MICRO_VERSION;
const guint	     bse_interface_age = BSE_INTERFACE_AGE;
const guint	     bse_binary_age = BSE_BINARY_AGE;
const gchar         *bse_version = BSE_VERSION;
const gdouble*	_bse_halftone_factor_table = NULL;
const guint*	_bse_halftone_factor_table_fixed = NULL;
const gdouble*	_bse_fine_tune_factor_table = NULL;


/* --- variables --- */
static guint		 bse_globals_lock_count = 0;
static BseGlobals	 bse_globals_current = { 0, };
const BseGlobals * const bse_globals = &bse_globals_current;
static const BseGlobals	 bse_globals_defaults = {
  0.1		/* step_volume_dB */,
  10		/* step_bpm */,
  8		/* step_balance */,
  4		/* step_transpose */,
  4		/* step_fine_tune */,
  1		/* step_env_time */,
  
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
  gint i;
  
  g_return_if_fail (_bse_halftone_factor_table == NULL);

  /* setup half tone factorization table
   */
  g_assert (BSE_MIN_NOTE == 0);
  for (i = 0; i <= BSE_MAX_NOTE; i++)
    {
      ht_factor_table_d[i] = pow (BSE_2_RAISED_TO_1_OVER_12_d,
				  ((gdouble) i) - BSE_KAMMER_NOTE);
      ht_factor_table_fixed_ui[i] = ht_factor_table_d[i] * 65536.0 + 0.5;
      BSE_IF_DEBUG (TABLES)
	{
	  if (i == BSE_MIN_NOTE || i == BSE_MAX_NOTE ||
	      (i >= BSE_KAMMER_NOTE - 6 && i <= BSE_KAMMER_NOTE + 12))
	    g_message ("ht-table: [%d] -> %.20f (%d)",
		       i, ht_factor_table_d[i], ht_factor_table_fixed_ui[i]);
	}
    }
  _bse_halftone_factor_table = ht_factor_table_d;
  _bse_halftone_factor_table_fixed = ht_factor_table_fixed_ui;
  
  /* setup fine tune factorization table, since fine tunes are in the
   * positive and in the negative range, we allow negative indieces here.
   */
  for (i = -BSE_MAX_FINE_TUNE; i <= BSE_MAX_FINE_TUNE; i++)
    {
      ft_factor_table_d[BSE_MAX_FINE_TUNE + i] = pow (BSE_2_RAISED_TO_1_OVER_1200_d, i);
      BSE_IF_DEBUG (TABLES)
	g_message ("ft-table: [%d] -> %.20f",
		   i, ft_factor_table_d[BSE_MAX_FINE_TUNE + i]);
    }
  _bse_fine_tune_factor_table = ft_factor_table_d + BSE_MAX_FINE_TUNE;
  
  /* setup BseGlobals
   */
  bse_globals_copy (&bse_globals_defaults, &bse_globals_current);
  
  bse_globals_lock_count = 0;
}

void
bse_globals_copy (const BseGlobals *globals_src,
		  BseGlobals       *globals)
{
  if (!globals_src)
    globals_src = &bse_globals_defaults;
  if (!globals)
    {
      g_return_if_fail (bse_globals_locked () == FALSE);

      bse_globals_unset (&bse_globals_current);
      globals = &bse_globals_current;
    }

  *globals = *globals_src;
  /* g_strdup()s */
}

void
bse_globals_unset (BseGlobals *globals)
{
  g_return_if_fail (globals != NULL);

  /* g_free()s */
  memset (globals, 0, sizeof (*globals));
}

void
bse_globals_lock (void)
{
  bse_globals_lock_count++;
  if (bse_globals_lock_count == 1)
    bse_gconfig_notify_lock_changed ();
}

void
bse_globals_unlock (void)
{
  if (bse_globals_lock_count)
    {
      bse_globals_lock_count--;
      if (bse_globals_lock_count == 0)
	{
	  bse_gconfig_notify_lock_changed ();
	}
    }
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

glong
bse_time_range_to_ms (BseTimeRangeType time_range)
{
  g_return_val_if_fail (time_range >= BSE_TIME_RANGE_SHORT, 0);
  g_return_val_if_fail (time_range <= BSE_TIME_RANGE_LONG, 0);

  switch (time_range)
    {
    case BSE_TIME_RANGE_SHORT:		return BSE_TIME_RANGE_SHORT_ms;
    case BSE_TIME_RANGE_MEDIUM:		return BSE_TIME_RANGE_MEDIUM_ms;
    case BSE_TIME_RANGE_LONG:		return BSE_TIME_RANGE_LONG_ms;
    }
  return 0;	/* can't be triggered */
}
