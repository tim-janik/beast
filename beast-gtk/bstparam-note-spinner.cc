// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
/* --- note parameter editors --- */
static gint
param_note_spinner_input (GtkSpinButton *spin_button,
                          gdouble       *svalue,
                          GxkParam      *param)
{
  const gchar *string = gtk_entry_get_text (GTK_ENTRY (spin_button));
  gchar *error;
  *svalue = sfi_note_from_string_err (string, &error);
  g_free (error);
  return error ? GTK_INPUT_ERROR : TRUE;
}
static gint
param_note_spinner_output (GtkSpinButton *spin_button,
                           GxkParam      *param)
{
  gchar *string = sfi_note_to_string (spin_button->adjustment->value);
  gxk_param_entry_set_text (param, GTK_WIDGET (spin_button), string);
  g_free (string);
  return TRUE;
}
static GtkWidget*
param_note_spinner_create (GxkParam    *param,
                           const gchar *tooltip,
                           guint        variant)
{
  GtkAdjustment *adjustment = gxk_param_get_adjustment (param);
  GtkWidget *widget = gtk_spin_button_new (adjustment, 0, 0);
  gxk_param_add_grab_widget (param, widget);
  g_object_set (widget,
                "visible", TRUE,
                "activates_default", TRUE,
                "width_chars", 0,
                NULL);
  gxk_widget_add_font_requisition (widget, 4+1, 1+1);
  gxk_param_entry_connect_handlers (param, widget, NULL);
  g_object_connect (widget,
                    "signal::input", param_note_spinner_input, param,
                    "signal::output", param_note_spinner_output, param,
                    NULL);
  gxk_widget_set_tooltip (widget, tooltip);
  return widget;
}
static void
param_note_spinner_update (GxkParam  *param,
                           GtkWidget *widget)
{
  /* contents are updated through the adjustment */
  gtk_editable_set_editable (GTK_EDITABLE (widget), param->editable);
}
static GxkParamEditor param_note_spinner = {
  { "note-spinner",     N_("Note Entry"), },
  { G_TYPE_INT, },
  { "note",             +10,    TRUE, },        /* options, rating, editing */
  param_note_spinner_create, param_note_spinner_update,
};
