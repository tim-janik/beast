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
#include "sfiprimitives.h"


/* --- SfiFBlock primitive type --- */
SfiFBlock*
sfi_fblock_new (void)
{
  SfiFBlock *fb = g_new (SfiFBlock, 1);

  fb->ref_count = 1;
  fb->n_floats = 0;
  fb->floats = NULL;
  return fb;
}

SfiFBlock*
sfi_fblock_ref (SfiFBlock *fblock)
{
  g_return_val_if_fail (fblock != NULL, NULL);
  g_return_val_if_fail (fblock->ref_count > 0, NULL);

  fblock->ref_count++;
  return fblock;
}

void
sfi_fblock_unref (SfiFBlock *fblock)
{
  g_return_if_fail (fblock != NULL);
  g_return_if_fail (fblock->ref_count > 0);

  fblock->ref_count--;
  if (fblock->ref_count == 0)
    {
      g_free (fblock->floats);
      g_free (fblock);
    }
}

SfiFBlock*
sfi_fblock_copy_deep (const SfiFBlock *fblock)
{
  SfiFBlock *fb;

  g_return_val_if_fail (fblock != NULL, NULL);
  g_return_val_if_fail (fblock->ref_count > 0, NULL);

  fb = sfi_fblock_new ();
  fb->n_floats = fblock->n_floats;
  fb->floats = g_memdup (fblock->floats, fblock->n_floats * sizeof (fblock->floats[0]));
  return fb;
}

void
sfi_fblock_append (SfiFBlock    *fblock,
		   guint         n_floats,
		   const gfloat *floats)
{
  g_return_if_fail (fblock != NULL);

  if (n_floats)
    {
      guint i;

      g_return_if_fail (floats != NULL);

      i = fblock->n_floats;
      fblock->n_floats += n_floats;
      fblock->floats = g_renew (gfloat, fblock->floats, fblock->n_floats);
      memcpy (fblock->floats + i, floats, n_floats * sizeof (fblock->floats[0]));
    }
}

void
sfi_fblock_append1 (SfiFBlock *fblock,
		    gfloat     float0)
{
  guint i;

  g_return_if_fail (fblock != NULL);

  i = fblock->n_floats++;
  fblock->floats = g_renew (gfloat, fblock->floats, fblock->n_floats);
  fblock->floats[i] = float0;
}

guint
sfi_fblock_length (const SfiFBlock *fblock)
{
  g_return_val_if_fail (fblock != NULL, 0);
  return fblock->n_floats;
}

gfloat*
sfi_fblock_get (const SfiFBlock *fblock)
{
  g_return_val_if_fail (fblock != NULL, NULL);
  return fblock->floats;
}


/* --- SfiSeq primitive type --- */
SfiSeq*
sfi_seq_new (void)
{
  SfiSeq *s;

  s = g_new (SfiSeq, 1);
  s->ref_count = 1;
  s->n_elements = 0;
  s->elements = NULL;
  return s;
}

SfiSeq*
sfi_seq_ref (SfiSeq *seq)
{
  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (seq->ref_count > 0, NULL);

  seq->ref_count++;
  return seq;
}

void
sfi_seq_unref (SfiSeq *seq)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (seq->ref_count > 0);

  seq->ref_count--;
  if (seq->ref_count == 0)
    {
      guint i;
      for (i = 0; i < seq->n_elements; i++)
	g_value_unset (seq->elements + i);
      g_free (seq->elements);
      g_free (seq);
    }
}

static inline gulong
upper_power2 (gulong number)
{
  return number ? 1 << g_bit_storage (number - 1) : 0;
}

static void
sfi_seq_append_copy (SfiSeq       *seq,
		     const GValue *value,
		     gboolean      deep_copy)
{
  guint i, l, n;

  g_return_if_fail (seq != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  l = upper_power2 (seq->n_elements);
  i = seq->n_elements++;
  n = upper_power2 (seq->n_elements);
  if (n > l)
    {
      seq->elements = g_realloc (seq->elements, n * sizeof (seq->elements[0]));
      memset (seq->elements + l, 0, (n - l) * sizeof (seq->elements[0]));
    }
  g_value_init (seq->elements + i, G_VALUE_TYPE (value));
  if (deep_copy)
    sfi_value_copy_deep (value, seq->elements + i);
  else
    g_value_copy (value, seq->elements + i);
}

SfiSeq*
sfi_seq_copy_deep (const SfiSeq *seq)
{
  SfiSeq *s;
  guint i;

  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (seq->ref_count > 0, NULL);

  s = sfi_seq_new ();
  for (i = 0; i < seq->n_elements; i++)
    sfi_seq_append_copy (s, seq->elements + i, TRUE);
  return s;
}

void
sfi_seq_append (SfiSeq       *seq,
		const GValue *value)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  sfi_seq_append_copy (seq, value, FALSE);
}

guint
sfi_seq_length (const SfiSeq *seq)
{
  return seq ? seq->n_elements : 0;
}

GValue*
sfi_seq_get (const SfiSeq *seq,
	     guint         index)
{
  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (index < seq->n_elements, NULL);

  return seq->elements + index;
}

gboolean
sfi_seq_check (SfiSeq *seq,
	       GType   element_type)
{
  guint i;
  
  g_return_val_if_fail (seq != NULL, FALSE);
  
  for (i = 0; i < seq->n_elements; i++)
    if (!G_VALUE_HOLDS (seq->elements + i, element_type))
      return FALSE;
  return TRUE;
}


/* --- SfiRec primitive type --- */
SfiRec*
sfi_rec_new (void)
{
  SfiRec *r = g_new (SfiRec, 1);
  r->n_fields = 0;
  r->ref_count = 1;
  r->fields = NULL;
  r->field_names = NULL;
  return r;
}

SfiRec*
sfi_rec_ref (SfiRec *rec)
{
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (rec->ref_count > 0, NULL);
  
  rec->ref_count++;
  
  return rec;
}

void
sfi_rec_unref (SfiRec *rec)
{
  g_return_if_fail (rec != NULL);
  g_return_if_fail (rec->ref_count > 0);
  
  rec->ref_count--;
  if (rec->ref_count == 0)
    {
      guint i;
      for (i = 0; i < rec->n_fields; i++)
	{
	  g_value_unset (rec->fields + i);
	  g_free (rec->field_names[i]);
	}
      g_free (rec->fields);
      g_free (rec->field_names);
      g_free (rec);
    }
}

static inline gchar*
dupcanon (const gchar *field_name)
{
  return g_strcanon (g_strdup (field_name),
		     G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS,
		     '-');
}

static void
sfi_rec_set_copy (SfiRec       *rec,
		  const gchar  *field_name,
		  const GValue *value,
		  gboolean      deep_copy)
{
  gchar *name;
  guint i;
  
  name = dupcanon (field_name);
  for (i = 0; i < rec->n_fields; i++)
    if (strcmp (name, rec->field_names[i]) == 0)
      break;
  if (i >= rec->n_fields)
    {
      i = rec->n_fields++;
      rec->fields = g_realloc (rec->fields, rec->n_fields * sizeof (rec->fields[0]));
      memset (rec->fields + i, 0, sizeof (rec->fields[0]));
      rec->field_names = g_realloc (rec->field_names, rec->n_fields * sizeof (rec->field_names[0]));
      rec->field_names[i] = name;
    }
  else
    {
      g_value_unset (rec->fields + i);
      g_free (name);
    }
  g_value_init (rec->fields + i, G_VALUE_TYPE (value));
  if (deep_copy)
    sfi_value_copy_deep (value, rec->fields + i);
  else
    g_value_copy (value, rec->fields + i);
}

void
sfi_rec_set (SfiRec       *rec,
	     const gchar  *field_name,
	     const GValue *value)
{
  g_return_if_fail (rec != NULL);
  g_return_if_fail (field_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  sfi_rec_set_copy (rec, field_name, value, FALSE);
}

guint
sfi_rec_n_fields (const SfiRec *rec)
{
  return rec ? rec->n_fields : 0;
}

GValue*
sfi_rec_field (const SfiRec *rec,
	       guint         index)
{
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (index < rec->n_fields, NULL);
  
  return rec->fields + index;
}

GValue*
sfi_rec_get (const SfiRec *rec,
	     const gchar  *field_name)
{
  gchar *name;
  guint i;
  
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (field_name != NULL, NULL);
  
  name = dupcanon (field_name);
  for (i = 0; i < rec->n_fields; i++)
    if (strcmp (name, rec->field_names[i]) == 0)
      break;
  g_free (name);
  if (i < rec->n_fields)
    return rec->fields + i;
  return NULL;
}

SfiRec*
sfi_rec_copy_deep (const SfiRec *rec)
{
  SfiRec *r;
  guint i;
  
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (rec->ref_count > 0, NULL);
  
  r = sfi_rec_new ();
  for (i = 0; i < rec->n_fields; i++)
    sfi_rec_set_copy (r, rec->field_names[i], &rec->fields[i], TRUE);
  return r;
}

gboolean
sfi_rec_check (SfiRec      *rec,
	       SfiRecFields rfields)
{
  guint i;

  g_return_val_if_fail (rec != NULL, FALSE);
  g_return_val_if_fail (rfields.n_fields > 0, FALSE);

  for (i = 0; i < rfields.n_fields; i++)
    {
      GValue *value = sfi_rec_get (rec, rfields.fields[i]->name);
      if (!value || !G_VALUE_HOLDS (value, G_PARAM_SPEC_VALUE_TYPE (rfields.fields[i])))
	return FALSE;
    }
  return TRUE;
}


/* --- ring (circular-list) --- */
#define sfi_new_struct(type, n)         ((type*) g_malloc (sizeof (type) * (n))) // FIXME
#define sfi_delete_structs(type, n, mem)      ({ /* FIXME */ \
  type *__typed_pointer = (mem); \
  g_free (__typed_pointer); \
})
#define sfi_delete_struct(type, mem)    (sfi_delete_structs (type, 1, (mem))) // FIXME
static void
sfi_free_node_list (gpointer mem,	// FIXME
		    gsize    node_size)
{
  struct { gpointer next, data; } *tmp, *node = mem;

  g_return_if_fail (node != NULL);
  g_return_if_fail (node_size >= 2 * sizeof (gpointer));

  /* FIXME: this can be optimized to an O(1) operation with T-style links in mem-caches */
  do
    {
      tmp = node->next;

      g_free (/*node_size,*/ node);
      node = tmp;
    }
  while (node);
}
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
  SfiRing *ring = sfi_new_struct (SfiRing, 1);
  
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
  SfiRing *walk;
  
  for (walk = head; walk; walk = sfi_ring_walk (head, walk))
    if (walk->data == data)
      return head;
  return sfi_ring_prepend_i (head, data);
}

SfiRing*
sfi_ring_append (SfiRing  *head,
		 gpointer data)
{
  SfiRing *ring;
  
  ring = sfi_ring_prepend_i (head, data);
  
  return head ? head : ring;
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
 * sfi_ring_split
 * @head1:   a non-empty ring
 * @head2:   a ring node different from @head1 contained in @head1
 * @returns: @head2 for convenience
 * Split a ring into two parts, starting the second ring with @head2.
 * @head2 must therefore be non-NULL and must be contained in the ring
 * formed by @head1.
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
      
      sfi_delete_struct (SfiRing, node);
      return NULL;
    }
  g_return_val_if_fail (node != node->next, head); /* node can't be a one item ring here */
  
  node->next->prev = node->prev;
  node->prev->next = node->next;
  if (head == node)
    head = node->next;
  sfi_delete_struct (SfiRing, node);
  
  return head;
}

static inline SfiRing*
sfi_ring_merge_sorted (SfiRing     *head1,
		       SfiRing     *head2,
		       GCompareFunc func)
{
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
	  gint cmp = func (head1->data, head2->data);
	  if (cmp <= 0)
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
sfi_ring_sort (SfiRing     *head,
	       GCompareFunc func)
{
  g_return_val_if_fail (func != NULL, head);

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
      return sfi_ring_merge_sorted (sfi_ring_sort (head, func),
				    sfi_ring_sort (ring, func),
				    func);
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
  
  for (walk = head; walk; walk = sfi_ring_walk (head, walk))
    if (walk->data == data)
      return sfi_ring_remove_node (head, walk);
  
  g_warning (G_STRLOC ": couldn't find data item (%p) to remove from ring (%p)", data, head);
  
  return head;
}

guint
sfi_ring_length (SfiRing *head)
{
  SfiRing *ring;
  guint i = 0;
  
  for (ring = head; ring; ring = sfi_ring_walk (head, ring))
    i++;

  return i;
}

SfiRing*
sfi_ring_find (SfiRing      *head,
	       gconstpointer data)
{
  SfiRing *ring;

  for (ring = head; ring; ring = sfi_ring_walk (head, ring))
    if (ring->data == (gpointer) data)
      return ring;

  return NULL;
}

SfiRing*
sfi_ring_nth (SfiRing *head,
	      guint    n)
{
  SfiRing *ring = head;

  while (n-- && ring)
    ring = sfi_ring_walk (head, ring);

  return ring;
}

gpointer
sfi_ring_nth_data (SfiRing *head,
		   guint    n)
{
  SfiRing *ring = head;

  while (n-- && ring)
    ring = sfi_ring_walk (head, ring);

  return ring ? ring->data : ring;
}

void
sfi_ring_free (SfiRing *head)
{
  if (head)
    {
      head->prev->next = NULL;
      sfi_free_node_list (head, sizeof (*head));
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
sfi_ring_insert_sorted (SfiRing	    *head,
			gpointer     data,
			GCompareFunc func)
{
  gint cmp;

  g_return_val_if_fail (func != NULL, head);

  if (!head)
    return sfi_ring_prepend (head, data);

  /* typedef gint (*GCompareFunc) (gconstpointer a,
   *                               gconstpointer b);
   */
  cmp = func (data, head->data);

  if (cmp >= 0)	/* insert after head */
    {
      SfiRing *tmp, *tail = head->prev;
      
      /* make appending an O(1) operation */
      if (head == tail || func (data, tail->data) >= 0)
	return sfi_ring_append (head, data);

      /* walk forward while data >= tmp (skipping equal nodes) */
      for (tmp = head->next; tmp != tail; tmp = tmp->next)
	if (func (data, tmp->data) < 0)
	  break;

      /* insert before sibling which is greater than data */
      sfi_ring_prepend (tmp, data);	/* keep current head */
      return head;
    }
  else /* cmp < 0 */
    return sfi_ring_prepend (head, data);
}
