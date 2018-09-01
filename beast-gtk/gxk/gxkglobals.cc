// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gxkglobals.hh"
#include "sfi/private.hh"

/* --- variables --- */
const GxkGlobals* gxk_globals = NULL;

/* --- functions --- */
void
gxk_init (void)
{
  static GxkGlobals globals = { NULL, };

  assert_return (gxk_globals == NULL);

  gxk_globals = &globals;

  globals.tooltips = gtk_tooltips_new ();
  g_object_ref (globals.tooltips);
  gtk_object_sink (GTK_OBJECT (globals.tooltips));

  gxk_init_utils ();
  gxk_init_params ();
  gxk_init_stock ();
  gxk_init_actions ();
  gxk_init_assortments ();
  gxk_init_radget_types ();
}

gulong
gxk_nullify_in_object (gpointer object,
                       gpointer _location)
{
  GObject **location = (GObject**) _location;
  GClosure *closure;
  assert_return (object != NULL, 0);
  assert_return (location != NULL, 0);
  assert_return (G_IS_OBJECT (object), 0);
  assert_return (GTK_IS_OBJECT (*location), 0);
  closure = g_cclosure_new_swap (G_CALLBACK (g_nullify_pointer), location, NULL);
  g_object_watch_closure ((GObject*) object, closure);
  return g_signal_connect_closure (*location, "destroy", closure, 0);
}
