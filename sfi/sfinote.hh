// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_NOTE_H__
#define __SFI_NOTE_H__
#include <sfi/sfitypes.hh>
G_BEGIN_DECLS
/* --- (MIDI) notes --- */
/* notes are generally kept in signed integers. though they are zero
 * bounded, they are often used within expression that shouldn't be
 * promoted into unsigned integer expressions.
 */
#define	SFI_MIN_NOTE			(0)	/* assumed to be 0 in various places */
#define	SFI_MAX_NOTE			(131 /* 123 */)
/* special note value to represent "no value specified"
 * or unparsable notes.
 */
#define	SFI_NOTE_VOID			(SFI_MAX_NOTE + 1)
/* kammer note, representing the kammer frequency's midi note value */
#define	SFI_KAMMER_NOTE			((SfiInt) (69) /* A' */)
#define	SFI_KAMMER_OCTAVE		((SfiInt) (+1))
/* resulting minimum and maximum octaves */
#define	SFI_MIN_OCTAVE			(SFI_NOTE_OCTAVE (SFI_MIN_NOTE))
#define	SFI_MAX_OCTAVE			(SFI_NOTE_OCTAVE (SFI_MAX_NOTE))
/* macro to retrieve a valid note. simply defaults
 * to kammer note for invalid note values.
 */
#define	SFI_NOTE_MAKE_VALID(n)		((n) > SFI_MAX_NOTE || (n) < SFI_MIN_NOTE ? SFI_KAMMER_NOTE : ((SfiInt) (n)))
#define	SFI_NOTE_IS_VALID(n)		((n) >= SFI_MIN_NOTE && (n) <= SFI_MAX_NOTE)
/* clamp note against boundaries in cases of underflow or overflow */
#define	SFI_NOTE_CLAMP(n)		(CLAMP (((SfiInt) (n)), SFI_MIN_NOTE, SFI_MAX_NOTE))
/* macros to compose and decompose note values into semitones and octaves */
#define	SFI_NOTE_OCTAVE(n)		((((SfiInt) (n)) - SFI_NOTE_SEMITONE (n) - (SFI_KAMMER_NOTE - 9)) / 12 + SFI_KAMMER_OCTAVE)
#define	SFI_NOTE_SEMITONE(n)		(((SfiInt) (n)) % 12 + (9 - (SFI_KAMMER_NOTE % 12)))
#define	SFI_NOTE_GENERIC(o,ht_i)	(SFI_KAMMER_NOTE - 9 + ((SfiInt) (ht_i)) + (((gint) (o)) - SFI_KAMMER_OCTAVE) * 12)
#define	SFI_NOTE_C(o)			(SFI_NOTE_GENERIC ((o), 0))
#define	SFI_NOTE_Cis(o)			(SFI_NOTE_GENERIC ((o), 1))
#define	SFI_NOTE_Des(o)			(SFI_NOTE_Cis (o))
#define	SFI_NOTE_D(o)			(SFI_NOTE_GENERIC ((o), 2))
#define	SFI_NOTE_Dis(o)			(SFI_NOTE_GENERIC ((o), 3))
#define	SFI_NOTE_Es(o)			(SFI_NOTE_Dis (o))
#define	SFI_NOTE_E(o)			(SFI_NOTE_GENERIC ((o), 4))
#define	SFI_NOTE_F(o)			(SFI_NOTE_GENERIC ((o), 5))
#define	SFI_NOTE_Fis(o)			(SFI_NOTE_GENERIC ((o), 6))
#define	SFI_NOTE_Ges(o)			(SFI_NOTE_Fis (o))
#define	SFI_NOTE_G(o)			(SFI_NOTE_GENERIC ((o), 7))
#define	SFI_NOTE_Gis(o)			(SFI_NOTE_GENERIC ((o), 8))
#define	SFI_NOTE_As(o)			(SFI_NOTE_Gis (o))
#define	SFI_NOTE_A(o)			(SFI_NOTE_GENERIC ((o), 9))
#define	SFI_NOTE_Ais(o)			(SFI_NOTE_GENERIC ((o), 10))
#define	SFI_NOTE_Bes(o)			(SFI_NOTE_Ais (o))
#define	SFI_NOTE_B(o)			(SFI_NOTE_GENERIC ((o), 11))
#define	_SFI_NOTE_SHIFT_AUX(n,ht,dfl)	(n + ht >= SFI_MIN_NOTE && n + ht <= SFI_MAX_NOTE ? n + ht : dfl)
#define	SFI_NOTE_SHIFT(n,ht_i)		(_SFI_NOTE_SHIFT_AUX ((SfiInt) (n), (gint) (ht), (SfiInt) (n)))
#define	SFI_NOTE_OCTAVE_UP(n)		(SFI_NOTE_SHIFT ((n), +12))
#define	SFI_NOTE_OCTAVE_DOWN(n)		(SFI_NOTE_SHIFT ((n), -12))
/* --- functions --- */
void	sfi_note_examine	 (SfiInt	 note,
				  gint          *octave_p,
				  gint          *semitone_p,
				  gboolean	*black_semitone_p,
				  gchar         *letter_p);
/* return a newly allocated string which describes `note' literally */
gchar*	sfi_note_to_string	 (SfiInt	 note);
/* return the numeric value of the note in `note_string' */
SfiInt	sfi_note_from_string	 (const gchar	*note_string);
SfiInt	sfi_note_from_string_err (const gchar	*note_string,
				  gchar        **error_p);
G_END_DECLS
#endif /* __SFI_NOTE_H__ */
/* vim:set ts=8 sts=2 sw=2: */
