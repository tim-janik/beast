/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "gxkstatusbar.hh"

#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <string.h>
#include "gxkdialog.hh"


#define LONGEST_TIMEOUT         (2147483647 /* 2^31-1 */)
#define PERC_CMP(f1, f2)        (fabs ((f1) - (f2)) < 1e-7)

/* --- prototypes --- */
static GxkStatusBar*    status_bar_get_current  (void);
static void             status_bar_queue_clear  (GxkStatusBar   *sbar,
                                                 guint           msecs);
static void             status_bar_set          (GxkStatusBar   *sbar,
                                                 gfloat          percentage,
                                                 const gchar    *message,
                                                 const gchar    *status_msg);


/* --- variables --- */
static GQuark     quark_status_bar = 0;
static GSList    *status_window_stack = NULL;
static gboolean   error_bell_enabled = TRUE;

/* --- functions --- */
void
gxk_status_enable_error_bell (gboolean enable_error_bell)
{
  error_bell_enabled = enable_error_bell != FALSE;
}

static void
status_bar_remove_timer (GxkStatusBar *sbar)
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
  GxkStatusBar *sbar = (GxkStatusBar*) data;
  
  status_bar_remove_timer (sbar);
  g_object_unref (sbar->pbar);
  g_object_unref (sbar->message);
  g_object_unref (sbar->status);
  g_free (sbar);
}

/**
 * @return		status bar container
 *
 * Create a status bar suitable to be packed into windows
 * with status bar support.
 */
GtkWidget*
gxk_status_bar_create (void)
{
  GtkWidget *obox, *hbox;
  GxkStatusBar *sbar = g_new0 (GxkStatusBar, 1);
  
  if (!quark_status_bar)
    quark_status_bar = g_quark_from_static_string ("GxkStatusBar");
  
  sbar->sbar = (GtkWidget*) g_object_new (GTK_TYPE_FRAME,
                                          "visible", FALSE,
                                          "shadow", GTK_SHADOW_OUT,
                                          NULL);
  obox = (GtkWidget*) g_object_new (GTK_TYPE_HBOX,
                                    "visible", TRUE,
                                    "homogeneous", FALSE,
                                    "resize_mode", GTK_RESIZE_QUEUE,
                                    "width_request", 110, /* squeeze labels into available space */
                                    "spacing", 0,
                                    "border_width", 1,
                                    "parent", sbar->sbar,
                                    NULL);
  sbar->pbar = (GtkProgressBar*) g_object_new (GTK_TYPE_PROGRESS_BAR,
                                               "visible", TRUE,
                                               "width_request", 100,
                                               NULL);
  sbar->prog = GTK_PROGRESS (sbar->pbar);
  gtk_progress_bar_set_pulse_step (sbar->pbar, 0.01);   /* per pulse increment */
  gtk_progress_bar_set_activity_blocks (sbar->pbar, 4); /* pbar length divider */
  gtk_progress_set_format_string (sbar->prog, "%p %%");
  gtk_progress_set_show_text (sbar->prog, TRUE);
  gtk_box_pack_start (GTK_BOX (obox), GTK_WIDGET (sbar->pbar), FALSE, TRUE, 0);
  
  hbox = (GtkWidget*) g_object_new (GTK_TYPE_HBOX,
                                    "visible", TRUE,
                                    "homogeneous", FALSE,
                                    "parent", g_object_new (GTK_TYPE_FRAME,
                                                            "shadow", GTK_SHADOW_IN,
                                                            "visible", TRUE,
                                                            "parent", obox,
                                                            NULL),
                                    NULL);
  sbar->message = (GtkLabel*) g_object_new (GTK_TYPE_LABEL,
                                            "visible", TRUE,
                                            "xalign", 0.0,
                                            NULL);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (sbar->message), TRUE, TRUE, GTK_STYLE_THICKNESS (hbox->style, y));
  sbar->status = (GtkLabel*) g_object_new (GTK_TYPE_LABEL,
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
  GxkStatusBar *sbar = (GxkStatusBar*) data;

  GDK_THREADS_ENTER ();
  sbar->timer_id = 0;
  status_bar_set (sbar, GXK_STATUS_IDLE, NULL, NULL);
  GDK_THREADS_LEAVE ();
  
  return FALSE;
}

static void
status_bar_queue_clear (GxkStatusBar *sbar,
                        guint         msecs)
{
  status_bar_remove_timer (sbar);
  
  if (!msecs)
    status_bar_set (sbar, GXK_STATUS_IDLE, NULL, NULL);
  else
    sbar->timer_id = g_timeout_add (msecs, status_bar_clear_handler, sbar);
}

/**
 * Clear the current status bar.
 */
void
gxk_status_clear (void)
{
  GxkStatusBar *sbar = status_bar_get_current ();
  
  if (sbar)
    status_bar_queue_clear (sbar, 0);
}

static void
status_bar_set (GxkStatusBar *sbar,
                gfloat        percentage,
                const gchar  *message,
                const gchar  *status_msg)
{
  guint clear_timeout = 7 * 1000;       /* default clearance timeout */
  const char *format = "   ";           /* idle progress format */
  gfloat fraction = 0;
  gboolean activity_pulse = FALSE;
  gboolean beep = FALSE;                /* flag beeps for errors */
  
  if (PERC_CMP (percentage, GXK_STATUS_IDLE_HINT) && !sbar->is_idle)
    return;             /* don't override existing status */
  if (!message) /* clear */
    percentage = GXK_STATUS_IDLE;
  
  sbar->is_idle = FALSE;
  if (PERC_CMP (percentage, GXK_STATUS_IDLE))
    {
      sbar->is_idle = TRUE;
      clear_timeout = 0;        /* no timeout */
      format = "...Idle...";
      message = NULL;
      status_msg = NULL;
    }
  else if (PERC_CMP (percentage, GXK_STATUS_IDLE_HINT))
    {
      sbar->is_idle = TRUE;
      format = "...Idle...";
    }
  else if (PERC_CMP (percentage, GXK_STATUS_ERROR))
    {
      clear_timeout = 25 * 1000;        /* error clearance timeout */
      format = "Error:";
      beep = TRUE;
    }
  else if (PERC_CMP (percentage, GXK_STATUS_WAIT))
    {
      clear_timeout = 0;        /* no timeout */
      format = "<>";
    }
  else if (PERC_CMP (percentage, GXK_STATUS_PROGRESS))
    {
      activity_pulse = TRUE;
    }
  else if (PERC_CMP (percentage, GXK_STATUS_DONE))
    {
      format = "%P %%";
      fraction = 1.0;
    }
  else  /* percentage should be 0..<100 */
    {
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
  if (beep && error_bell_enabled)
    {
#if GTK_CHECK_VERSION (2, 12, 0)
      if (sbar->sbar->window)
        gdk_window_beep (sbar->sbar->window);
      else
        gdk_beep();
#else
      gdk_beep();
#endif
    }
}

/**
 * @param percentage	progress percentage
 * @param message	message to be displayed
 * @param status_msg	error status
 *
 * Set the current status bar message, progress percentage
 * (usually 0% - 100% or one of the special values:
 * GXK_STATUS_ERROR, GXK_STATUS_WAIT, GXK_STATUS_IDLE,
 * GXK_STATUS_IDLE_HINT or GXK_STATUS_PROGRESS) and
 * error status.
 */
void
gxk_status_set (gfloat       percentage,
                const gchar *message,
                const gchar *status_msg)
{
  GxkStatusBar *sbar = status_bar_get_current ();
  
  if (sbar)
    status_bar_set (sbar, percentage, message, status_msg);
}

/**
 * @param percentage	progress percentage
 * @param status_msg	error status
 * @param message_fmt	printf style message to be displayed
 *
 * Similar to gxk_status_set() but supports construction of
 * the message through a printf(3) style argument list.
 */
void
gxk_status_printf (gfloat       percentage,
                   const gchar *status_msg,
                   const gchar *message_fmt,
                   ...)
{
  GxkStatusBar *sbar = status_bar_get_current ();
  
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

/**
 * @param libc_errno	errno value
 * @param message_fmt	printf style message to be displayed
 *
 * Similar to gxk_status_printf() but figures the error
 * status automatically from the passed in @a libc_errno.
 */
void
gxk_status_errnoprintf (gint         libc_errno,
			const gchar *message_fmt,
			...)
{
  GxkStatusBar *sbar = status_bar_get_current ();
  
  if (sbar)
    {
      gchar *buffer;
      va_list args;
      
      va_start (args, message_fmt);
      buffer = g_strdup_vprintf (message_fmt, args);
      va_end (args);

      if (libc_errno)
	status_bar_set (sbar, GXK_STATUS_ERROR, buffer, g_strerror (libc_errno));
      else
	status_bar_set (sbar, GXK_STATUS_DONE, buffer, NULL);
      g_free (buffer);
    }
}

/**
 * @param widget	status bar window
 *
 * Push a window onto the stack of windows that have
 * the current status bar.
 */
void
gxk_status_window_push (gpointer widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  widget = gtk_widget_get_toplevel ((GtkWidget*) widget);
  g_return_if_fail (GTK_IS_WINDOW (widget) == TRUE);
  
  gtk_widget_ref ((GtkWidget*) widget);
  status_window_stack = g_slist_prepend (status_window_stack, widget);
}

/**
 * Pop the most recently pushed window from the status bar
 * window stack.
 */
void
gxk_status_window_pop (void)
{
  g_return_if_fail (status_window_stack != NULL);
  
  gtk_widget_unref ((GtkWidget*) status_window_stack->data);
  status_window_stack = g_slist_remove (status_window_stack, status_window_stack->data);
}

static GxkStatusBar*
status_bar_get_current (void)
{
  GxkDialog *dialog;
  GSList *slist;
  
  for (slist = status_window_stack; slist; slist = slist->next)
    {
      dialog = GXK_DIALOG (slist->data);
      
      if (dialog->status_bar && GTK_WIDGET_DRAWABLE (dialog->status_bar))
        return (GxkStatusBar*) g_object_get_qdata (G_OBJECT (dialog->status_bar), quark_status_bar);
    }
  dialog = gxk_dialog_get_status_window ();
  
  return dialog ? (GxkStatusBar*) g_object_get_qdata (G_OBJECT (dialog->status_bar), quark_status_bar) : NULL;
}
