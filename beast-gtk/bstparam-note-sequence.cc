// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstsequence.hh"
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
