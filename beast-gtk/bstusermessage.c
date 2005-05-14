/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2004 Tim Janik
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
#include "bstusermessage.h"
#include "bstgconfig.h"
#include "bstmsgabsorb.h"
#include <string.h>


/* --- prototypes --- */
static GtkWidget*	create_janitor_dialog	(SfiProxy	   janitor);
static void             bst_user_message_popup  (const BseUserMsg *umsg);

/* --- variables --- */
static GSList *msg_windows = NULL;


/* --- functions --- */
static void
user_message (SfiProxy        server,
	      SfiRec         *user_msg_rec)
{
  BseUserMsg *umsg = bse_user_msg_from_rec (user_msg_rec);
  bst_user_message_popup (umsg);
  bse_user_msg_free (umsg);
}

static void
script_start (SfiProxy server,
	      SfiProxy janitor)
{
  create_janitor_dialog (janitor);
}

static void
script_error (SfiProxy     server,
	      const gchar *script_name,
	      const gchar *proc_name,
	      const gchar *reason)
{
  BseUserMsg umsg = { 0, };
  umsg.title = (char*) script_name;
  umsg.primary = g_strdup_printf (_("Failure in script \"%s\""), script_name);
  umsg.secondary = g_strdup_printf (_("Failed to start %s() of script \"%s\": %s"),
                                    proc_name, script_name, reason);
  umsg.msg_type = BSE_USER_MSG_ERROR;
  bst_user_message_popup (&umsg);
  g_free (umsg.primary);
  g_free (umsg.secondary);
}

void
bst_catch_scripts_and_msgs (void)
{
  bse_proxy_connect (BSE_SERVER,
		     "signal::user_message", user_message, NULL,
		     "signal::script_start", script_start, NULL,
		     "signal::script_error", script_error, NULL,
		     NULL);
}

void
bst_user_messages_kill (void)
{
  while (msg_windows)
    gtk_widget_destroy (msg_windows->data);
}

static void
dialog_destroyed (GtkWidget *dialog)
{
  msg_windows = g_slist_remove (msg_windows, dialog);
}

static gchar*
message_title (const BseUserMsg *umsg,
	       const gchar     **stock)
{
  const gchar *msg;
  switch (umsg->msg_type)
    {
    case BSE_USER_MSG_ERROR:
      *stock = BST_STOCK_ERROR;
      msg =_("Error");
      break;
    case BSE_USER_MSG_WARNING:
      *stock = BST_STOCK_WARNING;
      msg =_("Warning");
      break;
    case BSE_USER_MSG_INFO:
      *stock = BST_STOCK_INFO;
      msg = _("Info");
      break;
    default:
    case BSE_USER_MSG_MISC:
      *stock = BST_STOCK_DIAG;
      msg = _("Miscellaneous Message"); // _("Diagnostic");
      break;
    }
  if (umsg->title)
    return g_strconcat (msg, ": ", umsg->title, NULL);
  else
    return g_strdup (msg);
}

static void
janitor_action (gpointer   data,
		GtkWidget *widget)
{
  SfiProxy proxy = (SfiProxy) data;
  
  bse_janitor_trigger_action (proxy, g_object_get_data (G_OBJECT (widget), "user_data"));
}

static void
toggle_update_filter (GtkWidget *toggle,
                      gpointer   data)
{
  const gchar *config_check = data;
  if (config_check && bst_msg_absorb_config_adjust (config_check, GTK_TOGGLE_BUTTON (toggle)->active, TRUE))
    bst_msg_absorb_config_save();
}

static gchar*
adapt_message_spacing (const gchar *head,
                       const gchar *message,
                       const gchar *tail)
{
  GString *gstring = g_string_new (message);
  /* strip whitespaces */
  while (gstring->len && (gstring->str[0] == ' ' || gstring->str[0] == '\t' || gstring->str[0] == '\n'))
    g_string_erase (gstring, 0, 1);
  while (gstring->len && (gstring->str[gstring->len-1] == ' ' || gstring->str[gstring->len-1] == '\t' || gstring->str[gstring->len-1] == '\n'))
    g_string_erase (gstring, gstring->len-1, 1);
  /* combine parts */
  if (head)
    g_string_insert (gstring, 0, head);
  if (tail)
    g_string_append (gstring, tail);
  return g_string_free (gstring, FALSE);
}

static gchar*
strdup_umsg_hashkey (const BseUserMsg *umsg)
{
  return g_strdup_printf ("## %x ## %s ## %s ## %x", umsg->msg_type, umsg->primary, umsg->secondary, umsg->pid);
}

static void
update_dialog (GxkDialog        *dialog,
               const BseUserMsg *umsg,
	       SfiProxy          janitor)
{
  const gchar *stock;
  gchar *title = message_title (umsg, &stock);
  gxk_dialog_remove_actions (dialog);
  /* create new dialog layout from scratch */
  GtkWidget *table = gtk_table_new (1, 1, FALSE);
  g_object_set (table,
                "visible", TRUE,
                "border-width", 11,
                NULL);
  guint row = 0;
  /* stock icon (left from title/text) */
  if (stock)
    gtk_table_attach (GTK_TABLE (table), gxk_stock_image (stock, GXK_ICON_SIZE_INFO_SIGN),
                      0, 1, row, row + 1, /* left/right, top/bottom */
                      GTK_FILL, GTK_FILL, 0, 0);
  /* pure primary text */
  if (umsg->primary && umsg->secondary)
    {
      gchar *text = adapt_message_spacing (NULL, umsg->primary, NULL);
      GtkWidget *label = g_object_new (GTK_TYPE_LABEL, "visible", TRUE, "label", text, "selectable", TRUE, NULL);
      gxk_label_set_attributes (GTK_LABEL (label),
                                PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                                PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                0);
      gtk_table_attach (GTK_TABLE (table), label,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
      row++;
    }
  /* main (secondary) text */
  gchar *text_message = adapt_message_spacing ("\n", umsg->secondary ? umsg->secondary : umsg->primary, NULL);
  GtkWidget *text = g_object_new (GTK_TYPE_ALIGNMENT,
                                  "visible", TRUE,
                                  "xalign", 0.5,
                                  "yalign", 0.5,
                                  "xscale", 1.0,
                                  "yscale", 1.0,
                                  "child", gxk_scroll_text_create (GXK_SCROLL_TEXT_WIDGET_LOOK |
                                                                   // GXK_SCROLL_TEXT_CENTER |
                                                                   GXK_SCROLL_TEXT_VFIXED,
                                                                   text_message),
                                  NULL);
  g_free (text_message);
  gtk_table_attach (GTK_TABLE (table), text,
                    1, 2, row, row + 1, /* left/right, top/bottom */
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
  row++;
  if (!janitor) /* setup data for recognition and repetition */
    {
      g_object_set_data_full (dialog, "BEAST-user-message-hashkey", strdup_umsg_hashkey (umsg), g_free);
      GtkWidget *label = g_object_new (GTK_TYPE_LABEL, "visible", FALSE, "xalign", 1.0, "label", "", NULL);
      gtk_table_attach (GTK_TABLE (table), label,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
      row++;
      g_object_set_data_full (dialog, "BEAST-user-message-repeater", g_object_ref (label), g_object_unref);
      g_object_set_int (dialog, "BEAST-user-message-count", 1);
    }
  if (umsg && (umsg->log_domain || umsg->process || umsg->pid))
    {
      GtkWidget *exp = gtk_expander_new (_("Details:"));
      gtk_widget_show (exp);
      gtk_table_attach (GTK_TABLE (table), exp,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
      row++;
      GString *gstring = g_string_new (umsg->details);
      if (gstring->str[gstring->len - 1] != '\n')
        g_string_aprintf (gstring, "\n");
      g_string_aprintf (gstring, "\n");
      if (umsg->log_domain)
        g_string_aprintf (gstring, _("Origin:  %s\n"), umsg->log_domain);
      if (umsg->process)
        g_string_aprintf (gstring, _("Process: %s\n"), umsg->process);
      if (umsg->pid)
        g_string_aprintf (gstring, _("PID:     %u\n"), umsg->pid);
      while (gstring->len && gstring->str[gstring->len-1] == '\n')
        g_string_erase (gstring, gstring->len-1, 1);
      text = g_object_new (GTK_TYPE_ALIGNMENT,
                           "xalign", 0.0,
                           "yalign", 0.5,
                           "xscale", 1.0,
                           "yscale", 1.0,
                           "child", gxk_scroll_text_create (GXK_SCROLL_TEXT_WIDGET_BG |
                                                            GXK_SCROLL_TEXT_MONO |
                                                            GXK_SCROLL_TEXT_WRAP |
                                                            GXK_SCROLL_TEXT_VFIXED,
                                                            gstring->str),
                           NULL);
      g_string_free (gstring, TRUE);
      if (0) // GTKFIX: the expander child isn't properly visible with container_add in gtk+2.4.9
        gtk_container_add (GTK_CONTAINER (exp), text);
      else
        {
          gtk_table_attach (GTK_TABLE (table), text,
                            1, 2, row, row + 1, /* left/right, top/bottom */
                            GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
          row++;
          gxk_expander_connect_to_widget (exp, text);
        }
    }
  if (1) /* table vexpansion */
    {
      GtkWidget *space = g_object_new (GTK_TYPE_ALIGNMENT, "visible", TRUE, NULL);
      gtk_table_attach (GTK_TABLE (table), space,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        0, GTK_EXPAND, 0, 0);
      row++;
    }
  if (umsg && umsg->config_check)
    {
      GtkWidget *cb = gtk_check_button_new_with_label (umsg->config_check);
      gxk_widget_set_tooltip (cb, _("This setting can be changed in the \"Messages\" section of the preferences dialog"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), !bst_msg_absorb_config_match (umsg->config_check));
      g_signal_connect_data (cb, "unrealize", G_CALLBACK (toggle_update_filter), g_strdup (umsg->config_check), (GClosureNotify) g_free, G_CONNECT_AFTER);
      gtk_widget_show (cb);
      gtk_table_attach (GTK_TABLE (table), cb,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
      row++;
    }
  gxk_dialog_set_child (dialog, table);
  gxk_dialog_set_title (dialog, title);
  g_free (title);
  if (BSE_IS_JANITOR (janitor))
    {
      guint i, n = bse_janitor_n_actions (janitor);
      
      for (i = 0; i < n; i++)
	{
	  const gchar *action = bse_janitor_get_action (janitor, i);
	  const gchar *name = bse_janitor_get_action_name (janitor, i);
	  const gchar *blurb = bse_janitor_get_action_blurb (janitor, i);
	  
	  if (action)
	    {
	      GtkWidget *button = gxk_dialog_action_multi (dialog, name,
							   janitor_action, (gpointer) janitor,
							   action, GXK_DIALOG_MULTI_SWAPPED);
	      g_object_set_data_full (G_OBJECT (button), "user_data", g_strdup (action), g_free);
	      gxk_widget_set_tooltip (button, blurb);
	    }
	}
      gxk_dialog_action (dialog, BST_STOCK_CANCEL, gxk_toplevel_delete, NULL);
    }
}

static void
repeat_dialog (GxkDialog *dialog)
{
  GtkLabel *label = g_object_get_data (dialog, "BEAST-user-message-repeater");
  if (label)
    {
      gint count = g_object_get_int (dialog, "BEAST-user-message-count");
      gchar *rstr = g_strdup_printf (dngettext (_DOMAIN, _("Message has been repeated %u time"), _("Message has been repeated %u times"), count), count);
      g_object_set_int (dialog, "BEAST-user-message-count", count + 1);
      gtk_label_set_text (label, rstr);
      g_free (rstr);
      gtk_widget_show (GTK_WIDGET (label));
    }
}

static GtkWidget*
find_dialog (GSList           *dialog_list,
             const BseUserMsg *umsg)
{
  gchar *umid = strdup_umsg_hashkey (umsg);
  GtkWidget *widget = NULL;
  GSList *slist;
  for (slist = dialog_list; slist; slist = slist->next)
    {
      const gchar *hk = g_object_get_data (slist->data, "BEAST-user-message-hashkey");
      if (hk && strcmp (hk, umid) == 0)
        {
          widget = slist->data;
          break;
        }
    }
  g_free (umid);
  return widget;
}

static void
dialog_show_above_modals (GxkDialog *dialog)
{
  /* if a grab is in effect, we need to override it */
  GtkWidget *grab = gtk_grab_get_current();
  if (grab)
    {
      gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    }
  gtk_widget_show (GTK_WIDGET (dialog));
}

static void
bst_user_message_popup (const BseUserMsg *umsg)
{
  GxkDialog *dialog = (GxkDialog*) find_dialog (msg_windows, umsg);
  if (dialog)
    repeat_dialog (dialog);
  else if (umsg && umsg->config_check && bst_msg_absorb_config_match (umsg->config_check))
    bst_msg_absorb_config_update (umsg->config_check); /* message absorbed by configuration */
  else
    {
      dialog = gxk_dialog_new (NULL, NULL, 0, NULL, NULL);
      gxk_dialog_set_sizes (dialog, -1, -1, 512, -1);
      
      update_dialog (dialog, umsg, 0); /* deletes actions */
      gxk_dialog_add_flags (dialog, GXK_DIALOG_DELETE_BUTTON);
      g_object_connect (dialog,
                        "signal::destroy", dialog_destroyed, NULL,
                        NULL);
      msg_windows = g_slist_prepend (msg_windows, dialog);
      dialog_show_above_modals (dialog);
    }
}

void
bst_user_message_log_handler (const SfiLogMessage *msg)
{
  BseUserMsg umsg = { 0, };
  umsg.log_domain = msg->log_domain;
  switch (msg->level)
    {
    case SFI_MSG_ERROR:   umsg.msg_type = BSE_USER_MSG_ERROR;   break;
    case SFI_MSG_WARNING: umsg.msg_type = BSE_USER_MSG_WARNING; break;
    case SFI_MSG_INFO:    umsg.msg_type = BSE_USER_MSG_INFO;    break;
    default:              umsg.msg_type = BSE_USER_MSG_MISC;    break;
    }
  umsg.config_check = msg->config_check;
  umsg.title = msg->title;
  umsg.primary = msg->primary;
  umsg.secondary = msg->secondary;
  umsg.details = msg->details;
  umsg.pid = sfi_thread_get_pid (NULL);
  umsg.process = (char*) sfi_thread_get_name (NULL);
  bst_user_message_popup (&umsg);
}

static void
janitor_actions_changed (GxkDialog *dialog)
{
  SfiProxy janitor = (SfiProxy) g_object_get_data (G_OBJECT (dialog), "user-data");
  BseUserMsg umsg = { 0, };
  umsg.title = (char*) bse_janitor_get_script_name (janitor);
  bse_proxy_get (janitor,
		 "user-msg-type", &umsg.msg_type,
		 "user-msg", &umsg.primary,
		 NULL);
  umsg.details = g_strdup_printf (_("Procedure: %s\nScript: %s"), bse_janitor_get_proc_name (janitor), bse_janitor_get_script_name (janitor));
  umsg.pid = 0;
  umsg.process = NULL;
  update_dialog (dialog, &umsg, janitor);
  g_free (umsg.details);
}

static void
janitor_progress (GxkDialog *dialog,
		  SfiReal    progress)
{
  SfiProxy janitor = (SfiProxy) g_object_get_data (G_OBJECT (dialog), "user-data");
  const gchar *script = bse_janitor_get_script_name (janitor);
  const gchar *sbname = strrchr (script, '/');
  gchar *exec_name = g_strdup_printf ("%s", sbname ? sbname + 1 : script);
  // bse_janitor_get_proc_name (janitor);
  gxk_status_window_push (dialog);
  if (progress < 0)
    gxk_status_set (GXK_STATUS_PROGRESS, exec_name, _("processing"));
  else
    gxk_status_set (progress * 100.0, exec_name, _("processing"));
  gxk_status_window_pop ();
  g_free (exec_name);
}

static void
janitor_window_destroyed (GxkDialog *dialog)
{
  SfiProxy janitor = (SfiProxy) g_object_get_data (G_OBJECT (dialog), "user-data");
  
  bse_janitor_kill (janitor);
  bse_item_unuse (janitor);
  bse_proxy_disconnect (janitor,
			"any_signal", janitor_actions_changed, dialog,
			"any_signal", janitor_progress, dialog,
			"any_signal", gtk_widget_destroy, dialog,
			NULL);
}

static GtkWidget*
create_janitor_dialog (SfiProxy janitor)
{
  GxkDialog *dialog = gxk_dialog_new (NULL, NULL,
                                      GXK_DIALOG_STATUS_BAR, // | GXK_DIALOG_WINDOW_GROUP,
                                      NULL, NULL);
  gxk_dialog_set_sizes (dialog, -1, -1, 512, -1);
  
  g_object_set_data (G_OBJECT (dialog), "user-data", (gpointer) janitor);
  bse_proxy_connect (janitor,
		     "swapped-object-signal::action-changed", janitor_actions_changed, dialog,
		     "swapped-object-signal::property-notify::user-msg", janitor_actions_changed, dialog,
		     "swapped-object-signal::progress", janitor_progress, dialog,
		     "swapped-object-signal::closed", gtk_widget_destroy, dialog,
		     NULL);
  janitor_actions_changed (dialog);
  bse_item_use (janitor);
  g_object_connect (dialog,
		    "swapped_signal::destroy", janitor_window_destroyed, dialog,
		    NULL);
  dialog_show_above_modals (dialog);
  
  return GTK_WIDGET (dialog);
}
