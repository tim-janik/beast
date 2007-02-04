/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BST_SEQUENCE_H__
#define __BST_SEQUENCE_H__

#include <gtk/gtk.h>
#include "bstbseutils.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BST_TYPE_SEQUENCE              (bst_sequence_get_type ())
#define BST_SEQUENCE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_SEQUENCE, BstSequence))
#define BST_SEQUENCE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_SEQUENCE, BstSequenceClass))
#define BST_IS_SEQUENCE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_SEQUENCE))
#define BST_IS_SEQUENCE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_SEQUENCE))
#define BST_SEQUENCE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_SEQUENCE, BstSequenceClass))


/* --- structures --- */
typedef struct _BstSequence	 BstSequence;
typedef struct _BstSequenceClass BstSequenceClass;
struct _BstSequence
{
  GtkHBox          parent_object;

  guint            entered : 1;
  GtkWidget       *darea;
  gint	           n_rows;
  BseNoteSequence *sdata;
};
struct _BstSequenceClass
{
  GtkHBoxClass parent_class;

  void	(*seq_changed)	(BstSequence	*sequence);
};


/* --- prototypes --- */
GType		bst_sequence_get_type	(void);
void		bst_sequence_set_seq	(BstSequence	 *seq,
					 BseNoteSequence *sdata);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_SEQUENCE_H__ */

