/* GXK - Gtk+ Extension Kit
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
#include "gxkglobals.h"


/* --- variables --- */
const GxkGlobals* gxk_globals = NULL;


/* --- functions --- */
void
gxk_init (void)
{
  static GxkGlobals globals = { NULL, };

  g_return_if_fail (gxk_globals == NULL);

  gxk_globals = &globals;

  globals.tooltips = gtk_tooltips_new ();
  g_object_ref (globals.tooltips);
  gtk_object_sink (GTK_OBJECT (globals.tooltips));

  _gxk_init_utils ();
  _gxk_init_params ();
  _gxk_init_stock ();
  _gxk_init_actions ();
  _gxk_init_radget_types ();
}

gulong
gxk_nullify_in_object (gpointer object,
                       gpointer _location)
{
  GObject **location = _location;
  GClosure *closure;
  g_return_val_if_fail (object != NULL, 0);
  g_return_val_if_fail (location != NULL, 0);
  g_return_val_if_fail (G_IS_OBJECT (object), 0);
  g_return_val_if_fail (GTK_IS_OBJECT (*location), 0);
  closure = g_cclosure_new_swap (G_CALLBACK (g_nullify_pointer), location, NULL);
  g_object_watch_closure (object, closure);
  return g_signal_connect_closure (*location, "destroy", closure, 0);
}
