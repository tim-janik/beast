/* tsmview.c - TagSpanMarkup Viewer
 * Copyright (C) 2002 Tim Janik
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
#include "bsttexttools.h"
#include "bstdialog.h"


#define PRGNAME "tsmview"



/* --- variables --- */
gboolean          bst_dvl_hints = FALSE;


/* --- functions --- */
int
main (int   argc,
      char *argv[])
{
  GtkWidget *sctext, *dialog;
  gchar *str, *title;
  guint i;

  if (argc < 2)
    {
      g_printerr (PRGNAME ": missing filename argument\n");
      return 1;
    }

  /* initialize modules
   */
  g_type_init ();
  _bst_utils_init ();
  bst_init_gentypes ();
  gtk_init (&argc, &argv);

  title = g_strdup (argv[1]);
  bst_text_add_tsm_path (".");
  sctext = bst_scroll_text_create (BST_TEXT_VIEW_SHEET_BG | BST_TEXT_VIEW_NAVIGATABLE, NULL);
  bst_scroll_text_enter (sctext, argv[1]);
  for (i = 2; i < argc; i++)
    {
      bst_scroll_text_enter (sctext, argv[i]);  // FIXME: should append
      str = title;
      title = g_strconcat (title, " ", argv[i], NULL);
      g_free (str);
    }

  str = title;
  title = g_strdup ("tsmview");	// g_strconcat (title, " - tsmview", NULL);
  g_free (str);

  dialog = bst_dialog_new (NULL, NULL, BST_DIALOG_DELETE_BUTTON, title, NULL);
  g_free (title);

  g_object_connect (dialog, "signal::destroy", gtk_main_quit, NULL, NULL);

  bst_dialog_set_child (BST_DIALOG (dialog), sctext);

  g_object_set (dialog,
		"default_width", 560,
		"default_height", 640,
		"visible", TRUE,
		NULL);
  gtk_main ();

  return 0;
}
