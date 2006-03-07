/* BIRNET - Synthesis Fusion Kit Interface
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
#ifndef __BIRNET_PRIMITIVES_H__
#define __BIRNET_PRIMITIVES_H__

#include <birnet/birnetvalues.h>

G_BEGIN_DECLS


/* --- BirnetBBlock primitive type --- */
struct _BirnetBBlock {
  guint   ref_count;
  guint   n_bytes;
  guint8 *bytes;
};
BirnetBBlock* birnet_bblock_new	   (void);
BirnetBBlock* birnet_bblock_new_sized	   (guint	     size);
BirnetBBlock* birnet_bblock_ref	   (BirnetBBlock	    *bblock);
void	   birnet_bblock_unref	   (BirnetBBlock	    *bblock);
void	   birnet_bblock_resize	   (BirnetBBlock       *bblock,
				    guint            size);
BirnetBBlock* birnet_bblock_copy_deep	   (const BirnetBBlock *bblock);
#define	   birnet_bblock_copy_shallow birnet_bblock_ref
void	   birnet_bblock_append	   (BirnetBBlock	    *bblock,
				    guint            n_bytes,
				    const guint8    *bytes);
void	   birnet_bblock_append1	   (BirnetBBlock	    *bblock,
				    guint8	     byte0);
guint	   birnet_bblock_length	   (const BirnetBBlock *bblock);
guint8*	   birnet_bblock_get	   (const BirnetBBlock *bblock);


/* --- BirnetFBlock primitive type --- */
struct _BirnetFBlock {
  guint     ref_count;
  guint     n_values;
  gfloat   *values;
  GFreeFunc freefunc;
};
BirnetFBlock* birnet_fblock_new	   (void);
BirnetFBlock* birnet_fblock_new_sized	   (guint	     size);
BirnetFBlock* birnet_fblock_new_foreign  (guint            n_values,
                                    gfloat          *values,
                                    GFreeFunc        freefunc);
BirnetFBlock* birnet_fblock_ref	   (BirnetFBlock	    *fblock);
void	   birnet_fblock_unref	   (BirnetFBlock	    *fblock);
void	   birnet_fblock_resize	   (BirnetFBlock	    *fblock,
				    guint	     size);
BirnetFBlock* birnet_fblock_copy_deep	   (const BirnetFBlock *fblock);
#define	   birnet_fblock_copy_shallow birnet_fblock_ref
void	   birnet_fblock_append	   (BirnetFBlock	    *fblock,
				    guint            n_values,
				    const gfloat    *values);
void	   birnet_fblock_append1	   (BirnetFBlock	    *fblock,
				    gfloat	     float0);
guint	   birnet_fblock_length	   (const BirnetFBlock *fblock);
gfloat*	   birnet_fblock_get	   (const BirnetFBlock *fblock);


/* --- BirnetSeq primitive type --- */
struct _BirnetSeq {
  guint   ref_count;
  guint   n_elements;
  GValue *elements;
};
BirnetSeq*	 birnet_seq_new		(void);
BirnetSeq*	 birnet_seq_ref		(BirnetSeq		 *seq);
void	 birnet_seq_unref		(BirnetSeq		 *seq);
BirnetSeq*	 birnet_seq_copy_deep	(const BirnetSeq	 *seq);
#define	 birnet_seq_copy_shallow	birnet_seq_ref
void	 birnet_seq_append		(BirnetSeq		 *seq,
				 const GValue	 *value);
GValue*	 birnet_seq_append_empty	(BirnetSeq		 *seq,
				 GType            value_type);
void	 birnet_seq_clear		(BirnetSeq		 *seq);
guint	 birnet_seq_length		(const BirnetSeq	 *seq);
GValue*	 birnet_seq_get		(const BirnetSeq	 *seq,
				 guint		  index);
gboolean birnet_seq_check		(BirnetSeq		 *seq,
				 GType		  element_type);
gboolean birnet_seq_validate       (BirnetSeq          *seq,
                                 GParamSpec      *pspec);
/* convenience */
void     birnet_seq_append_bool	(BirnetSeq          *seq,
				 BirnetBool	  v_bool);
void     birnet_seq_append_int	(BirnetSeq          *seq,
				 BirnetInt	  	  v_int);
void     birnet_seq_append_num	(BirnetSeq          *seq,
				 BirnetNum		  v_num);
void     birnet_seq_append_real	(BirnetSeq          *seq,
				 BirnetReal	  v_real);
void     birnet_seq_append_string	(BirnetSeq          *seq,
				 const gchar	 *string);
void     birnet_seq_append_choice	(BirnetSeq          *seq,
				 const gchar	 *choice);
void     birnet_seq_append_bblock	(BirnetSeq          *seq,
				 BirnetBBlock	 *bblock);
void     birnet_seq_append_fblock	(BirnetSeq          *seq,
				 BirnetFBlock	 *fblock);
void     birnet_seq_append_pspec	(BirnetSeq          *seq,
				 GParamSpec	 *pspec);
void     birnet_seq_append_seq	(BirnetSeq          *seq,
				 BirnetSeq		 *v_seq);
void     birnet_seq_append_rec	(BirnetSeq          *seq,
				 BirnetRec		 *rec);
void     birnet_seq_append_proxy	(BirnetSeq          *seq,
				 BirnetProxy	  proxy);
BirnetBool	     birnet_seq_get_bool	(BirnetSeq		*seq,
				 guint		 index);
BirnetInt	     birnet_seq_get_int	(BirnetSeq		*seq,
				 guint           index);
BirnetNum	     birnet_seq_get_num	(BirnetSeq		*seq,
				 guint           index);
BirnetReal	     birnet_seq_get_real	(BirnetSeq		*seq,
				 guint           index);
const gchar* birnet_seq_get_string (BirnetSeq		*seq,
				 guint           index);
const gchar* birnet_seq_get_choice (BirnetSeq		*seq,
				 guint           index);
BirnetBBlock*   birnet_seq_get_bblock	(BirnetSeq		*seq,
				 guint           index);
BirnetFBlock*   birnet_seq_get_fblock	(BirnetSeq		*seq,
				 guint           index);
GParamSpec*  birnet_seq_get_pspec	(BirnetSeq		*seq,
				 guint           index);
BirnetSeq*	     birnet_seq_get_seq	(BirnetSeq		*seq,
				 guint           index);
BirnetRec*	     birnet_seq_get_rec	(BirnetSeq		*seq,
				 guint           index);
BirnetProxy     birnet_seq_get_proxy	(BirnetSeq		*seq,
				 guint           index);
/* conversion convenience */
gchar**	     birnet_seq_to_strv	(BirnetSeq		*seq);
BirnetSeq*	     birnet_seq_from_strv	(gchar	       **strv);
BirnetSeq*	     birnet_seq_from_cstrv	(const gchar   **strv);


/* --- BirnetRec primitive type --- */
struct _BirnetRec {
  guint    ref_count;
  guint    n_fields;
  guint	   sorted : 1;
  GValue  *fields;
  gchar  **field_names;
};
BirnetRec*	   birnet_rec_new		(void);
BirnetRec*	   birnet_rec_ref		(BirnetRec		 *rec);
void	   birnet_rec_unref	(BirnetRec          *rec);
BirnetRec*	   birnet_rec_copy_deep	(BirnetRec		 *rec);
#define	   birnet_rec_copy_shallow	birnet_rec_ref
void       birnet_rec_swap_fields  (BirnetRec          *rec,
				 BirnetRec		 *swapper);
gboolean   birnet_rec_validate	(BirnetRec		 *rec,
				 BirnetRecFields	  fields);
void       birnet_rec_clear        (BirnetRec          *rec);
void       birnet_rec_set          (BirnetRec          *rec,
				 const gchar     *field_name,
				 const GValue    *value);
GValue*    birnet_rec_get          (BirnetRec          *rec,
				 const gchar     *field_name);
GValue*    birnet_rec_forced_get   (BirnetRec          *rec,
                                 const gchar     *field_name,
                                 GType            gtype);
guint      birnet_rec_n_fields     (const BirnetRec    *rec);
GValue*    birnet_rec_field        (const BirnetRec    *rec,
				 guint            index);
gboolean   birnet_rec_check	(BirnetRec		 *rec,
				 BirnetRecFields	  rfields);
void	   birnet_rec_sort		(BirnetRec          *rec);
/* convenience */
void       birnet_rec_set_bool	(BirnetRec          *rec,
				 const gchar     *field_name,
				 BirnetBool	  v_bool);
void       birnet_rec_set_int	(BirnetRec          *rec,
				 const gchar     *field_name,
				 BirnetInt	  	  v_int);
void       birnet_rec_set_num	(BirnetRec          *rec,
				 const gchar     *field_name,
				 BirnetNum		  v_num);
void       birnet_rec_set_real	(BirnetRec          *rec,
				 const gchar     *field_name,
				 BirnetReal	  v_real);
void       birnet_rec_set_string	(BirnetRec          *rec,
				 const gchar     *field_name,
				 const gchar	 *string);
void       birnet_rec_set_choice	(BirnetRec          *rec,
				 const gchar     *field_name,
				 const gchar	 *choice);
void       birnet_rec_set_bblock	(BirnetRec          *rec,
				 const gchar     *field_name,
				 BirnetBBlock	 *bblock);
void       birnet_rec_set_fblock	(BirnetRec          *rec,
				 const gchar     *field_name,
				 BirnetFBlock	 *fblock);
void       birnet_rec_set_pspec	(BirnetRec          *rec,
				 const gchar     *field_name,
				 GParamSpec	 *pspec);
void       birnet_rec_set_seq	(BirnetRec          *rec,
				 const gchar     *field_name,
				 BirnetSeq		 *seq);
void       birnet_rec_set_rec	(BirnetRec          *rec,
				 const gchar     *field_name,
				 BirnetRec		 *v_rec);
void       birnet_rec_set_proxy	(BirnetRec          *rec,
				 const gchar     *field_name,
				 BirnetProxy	  proxy);
BirnetBool	     birnet_rec_get_bool	(BirnetRec		*rec,
				 const gchar	*field_name);
BirnetInt	     birnet_rec_get_int	(BirnetRec		*rec,
				 const gchar	*field_name);
BirnetNum	     birnet_rec_get_num	(BirnetRec		*rec,
				 const gchar	*field_name);
BirnetReal	     birnet_rec_get_real	(BirnetRec		*rec,
				 const gchar	*field_name);
const gchar* birnet_rec_get_string (BirnetRec		*rec,
				 const gchar	*field_name);
const gchar* birnet_rec_get_choice (BirnetRec		*rec,
				 const gchar	*field_name);
BirnetBBlock*   birnet_rec_get_bblock	(BirnetRec		*rec,
				 const gchar	*field_name);
BirnetFBlock*   birnet_rec_get_fblock	(BirnetRec		*rec,
				 const gchar	*field_name);
GParamSpec*  birnet_rec_get_pspec	(BirnetRec		*rec,
				 const gchar	*field_name);
BirnetSeq*	     birnet_rec_get_seq	(BirnetRec		*rec,
				 const gchar	*field_name);
BirnetRec*	     birnet_rec_get_rec	(BirnetRec		*rec,
				 const gchar	*field_name);
BirnetProxy     birnet_rec_get_proxy	(BirnetRec		*rec,
				 const gchar	*field_name);


/* --- basic comparisons --- */
typedef gint (*BirnetCompareFunc)          (gconstpointer   value1,
                                         gconstpointer   value2,
                                         gpointer        data);
gint     birnet_pointer_cmp                (gconstpointer   value1,
                                         gconstpointer   value2,
                                         gpointer        dummy);


/* --- ring (circular-list) --- */
typedef gpointer (*BirnetRingDataFunc)	(gpointer	 data,
					 gpointer	 func_data);
struct _BirnetRing		// FIXME: move BirnetRing into its own object file
{
  gpointer  data;
  BirnetRing  *next;
  BirnetRing  *prev;
};
BirnetRing*        birnet_ring_prepend        (BirnetRing        *head,
                                         gpointer        data);
BirnetRing*        birnet_ring_prepend_uniq   (BirnetRing        *head,
                                         gpointer        data);
BirnetRing*        birnet_ring_append         (BirnetRing        *head,
                                         gpointer        data);
BirnetRing*        birnet_ring_append_uniq    (BirnetRing        *head,
                                         gpointer        data);
BirnetRing*        birnet_ring_insert         (BirnetRing        *head,
                                         gpointer        data,
                                         gint            position);
BirnetRing*        birnet_ring_insert_before  (BirnetRing        *head,
                                         BirnetRing        *sibling,
                                         gpointer        data);
gint            birnet_ring_position       (const BirnetRing  *head,
                                         const BirnetRing  *node);
gint            birnet_ring_index          (const BirnetRing  *head,
                                         gconstpointer   data);
BirnetRing*        birnet_ring_nth            (const BirnetRing  *head,
                                         guint           n);
gpointer        birnet_ring_nth_data       (const BirnetRing  *head,
                                         guint           n);
BirnetRing*        birnet_ring_find           (const BirnetRing  *head,
                                         gconstpointer   data);
BirnetRing*        birnet_ring_remove_node    (BirnetRing        *head,
                                         BirnetRing        *node);
BirnetRing*        birnet_ring_remove         (BirnetRing        *head,
                                         gpointer        data);
guint           birnet_ring_length         (const BirnetRing  *head);
gint            birnet_ring_cmp_length     (const BirnetRing  *head,
                                         guint           test_length);
BirnetRing*        birnet_ring_copy           (const BirnetRing  *head);
BirnetRing*        birnet_ring_copy_deep      (const BirnetRing  *head,
                                         BirnetRingDataFunc copy,
                                         gpointer        func_data);
BirnetRing*        birnet_ring_copy_rest      (const BirnetRing  *ring,
                                         const BirnetRing  *head);
BirnetRing*        birnet_ring_concat         (BirnetRing        *head1,
                                         BirnetRing        *head2);
BirnetRing*        birnet_ring_split          (BirnetRing        *head1,
                                         BirnetRing        *head2);
BirnetRing*        birnet_ring_reverse        (BirnetRing        *head);
gpointer        birnet_ring_pop_head       (BirnetRing       **head);
gpointer        birnet_ring_pop_tail       (BirnetRing       **head);
#define         birnet_ring_push_head      birnet_ring_prepend
#define         birnet_ring_push_tail      birnet_ring_append
void            birnet_ring_free           (BirnetRing        *head);
void            birnet_ring_free_deep      (BirnetRing        *head,
                                         GDestroyNotify  data_destroy);
#define birnet_ring_tail(head)             ((head) ? (head)->prev : NULL)
#define birnet_ring_walk(node,head_bound)  ((node)->next != (head_bound) ? (node)->next : NULL)
#define birnet_ring_next                   birnet_ring_walk

BirnetRing* birnet_ring_from_list             (GList          *list);
BirnetRing* birnet_ring_from_list_and_free    (GList          *list);
BirnetRing* birnet_ring_from_slist            (GSList         *slist);
BirnetRing* birnet_ring_from_slist_and_free   (GSList         *slist);

/* ring-modifying cmp-based operations */
BirnetRing* birnet_ring_insert_sorted         (BirnetRing        *head,
                                         gpointer        insertion_data,
                                         BirnetCompareFunc  cmp,
                                         gpointer        cmp_data);
BirnetRing* birnet_ring_merge_sorted          (BirnetRing        *head1,
                                         BirnetRing        *head2,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
BirnetRing* birnet_ring_sort                  (BirnetRing        *head,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
BirnetRing* birnet_ring_uniq                  (BirnetRing        *sorted_ring1,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
BirnetRing* birnet_ring_uniq_free_deep        (BirnetRing        *sorted_ring1,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data,
                                         GDestroyNotify  data_destroy);
BirnetRing* birnet_ring_reorder               (BirnetRing        *unordered_ring,
                                         const BirnetRing  *new_ring_order);
/* ring-copying cmp-based operations */
BirnetRing* birnet_ring_copy_deep_uniq        (const BirnetRing  *sorted_ring1,
                                         BirnetRingDataFunc copy,
                                         gpointer        copy_data,
                                         BirnetCompareFunc  cmp,
                                         gpointer        cmp_data);
BirnetRing* birnet_ring_copy_uniq             (const BirnetRing  *sorted_ring1,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
BirnetRing* birnet_ring_union                 (const BirnetRing  *sorted_set1,
                                         const BirnetRing  *sorted_set2,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
BirnetRing* birnet_ring_intersection          (const BirnetRing  *sorted_set1,
                                         const BirnetRing  *sorted_set2,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
BirnetRing* birnet_ring_difference            (const BirnetRing  *sorted_set1,
                                         const BirnetRing  *sorted_set2,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
BirnetRing* birnet_ring_symmetric_difference  (const BirnetRing  *sorted_set1,
                                         const BirnetRing  *sorted_set2,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
/* const-result cmp-based operations */
gboolean birnet_ring_includes              (const BirnetRing  *sorted_super_set,
                                         const BirnetRing  *sorted_sub_set,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
gboolean birnet_ring_mismatch              (BirnetRing       **sorted_ring1_p,
                                         BirnetRing       **sorted_ring2_p,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
gboolean birnet_ring_equals                (const BirnetRing  *sorted_set1,
                                         const BirnetRing  *sorted_set2,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
BirnetRing* birnet_ring_min_node              (const BirnetRing  *head,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
BirnetRing* birnet_ring_max_node              (const BirnetRing  *head,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
gpointer birnet_ring_min                   (const BirnetRing  *head,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);
gpointer birnet_ring_max                   (const BirnetRing  *head,
                                         BirnetCompareFunc  cmp,
                                         gpointer        data);

G_END_DECLS

#endif /* __BIRNET_PRIMITIVES_H__ */

/* vim:set ts=8 sts=2 sw=2: */
