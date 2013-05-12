// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstprofiler.hh"
#include "bse/bse.hh"
#include <string.h>

using Rapicorn::TaskStatus;     // FIXME

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
static std::vector<TaskStatus> cached_task_list;

/* --- funtions --- */
static void
thread_info_cell_fill_value (GtkWidget *profiler,
                             guint      column,
                             guint      row,
                             GValue    *value)
{
  g_return_if_fail (row < cached_task_list.size());
  TaskStatus *info = &cached_task_list[row];
  switch (column)
    {
    case TCOL_NAME:
      sfi_value_take_string (value, g_strdup_format ("%s", info->name.c_str()));
      break;
    case TCOL_PROC:
      sfi_value_take_string (value, info->processor ? g_strdup_format ("%d", info->processor) : g_strdup (""));
      break;
    case TCOL_TID:
      sfi_value_take_string (value, info->task_id ? g_strdup_format ("%u", info->task_id) : g_strdup (""));
      break;
    case TCOL_PRIO:
      sfi_value_take_string (value, info->task_id ? g_strdup_format ("%d", info->priority) : g_strdup (""));
      break;
    case TCOL_PERC:
      sfi_value_take_string (value, g_strdup_format ("%5.2f%%", (info->utime + info->stime) * 0.0001));
      break;
    case TCOL_UTIME:
      sfi_value_take_string (value, g_strdup_format ("%7.3f", info->utime * 0.001));
      break;
    case TCOL_STIME:
      sfi_value_take_string (value, g_strdup_format ("%7.3f", info->stime * 0.001));
      break;
    }
}

static void
profiler_update (void)
{
  GxkListWrapper *lwrapper = (GxkListWrapper*) g_object_get_data ((GObject*) profiler_dialog, "list-wrapper");
  // update and fetch stats
  Bse::TaskRegistry::update();
  std::vector<TaskStatus> tasks = Bse::TaskRegistry::list();
  cached_task_list = tasks;                             // keep local copy for list wrapper updates
  cached_task_list.push_back (TaskStatus (0, 0));       // add one spare for Totals
  // update stats in list
  if (cached_task_list.size() != lwrapper->n_rows)
    {
      for (size_t i = cached_task_list.size(); i < lwrapper->n_rows; i++)
        gxk_list_wrapper_notify_delete (lwrapper, lwrapper->n_rows - 1);
      if (cached_task_list.size() > lwrapper->n_rows)
        gxk_list_wrapper_notify_append (lwrapper, cached_task_list.size() - lwrapper->n_rows);
    }
  TaskStatus &totals = cached_task_list[cached_task_list.size() - 1];
  totals.name = _("Totals");
  totals.priority = G_MAXINT;
  totals.state = TaskStatus::RUNNING;
  totals.processor = 0;
  for (size_t i = 0; i < tasks.size(); i++)
    {
      const TaskStatus &ninfo = tasks[i];
      gxk_list_wrapper_notify_change (lwrapper, i);
      totals.utime += ninfo.utime;
      totals.stime += ninfo.stime;
      totals.priority = MIN (ninfo.priority, totals.priority);
    }
  gxk_list_wrapper_notify_change (lwrapper, cached_task_list.size() - 1);
}

static gboolean
profiler_timer (gpointer data)
{
  bool visible;
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
