/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002, 2003 Tim Janik
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
#include "bsenote.h"
#include "bseutils.h"
#include "gslieee754.h"
#include <string.h>


/* --- functions --- */
gdouble
bse_freq_from_note (gint note)
{
  return bse_note_to_freq (note);
}

gint
bse_note_from_freq (gdouble freq)
{
  gdouble d;
  gint note;

  freq /= BSE_KAMMER_FREQUENCY_f;
  d = log (freq) / BSE_LN_2_POW_1_DIV_12_d;
  note = gsl_ftoi (BSE_KAMMER_NOTE + d);

  return note >= BSE_MIN_NOTE && note <= BSE_MAX_NOTE ? note : BSE_NOTE_VOID;
}

gint
bse_note_from_freq_bounded (gdouble freq)
{
  gdouble d;
  gint note;

  freq /= BSE_KAMMER_FREQUENCY_f;
  d = log (freq) / BSE_LN_2_POW_1_DIV_12_d;
  note = gsl_ftoi (BSE_KAMMER_NOTE + d);

  return CLAMP (note, BSE_MIN_NOTE, BSE_MAX_NOTE);
}

gint
bse_note_fine_tune_from_note_freq (gint    note,
				   gdouble freq)
{
  gdouble d;
  gint fine_tune;
  
  freq /= BSE_KAMMER_FREQUENCY_f * BSE_SEMITONE_FACTOR (note);
  d = log (freq) / BSE_LN_2_POW_1_DIV_1200_d;
  fine_tune = gsl_ftoi (d);

  return CLAMP (fine_tune, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE);
}

gdouble
bse_note_to_freq (gint note)
{
  if (note >= BSE_MIN_NOTE && note <= BSE_MAX_NOTE)
    return BSE_KAMMER_FREQUENCY_f * BSE_SEMITONE_FACTOR (note);
  else
    return 0.0;
}

gdouble
bse_note_to_tuned_freq (gint note,
			gint fine_tune)
{
  if (note >= BSE_MIN_NOTE && note <= BSE_MAX_NOTE)
    return BSE_KAMMER_FREQUENCY_f * BSE_SEMITONE_FACTOR (note) * BSE_FINE_TUNE_FACTOR (fine_tune);
  else
    return 0.0;
}


/* --- freq array --- */
struct BseFreqArray {
  guint    n_values;
  guint    n_prealloced;
  gdouble *values;
};

BseFreqArray*
bse_freq_array_new (guint prealloc)
{
  BseFreqArray *farray = g_new0 (BseFreqArray, 1);
  
  farray->n_prealloced = prealloc;
  farray->values = g_new0 (gdouble, farray->n_prealloced);
  
  return farray;
}

void
bse_freq_array_free (BseFreqArray *farray)
{
  g_return_if_fail (farray != NULL);
  
  g_free (farray->values);
  g_free (farray);
}

guint
bse_freq_array_n_values (BseFreqArray *farray)
{
  g_return_val_if_fail (farray != NULL, 0);

  return farray->n_values;
}

gdouble
bse_freq_array_get (BseFreqArray *farray,
                    guint         index)
{
  g_return_val_if_fail (farray != NULL, 0);
  g_return_val_if_fail (index < farray->n_values, 0);
  
  return farray->values[index];
}

void
bse_freq_array_insert (BseFreqArray *farray,
                       guint         index,
                       gdouble       value)
{
  guint i;
  
  g_return_if_fail (farray != NULL);
  g_return_if_fail (index <= farray->n_values);
  
  i = farray->n_values;
  i = farray->n_values += 1;
  if (farray->n_values > farray->n_prealloced)
    {
      farray->n_prealloced = farray->n_values;
      farray->values = g_renew (gdouble, farray->values, farray->n_prealloced);
    }
  g_memmove (farray->values + index + 1,
             farray->values + index,
             i - index);
  farray->values[index] = value;
}

void
bse_freq_array_append (BseFreqArray *farray,
                       gdouble       value)
{
  bse_freq_array_insert (farray, farray->n_values, value);
}

void
bse_freq_array_set (BseFreqArray *farray,
                    guint         index,
                    gdouble       value)
{
  g_return_if_fail (farray != NULL);
  g_return_if_fail (index < farray->n_values);
  
  farray->values[index] = value;
}

gboolean
bse_freq_arrays_match_freq (gfloat        match_freq,
                            BseFreqArray *inclusive_set,
                            BseFreqArray *exclusive_set)
{
  guint i;
  
  if (exclusive_set)
    for (i = 0; i < exclusive_set->n_values; i++)
      {
	gdouble *value = exclusive_set->values + i;
        
	if (fabs (*value - match_freq) < BSE_FREQUENCY_EPSILON)
	  return FALSE;
      }
  
  if (!inclusive_set)
    return TRUE;
  
  for (i = 0; i < inclusive_set->n_values; i++)
    {
      gdouble *value = inclusive_set->values + i;
      
      if (fabs (*value - match_freq) < BSE_FREQUENCY_EPSILON)
	return TRUE;
    }
  return FALSE;
}
