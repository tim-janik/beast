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
#include "bsesequence.h"




/* --- functions --- */
static gpointer
sequence_copy (gpointer boxed)
{
  return boxed ? bse_sequence_copy (boxed) : NULL;
}

static void
sequence_free (gpointer boxed)
{
  if (boxed)
    bse_sequence_free (boxed);
}

GType
bse_sequence_get_type (void)
{
  static GType type;

  if (!type)
    type = g_boxed_type_register_static ("BseSequence",
					 sequence_copy,
					 sequence_free);
  return type;
}

BseSequence*
bse_sequence_new (guint n_notes,
		  gint  offset)
{
  BseSequence *seq;

  g_return_val_if_fail (n_notes > 0, NULL);

  seq = g_malloc0 (sizeof (BseSequence) + sizeof (seq->notes[0]) * (n_notes - 1));
  seq->n_notes = n_notes;
  seq->offset = offset;

  return seq;
}

BseSequence*
bse_sequence_copy (const BseSequence *seq)
{
  guint size;

  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (seq->n_notes > 0, NULL);

  size = sizeof (BseSequence) + sizeof (seq->notes[0]) * (seq->n_notes - 1);

  return g_memdup (seq, size);
}

void
bse_sequence_free (BseSequence *seq)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (seq->n_notes > 0);

  g_free (seq);
}

BseSequence*
bse_sequence_resize (BseSequence *seq,
		     guint        n_notes)
{
  guint size, i;
  
  g_return_val_if_fail (seq != NULL, seq);
  g_return_val_if_fail (seq->n_notes > 0, seq);
  g_return_val_if_fail (n_notes > 0, seq);

  i = seq->n_notes;
  seq->n_notes = n_notes;
  size = sizeof (BseSequence) + sizeof (seq->notes[0]) * (seq->n_notes - 1);
  seq = g_realloc (seq, size);
  if (i < seq->n_notes)
    memset (seq->notes + i, 0, sizeof (seq->notes[0]) * (seq->n_notes - i));
  return seq;
}
