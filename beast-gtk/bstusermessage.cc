// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstusermessage.hh"
#include "bstgconfig.hh"
#include "bstmsgabsorb.hh"
#include <string.h>
#include <errno.h>

/* --- variables --- */
static GSList *msg_windows = NULL;

/* --- functions --- */
const char*
bst_msg_type_ident (BstMsgType bmt)
{
  const Aida::EnumValue ev = Aida::enum_info<Bse::UserMessageType>().find_value (bmt);
  if (ev.ident)
    return ev.ident;
  switch (bmt)
    {
    case BST_MSG_SCRIPT:        return "script";
    default: ;
    }
  assert_return_unreached (NULL);
}

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

static gchar*
message_title (const BstMessage *msg,
	       const gchar     **stock,
	       const gchar     **prefix)
{
  switch (msg->type)
    {
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
    case BST_MSG_DEBUG:
      *stock = BST_STOCK_DIAG;
      break;
    default:
    case BST_MSG_INFO:
      *stock = BST_STOCK_INFO;
      break;
    }
  const gchar *message = msg->label ? msg->label : msg->ident;
  if (msg->title)
    return g_strconcat (message, ": ", msg->title, NULL);
  else
    return g_strdup (message);
}

static void
toggle_update_filter (GtkWidget *toggle,
                      gpointer   data)
{
  const gchar *config_check = (const char*) data;
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
  /* prefer hashing by process name over PID */
  if (msg->process)
    return g_strdup_format ("## %x ## %s ## %s ## P%s", msg->type, msg->primary, msg->secondary, msg->process);
  else
    return g_strdup_format ("## %x ## %s ## %s ## N%x", msg->type, msg->primary, msg->secondary, msg->pid);
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
      GtkWidget *label = (GtkWidget*) g_object_new (GTK_TYPE_LABEL, "visible", TRUE, "label", text, "selectable", TRUE, NULL);
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
      GtkWidget *main_text = (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT,
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
      g_object_set_data_full ((GObject*) dialog, "BEAST-message-hashkey", strdup_msg_hashkey (msg), g_free);
      GtkWidget *label = (GtkWidget*) g_object_new (GTK_TYPE_LABEL, "visible", FALSE, "xalign", 1.0, "label", "", NULL);
      gtk_table_attach (GTK_TABLE (table), label,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
      row++;
      g_object_set_data_full ((GObject*) dialog, "BEAST-user-message-repeater", g_object_ref (label), g_object_unref);
      g_object_set_int (dialog, "BEAST-user-message-count", 1);
    }
  /* add details section */
  if (msg->log_domain || msg->details || msg->process || msg->pid)
    {
      GtkWidget *exp = gtk_expander_new (_("Details:"));
      gtk_widget_show (exp);
      gtk_table_attach (GTK_TABLE (table), exp,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 5);
      row++;
      GString *gstring = g_string_new (msg->details);
      while (gstring->len && gstring->str[gstring->len - 1] == '\n')
        g_string_erase (gstring, gstring->len - 1, 1);
      g_string_add_format (gstring, "\n\n");
      if (hastext (msg->process))
        g_string_add_format (gstring, _("Process: %s\n"), msg->process);
      if (hastext (msg->log_domain) && !hastext (msg->process))
        g_string_add_format (gstring, _("Origin:  %s\n"), msg->log_domain);
      if (msg->pid && BST_DVL_HINTS)
          g_string_add_format (gstring, _("PID:     %u\n"), msg->pid);
      while (gstring->len && gstring->str[gstring->len - 1] == '\n')
        g_string_erase (gstring, gstring->len - 1, 1);
      gchar *text = adapt_message_spacing (NULL, gstring->str, NULL);
      g_string_free (gstring, TRUE);
      GtkWidget *label = (GtkWidget*) g_object_new (GTK_TYPE_LABEL, "visible", TRUE, "wrap", TRUE, "xalign", 0.0, "label", text, "selectable", TRUE, NULL);
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
      GtkWidget *space = (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT, "visible", TRUE, NULL);
      gtk_table_attach (GTK_TABLE (table), space,
                        1, 2, row, row + 1, /* left/right, top/bottom */
                        GtkAttachOptions (0), GTK_EXPAND, 0, 0);
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

void
bst_msg_bit_free (BstMsgBit *mbit)
{
  g_free (mbit->text);
  g_free (mbit->stock_icon);
  g_free (mbit->options);
  g_free (mbit);
}

BstMsgBit*
bst_msg_bit_create (guint8 msg_part_id, const std::string &text)
{
  int saved_errno = errno;
  /* construct message */
  BstMsgBit *mbit = g_new0 (BstMsgBit, 1);
  mbit->id = msg_part_id;
  mbit->text = g_strdup (text.c_str());
  mbit->stock_icon = NULL;
  mbit->options = NULL;
  errno = saved_errno;
  return mbit;
}

BstMsgBit*
bst_msg_bit_create_choice (guint                   id,
                           const gchar            *name,
                           const gchar            *stock_icon,
                           const gchar            *options)
{
  int saved_errno = errno;
  // assert_return (options && options[0], NULL);
  BstMsgBit *mbit = g_new0 (BstMsgBit, 1);
  mbit->id = id;
  mbit->text = g_strdup (name);
  mbit->stock_icon = g_strdup (stock_icon);
  mbit->options = g_strdup (options);
  errno = saved_errno;
  return mbit;
}

static void
message_dialog_choice_triggered (GtkWidget *choice,
                                 gpointer   data)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (choice);
  if (GXK_IS_DIALOG (toplevel))
    g_object_set_data ((GObject*) toplevel, "bst-modal-choice-result", data);
  gxk_toplevel_delete (choice);
}

static void
repeat_dialog (GxkDialog *dialog)
{
  GtkLabel *label = (GtkLabel*) g_object_get_data ((GObject*) dialog, "BEAST-user-message-repeater");
  if (label)
    {
      gint count = g_object_get_int (dialog, "BEAST-user-message-count");
      gchar *rstr = g_strdup_format (dngettext (bst_gettext_domain(), _("Message has been repeated %u time"), _("Message has been repeated %u times"), count), count);
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
      const gchar *hk = (const gchar*) g_object_get_data ((GObject*) slist->data, "BEAST-message-hashkey");
      if (hk && strcmp (hk, mid) == 0)
        {
          widget = (GtkWidget*) slist->data;
          break;
        }
    }
  g_free (mid);
  return widget;
}

static void
dialog_show_above_modals (GxkDialog *dialog,
                          gboolean   must_return_visible)
{
  /* if a grab is in effect, we need to override it */
  GtkWidget *grab = gtk_grab_get_current();
  if (grab)
    {
      gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    }
  gtk_widget_show (GTK_WIDGET (dialog));
}

guint
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
  if (!msg.config_check && msg.type == BST_MSG_DEBUG)
    msg.config_check = _("Display dialogs with debugging messages");
  /* check the simple non-choice dialog types */
  GxkDialog *dialog;
  if (!msg.n_msg_bits)
    {
      dialog = (GxkDialog*) find_dialog (msg_windows, &msg);
      if (dialog)
        {
          repeat_dialog (dialog);
          return 0;
        }
      else if (msg.config_check && bst_msg_absorb_config_match (msg.config_check))
        {
          bst_msg_absorb_config_update (msg.config_check); /* message absorbed by configuration */
          return 0;
        }
    }
  /* create new dialog */
  dialog = (GxkDialog*) gxk_dialog_new (NULL, NULL, GxkDialogFlags (0), NULL, NULL);
  gxk_dialog_set_sizes (dialog, -1, -1, 512, -1);
  bst_msg_dialog_update (dialog, &msg, TRUE); /* deletes actions */
  g_object_connect (dialog, "signal::destroy", dialog_destroyed, NULL, NULL);
  msg_windows = g_slist_prepend (msg_windows, dialog);
  /* add choices */
  guint j;
  for (j = 0; j < msg.n_msg_bits; j++)
    {
      const BstMsgBit *mbit = msg.msg_bits[j];
      GtkWidget *widget = gxk_dialog_action_multi (dialog, mbit->text, (void*) message_dialog_choice_triggered, (void*) size_t (mbit->id), mbit->stock_icon,
                                                   mbit->options && strchr (mbit->options, 'D') ? GXK_DIALOG_MULTI_DEFAULT : GxkDialogMultiFlags (0));
      if (mbit->options && strchr (mbit->options, 'I'))
        gtk_widget_set_sensitive (widget, FALSE);
    }
  /* fire up dialog */
  guint result = 0;
  if (!msg.n_msg_bits)
    {
      gxk_dialog_add_flags (dialog, GXK_DIALOG_DELETE_BUTTON);
      dialog_show_above_modals (dialog, FALSE);
    }
  else
    {
      g_object_set_data ((GObject*) dialog, "bst-modal-choice-result", (gpointer) 0);
      gxk_dialog_add_flags (dialog, GXK_DIALOG_POPUP_POS | GXK_DIALOG_MODAL);
      g_object_ref (dialog);
      dialog_show_above_modals (dialog, TRUE);
      while (GTK_WIDGET_VISIBLE (dialog))
        {
          GDK_THREADS_LEAVE ();
          g_main_iteration (TRUE);
          GDK_THREADS_ENTER ();
        }
      result = size_t (g_object_get_data ((GObject*) dialog, "bst-modal-choice-result"));
      g_object_unref (dialog);
    }
  return result;
}

static char*
text_concat (char *prefix,
             char *text)
{
  char *result = g_strconcat (prefix ? prefix : "", prefix && text ? "\n" : "", text, NULL);
  g_free (prefix);
  return result;
}


/**
 * bst_message_dialog_display
 * @param log_domain   log domain
 * @param mtype        one of %BST_MSG_ERROR, %BST_MSG_WARNING, %BST_MSG_INFO, %BST_MSG_DIAG
 * @param n_bits       number of message bits
 * @param bits         message bits from bst_msg_bit_printf
 *
 * Present a message dialog to the user. The current value of errno
 * is preserved around calls to this function. Usually this function isn't
 * used directly, but bst_msg_dialog() is called instead which does not require
 * %NULL termination of its argument list and automates the @a log_domain argument.
 * The @a log_domain indicates the calling module and relates to %G_LOG_DOMAIN
 * as used by g_log().
 * The msg bit arguments passed in form various parts of the log message, the
 * following macro set is provided to construct the parts from printf-style
 * argument lists:
 * - BST_MSG_TITLE(): format message title
 * - BST_MSG_TEXT1(): format primary message (also BST_MSG_PRIMARY())
 * - BST_MSG_TEXT2(): format secondary message, optional (also BST_MSG_SECONDARY())
 * - BST_MSG_TEXT3(): format details of the message, optional (also BST_MSG_DETAIL())
 * - BST_MSG_CHECK(): format configuration check statement to enable/disable log messages of this type.
 * - BST_MSG_CHOICE():   add buttons other than cancel to the message dialog
 * - BST_MSG_CHOICE_D(): same as BST_MSG_CHOICE(), for default buttons
 * - BST_MSG_CHOICE_S(): same as BST_MSG_CHOICE(), for insensitive buttons
 * This function is MT-safe and may be called from any thread.
 */
guint
bst_message_dialog_display (const char     *log_domain,
                            BstMsgType      mtype, /* BST_MSG_DEBUG is not really useful here */
                            guint           n_bits,
                            BstMsgBit     **bits)
{
  gint saved_errno = errno;
  BstMessage msg = { 0, };
  msg.log_domain = log_domain;
  msg.type = mtype;
  msg.ident = bst_msg_type_ident (mtype);
  msg.label = bst_msg_type_ident (mtype);
  msg.process = g_strdup (Bse::this_thread_get_name().c_str());
  msg.pid = Bse::this_thread_gettid();
  msg.n_msg_bits = 0;
  msg.msg_bits = NULL;
  /* collect msg bits */
  guint i;
  for (i = 0; i < n_bits; i++)
    {
      BstMsgBit *mbit = bits[i];
      if (mbit->options) /* choice bit */
        {
          msg.msg_bits = g_renew (BstMsgBit*, msg.msg_bits, msg.n_msg_bits + 1);
          msg.msg_bits[msg.n_msg_bits++] = mbit;
          continue;
        }
      switch (mbit->id)
        {
        case '0':
          msg.title = text_concat ((char*) msg.title, mbit->text);
          break;
        case '1':
          msg.primary = text_concat ((char*) msg.primary, mbit->text);
          break;
        case '2':
          msg.secondary = text_concat ((char*) msg.secondary, mbit->text);
          break;
        case '3':
          msg.details = text_concat ((char*) msg.details, mbit->text);
          break;
        case 'c':
          msg.config_check = text_concat ((char*) msg.config_check, mbit->text);
          break;
        }
      bst_msg_bit_free (mbit);
    }
  guint result = bst_message_handler (&msg);
  g_free ((char*) msg.title);
  g_free ((char*) msg.primary);
  g_free ((char*) msg.secondary);
  g_free ((char*) msg.details);
  g_free ((char*) msg.config_check);
  for (i = 0; i < msg.n_msg_bits; i++)
    bst_msg_bit_free (msg.msg_bits[i]);
  g_free (msg.msg_bits);
  errno = saved_errno;
  return result;
}

void
bst_message_dialogs_popdown (void)
{
  while (msg_windows)
    gtk_widget_destroy ((GtkWidget*) msg_windows->data);
}

static void
server_user_message (const Bse::UserMessage &umsg)
{
  auto convert_msg_type = [] (Bse::UserMessageType mtype) {
    switch (mtype)
      {
      case Bse::UserMessageType::ERROR:          return BST_MSG_ERROR;
      case Bse::UserMessageType::WARNING:        return BST_MSG_WARNING;
      case Bse::UserMessageType::DEBUG:          return BST_MSG_DEBUG;
      default:
      case Bse::UserMessageType::INFO:           return BST_MSG_INFO;
      }
  };
  BstMessage msg = { 0, };
  msg.log_domain = "BSE";
  msg.type = convert_msg_type (umsg.utype);
  msg.title = umsg.title.c_str();
  msg.primary = umsg.text1.c_str();
  msg.secondary = umsg.text2.c_str();
  msg.details = umsg.text3.c_str();
  Bse::String cfg = Bse::string_format (_("Show messages about %s"), umsg.label.c_str());
  msg.config_check = cfg.c_str();
  const Aida::EnumValue ev = Aida::enum_info<Bse::UserMessageType>().find_value (umsg.utype);
  msg.ident = ev.ident;
  msg.label = NULL;
  msg.process = 0;
  msg.pid = 0;
  bst_message_handler (&msg);
}

void
bst_message_connect_to_server (void)
{
  // FIXME: bse_server.sig_user_message() += server_user_message;
  bse_proxy_set (BSE_SERVER, "log-messages", FALSE, NULL);
}
