/* testgui - GUI test program
 * Copyright (C) 2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "topconfig.h"
#include "bstutils.h"
#include "bse/bse.h"
#include "bstdbmeter.h"

/* --- FIXME: --- */
gboolean            bst_developer_hints = FALSE;
gboolean            bst_debug_extensions = FALSE;
gboolean            bst_main_loop_running = TRUE;
void beast_show_about_box (void) {}
     

/* --- functions --- */
int
main (int   argc,
      char *argv[])
{
  /* initialize i18n */
  bindtextdomain (BST_GETTEXT_DOMAIN, BST_PATH_LOCALE);
  bind_textdomain_codeset (BST_GETTEXT_DOMAIN, "UTF-8");
  textdomain (BST_GETTEXT_DOMAIN);
  setlocale (LC_ALL, "");

  /* initialize GLib */
  g_thread_init (NULL);
  g_type_init ();

  /* initialize Sfi */
  sfi_init ();
  sfi_debug_allow ("misc");
  /* ensure SFI can wake us up */
  sfi_thread_set_name ("TestGUI");
  sfi_thread_set_wakeup ((SfiThreadWakeup) g_main_context_wakeup, g_main_context_default (), NULL);

  /* initialize Gtk+ and enter threading mode */
  gtk_init (&argc, &argv);
  g_set_prgname ("testgui");            /* override Gdk's program name */
  g_set_application_name ("TestGUI");   /* User visible name */
  GDK_THREADS_ENTER ();

  /* initialize Gtk+ Extension Kit */
  gxk_init ();
  /* add documentation search paths */
  gxk_text_add_tsm_path (BST_PATH_DOCS);
  gxk_text_add_tsm_path (BST_PATH_IMAGES);
  gxk_text_add_tsm_path (".");

  /* initialize BEAST GUI components */
  _bst_init_utils ();
  _bst_init_params ();
  _bst_gconfig_init ();
  _bst_skin_config_init ();

  /* start BSE core and connect */
  SfiRec *bseconfig = sfi_rec_new ();
  bse_init_async (&argc, &argv, bseconfig);
  sfi_rec_unref (bseconfig);
  sfi_glue_context_push (bse_init_glue_context ("TestGUI"));
  GSource *source = g_source_simple (G_PRIORITY_HIGH - 100,
                                     (GSourcePending) sfi_glue_context_pending,
                                     (GSourceDispatch) sfi_glue_context_dispatch,
                                     NULL, NULL, NULL);
  g_source_attach (source, NULL);
  g_source_unref (source);

  GtkWidget *dialog = gxk_dialog_new (NULL, NULL, GXK_DIALOG_DELETE_BUTTON, "Test Window", NULL);
  g_object_connect (dialog, "signal::destroy", gtk_main_quit, NULL, NULL);
  g_object_set (dialog,
		"default_width", 25, // 560,
		"default_height", 25, // 640,
		NULL);
  GtkWidget *meter = g_object_new (BST_TYPE_DB_METER, NULL);
  gxk_dialog_set_child (GXK_DIALOG (dialog),
                        g_object_new (GTK_TYPE_ALIGNMENT,
                                      "visible", TRUE,
                                      "border_width", 10,
                                      "child", meter,
                                      NULL));

  gtk_widget_show_now (dialog);
  g_object_set (dialog,
                "height-request", 25,
                "width-request", 25,
		NULL);
  gtk_main ();

  return 0;
}
