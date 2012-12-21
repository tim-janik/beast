/* BEAST - Better Audio System
 * Copyright (C) 2002-2003 Tim Janik
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
#include "bstsequence.h"

/* --- string parameters --- */
static void
param_note_sequence_changed (GtkWidget *swidget,
                             GxkParam  *param)
{
  if (!param->updating)
    {
      sfi_value_take_rec (&param->value, bse_note_sequence_to_rec (BST_SEQUENCE (swidget)->sdata));
      gxk_param_apply_value (param);
    }
}

static GtkWidget*
param_note_sequence_create (GxkParam    *param,
                            const gchar *tooltip,
                            guint        variant)
{
  GtkWidget *widget = (GtkWidget*) g_object_new (BST_TYPE_SEQUENCE, NULL);
  g_object_connect (widget,
		    "signal::seq-changed", param_note_sequence_changed, param,
		    NULL);
  gxk_widget_set_tooltip (widget, tooltip);
  gxk_widget_add_option (widget, "hexpand", "+");
  gxk_widget_add_option (widget, "vexpand", "+");
  return widget;
}

static void
param_note_sequence_update (GxkParam  *param,
			    GtkWidget *widget)
{
  SfiRec *rec = sfi_value_get_rec (&param->value);
  if (rec)
    {
      BseNoteSequence *nseq = bse_note_sequence_from_rec (sfi_value_get_rec (&param->value));
      bst_sequence_set_seq (BST_SEQUENCE (widget), nseq);
      bse_note_sequence_free (nseq);
    }
}

static GxkParamEditor param_note_sequence = {
  { "note-sequence",    N_("Note Sequence Grid Editor"), },
  { G_TYPE_BOXED,       "SfiRec", },
  { "note-sequence",    +5,     TRUE, },        /* options, rating, editing */
  param_note_sequence_create,   param_note_sequence_update,
};
