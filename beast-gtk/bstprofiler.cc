/* BEAST - Better Audio System
 * Copyright (C) 2004 Tim Janik
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
#include "bstprofiler.h"
#include <string.h>


/* --- thread view --- */
enum {
  TCOL_NAME,
  TCOL_TID,
  TCOL_PROC,
  TCOL_PRIO,
  TCOL_PERC,
  TCOL_UTIME,
  TCOL_STIME,
  N_TCOLS
};


/* --- variables --- */
static GtkWidget     *profiler_dialog = NULL;
static GxkRadget     *profiler = NULL;
static guint          timer_id = 0;
static guint          n_thread_infos = 0;
static BseThreadInfo *thread_infos = NULL;


/* --- funtions --- */
#if 0
static gchar
char_state_from_thread_state (BseThreadState thread_state)
{
  switch (thread_state)
    {
    default:
    case BSE_THREAD_STATE_UNKNOWN:      return SFI_THREAD_UNKNOWN;
    case BSE_THREAD_STATE_RUNNING:      return SFI_THREAD_RUNNING;
    case BSE_THREAD_STATE_SLEEPING:     return SFI_THREAD_SLEEPING;
    case BSE_THREAD_STATE_DISKWAIT:     return SFI_THREAD_DISKWAIT;
    case BSE_THREAD_STATE_TRACED:       return SFI_THREAD_TRACED;
    case BSE_THREAD_STATE_PAGING:       return SFI_THREAD_PAGING;
    case BSE_THREAD_STATE_ZOMBIE:       return SFI_THREAD_ZOMBIE;
    case BSE_THREAD_STATE_DEAD:         return SFI_THREAD_DEAD;
    }
}
#endif

static BseThreadState
thread_state_from_char_state (gchar thread_state)
{
  switch (thread_state)
    {
    default:
    case SFI_THREAD_UNKNOWN:    return BSE_THREAD_STATE_UNKNOWN;
    case SFI_THREAD_RUNNING:    return BSE_THREAD_STATE_RUNNING;
    case SFI_THREAD_SLEEPING:   return BSE_THREAD_STATE_SLEEPING;
    case SFI_THREAD_DISKWAIT:   return BSE_THREAD_STATE_DISKWAIT;
    case SFI_THREAD_TRACED:     return BSE_THREAD_STATE_TRACED;
    case SFI_THREAD_PAGING:     return BSE_THREAD_STATE_PAGING;
    case SFI_THREAD_ZOMBIE:     return BSE_THREAD_STATE_ZOMBIE;
    case SFI_THREAD_DEAD:       return BSE_THREAD_STATE_DEAD;
    }
}

static void
thread_info_cell_fill_value (GtkWidget *profiler,
                             guint      column,
                             guint      row,
                             GValue    *value)
{
  BseThreadInfo *info = thread_infos + row;
  switch (column)
    {
    case TCOL_NAME:
      sfi_value_take_string (value, g_strdup_printf ("%s", info->name));
      break;
    case TCOL_PROC:
      sfi_value_take_string (value, info->processor ? g_strdup_printf ("%d", info->processor) : g_strdup (""));
      break;
    case TCOL_TID:
      sfi_value_take_string (value, info->thread_id ? g_strdup_printf ("%u", info->thread_id) : g_strdup (""));
      break;
    case TCOL_PRIO:
      sfi_value_take_string (value, info->thread_id ? g_strdup_printf ("%d", info->priority) : g_strdup (""));
      break;
    case TCOL_PERC:
      sfi_value_take_string (value, g_strdup_printf ("%5.2f%%", (info->utime + info->stime) * 0.0001));
      break;
    case TCOL_UTIME:
      sfi_value_take_string (value, g_strdup_printf ("%7.3f", info->utime * 0.001));
      break;
    case TCOL_STIME:
      sfi_value_take_string (value, g_strdup_printf ("%7.3f", info->stime * 0.001));
      break;
    }
}

static void
update_infos (GSList         *slist,
              GxkListWrapper *lw)
{
  guint i, n = g_slist_length (slist) + 1;
  while (n_thread_infos > n)
    {
      BseThreadInfo *info = thread_infos + --n_thread_infos;
      g_free (info->name);
      gxk_list_wrapper_notify_delete (lw, n_thread_infos);
    }
  if (n > n_thread_infos)
    {
      thread_infos = g_renew (BseThreadInfo, thread_infos, n);
      guint delta = n - n_thread_infos;
      memset (thread_infos + n_thread_infos, 0, sizeof (thread_infos[0]) * delta);
      n_thread_infos = n;
      gxk_list_wrapper_notify_append (lw, delta);
    }
  BseThreadInfo *totals = thread_infos + n - 1;
  gboolean totals_changed = !totals->name;
  totals->utime = 0;
  totals->stime = 0;
  totals->priority = G_MAXINT;
  for (i = 0; i < n - 1; i++)
    {
      BseThreadInfo *oinfo = thread_infos + i;
      BseThreadInfo *ninfo = (BseThreadInfo*) slist->data;
      slist = slist->next;
      if (!oinfo->name || strcmp (oinfo->name, ninfo->name) ||
          oinfo->thread_id != ninfo->thread_id ||
          oinfo->state != ninfo->state ||
          oinfo->priority != ninfo->priority ||
          oinfo->processor != ninfo->processor ||
          oinfo->utime != ninfo->utime ||
          oinfo->stime != ninfo->stime)
        {
          g_free (oinfo->name);
          oinfo->name = g_strdup (ninfo->name);
          oinfo->thread_id = ninfo->thread_id;
          oinfo->state = ninfo->state;
          oinfo->priority = ninfo->priority;
          oinfo->processor = ninfo->processor;
          oinfo->utime = ninfo->utime;
          oinfo->stime = ninfo->stime;
          gxk_list_wrapper_notify_change (lw, i);
          totals_changed = TRUE;
        }
      totals->utime += oinfo->utime;
      totals->stime += oinfo->stime;
      totals->priority = MIN (oinfo->priority, totals->priority);
    }
  if (totals_changed)
    {
      g_free (totals->name);
      totals->name = g_strdup (_("Totals"));
      totals->state = BSE_THREAD_STATE_RUNNING;
      totals->processor = 0;
      gxk_list_wrapper_notify_change (lw, n - 1);
    }
}

static void
profiler_update (void)
{
  GxkListWrapper *lwrapper = (GxkListWrapper*) g_object_get_data ((GObject*) profiler_dialog, "list-wrapper");
  BseThreadTotals *tt = bse_collect_thread_totals ();
  sfi_thread_sleep (0); /* update accounting for self */
  BirnetThreadInfo *si = sfi_thread_info_collect (sfi_thread_self());
  BseThreadInfo bi = { 0, };
  GSList *slist = NULL;
  guint i;
  bi.name = si->name;
  bi.thread_id = si->thread_id;
  bi.state = thread_state_from_char_state (si->state);
  bi.priority = si->priority;
  bi.processor = si->processor;
  bi.utime = si->utime;
  bi.stime = si->stime;
  bi.cutime = si->cutime;
  bi.cstime = si->cstime;
  for (i = 0; i < tt->synthesis->n_thread_infos; i++)
    slist = g_slist_prepend (slist, tt->synthesis->thread_infos[i]);
  if (tt->sequencer)
    slist = g_slist_prepend (slist, tt->sequencer);
  slist = g_slist_prepend (slist, tt->main);
  slist = g_slist_prepend (slist, &bi);
  update_infos (slist, lwrapper);
  g_slist_free (slist);
  sfi_thread_info_free (si);
}

static gboolean
profiler_timer (gpointer data)
{
  gboolean visible;
  GDK_THREADS_ENTER ();
  visible = profiler_dialog && GTK_WIDGET_VISIBLE (profiler_dialog);
  if (visible)
    profiler_update ();
  else
    timer_id = 0;
  GDK_THREADS_LEAVE ();
  return visible;
}

GtkWidget*
bst_profiler_window_get (void)
{
  if (!profiler)
    {
      profiler = gxk_radget_create ("beast", "profiler", NULL);
      profiler_dialog = (GtkWidget*) gxk_dialog_new (&profiler, NULL,
                                                     GXK_DIALOG_HIDE_ON_DELETE,
                                                     _("Profiler"),
                                                     (GtkWidget*) profiler);
      GxkListWrapper *lwrapper = gxk_list_wrapper_new (N_TCOLS,
                                                       G_TYPE_STRING,   /* TCOL_NAME */
                                                       G_TYPE_STRING,   /* TCOL_TID */
                                                       G_TYPE_STRING,   /* TCOL_PERC */
                                                       G_TYPE_STRING,   /* TCOL_UTIME */
                                                       G_TYPE_STRING,   /* TCOL_STIME */
                                                       G_TYPE_STRING,   /* TCOL_PRIO */
                                                       G_TYPE_STRING    /* TCOL_PROC */
                                                       );
      g_signal_connect_object (lwrapper, "fill-value",
                               G_CALLBACK (thread_info_cell_fill_value),
                               profiler, G_CONNECT_SWAPPED);
      GtkTreeModel *smodel = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (lwrapper));
      g_object_unref (lwrapper);
      GtkTreeView *tview = (GtkTreeView*) gxk_radget_find (profiler, "tree-view");
      gtk_tree_view_set_model (tview, smodel);
      GtkTreeSelection *tsel = gtk_tree_view_get_selection (tview);
      gtk_tree_selection_set_mode (tsel, GTK_SELECTION_NONE);
      g_object_unref (smodel);
      gxk_tree_view_add_text_column (tview, TCOL_NAME, "SAG", 0.0, _("Thread Name"), NULL, NULL, NULL, GConnectFlags (0));
      if (BST_DVL_HINTS)
        gxk_tree_view_add_text_column (tview, TCOL_TID, "SA", 0.5, _("TID"), _("Thread ID (on some systems the process ID)"), NULL, NULL, GConnectFlags (0));
      gxk_tree_view_add_text_column (tview, TCOL_PERC, "SAG", 1.0, _("CPU%"), _("Percentage of CPU usage"), NULL, NULL, GConnectFlags (0));
      gxk_tree_view_add_text_column (tview, TCOL_UTIME, "SAG", 1.0, _("UTime"), _("Average number of milliseconds per second of user CPU time used by thread"), NULL, NULL, GConnectFlags (0));
      gxk_tree_view_add_text_column (tview, TCOL_STIME, "SAG", 1.0, _("STime"), _("Average number of milliseconds per second of system CPU time used by thread"), NULL, NULL, GConnectFlags (0));
      /*
        gxk_tree_view_add_text_column (tview, TCOL_STATE, "F", 0.5, _("S"),
        _("Thread State:\n"
        "'?' - thread state is unknown\n"
        "'R' - thread is running\n"
        "'S' - thread is sleeping\n"
        "'D' - thread waits on disk\n"
        "'T' - thread is traced\n"
        "'W' - thread is swapping memory\n"
        "'Z' - thread died unexpectedly\n"
        "'X' - thread exited"),
        NULL, NULL, 0);
      */
      gxk_tree_view_add_text_column (tview, TCOL_PRIO, "AG", 0.5, _("Nice"), _("Thread priority from -20 (high) to +19 (low)"), NULL, NULL, GConnectFlags (0));
      gxk_tree_view_add_text_column (tview, TCOL_PROC, "A", 0.5, _("CPU#"), _("CPU the thread is currently running on"), NULL, NULL, GConnectFlags (0));
      g_object_set_data ((GObject*) profiler_dialog, "list-wrapper", lwrapper);
      profiler_update ();
    }
  if (!timer_id)
    timer_id = g_timeout_add (500, profiler_timer, NULL);
  return profiler_dialog;
}
