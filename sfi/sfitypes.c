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
#include "sfitypes.h"
#include "sfivalues.h"
#include "sfiparams.h"
#include "sfiprimitives.h"
#include "sfitime.h"



/* --- variables --- */
static GQuark quark_boxed_sequence = 0;
static GQuark quark_boxed_record = 0;


/* --- functions --- */
void
sfi_init (void)
{
  static gboolean initialized = FALSE;

  if (!initialized)
    {
      initialized = TRUE;

      g_type_init ();
      quark_boxed_sequence = g_quark_from_static_string ("sfi-boxed-sequence-info");
      quark_boxed_record = g_quark_from_static_string ("sfi-boxed-record-info");
      _sfi_init_values ();
      _sfi_init_params ();
      _sfi_init_time ();
    }
}


/* --- boxed types --- */
GType
sfi_boxed_make_record (const SfiBoxedRecordInfo *info,
		       GBoxedCopyFunc            copy,
		       GBoxedFreeFunc            free)
{
  GType btype;

  g_return_val_if_fail (info != NULL && copy != NULL && free != NULL, 0);

  btype = g_boxed_type_register_static (info->name, copy, free);
  g_type_set_qdata (btype, quark_boxed_record, (gpointer) info);
  if (info->rec2boxed)
    g_value_register_transform_func (SFI_TYPE_REC, btype, info->rec2boxed);
  if (info->boxed2rec)
    g_value_register_transform_func (btype, SFI_TYPE_REC, info->boxed2rec);
  return btype;
}

const SfiBoxedRecordInfo*
sfi_boxed_get_record_info (GType boxed_type)
{
  return g_type_get_qdata (boxed_type, quark_boxed_record);
}

GType
sfi_boxed_make_sequence (const SfiBoxedSequenceInfo *info,
			 GBoxedCopyFunc              copy,
			 GBoxedFreeFunc              free)
{
  GType btype;

  g_return_val_if_fail (info != NULL && copy != NULL && free != NULL, 0);

  btype = g_boxed_type_register_static (info->name, copy, free);
  g_type_set_qdata (btype, quark_boxed_sequence, (gpointer) info);
  if (info->seq2boxed)
    g_value_register_transform_func (SFI_TYPE_SEQ, btype, info->seq2boxed);
  if (info->boxed2seq)
    g_value_register_transform_func (btype, SFI_TYPE_SEQ, info->boxed2seq);
  return btype;
}

const SfiBoxedSequenceInfo*
sfi_boxed_get_sequence_info (GType boxed_type)
{
  return g_type_get_qdata (boxed_type, quark_boxed_sequence);
}


/* --- FIXME: hacks! */
void
sfi_set_error (GError       **errorp,
	       GQuark         domain,
	       gint           code,
	       const gchar   *format,
	       ...)
{
  if (errorp && !*errorp)
    {
      gchar *message;
      va_list args;
      va_start (args, format);
      message = g_strdup_vprintf (format, args);
      *errorp = g_error_new_literal (domain, code, message);
      g_free (message);
      va_end (args);
    }
}

static inline gchar
char_canon (gchar c)
{
  if (c >= '0' && c <= '9')
    return c;
  else if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  else if (c >= 'a' && c <= 'z')
    return c;
  else
    return '-';
}

gchar*
sfi_strdup_canon (const gchar *identifier)
{
  gchar *str = g_strdup (identifier);

  if (str)
    {
      gchar *p = str;
      while (*p)
	*p++ = char_canon (*p);
    }
  return str;
}

static inline gboolean
eval_match (const gchar *str1,
	    const gchar *str2)
{
  while (*str1 && *str2)
    {
      guchar s1 = char_canon (*str1++);
      guchar s2 = char_canon (*str2++);
      if (s1 != s2)
	return FALSE;
    }
  return *str1 == 0 && *str2 == 0;
}

gboolean
sfi_choice_match_detailed (const gchar *choice_val1,
			   const gchar *choice_val2,
			   gboolean     l1_ge_l2)
{
  guint l1, l2;

  g_return_val_if_fail (choice_val1 != NULL, FALSE);
  g_return_val_if_fail (choice_val2 != NULL, FALSE);

  l1 = strlen (choice_val1);
  l2 = strlen (choice_val2);
  if (l1_ge_l2 && l1 < l2)
    return FALSE;
  return eval_match (choice_val1 + l1 - MIN (l1, l2), choice_val2 + l2 - MIN (l1, l2));
}

gboolean
sfi_choice_match (const gchar *choice_val1,
		  const gchar *choice_val2)
{
  return sfi_choice_match_detailed (choice_val1, choice_val2, FALSE);
}

static inline gint
consts_rmatch (guint        l1,
	       const gchar *str1,
	       guint        l2,
	       const gchar *str2)
{
  gint i, length = MIN (l1, l2);
  for (i = 1; i <= length; i++)
    {
      gint c1 = str1[l1 - i], c2 = str2[l2 - i];
      if (c1 != c2)
	return c1 > c2 ? +1 : -1;
    }
  return 0; /* missing out the length check here which normal strcmp() does */
}

guint
sfi_constants_get_index (guint               n_consts,
			 const SfiConstants *rsorted_consts,
			 const gchar        *constant)
{
  guint l, offs, order, n_nodes = n_consts;
  gchar *key;
  gint i, cmp;
  
  g_return_val_if_fail (constant != NULL, 0);

  /* canonicalize key */
  l = strlen (constant);
  key = g_new (gchar, l);
  for (offs = 0; offs < l; offs++)
    key[offs] = char_canon (constant[offs]);

  /* perform binary search with chopped tail match */
  offs = 0;
  while (offs < n_nodes)
    {
      i = (offs + n_nodes) >> 1;
      cmp = consts_rmatch (l, key, rsorted_consts[i].name_length, rsorted_consts[i].name);
      if (cmp == 0)
	goto have_match;
      else if (cmp < 0)
	n_nodes = i;
      else /* (cmp > 0) */
	offs = i + 1;
    }
  /* no match */
  g_free (key);
  return 0;

  /* explore neighboured matches and favour early indices */
 have_match:
  offs = i;
  order = rsorted_consts[offs].index;
  /* walk lesser matches */
  for (i = 1; i <= offs; i++)
    if (consts_rmatch (l, key, rsorted_consts[offs - i].name_length, rsorted_consts[offs - i].name) == 0)
      order = MIN (order, rsorted_consts[offs - i].index);
    else
      break;
  /* walk greater matches */
  for (i = 1; offs + i < n_consts; i++)
    if (consts_rmatch (l, key, rsorted_consts[offs + i].name_length, rsorted_consts[offs + i].name) == 0)
      order = MIN (order, rsorted_consts[offs + i].index);
    else
      break;
  g_free (key);
  return order;
}

const gchar*
sfi_constants_get_name (guint               n_consts,
			const SfiConstants *consts,
			guint               index)
{
  guint i;

  for (i = 0; i < n_consts; i++)
    if (consts[i].index == index)
      return consts[i].name;
  return NULL;
}

gint
sfi_constants_rcmp (const gchar *canon_identifier1,
		    const gchar *canon_identifier2)
{
  gint cmp, l1, l2;

  g_return_val_if_fail (canon_identifier1 != NULL, 0);
  g_return_val_if_fail (canon_identifier2 != NULL, 0);

  l1 = strlen (canon_identifier1);
  l2 = strlen (canon_identifier2);
  cmp = consts_rmatch (l1, canon_identifier1, l2, canon_identifier2);
  if (!cmp)	/* fixup missing length check */
    return l1 - l2;
  return cmp;
}
