/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */


/* --- string parameters --- */
static void
param_note_sequence_change_value (GtkWidget *swidget,
				  BstParam  *bparam)
{
  if (!bparam->updating)
    {
      sfi_value_take_rec (&bparam->value, bse_note_sequence_to_rec (BST_SEQUENCE (swidget)->sdata));
      bst_param_apply_value (bparam);
    }
}

static BstGMask*
param_note_sequence_create_gmask (BstParam    *bparam,
				  const gchar *tooltip,
				  GtkWidget   *gmask_parent)
{
  GtkWidget *action, *prompt, *xframe;
  BstGMask *gmask;

  action = g_object_new (BST_TYPE_SEQUENCE,
			 "visible", TRUE,
			 NULL);
  g_object_connect (action,
		    "signal::seq-changed", param_note_sequence_change_value, bparam,
		    NULL);
  xframe = g_object_new (BST_TYPE_XFRAME,
			 "visible", TRUE,
			 NULL);
  g_object_connect (xframe,
		    "swapped_signal::button_check", bst_param_xframe_check_button, bparam,
		    NULL);
  prompt = g_object_new (GTK_TYPE_LABEL,
			 "visible", TRUE,
                         "label", g_param_spec_get_nick (bparam->pspec),
			 "xalign", 0.0,
			 "parent", xframe,
			 NULL);
  gmask = bst_gmask_form_big (gmask_parent, action);
  bst_gmask_set_prompt (gmask, prompt);
  bst_gmask_set_tip (gmask, tooltip);

  return gmask;
}

static GtkWidget*
param_note_sequence_create_widget (BstParam    *bparam,
				   const gchar *tooltip)
{
  GtkWidget *action = g_object_new (BST_TYPE_SEQUENCE,
				    "visible", TRUE,
				    NULL);
  g_object_connect (action,
		    "signal::seq-changed", param_note_sequence_change_value, bparam,
		    NULL);
  return action;
}

static void
param_note_sequence_update (BstParam  *bparam,
			    GtkWidget *action)
{
  BseNoteSequence *nseq = bse_note_sequence_from_rec (sfi_value_get_rec (&bparam->value));
  bst_sequence_set_seq (BST_SEQUENCE (action), nseq);
  bse_note_sequence_free (nseq);
}

struct _BstParamImpl param_note_sequence = {
  "NoteSequence",	+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_REC,		"note-sequence" /* hints */,
  param_note_sequence_create_gmask,
  NULL, /* create_widget */
  param_note_sequence_update,
};

struct _BstParamImpl rack_note_sequence = {
  "NoteSequence",	+5 /* rating */,
  0 /* variant */,	BST_PARAM_EDITABLE,
  SFI_SCAT_REC,		"note-sequence" /* hints */,
  NULL, /* create_gmask */
  param_note_sequence_create_widget,
  param_note_sequence_update,
};
