/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
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


/* --- automation setup editor --- */
#include "bstprocedure.h"

static void
param_automation_dialog_cancel (GxkDialog *dialog)
{
  g_object_set_data (dialog, "beast-GxkParam", NULL);
  gxk_toplevel_delete (GTK_WIDGET (dialog));
}

static void
param_automation_dialog_ok (GxkDialog *dialog)
{
  GxkParam *param = g_object_get_data (dialog, "beast-GxkParam");
  g_object_set_data (dialog, "beast-GxkParam", NULL);
  if (param)
    {
      SfiProxy proxy = bst_param_get_proxy (param);
      GxkParam *param_channel = g_object_get_data (dialog, "GxkParam-automation-channel");
      GxkParam *param_control = g_object_get_data (dialog, "GxkParam-automation-control");
      gint midi_channel = sfi_value_get_int (&param_channel->value);
      gint control_type = bse_midi_signal_type_from_choice (sfi_value_get_choice (&param_control->value));
      bse_source_set_automation (proxy, param->pspec->name, midi_channel, control_type);
    }
  gxk_toplevel_delete (GTK_WIDGET (dialog));
}

static void
param_automation_popup_editor (GtkWidget *widget,
                               GxkParam  *param)
{
  SfiProxy proxy = bst_param_get_proxy (param);
  if (proxy)
    {
      static GxkDialog *automation_dialog = NULL;
      if (!automation_dialog)
        {
          automation_dialog = g_object_new (GXK_TYPE_DIALOG, NULL);
          /* configure dialog */
          g_object_set (automation_dialog,
                        "flags", (GXK_DIALOG_HIDE_ON_DELETE |
                                  GXK_DIALOG_PRESERVE_STATE |
                                  GXK_DIALOG_POPUP_POS |
                                  GXK_DIALOG_MODAL),
                        NULL);
          // gxk_dialog_set_sizes (automation_dialog, 550, 300, 600, 320);
          GtkBox *vbox = g_object_new (GTK_TYPE_VBOX, "visible", TRUE, "border-width", 5, NULL);
          /* setup parameter: midi_channel */
          GParamSpec *pspec = bst_procedure_ref_pspec ("BseSource+set-automation", "midi-channel");
          GxkParam *dialog_param = bst_param_new_value (pspec, NULL, NULL);
          g_param_spec_unref (pspec);
          bst_param_create_gmask (dialog_param, NULL, GTK_WIDGET (vbox));
          g_object_set_data_full (automation_dialog, "GxkParam-automation-channel", dialog_param, gxk_param_destroy);
          /* setup parameter: control_type */
          pspec = bst_procedure_ref_pspec ("BseSource+set-automation", "control-type");
          dialog_param = bst_param_new_value (pspec, NULL, NULL);
          g_param_spec_unref (pspec);
          bst_param_create_gmask (dialog_param, NULL, GTK_WIDGET (vbox));
          g_object_set_data_full (automation_dialog, "GxkParam-automation-control", dialog_param, gxk_param_destroy);
          /* dialog contents */
          gxk_dialog_set_child (GXK_DIALOG (automation_dialog), GTK_WIDGET (vbox));
          /* provide buttons */
          gxk_dialog_default_action_swapped (automation_dialog, BST_STOCK_OK, param_automation_dialog_ok, automation_dialog);
          gxk_dialog_action_swapped (automation_dialog, BST_STOCK_CANCEL, param_automation_dialog_cancel, automation_dialog);
        }
      g_object_set_data (automation_dialog, "beast-GxkParam", param);
      GxkParam *param_channel = g_object_get_data (automation_dialog, "GxkParam-automation-channel");
      GxkParam *param_control = g_object_get_data (automation_dialog, "GxkParam-automation-control");
      sfi_value_set_int (&param_channel->value, bse_source_get_automation_channel (proxy, param->pspec->name));
      sfi_value_set_choice (&param_control->value, bse_midi_signal_type_to_choice (bse_source_get_automation_control (proxy, param->pspec->name)));
      gxk_param_apply_value (param_channel); /* update model, auto updates GUI */
      gxk_param_apply_value (param_control); /* update model, auto updates GUI */
      /* setup for proxy */
      bst_window_sync_title_to_proxy (automation_dialog, proxy,
                                      /* TRANSLATORS: this is a dialog title and %s is replaced by an object name */
                                      _("Control Automation: %s"));
      /* cleanup connections to old parent_window */
      if (GTK_WINDOW (automation_dialog)->group)
        gtk_window_group_remove_window (GTK_WINDOW (automation_dialog)->group, GTK_WINDOW (automation_dialog));
      gtk_window_set_transient_for (GTK_WINDOW (automation_dialog), NULL);
      /* setup connections to new parent_window */
      GtkWindow *parent_window = (GtkWindow*) gtk_widget_get_ancestor (widget, GTK_TYPE_WINDOW);
      if (parent_window)
        {
          gtk_window_set_transient_for (GTK_WINDOW (automation_dialog), parent_window);
          if (parent_window->group)
            gtk_window_group_add_window (parent_window->group, GTK_WINDOW (automation_dialog));
        }
      gxk_widget_showraise (GTK_WIDGET (automation_dialog));
    }
}

static void
param_automation_unrequest_focus_space (GtkWidget      *button,  // GTKFIX: GtkButton requests focus space for !CAN_FOCUS
                                        GtkRequisition *requisition)
{
  gint focus_width = 0, focus_pad = 0;
  gtk_widget_style_get (button, "focus-line-width", &focus_width, "focus-padding", &focus_pad, NULL);
  if (requisition->width > 2 * (focus_width + focus_pad) &&
      requisition->height > 2 * (focus_width + focus_pad))
    {
      requisition->width -= 2 * (focus_width + focus_pad);
      requisition->height -= 2 * (focus_width + focus_pad);
    }
}

static GtkWidget*
param_automation_create (GxkParam    *param,
                         const gchar *tooltip,
                         guint        variant)
{
  /* create fake-entry dialog-popup button */
  GtkWidget *widget = g_object_new (GTK_TYPE_EVENT_BOX, NULL);
  gxk_widget_modify_normal_bg_as_base (widget);
  GtkWidget *button = g_object_new (GTK_TYPE_BUTTON,
                                    "can-focus", 0,
                                    "parent", widget,
                                    NULL);
  gxk_widget_modify_normal_bg_as_base (button);
  g_object_connect (button, "signal_after::size-request", param_automation_unrequest_focus_space, button, NULL);
  GtkWidget *label = g_object_new (GTK_TYPE_LABEL,
                                   "label", "88",
                                   "xpad", 2,
                                   "parent", button,
                                   NULL);
  gchar *string = g_strdup_printf (_("Setup automation for property: %s"), g_param_spec_get_nick (param->pspec));
  gxk_widget_set_tooltip (button, string);
  g_free (string);
  gtk_widget_show_all (widget);
  /* store handles */
  g_object_set_data (widget, "beast-GxkParam", param);
  g_object_set_data (widget, "beast-GxkParam-label", label);
  /* connections */
  g_object_connect (button, "signal::clicked", param_automation_popup_editor, param, NULL);
  return widget;
}

static void
param_automation_update (GxkParam  *param,
                         GtkWidget *widget)
{
  SfiProxy proxy = bst_param_get_proxy (param);
  gchar *content = NULL;
  if (proxy)
    {
      gint midi_channel = bse_source_get_automation_channel (proxy, param->pspec->name);
      gint control_type = bse_source_get_automation_control (proxy, param->pspec->name);
      if (control_type >= BSE_MIDI_SIGNAL_CONTINUOUS_0 && control_type <= BSE_MIDI_SIGNAL_CONTINUOUS_31)
        control_type = control_type - BSE_MIDI_SIGNAL_CONTINUOUS_0 + 0x80;
      else if (control_type >= BSE_MIDI_SIGNAL_CONTROL_0 && control_type <= BSE_MIDI_SIGNAL_CONTROL_127)
        control_type = control_type - BSE_MIDI_SIGNAL_CONTROL_0 + 0x00;
      else
        control_type += 0x1000; /* shouldn't happen */
      if (midi_channel)
        content = g_strdup_printf ("%u:%02X", midi_channel, control_type);
      else
        content = g_strdup_printf ("%02X", control_type);
    }
  GtkWidget *label = g_object_get_data (widget, "beast-GxkParam-label");
  g_object_set (label,
                "label", content ? content : "--",
                NULL);
  g_free (content);
  gtk_widget_set_sensitive (widget, proxy && !bse_source_is_prepared (proxy));
}

static GxkParamEditor param_automation = {
  { "automation",       N_("Control Automation"), },
  { 0, },
  { "automate",         -5,     TRUE, },        /* options, rating, editing */
  param_automation_create, param_automation_update,
};
