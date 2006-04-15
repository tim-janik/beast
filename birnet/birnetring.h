/* BirnetRing
 * Copyright (C) 2002-2006 Tim Janik
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
#ifndef __BIRNET_RING_H__
#define __BIRNET_RING_H__

#include <birnet/birnetcore.h>

G_BEGIN_DECLS


/* --- basic comparisons --- */
typedef gint (*BirnetCompareFunc)	(gconstpointer   value1,
                                         gconstpointer   value2,
                                         gpointer        data);
gint     birnet_pointer_cmp             (gconstpointer   value1,
                                         gconstpointer   value2,
                                         gpointer        dummy);


/* --- ring (circular-list) --- */
typedef gpointer (*BirnetRingDataFunc)	(gpointer	 data,
					 gpointer	 func_data);
typedef struct BirnetRing BirnetRing;
struct BirnetRing
{
  gpointer  data;
  BirnetRing  *next;
  BirnetRing  *prev;
};
BirnetRing* birnet_ring_prepend 	(BirnetRing             *head,
					 gpointer                data);
BirnetRing* birnet_ring_prepend_uniq    (BirnetRing             *head,
					 gpointer                data);
BirnetRing* birnet_ring_append          (BirnetRing             *head,
					 gpointer                data);
BirnetRing* birnet_ring_append_uniq     (BirnetRing             *head,
					 gpointer                data);
BirnetRing* birnet_ring_insert          (BirnetRing             *head,
					 gpointer                data,
					 gint                    position);
BirnetRing* birnet_ring_insert_before	(BirnetRing             *head,
					 BirnetRing             *sibling,
					 gpointer                data);
gint        birnet_ring_position        (const BirnetRing       *head,
					 const BirnetRing       *node);
gint        birnet_ring_index           (const BirnetRing       *head,
					 gconstpointer           data);
BirnetRing* birnet_ring_nth             (const BirnetRing       *head,
					 guint                   n);
gpointer    birnet_ring_nth_data        (const BirnetRing       *head,
					 guint                   n);
BirnetRing* birnet_ring_find            (const BirnetRing       *head,
					 gconstpointer           data);
BirnetRing* birnet_ring_remove_node     (BirnetRing             *head,
					 BirnetRing             *node);
BirnetRing* birnet_ring_remove          (BirnetRing             *head,
					 gpointer                data);
guint       birnet_ring_length          (const BirnetRing       *head);
gint        birnet_ring_cmp_length      (const BirnetRing       *head,
					 guint                   test_length);
BirnetRing* birnet_ring_copy            (const BirnetRing       *head);
BirnetRing* birnet_ring_copy_deep       (const BirnetRing       *head,
					 BirnetRingDataFunc      copy,
					 gpointer                func_data);
BirnetRing* birnet_ring_copy_rest       (const BirnetRing       *ring,
					 const BirnetRing       *head);
BirnetRing* birnet_ring_concat          (BirnetRing             *head1,
					 BirnetRing             *head2);
BirnetRing* birnet_ring_split           (BirnetRing             *head1,
					 BirnetRing             *head2);
BirnetRing* birnet_ring_reverse         (BirnetRing             *head);
gpointer    birnet_ring_pop_head        (BirnetRing             **head);
gpointer    birnet_ring_pop_tail        (BirnetRing             **head);
#define     birnet_ring_push_head        birnet_ring_prepend
#define     birnet_ring_push_tail        birnet_ring_append
void        birnet_ring_free            (BirnetRing             *head);
void        birnet_ring_free_deep       (BirnetRing             *head,
					 GDestroyNotify          data_destroy);

BirnetRing* birnet_ring_from_list             	(GList       	*list);
BirnetRing* birnet_ring_from_list_and_free	(GList          *list);
BirnetRing* birnet_ring_from_slist            	(GSList         *slist);
BirnetRing* birnet_ring_from_slist_and_free	(GSList         *slist);
#define     birnet_ring_tail(head)            	((head) ? (head)->prev : NULL)
#define     birnet_ring_walk(node,head_bound) 	((node)->next != (head_bound) ? (node)->next : NULL)
#define     birnet_ring_next(node,head_bound)  	 birnet_ring_walk (node, head_bound)


/* ring-modifying cmp-based operations */
BirnetRing* birnet_ring_insert_sorted	(BirnetRing        	*head,
                                         gpointer       	 insertion_data,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 cmp_data);
BirnetRing* birnet_ring_merge_sorted    (BirnetRing        	*head1,
                                         BirnetRing        	*head2,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
BirnetRing* birnet_ring_sort            (BirnetRing        	*head,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
BirnetRing* birnet_ring_uniq            (BirnetRing        	*sorted_ring1,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
BirnetRing* birnet_ring_uniq_free_deep  (BirnetRing        	*sorted_ring1,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data,
                                         GDestroyNotify 	 data_destroy);
BirnetRing* birnet_ring_reorder         (BirnetRing        	*unordered_ring,
                                         const BirnetRing  	*new_ring_order);
/* ring-copying cmp-based operations */
BirnetRing* birnet_ring_copy_deep_uniq	(const BirnetRing  	*sorted_ring1,
                                         BirnetRingDataFunc	 copy,
                                         gpointer       	 copy_data,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 cmp_data);
BirnetRing* birnet_ring_copy_uniq   	(const BirnetRing  	*sorted_ring1,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
BirnetRing* birnet_ring_union           (const BirnetRing  	*sorted_set1,
                                         const BirnetRing  	*sorted_set2,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
BirnetRing* birnet_ring_intersection    (const BirnetRing  	*sorted_set1,
                                         const BirnetRing  	*sorted_set2,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
BirnetRing* birnet_ring_difference    	(const BirnetRing  	*sorted_set1,
                                         const BirnetRing  	*sorted_set2,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
BirnetRing* birnet_ring_symmetric_difference (const BirnetRing  *sorted_set1,
					      const BirnetRing  *sorted_set2,
					      BirnetCompareFunc  cmp,
					      gpointer       	 data);
/* const-result cmp-based operations */
gboolean    birnet_ring_includes      	(const BirnetRing  	*sorted_super_set,
                                         const BirnetRing  	*sorted_sub_set,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
gboolean    birnet_ring_mismatch      	(BirnetRing            **sorted_ring1_p,
                                         BirnetRing            **sorted_ring2_p,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
gboolean    birnet_ring_equals      	(const BirnetRing  	*sorted_set1,
                                         const BirnetRing  	*sorted_set2,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
BirnetRing* birnet_ring_min_node    	(const BirnetRing  	*head,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
BirnetRing* birnet_ring_max_node    	(const BirnetRing  	*head,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
gpointer    birnet_ring_min            	(const BirnetRing  	*head,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);
gpointer    birnet_ring_max           	(const BirnetRing  	*head,
                                         BirnetCompareFunc 	 cmp,
                                         gpointer       	 data);

G_END_DECLS

#endif /* __BIRNET_RING_H__ */

/* vim:set ts=8 sts=2 sw=2: */
