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
#include <errno.h>


/* --- prototypes --- */
static GtkWidget*	create_janitor_dialog	(SfiProxy	   janitor);

/* --- variables --- */
static GSList *msg_windows = NULL;


/* --- functions --- */
static void
dialog_destroyed (GtkWidget *dialog)
{
  msg_windows = g_slist_remove (msg_windows, dialog);
}

static inline gboolean
hastext (const gchar *string)
{
  if (!string || !string[0])
    return FALSE;
  while (string[0] == '\t' || string[0] == ' ' || string[0] == '\n' || string[0] == '\r')
    string++;
  return string[0] != 0;
}

static const gchar*
procedure_get_title (const gchar *procedure)
{
  if (hastext (procedure))
    {
      BseCategorySeq *cseq = bse_categories_match_typed ("*", procedure);
      if (cseq->n_cats)
        return cseq->cats[0]->category + cseq->cats[0]->lindex + 1;
    }
  return NULL;
}

static gchar*
message_title (const BstMessage *msg,
	       const gchar     **stock,
	       const gchar     **prefix)
{
  switch (msg->type)
    {
    case BST_MSG_FATAL:
      *stock = BST_STOCK_ERROR;
      *prefix = _("Fatal Error: ");
      break;
    case BST_MSG_ERROR:
      *stock = BST_STOCK_ERROR;
      *prefix = _("Error: ");
      break;
    case BST_MSG_WARNING:
      *stock = BST_STOCK_WARNING;
      break;
    case BST_MSG_SCRIPT:
      *stock = BST_STOCK_EXECUTE;
      break;
    case BST_MSG_INFO:
      *stock = BST_STOCK_INFO;
      break;
    default:
    case BST_MSG_DIAG:
      *stock = BST_STOCK_DIAG;
      break;
    case BST_MSG_DEBUG:
      *stock = BST_STOCK_DIAG;
      break;
    }
  const gchar *message = msg->label ? msg->label : msg->ident;
  if (msg->title)
    return g_strconcat (message, ": ", msg->title, NULL);
  else
    {
      const gchar *proc_name = msg->janitor ? bse_janitor_get_proc_name (msg->janitor) : NULL;
      const gchar *proc_title = procedure_get_title (proc_name);
      if (proc_title)
        return g_strconcat (message, ": ", proc_title, NULL);
      else
        return g_strdup (message);
    }
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
strdup_msg_hashkey (const BstMessage *msg)
{
  /* prefer hashing by janitor/process name over PID */
  if (msg->janitor)
    return g_strdup_printf ("## %x ## %s ## %s ## J%s:%s", msg->type, msg->primary, msg->secondary,
                            bse_janitor_get_script_name (msg->janitor), bse_janitor_get_proc_name (msg->janitor));
  else if (msg->process)
    return g_strdup_printf ("## %x ## %s ## %s ## P%s", msg->type, msg->primary, msg->secondary, msg->process);
  else
    return g_strdup_printf ("## %x ## %s ## %s ## N%x", msg->type, msg->primary, msg->secondary, msg->pid);
}

static void
bst_msg_dialog_update (GxkDialog        *dialog,
                       const BstMessage *msg,
                       gboolean          accumulate_repetitions)
{
  const gchar *stock, *primary_prefix = NULL;
  gchar *title = message_title (msg, &stock, &primary_prefix);
  gxk_dialog_remove_actions (dialog);
  /* create new dialog layout from scratch */
  GtkWidget *table = gtk_table_new (1, 1, FALSE);
  g_object_set (table, "visible", TRUE, "border-width", 11, NULL);
  guint row = 0;
  /* stock icon (positioned left from title/text) */
  if (stock)
    gtk_table_attach (GTK_TABLE (table), gxk_stock_image (stock, GXK_ICON_SIZE_INFO_SIGN),
                      0, 1, row, row + 1, /* left/right, top/bottom */
                      GTK_FILL, GTK_FILL, 0, 0);
  const gchar *primary = (msg->primary || msg->secondary) ? msg->primary : msg->secondary;
  const gchar *secondary = (msg->primary && msg->secondary) ? msg->secondary : NULL;
  /* primary text */
  if (primary)
    {
      gchar *text = adapt_message_spacing (primary_prefix, primary, NULL);
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
  /* secondary text */
  if (secondary)
    {
      gchar *text_message = adapt_message_spacing ("\n", secondary, NULL);
      GtkWidget *scroll_text = gxk_scroll_text_create (GXK_SCROLL_TEXT_WIDGET_LOOK |
                                                       // GXK_SCROLL_TEXT_CENTER |
                                                       GXK_SCROLL_TEXT_VFIXED,
                                                       text_message);
      GtkWidget *main_text = g_object_new (GTK_TYPE_ALIGNMENT,
                                           "visible", TRUE,
                                           "xalign", 0.5,
                                           "yalign", 0.5,
                                           "xscale", 1.0,
                                           "yscale", 1.0,
                                           "child", scroll_text,
                                           NULL);
      g_free (text_message);
      gtk_table_attach (GTK_TABLE (table), main_text,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
      row++;
    }
  /* setup data for recognition and message repetition counter */
  if (accumulate_repetitions)
    {
      g_object_set_data_full (dialog, "BEAST-message-hashkey", strdup_msg_hashkey (msg), g_free);
      GtkWidget *label = g_object_new (GTK_TYPE_LABEL, "visible", FALSE, "xalign", 1.0, "label", "", NULL);
      gtk_table_attach (GTK_TABLE (table), label,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
      row++;
      g_object_set_data_full (dialog, "BEAST-user-message-repeater", g_object_ref (label), g_object_unref);
      g_object_set_int (dialog, "BEAST-user-message-count", 1);
    }
  /* add details section */
  if (msg->log_domain || msg->details || msg->janitor || msg->process || msg->pid)
    {
      GtkWidget *exp = gtk_expander_new (_("Details:"));
      gtk_widget_show (exp);
      gtk_table_attach (GTK_TABLE (table), exp,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
      row++;
      const gchar *proc_name = !msg->janitor ? NULL : bse_janitor_get_proc_name (msg->janitor);
      const gchar *script_name = proc_name ? bse_janitor_get_script_name (msg->janitor) : NULL;
      GString *gstring = g_string_new (msg->details);
      while (gstring->len && gstring->str[gstring->len - 1] == '\n')
        g_string_erase (gstring, gstring->len - 1, 1);
      g_string_aprintf (gstring, "\n\n");
      if (hastext (proc_name))
        g_string_aprintf (gstring, _("Procedure: %s\nScript: %s\n"), proc_name, script_name);
      if (hastext (msg->process))
        g_string_aprintf (gstring, _("Process: %s\n"), msg->process);
      if (hastext (msg->log_domain) && !hastext (proc_name) && !hastext (msg->process))
        g_string_aprintf (gstring, _("Origin:  %s\n"), msg->log_domain);
      if (msg->pid && BST_DVL_HINTS)
          g_string_aprintf (gstring, _("PID:     %u\n"), msg->pid);
      while (gstring->len && gstring->str[gstring->len - 1] == '\n')
        g_string_erase (gstring, gstring->len - 1, 1);
      gchar *text = adapt_message_spacing (NULL, gstring->str, NULL);
      g_string_free (gstring, TRUE);
      GtkWidget *label = g_object_new (GTK_TYPE_LABEL, "visible", TRUE, "wrap", TRUE, "xalign", 0.0, "label", text, "selectable", TRUE, NULL);
      g_free (text);
      gxk_label_set_attributes (GTK_LABEL (label),
                                PANGO_ATTR_FAMILY, "monospace",
                                0);
      if (0) // the expander child isn't properly visible with container_add in gtk+2.4.9
        gtk_container_add (GTK_CONTAINER (exp), label);
      else
        {
          gtk_table_attach (GTK_TABLE (table), label,
                            1, 2, row, row + 1, /* left/right, top/bottom */
                            GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
          row++;
          gxk_expander_connect_to_widget (exp, label);
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
  if (msg->config_check)
    {
      GtkWidget *cb = gtk_check_button_new_with_label (msg->config_check);
      gxk_widget_set_tooltip (cb, _("This setting can be changed in the \"Messages\" section of the preferences dialog"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), !bst_msg_absorb_config_match (msg->config_check));
      g_signal_connect_data (cb, "unrealize", G_CALLBACK (toggle_update_filter), g_strdup (msg->config_check), (GClosureNotify) g_free, G_CONNECT_AFTER);
      gtk_widget_show (cb);
      gtk_table_attach (GTK_TABLE (table), cb,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
      row++;
    }
  gxk_dialog_set_child (dialog, table);
  gxk_dialog_set_title (dialog, title);
  g_free (title);
}

static void
bst_msg_dialog_janitor_update (GxkDialog        *dialog,
                               SfiProxy          janitor)
{
  g_return_if_fail (BSE_IS_JANITOR (janitor));

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
  GtkWidget *bwidget = gxk_dialog_action (dialog, BST_STOCK_CANCEL, gxk_toplevel_delete, NULL);
  gxk_dialog_set_focus (dialog, bwidget);
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
             const BstMessage *msg)
{
  gchar *mid = strdup_msg_hashkey (msg);
  GtkWidget *widget = NULL;
  GSList *slist;
  for (slist = dialog_list; slist; slist = slist->next)
    {
      const gchar *hk = g_object_get_data (slist->data, "BEAST-message-hashkey");
      if (hk && strcmp (hk, mid) == 0)
        {
          widget = slist->data;
          break;
        }
    }
  g_free (mid);
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

void
bst_message_handler (const BstMessage *const_msg)
{
  BstMessage msg = *const_msg;
  /* perform slight message patch ups */
  if (!hastext (msg.title))
    msg.title = NULL;
  if (!hastext (msg.primary))
    msg.primary = NULL;
  if (!hastext (msg.secondary))
    msg.secondary = NULL;
  if (!hastext (msg.details))
    msg.details = NULL;
  if (!hastext (msg.config_check))
    msg.config_check = NULL;
  if (!msg.config_check && msg.type == BST_MSG_INFO)
    msg.config_check = _("Display dialogs with information messages");
  if (!msg.config_check && msg.type == BST_MSG_DIAG)
    msg.config_check = _("Display dialogs with dignostic messages");
  if (!msg.config_check && msg.type == BST_MSG_DEBUG)
    msg.config_check = _("Display dialogs with debugging messages");
  GxkDialog *dialog = (GxkDialog*) find_dialog (msg_windows, &msg);
  if (dialog)
    repeat_dialog (dialog);
  else if (msg.config_check && bst_msg_absorb_config_match (msg.config_check))
    bst_msg_absorb_config_update (msg.config_check); /* message absorbed by configuration */
  else
    {
      dialog = gxk_dialog_new (NULL, NULL, 0, NULL, NULL);
      gxk_dialog_set_sizes (dialog, -1, -1, 512, -1);
      
      bst_msg_dialog_update (dialog, &msg, TRUE); /* deletes actions */
      gxk_dialog_add_flags (dialog, GXK_DIALOG_DELETE_BUTTON);
      g_object_connect (dialog,
                        "signal::destroy", dialog_destroyed, NULL,
                        NULL);
      msg_windows = g_slist_prepend (msg_windows, dialog);
      dialog_show_above_modals (dialog);
    }
}

static BstMsgType
bst_msg_type_from_user_msg_type (BseMsgType utype)
{
  g_static_assert (BST_MSG_NONE    == BSE_MSG_NONE);
  g_static_assert (BST_MSG_FATAL   == BSE_MSG_FATAL);
  g_static_assert (BST_MSG_ERROR   == BSE_MSG_ERROR);
  g_static_assert (BST_MSG_WARNING == BSE_MSG_WARNING);
  g_static_assert (BST_MSG_SCRIPT  == BSE_MSG_SCRIPT);
  g_static_assert (BST_MSG_INFO    == BSE_MSG_INFO);
  g_static_assert (BST_MSG_DIAG    == BSE_MSG_DIAG);
  g_static_assert (BST_MSG_DEBUG   == BSE_MSG_DEBUG);
  return utype;
}

static void
message_free_fields (BstMessage *msg)
{
  g_free (msg->log_domain);
  g_free (msg->title);
  g_free (msg->primary);
  g_free (msg->secondary);
  g_free (msg->details);
  g_free (msg->config_check);
  g_free (msg->process);
  g_free (msg->msg_bits);
}

static void
message_fill_from_script (BstMessage    *msg,
                          BstMsgType     mtype,
                          SfiProxy       janitor,
                          const gchar   *primary,
                          const gchar   *script_name,
                          const gchar   *proc_name,
                          const gchar   *user_msg)
{
  msg->log_domain = NULL;
  msg->type = mtype;
  msg->ident = (char*) sfi_msg_type_ident (msg->type);
  msg->label = (char*) sfi_msg_type_label (msg->type);
  const gchar *proc_title = NULL;
  if (hastext (proc_name))
    {
      BseCategorySeq *cseq = bse_categories_match_typed ("*", proc_name);
      if (cseq->n_cats)
        proc_title = cseq->cats[0]->category + cseq->cats[0]->lindex + 1;
    }
  msg->title = g_strdup (proc_title);
  msg->primary = g_strdup (primary ? primary : proc_title);
  if (user_msg && user_msg[0])
    msg->secondary = g_strdup (user_msg);
  else
    {
      gchar *script_base = g_path_get_basename (script_name);
      msg->secondary = g_strdup_printf (_("Executing procedure '%s' from script '%s'."), proc_name, script_base);
      g_free (script_base);
    }
  if (!janitor && hastext (proc_name))
    msg->details = g_strdup_printf (_("Procedure: %s\nScript: %s\n"), proc_name, script_name);
  else
    msg->details = NULL;
  msg->config_check = NULL;
  msg->janitor = janitor;
  msg->process = NULL;
  msg->pid = 0;
  msg->n_msg_bits = 0;
  msg->msg_bits = NULL;
}

static void
janitor_actions_changed (GxkDialog *dialog)
{
  SfiProxy janitor = (SfiProxy) g_object_get_data (G_OBJECT (dialog), "user-data");
  BstMessage msg = { 0, };
  BseMsgType utype = 0;
  const gchar *user_msg = NULL;
  bse_proxy_get (janitor, "user-msg-type", &utype, "user-msg", &user_msg, NULL);
  const gchar *proc_name = bse_janitor_get_proc_name (janitor);
  const gchar *script_name = bse_janitor_get_script_name (janitor);
  message_fill_from_script (&msg, BST_MSG_SCRIPT, 0, NULL, script_name, proc_name, NULL);
  bst_msg_dialog_update (dialog, &msg, FALSE);
  bst_msg_dialog_janitor_update (dialog, janitor);
  message_free_fields (&msg);
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
janitor_unconnected (GxkDialog *dialog)
{
  SfiProxy janitor = (SfiProxy) g_object_get_data (G_OBJECT (dialog), "user-data");
  gboolean connected = FALSE;
  const gchar *exit_reason = NULL;
  gint exit_code;
  bse_proxy_get (janitor, "connected", &connected, "exit-code", &exit_code, "exit-reason", &exit_reason, NULL);
  g_printerr ("GUI:janitor: connected=%d exit-code=%d exit-reason=%s\n", connected, exit_code, exit_reason);
  if (!connected)
    {
      const gchar *proc_name = bse_janitor_get_proc_name (janitor);
      const gchar *script_name = bse_janitor_get_script_name (janitor);
      /* destroy script dialog *and* janitor reference */
      gtk_widget_destroy (GTK_WIDGET (dialog));
      /* notify user about unsuccessful exits */
      if (exit_reason)
        {
          BstMessage msg = { 0, };
          gchar *error_msg = g_strdup_printf (_("An error occoured during execution of script procedure '%s': %s"), proc_name, exit_reason);
          message_fill_from_script (&msg, BST_MSG_ERROR, 0, _("Script execution error."), script_name, proc_name, error_msg);
          g_free (error_msg);
          bst_message_handler (&msg);
          message_free_fields (&msg);
        }
      else
        g_printerr ("successful script: %s (%s) (exit: %d)\n", script_name, proc_name, exit_code);
    }
}

static void
janitor_window_deleted (GxkDialog *dialog)
{
  SfiProxy janitor = (SfiProxy) g_object_get_data (G_OBJECT (dialog), "user-data");
  
  bse_proxy_disconnect (janitor,
			"any_signal", janitor_actions_changed, dialog,
			"any_signal", janitor_progress, dialog,
			"any_signal", janitor_unconnected, dialog,
			NULL);
  bse_janitor_kill (janitor);
  bse_item_unuse (janitor);
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
		     "swapped-object-signal::property-notify::connected", janitor_unconnected, dialog,
		     NULL);
  janitor_actions_changed (dialog);
  bse_item_use (janitor);
  g_object_connect (dialog, "swapped_signal::destroy", janitor_window_deleted, dialog, NULL);
  dialog_show_above_modals (dialog);
  
  return GTK_WIDGET (dialog);
}

void
bst_message_log_handler (const SfiMessage *lmsg)
{
  BstMessage msg = { 0, };
  msg.log_domain = lmsg->log_domain;
  msg.type = lmsg->type;
  msg.ident = (char*) sfi_msg_type_ident (msg.type);
  msg.label = (char*) sfi_msg_type_label (msg.type);
  msg.config_check = lmsg->config_check;
  msg.title = lmsg->title;
  msg.primary = lmsg->primary;
  msg.secondary = lmsg->secondary;
  msg.details = lmsg->details;
  msg.janitor = bse_script_janitor();
  msg.process = (char*) sfi_thread_get_name (NULL);
  msg.pid = sfi_thread_get_pid (NULL);
  msg.n_msg_bits = lmsg->n_msg_bits;
  msg.msg_bits = lmsg->msg_bits;
  bst_message_handler (&msg);
}

void
bst_message_synth_msg_handler (const BseMessage *umsg)
{
  BstMessage msg = { 0, };
  msg.log_domain = umsg->log_domain;
  msg.type = bst_msg_type_from_user_msg_type (umsg->type);
  msg.ident = (char*) sfi_msg_type_ident (msg.type);
  msg.label = (char*) sfi_msg_type_label (msg.type);
  msg.config_check = umsg->config_check;
  msg.title = umsg->title;
  msg.primary = umsg->primary;
  msg.secondary = umsg->secondary;
  msg.details = umsg->details;
  msg.janitor = umsg->janitor;
  msg.process = umsg->process;
  msg.pid = umsg->pid;
  bst_message_handler (&msg);
}

/**
 * bst_message_dialog_elist
 * @log_domain:   log domain
 * @level:        one of %BST_MSG_ERROR, %BST_MSG_WARNING, %BST_MSG_INFO, %BST_MSG_DIAG
 * @lbit1:        msg bit
 * @lbit2:        msg bit
 * @...:          list of more msg bits, NULL terminated
 *
 * Present a message dialog to the user. The current value of errno
 * is preserved around calls to this function. Usually this function isn't
 * used directly, but bst_msg_dialog() is called instead which does not require
 * %NULL termination of its argument list and automates the @log_domain argument.
 * The @log_domain indicates the calling module and relates to %G_LOG_DOMAIN
 * as used by g_log().
 * The msg bit arguments passed in form various parts of the log message, the
 * following macro set is provided to construct the parts from printf-style
 * argument lists:
 * - BST_MSG_TITLE(): format message title
 * - BST_MSG_TEXT1(): format primary message (also BST_MSG_PRIMARY())
 * - BST_MSG_TEXT2(): format secondary message, optional (also BST_MSG_SECONDARY())
 * - BST_MSG_TEXT3(): format details of the message, optional (also BST_MSG_DETAIL())
 * - BST_MSG_CHECK(): format configuration check statement to enable/disable log messages of this type.
 * This function is MT-safe and may be called from any thread.
 */
void
bst_message_dialog_elist (const char     *log_domain,
                          BstMsgType      type, /* BST_MSG_DEBUG is not really useful here */
                          SfiMsgBit      *lbit1,
                          SfiMsgBit      *lbit2,
                          ...)
{
  gint saved_errno = errno;
  guint n = 0;
  SfiMsgBit **bits = NULL;
  /* collect msg bits */
  if (lbit1)
    {
      bits = g_renew (SfiMsgBit*, bits, n + 1);
      bits[n++] = lbit1;
      SfiMsgBit *lbit = lbit2;
      va_list args;
      va_start (args, lbit2);
      while (lbit)
        {
          bits = g_renew (SfiMsgBit*, bits, n + 1);
          bits[n++] = lbit;
          lbit = va_arg (args, SfiMsgBit*);
        }
      va_end (args);
    }
  bits = g_renew (SfiMsgBit*, bits, n + 1);
  bits[n] = NULL;
  sfi_msg_log_trampoline (log_domain, type, bits, bst_message_log_handler);
  g_free (bits);
  errno = saved_errno;
}

void
bst_message_dialogs_popdown (void)
{
  while (msg_windows)
    gtk_widget_destroy (msg_windows->data);
}

static void
server_bse_message (SfiProxy        server,
                    SfiRec         *bse_msg_rec)
{
  BseMessage *umsg = bse_message_from_rec (bse_msg_rec);
  bst_message_synth_msg_handler (umsg);
  bse_message_free (umsg);
}

static void
server_script_start (SfiProxy server,
                     SfiProxy janitor)
{
  create_janitor_dialog (janitor);
}

static void
server_script_error (SfiProxy     server,
                     const gchar *script_name,
                     const gchar *proc_name,
                     const gchar *reason)
{
  /* this signal is emitted (without janitor) when script execution failed */
  BstMessage msg = { 0, };
  gchar *error_msg = g_strdup_printf (_("Failed to execute script procedure '%s': %s"), proc_name, reason);
  message_fill_from_script (&msg, BST_MSG_ERROR, 0, _("Script execution error."), script_name, proc_name, error_msg);
  g_free (error_msg);
  bst_message_handler (&msg);
  message_free_fields (&msg);
}

void
bst_message_connect_to_server (void)
{
  bse_proxy_connect (BSE_SERVER,
		     "signal::message", server_bse_message, NULL,
		     "signal::script_start", server_script_start, NULL,
		     "signal::script_error", server_script_error, NULL,
		     NULL);
}

static int
msg_id_compare (const void *v1, const void *v2)
{
  const BstMsgID *mid1 = v1;
  const BstMsgID *mid2 = v2;
  return strcmp (mid1->ident, mid2->ident);
}

const BstMsgID*
bst_message_list_types (guint *n_types)
{
  static const BstMsgID *const_msg_ids = NULL;
  static guint           const_n_msg_ids = 0;
  if (!const_msg_ids)
    {
      BstMsgID *msg_ids = NULL;
      guint i = 0;
      const gchar *ident = sfi_msg_type_ident (i);
      while (ident)
        {
          msg_ids = g_renew (BstMsgID, msg_ids, i + 1);
          msg_ids[i].type = i;
          msg_ids[i].ident = ident;
          msg_ids[i].label = sfi_msg_type_label (msg_ids[i].type);
          i++;
          ident = sfi_msg_type_ident (i);
        }
      msg_ids = g_renew (BstMsgID, msg_ids, i + 1);
      msg_ids[i].type = 0;
      msg_ids[i].ident = NULL;
      msg_ids[i].label = NULL;
      qsort (msg_ids, i, sizeof (msg_ids[0]), msg_id_compare);
      const_msg_ids = msg_ids;
      const_n_msg_ids = i;
    }
  if (n_types)
    *n_types = const_n_msg_ids;
  return const_msg_ids;
}
