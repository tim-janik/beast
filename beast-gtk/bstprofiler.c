/* BEAST - Bedevilled Audio System
 * Copyright (C) 2004 Tim Janik
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
#include "bstprofiler.h"

/* --- variables --- */
static GtkWidget *profiler = NULL;
static guint      timer_id = 0;

/* --- funtions --- */
static gboolean
profile_timer (gpointer data)
{
  gboolean visible;
  GDK_THREADS_ENTER ();
  visible = profiler && GTK_WIDGET_VISIBLE (profiler);
  if (visible)
    {
      GtkLabel *textfield = gxk_gadget_find (profiler, "textfield");
      BseThreadTotals *tt = bse_collect_thread_totals ();
      sfi_thread_sleep (0); /* update accounting for self */
      SfiThreadInfo *si = sfi_thread_info_collect (sfi_thread_self());
      GString *gstring = g_string_new ("");
      g_string_printfa (gstring, "%7.5f %7.5f %5.2f%% %35s\n",
                        si->utime * 0.000001, si->stime * 0.000001,
                        (si->utime + si->stime) * 0.0001,
                        si->name);
      g_string_printfa (gstring, "%7.5f %7.5f %5.2f%% %35s\n",
                        tt->main->utime * 0.000001, tt->main->stime * 0.000001,
                        (tt->main->utime + tt->main->stime) * 0.0001,
                        tt->main->name);
      if (tt->sequencer)
        g_string_printfa (gstring, "%7.5f %7.5f %5.2f%% %35s\n",
                          tt->sequencer->utime * 0.000001, tt->sequencer->stime * 0.000001,
                          (tt->sequencer->utime + tt->sequencer->stime) * 0.0001,
                          tt->sequencer->name);
      if (tt->synthesis->n_thread_infos)
        g_string_printfa (gstring, "%7.5f %7.5f %5.2f%% %35s\n",
                          tt->synthesis->thread_infos[0]->utime * 0.000001,
                          tt->synthesis->thread_infos[0]->stime * 0.000001,
                          (tt->synthesis->thread_infos[0]->utime +
                           tt->synthesis->thread_infos[0]->stime) * 0.0001,
                          tt->synthesis->thread_infos[0]->name);
      sfi_thread_info_free (si);
      g_object_set (textfield, "label", gstring->str, NULL);
      g_string_free (gstring, TRUE);
    }
  else
    timer_id = 0;
  GDK_THREADS_LEAVE ();
  return visible;
}

GtkWidget*
bst_profiler_window_get (void)
{
  if (!profiler)
    profiler = gxk_dialog_new (&profiler, NULL,
                               GXK_DIALOG_HIDE_ON_DELETE,
                               _("Profiler"),
                               gxk_gadget_create ("beast", "profiler", NULL));
  if (!timer_id)
    timer_id = g_timeout_add (500, profile_timer, NULL);
  return profiler;
}
