// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "topconfig.h"
#include "bstutils.hh"
#include "bse/bse.hh"
#include "bstdbmeter.hh"
#include "bstparam.hh"
#include "bstgconfig.hh"
#include "bstskinconfig.hh"

/* --- FIXME: --- */
gboolean            bst_developer_hints = FALSE;
gboolean            bst_debug_extensions = FALSE;
gboolean            bst_main_loop_running = TRUE;
void beast_show_about_box (void) {}


/* --- functions --- */
static gboolean
change_beam_value (gpointer data)
{
  if (!GTK_WIDGET_DRAWABLE (data))
    {
      g_object_unref (data);
      return FALSE;
    }
  GDK_THREADS_ENTER ();
  BstDBBeam *self = BST_DB_BEAM (data);
  double v = rand() / ((double) RAND_MAX) * (self->dbsetup->maxdb - self->dbsetup->mindb) + self->dbsetup->mindb;
  if (v > self->dbsetup->maxdb)
    v = self->dbsetup->mindb;
  bst_db_beam_set_value (self, v);
  GDK_THREADS_LEAVE ();
  return TRUE;
}

static GtkWidget*
create_db_meter (GtkOrientation  orientation)
{
  GtkWidget *widget = bst_db_meter_new (orientation, 0);
  BstDBMeter *self = BST_DB_METER (widget);
  BstDBBeam *dbbeam;

  bst_db_meter_create_dashes (self, GTK_JUSTIFY_RIGHT, 2);
  bst_db_meter_create_scale (self, 2);
  bst_db_meter_create_dashes (self, GTK_JUSTIFY_FILL, 2);

  dbbeam = bst_db_meter_create_beam (self, 2);
  bst_db_meter_create_dashes (self, GTK_JUSTIFY_LEFT, 2);
  g_timeout_add (50, change_beam_value, g_object_ref (dbbeam));

  bst_db_meter_create_numbers (self, 2);
  bst_db_meter_create_dashes (self, GTK_JUSTIFY_CENTER, 2);

  return widget;
}

static void
build_db_meter_test (GtkBox *box)
{
  GtkWidget *meter;
  meter = bst_db_meter_new (GTK_ORIENTATION_HORIZONTAL, 1);
  bst_db_beam_set_value (bst_db_meter_get_beam ((BstDBMeter*) meter, 0), G_MAXDOUBLE);
  gtk_box_pack_start (box, meter, TRUE, TRUE, 5);
  meter = create_db_meter (GTK_ORIENTATION_VERTICAL);
  gtk_box_pack_start (box, meter, TRUE, TRUE, 5);
  meter = create_db_meter (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (box, meter, FALSE, TRUE, 5);
  meter = bst_db_meter_new (GTK_ORIENTATION_VERTICAL, 2);
  bst_db_beam_set_value (bst_db_meter_get_beam ((BstDBMeter*) meter, 1), G_MAXDOUBLE);
  gtk_box_pack_start (box, meter, TRUE, TRUE, 5);
}

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
  sfi_init (&argc, argv, "TestGUI");
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
  Bse::init_async (&argc, argv, "TestGUI");
  sfi_glue_context_push (Bse::init_glue_context ("TestGUI", []() { g_main_context_wakeup (g_main_context_default()); }));
  GSource *source = g_source_simple (G_PRIORITY_HIGH - 100,
                                     (GSourcePending) sfi_glue_context_pending,
                                     (GSourceDispatch) sfi_glue_context_dispatch,
                                     NULL, NULL, NULL);
  g_source_attach (source, NULL);
  g_source_unref (source);

  GtkWidget *dialog = (GtkWidget*) gxk_dialog_new (NULL, NULL, GXK_DIALOG_DELETE_BUTTON, "Test Window", NULL);
  g_object_connect (dialog, "signal::destroy", gtk_main_quit, NULL, NULL);
  g_object_set (dialog,
		"default_width", 25, // 560,
		"default_height", 25, // 640,
		NULL);
  GtkWidget *box = (GtkWidget*) g_object_new (GTK_TYPE_VBOX, "visible", TRUE, NULL);
  gxk_dialog_set_child (GXK_DIALOG (dialog),
                        (GtkWidget*) g_object_new (GTK_TYPE_ALIGNMENT,
                                                   "visible", TRUE,
                                                   "border_width", 10,
                                                   "child", box,
                                                   NULL));

  build_db_meter_test (GTK_BOX (box));

  gtk_widget_show_now (dialog);
  g_object_set (dialog,
                "height-request", 25,
                "width-request", 25,
		NULL);
  gtk_main ();

  return 0;
}
