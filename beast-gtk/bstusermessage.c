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
#include "bstusermessage.h"


/* --- prototypes --- */
static GtkWidget*	create_script_control_dialog	(BswProxy	script_control);


/* --- variables --- */
static GSList *msg_windows = NULL;


/* --- functions --- */
static void
user_message (BswProxy        server,
	      BswUserMsgType  msg_type,
	      const gchar    *message)
{
  bst_user_message_popup (msg_type, message);
}

static void
script_start (BswProxy server,
	      BswProxy script_control)
{
  create_script_control_dialog (script_control);
}

static void
script_error (BswProxy     server,
	      const gchar *script_name,
	      const gchar *proc_name,
	      const gchar *reason)
{
  gchar *msg = g_strdup_printf ("Invocation of %s() from \"%s\" failed: %s",
				proc_name, script_name, reason);
  bst_user_message_popup (BSW_USER_MSG_ERROR, msg);
  g_free (msg);
}

void
bst_catch_scripts_and_msgs (void)
{
  bsw_proxy_connect (BSW_SERVER,
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

static const gchar*
message_title (BswUserMsgType mtype,
	       const gchar  **stock)
{
  gchar *msg;
  switch (mtype)
    {
    case BSW_USER_MSG_INFO:
      *stock = BST_STOCK_INFO;
      msg = "Notice";
      break;
    case BSW_USER_MSG_QUESTION:
      *stock = BST_STOCK_QUESTION;
      msg = "Question";
      break;
    case BSW_USER_MSG_WARNING:
      *stock = BST_STOCK_WARNING;
      msg ="Warning";
      break;
    case BSW_USER_MSG_ERROR:
      *stock = BST_STOCK_ERROR;
      msg ="Error";
      break;
    default:
      *stock = NULL;
      msg = "Miscellaneous Message";
      break;
    }
  return msg;
}

static void
sctrl_action (gpointer   data,
	      GtkWidget *widget)
{
  BswProxy proxy = (BswProxy) data;

  bsw_script_control_trigger_action (proxy, g_object_get_data (G_OBJECT (widget), "user_data"));
}

static void
update_dialog (GxkDialog     *dialog,
	       BswUserMsgType msg_type,
	       const gchar   *message,
	       BswProxy       sctrl)
{
  const gchar *stock, *title = message_title (msg_type, &stock);
  GtkWidget *hbox;
  gchar *xmessage;

  gxk_dialog_remove_actions (dialog);
  
  hbox = g_object_new (GTK_TYPE_HBOX,
		       "visible", TRUE,
		       NULL);
  if (stock)
    gtk_box_pack_start (GTK_BOX (hbox), gxk_stock_image (stock, BST_SIZE_INFO_SIGN),
			FALSE, FALSE, 5);
  xmessage = g_strconcat (" \n", message, NULL);
  gtk_box_pack_start (GTK_BOX (hbox),
		      g_object_new (GTK_TYPE_ALIGNMENT,
				    "visible", TRUE,
				    "xalign", 0.5,
				    "yalign", 0.5,
				    "xscale", 1.0,
				    "yscale", 0.75,
				    "child", gxk_scroll_text_create (GXK_SCROLL_TEXT_CENTER, message),
				    NULL),
		      TRUE, TRUE, 5);
  g_free (xmessage);	/* grrr, the new text widget is still enormously buggy */
  gxk_dialog_set_child (dialog, hbox);
  gxk_dialog_set_title (dialog, title);
  if (BSW_IS_SCRIPT_CONTROL (sctrl))
    {
      guint i, n = bsw_script_control_n_actions (sctrl);

      for (i = 0; i < n; i++)
	{
	  gchar *action = bsw_script_control_get_action (sctrl, i);
	  gchar *name = bsw_script_control_get_action_name (sctrl, i);
	  gchar *blurb = bsw_script_control_get_action_blurb (sctrl, i);

	  if (action)
	    {
	      GtkWidget *button = gxk_dialog_action_multi (dialog, name,
							   sctrl_action, (gpointer) sctrl,
							   action, GXK_DIALOG_MULTI_SWAPPED);
	      g_object_set_data_full (G_OBJECT (button), "user_data", g_strdup (action), g_free);
	      gtk_tooltips_set_tip (BST_TOOLTIPS, button, blurb, NULL);
	    }
	}
      gxk_dialog_action (dialog, BST_STOCK_CANCEL, gxk_toplevel_delete, NULL);
    }
}

GtkWidget*
bst_user_message_popup (BswUserMsgType msg_type,
			const gchar   *message)
{
  GxkDialog *dialog = gxk_dialog_new (NULL, NULL, 0, NULL, NULL);
  GtkWidget *widget = GTK_WIDGET (dialog);

  update_dialog (dialog, msg_type, message, 0);	/* deletes actions */
  gxk_dialog_add_flags (dialog, GXK_DIALOG_DELETE_BUTTON);
  g_object_connect (dialog,
		    "signal::destroy", dialog_destroyed, NULL,
		    NULL);
  msg_windows = g_slist_prepend (msg_windows, dialog);
  gtk_widget_show (widget);
  return widget;
}

static void
sctrl_actions_changed (GxkDialog *dialog)
{
  BswProxy sctrl = (BswProxy) g_object_get_data (G_OBJECT (dialog), "user-data");
  BswUserMsgType msg_type;
  gchar *message;

  g_object_get (bse_object_from_id (sctrl),
		"user-msg-type", &msg_type,
		"user-msg", &message,
		NULL);
  update_dialog (dialog, msg_type, message, sctrl);
  g_free (message);
}

static void
sctrl_progress (GxkDialog *dialog,
		gfloat     progress)
{
  BswProxy sctrl = (BswProxy) g_object_get_data (G_OBJECT (dialog), "user-data");
  gchar *exec_name = g_strdup_printf ("%s::%s()",
				      bsw_script_control_get_script_name (sctrl),
				      bsw_script_control_get_proc_name (sctrl));

  gxk_status_window_push (dialog);
  if (progress < 0)
    gxk_status_set (GXK_STATUS_PROGRESS, exec_name, "processing");
  else
    gxk_status_set (progress * 100.0, exec_name, "processing");
  gxk_status_window_pop ();
  g_free (exec_name);
}

static void
sctrl_window_destroyed (GxkDialog *dialog)
{
  BswProxy sctrl = (BswProxy) g_object_get_data (G_OBJECT (dialog), "user-data");

  bsw_script_control_kill (sctrl);
  bsw_item_unuse (sctrl);
  bsw_proxy_disconnect (sctrl,
			"any_signal", sctrl_actions_changed, dialog,
			"any_signal", sctrl_progress, dialog,
			"any_signal", gtk_widget_destroy, dialog,
			NULL);
}

static GtkWidget*
create_script_control_dialog (BswProxy script_control)
{
  GxkDialog *dialog = gxk_dialog_new (NULL, NULL, GXK_DIALOG_STATUS_SHELL, NULL, NULL);

  g_object_set_data (G_OBJECT (dialog), "user-data", (gpointer) script_control);
  bsw_proxy_connect (script_control,
		     "swapped-object-signal::action-changed", sctrl_actions_changed, dialog,
		     "swapped-object-signal::notify::user-msg", sctrl_actions_changed, dialog,
		     "swapped-object-signal::progress", sctrl_progress, dialog,
		     "swapped-object-signal::killed", gtk_widget_destroy, dialog,
		     NULL);
  sctrl_actions_changed (dialog);
  bsw_item_use (script_control);
  g_object_connect (dialog,
		    "swapped_signal::destroy", sctrl_window_destroyed, dialog,
		    NULL);
  gtk_widget_show (GTK_WIDGET (dialog));

  return GTK_WIDGET (dialog);
}
