/* BSW - Bedevilled Sound Engine Wrapper
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
#include "bswcommon.h"

#include "../PKG_config.h"
#include "bswprivate.h"
#include "gslcommon.h"
#include "bseplugin.h"
#include "bseglue.h"
#include "bsescripthelper.h"
#include <string.h>


#define	ITER_ITEMS_PREALLOC	(8)


/* --- BSW iterator base --- */
typedef union
{
  gulong   v_ulong;
  gfloat   v_float;
  gchar   *v_string;
  gpointer v_pointer;
} BswIterItem;
typedef BswIterItem (*BswIterCopyItem)	(const BswIterItem	item);
typedef void	    (*BswIterFreeItem)	(BswIterItem		item);
typedef struct
{
  BswIterCopyItem copy;
  BswIterFreeItem free;
} BswIterFuncs;
struct _BswIter
{
  GType        type;
  guint        n_items;
  guint        pos;
  BswIterItem *items;
  guint        prealloc;
};

static GQuark iter_func_quark = 0;

static gpointer
iter_copy (gpointer boxed)
{
  return boxed ? bsw_iter_copy (boxed) : NULL;
}

static void
iter_free (gpointer boxed)
{
  if (boxed)
    bsw_iter_free (boxed);
}

static GType
bsw_iter_make_type (const gchar      *type_name,
		    BswIterFuncs     *funcs,
		    BseGlueBoxedToSeq b2seq)
{
  GType type;

  g_return_val_if_fail (strncmp ("BswIter", type_name, 7) == 0, 0);
  g_return_val_if_fail (funcs != NULL, 0);

  type = bse_glue_make_rosequence (type_name, iter_copy, iter_free, b2seq);

  if (!iter_func_quark)
    iter_func_quark = g_quark_from_static_string ("BswIterFuncs");

  g_type_set_qdata (type, iter_func_quark, funcs);

  return type;
}

static BswIterFuncs*
bsw_iter_funcs (GType type)
{
  return g_type_get_qdata (type, iter_func_quark);
}

static inline guint
bsw_iter_grow1 (BswIter *iter)
{
  guint i;

  i = iter->n_items++;
  if (iter->n_items > iter->prealloc)
    {
      iter->prealloc += ITER_ITEMS_PREALLOC;
      iter->items = g_realloc (iter->items, sizeof (iter->items[0]) * iter->prealloc);
      memset (iter->items + i, 0, sizeof (iter->items[0]) * (iter->prealloc - i));
    }
  return i;
}

gboolean
bsw_iter_check_type (GType type)
{
  BswIterFuncs *funcs = bsw_iter_funcs (type);
  gchar *name = g_type_name (type);

  return funcs && strncmp ("BswIter", name, 7) == 0;
}

BswIter*
bsw_iter_create (GType type,
		 guint prealloc)
{
  BswIter *iter;

  g_return_val_if_fail (bsw_iter_check_type (type), NULL);

  iter = gsl_new_struct (BswIter, 1);
  iter->type = type;

  iter->n_items = 0;
  iter->prealloc = prealloc;
  iter->pos = 0;

  iter->items = g_malloc0 (sizeof (iter->items[0]) * iter->prealloc);

  return iter;
}

BswIter*
bsw_iter_copy (BswIter *iter)
{
  BswIterFuncs *funcs;
  BswIter *diter;
  guint i;

  g_return_val_if_fail (BSW_IS_ITER (iter), NULL);

  funcs = bsw_iter_funcs (iter->type);
  diter = bsw_iter_create (iter->type, iter->n_items);
  if (funcs->copy)
    for (i = 0; i < iter->n_items; i++)
      diter->items[i] = funcs->copy (iter->items[i]);
  else
    for (i = 0; i < iter->n_items; i++)
      memcpy (&diter->items[i], &iter->items[i], sizeof (iter->items[i]));
  diter->n_items = iter->n_items;

  return diter;
}

void
bsw_iter_free (BswIter *iter)
{
  BswIterFuncs *funcs;
  guint i;

  g_return_if_fail (BSW_IS_ITER (iter));

  funcs = bsw_iter_funcs (iter->type);
  if (funcs->free)
    for (i = 0; i < iter->n_items; i++)
      funcs->free (iter->items[i]);
  g_free (iter->items);
  iter->type = 0;
  gsl_delete_struct (BswIter, iter);
}

void
bsw_iter_rewind (BswIter *iter)
{
  g_return_if_fail (BSW_IS_ITER (iter));

  iter->pos = 0;
}

guint
bsw_iter_n_left (BswIter *iter)
{
  g_return_val_if_fail (BSW_IS_ITER (iter), 0);

  return iter->n_items - iter->pos;
}

void
bsw_iter_next (BswIter *iter)
{
  g_return_if_fail (BSW_IS_ITER (iter));
  g_return_if_fail (iter->pos < iter->n_items);

  iter->pos++;
}

void
bsw_iter_prev (BswIter *iter)
{
  g_return_if_fail (BSW_IS_ITER (iter));
  g_return_if_fail (iter->pos > 0);

  iter->pos--;
}

void
bsw_iter_jump (BswIter *iter,
	       guint    nth)
{
  g_return_if_fail (BSW_IS_ITER (iter));
  g_return_if_fail (nth <= iter->n_items);

  iter->pos = nth;
}

gboolean
bsw_iter_check (const BswIter *iter)
{
  return iter != NULL && bsw_iter_check_type (iter->type);
}

gboolean
bsw_iter_check_is_a (const BswIter *iter,
		     GType          type)
{
  return bsw_iter_check (iter) && g_type_is_a (iter->type, type);
}


/* --- discrete iterators --- */
static GslGlueSeq*
iter_int_to_sequence (gpointer boxed)
{
  BswIterInt *iter = boxed;
  GslGlueSeq *seq = gsl_glue_seq ();

  bsw_iter_rewind (iter);
  for (; bsw_iter_n_left (iter); bsw_iter_next (iter))
    {
      GslGlueValue val = gsl_glue_value_int (bsw_iter_get_int (iter));
      gsl_glue_seq_take_append (seq, &val);
    }
  return seq;
}

GType
bsw_iter_int_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static BswIterFuncs funcs = {
	(BswIterCopyItem) NULL,
	(BswIterFreeItem) NULL,
      };
      type = bsw_iter_make_type ("BswIterInt", &funcs, iter_int_to_sequence);
    }
  return type;
}

gint
bsw_iter_get_int (BswIterInt *iter)
{
  g_return_val_if_fail (BSW_IS_ITER_INT (iter), 0);
  g_return_val_if_fail (iter->pos < iter->n_items, 0);

  return iter->items[iter->pos].v_ulong;
}

static BswIterItem
iter_string_copy (const BswIterItem item)
{
  BswIterItem ditem;
  ditem.v_string = g_strdup (item.v_string);
  return ditem;
}

static void
iter_string_free (BswIterItem item)
{
  g_free (item.v_string);
}

static GslGlueSeq*
iter_string_to_sequence (gpointer boxed)
{
  BswIterString *iter = boxed;
  GslGlueSeq *seq = gsl_glue_seq ();

  bsw_iter_rewind (iter);
  for (; bsw_iter_n_left (iter); bsw_iter_next (iter))
    {
      GslGlueValue val = gsl_glue_value_string (bsw_iter_get_string (iter));
      gsl_glue_seq_take_append (seq, &val);
    }
  return seq;
}

GType
bsw_iter_string_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static BswIterFuncs funcs = {
	iter_string_copy,
	iter_string_free,
      };
      type = bsw_iter_make_type ("BswIterString", &funcs, iter_string_to_sequence);
    }
  return type;
}

const gchar*
bsw_iter_get_string (BswIterString *iter)
{
  g_return_val_if_fail (BSW_IS_ITER_STRING (iter), NULL);
  g_return_val_if_fail (iter->pos < iter->n_items, NULL);

  return iter->items[iter->pos].v_string;
}

void
bsw_iter_add_string_take_ownership (BswIterString *iter,
				    gchar         *string)
{
  guint i;

  g_return_if_fail (BSW_IS_ITER_STRING (iter));

  i = bsw_iter_grow1 (iter);
  iter->items[i].v_string = string;
}


/* --- BSW proxy --- */
static void
value_transform_proxy_object (const GValue *src_value,
			      GValue       *dest_value)
{
  BswProxy proxy = bsw_value_get_proxy (src_value);

  g_value_set_object (dest_value, proxy ? bse_object_from_id (proxy) : NULL);
}

static void
value_transform_object_proxy (const GValue *src_value,
			      GValue       *dest_value)
{
  BseObject *object = g_value_get_object (src_value);

  bsw_value_set_proxy (dest_value, object ? BSE_OBJECT_ID (object) : 0);
}

GType
bsw_proxy_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      type = g_pointer_type_register_static ("BswProxy");
      g_value_register_transform_func (BSW_TYPE_PROXY, BSE_TYPE_OBJECT, value_transform_proxy_object);
      g_value_register_transform_func (BSE_TYPE_OBJECT, BSW_TYPE_PROXY, value_transform_object_proxy);
    }
  return type;
}

void
bsw_value_set_proxy (GValue  *value,
		     BswProxy proxy)
{
  g_return_if_fail (BSW_VALUE_HOLDS_PROXY (value));

  value->data[0].v_pointer = (gpointer) proxy;
}

BswProxy
bsw_value_get_proxy (const GValue *value)
{
  g_return_val_if_fail (BSW_VALUE_HOLDS_PROXY (value), 0);

  return (BswProxy) value->data[0].v_pointer;
}

GParamSpec*
bsw_param_spec_proxy (const gchar *name,
		      const gchar *nick,
		      const gchar *blurb,
		      GParamFlags  flags)
{
  GParamSpec *pspec = g_param_spec_pointer (name, nick, blurb, flags);

  pspec->value_type = BSW_TYPE_PROXY;

  return pspec;
}

static GslGlueSeq*
iter_proxy_to_sequence (gpointer boxed)
{
  BswIterProxy *iter = boxed;
  GslGlueSeq *seq = gsl_glue_seq ();

  bsw_iter_rewind (iter);
  for (; bsw_iter_n_left (iter); bsw_iter_next (iter))
    {
      GslGlueValue val = gsl_glue_value_proxy (bsw_iter_get_proxy (iter));
      gsl_glue_seq_take_append (seq, &val);
    }
  return seq;
}

GType
bsw_iter_proxy_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static BswIterFuncs funcs = {
        (BswIterCopyItem) NULL,
	(BswIterFreeItem) NULL,
      };
      type = bsw_iter_make_type ("BswIterProxy", &funcs, iter_proxy_to_sequence);
    }
  return type;
}

BswProxy
bsw_iter_get_proxy (BswIterProxy *iter)
{
  g_return_val_if_fail (BSW_IS_ITER_PROXY (iter), 0);
  g_return_val_if_fail (iter->pos < iter->n_items, 0);

  return iter->items[iter->pos].v_ulong;
}

void
bsw_iter_add_proxy (BswIterProxy *iter,
		    BswProxy      proxy)
{
  guint i;

  g_return_if_fail (BSW_IS_ITER_PROXY (iter));

  i = bsw_iter_grow1 (iter);
  iter->items[i].v_ulong = proxy;
}


/* --- BSW Part Note --- */
static gpointer
part_note_copy (gpointer boxed)
{
  BswPartNote *pnote = NULL;

  if (boxed)
    {
      pnote = gsl_new_struct (BswPartNote, 1);

      memcpy (pnote, boxed, sizeof (*pnote));
    }
  return pnote;
}

void
bsw_part_note_free (BswPartNote *pnote)
{
  if (pnote)
    gsl_delete_struct (BswPartNote, pnote);
}

static GslGlueRec*
part_note_to_record (gpointer crecord)
{
  BswPartNote *note = crecord;
  GslGlueValue val;
  GslGlueRec *rec;

  rec = gsl_glue_rec ();
  val = gsl_glue_value_int (note->tick);	// FIXME: uint
  gsl_glue_rec_take (rec, "tick", &val);
  val = gsl_glue_value_int (note->duration);	// FIXME: uint
  gsl_glue_rec_take (rec, "duration", &val);
  val = gsl_glue_value_float (note->freq);
  gsl_glue_rec_take (rec, "frequency", &val);
  val = gsl_glue_value_float (note->velocity);
  gsl_glue_rec_take (rec, "velocity", &val);
  val = gsl_glue_value_bool (note->selected);
  gsl_glue_rec_take (rec, "selected", &val);
  return rec;
}

GType
bsw_part_note_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = bse_glue_make_rorecord ("BswPartNote", part_note_copy, (GBoxedFreeFunc) bsw_part_note_free,
				   part_note_to_record);
  return type;
}

BswPartNote*
bsw_part_note (guint    tick,
	       guint    duration,
	       gfloat   freq,
	       gfloat   velocity,
	       gboolean selected)
{
  BswPartNote *pnote = gsl_new_struct (BswPartNote, 1);

  pnote->tick = tick;
  pnote->freq = freq;
  pnote->duration = duration;
  pnote->velocity = velocity;
  pnote->selected = selected != FALSE;

  return pnote;
}

static BswIterItem
iter_part_note_copy (const BswIterItem item)
{
  BswIterItem ditem;
  ditem.v_pointer = item.v_pointer ? g_boxed_copy (BSW_TYPE_PART_NOTE, item.v_pointer) : NULL;
  return ditem;
}

static void
iter_part_note_free (BswIterItem item)
{
  if (item.v_pointer)
    g_boxed_free (BSW_TYPE_PART_NOTE, item.v_pointer);
}

static GslGlueSeq*
iter_part_note_to_sequence (gpointer boxed)
{
  BswIterPartNote *iter = boxed;
  GslGlueSeq *seq = gsl_glue_seq ();

  bsw_iter_rewind (iter);
  for (; bsw_iter_n_left (iter); bsw_iter_next (iter))
    {
      GslGlueValue val = bse_glue_boxed_to_value (BSW_TYPE_PART_NOTE, bsw_iter_get_part_note (iter));
      gsl_glue_seq_take_append (seq, &val);
    }
  return seq;
}

GType
bsw_iter_part_note_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static BswIterFuncs funcs = {
	iter_part_note_copy,
	iter_part_note_free,
      };
      type = bsw_iter_make_type ("BswIterPartNote", &funcs, iter_part_note_to_sequence);
    }
  return type;
}

BswPartNote*
bsw_iter_get_part_note (BswIterPartNote *iter)
{
  g_return_val_if_fail (BSW_IS_ITER_PART_NOTE (iter), NULL);
  g_return_val_if_fail (iter->pos < iter->n_items, NULL);

  return iter->items[iter->pos].v_pointer;
}

void
bsw_iter_add_part_note_take_ownership (BswIterPartNote *iter,
				       BswPartNote     *pnote)
{
  guint i;

  g_return_if_fail (BSW_IS_ITER_PART_NOTE (iter));

  i = bsw_iter_grow1 (iter);
  iter->items[i].v_pointer = pnote;
}


/* -- BSW Note Description --- */
static gpointer
note_description_copy (gpointer boxed)
{
  BswNoteDescription *info = NULL;

  if (boxed)
    {
      info = gsl_new_struct (BswNoteDescription, 1);

      memcpy (info, boxed, sizeof (*info));
      info->name = g_strdup (info->name);
    }
  return info;
}

void
bsw_note_description_free (BswNoteDescription *info)
{
  if (info)
    {
      g_free (info->name);
      gsl_delete_struct (BswNoteDescription, info);
    }
}

static GslGlueRec*
note_description_to_glue_rec (gpointer crecord)
{
  BswNoteDescription *info = crecord;
  GslGlueValue val;
  GslGlueRec *rec;

  rec = gsl_glue_rec ();
  /* note */
  val = gsl_glue_value_int (info->note);
  gsl_glue_rec_take (rec, "note", &val);
  /* octave */
  val = gsl_glue_value_int (info->octave);
  gsl_glue_rec_take (rec, "octave", &val);
  /* freq */
  val = gsl_glue_value_float (info->freq);
  gsl_glue_rec_take (rec, "frequency", &val);
  /* fine_tune */
  val = gsl_glue_value_int (info->fine_tune);
  gsl_glue_rec_take (rec, "fine_tune", &val);
  /* half_tone */
  val = gsl_glue_value_int (info->half_tone);
  gsl_glue_rec_take (rec, "half_tone", &val);
  /* upshift */
  val = gsl_glue_value_bool (info->upshift);
  gsl_glue_rec_take (rec, "upshift", &val);
  /* letter */
  val = gsl_glue_value_int (info->letter);
  gsl_glue_rec_take (rec, "letter", &val);
  /* name */
  val = gsl_glue_value_string (info->name);
  gsl_glue_rec_take (rec, "name", &val);
  /* max_fine_tune */
  val = gsl_glue_value_int (info->max_fine_tune);
  gsl_glue_rec_take (rec, "max_fine_tune", &val);
  /* kammer_note */
  val = gsl_glue_value_int (info->kammer_note);
  gsl_glue_rec_take (rec, "kammer_note", &val);
  
  return rec;
}

GType
bsw_note_description_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = bse_glue_make_rorecord ("BswNoteDescription",
				   note_description_copy,
				   (GBoxedFreeFunc) bsw_note_description_free,
				   note_description_to_glue_rec);
  return type;
}

BswNoteDescription*
bsw_note_description (guint note,
		      gint  fine_tune)
{
  BswNoteDescription *info = gsl_new_struct (BswNoteDescription, 1);

  if (note >= BSE_MIN_NOTE && note <= BSE_MAX_NOTE)
    {
      info->note = note;
      bse_note_examine (info->note,
			&info->octave,
			&info->half_tone,
			&info->upshift,
			&info->letter);
      info->fine_tune = CLAMP (fine_tune, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE);
      info->freq = bse_note_to_tuned_freq (info->note, info->fine_tune);
      info->name = bse_note_to_string (info->note);
      info->max_fine_tune = BSE_MAX_FINE_TUNE;
      info->kammer_note = BSE_KAMMER_NOTE;
    }
  else
    {
      memset (info, 0, sizeof (info));
      info->note = BSE_NOTE_VOID;
      info->name = NULL;
      info->max_fine_tune = BSE_MAX_FINE_TUNE;
      info->kammer_note = BSE_KAMMER_NOTE;
    }
  return info;
}


/* --- BSW Note Sequence --- */
static gpointer
note_sequence_copy (gpointer boxed)
{
  return boxed ? bsw_note_sequence_copy (boxed) : NULL;
}

static void
note_sequence_free (gpointer boxed)
{
  if (boxed)
    bsw_note_sequence_free (boxed);
}

GType
bsw_note_sequence_get_type (void)
{
  static GType type;

  if (!type)
    type = g_boxed_type_register_static ("BswNoteSequence",
					 note_sequence_copy,
					 note_sequence_free);
  return type;
}

BswNoteSequence*
bsw_note_sequence_new (guint n_notes)
{
  BswNoteSequence *seq;
  guint i;

  seq = g_malloc (sizeof (BswNoteSequence) + sizeof (seq->notes[0]) * (MAX (n_notes, 1) - 1));
  seq->offset = BSE_KAMMER_NOTE;
  seq->n_notes = n_notes;
  for (i = 0; i < seq->n_notes; i++)
    seq->notes[i].note = BSE_NOTE_VOID;

  return seq;
}

BswNoteSequence*
bsw_note_sequence_copy (const BswNoteSequence *seq)
{
  guint size;

  g_return_val_if_fail (seq != NULL, NULL);

  size = sizeof (BswNoteSequence) + sizeof (seq->notes[0]) * (MAX (seq->n_notes, 1) - 1);

  return g_memdup (seq, size);
}

void
bsw_note_sequence_free (BswNoteSequence *seq)
{
  g_return_if_fail (seq != NULL);

  g_free (seq);
}

BswNoteSequence*
bsw_note_sequence_resize (BswNoteSequence *seq,
			  guint            n_notes)
{
  guint size, i;

  g_return_val_if_fail (seq != NULL, seq);

  i = seq->n_notes;
  seq->n_notes = n_notes;
  size = sizeof (BswNoteSequence) + sizeof (seq->notes[0]) * (MAX (seq->n_notes, 1) - 1);
  seq = g_realloc (seq, size);
  for (; i < seq->n_notes; i++)
    seq->notes[i].note = BSE_NOTE_VOID;
  return seq;
}


/* --- BSW value block --- */
static gpointer
vblock_copy (gpointer boxed)
{
  return boxed ? bsw_value_block_ref (boxed) : NULL;
}

static void
vblock_free (gpointer boxed)
{
  if (boxed)
    bsw_value_block_unref (boxed);
}

GType
bsw_value_block_get_type (void)
{
  static GType type = 0;
  if (!type)
    type = g_boxed_type_register_static ("BswValueBlock", vblock_copy, vblock_free);
  return type;
}

BswValueBlock*
bsw_value_block_new (guint n_values)
{
  BswValueBlock *vblock = g_malloc0 (sizeof (BswValueBlock) + sizeof (vblock->values[0]) * (MAX (n_values, 1) - 1));

  vblock->ref_count = 1;
  vblock->n_values = n_values;

  return vblock;
}

BswValueBlock*
bsw_value_block_ref (BswValueBlock *vblock)
{
  g_return_val_if_fail (vblock != NULL, NULL);
  g_return_val_if_fail (vblock->ref_count > 0, NULL);

  vblock->ref_count++;

  return vblock;
}

void
bsw_value_block_unref (BswValueBlock *vblock)
{
  g_return_if_fail (vblock != NULL);
  g_return_if_fail (vblock->ref_count > 0);

  vblock->ref_count--;
  if (!vblock->ref_count)
    g_free (vblock);
}


/* --- BSW Icon --- */
#define STATIC_REF_COUNT (1 << 31)

BswIcon*
bsw_icon_ref_static (BswIcon *icon)
{
  g_return_val_if_fail (icon != NULL, NULL);
  g_return_val_if_fail (icon->ref_count > 0, NULL);

  icon->ref_count |= STATIC_REF_COUNT;

  return icon;
}

BswIcon*
bsw_icon_ref (BswIcon *icon)
{
  g_return_val_if_fail (icon != NULL, NULL);
  g_return_val_if_fail (icon->ref_count > 0, NULL);

  if (!(icon->ref_count & STATIC_REF_COUNT))
    icon->ref_count += 1;

  return icon;
}

void
bsw_icon_unref (BswIcon *icon)
{
  g_return_if_fail (icon != NULL);
  g_return_if_fail (icon->ref_count > 0);

  if (!(icon->ref_count & STATIC_REF_COUNT))
    {
      icon->ref_count -= 1;
      if (!icon->ref_count)
	{
	  g_free (icon->pixels);
	  g_free (icon);
	}
    }
}


/* --- initialize scripts and plugins --- */
static void
handle_message (GString *gstring,
		gpointer captured)
{
  if (!captured && gstring->len)
    {
      g_message ("%s", gstring->str);
      g_string_set_size (gstring, 0);
    }
}

void
bsw_register_plugins (const gchar *path,
		      gboolean     verbose,
		      gchar      **messages)
{
  GSList *free_list, *list;
  GString *gstring = g_string_new (NULL);

  if (!path)
    path = BSE_PATH_PLUGINS;

  free_list = bse_plugin_dir_list_files (path);
  for (list = free_list; list; list = list->next)
    {
      gchar *string = list->data;
      const gchar *error;

      if (verbose)
	g_string_printfa (gstring, "register plugin \"%s\"...", string);
      handle_message (gstring, messages);
      error = bse_plugin_check_load (string);
      if (error)
	g_string_printfa (gstring, "error encountered loading plugin \"%s\": %s", string, error);
      handle_message (gstring, messages);
      g_free (string);
    }
  g_slist_free (free_list);
  if (!free_list && verbose)
    {
      g_string_printfa (gstring, "strange, can't find any plugins, please check %s", path);
      handle_message (gstring, messages);
    }
  if (messages && gstring->len)
    *messages = g_string_free (gstring, FALSE);
  else
    {
      if (messages)
	*messages = NULL;
      g_string_free (gstring, TRUE);
    }
}

void
bsw_register_scripts (const gchar *path,
		      gboolean     verbose,
		      gchar      **messages)
{
  GSList *free_list, *list;
  GString *gstring = g_string_new (NULL);

  if (!path)
    path = BSW_PATH_SCRIPTS;

  free_list = bse_script_dir_list_files (path);
  for (list = free_list; list; list = list->next)
    {
      gchar *string = list->data;
      const gchar *error;

      if (verbose)
	g_string_printfa (gstring, "register script \"%s\"...", string);
      handle_message (gstring, messages);
      error = bse_script_file_register (string);
      if (error)
	g_string_printfa (gstring, "error encountered loading script \"%s\": %s", string, error);
      handle_message (gstring, messages);
      g_free (string);
    }
  g_slist_free (free_list);
  if (!free_list && verbose)
    {
      g_string_printfa (gstring, "strange, can't find any scripts, please check %s", path);
      handle_message (gstring, messages);
    }
  if (messages && gstring->len)
    *messages = g_string_free (gstring, FALSE);
  else
    {
      if (messages)
	*messages = NULL;
      g_string_free (gstring, TRUE);
    }
}


/* --- missing GLib --- */
static inline gchar
check_lower (gchar c)
{
  return c >= 'a' && c <= 'z';
}

static inline gchar
check_upper (gchar c)
{
  return c >= 'A' && c <= 'Z';
}

static inline gchar
char_convert (gchar    c,
	      gchar    fallback,
	      gboolean want_upper)
{
  if (c >= '0' && c <= '9')
    return c;
  if (want_upper)
    {
      if (check_lower (c))
	return c - 'a' + 'A';
      else if (check_upper (c))
	return c;
    }
  else
    {
      if (check_upper (c))
	return c - 'A' + 'a';
      else if (check_lower (c))
	return c;
    }
  return fallback;
}

static gchar*
type_name_to_cname (const gchar *type_name,
		    const gchar *insert,
		    gchar        fallback,
		    gboolean     want_upper)
{
  const gchar *s;
  gchar *result, *p;
  guint was_upper, ilen;

  s = type_name;

  /* special casing for GLib types */
  if (strcmp (s, "GString") == 0)
    s = "GGString";	/* G_TYPE_GSTRING */
  else if (check_lower (s[0]))
    {
      static const struct { gchar *gname; gchar *xname; } glib_ftypes[] = {
	{ "gboolean",	"GBoolean" },
	{ "gchar",	"GChar" },
	{ "guchar",	"GUChar" },
	{ "gint",	"GInt" },
	{ "guint",	"GUInt" },
	{ "glong",	"GLong" },
	{ "gulong",	"GULong" },
	{ "gint64",	"GInt64" },
	{ "guint64",	"GUInt64" },
	{ "gfloat",	"GFloat" },
	{ "gdouble",	"GDouble" },
	{ "gpointer",	"GPointer" },
	{ "gchararray",	"GString" },	/* G_TYPE_STRING */
      };
      guint i;

      for (i = 0; i < G_N_ELEMENTS (glib_ftypes); i++)
	if (strcmp (s, glib_ftypes[i].gname) == 0)
	  {
	    s = glib_ftypes[i].xname;
	    break;
	  }
    }

  ilen = strlen (insert);
  result = g_new (gchar, strlen (s) * 2 + ilen + 1);
  p = result;

  *p++ = char_convert (*s++, fallback, want_upper);
  while (*s && !check_upper (*s))
    *p++ = char_convert (*s++, fallback, want_upper);
  strcpy (p, insert);
  p += ilen;
  was_upper = 0;
  while (*s)
    {
      if (check_upper (*s))
	{
	  if (!was_upper || (s[1] && check_lower (s[1]) && was_upper >= 2))
	    *p++ = fallback;
	  was_upper++;
	}
      else
	was_upper = 0;
      *p++ = char_convert (*s, fallback, want_upper);
      s++;
    }
  *p++ = 0;

  return result;
}

gchar*
g_type_name_to_cname (const gchar *type_name)
{
  g_return_val_if_fail (type_name != NULL, NULL);

  return type_name_to_cname (type_name, "", '_', FALSE);
}

gchar*
g_type_name_to_sname (const gchar *type_name)
{
  g_return_val_if_fail (type_name != NULL, NULL);

  return type_name_to_cname (type_name, "", '-', FALSE);
}

gchar*
g_type_name_to_cupper (const gchar *type_name)
{
  g_return_val_if_fail (type_name != NULL, NULL);

  return type_name_to_cname (type_name, "", '_', TRUE);
}

gchar*
g_type_name_to_type_macro (const gchar *type_name)
{
  g_return_val_if_fail (type_name != NULL, NULL);

  return type_name_to_cname (type_name, "_TYPE", '_', TRUE);
}

gchar*
bsw_type_name_to_sname (const gchar *type_name)
{
  gchar *name = g_type_name_to_sname (type_name);

  if (name && name[0] == 'b' && name[1] == 's' && name[2] == 'e')
    name[2] = 'w';

  return name;
}
