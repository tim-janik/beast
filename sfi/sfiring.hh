/* SfiRing
 * Copyright (C) 2002-2006 Tim Janik
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
#ifndef __SFI_RING_H__
#define __SFI_RING_H__

#include <sfi/sfitypes.hh>

G_BEGIN_DECLS;


/* --- basic comparisons --- */
typedef gint (*SfiCompareFunc)		(gconstpointer   value1,
					 gconstpointer   value2,
					 gpointer        data);
gint     sfi_pointer_cmp             	(gconstpointer   value1,
					 gconstpointer   value2,
					 gpointer        dummy);


/* --- ring (circular-list) --- */
typedef gpointer (*SfiRingDataFunc)	(gpointer	 data,
					 gpointer	 func_data);
typedef struct SfiRing SfiRing;
struct SfiRing
{
  gpointer  data;
  SfiRing  *next;
  SfiRing  *prev;
};
SfiRing* sfi_ring_prepend               (SfiRing             *head,
                                         gpointer             data);
SfiRing* sfi_ring_prepend_uniq          (SfiRing             *head,
                                         gpointer             data);
SfiRing* sfi_ring_append                (SfiRing             *head,
                                         gpointer             data);
SfiRing* sfi_ring_append_uniq           (SfiRing             *head,
                                         gpointer             data);
SfiRing* sfi_ring_insert                (SfiRing             *head,
                                         gpointer             data,
                                         gint                 position);
SfiRing* sfi_ring_insert_before         (SfiRing             *head,
                                         SfiRing             *sibling,
                                         gpointer             data);
gint        sfi_ring_position           (const SfiRing       *head,
					 const SfiRing       *node);
gint        sfi_ring_index              (const SfiRing       *head,
					 gconstpointer        data);
SfiRing* sfi_ring_nth                	(const SfiRing       *head,
					 guint                n);
gpointer    sfi_ring_nth_data           (const SfiRing       *head,
					 guint                n);
SfiRing* sfi_ring_find               	(const SfiRing       *head,
					 gconstpointer        data);
SfiRing* sfi_ring_remove_node        	(SfiRing             *head,
					 SfiRing             *node);
SfiRing* sfi_ring_remove             	(SfiRing             *head,
					 gpointer             data);
guint       sfi_ring_length             (const SfiRing       *head);
gint        sfi_ring_cmp_length         (const SfiRing       *head,
					 guint                test_length);
SfiRing* sfi_ring_copy               	(const SfiRing       *head);
SfiRing* sfi_ring_copy_deep          	(const SfiRing       *head,
					 SfiRingDataFunc      copy,
					 gpointer             func_data);
SfiRing* sfi_ring_copy_rest          	(const SfiRing       *ring,
					 const SfiRing       *head);
SfiRing* sfi_ring_concat             	(SfiRing             *head1,
					 SfiRing             *head2);
SfiRing* sfi_ring_split              	(SfiRing             *head1,
					 SfiRing             *head2);
SfiRing* sfi_ring_reverse            	(SfiRing             *head);
gpointer    sfi_ring_pop_head           (SfiRing             **head);
gpointer    sfi_ring_pop_tail           (SfiRing             **head);
#define     sfi_ring_push_head        sfi_ring_prepend
#define     sfi_ring_push_tail        sfi_ring_append
void        sfi_ring_free               (SfiRing             *head);
void        sfi_ring_free_deep          (SfiRing             *head,
					 GDestroyNotify          data_destroy);

SfiRing* sfi_ring_from_list           	(GList       	*list);
SfiRing* sfi_ring_from_list_and_free	(GList          *list);
SfiRing* sfi_ring_from_slist            (GSList         *slist);
SfiRing* sfi_ring_from_slist_and_free	(GSList         *slist);
#define     sfi_ring_tail(head)            	((head) ? (head)->prev : NULL)
#define     sfi_ring_walk(node,head_bound) 	((node)->next != (head_bound) ? (node)->next : NULL)
#define     sfi_ring_next(node,head_bound)  	 sfi_ring_walk (node, head_bound)


/* ring-modifying cmp-based operations */
SfiRing* sfi_ring_insert_sorted		(SfiRing        	*head,
					 gpointer       	 insertion_data,
					 SfiCompareFunc 	 cmp,
					 gpointer       	 cmp_data);
SfiRing* sfi_ring_merge_sorted       	(SfiRing        	*head1,
					 SfiRing        	*head2,
					 SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
SfiRing* sfi_ring_sort               	(SfiRing        	*head,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
SfiRing* sfi_ring_uniq               	(SfiRing        	*sorted_ring1,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
SfiRing* sfi_ring_uniq_free_deep     	(SfiRing        	*sorted_ring1,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data,
                                         GDestroyNotify 	 data_destroy);
SfiRing* sfi_ring_reorder            	(SfiRing        	*unordered_ring,
					 const SfiRing  	*new_ring_order);
/* ring-copying cmp-based operations */
SfiRing* sfi_ring_copy_deep_uniq	(const SfiRing  	*sorted_ring1,
                                         SfiRingDataFunc	 copy,
                                         gpointer       	 copy_data,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 cmp_data);
SfiRing* sfi_ring_copy_uniq   		(const SfiRing  	*sorted_ring1,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
SfiRing* sfi_ring_union              	(const SfiRing  	*sorted_set1,
                                         const SfiRing  	*sorted_set2,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
SfiRing* sfi_ring_intersection       	(const SfiRing  	*sorted_set1,
                                         const SfiRing  	*sorted_set2,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
SfiRing* sfi_ring_difference    	(const SfiRing  	*sorted_set1,
                                         const SfiRing  	*sorted_set2,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
SfiRing* sfi_ring_symmetric_difference  (const SfiRing  	*sorted_set1,
					 const SfiRing  	*sorted_set2,
					 SfiCompareFunc 	 cmp,
					 gpointer       	 data);
/* const-result cmp-based operations */
gboolean    sfi_ring_includes      	(const SfiRing  	*sorted_super_set,
                                         const SfiRing  	*sorted_sub_set,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
gboolean    sfi_ring_mismatch      	(SfiRing               **sorted_ring1_p,
                                         SfiRing               **sorted_ring2_p,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
gboolean    sfi_ring_equals      	(const SfiRing  	*sorted_set1,
                                         const SfiRing  	*sorted_set2,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
SfiRing* sfi_ring_min_node    		(const SfiRing  	*head,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
SfiRing* sfi_ring_max_node    		(const SfiRing  	*head,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
gpointer    sfi_ring_min            	(const SfiRing  	*head,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);
gpointer    sfi_ring_max           	(const SfiRing  	*head,
                                         SfiCompareFunc 	 cmp,
                                         gpointer       	 data);

G_END_DECLS;

#endif /* __SFI_RING_H__ */

/* vim:set ts=8 sts=2 sw=2: */
