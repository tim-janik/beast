/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000, 2001 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include        "bststatusbar.h"

#include        "bstdialog.h"

#include        <gdk/gdkkeysyms.h>
#include        <string.h>


#define LONGEST_TIMEOUT         (2147483647 /* 2^31-1 */)
#define PERC_CMP(f1, f2)        (fabs ((f1) - (f2)) < 1e-7)


/* --- prototypes --- */
static BstStatusBar*    status_bar_get_current  (void);
static void             status_bar_queue_clear  (BstStatusBar   *sbar,
                                                 guint           msecs);
static void             status_bar_set          (BstStatusBar   *sbar,
                                                 gfloat          percentage,
                                                 const gchar    *message,
                                                 const gchar    *status_msg);


/* --- variables --- */
static GQuark     quark_status_bar = 0;
static GSList    *status_window_stack = NULL;
static guint      proc_catch_count = 0;

/* --- functions --- */
static void
status_bar_remove_timer (BstStatusBar *sbar)
{
  if (sbar->timer_id)
    {
      g_source_remove (sbar->timer_id);
      sbar->timer_id = 0;
    }
}

static void
sbar_free (gpointer data)
{
  BstStatusBar *sbar = data;
  
  status_bar_remove_timer (sbar);
  g_object_unref (sbar->pbar);
  g_object_unref (sbar->message);
  g_object_unref (sbar->status);
  g_free (sbar);
}

GtkWidget*
bst_status_bar_create (void)
{
  GtkWidget *obox, *hbox;
  BstStatusBar *sbar = g_new0 (BstStatusBar, 1);
  
  if (!quark_status_bar)
    quark_status_bar = g_quark_from_static_string ("BstStatusBar");
  
  sbar->sbar = g_object_new (GTK_TYPE_FRAME,
                             "visible", FALSE,
                             "shadow", GTK_SHADOW_OUT,
                             NULL);
  obox = g_object_new (GTK_TYPE_HBOX,
                       "visible", TRUE,
                       "homogeneous", FALSE,
                       "resize_mode", GTK_RESIZE_QUEUE,
                       "width_request", 110, /* squeeze labels into available space */
                       "spacing", 0,
                       "border_width", 1,
                       "parent", sbar->sbar,
                       NULL);
  sbar->pbar = g_object_new (GTK_TYPE_PROGRESS_BAR,
                             "visible", TRUE,
                             "width_request", 100,
                             NULL);
  sbar->prog = GTK_PROGRESS (sbar->pbar);
  gtk_progress_bar_set_pulse_step (sbar->pbar, 0.01);   /* per pulse increment */
  gtk_progress_bar_set_activity_blocks (sbar->pbar, 4); /* pbar length divider */
  gtk_progress_set_format_string (sbar->prog, "%p %%");
  gtk_progress_set_show_text (sbar->prog, TRUE);
  gtk_box_pack_start (GTK_BOX (obox), GTK_WIDGET (sbar->pbar), FALSE, TRUE, 0);
  
  hbox = g_object_new (GTK_TYPE_HBOX,
                       "visible", TRUE,
                       "homogeneous", FALSE,
                       "parent", g_object_new (GTK_TYPE_FRAME,
                                               "shadow", GTK_SHADOW_IN,
                                               "visible", TRUE,
                                               "parent", obox,
                                               NULL),
                       NULL);
  sbar->message = g_object_new (GTK_TYPE_LABEL,
                                "visible", TRUE,
                                "xalign", 0.0,
                                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (sbar->message), TRUE, TRUE, GTK_STYLE_THICKNESS (hbox->style, y));
  sbar->status = g_object_new (GTK_TYPE_LABEL,
                               "xalign", 1.0,
                               NULL);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (sbar->status), FALSE, TRUE, GTK_STYLE_THICKNESS (hbox->style, y));
  
  g_object_ref (sbar->pbar);
  g_object_ref (sbar->message);
  g_object_ref (sbar->status);
  sbar->is_idle = FALSE;
  sbar->timer_id = 0;
  status_bar_queue_clear (sbar, 0);
  g_object_set_qdata_full (G_OBJECT (sbar->sbar), quark_status_bar, sbar, sbar_free);
  
  return sbar->sbar;
}

static gboolean
status_bar_clear_handler (gpointer data)
{
  BstStatusBar *sbar = data;

  GDK_THREADS_ENTER ();
  sbar->timer_id = 0;
  status_bar_set (sbar, BST_STATUS_IDLE, NULL, NULL);
  GDK_THREADS_LEAVE ();
  
  return FALSE;
}

static void
status_bar_queue_clear (BstStatusBar *sbar,
                        guint         msecs)
{
  status_bar_remove_timer (sbar);
  
  if (!msecs)
    status_bar_set (sbar, BST_STATUS_IDLE, NULL, NULL);
  else
    sbar->timer_id = g_timeout_add (msecs, status_bar_clear_handler, sbar);
}

void
bst_status_clear (void)
{
  BstStatusBar *sbar = status_bar_get_current ();
  
  if (sbar)
    status_bar_queue_clear (sbar, 0);
}

static void
status_bar_set (BstStatusBar *sbar,
                gfloat        percentage,
                const gchar  *message,
                const gchar  *status_msg)
{
  guint clear_timeout = 7 * 1000;       /* default clearance timeout */
  gchar *format = "   ";                /* idle progress format */
  gfloat fraction = 0;
  gboolean activity_pulse = FALSE;
  gboolean beep = FALSE;                /* flag beeps for errors */
  
  if (PERC_CMP (percentage, BST_STATUS_IDLE_HINT) && !sbar->is_idle)
    return;             /* don't override existing status */
  if (!message) /* clear */
    percentage = BST_STATUS_IDLE;
  
  sbar->is_idle = FALSE;
  if (PERC_CMP (percentage, BST_STATUS_IDLE))
    {
      sbar->is_idle = TRUE;
      clear_timeout = 0;        /* no timeout */
      format = "...Idle...";
      message = NULL;
      status_msg = NULL;
    }
  else if (PERC_CMP (percentage, BST_STATUS_IDLE_HINT))
    {
      sbar->is_idle = TRUE;
      format = "...Idle...";
    }
  else if (PERC_CMP (percentage, BST_STATUS_ERROR))
    {
      clear_timeout = 25 * 1000;        /* error clearance timeout */
      format = "Error:";
      beep = TRUE;
    }
  else if (PERC_CMP (percentage, BST_STATUS_WAIT))
    {
      clear_timeout = 0;        /* no timeout */
      format = "<>";
    }
  else if (PERC_CMP (percentage, BST_STATUS_PROGRESS))
    {
      clear_timeout = 0;        /* no timeout */
      activity_pulse = TRUE;
    }
  else if (PERC_CMP (percentage, BST_STATUS_DONE))
    {
      format = "%P %%";
      fraction = 1.0;
    }
  else  /* percentage should be 0..<100 */
    {
      clear_timeout = 0;        /* no timeout */
      format = "%P %%";
      fraction = CLAMP (percentage, 0, 100) / 100.0;
    }
  gtk_progress_set_format_string (sbar->prog, format);
  if (activity_pulse)
    gtk_progress_bar_pulse (sbar->pbar);
  else
    gtk_progress_bar_set_fraction (sbar->pbar, fraction);
  gtk_label_set_text (sbar->message, message);
  gtk_label_set_text (sbar->status, status_msg);
  if (status_msg)
    gtk_widget_show (GTK_WIDGET (sbar->status));
  else
    gtk_widget_hide (GTK_WIDGET (sbar->status));
  if (clear_timeout)
    status_bar_queue_clear (sbar, clear_timeout);
  else
    status_bar_remove_timer (sbar);
  if (beep)
    gdk_beep ();
}

void
bst_status_set (gfloat       percentage,
                const gchar *message,
                const gchar *status_msg)
{
  BstStatusBar *sbar = status_bar_get_current ();
  
  if (sbar)
    status_bar_set (sbar, percentage, message, status_msg);
}

void
bst_status_printf (gfloat       percentage,
                   const gchar *status_msg,
                   const gchar *message_fmt,
                   ...)
{
  BstStatusBar *sbar = status_bar_get_current ();
  
  if (sbar)
    {
      gchar *buffer;
      va_list args;
      
      va_start (args, message_fmt);
      buffer = g_strdup_vprintf (message_fmt, args);
      va_end (args);
      
      status_bar_set (sbar, percentage, buffer, status_msg);
      g_free (buffer);
    }
}

void
bst_status_eprintf (BswErrorType error,
		    const gchar *message_fmt,
		    ...)
{
  BstStatusBar *sbar = status_bar_get_current ();
  
  if (sbar)
    {
      gchar *buffer;
      va_list args;
      
      va_start (args, message_fmt);
      buffer = g_strdup_vprintf (message_fmt, args);
      va_end (args);

      if (error)
	status_bar_set (sbar, BST_STATUS_ERROR, buffer, bsw_error_blurb (error));
      else
	status_bar_set (sbar, BST_STATUS_DONE, buffer, NULL);
      g_free (buffer);
    }
}

void
bst_status_errnoprintf (gint         libc_errno,
			const gchar *message_fmt,
			...)
{
  BstStatusBar *sbar = status_bar_get_current ();
  
  if (sbar)
    {
      gchar *buffer;
      va_list args;
      
      va_start (args, message_fmt);
      buffer = g_strdup_vprintf (message_fmt, args);
      va_end (args);

      if (libc_errno)
	status_bar_set (sbar, BST_STATUS_ERROR, buffer, g_strerror (libc_errno));
      else
	status_bar_set (sbar, BST_STATUS_DONE, buffer, NULL);
      g_free (buffer);
    }
}

void
bst_status_window_push (gpointer widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  widget = gtk_widget_get_toplevel (widget);
  g_return_if_fail (GTK_IS_WINDOW (widget) == TRUE);
  
  gtk_widget_ref (widget);
  status_window_stack = g_slist_prepend (status_window_stack, widget);
}

void
bst_status_window_pop (void)
{
  g_return_if_fail (status_window_stack != NULL);
  
  gtk_widget_unref (status_window_stack->data);
  status_window_stack = g_slist_remove (status_window_stack, status_window_stack->data);
}

static BstStatusBar*
status_bar_get_current (void)
{
  BstDialog *dialog;
  GSList *slist;
  
  for (slist = status_window_stack; slist; slist = slist->next)
    {
      dialog = BST_DIALOG (slist->data);
      
      if (dialog->status_bar && GTK_WIDGET_DRAWABLE (dialog->status_bar))
        return g_object_get_qdata (G_OBJECT (dialog->status_bar), quark_status_bar);
    }
  dialog = bst_dialog_get_status_window ();
  
  return dialog ? g_object_get_qdata (G_OBJECT (dialog->status_bar), quark_status_bar) : NULL;
}

static void
script_status (BseServer      *server,
	       BswScriptStatus status,
	       const gchar    *script_name,
	       gfloat          progress,
	       BseErrorType    error,
	       gpointer        data)
{
  switch (status)
    {
    case BSW_SCRIPT_STATUS_START:
      bst_status_set (0, script_name, NULL);
      break;
    case BSW_SCRIPT_STATUS_PROGRESS:
      if (progress < 0)
	bst_status_set (BST_STATUS_PROGRESS, script_name, NULL);
      else
	bst_status_set (progress * 100.0, script_name, NULL);
      break;
    case BSW_SCRIPT_STATUS_END:
      bst_status_eprintf (error, script_name);
      break;
    case BSW_SCRIPT_STATUS_PROC_END:	/* single procedure script */
      bst_status_eprintf (error, script_name);
      break;
    }
}

void
bst_status_bar_catch_script (void)
{
  if (!proc_catch_count++)
    bsw_proxy_connect (BSW_SERVER,
		       "signal::script_status", script_status, NULL,
		       NULL);
}

void
bst_status_bar_uncatch_script (void)
{
  g_return_if_fail (proc_catch_count > 0);
  
  if (!--proc_catch_count)
    bsw_proxy_disconnect (BSW_SERVER,
			  "any_signal", script_status, NULL,
			  NULL);
}
