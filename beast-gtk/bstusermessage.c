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

#include "bstdialog.h"


/* --- prototypes --- */
static void	user_message	(BswProxy        server,
				 BswUserMsgType  msg_type,
				 const gchar    *message);


/* --- variables --- */
static GSList *msg_windows = NULL;


/* --- functions --- */
void
bst_user_messages_listen (void)
{
  bsw_proxy_connect (BSW_SERVER,
		     "signal::user-message", user_message, NULL,
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
message_title (BswUserMsgType mtype,
	       gchar        **stock)
{
  gchar *msg;
  switch (mtype)
    {
    case BSW_USER_MSG_DISCARD:
      *stock = NULL;
      msg = NULL;
      break;
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
      msg = "Misc Message";
      break;
    }
  return msg;
}

static void
user_message (BswProxy        server,
	      BswUserMsgType  msg_type,
	      const gchar    *message)
{
  gchar *stock, *title = message_title (msg_type, &stock);

  if (title)
    {
      GtkWidget *dialog, *hbox;

      hbox = g_object_new (GTK_TYPE_HBOX,
			   "visible", TRUE,
			   NULL);
      if (stock)
	gtk_box_pack_start (GTK_BOX (hbox), bst_image_from_stock (stock, BST_SIZE_INFO_SIGN),
			    FALSE, FALSE, 5);
      gtk_box_pack_start (GTK_BOX (hbox),
			  g_object_new (GTK_TYPE_ALIGNMENT,
					"visible", TRUE,
					"xalign", 0.5,
					"yalign", 0.5,
					"xscale", 1.0,
					"yscale", 0.0,
					"child", bst_wrap_text_create (TRUE, message),
					NULL),
			  TRUE, TRUE, 5);
      dialog = bst_dialog_new (NULL, NULL, BST_DIALOG_DELETE_BUTTON, title, hbox);
      g_object_set (dialog,
		    "width_request", 320,
		    "height_request", 200,
		    NULL);
      g_object_connect (dialog,
			"signal::destroy", dialog_destroyed, NULL,
			NULL);
      msg_windows = g_slist_prepend (msg_windows, dialog);
      gtk_widget_show (dialog);
    }
  else
    g_message ("Ignoring Message: %s\n", message);
}
