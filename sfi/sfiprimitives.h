/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __SFI_PRIMITIVES_H__
#define __SFI_PRIMITIVES_H__

#include <sfi/sfivalues.h>

G_BEGIN_DECLS


/* --- SfiBBlock primitive type --- */
struct _SfiBBlock {
  guint   ref_count;
  guint   n_bytes;
  guint8 *bytes;
};
SfiBBlock* sfi_bblock_new	   (void);
SfiBBlock* sfi_bblock_new_sized	   (guint	     size);
SfiBBlock* sfi_bblock_ref	   (SfiBBlock	    *bblock);
void	   sfi_bblock_unref	   (SfiBBlock	    *bblock);
void	   sfi_bblock_resize	   (SfiBBlock       *bblock,
				    guint            size);
SfiBBlock* sfi_bblock_copy_deep	   (const SfiBBlock *bblock);
#define	   sfi_bblock_copy_shallow sfi_bblock_ref
void	   sfi_bblock_append	   (SfiBBlock	    *bblock,
				    guint            n_bytes,
				    const guint8    *bytes);
void	   sfi_bblock_append1	   (SfiBBlock	    *bblock,
				    guint8	     byte0);
guint	   sfi_bblock_length	   (const SfiBBlock *bblock);
guint8*	   sfi_bblock_get	   (const SfiBBlock *bblock);


/* --- SfiFBlock primitive type --- */
struct _SfiFBlock {
  guint   ref_count;
  guint   n_values;
  gfloat *values;
};
SfiFBlock* sfi_fblock_new	   (void);
SfiFBlock* sfi_fblock_new_sized	   (guint	     size);
SfiFBlock* sfi_fblock_ref	   (SfiFBlock	    *fblock);
void	   sfi_fblock_unref	   (SfiFBlock	    *fblock);
void	   sfi_fblock_resize	   (SfiFBlock	    *fblock,
				    guint	     size);
SfiFBlock* sfi_fblock_copy_deep	   (const SfiFBlock *fblock);
#define	   sfi_fblock_copy_shallow sfi_fblock_ref
void	   sfi_fblock_append	   (SfiFBlock	    *fblock,
				    guint            n_values,
				    const gfloat    *values);
void	   sfi_fblock_append1	   (SfiFBlock	    *fblock,
				    gfloat	     float0);
guint	   sfi_fblock_length	   (const SfiFBlock *fblock);
gfloat*	   sfi_fblock_get	   (const SfiFBlock *fblock);


/* --- SfiSeq primitive type --- */
struct _SfiSeq {
  guint   ref_count;
  guint   n_elements;
  GValue *elements;
};
SfiSeq*	   sfi_seq_new		(void);
SfiSeq*	   sfi_seq_ref		(SfiSeq		 *seq);
void	   sfi_seq_unref	(SfiSeq		 *seq);
SfiSeq*	   sfi_seq_copy_deep	(const SfiSeq	 *seq);
#define	   sfi_seq_copy_shallow	sfi_seq_ref
void	   sfi_seq_append	(SfiSeq		 *seq,
				 const GValue	 *value);
void	   sfi_seq_clear	(SfiSeq		 *seq);
guint	   sfi_seq_length	(const SfiSeq	 *seq);
GValue*	   sfi_seq_get		(const SfiSeq	 *seq,
				 guint		  index);
gboolean   sfi_seq_check	(SfiSeq		 *seq,
				 GType		  element_type);


/* --- SfiRec primitive type --- */
struct _SfiRec {
  guint    ref_count;
  guint    n_fields;
  GValue  *fields;
  gchar  **field_names;
};
SfiRec*	   sfi_rec_new		(void);
SfiRec*	   sfi_rec_ref		(SfiRec		 *rec);
void	   sfi_rec_unref	(SfiRec          *rec);
SfiRec*	   sfi_rec_copy_deep	(const SfiRec	 *rec);
#define	   sfi_rec_copy_shallow	sfi_rec_ref
void       sfi_rec_set          (SfiRec          *rec,
				 const gchar     *field_name,
				 const GValue    *value);
GValue*    sfi_rec_get          (const SfiRec    *rec,
				 const gchar     *field_name);
guint      sfi_rec_n_fields     (const SfiRec    *rec);
GValue*    sfi_rec_field        (const SfiRec    *rec,
				 guint            index);
gboolean   sfi_rec_check	(SfiRec		 *rec,
				 SfiRecFields	  rfields);


/* --- ring (circular-list) --- */
struct _SfiRing		// FIXME: move rings into their own object files
{
  SfiRing  *next;
  SfiRing  *prev;
  gpointer  data;
};
SfiRing*        sfi_ring_prepend        (SfiRing        *head,
					 gpointer        data);
SfiRing*        sfi_ring_prepend_uniq   (SfiRing        *head,
					 gpointer        data);
SfiRing*        sfi_ring_append         (SfiRing        *head,
					 gpointer        data);
SfiRing*        sfi_ring_insert_sorted  (SfiRing        *head,
					 gpointer        data,
					 GCompareFunc    func);
SfiRing*        sfi_ring_remove_node    (SfiRing        *head,
					 SfiRing        *node);
SfiRing*        sfi_ring_remove         (SfiRing        *head,
					 gpointer        data);
guint           sfi_ring_length         (SfiRing        *head);
SfiRing*        sfi_ring_concat         (SfiRing        *head1,
					 SfiRing        *head2);
SfiRing*        sfi_ring_find           (SfiRing        *head,
					 gconstpointer   data);
SfiRing*        sfi_ring_nth            (SfiRing        *head,
					 guint           n);
gpointer        sfi_ring_nth_data       (SfiRing        *head,
					 guint           n);
SfiRing*        sfi_ring_split          (SfiRing        *head1,
					 SfiRing        *head2);
SfiRing*        sfi_ring_sort           (SfiRing        *head,
					 GCompareFunc    func);
gpointer        sfi_ring_pop_head       (SfiRing       **head);
gpointer        sfi_ring_pop_tail       (SfiRing       **head);
#define         sfi_ring_push_head      sfi_ring_prepend
#define         sfi_ring_push_tail      sfi_ring_append
void            sfi_ring_free           (SfiRing        *head);
#define sfi_ring_tail(head)             ((head) ? (head)->prev : NULL)
#define sfi_ring_walk(head,node)        ((node) != (head)->prev ? (node)->next : NULL)


G_END_DECLS

#endif /* __SFI_PRIMITIVES_H__ */

/* vim:set ts=8 sts=2 sw=2: */
