/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
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
#ifndef __BSE_COMPAT_H__
#define __BSE_COMPAT_H__

#include        <bse/bseenums.h>

G_BEGIN_DECLS


#define	BSE_MIN_NOTE		SFI_MIN_NOTE
#define	BSE_KAMMER_NOTE		SFI_KAMMER_NOTE
#define	BSE_MAX_NOTE		SFI_MAX_NOTE
#define	BSE_NOTE_CLAMP		SFI_NOTE_CLAMP
#define	BSE_NOTE_IS_VALID	SFI_NOTE_IS_VALID
#define	BSE_KAMMER_OCTAVE	SFI_KAMMER_OCTAVE
#define	BSE_MIN_OCTAVE		SFI_MIN_OCTAVE
#define	BSE_MAX_OCTAVE		SFI_MAX_OCTAVE
#define	BSE_NOTE_VOID		SFI_NOTE_VOID
#define	BSE_NOTE_UNPARSABLE	SFI_NOTE_VOID
#define	BSE_NOTE_GENERIC	SFI_NOTE_GENERIC
#define	bse_note_from_string	sfi_note_from_string
#define	bse_note_to_string	sfi_note_to_string
#define	bse_note_examine	sfi_note_examine

#define bse_param_spec_genum(name,nick,blurb, genum_type, default_value, hints) \
   bse_param_spec_enum (name,nick,blurb, default_value, genum_type, hints)
#define bse_param_spec_freq_simple(name, nick, blurb, hints) \
  bse_param_spec_freq (name, nick, blurb, BSE_KAMMER_FREQUENCY_f, hints)
#define bse_param_spec_fine_tune(name, nick, blurb)	\
  sfi_pspec_int (name, nick, blurb, 0, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE, 10, \
                 SFI_PARAM_HINT_SCALE SFI_PARAM_DEFAULT)
#define	bse_param_spec_octave(name, nick, blurb)	\
  sfi_pspec_int (name, nick, blurb, BSE_KAMMER_OCTAVE, \
                 BSE_MIN_OCTAVE, BSE_MAX_OCTAVE, 2, SFI_PARAM_DEFAULT)
#define bse_pspec_note(name, nick, blurb, default_value, hints) \
  sfi_pspec_note (name, nick, blurb, default_value, SFI_MIN_NOTE, SFI_MAX_NOTE, FALSE, hints)
#define bse_pspec_note_simple(name, nick, blurb, hints) \
  bse_pspec_note (name, nick, blurb, SFI_KAMMER_NOTE, hints)


G_END_DECLS

#endif /* __BSE_COMPAT_H__ */
