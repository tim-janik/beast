// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gxksimplelabel.hh"
#include "gxkauxwidgets.hh"
/* --- toggle/check button editors --- */
enum {
  PARAM_TOGGLE_CHECKER,
  PARAM_TOGGLE_WITH_LABEL,
};
static void
param_toggle_change_value (GtkWidget *toggle,
			   GxkParam  *param)
{
  if (!param->updating)
    {
      g_value_set_boolean (&param->value, GTK_TOGGLE_BUTTON (toggle)->active);
      gxk_param_apply_value (param);
    }
}
static GtkWidget*
param_toggle_create (GxkParam    *param,
                     const gchar *tooltip,
                     guint        variant)
{
  GtkWidget *widget;
  GType type = GTK_TYPE_CHECK_BUTTON;
  if (variant == PARAM_TOGGLE_WITH_LABEL ||
      g_param_spec_check_option (param->pspec, "trigger"))
    type = GTK_TYPE_TOGGLE_BUTTON;
  else if (g_param_spec_check_option (param->pspec, "radio"))
    type = GXK_TYPE_FREE_RADIO_BUTTON;
  widget = (GtkWidget*) g_object_new (type, "visible", TRUE, NULL);
  g_object_connect (widget,
		    "signal::clicked", param_toggle_change_value, param,
		    NULL);
  gxk_widget_set_tooltip (widget, tooltip);
  if (variant == PARAM_TOGGLE_WITH_LABEL)
    g_object_new (GTK_TYPE_LABEL,
                  "visible", TRUE,
                  "use-underline", FALSE,
                  "label", g_param_spec_get_nick (param->pspec),
                  "parent", widget,
                  NULL);
  else
    g_object_new (GXK_TYPE_SIMPLE_LABEL,
                  "visible", TRUE,
                  "use-underline", FALSE,
                  "label", g_param_spec_get_nick (param->pspec),
                  "parent", widget,
                  "auto-cut", TRUE,
                  NULL);
  return widget;
}
static void
param_toggle_update (GxkParam  *param,
		     GtkWidget *widget)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), g_value_get_boolean (&param->value));
}
static GxkParamEditor param_toggle = {
  { "toggle",           N_("Check/ToggleRadio Button"), },
  { G_TYPE_BOOLEAN, },
  { NULL,         +5,   TRUE, },        /* options, rating, editing */
  param_toggle_create, param_toggle_update, PARAM_TOGGLE_CHECKER,
};
static GxkParamEditor param_toggle_empty = {
  { "toggle+label",     N_("Toggle Button"), },
  { G_TYPE_BOOLEAN, },
  { NULL,         +4,   TRUE, },        /* options, rating, editing */
  param_toggle_create, param_toggle_update, PARAM_TOGGLE_WITH_LABEL,
};
