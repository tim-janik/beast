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
#include "sfiring.h"
#include "sfimemory.h"
#include <stdlib.h>
#include <string.h>

#define HAVE_GSLICE     GLIB_CHECK_VERSION (2, 9, 7)

/* --- memory handling --- */
static inline SfiRing*
node_alloc (void)
{
#if HAVE_GSLICE
  return g_slice_new (SfiRing);
#else
  return (SfiRing*) g_list_alloc();
#endif
}

static inline void
node_free (SfiRing *node)
{
#if HAVE_GSLICE
  g_slice_free (SfiRing, node);
#else
  g_list_free_1 ((GList*) node);
#endif
}

static inline void
free_node_list (SfiRing *start)
{
  /* NULL terminated list of ->next pointers */
#if HAVE_GSLICE
  g_slice_free_chain (SfiRing, start, next);
#else
  g_list_free ((GList*) start);
#endif
}

/* --- basic comparisons --- */
gint
sfi_pointer_cmp (gconstpointer   value1,
                 gconstpointer   value2,
                 gpointer        dummy)
{
  const char *p1 = (const char*) value1;
  const char *p2 = (const char*) value2;
  return p1 < p2 ? -1 : p1 != p2;
}


/* --- ring (circular-list) --- */
static inline SfiRing*
sfi_ring_prepend_link_i (SfiRing *head,
                         SfiRing *ring)
{
  if (!head)
    {
      ring->prev = ring;
      ring->next = ring;
    }
  else
    {
      ring->prev = head->prev;
      ring->next = head;
      head->prev->next = ring;
      head->prev = ring;
    }
  return ring;
}

static inline SfiRing*
sfi_ring_prepend_i (SfiRing *head,
                    gpointer data)
{
  SfiRing *ring = node_alloc();
  
  ring->data = data;
  return sfi_ring_prepend_link_i (head, ring);
}

static inline SfiRing*
sfi_ring_append_link_i (SfiRing *head,
                        SfiRing *ring)
{
  sfi_ring_prepend_link_i (head, ring);
  return head ? head : ring;
}

SfiRing*
sfi_ring_prepend (SfiRing  *head,
                  gpointer data)
{
  return sfi_ring_prepend_i (head, data);
}

SfiRing*
sfi_ring_prepend_uniq (SfiRing  *head,
                       gpointer data)
{
  SfiRing *ring;
  for (ring = head; ring; ring = sfi_ring_walk (ring, head))
    if (ring->data == data)
      return head;
  return sfi_ring_prepend_i (head, data);
}

SfiRing*
sfi_ring_append (SfiRing *head,
                 gpointer data)
{
  SfiRing *ring = sfi_ring_prepend_i (head, data);
  return head ? head : ring;
}

SfiRing*
sfi_ring_append_uniq (SfiRing *head,
                      gpointer data)
{
  SfiRing *ring;
  for (ring = head; ring; ring = sfi_ring_walk (ring, head))
    if (ring->data == data)
      return head;
  ring = sfi_ring_prepend_i (head, data);
  return head ? head : ring;
}

SfiRing*
sfi_ring_insert_before (SfiRing        *head,
                        SfiRing        *sibling,
                        gpointer        data)
{
  if (!sibling)
    return sfi_ring_append (head, data);
  SfiRing *node = sfi_ring_prepend (sibling, data);
  return sibling == head ? node : head;
}

SfiRing*
sfi_ring_insert (SfiRing *head,
                 gpointer data,
                 gint     position)
{
  if (position < 0)
    return sfi_ring_append (head, data);
  else if (position == 0)
    return sfi_ring_prepend (head, data);
  SfiRing *node = sfi_ring_nth (head, position);
  if (node)
    return sfi_ring_insert_before (head, node, data);
  else
    return sfi_ring_append (head, data);
}

gint
sfi_ring_position (const SfiRing  *head,
                   const SfiRing  *node)
{
  guint i = 0;
  const SfiRing *ring;
  for (ring = head; ring; ring = sfi_ring_walk (ring, head), i++)
    if (ring == node)
      return i;
  return -1;
}

gint
sfi_ring_index (const SfiRing *head,
                gconstpointer  data)
{
  guint i = 0;
  const SfiRing *ring;
  for (ring = head; ring; ring = sfi_ring_walk (ring, head), i++)
    if (ring->data == data)
      return i;
  return -1;
}

SfiRing*
sfi_ring_copy (const SfiRing *head)
{
  const SfiRing *walk;
  SfiRing *dest = NULL;
  for (walk = head; walk; walk = sfi_ring_walk (walk, head))
    dest = sfi_ring_append (dest, walk->data);
  return dest;
}

SfiRing*
sfi_ring_copy_deep (const SfiRing  *head,
                    SfiRingDataFunc copy,
                    gpointer        func_data)
{
  const SfiRing *walk;
  SfiRing *dest = NULL;
  for (walk = head; walk; walk = sfi_ring_walk (walk, head))
    dest = sfi_ring_append (dest, copy (walk->data, func_data));
  return dest;
}

SfiRing*
sfi_ring_copy_rest (const SfiRing *ring,
                    const SfiRing *head)
{
  const SfiRing *walk;
  SfiRing *dest = NULL;
  for (walk = ring; walk; walk = sfi_ring_walk (walk, head))
    dest = sfi_ring_append (dest, walk->data);
  return dest;
}

SfiRing*
sfi_ring_concat (SfiRing *head1,
                 SfiRing *head2)
{
  SfiRing *tail1, *tail2;
  
  if (!head1)
    return head2;
  if (!head2)
    return head1;
  tail1 = head1->prev;
  tail2 = head2->prev;
  head1->prev = tail2;
  tail2->next = head1;
  head2->prev = tail1;
  tail1->next = head2;
  
  return head1;
}

/**
 * @param head1	a non-empty ring
 * @param head2	a ring node different from @a head1 contained in @a head1
 * @param returns	@a head2 for convenience
 *
 * Split a ring into two parts, starting the second ring with @a head2.
 * @a head2 must therefore be non-NULL and must be contained in the ring
 * formed by @a head1.
 */
SfiRing*
sfi_ring_split (SfiRing *head1,
                SfiRing *head2)
{
  SfiRing *tail1, *tail2;
  
  g_return_val_if_fail (head1 != NULL, NULL);
  g_return_val_if_fail (head2 != NULL, NULL);
  g_return_val_if_fail (head1 != head2, NULL);
  
  tail1 = head2->prev;
  tail2 = head1->prev;
  head2->prev = tail2;
  tail2->next = head2;
  head1->prev = tail1;
  tail1->next = head1;
  return head2;
}

static inline SfiRing*
sfi_ring_unlink_node_dangling (SfiRing *head,
                               SfiRing *node)
{
  g_assert (head && node);
  /* special case one item ring */
  if (head->prev == head)
    return NULL;
  /* unlink */
  node->next->prev = node->prev;
  node->prev->next = node->next;
  if (head == node)
    head = node->next;
  /* node->prev and node->next are dangling now */
  return head;
}

SfiRing*
sfi_ring_remove_node (SfiRing *head,
                      SfiRing *node)
{
  if (!head)
    g_return_val_if_fail (head == NULL && node == NULL, NULL);
  if (!head || !node)
    return NULL;
  
  /* special case one item ring */
  if (head->prev == head)
    {
      g_return_val_if_fail (node == head, head);
      
      node_free (node);
      return NULL;
    }
  g_return_val_if_fail (node != node->next, head); /* node can't be a one item ring here */
  
  node->next->prev = node->prev;
  node->prev->next = node->next;
  if (head == node)
    head = node->next;
  node_free (node);
  
  return head;
}

SfiRing*
sfi_ring_reverse (SfiRing *head)
{
  if (head)
    {
      SfiRing *ring = head = head->prev;
      do
	{
	  SfiRing *tmp = ring;
	  ring = tmp->next;
	  tmp->next = tmp->prev;
	  tmp->prev = ring;
	}
      while (ring != head);
    }
  return head;
}

SfiRing*
sfi_ring_remove (SfiRing *head,
                 gpointer data)
{
  SfiRing *walk;
  
  if (!head)
    return NULL;
  
  /* make tail data removal an O(1) operation */
  if (head->prev->data == data)
    return sfi_ring_remove_node (head, head->prev);
  
  for (walk = head; walk; walk = sfi_ring_walk (walk, head))
    if (walk->data == data)
      return sfi_ring_remove_node (head, walk);
  
  /* g_warning (G_STRLOC ": couldn't find data item (%p) to remove from ring (%p)", data, head); */
  
  return head;
}

guint
sfi_ring_length (const SfiRing *head)
{
  const SfiRing *ring;
  guint i = 0;
  
  for (ring = head; ring; ring = sfi_ring_walk (ring, head))
    i++;
  
  return i;
}

gint    /* essentially compute length(ring) - test_length, clamped to -1..+1 */
sfi_ring_cmp_length (const SfiRing *head,
                     guint          test_length)
{
  const SfiRing *ring = head;
  
  while (test_length && ring)
    {
      test_length--;
      ring = sfi_ring_walk (ring, head);
    }
  
  return test_length > 0 ? -1 : ring != NULL;
}

SfiRing*
sfi_ring_find (const SfiRing *head,
               gconstpointer  data)
{
  const SfiRing *ring;
  for (ring = head; ring; ring = sfi_ring_walk (ring, head))
    if (ring->data == (gpointer) data)
      return (SfiRing*) ring;
  return NULL;
}

SfiRing*
sfi_ring_nth (const SfiRing *head,
              guint          n)
{
  const SfiRing *ring = head;
  while (n-- && ring)
    ring = sfi_ring_walk (ring, head);
  return (SfiRing*) ring;
}

gpointer
sfi_ring_nth_data (const SfiRing *head,
                   guint          n)
{
  const SfiRing *ring = head;
  
  while (n-- && ring)
    ring = sfi_ring_walk (ring, head);
  
  return ring ? ring->data : NULL;
}

void
sfi_ring_free_deep (SfiRing        *head,
                    GDestroyNotify  data_destroy)
{
  while (head)
    {
      gpointer data = sfi_ring_pop_head (&head);
      data_destroy (data);
      data = sfi_ring_pop_head (&head);
    }
}

void
sfi_ring_free (SfiRing *head)
{
  if (head)
    {
      head->prev->next = NULL;
      free_node_list (head);
    }
}

gpointer
sfi_ring_pop_head (SfiRing **head_p)
{
  gpointer data;
  
  g_return_val_if_fail (head_p != NULL, NULL);
  
  if (!*head_p)
    return NULL;
  data = (*head_p)->data;
  *head_p = sfi_ring_remove_node (*head_p, *head_p);
  
  return data;
}

gpointer
sfi_ring_pop_tail (SfiRing **head_p)
{
  gpointer data;
  
  g_return_val_if_fail (head_p != NULL, NULL);
  
  if (!*head_p)
    return NULL;
  data = (*head_p)->prev->data;
  *head_p = sfi_ring_remove_node (*head_p, (*head_p)->prev);
  
  return data;
}

SfiRing*
sfi_ring_from_list (GList *list)
{
  SfiRing *ring = NULL;
  for (; list; list = list->next)
    ring = sfi_ring_append (ring, list->data);
  return ring;
}

SfiRing*
sfi_ring_from_list_and_free (GList *list)
{
  SfiRing *ring = NULL;
  GList *free_list = list;
  for (; list; list = list->next)
    ring = sfi_ring_append (ring, list->data);
  g_list_free (free_list);
  return ring;
}

SfiRing*
sfi_ring_from_slist (GSList *slist)
{
  SfiRing *ring = NULL;
  for (; slist; slist = slist->next)
    ring = sfi_ring_append (ring, slist->data);
  return ring;
}

SfiRing*
sfi_ring_from_slist_and_free (GSList *slist)
{
  SfiRing *ring = NULL;
  GSList *free_slist = slist;
  for (; slist; slist = slist->next)
    ring = sfi_ring_append (ring, slist->data);
  g_slist_free (free_slist);
  return ring;
}

SfiRing*
sfi_ring_insert_sorted (SfiRing	      *head,
                        gpointer       insertion_data,
                        SfiCompareFunc cmp,
                        gpointer       cmp_data)
{
  g_return_val_if_fail (cmp != NULL, head);
  if (!head)
    return sfi_ring_prepend (head, insertion_data);
  
  /* implement stable sorting by inserting insertion_data *after* equal nodes */
  
  if (cmp (insertion_data, head->data, cmp_data) >= 0)  /* insert after head */
    {
      SfiRing *tmp, *tail = head->prev;
      
      /* make appending an O(1) operation */
      if (head == tail || cmp (insertion_data, tail->data, cmp_data) >= 0)
	return sfi_ring_append (head, insertion_data);
      
      /* walk forward while data >= tmp (skipping equal nodes) */
      for (tmp = head->next; tmp != tail; tmp = tmp->next)
	if (cmp (insertion_data, tmp->data, cmp_data) < 0)
	  break;
      
      /* insert before sibling which is greater than insertion_data */
      sfi_ring_prepend (tmp, insertion_data); /* keep current head */
      return head;
    }
  else /* cmp < 0 */
    return sfi_ring_prepend (head, insertion_data);
}

SfiRing*
sfi_ring_merge_sorted (SfiRing        *head1,
                       SfiRing        *head2,
                       SfiCompareFunc  cmp,
                       gpointer        data)
{
  /* implement stable sorting by inserting head2 members *after* equal nodes from head1 */
  
  if (head1 && head2)
    {
      SfiRing *tail1 = head1->prev;
      SfiRing *tail2 = head2->prev;
      SfiRing *tmp, *ring = NULL;
      /* NULL terminate rings */
      tail1->next = NULL;
      tail2->next = NULL;
      while (head1 && head2)
	{
	  gint c = cmp (head1->data, head2->data, data);
	  if (c <= 0)
	    {
	      tmp = head1;
	      head1 = head1->next;
	    }
	  else
	    {
	      tmp = head2;
	      head2 = head2->next;
	    }
	  ring = sfi_ring_append_link_i (ring, tmp);
	}
      /* reform valid rings, concat sorted rest */
      if (head1)
	{
	  tail1->next = head1;
	  head1->prev = tail1;
	  return sfi_ring_concat (ring, head1);
	}
      if (head2)
	{
	  tail2->next = head2;
	  head2->prev = tail2;
	  return sfi_ring_concat (ring, head2);
	}
      return ring;
    }
  else
    return sfi_ring_concat (head1, head2);
}

SfiRing*
sfi_ring_sort (SfiRing        *head,
               SfiCompareFunc  cmp,
               gpointer        data)
{
  g_return_val_if_fail (cmp != NULL, head);
  
  /* stable sorting guaranteed by sfi_ring_merge_sorted() */
  
  if (head && head->next != head)
    {
      SfiRing *ring, *tmp, *tail = head->prev;
      /* find middle node to get log2 recursion depth */
      ring = tmp = head->next;
      while (tmp != tail && tmp->next != tail)
	{
	  ring = ring->next;
	  tmp = tmp->next->next;
	}
      sfi_ring_split (head, ring);
      return sfi_ring_merge_sorted (sfi_ring_sort (head, cmp, data),
                                    sfi_ring_sort (ring, cmp, data),
                                    cmp, data);
    }
  return head;
}

SfiRing* /* eliminates duplicate nodes */
sfi_ring_uniq (SfiRing        *sorted_ring1,
               SfiCompareFunc  cmp,
               gpointer        data)
{
  /* always preserves the first of a sublist of consequtive equal elements */
  SfiRing *r1 = sorted_ring1;
  SfiRing *r2 = NULL;
  if (r1)
    {
      SfiRing *last = r1;
      r1 = sfi_ring_unlink_node_dangling (r1, last);
      r2 = last->next = last->prev = last; /* form new ring */
      while (r1)
        {
          SfiRing *node = r1;
          r1 = sfi_ring_unlink_node_dangling (r1, node);
          if (cmp (last->data, node->data, data))
            {
              last = node;
              r2 = sfi_ring_append_link_i (r2, last);
            }
          else
            node_free (node);
        }
    }
  return r2;
}

SfiRing* /* eliminates duplicate nodes */
sfi_ring_uniq_free_deep (SfiRing        *sorted_ring1,
                         SfiCompareFunc  cmp,
                         gpointer        data,
                         GDestroyNotify  data_destroy)
{
  /* always preserves the first of a sublist of consequtive equal elements */
  if (!data_destroy)
    return sfi_ring_uniq (sorted_ring1, cmp, data);
  SfiRing *r1 = sorted_ring1;
  SfiRing *r2 = NULL;
  if (r1)
    {
      SfiRing *last = r1;
      r1 = sfi_ring_unlink_node_dangling (r1, last);
      r2 = last->next = last->prev = last; /* form new ring */
      while (r1)
        {
          SfiRing *node = r1;
          r1 = sfi_ring_unlink_node_dangling (r1, node);
          if (cmp (last->data, node->data, data))
            {
              last = node;
              r2 = sfi_ring_append_link_i (r2, last);
            }
          else
            {
              data_destroy (node->data);
              node_free (node);
            }
        }
    }
  return r2;
}

SfiRing* /* eliminates duplicate nodes */
sfi_ring_copy_deep_uniq (const SfiRing  *sorted_ring1,
                         SfiRingDataFunc copy,
                         gpointer        copy_data,
                         SfiCompareFunc  cmp,
                         gpointer        cmp_data)
{
  /* always preserves the first of a sublist of consequtive equal elements */
  if (!copy)
    return sfi_ring_copy_uniq (sorted_ring1, cmp, cmp_data);
  const SfiRing *r1 = sorted_ring1;
  SfiRing *r2 = NULL;
  if (r1)
    {
      gpointer last_data = r1->data;
      r2 = sfi_ring_append (r2, copy (last_data, copy_data));
      for (r1 = sfi_ring_walk (r1, sorted_ring1); r1; r1 = sfi_ring_walk (r1, sorted_ring1))
        if (cmp (last_data, r1->data, cmp_data))
          {
            last_data = r1->data;
            r2 = sfi_ring_append (r2, copy (last_data, copy_data));
          }
    }
  return r2;
}

SfiRing* /* eliminates duplicate nodes */
sfi_ring_copy_uniq (const SfiRing  *sorted_ring1,
                    SfiCompareFunc  cmp,
                    gpointer        data)
{
  /* always preserves the first of a sublist of consequtive equal elements */
  const SfiRing *r1 = sorted_ring1;
  SfiRing *r2 = NULL;
  if (r1)
    {
      gpointer last_data = r1->data;
      r2 = sfi_ring_append (r2, last_data);
      for (r1 = sfi_ring_walk (r1, sorted_ring1); r1; r1 = sfi_ring_walk (r1, sorted_ring1))
        if (cmp (last_data, r1->data, data))
          {
            last_data = r1->data;
            r2 = sfi_ring_append (r2, last_data);
          }
    }
  return r2;
}

/**
 * @param sorted_set1   Sorted ring 1
 * @param sorted_set2   Sorted ring 2
 * @param cmp           Compare function for node data
 * @param data          Data argument passed into the cmp() function
 * @return              Newly created (or empty) ring
 *
 * Return a newly created ring that contains all data items from @a sorted_set1
 * and @a sorted_set2, omitting duplicates.
 * Items are considered equal according to the cmp() function.
 * For two equal items contained in both sets, the data pointer from
 * @a sorted_set1 will be added to the resulting set.
 * In mathematical terms, the returned ring is the union (sorted_set1, sorted_set2).
 * The complexity is O(MAX (length (sorted_set1), length (sorted_set2))).
 */
SfiRing* /* merges rings without dups */
sfi_ring_union (const SfiRing  *sorted_set1,
                const SfiRing  *sorted_set2,
                SfiCompareFunc  cmp,
                gpointer           data)
{
  /* for two equal elements from both sets, the element from sorted_set1 is picked, the one from sorted_set2 ignored */
  const SfiRing *r1 = sorted_set1, *r2 = sorted_set2;
  SfiRing *d = NULL;
  while (r1 && r2)
    {
      gint c = cmp (r1->data, r2->data, data);
      if (c < 0)
        {
          d = sfi_ring_append (d, r1->data);
          r1 = sfi_ring_walk (r1, sorted_set1);
        }
      else if (c > 0)
        {
          d = sfi_ring_append (d, r2->data);
          r2 = sfi_ring_walk (r2, sorted_set2);
        }
      else
        {
          d = sfi_ring_append (d, r1->data);
          r1 = sfi_ring_walk (r1, sorted_set1);
          r2 = sfi_ring_walk (r2, sorted_set2);
        }
    }
  return sfi_ring_concat (d, sfi_ring_copy_rest (r1 ? r1 : r2, r1 ? sorted_set1 : sorted_set2));
}

/**
 * @param sorted_set1   Sorted ring 1
 * @param sorted_set2   Sorted ring 2
 * @param cmp           Compare function for node data
 * @param data          Data argument passed into the cmp() function
 * @return              Newly created (or empty) ring
 *
 * Return a newly created ring that contains all data items which are contained
 * in both sets, @a sorted_set1 and @a sorted_set2.
 * Items are considered equal according to the cmp() function.
 * For two equal items contained in both sets, the data pointer from
 * @a sorted_set1 will be added to the resulting set.
 * In mathematical terms, the returned ring is the intersection (sorted_set1, sorted_set2).
 * The complexity is O(MAX (length (sorted_set1), length (sorted_set2))).
 */
SfiRing* /* returns nodes contained in both rings */
sfi_ring_intersection (const SfiRing  *sorted_set1,
                       const SfiRing  *sorted_set2,
                       SfiCompareFunc  cmp,
                       gpointer        data)
{
  /* for two equal elements from both sets, only elements from sorted_set1 are picked */
  const SfiRing *r1 = sorted_set1, *r2 = sorted_set2;
  SfiRing *d = NULL;
  while (r1 && r2)
    {
      gint c = cmp (r1->data, r2->data, data);
      if (c < 0)
        r1 = sfi_ring_walk (r1, sorted_set1);
      else if (c > 0)
        r2 = sfi_ring_walk (r2, sorted_set2);
      else
        {
          d = sfi_ring_append (d, r1->data);
          r1 = sfi_ring_walk (r1, sorted_set1);
          r2 = sfi_ring_walk (r2, sorted_set2);
        }
    }
  return d;
}

/**
 * @param sorted_set1   Sorted ring 1
 * @param sorted_set2   Sorted ring 2
 * @param cmp           Compare function for node data
 * @param data          Data argument passed into the cmp() function
 * @return              Newly created (or empty) ring
 *
 * Collect and return all data items from @a sorted_set1 which are not contained in
 * @a sorted_set2, according to the cmp() function.
 * In mathematical terms, the returned ring is the difference (sorted_set1, sorted_set2).
 * The complexity is O(MAX (length (sorted_set1), length (sorted_set2))).
 */
SfiRing*
sfi_ring_difference (const SfiRing  *sorted_set1,
                     const SfiRing  *sorted_set2,
                     SfiCompareFunc  cmp,
                     gpointer        data)
{
  const SfiRing *r1 = sorted_set1, *r2 = sorted_set2;
  SfiRing *d = NULL;
  while (r1 && r2)
    {
      gint c = cmp (r1->data, r2->data, data);
      if (c < 0)
        {
          d = sfi_ring_append (d, r1->data);
          r1 = sfi_ring_walk (r1, sorted_set1);
        }
      else if (c > 0)
        r2 = sfi_ring_walk (r2, sorted_set2);
      else
        {
          r1 = sfi_ring_walk (r1, sorted_set1);
          r2 = sfi_ring_walk (r2, sorted_set2);
        }
    }
  return sfi_ring_concat (d, sfi_ring_copy_rest (r1, sorted_set1));
}

/**
 * @param sorted_set1   Sorted ring 1
 * @param sorted_set2   Sorted ring 2
 * @param cmp           Compare function for node data
 * @param data          Data argument passed into the cmp() function
 * @return              Newly created (or empty) ring
 *
 * Collect and return all data items from @a sorted_set1 which are not contained in
 * @a sorted_set2, and all data items from @a sorted_set2 which are not contained in
 * @a sorted_set1, according to the cmp() function.
 * In mathematical terms, the returned ring is the union (difference (sorted_set1, sorted_set2)
 * + difference (sorted_set2, sorted_set1)).
 * The complexity is O(MAX (length (sorted_set1), length (sorted_set2))).
 */
SfiRing*
sfi_ring_symmetric_difference (const SfiRing  *sorted_set1,
                               const SfiRing  *sorted_set2,
                               SfiCompareFunc  cmp,
                               gpointer        data)
{
  const SfiRing *r1 = sorted_set1, *r2 = sorted_set2;
  SfiRing *d = NULL;
  while (r1 && r2)
    {
      gint c = cmp (r1->data, r2->data, data);
      if (c < 0)
        {
          d = sfi_ring_append (d, r1->data);
          r1 = sfi_ring_walk (r1, sorted_set1);
        }
      else if (c > 0)
        {
          d = sfi_ring_append (d, r2->data);
          r2 = sfi_ring_walk (r2, sorted_set2);
        }
      else
        {
          r1 = sfi_ring_walk (r1, sorted_set1);
          r2 = sfi_ring_walk (r2, sorted_set2);
        }
    }
  return sfi_ring_concat (d, sfi_ring_copy_rest (r1 ? r1 : r2, r1 ? sorted_set1 : sorted_set2));
}

static inline int
pointerloccmp (const void *pp1,
               const void *pp2)
{
  void* const * p1 = (void**) pp1;
  void* const * p2 = (void**) pp2;
  return *p1 < *p2 ? -1 : *p1 != *p2;
}

static inline gboolean
ring_reorder_lookup (guint          n_items,
                     gpointer      *items,
                     gpointer       key,
                     guint         *indexp)
{
  guint offset = 0, n = n_items;
  while (offset < n)
    {
      guint i = (offset + n) >> 1;
      gint cmp = key < items[i] ? -1 : key != items[i];
      if (cmp < 0)
        n = i;
      else if (cmp > 0)
        offset = i + 1;
      else /* match */
        {
          *indexp = i;
          return TRUE;
        }
    }
  return FALSE;
}

/**
 * @param unordered_ring        Unsorted ring
 * @param new_ring_order        Ring with arbitrary order
 * @return                      Reordered version of @a unordered_ring
 *
 * Reorders the data pointers of @a unordered_ring according to the order
 * as given by @a new_ring_order.
 * The complexity involves roughly 3 * length(unordered_ring) + qsort(unordered_ring) + length(new_ring_order) * bsearch(unordered_ring)),
 * i.e. it is at worst O(length(unordered_ring) * log (length(unordered_ring)) * length(new_ring_order)).
 */
SfiRing* /* reproduce all elements from unordered_ring in the order new_ring_order */
sfi_ring_reorder (SfiRing        *unordered_ring,
                  const SfiRing  *new_ring_order)
{
  if (!unordered_ring || !new_ring_order)
    return unordered_ring;
  const SfiRing *ring;

  /* construct a sorted array for faster lookups; O(length(unordered_ring)) */
  gpointer *items = NULL;
  guint i, n_items = 0, n_alloced = 0;
  for (ring = unordered_ring; ring; ring = sfi_ring_walk (ring, unordered_ring))
    {
      i = n_items++;
      if (n_items > n_alloced)
        {
          n_alloced = sfi_alloc_upper_power2 (MAX (n_items, 32));
          items = g_renew (gpointer, items, n_alloced);
        }
      items[i] = ring->data;
    }
  sfi_ring_free (unordered_ring);
  unordered_ring = NULL;
  qsort (items, n_items, sizeof (items[0]), pointerloccmp);

  /* collapse duplicates; O(length(unordered_ring)) */
  guint j = 0, *counts = g_new0 (guint, n_items);
  for (i = 0; i < n_items; i++)
    if (items[j] != items[i])
      {
        if (++j != i)
          items[j] = items[i];
        counts[j] = 1;
      }
    else /* catch dups */
      counts[j]++;
  n_items = j + 1;      /* shrink to number of different items */
  
  /* pick unordered_ring members in the order given by new_ring_order;
   * O(length(new_ring_order) * O(bsearch(unordered_ring)))
   */
  for (ring = new_ring_order; ring; ring = sfi_ring_walk (ring, new_ring_order))
    if (ring_reorder_lookup (n_items, items, ring->data, &i) && counts[i])
      {
        counts[i]--;
        unordered_ring = sfi_ring_append (unordered_ring, ring->data);
      }
  
  /* append left-over members from sorted_ring; O(length(unordered_ring)) */
  for (i = 0; i < n_items; i++)
    while (counts[i]--)
      unordered_ring = sfi_ring_append (unordered_ring, items[i]);
  
  g_free (items);
  g_free (counts);
  return unordered_ring;
}

gboolean
sfi_ring_includes (const SfiRing  *sorted_super_set,
                   const SfiRing  *sorted_sub_set,
                   SfiCompareFunc  cmp,
                   gpointer        data)
{
  const SfiRing *r1 = sorted_super_set, *r2 = sorted_sub_set;
  while (r1 && r2)
    {
      gint c = cmp (r1->data, r2->data, data);
      if (c > 0)
        return FALSE;
      else if (c == 0)
        r2 = sfi_ring_walk (r2, sorted_sub_set);
      // FIXME: misses backtracking to find "abc" in "aabc"
      r1 = sfi_ring_walk (r1, sorted_super_set);
    }
  return !r2;
}

/**
 * @param sorted_ring1  Sorted ring 1
 * @param sorted_ring2  Sorted ring 2
 * @param cmp           Compare function for node data
 * @param data          Data argument passed into the cmp() function
 * @return              TRUE for equal rings, FALSE otherwise
 *
 * Compare two rings according to the cmp() function. Return FALSE as
 * soon as a mismatch is found, returns TRUE for rings which are equal according to cmp().
 * The complexity is at most O(MIN (length (sorted_ring1), length (sorted_ring2))).
 */
gboolean
sfi_ring_equals (const SfiRing  *sorted_ring1,
                 const SfiRing  *sorted_ring2,
                 SfiCompareFunc  cmp,
                 gpointer        data)
{
  const SfiRing *r1 = sorted_ring1, *r2 = sorted_ring2;
  while (r1 && r2)
    {
      if (cmp (r1->data, r2->data, data))
        return FALSE;
      r1 = sfi_ring_walk (r1, sorted_ring1);
      r2 = sfi_ring_walk (r2, sorted_ring2);
    }
  return r1 == r2; /* both need to be NULL */
}

/**
 * @param sorted_ring1_p        Pointer to sorted ring 1
 * @param sorted_ring2_p        Pointer to sorted ring 2
 * @param cmp                   Compare function for node data
 * @param data                  Data argument passed into the cmp() function
 * @return                      TRUE for mismatching rings, FALSE otherwise
 *
 * Compare two rings according to the cmp() function. For mismatching rings,
 * @a sorted_ring1_p and @a sorted_ring2_p are set to point to the mismatching nodes.
 * The complexity is at most O(MIN (length (sorted_ring1), length (sorted_ring2))).
 */
gboolean
sfi_ring_mismatch (SfiRing       **sorted_ring1_p,
                   SfiRing       **sorted_ring2_p,
                   SfiCompareFunc  cmp,
                   gpointer        data)
{
  SfiRing *head1 = *sorted_ring1_p, *head2 = *sorted_ring2_p;
  SfiRing *r1 = head1, *r2 = head2;
  while (r1 && r2)
    {
      if (cmp (r1->data, r2->data, data))
        goto mismatch;
      r1 = sfi_ring_walk (r1, head1);
      r2 = sfi_ring_walk (r2, head2);
    }
  if (r1 == r2) /* both are NULL */
    return FALSE;
 mismatch:
  *sorted_ring1_p = r1;
  *sorted_ring2_p = r2;
  return TRUE;
}

/**
 * @param head  Head node of a ring or NULL for an empty ring
 * @param cmp   Compare function for node data
 * @param data  Data argument passed into the cmp() function
 * @return      Node with minimum data argument or NULL for empty rings
 *
 * Find and return the node holding the minimum data pointer of a ring, measured by evaluating cmp().
 * The complexity is O(length (head)).
 */
SfiRing*
sfi_ring_min_node (const SfiRing  *head,
                   SfiCompareFunc  cmp,
                   gpointer        data)
{
  const SfiRing *ring = head, *last = NULL;
  if (ring)
    {
      last = ring;
      for (ring = sfi_ring_walk (ring, head); ring; ring = sfi_ring_walk (ring, head))
        if (cmp (last->data, ring->data, data) > 0)
          last = ring;
    }
  return (SfiRing*) last;
}

/**
 * @param head  Head node of a ring or NULL for an empty ring
 * @param cmp   Compare function for node data
 * @param data  Data argument passed into the cmp() function
 * @return      Node with maximum data argument or NULL for empty rings
 *
 * Find and return the node holding the maximum data pointer of a ring, measured by evaluating cmp().
 * The complexity is O(length (head)).
 */
SfiRing*
sfi_ring_max_node (const SfiRing  *head,
                   SfiCompareFunc  cmp,
                   gpointer        data)
{
  const SfiRing *ring = head, *last = NULL;
  if (ring)
    {
      last = ring;
      for (ring = sfi_ring_walk (ring, head); ring; ring = sfi_ring_walk (ring, head))
        if (cmp (last->data, ring->data, data) < 0)
          last = ring;
    }
  return (SfiRing*) last;
}

/**
 * @param head  Head node of a ring or NULL for an empty ring
 * @param cmp   Compare function for node data
 * @param data  Data argument passed into the cmp() function
 * @return      Minimum data argument or NULL for empty rings
 *
 * Find and return the minimum data pointer of a ring, measured by evaluating cmp().
 * The complexity is O(length (head)).
 */
gpointer
sfi_ring_min (const SfiRing  *head,
              SfiCompareFunc  cmp,
              gpointer        data)
{
  SfiRing *ring = sfi_ring_min_node (head, cmp, data);
  return ring ? ring->data : NULL;
}

/**
 * @param head  Head node of a ring or NULL for an empty ring
 * @param cmp   Compare function for node data
 * @param data  Data argument passed into the cmp() function
 * @return      Maximum data argument or NULL for empty rings
 *
 * Find and return the maximum data pointer of a ring, measured by evaluating cmp().
 * The complexity is O(length (head)).
 */
gpointer
sfi_ring_max (const SfiRing  *head,
              SfiCompareFunc  cmp,
              gpointer        data)
{
  SfiRing *ring = sfi_ring_max_node (head, cmp, data);
  return ring ? ring->data : NULL;
}
