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
#include	"bststatusbar.h"

#include	"bstdialog.h"

#include	<gdk/gdkkeysyms.h>
#include	<string.h>

#define	STATUS_LASTS_ms		(7*1000)
#define	STATUS_ERROR_LASTS_ms	(25*1000)

/* --- prototypes --- */
static void	status_bar_remove_timer		(GtkWidget	*sbar);
static gboolean	procedure_share			(gpointer	 func_data,
						 const gchar	*proc_name,
						 gfloat		 proc_progress);
static gboolean	procedure_finished		(gpointer	 func_data,
						 const gchar	*proc_name,
						 BseErrorType	 exit_status);


/* --- variables --- */
static GtkWidget *proc_window = NULL;
static GSList    *status_window_stack = NULL;
static guint      proc_notifier = 0;
static guint	  proc_catch_count = 0;

/* --- functions --- */
void
bst_status_bar_catch_procedure (void)
{
  if (!proc_catch_count++)
    {
      proc_notifier = bse_procedure_notifier_add (procedure_finished, NULL);
      bse_procedure_push_share_hook (procedure_share, NULL);
    }
}

void
bst_status_bar_uncatch_procedure (void)
{
  g_return_if_fail (proc_catch_count > 0);

  if (!--proc_catch_count)
    {
      bse_procedure_pop_share_hook ();
      bse_procedure_notifier_remove (proc_notifier);
      proc_notifier = 0;
    }
}

GtkWidget*
bst_status_bar_get_current (void)
{
  BstDialog *dialog;
  GSList *slist;

  for (slist = status_window_stack; slist; slist = slist->next)
    {
      dialog = BST_DIALOG (slist->data);

      if (dialog->status_bar && GTK_WIDGET_DRAWABLE (dialog->status_bar))
	return dialog->status_bar;
    }
  dialog = bst_dialog_get_status_window ();

  return dialog ? dialog->status_bar : NULL;
}

GtkWidget*
bst_status_bar_from_widget (GtkWidget *widget)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  widget = gtk_widget_get_toplevel (widget);
  if (BST_IS_DIALOG (widget))
    return BST_DIALOG (widget)->status_bar;
  return NULL;
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

static gint
filter_procedure_events (GtkWidget *widget,
			 GdkEvent  *event,
			 GtkWidget *abort)
{
  switch (event->type)
    {
    case GDK_KEY_PRESS:
      if (event->key.keyval == GDK_Escape)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (abort), TRUE);
    case GDK_KEY_RELEASE:
    case GDK_DELETE:
      return TRUE;
    default:
      return FALSE;
    }
}

static gboolean
procedure_share (gpointer     func_data,
		 const gchar *proc_name,
		 gfloat	      proc_progress)
{
  GtkWidget *sbar;
  gboolean should_abort = FALSE;
  
  if (!proc_window)
    {
      sbar = bst_status_bar_get_current ();
      if (sbar)
	{
	  proc_window = gtk_widget_get_ancestor (sbar, GTK_TYPE_WINDOW);
	  if (proc_window)
	    gtk_widget_ref (proc_window);
	}
    }
  sbar = proc_window ? bst_status_bar_from_widget (proc_window) : NULL;
  
  if (sbar)
    {
      GtkObject *object = GTK_OBJECT (sbar);
      GtkWidget *progress = gtk_object_get_data (object, "bst-progress");
      GtkWidget *msg = gtk_object_get_data (object, "bst-msg");
      GtkWidget *status = gtk_object_get_data (object, "bst-status");
      GtkWidget *abort = gtk_object_get_data (object, "bst-abort");
      guint event_signal_handler;
      gchar *text = strrchr (proc_name, ':');
      
      status_bar_remove_timer (sbar);
      
      if (proc_progress < 0)
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progress));
      else
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), proc_progress);
      
      gtk_label_set_text (GTK_LABEL (msg), text ? text + 1 : proc_name);
      gtk_widget_hide (status);
      if (!GTK_WIDGET_VISIBLE (abort))
	{
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (abort), FALSE);
	  gtk_widget_show (abort);
	}
      
      gtk_widget_ref (abort);

      /* prevent delete events */
      event_signal_handler = gtk_signal_connect_after (GTK_OBJECT (proc_window),
						       "event",
						       GTK_SIGNAL_FUNC (filter_procedure_events),
						       abort);
      gtk_grab_add (abort);

      GDK_THREADS_LEAVE ();
      while (g_main_pending ())
	g_main_iteration (FALSE);
      GDK_THREADS_ENTER ();

      gtk_grab_remove (abort);
      gtk_signal_disconnect (GTK_OBJECT (proc_window), event_signal_handler);
      
      should_abort = !GTK_WIDGET_REALIZED (abort) || gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (abort));
      
      gtk_widget_unref (abort);
    }
  
  return should_abort;
}

static gboolean
procedure_finished (gpointer	 func_data,
		    const gchar *proc_name,
		    BseErrorType exit_status)
{
  GtkWidget *sbar;
  
  sbar = proc_window ? bst_status_bar_from_widget (proc_window) : NULL;
  if (!sbar)
    sbar = bst_status_bar_get_current ();
  
  if (sbar)
    {
      gchar *text = strrchr (proc_name, ':');
      
      if (text)
	bst_status_bar_set_permanent (sbar, exit_status ? 0 : 100, text + 1, bse_error_blurb (exit_status));
      else
	bst_status_bar_set_permanent (sbar, exit_status ? 0 : 100, proc_name, bse_error_blurb (exit_status));
      bst_status_bar_queue_clear (sbar, exit_status ? STATUS_ERROR_LASTS_ms : STATUS_LASTS_ms);
    }
  
  if (proc_window)
    {
      gtk_widget_unref (proc_window);
      proc_window = NULL;
    }
  
  return TRUE;
}

GtkWidget*
bst_status_bar_create (void)
{
  GtkWidget *obox, *sbar, *hbox, *prog, *msg, *status, *abort;
  static const gchar *status_bar_rc_string =
    ( "style'BstStatusBar-Abort-style'"
      "{"
      "#font='-misc-fixed-*-*-*-*-*-130-*-*-*-*-*-*'\n"
      "fg[NORMAL]={1.,0.,0.}"
      "fg[ACTIVE]={1.,0.,0.}"
      "fg[PRELIGHT]={1.,0.,0.}"
      "bg[NORMAL]={.7,.7,.7}"
      "}"
      "widget'*.BstStatusBar.*.AbortButton*'style'BstStatusBar-Abort-style'"
      "\n");

  if (status_bar_rc_string)
    {
      gtk_rc_parse_string (status_bar_rc_string);
      status_bar_rc_string = NULL;
    }
  
  sbar = gtk_widget_new (GTK_TYPE_FRAME,
			 "visible", FALSE,
			 "shadow", GTK_SHADOW_OUT,
			 NULL);
  obox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "name", "BstStatusBar",
			 "homogeneous", FALSE,
			 "resize_mode", GTK_RESIZE_QUEUE,
			 "width_request", 110, /* squeeze labels into available space */
			 "spacing", 0,
			 "border_width", 1,
			 "parent", sbar,
			 NULL);
  prog = gtk_widget_new (GTK_TYPE_PROGRESS_BAR,
			 "visible", TRUE,
			 "width_request", 100,
			 NULL);
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (prog), 0.01);	/* per pulse increment */
  gtk_progress_bar_set_activity_blocks (GTK_PROGRESS_BAR (prog), 4);	/* pbar length divider */
  gtk_progress_set_format_string (GTK_PROGRESS (prog), "%p %%");
  gtk_progress_set_show_text (GTK_PROGRESS (prog), TRUE);
  gtk_box_pack_start (GTK_BOX (obox), prog, FALSE, TRUE, 0);
  
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "homogeneous", FALSE,
			 "parent", gtk_widget_new (GTK_TYPE_FRAME,
						   "shadow", GTK_SHADOW_IN,
						   "visible", TRUE,
						   "parent", obox,
						   NULL),
			 NULL);
  
  msg = gtk_widget_new (GTK_TYPE_LABEL,
			"visible", TRUE,
			"xalign", 0.0,
			NULL);
  gtk_box_pack_start (GTK_BOX (hbox), msg, TRUE, TRUE, GTK_STYLE_THICKNESS (hbox->style, y));
  status = gtk_widget_new (GTK_TYPE_LABEL,
			   "xalign", 1.0,
			   NULL);
  gtk_box_pack_end (GTK_BOX (hbox), status, FALSE, TRUE, GTK_STYLE_THICKNESS (hbox->style, y));
  abort = gtk_widget_new (GTK_TYPE_TOGGLE_BUTTON,
			  "label", "Abort",
			  "name", "AbortButton",
			  "height_request", 1, /* squeeze into available space */
			  "can_focus", FALSE,
			  NULL);
  gtk_box_pack_end (GTK_BOX (hbox), abort, FALSE, TRUE, 0);
  
  g_object_set_data_full (G_OBJECT (sbar), "bst-progress", g_object_ref (prog), (GDestroyNotify) g_object_unref);
  g_object_set_data_full (G_OBJECT (sbar), "bst-msg", g_object_ref (msg), (GDestroyNotify) g_object_unref);
  g_object_set_data_full (G_OBJECT (sbar), "bst-status", g_object_ref (status), (GDestroyNotify) g_object_unref);
  g_object_set_data_full (G_OBJECT (sbar), "bst-abort", g_object_ref (abort), (GDestroyNotify) g_object_unref);
  
  gtk_object_ref (GTK_OBJECT (sbar));

  bst_status_bar_queue_clear (sbar, 0);

  return sbar;
}

static void
status_bar_remove_timer (GtkWidget *sbar)
{
  guint timer_id;
  
  timer_id = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (sbar), "bst-status-bar-timer"));
  if (timer_id)
    {
      gtk_object_remove_data (GTK_OBJECT (sbar), "bst-status-bar-timer");
      gtk_timeout_remove (timer_id);
      gtk_object_unref (GTK_OBJECT (sbar));
    }
}

static gboolean
status_bar_queue_clear (GtkWidget *sbar)
{
  bst_status_bar_set_permanent (sbar, 0, NULL, NULL);
  
  return FALSE;
}

void
bst_status_bar_queue_clear (GtkWidget *sbar,
			    guint      msecs)
{
  GtkObject *object;
  guint timer_id;
  
  g_return_if_fail (GTK_IS_WIDGET (sbar));
  
  object = GTK_OBJECT (sbar);
  
  gtk_object_ref (object);
  
  status_bar_remove_timer (sbar);
  
  if (!msecs)
    bst_status_bar_set_permanent (sbar, 0, NULL, NULL);
  else
    {
      gtk_object_ref (object);
      timer_id = gtk_timeout_add (msecs,
				  (GtkFunction) status_bar_queue_clear,
				  sbar);
      gtk_object_set_data (object, "bst-status-bar-timer", GINT_TO_POINTER (timer_id));
    }
  
  gtk_object_unref (object);
}

void
bst_status_bar_set_permanent (GtkWidget	  *sbar,
			      gfloat	   percentage,
			      const gchar *message,
			      const gchar *status_msg)
{
  GtkObject *object;
  
  g_return_if_fail (GTK_IS_WIDGET (sbar));
  
  object = GTK_OBJECT (sbar);
  gtk_object_ref (object);
  
  status_bar_remove_timer (sbar);
  if (sbar)
    {
      GtkWidget *progress = gtk_object_get_data (object, "bst-progress");
      GtkWidget *msg = gtk_object_get_data (object, "bst-msg");
      GtkWidget *status = gtk_object_get_data (object, "bst-status");
      GtkWidget *abort = gtk_object_get_data (object, "bst-abort");
      
      gtk_widget_hide (abort);
      gtk_progress_set_activity_mode (GTK_PROGRESS (progress), FALSE);
      gtk_progress_set_value (GTK_PROGRESS (progress), percentage);
      gtk_label_set_text (GTK_LABEL (msg), message ? message : "...Idle...");
      gtk_label_set_text (GTK_LABEL (status), status_msg);
      if (status_msg)
	gtk_widget_show (status);
      else
	gtk_widget_hide (status);
    }
  
  gtk_object_unref (object);
}

void
bst_status_bar_set (GtkWidget	*sbar,
		    gfloat	 percentage,
		    const gchar *message,
		    const gchar *status_msg)
{
  GtkObject *object;
  
  g_return_if_fail (GTK_IS_WIDGET (sbar));
  
  object = GTK_OBJECT (sbar);
  gtk_object_ref (object);

  bst_status_bar_set_permanent (sbar, percentage, message, status_msg);
  bst_status_bar_queue_clear (sbar, percentage < 99.9999 ? STATUS_ERROR_LASTS_ms : STATUS_LASTS_ms);

  gtk_object_unref (object);
}

void
bst_status_set (gfloat	     percentage,
		const gchar *message,
		const gchar *status_msg)
{
  GtkWidget *sbar = bst_status_bar_get_current ();
  
  if (sbar)
    bst_status_bar_set (sbar, percentage, message, status_msg);
}

void
bst_status_printf (gfloat          percentage,
		   const gchar    *status_msg,
		   const gchar    *message_fmt,
		   ...)
{
  gchar *buffer;
  va_list args;

  va_start (args, message_fmt);
  buffer = g_strdup_vprintf (message_fmt, args);
  va_end (args);

  bst_status_set (percentage, buffer, status_msg);
  g_free (buffer);
}

void
bst_status_clear (void)
{
  GtkWidget *sbar = bst_status_bar_get_current ();
  
  if (sbar)
    bst_status_bar_queue_clear (sbar, 0);
}
