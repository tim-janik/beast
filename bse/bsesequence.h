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
#ifndef __BSE_SEQUENCE_H__
#define __BSE_SEQUENCE_H__

#include        <bse/bsedefs.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- macros --- */
#define	BSE_TYPE_SEQUENCE	(bse_sequence_get_type ())


/* --- structures --- */
typedef struct {
  guint  n_notes;
  gint  offset;		/* recommended */
  struct {
    gint note;
  } notes[1];	/* flexible array */
} BseSequence;



/* --- prototypes --- */
GType		bse_sequence_get_type	(void);
BseSequence*	bse_sequence_new	(guint			 n_notes,
					 gint			 offset);
BseSequence*	bse_sequence_copy	(const BseSequence	*sequence);
void		bse_sequence_free	(BseSequence		*sequence);
BseSequence*	bse_sequence_resize	(BseSequence		*sequence,
					 guint			 n_notes);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SEQUENCE_H__ */
