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
#ifndef __BSE_NOTE_H__
#define __BSE_NOTE_H__

#include        <bse/bseglobals.h>

G_BEGIN_DECLS

/* --- import Sfi macros --- */
#define BSE_MIN_NOTE            SFI_MIN_NOTE
#define BSE_MAX_NOTE            SFI_MAX_NOTE
#define BSE_KAMMER_NOTE         SFI_KAMMER_NOTE
#define BSE_NOTE_VOID           SFI_NOTE_VOID
#define BSE_NOTE_UNPARSABLE     SFI_NOTE_VOID
#define BSE_NOTE_CLAMP          SFI_NOTE_CLAMP
#define BSE_NOTE_IS_VALID       SFI_NOTE_IS_VALID
#define BSE_NOTE_MAKE_VALID     SFI_NOTE_MAKE_VALID
#define BSE_KAMMER_OCTAVE       SFI_KAMMER_OCTAVE
#define BSE_MIN_OCTAVE          SFI_MIN_OCTAVE
#define BSE_MAX_OCTAVE          SFI_MAX_OCTAVE
#define bse_note_from_string    sfi_note_from_string
#define bse_note_to_string      sfi_note_to_string
#define bse_note_examine        sfi_note_examine


/* --- construct notes --- */
#define BSE_NOTE_OCTAVE(n)              SFI_NOTE_OCTAVE (n)
#define BSE_NOTE_SEMITONE(n)            SFI_NOTE_SEMITONE (n)
#define BSE_NOTE_GENERIC(o,ht_i)        SFI_NOTE_GENERIC (o, ht_i)
#define BSE_NOTE_C(o)                   (BSE_NOTE_GENERIC ((o), 0))
#define BSE_NOTE_Cis(o)                 (BSE_NOTE_GENERIC ((o), 1))
#define BSE_NOTE_Des(o)                 (BSE_NOTE_Cis (o))
#define BSE_NOTE_D(o)                   (BSE_NOTE_GENERIC ((o), 2))
#define BSE_NOTE_Dis(o)                 (BSE_NOTE_GENERIC ((o), 3))
#define BSE_NOTE_Es(o)                  (BSE_NOTE_Dis (o))
#define BSE_NOTE_E(o)                   (BSE_NOTE_GENERIC ((o), 4))
#define BSE_NOTE_F(o)                   (BSE_NOTE_GENERIC ((o), 5))
#define BSE_NOTE_Fis(o)                 (BSE_NOTE_GENERIC ((o), 6))
#define BSE_NOTE_Ges(o)                 (BSE_NOTE_Fis (o))
#define BSE_NOTE_G(o)                   (BSE_NOTE_GENERIC ((o), 7))
#define BSE_NOTE_Gis(o)                 (BSE_NOTE_GENERIC ((o), 8))
#define BSE_NOTE_As(o)                  (BSE_NOTE_Gis (o))
#define BSE_NOTE_A(o)                   (BSE_NOTE_GENERIC ((o), 9))
#define BSE_NOTE_Ais(o)                 (BSE_NOTE_GENERIC ((o), 10))
#define BSE_NOTE_Bes(o)                 (BSE_NOTE_Ais (o))
#define BSE_NOTE_B(o)                   (BSE_NOTE_GENERIC ((o), 11))
#define BSE_NOTE_SHIFT(n,ht_i)          SFI_NOTE_SHIFT (n, ht_i)
#define BSE_NOTE_OCTAVE_UP(n)           (BSE_NOTE_SHIFT ((n), +12))
#define BSE_NOTE_OCTAVE_DOWN(n)         (BSE_NOTE_SHIFT ((n), -12))


/* --- get frequencies from notes --- */
#define BSE_FREQ_GENERIC(o,ht_i)        (bse_freq_from_note (BSE_NOTE_GENERIC (o, ht_i)))
#define BSE_FREQ_C(o)                   (bse_freq_from_note (BSE_NOTE_C (o)))
#define BSE_FREQ_Cis(o)                 (bse_freq_from_note (BSE_NOTE_Cis (o)))
#define BSE_FREQ_Des(o)                 (bse_freq_from_note (BSE_NOTE_Des (o)))
#define BSE_FREQ_D(o)                   (bse_freq_from_note (BSE_NOTE_D (o)))
#define BSE_FREQ_Dis(o)                 (bse_freq_from_note (BSE_NOTE_Dis (o)))
#define BSE_FREQ_Es(o)                  (bse_freq_from_note (BSE_NOTE_Es (o)))
#define BSE_FREQ_E(o)                   (bse_freq_from_note (BSE_NOTE_E (o)))
#define BSE_FREQ_F(o)                   (bse_freq_from_note (BSE_NOTE_F (o)))
#define BSE_FREQ_Fis(o)                 (bse_freq_from_note (BSE_NOTE_Fis (o)))
#define BSE_FREQ_Ges(o)                 (bse_freq_from_note (BSE_NOTE_Ges (o)))
#define BSE_FREQ_G(o)                   (bse_freq_from_note (BSE_NOTE_G (o)))
#define BSE_FREQ_Gis(o)                 (bse_freq_from_note (BSE_NOTE_Gis (o)))
#define BSE_FREQ_As(o)                  (bse_freq_from_note (BSE_NOTE_As (o)))
#define BSE_FREQ_A(o)                   (bse_freq_from_note (BSE_NOTE_A (o)))
#define BSE_FREQ_Ais(o)                 (bse_freq_from_note (BSE_NOTE_Ais (o)))
#define BSE_FREQ_Bes(o)                 (bse_freq_from_note (BSE_NOTE_Bes (o)))
#define BSE_FREQ_B(o)                   (bse_freq_from_note (BSE_NOTE_B (o)))


/* --- functions --- */
gdouble                 bse_freq_from_note              (gint           note);
gint
bse_note_from_freq (gdouble freq);
gint
bse_note_from_freq_bounded (gdouble freq);
gint
bse_note_fine_tune_from_note_freq (gint    note,
                                   gdouble freq);
gdouble
bse_note_to_freq (gint note);
gdouble
bse_note_to_tuned_freq (gint note,
                        gint fine_tune);

/* --- freq array --- */
typedef struct BseFreqArray BseFreqArray;
BseFreqArray*   bse_freq_array_new              (guint           prealloc);
void            bse_freq_array_free             (BseFreqArray   *farray);
guint           bse_freq_array_n_values         (BseFreqArray   *farray);
gdouble         bse_freq_array_get              (BseFreqArray   *farray,
                                                 guint           index);
void            bse_freq_array_insert           (BseFreqArray   *farray,
                                                 guint           index,
                                                 gdouble         value);
void            bse_freq_array_append           (BseFreqArray   *farray,
                                                 gdouble         value);
#define         bse_freq_array_prepend(a,v)      bse_freq_array_insert ((a), 0, (v))
void            bse_freq_array_set              (BseFreqArray   *farray,
                                                 guint           index,
                                                 gdouble         value);
/* find match_freq in inclusive_set (NULL acts as wildcard) and don't
 * find match_freq in exclusive_set (NULL acts as empty set).
 */
gboolean        bse_freq_arrays_match_freq      (gfloat          match_freq,
                                                 BseFreqArray   *inclusive_set,
                                                 BseFreqArray   *exclusive_set);

G_END_DECLS

#endif /* __BSE_NOTE_H__ */
