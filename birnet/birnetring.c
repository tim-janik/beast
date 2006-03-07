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
#include <stdlib.h>
#include <string.h>
#include "birnetprimitives.h"
#include "birnetmemory.h"
#include "birnetparams.h"


/* --- BirnetBBlock primitive type --- */
BirnetBBlock*
birnet_bblock_new (void)
{
  BirnetBBlock *bblock = birnet_new_struct (BirnetBBlock, 1);

  bblock->ref_count = 1;
  bblock->n_bytes = 0;
  bblock->bytes = NULL;
  return bblock;
}

BirnetBBlock*
birnet_bblock_new_sized (guint size)
{
  BirnetBBlock *bblock = birnet_bblock_new ();
  birnet_bblock_resize (bblock, size);
  return bblock;
}

BirnetBBlock*
birnet_bblock_ref (BirnetBBlock *bblock)
{
  g_return_val_if_fail (bblock != NULL, NULL);
  g_return_val_if_fail (bblock->ref_count > 0, NULL);

  bblock->ref_count++;
  return bblock;
}

void
birnet_bblock_unref (BirnetBBlock *bblock)
{
  g_return_if_fail (bblock != NULL);
  g_return_if_fail (bblock->ref_count > 0);

  bblock->ref_count--;
  if (bblock->ref_count == 0)
    {
      g_free (bblock->bytes);
      birnet_delete_struct (BirnetBBlock, bblock);
    }
}

void
birnet_bblock_resize (BirnetBBlock *bblock,
		   guint      size)
{
  guint i;

  g_return_if_fail (bblock != NULL);

  i = bblock->n_bytes;
  bblock->n_bytes = size;
  bblock->bytes = g_renew (guint8, bblock->bytes, bblock->n_bytes);
  if (size > i)
    memset (bblock->bytes + i, 0, sizeof (bblock->bytes[0]) * (size - i));
}

BirnetBBlock*
birnet_bblock_copy_deep (const BirnetBBlock *bblock)
{
  BirnetBBlock *fb;

  g_return_val_if_fail (bblock != NULL, NULL);
  g_return_val_if_fail (bblock->ref_count > 0, NULL);

  fb = birnet_bblock_new ();
  fb->n_bytes = bblock->n_bytes;
  fb->bytes = g_memdup (bblock->bytes, bblock->n_bytes * sizeof (bblock->bytes[0]));
  return fb;
}

void
birnet_bblock_append (BirnetBBlock    *bblock,
		   guint         n_bytes,
		   const guint8 *bytes)
{
  g_return_if_fail (bblock != NULL);

  if (n_bytes)
    {
      guint i;

      g_return_if_fail (bytes != NULL);

      i = bblock->n_bytes;
      bblock->n_bytes += n_bytes;
      bblock->bytes = g_renew (guint8, bblock->bytes, bblock->n_bytes);
      memcpy (bblock->bytes + i, bytes, n_bytes * sizeof (bblock->bytes[0]));
    }
}

void
birnet_bblock_append1 (BirnetBBlock *bblock,
		    guint8     byte0)
{
  guint i;

  g_return_if_fail (bblock != NULL);

  i = bblock->n_bytes++;
  bblock->bytes = g_renew (guint8, bblock->bytes, bblock->n_bytes);
  bblock->bytes[i] = byte0;
}

guint
birnet_bblock_length (const BirnetBBlock *bblock)
{
  g_return_val_if_fail (bblock != NULL, 0);
  return bblock->n_bytes;
}

guint8*
birnet_bblock_get (const BirnetBBlock *bblock)
{
  g_return_val_if_fail (bblock != NULL, NULL);
  return bblock->bytes;
}


/* --- BirnetFBlock primitive type --- */
BirnetFBlock*
birnet_fblock_new (void)
{
  BirnetFBlock *fblock = birnet_new_struct (BirnetFBlock, 1);
  fblock->ref_count = 1;
  fblock->n_values = 0;
  fblock->values = NULL;
  fblock->freefunc = g_free;
  return fblock;
}

BirnetFBlock*
birnet_fblock_new_sized (guint size)
{
  BirnetFBlock *fblock = birnet_fblock_new ();
  birnet_fblock_resize (fblock, size);
  return fblock;
}

BirnetFBlock*
birnet_fblock_new_foreign (guint     n_values,
                        gfloat   *values,
                        GFreeFunc freefunc)
{
  g_return_val_if_fail (n_values == 0 || values != NULL, NULL);
  BirnetFBlock *fblock = birnet_fblock_new ();
  fblock->n_values = n_values;
  fblock->values = values;
  fblock->freefunc = freefunc;
  return fblock;
}

BirnetFBlock*
birnet_fblock_ref (BirnetFBlock *fblock)
{
  g_return_val_if_fail (fblock != NULL, NULL);
  g_return_val_if_fail (fblock->ref_count > 0, NULL);

  fblock->ref_count++;
  return fblock;
}

void
birnet_fblock_unref (BirnetFBlock *fblock)
{
  g_return_if_fail (fblock != NULL);
  g_return_if_fail (fblock->ref_count > 0);

  fblock->ref_count--;
  if (fblock->ref_count == 0)
    {
      fblock->freefunc (fblock->values);
      birnet_delete_struct (BirnetFBlock, fblock);
    }
}

static void
fblock_resize (BirnetFBlock *fblock,
               guint      size)
{
  guint oldsize = fblock->n_values;
  fblock->n_values = size;
  if (fblock->freefunc == g_free)
    fblock->values = g_renew (gfloat, fblock->values, fblock->n_values);
  else
    {
      gfloat *values = g_new (gfloat, fblock->n_values);
      memcpy (values, fblock->values, MIN (fblock->n_values, oldsize) * sizeof (values[0]));
      fblock->freefunc (fblock->values);
      fblock->values = values;
      fblock->freefunc = g_free;
    }
}

void
birnet_fblock_resize (BirnetFBlock *fblock,
		   guint      size)
{
  g_return_if_fail (fblock != NULL);

  guint osize = fblock->n_values;
  fblock_resize (fblock, size);
  if (size > osize)
    memset (fblock->values + osize, 0, sizeof (fblock->values[0]) * (size - osize));
}

BirnetFBlock*
birnet_fblock_copy_deep (const BirnetFBlock *fblock)
{
  BirnetFBlock *fb;

  g_return_val_if_fail (fblock != NULL, NULL);
  g_return_val_if_fail (fblock->ref_count > 0, NULL);

  fb = birnet_fblock_new ();
  fb->n_values = fblock->n_values;
  fb->values = g_memdup (fblock->values, fblock->n_values * sizeof (fblock->values[0]));
  return fb;
}

void
birnet_fblock_append (BirnetFBlock    *fblock,
		   guint         n_values,
		   const gfloat *values)
{
  g_return_if_fail (fblock != NULL);

  if (n_values)
    {
      g_return_if_fail (values != NULL);
      guint oldsize = fblock->n_values;
      fblock_resize (fblock, oldsize + n_values);
      memcpy (fblock->values + oldsize, values, n_values * sizeof (fblock->values[0]));
    }
}

void
birnet_fblock_append1 (BirnetFBlock *fblock,
		    gfloat     float0)
{
  g_return_if_fail (fblock != NULL);
  fblock_resize (fblock, fblock->n_values + 1);
  fblock->values[fblock->n_values - 1] = float0;
}

guint
birnet_fblock_length (const BirnetFBlock *fblock)
{
  g_return_val_if_fail (fblock != NULL, 0);
  return fblock->n_values;
}

gfloat*
birnet_fblock_get (const BirnetFBlock *fblock)
{
  g_return_val_if_fail (fblock != NULL, NULL);
  return fblock->values;
}


/* --- BirnetSeq primitive type --- */
BirnetSeq*
birnet_seq_new (void)
{
  BirnetSeq *s;

  s = birnet_new_struct (BirnetSeq, 1);
  s->ref_count = 1;
  s->n_elements = 0;
  s->elements = NULL;
  return s;
}

BirnetSeq*
birnet_seq_ref (BirnetSeq *seq)
{
  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (seq->ref_count > 0, NULL);

  seq->ref_count++;
  return seq;
}

void
birnet_seq_clear (BirnetSeq *seq)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (seq->ref_count > 0);

  while (seq->n_elements)
    g_value_unset (seq->elements + --seq->n_elements);
  g_free (seq->elements);
  seq->elements = NULL;
}

void
birnet_seq_unref (BirnetSeq *seq)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (seq->ref_count > 0);

  seq->ref_count--;
  if (seq->ref_count == 0)
    {
      while (seq->n_elements)
	g_value_unset (seq->elements + --seq->n_elements);
      g_free (seq->elements);
      birnet_delete_struct (BirnetSeq, seq);
    }
}

static inline gulong
upper_power2 (gulong number)
{
  return number ? 1 << g_bit_storage (number - 1) : 0;
}

static void
birnet_seq_append_copy (BirnetSeq       *seq,
                     GType         value_type,
		     gboolean      deep_copy,
		     const GValue *value)
{
  guint i, l, n;

  g_return_if_fail (seq != NULL);

  l = upper_power2 (seq->n_elements);
  i = seq->n_elements++;
  n = upper_power2 (seq->n_elements);
  if (n > l)
    {
      seq->elements = g_realloc (seq->elements, n * sizeof (seq->elements[0]));
      memset (seq->elements + l, 0, (n - l) * sizeof (seq->elements[0]));
    }
  g_value_init (seq->elements + i, value_type);
  if (deep_copy)
    birnet_value_copy_deep (value, seq->elements + i);
  else if (value)
    g_value_copy (value, seq->elements + i);
}

BirnetSeq*
birnet_seq_copy_deep (const BirnetSeq *seq)
{
  BirnetSeq *s;
  guint i;

  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (seq->ref_count > 0, NULL);

  s = birnet_seq_new ();
  for (i = 0; i < seq->n_elements; i++)
    birnet_seq_append_copy (s, G_VALUE_TYPE (seq->elements + i), TRUE, seq->elements + i);
  return s;
}

void
birnet_seq_append (BirnetSeq       *seq,
		const GValue *value)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  birnet_seq_append_copy (seq, G_VALUE_TYPE (value), FALSE, value);
}

GValue*
birnet_seq_append_empty (BirnetSeq          *seq,
                      GType            value_type)
{
  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (G_TYPE_IS_VALUE (value_type), NULL);

  birnet_seq_append_copy (seq, value_type, FALSE, NULL);
  return seq->elements + seq->n_elements - 1;
}

guint
birnet_seq_length (const BirnetSeq *seq)
{
  return seq ? seq->n_elements : 0;
}

GValue*
birnet_seq_get (const BirnetSeq *seq,
	     guint         index)
{
  g_return_val_if_fail (seq != NULL, NULL);
  g_return_val_if_fail (index < seq->n_elements, NULL);
  
  return seq->elements + index;
}

gboolean
birnet_seq_check (BirnetSeq *seq,
	       GType   element_type)
{
  guint i;
  
  g_return_val_if_fail (seq != NULL, FALSE);
  
  for (i = 0; i < seq->n_elements; i++)
    if (!G_VALUE_HOLDS (seq->elements + i, element_type))
      return FALSE;
  return TRUE;
}

gboolean
birnet_seq_validate (BirnetSeq     *seq,
                  GParamSpec *pspec)
{
  g_return_val_if_fail (seq != NULL, FALSE);
  g_return_val_if_fail (pspec != NULL, FALSE);

  GValue *v = birnet_value_seq (seq);
  gboolean changed = g_param_value_validate (pspec, v);
  birnet_value_free (v);
  return changed;
}

void
birnet_seq_append_bool (BirnetSeq      *seq,
		     BirnetBool      v_bool)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_BOOL);
  birnet_value_set_bool (value, v_bool);
}

void
birnet_seq_append_int (BirnetSeq      *seq,
		    BirnetInt       v_int)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_INT);
  birnet_value_set_int (value, v_int);
}

void
birnet_seq_append_num (BirnetSeq      *seq,
		    BirnetNum       v_num)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_NUM);
  birnet_value_set_num (value, v_num);
}

void
birnet_seq_append_real (BirnetSeq          *seq,
		     BirnetReal         v_real)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_REAL);
  birnet_value_set_real (value, v_real);
}

void
birnet_seq_append_string (BirnetSeq      *seq,
		       const gchar *string)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_STRING);
  birnet_value_set_string (value, string);
}

void
birnet_seq_append_choice (BirnetSeq      *seq,
		       const gchar *choice)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_CHOICE);
  birnet_value_set_choice (value, choice);
}

void
birnet_seq_append_bblock (BirnetSeq      *seq,
		       BirnetBBlock   *bblock)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_BBLOCK);
  birnet_value_set_bblock (value, bblock);
}

void
birnet_seq_append_fblock (BirnetSeq      *seq,
		       BirnetFBlock   *fblock)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_FBLOCK);
  birnet_value_set_fblock (value, fblock);
}

void
birnet_seq_append_pspec (BirnetSeq      *seq,
		      GParamSpec  *pspec)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_PSPEC);
  birnet_value_set_pspec (value, pspec);
}

void
birnet_seq_append_seq (BirnetSeq      *seq,
		    BirnetSeq      *v_seq)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_SEQ);
  birnet_value_set_seq (value, v_seq);
}

void
birnet_seq_append_rec (BirnetSeq      *seq,
		    BirnetRec      *rec)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_REC);
  birnet_value_set_rec (value, rec);
}

void
birnet_seq_append_proxy (BirnetSeq      *seq,
		      BirnetProxy     proxy)
{
  GValue *value = birnet_seq_append_empty (seq, BIRNET_TYPE_PROXY);
  birnet_value_set_proxy (value, proxy);
}

static inline BirnetNum
value_as_num (GValue *v)
{
  if (v)
    {
      if (BIRNET_VALUE_HOLDS_BOOL (v))
	return birnet_value_get_bool (v);
      else if (BIRNET_VALUE_HOLDS_INT (v))
	return birnet_value_get_int (v);
      else if (BIRNET_VALUE_HOLDS_REAL (v))
	return birnet_value_get_real (v);
      else if (BIRNET_VALUE_HOLDS_NUM (v))
	return birnet_value_get_num (v);
    }
  return 0;
}

static inline BirnetReal
value_as_real (GValue *v)
{
  if (v)
    {
      if (BIRNET_VALUE_HOLDS_BOOL (v))
	return birnet_value_get_bool (v);
      else if (BIRNET_VALUE_HOLDS_INT (v))
	return birnet_value_get_int (v);
      else if (BIRNET_VALUE_HOLDS_REAL (v))
	return birnet_value_get_real (v);
      else if (BIRNET_VALUE_HOLDS_NUM (v))
	return birnet_value_get_num (v);
    }
  return 0;
}

static inline const gchar*
value_as_string (GValue *v)
{
  if (v)
    {
      if (BIRNET_VALUE_HOLDS_STRING (v))
	return birnet_value_get_string (v);
      else if (BIRNET_VALUE_HOLDS_CHOICE (v))
	return birnet_value_get_choice (v);
    }
  return NULL;
}

BirnetBool
birnet_seq_get_bool (BirnetSeq *seq,
		  guint   index)
{
  return value_as_num (birnet_seq_get (seq, index)) != 0;
}

BirnetInt
birnet_seq_get_int (BirnetSeq *seq,
		 guint   index)
{
  return value_as_num (birnet_seq_get (seq, index));
}

BirnetNum
birnet_seq_get_num (BirnetSeq *seq,
		 guint   index)
{
  return value_as_num (birnet_seq_get (seq, index));
}

BirnetReal
birnet_seq_get_real (BirnetSeq *seq,
		  guint   index)
{
  return value_as_real (birnet_seq_get (seq, index));
}

const gchar*
birnet_seq_get_string (BirnetSeq *seq,
		    guint   index)
{
  return value_as_string (birnet_seq_get (seq, index));
}

const gchar*
birnet_seq_get_choice (BirnetSeq *seq,
		    guint   index)
{
  return value_as_string (birnet_seq_get (seq, index));
}

BirnetBBlock*
birnet_seq_get_bblock (BirnetSeq *seq,
		    guint   index)
{
  GValue *v = birnet_seq_get (seq, index);
  if (v && BIRNET_VALUE_HOLDS_BBLOCK (v))
    return birnet_value_get_bblock (v);
  return NULL;
}

BirnetFBlock*
birnet_seq_get_fblock (BirnetSeq *seq,
		    guint   index)
{
  GValue *v = birnet_seq_get (seq, index);
  if (v && BIRNET_VALUE_HOLDS_FBLOCK (v))
    return birnet_value_get_fblock (v);
  return NULL;
}

GParamSpec*
birnet_seq_get_pspec (BirnetSeq *seq,
		   guint   index)
{
  GValue *v = birnet_seq_get (seq, index);
  if (v && BIRNET_VALUE_HOLDS_PSPEC (v))
    return birnet_value_get_pspec (v);
  return NULL;
}

BirnetSeq*
birnet_seq_get_seq (BirnetSeq *seq,
		 guint   index)
{
  GValue *v = birnet_seq_get (seq, index);
  if (v && BIRNET_VALUE_HOLDS_SEQ (v))
    return birnet_value_get_seq (v);
  return NULL;
}

BirnetRec*
birnet_seq_get_rec (BirnetSeq *seq,
		 guint   index)
{
  GValue *v = birnet_seq_get (seq, index);
  if (v && BIRNET_VALUE_HOLDS_REC (v))
    return birnet_value_get_rec (v);
  return NULL;
}

BirnetProxy
birnet_seq_get_proxy (BirnetSeq *seq,
		   guint   index)
{
  GValue *v = birnet_seq_get (seq, index);
  if (v && BIRNET_VALUE_HOLDS_PROXY (v))
    return birnet_value_get_proxy (v);
  return 0;
}

gchar**
birnet_seq_to_strv (BirnetSeq *seq)
{
  GSList *slist = NULL;
  gchar **strv;
  guint i;

  g_return_val_if_fail (seq != NULL, NULL);

  for (i = 0; i < seq->n_elements; i++)
    if (G_VALUE_HOLDS_STRING (seq->elements + i))
      slist = g_slist_prepend (slist, birnet_value_get_string (seq->elements + i));
  slist = g_slist_reverse (slist);
  strv = g_strslistv (slist);
  g_slist_free (slist);
  return strv;
}

BirnetSeq*
birnet_seq_from_strv (gchar **strv)
{
  BirnetSeq *seq;
  guint i;
  if (!strv)
    return NULL;

  seq = birnet_seq_new ();
  for (i = 0; strv[i]; i++)
    birnet_seq_append_string (seq, strv[i]);
  return seq;
}

BirnetSeq*
birnet_seq_from_cstrv (const gchar **strv)
{
  BirnetSeq *seq;
  guint i;
  if (!strv)
    return NULL;

  seq = birnet_seq_new ();
  for (i = 0; strv[i]; i++)
    birnet_seq_append_string (seq, strv[i]);
  return seq;
}


/* --- BirnetRec primitive type --- */
BirnetRec*
birnet_rec_new (void)
{
  BirnetRec *rec = birnet_new_struct (BirnetRec, 1);
  rec->ref_count = 1;
  rec->n_fields = 0;
  rec->sorted = TRUE;
  rec->fields = NULL;
  rec->field_names = NULL;
  return rec;
}

BirnetRec*
birnet_rec_ref (BirnetRec *rec)
{
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (rec->ref_count > 0, NULL);
  
  rec->ref_count++;
  
  return rec;
}

static void
birnet_rec_empty (BirnetRec *rec)
{
  guint i;
  
  for (i = 0; i < rec->n_fields; i++)
    {
      g_value_unset (rec->fields + i);
      g_free (rec->field_names[i]);
    }
  g_free (rec->fields);
  g_free (rec->field_names);
  rec->n_fields = 0;
  rec->sorted = TRUE;
  rec->fields = NULL;
  rec->field_names = NULL;
}

void
birnet_rec_unref (BirnetRec *rec)
{
  g_return_if_fail (rec != NULL);
  g_return_if_fail (rec->ref_count > 0);
  
  rec->ref_count--;
  if (rec->ref_count == 0)
    {
      birnet_rec_empty (rec);
      birnet_delete_struct (BirnetRec, rec);
    }
}

void
birnet_rec_clear (BirnetRec *rec)
{
  g_return_if_fail (rec != NULL);
  g_return_if_fail (rec->ref_count > 0);

  birnet_rec_empty (rec);
}

guint
birnet_rec_n_fields (const BirnetRec *rec)
{
  g_return_val_if_fail (rec != NULL, 0);
  return rec ? rec->n_fields : 0;
}

GValue*
birnet_rec_field (const BirnetRec *rec,
	       guint         index)
{
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (index < rec->n_fields, NULL);
  
  return rec->fields + index;
}

static inline gchar*
dupcanon (const gchar *field_name)
{
  return g_strcanon (g_strdup (field_name),
		     G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS,
		     '-');
}

static inline guint
birnet_rec_lookup (BirnetRec      *rec,
		const gchar *field_name)
{
  if (rec->sorted)
    {
      gchar **nodes = rec->field_names;
      guint n_nodes = rec->n_fields, offs = 0;
      gint cmp = 0;
      while (offs < n_nodes)
	{
	  guint i = (offs + n_nodes) >> 1;
	  cmp = strcmp (field_name, nodes[i]);
	  if (cmp == 0)
	    return i;
	  else if (cmp < 0)
	    n_nodes = i;
	  else /* (cmp > 0) */
	    offs = i + 1;
	}
    }
  else
    {
      guint i;
      for (i = 0; i < rec->n_fields; i++)
	if (strcmp (field_name, rec->field_names[i]) == 0)
	  return i;
    }
  return rec->n_fields; /* no match */
}

static void
birnet_rec_set_copy (BirnetRec       *rec,
		  const gchar  *field_name,
                  GType         value_type,
		  gboolean      deep_copy,
		  const GValue *value)
{
  gchar *name;
  guint i;
  
  name = dupcanon (field_name);
  i = birnet_rec_lookup (rec, name);
  if (i >= rec->n_fields)
    {
      i = rec->n_fields++;
      rec->fields = g_realloc (rec->fields, rec->n_fields * sizeof (rec->fields[0]));
      memset (rec->fields + i, 0, sizeof (rec->fields[0]));
      rec->field_names = g_realloc (rec->field_names, rec->n_fields * sizeof (rec->field_names[0]));
      rec->field_names[i] = name;
      /* we don't sort upon insertion to speed up record creation */
      rec->sorted = FALSE;
    }
  else
    {
      g_value_unset (rec->fields + i);
      g_free (name);
    }
  g_value_init (rec->fields + i, value_type);
  if (deep_copy)
    birnet_value_copy_deep (value, rec->fields + i);
  else if (value)
    g_value_copy (value, rec->fields + i);
}

void
birnet_rec_set (BirnetRec       *rec,
	     const gchar  *field_name,
	     const GValue *value)
{
  g_return_if_fail (rec != NULL);
  g_return_if_fail (field_name != NULL);
  g_return_if_fail (BIRNET_IS_VALUE (value));
  
  birnet_rec_set_copy (rec, field_name, G_VALUE_TYPE (value), FALSE, value);
}

GValue*
birnet_rec_get (BirnetRec      *rec,
	     const gchar *field_name)
{
  gchar *name;
  guint i;
  
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (field_name != NULL, NULL);

  if (!rec->sorted)
    birnet_rec_sort (rec);
  name = dupcanon (field_name);
  i = birnet_rec_lookup (rec, name);
  g_free (name);
  if (i < rec->n_fields)
    return rec->fields + i;
  return NULL;
}

GValue*
birnet_rec_forced_get (BirnetRec          *rec,
                    const gchar     *field_name,
                    GType            value_type)
{
  gchar *name;
  guint i;
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (field_name != NULL, NULL);
  g_return_val_if_fail (G_TYPE_IS_VALUE (value_type), NULL);
  if (!rec->sorted)
    birnet_rec_sort (rec);
  name = dupcanon (field_name);
  i = birnet_rec_lookup (rec, name);
  if (i < rec->n_fields)
    {
      GValue *value = rec->fields + i;
      g_free (name);
      if (G_VALUE_TYPE (value) != value_type)
        {
          g_value_unset (value);
          g_value_init (value, value_type);
        }
      return value;
    }
  birnet_rec_set_copy (rec, field_name, value_type, FALSE, NULL);
  birnet_rec_sort (rec);
  i = birnet_rec_lookup (rec, name);
  g_free (name);
  return rec->fields + i;
}

BirnetRec*
birnet_rec_copy_deep (BirnetRec *rec)
{
  BirnetRec *r;
  guint i;
  
  g_return_val_if_fail (rec != NULL, NULL);
  g_return_val_if_fail (rec->ref_count > 0, NULL);

  birnet_rec_sort (rec);
  r = birnet_rec_new ();
  for (i = 0; i < rec->n_fields; i++)
    birnet_rec_set_copy (r, rec->field_names[i], G_VALUE_TYPE (&rec->fields[i]), TRUE, &rec->fields[i]);
  r->sorted = TRUE;
  return r;
}

gboolean
birnet_rec_check (BirnetRec      *rec,
	       BirnetRecFields rfields)
{
  guint i;

  g_return_val_if_fail (rec != NULL, FALSE);
  g_return_val_if_fail (rfields.n_fields > 0, FALSE);

  if (!rec->sorted)
    birnet_rec_sort (rec);
  for (i = 0; i < rfields.n_fields; i++)
    {
      guint n = birnet_rec_lookup (rec, rfields.fields[i]->name);
      GValue *value = n < rec->n_fields ? rec->fields + n : NULL;
      if (!value || !G_VALUE_HOLDS (value, G_PARAM_SPEC_VALUE_TYPE (rfields.fields[i])))
	return FALSE;
    }
  return TRUE;
}

static int
strpointercmp (const void *p1,
	       const void *p2)
{
  gchar *const *s1 = p1;
  gchar *const *s2 = p2;
  return strcmp (*s1, *s2);
}

void
birnet_rec_sort (BirnetRec *rec)
{
  g_return_if_fail (rec != NULL);
  
  if (!rec->sorted && rec->n_fields > 1)
    {
      gchar **fnames = g_memdup (rec->field_names, rec->n_fields * sizeof (rec->field_names[0]));
      GValue *fields = g_new (GValue, rec->n_fields);
      guint i;
      /* sort field names */
      qsort (fnames, rec->n_fields, sizeof (fnames[0]), strpointercmp);
      /* sort fields */
      for (i = 0; i < rec->n_fields; i++)
	{
	  guint n = 0;
	  /* find corresponding field */
	  while (rec->field_names[n] != fnames[i])
	    n++;
	  /* relocate field */
	  memcpy (fields + i, rec->fields + n, sizeof (fields[0]));
	}
      g_free (rec->field_names);
      rec->field_names = fnames;
      g_free (rec->fields);
      rec->fields = fields;
    }
  rec->sorted = TRUE;
}

void
birnet_rec_swap_fields (BirnetRec *rec,
		     BirnetRec *swapper)
{
  guint n;
  GValue *fields;
  gchar **names;

  g_return_if_fail (rec != NULL);
  g_return_if_fail (swapper != NULL);
  
  birnet_rec_sort (rec);
  birnet_rec_sort (swapper);
  n = rec->n_fields;
  fields = rec->fields;
  names = rec->field_names;
  rec->n_fields = swapper->n_fields;
  rec->fields = swapper->fields;
  rec->field_names = swapper->field_names;
  swapper->n_fields = n;
  swapper->fields = fields;
  swapper->field_names = names;
}

gboolean
birnet_rec_validate (BirnetRec      *rec,
		  BirnetRecFields fields)
{
  GParamSpec *pspec;
  GValue *v;
  gboolean changed;

  g_return_val_if_fail (rec != NULL, FALSE);

  pspec = birnet_pspec_rec ("auto", NULL, NULL, fields, ":readwrite");
  v = birnet_value_rec (rec);
  changed = g_param_value_validate (pspec, v);
  birnet_value_free (v);
  g_param_spec_sink (pspec);
  return changed;
}

void
birnet_rec_set_bool (BirnetRec      *rec,
		  const gchar *field_name,
		  BirnetBool      v_bool)
{
  GValue *value = birnet_value_bool (v_bool);
  birnet_rec_set (rec, field_name, value);
  birnet_value_free (value);
}

void
birnet_rec_set_int	(BirnetRec      *rec,
		 const gchar *field_name,
		 BirnetInt	      v_int)
{
  GValue *value = birnet_value_int (v_int);
  birnet_rec_set (rec, field_name, value);
  birnet_value_free (value);
}

void
birnet_rec_set_num	(BirnetRec      *rec,
		 const gchar *field_name,
		 BirnetNum	      v_num)
{
  GValue *value = birnet_value_num (v_num);
  birnet_rec_set (rec, field_name, value);
  birnet_value_free (value);
}

void
birnet_rec_set_real (BirnetRec          *rec,
		  const gchar     *field_name,
		  BirnetReal	  v_real)
{
  GValue *value = birnet_value_real (v_real);
  birnet_rec_set (rec, field_name, value);
  birnet_value_free (value);
}

void
birnet_rec_set_string (BirnetRec      *rec,
		    const gchar *field_name,
		    const gchar	*string)
{
  GValue value = { 0, };
  g_value_init (&value, BIRNET_TYPE_STRING);
  g_value_set_static_string (&value, string);
  birnet_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
birnet_rec_set_choice (BirnetRec      *rec,
		    const gchar *field_name,
		    const gchar	*choice)
{
  GValue value = { 0, };
  g_value_init (&value, BIRNET_TYPE_CHOICE);
  g_value_set_static_string (&value, choice);
  birnet_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
birnet_rec_set_bblock (BirnetRec      *rec,
		    const gchar *field_name,
		    BirnetBBlock	*bblock)
{
  GValue value = { 0, };
  g_value_init (&value, BIRNET_TYPE_BBLOCK);
  g_value_set_static_boxed (&value, bblock);
  birnet_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
birnet_rec_set_fblock (BirnetRec      *rec,
		    const gchar *field_name,
		    BirnetFBlock	*fblock)
{
  GValue value = { 0, };
  g_value_init (&value, BIRNET_TYPE_FBLOCK);
  g_value_set_static_boxed (&value, fblock);
  birnet_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
birnet_rec_set_pspec (BirnetRec      *rec,
		   const gchar *field_name,
		   GParamSpec  *pspec)
{
  GValue *value = birnet_value_pspec (pspec);
  birnet_rec_set (rec, field_name, value);
  birnet_value_free (value);
}

void
birnet_rec_set_seq	(BirnetRec      *rec,
		 const gchar *field_name,
		 BirnetSeq	     *seq)
{
  GValue value = { 0, };
  g_value_init (&value, BIRNET_TYPE_SEQ);
  g_value_set_static_boxed (&value, seq);
  birnet_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
birnet_rec_set_rec (BirnetRec      *rec,
		 const gchar *field_name,
		 BirnetRec	     *v_rec)
{
  GValue value = { 0, };
  g_value_init (&value, BIRNET_TYPE_REC);
  g_value_set_static_boxed (&value, v_rec);
  birnet_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

void
birnet_rec_set_proxy (BirnetRec      *rec,
		   const gchar *field_name,
		   BirnetProxy     proxy)
{
  GValue value = { 0, };
  g_value_init (&value, BIRNET_TYPE_PROXY);
  birnet_value_set_proxy (&value, proxy);
  birnet_rec_set (rec, field_name, &value);
  g_value_unset (&value);
}

BirnetBool
birnet_rec_get_bool (BirnetRec      *rec,
		  const gchar *field_name)
{
  return value_as_num (birnet_rec_get (rec, field_name)) != 0;
}

BirnetInt
birnet_rec_get_int (BirnetRec      *rec,
		 const gchar *field_name)
{
  return value_as_num (birnet_rec_get (rec, field_name));
}

BirnetNum
birnet_rec_get_num (BirnetRec      *rec,
		 const gchar *field_name)
{
  return value_as_num (birnet_rec_get (rec, field_name));
}

BirnetReal
birnet_rec_get_real (BirnetRec      *rec,
		  const gchar *field_name)
{
  return value_as_real (birnet_rec_get (rec, field_name));
}

const gchar*
birnet_rec_get_string (BirnetRec      *rec,
		    const gchar *field_name)
{
  return value_as_string (birnet_rec_get (rec, field_name));
}

const gchar*
birnet_rec_get_choice (BirnetRec      *rec,
		    const gchar *field_name)
{
  return value_as_string (birnet_rec_get (rec, field_name));
}

BirnetBBlock*
birnet_rec_get_bblock (BirnetRec      *rec,
		    const gchar *field_name)
{
  GValue *v = birnet_rec_get (rec, field_name);
  if (v && BIRNET_VALUE_HOLDS_BBLOCK (v))
    return birnet_value_get_bblock (v);
  return NULL;
}

BirnetFBlock*
birnet_rec_get_fblock (BirnetRec      *rec,
		    const gchar *field_name)
{
  GValue *v = birnet_rec_get (rec, field_name);
  if (v && BIRNET_VALUE_HOLDS_FBLOCK (v))
    return birnet_value_get_fblock (v);
  return NULL;
}

GParamSpec*
birnet_rec_get_pspec (BirnetRec      *rec,
		   const gchar *field_name)
{
  GValue *v = birnet_rec_get (rec, field_name);
  if (v && BIRNET_VALUE_HOLDS_PSPEC (v))
    return birnet_value_get_pspec (v);
  return NULL;
}

BirnetSeq*
birnet_rec_get_seq (BirnetRec      *rec,
		 const gchar *field_name)
{
  GValue *v = birnet_rec_get (rec, field_name);
  if (v && BIRNET_VALUE_HOLDS_SEQ (v))
    return birnet_value_get_seq (v);
  return NULL;
}

BirnetRec*
birnet_rec_get_rec (BirnetRec      *rec,
		 const gchar *field_name)
{
  GValue *v = birnet_rec_get (rec, field_name);
  if (v && BIRNET_VALUE_HOLDS_REC (v))
    return birnet_value_get_rec (v);
  return NULL;
}

BirnetProxy
birnet_rec_get_proxy (BirnetRec      *rec,
		   const gchar *field_name)
{
  GValue *v = birnet_rec_get (rec, field_name);
  if (v && BIRNET_VALUE_HOLDS_PROXY (v))
    return birnet_value_get_proxy (v);
  return 0;
}


/* --- basic comparisons --- */
gint
birnet_pointer_cmp (gconstpointer   value1,
                 gconstpointer   value2,
                 gpointer        dummy)
{
  const char *p1 = value1;
  const char *p2 = value2;
  return p1 < p2 ? -1 : p1 != p2;
}


/* --- ring (circular-list) --- */
static inline BirnetRing*
birnet_ring_prepend_link_i (BirnetRing *head,
			 BirnetRing *ring)
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

static inline BirnetRing*
birnet_ring_prepend_i (BirnetRing *head,
		    gpointer data)
{
  BirnetRing *ring = birnet_new_struct (BirnetRing, 1);
  
  ring->data = data;
  return birnet_ring_prepend_link_i (head, ring);
}

static inline BirnetRing*
birnet_ring_append_link_i (BirnetRing *head,
			BirnetRing *ring)
{
  birnet_ring_prepend_link_i (head, ring);
  return head ? head : ring;
}

BirnetRing*
birnet_ring_prepend (BirnetRing  *head,
		  gpointer data)
{
  return birnet_ring_prepend_i (head, data);
}

BirnetRing*
birnet_ring_prepend_uniq (BirnetRing  *head,
		       gpointer data)
{
  BirnetRing *ring;
  for (ring = head; ring; ring = birnet_ring_walk (ring, head))
    if (ring->data == data)
      return head;
  return birnet_ring_prepend_i (head, data);
}

BirnetRing*
birnet_ring_append (BirnetRing *head,
		 gpointer data)
{
  BirnetRing *ring = birnet_ring_prepend_i (head, data);
  return head ? head : ring;
}

BirnetRing*
birnet_ring_append_uniq (BirnetRing *head,
                      gpointer data)
{
  BirnetRing *ring;
  for (ring = head; ring; ring = birnet_ring_walk (ring, head))
    if (ring->data == data)
      return head;
  ring = birnet_ring_prepend_i (head, data);
  return head ? head : ring;
}

BirnetRing*
birnet_ring_insert_before (BirnetRing        *head,
                        BirnetRing        *sibling,
                        gpointer        data)
{
  if (!sibling)
    return birnet_ring_append (head, data);
  BirnetRing *node = birnet_ring_prepend (sibling, data);
  return sibling == head ? node : head;
}

BirnetRing*
birnet_ring_insert (BirnetRing *head,
                 gpointer data,
                 gint     position)
{
  if (position < 0)
    return birnet_ring_append (head, data);
  else if (position == 0)
    return birnet_ring_prepend (head, data);
  BirnetRing *node = birnet_ring_nth (head, position);
  if (node)
    return birnet_ring_insert_before (head, node, data);
  else
    return birnet_ring_append (head, data);
}

gint
birnet_ring_position (const BirnetRing  *head,
                   const BirnetRing  *node)
{
  guint i = 0;
  const BirnetRing *ring;
  for (ring = head; ring; ring = birnet_ring_walk (ring, head), i++)
    if (ring == node)
      return i;
  return -1;
}

gint
birnet_ring_index (const BirnetRing *head,
                gconstpointer  data)
{
  guint i = 0;
  const BirnetRing *ring;
  for (ring = head; ring; ring = birnet_ring_walk (ring, head), i++)
    if (ring->data == data)
      return i;
  return -1;
}

BirnetRing*
birnet_ring_copy (const BirnetRing *head)
{
  const BirnetRing *walk;
  BirnetRing *dest = NULL;
  for (walk = head; walk; walk = birnet_ring_walk (walk, head))
    dest = birnet_ring_append (dest, walk->data);
  return dest;
}

BirnetRing*
birnet_ring_copy_deep (const BirnetRing  *head,
		    BirnetRingDataFunc copy,
		    gpointer        func_data)
{
  const BirnetRing *walk;
  BirnetRing *dest = NULL;
  for (walk = head; walk; walk = birnet_ring_walk (walk, head))
    dest = birnet_ring_append (dest, copy (walk->data, func_data));
  return dest;
}

BirnetRing*
birnet_ring_copy_rest (const BirnetRing *ring,
                    const BirnetRing *head)
{
  const BirnetRing *walk;
  BirnetRing *dest = NULL;
  for (walk = ring; walk; walk = birnet_ring_walk (walk, head))
    dest = birnet_ring_append (dest, walk->data);
  return dest;
}

BirnetRing*
birnet_ring_concat (BirnetRing *head1,
		 BirnetRing *head2)
{
  BirnetRing *tail1, *tail2;
  
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
BirnetRing*
birnet_ring_split (BirnetRing *head1,
		BirnetRing *head2)
{
  BirnetRing *tail1, *tail2;

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

static inline BirnetRing*
birnet_ring_unlink_node_dangling (BirnetRing *head,
                               BirnetRing *node)
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

BirnetRing*
birnet_ring_remove_node (BirnetRing *head,
		      BirnetRing *node)
{
  if (!head)
    g_return_val_if_fail (head == NULL && node == NULL, NULL);
  if (!head || !node)
    return NULL;
  
  /* special case one item ring */
  if (head->prev == head)
    {
      g_return_val_if_fail (node == head, head);
      
      birnet_delete_struct (BirnetRing, node);
      return NULL;
    }
  g_return_val_if_fail (node != node->next, head); /* node can't be a one item ring here */
  
  node->next->prev = node->prev;
  node->prev->next = node->next;
  if (head == node)
    head = node->next;
  birnet_delete_struct (BirnetRing, node);
  
  return head;
}

BirnetRing*
birnet_ring_reverse (BirnetRing *head)
{
  if (head)
    {
      BirnetRing *ring = head = head->prev;
      do
	{
	  BirnetRing *tmp = ring;
	  ring = tmp->next;
	  tmp->next = tmp->prev;
	  tmp->prev = ring;
	}
      while (ring != head);
    }
  return head;
}

BirnetRing*
birnet_ring_remove (BirnetRing *head,
		 gpointer data)
{
  BirnetRing *walk;

  if (!head)
    return NULL;
  
  /* make tail data removal an O(1) operation */
  if (head->prev->data == data)
    return birnet_ring_remove_node (head, head->prev);
  
  for (walk = head; walk; walk = birnet_ring_walk (walk, head))
    if (walk->data == data)
      return birnet_ring_remove_node (head, walk);
  
  /* g_warning (G_STRLOC ": couldn't find data item (%p) to remove from ring (%p)", data, head); */
  
  return head;
}

guint
birnet_ring_length (const BirnetRing *head)
{
  const BirnetRing *ring;
  guint i = 0;
  
  for (ring = head; ring; ring = birnet_ring_walk (ring, head))
    i++;

  return i;
}

gint    /* essentially compute length(ring) - test_length, clamped to -1..+1 */
birnet_ring_cmp_length (const BirnetRing *head,
                     guint          test_length)
{
  const BirnetRing *ring = head;

  while (test_length && ring)
    {
      test_length--;
      ring = birnet_ring_walk (ring, head);
    }

  return test_length > 0 ? -1 : ring != NULL;
}

BirnetRing*
birnet_ring_find (const BirnetRing*head,
	       gconstpointer data)
{
  const BirnetRing *ring;
  for (ring = head; ring; ring = birnet_ring_walk (ring, head))
    if (ring->data == (gpointer) data)
      return (BirnetRing*) ring;
  return NULL;
}

BirnetRing*
birnet_ring_nth (const BirnetRing *head,
	      guint          n)
{
  const BirnetRing *ring = head;
  while (n-- && ring)
    ring = birnet_ring_walk (ring, head);
  return (BirnetRing*) ring;
}

gpointer
birnet_ring_nth_data (const BirnetRing *head,
		   guint          n)
{
  const BirnetRing *ring = head;

  while (n-- && ring)
    ring = birnet_ring_walk (ring, head);

  return ring ? ring->data : NULL;
}

void
birnet_ring_free_deep (BirnetRing        *head,
		    GDestroyNotify  data_destroy)
{
  while (head)
    {
      gpointer data = birnet_ring_pop_head (&head);
      data_destroy (data);
      data = birnet_ring_pop_head (&head);
    }
}

void
birnet_ring_free (BirnetRing *head)
{
  if (head)
    {
      head->prev->next = NULL;
      _birnet_free_node_list (head, sizeof (*head));
    }
}

gpointer
birnet_ring_pop_head (BirnetRing **head_p)
{
  gpointer data;
  
  g_return_val_if_fail (head_p != NULL, NULL);
  
  if (!*head_p)
    return NULL;
  data = (*head_p)->data;
  *head_p = birnet_ring_remove_node (*head_p, *head_p);
  
  return data;
}

gpointer
birnet_ring_pop_tail (BirnetRing **head_p)
{
  gpointer data;
  
  g_return_val_if_fail (head_p != NULL, NULL);
  
  if (!*head_p)
    return NULL;
  data = (*head_p)->prev->data;
  *head_p = birnet_ring_remove_node (*head_p, (*head_p)->prev);
  
  return data;
}

BirnetRing*
birnet_ring_from_list (GList *list)
{
  BirnetRing *ring = NULL;
  for (; list; list = list->next)
    ring = birnet_ring_append (ring, list->data);
  return ring;
}

BirnetRing*
birnet_ring_from_list_and_free (GList *list)
{
  BirnetRing *ring = NULL;
  GList *free_list = list;
  for (; list; list = list->next)
    ring = birnet_ring_append (ring, list->data);
  g_list_free (free_list);
  return ring;
}

BirnetRing*
birnet_ring_from_slist (GSList *slist)
{
  BirnetRing *ring = NULL;
  for (; slist; slist = slist->next)
    ring = birnet_ring_append (ring, slist->data);
  return ring;
}

BirnetRing*
birnet_ring_from_slist_and_free (GSList *slist)
{
  BirnetRing *ring = NULL;
  GSList *free_slist = slist;
  for (; slist; slist = slist->next)
    ring = birnet_ring_append (ring, slist->data);
  g_slist_free (free_slist);
  return ring;
}

BirnetRing*
birnet_ring_insert_sorted (BirnetRing	      *head,
			gpointer       insertion_data,
                        BirnetCompareFunc cmp,
                        gpointer       cmp_data)
{
  g_return_val_if_fail (cmp != NULL, head);
  if (!head)
    return birnet_ring_prepend (head, insertion_data);

  /* implement stable sorting by inserting insertion_data *after* equal nodes */

  if (cmp (insertion_data, head->data, cmp_data) >= 0)  /* insert after head */
    {
      BirnetRing *tmp, *tail = head->prev;
      
      /* make appending an O(1) operation */
      if (head == tail || cmp (insertion_data, tail->data, cmp_data) >= 0)
	return birnet_ring_append (head, insertion_data);

      /* walk forward while data >= tmp (skipping equal nodes) */
      for (tmp = head->next; tmp != tail; tmp = tmp->next)
	if (cmp (insertion_data, tmp->data, cmp_data) < 0)
	  break;

      /* insert before sibling which is greater than insertion_data */
      birnet_ring_prepend (tmp, insertion_data); /* keep current head */
      return head;
    }
  else /* cmp < 0 */
    return birnet_ring_prepend (head, insertion_data);
}

BirnetRing*
birnet_ring_merge_sorted (BirnetRing        *head1,
		       BirnetRing        *head2,
                       BirnetCompareFunc  cmp,
                       gpointer        data)
{
  /* implement stable sorting by inserting head2 members *after* equal nodes from head1 */

  if (head1 && head2)
    {
      BirnetRing *tail1 = head1->prev;
      BirnetRing *tail2 = head2->prev;
      BirnetRing *tmp, *ring = NULL;
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
	  ring = birnet_ring_append_link_i (ring, tmp);
	}
      /* reform valid rings, concat sorted rest */
      if (head1)
	{
	  tail1->next = head1;
	  head1->prev = tail1;
	  return birnet_ring_concat (ring, head1);
	}
      if (head2)
	{
	  tail2->next = head2;
	  head2->prev = tail2;
	  return birnet_ring_concat (ring, head2);
	}
      return ring;
    }
  else
    return birnet_ring_concat (head1, head2);
}

BirnetRing*
birnet_ring_sort (BirnetRing        *head,
               BirnetCompareFunc  cmp,
               gpointer        data)
{
  g_return_val_if_fail (cmp != NULL, head);

  /* stable sorting guaranteed by birnet_ring_merge_sorted() */

  if (head && head->next != head)
    {
      BirnetRing *ring, *tmp, *tail = head->prev;
      /* find middle node to get log2 recursion depth */
      ring = tmp = head->next;
      while (tmp != tail && tmp->next != tail)
	{
	  ring = ring->next;
	  tmp = tmp->next->next;
	}
      birnet_ring_split (head, ring);
      return birnet_ring_merge_sorted (birnet_ring_sort (head, cmp, data),
				    birnet_ring_sort (ring, cmp, data),
				    cmp, data);
    }
  return head;
}

BirnetRing* /* eliminates duplicate nodes */
birnet_ring_uniq (BirnetRing        *sorted_ring1,
               BirnetCompareFunc  cmp,
               gpointer        data)
{
  /* always preserves the first of a sublist of consequtive equal elements */
  BirnetRing *r1 = sorted_ring1;
  BirnetRing *r2 = NULL;
  if (r1)
    {
      BirnetRing *last = r1;
      r1 = birnet_ring_unlink_node_dangling (r1, last);
      r2 = last->next = last->prev = last; /* form new ring */
      while (r1)
        {
          BirnetRing *node = r1;
          r1 = birnet_ring_unlink_node_dangling (r1, node);
          if (cmp (last->data, node->data, data))
            {
              last = node;
              r2 = birnet_ring_append_link_i (r2, last);
            }
          else
            birnet_delete_struct (BirnetRing, node);
        }
    }
  return r2;
}

BirnetRing* /* eliminates duplicate nodes */
birnet_ring_uniq_free_deep (BirnetRing        *sorted_ring1,
                         BirnetCompareFunc  cmp,
                         gpointer        data,
                         GDestroyNotify  data_destroy)
{
  /* always preserves the first of a sublist of consequtive equal elements */
  if (!data_destroy)
    return birnet_ring_uniq (sorted_ring1, cmp, data);
  BirnetRing *r1 = sorted_ring1;
  BirnetRing *r2 = NULL;
  if (r1)
    {
      BirnetRing *last = r1;
      r1 = birnet_ring_unlink_node_dangling (r1, last);
      r2 = last->next = last->prev = last; /* form new ring */
      while (r1)
        {
          BirnetRing *node = r1;
          r1 = birnet_ring_unlink_node_dangling (r1, node);
          if (cmp (last->data, node->data, data))
            {
              last = node;
              r2 = birnet_ring_append_link_i (r2, last);
            }
          else
            {
              data_destroy (node->data);
              birnet_delete_struct (BirnetRing, node);
            }
        }
    }
  return r2;
}

BirnetRing* /* eliminates duplicate nodes */
birnet_ring_copy_deep_uniq (const BirnetRing  *sorted_ring1,
                         BirnetRingDataFunc copy,
                         gpointer        copy_data,
                         BirnetCompareFunc  cmp,
                         gpointer        cmp_data)
{
  /* always preserves the first of a sublist of consequtive equal elements */
  if (!copy)
    return birnet_ring_copy_uniq (sorted_ring1, cmp, cmp_data);
  const BirnetRing *r1 = sorted_ring1;
  BirnetRing *r2 = NULL;
  if (r1)
    {
      gpointer last_data = r1->data;
      r2 = birnet_ring_append (r2, copy (last_data, copy_data));
      for (r1 = birnet_ring_walk (r1, sorted_ring1); r1; r1 = birnet_ring_walk (r1, sorted_ring1))
        if (cmp (last_data, r1->data, cmp_data))
          {
            last_data = r1->data;
            r2 = birnet_ring_append (r2, copy (last_data, copy_data));
          }
    }
  return r2;
}

BirnetRing* /* eliminates duplicate nodes */
birnet_ring_copy_uniq (const BirnetRing  *sorted_ring1,
                    BirnetCompareFunc  cmp,
                    gpointer        data)
{
  /* always preserves the first of a sublist of consequtive equal elements */
  const BirnetRing *r1 = sorted_ring1;
  BirnetRing *r2 = NULL;
  if (r1)
    {
      gpointer last_data = r1->data;
      r2 = birnet_ring_append (r2, last_data);
      for (r1 = birnet_ring_walk (r1, sorted_ring1); r1; r1 = birnet_ring_walk (r1, sorted_ring1))
        if (cmp (last_data, r1->data, data))
          {
            last_data = r1->data;
            r2 = birnet_ring_append (r2, last_data);
          }
    }
  return r2;
}

BirnetRing* /* merges rings without dups */
birnet_ring_union (const BirnetRing  *sorted_set1,
                const BirnetRing  *sorted_set2,
                BirnetCompareFunc  cmp,
                gpointer        data)
{
  /* for two equal elements from both sets, the element from sorted_set1 is picked, the one from sorted_set2 discarded */
  const BirnetRing *r1 = sorted_set1, *r2 = sorted_set2;
  BirnetRing *d = NULL;
  while (r1 && r2)
    {
      gint c = cmp (r1->data, r2->data, data);
      if (c < 0)
        {
          d = birnet_ring_append (d, r1->data);
          r1 = birnet_ring_walk (r1, sorted_set1);
        }
      else if (c > 0)
        {
          d = birnet_ring_append (d, r2->data);
          r2 = birnet_ring_walk (r2, sorted_set2);
        }
      else
        {
          d = birnet_ring_append (d, r1->data);
          r1 = birnet_ring_walk (r1, sorted_set1);
          r2 = birnet_ring_walk (r2, sorted_set2);
        }
    }
  return birnet_ring_concat (d, birnet_ring_copy_rest (r1 ? r1 : r2, r1 ? sorted_set1 : sorted_set2));
}

BirnetRing* /* returns nodes contained in both rings */
birnet_ring_intersection (const BirnetRing  *sorted_set1,
                       const BirnetRing  *sorted_set2,
                       BirnetCompareFunc  cmp,
                       gpointer        data)
{
  /* for two equal elements from both sets, only elements from sorted_set1 are picked */
  const BirnetRing *r1 = sorted_set1, *r2 = sorted_set2;
  BirnetRing *d = NULL;
  while (r1 && r2)
    {
      gint c = cmp (r1->data, r2->data, data);
      if (c < 0)
        r1 = birnet_ring_walk (r1, sorted_set1);
      else if (c > 0)
        r2 = birnet_ring_walk (r2, sorted_set2);
      else
        {
          d = birnet_ring_append (d, r1->data);
          r1 = birnet_ring_walk (r1, sorted_set1);
          r2 = birnet_ring_walk (r2, sorted_set2);
        }
    }
  return d;
}

BirnetRing* /* produces set1 without the elements of set2 */
birnet_ring_difference (const BirnetRing  *sorted_set1,
                     const BirnetRing  *sorted_set2,
                     BirnetCompareFunc  cmp,
                     gpointer        data)
{
  const BirnetRing *r1 = sorted_set1, *r2 = sorted_set2;
  BirnetRing *d = NULL;
  while (r1 && r2)
    {
      gint c = cmp (r1->data, r2->data, data);
      if (c < 0)
        {
          d = birnet_ring_append (d, r1->data);
          r1 = birnet_ring_walk (r1, sorted_set1);
        }
      else if (c > 0)
        r2 = birnet_ring_walk (r2, sorted_set2);
      else
        {
          r1 = birnet_ring_walk (r1, sorted_set1);
          r2 = birnet_ring_walk (r2, sorted_set2);
        }
    }
  return birnet_ring_concat (d, birnet_ring_copy_rest (r1, sorted_set1));
}

BirnetRing* /* prduces difference (set1, set2) + difference (set2, set1) */
birnet_ring_symmetric_difference (const BirnetRing  *sorted_set1,
                               const BirnetRing  *sorted_set2,
                               BirnetCompareFunc  cmp,
                               gpointer        data)
{
  const BirnetRing *r1 = sorted_set1, *r2 = sorted_set2;
  BirnetRing *d = NULL;
  while (r1 && r2)
    {
      gint c = cmp (r1->data, r2->data, data);
      if (c < 0)
        {
          d = birnet_ring_append (d, r1->data);
          r1 = birnet_ring_walk (r1, sorted_set1);
        }
      else if (c > 0)
        {
          d = birnet_ring_append (d, r2->data);
          r2 = birnet_ring_walk (r2, sorted_set2);
        }
      else
        {
          r1 = birnet_ring_walk (r1, sorted_set1);
          r2 = birnet_ring_walk (r2, sorted_set2);
        }
    }
  return birnet_ring_concat (d, birnet_ring_copy_rest (r1 ? r1 : r2, r1 ? sorted_set1 : sorted_set2));
}

static inline int
pointerloccmp (const void *pp1,
               const void *pp2)
{
  const gpointer *p1 = pp1;
  const gpointer *p2 = pp2;
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

BirnetRing* /* reproduce all elements from unordered_ring in the order new_ring_order */
birnet_ring_reorder (BirnetRing        *unordered_ring,
                  const BirnetRing  *new_ring_order)
{
  if (!unordered_ring || !new_ring_order)
    return unordered_ring;
  const BirnetRing *ring;

  /* construct a sorted array for faster lookups */
  gpointer *items = NULL;
  guint i, n_items = 0, n_alloced = 0;
  for (ring = unordered_ring; ring; ring = birnet_ring_walk (ring, unordered_ring))
    {
      i = n_items++;
      if (n_items > n_alloced)
        {
          n_alloced = birnet_alloc_upper_power2 (MAX (n_items, 32));
          items = g_renew (gpointer, items, n_alloced);
        }
      items[i] = ring->data;
    }
  birnet_ring_free (unordered_ring);
  unordered_ring = NULL;
  qsort (items, n_items, sizeof (items[0]), pointerloccmp);

  /* collapse duplicates */
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

  /* pick unordered_ring members in the order given by new_ring_order */
  for (ring = new_ring_order; ring; ring = birnet_ring_walk (ring, new_ring_order))
    if (ring_reorder_lookup (n_items, items, ring->data, &i) && counts[i])
      {
        counts[i]--;
        unordered_ring = birnet_ring_append (unordered_ring, ring->data);
      }

  /* append left-over members from sorted_ring */
  for (i = 0; i < n_items; i++)
    while (counts[i]--)
      unordered_ring = birnet_ring_append (unordered_ring, items[i]);

  g_free (items);
  g_free (counts);
  return unordered_ring;
}

gboolean
birnet_ring_includes (const BirnetRing  *sorted_super_set,
                   const BirnetRing  *sorted_sub_set,
                   BirnetCompareFunc  cmp,
                   gpointer        data)
{
  const BirnetRing *r1 = sorted_super_set, *r2 = sorted_sub_set;
  while (r1 && r2)
    {
      gint c = cmp (r1->data, r2->data, data);
      if (c > 0)
        return FALSE;
      else if (c == 0)
        r2 = birnet_ring_walk (r2, sorted_sub_set);
      r1 = birnet_ring_walk (r1, sorted_super_set);
    }
  return !r2;
}

gboolean
birnet_ring_equals (const BirnetRing  *sorted_ring1,
                 const BirnetRing  *sorted_ring2,
                 BirnetCompareFunc  cmp,
                 gpointer        data)
{
  const BirnetRing *r1 = sorted_ring1, *r2 = sorted_ring2;
  while (r1 && r2)
    {
      if (cmp (r1->data, r2->data, data))
        return FALSE;
      r1 = birnet_ring_walk (r1, sorted_ring1);
      r2 = birnet_ring_walk (r2, sorted_ring2);
    }
  return r1 == r2; /* both need to be NULL */
}

gboolean
birnet_ring_mismatch (BirnetRing       **sorted_ring1_p,
                   BirnetRing       **sorted_ring2_p,
                   BirnetCompareFunc  cmp,
                   gpointer        data)
{
  BirnetRing *head1 = *sorted_ring1_p, *head2 = *sorted_ring2_p;
  BirnetRing *r1 = head1, *r2 = head2;
  while (r1 && r2)
    {
      if (cmp (r1->data, r2->data, data))
        goto mismatch;
      r1 = birnet_ring_walk (r1, head1);
      r2 = birnet_ring_walk (r2, head2);
    }
  if (r1 == r2) /* both are NULL */
    return FALSE;
 mismatch:
  *sorted_ring1_p = r1;
  *sorted_ring2_p = r2;
  return TRUE;
}

BirnetRing*
birnet_ring_min_node (const BirnetRing  *head,
                   BirnetCompareFunc  cmp,
                   gpointer        data)
{
  const BirnetRing *ring = head, *last = NULL;
  if (ring)
    {
      last = ring;
      for (ring = birnet_ring_walk (ring, head); ring; ring = birnet_ring_walk (ring, head))
        if (cmp (last->data, ring->data, data) > 0)
          last = ring;
    }
  return (BirnetRing*) last;
}

BirnetRing*
birnet_ring_max_node (const BirnetRing  *head,
                   BirnetCompareFunc  cmp,
                   gpointer        data)
{
  const BirnetRing *ring = head, *last = NULL;
  if (ring)
    {
      last = ring;
      for (ring = birnet_ring_walk (ring, head); ring; ring = birnet_ring_walk (ring, head))
        if (cmp (last->data, ring->data, data) < 0)
          last = ring;
    }
  return (BirnetRing*) last;
}

gpointer
birnet_ring_min (const BirnetRing  *head,
              BirnetCompareFunc  cmp,
              gpointer        data)
{
  BirnetRing *ring = birnet_ring_min_node (head, cmp, data);
  return ring ? ring->data : NULL;
}

gpointer
birnet_ring_max (const BirnetRing  *head,
              BirnetCompareFunc  cmp,
              gpointer        data)
{
  BirnetRing *ring = birnet_ring_max_node (head, cmp, data);
  return ring ? ring->data : NULL;
}
