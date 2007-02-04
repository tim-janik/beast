/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998-2002 Tim Janik and Red Hat, Inc.
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
#include "bstxkb.h"

#include "topconfig.h"
#include <string.h>

#ifdef	BST_WITH_XKB

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>


/* --- variables --- */
static Display   *bst_xkb_display = NULL;
static XkbDescPtr bst_xkb_desc = NULL;


/* --- functions --- */
gboolean
bst_xkb_open (const gchar *const_display,
	      gboolean     sync)
{
  gchar *display = (gchar*) const_display;
  int ext_base_event, ext_base_error, ext_status;
  int ext_major = XkbMajorVersion;
  int ext_minor = XkbMinorVersion;

  g_return_val_if_fail (display != NULL, FALSE);
  g_return_val_if_fail (bst_xkb_display == NULL, FALSE);

  bst_xkb_display = XkbOpenDisplay (display,
				    &ext_base_event,
				    &ext_base_error,
				    &ext_major,
				    &ext_minor,
				    &ext_status);
  /* possible values for ext_status:
   *   XkbOD_BadLibraryVersion library version mismatch
   *   XkbOD_ConnectionRefused unable to open display
   *   XkbOD_BadServerVersion  server version mismatch
   *   XkbOD_NonXkbServer      XKB extension not present
   *   XkbOD_Success           guess what
   */
  if (bst_xkb_display && sync)
    XSynchronize (bst_xkb_display, True);
  if (bst_xkb_display && ext_status == XkbOD_Success)
    {
      bst_xkb_desc = XkbGetKeyboard (bst_xkb_display,
				     XkbAllComponentsMask,
				     XkbUseCoreKbd);
      if (bst_xkb_desc && (!bst_xkb_desc->names || !bst_xkb_desc->geom))
	{
	  XkbFreeKeyboard (bst_xkb_desc,
			   XkbAllComponentsMask,
			   True);
	  bst_xkb_desc = NULL;
	}
    }
  if (bst_xkb_display && !bst_xkb_desc)
    {
      XCloseDisplay (bst_xkb_display);
      bst_xkb_display = NULL;
    }

  return bst_xkb_desc != NULL;
}

void
bst_xkb_close (void)
{
  g_return_if_fail (bst_xkb_display != NULL);

  XkbFreeKeyboard (bst_xkb_desc, XkbAllComponentsMask, True);
  bst_xkb_desc = NULL;
  XCloseDisplay (bst_xkb_display);
  bst_xkb_display = NULL;
}

const gchar*
bst_xkb_get_symbol (gboolean physical)
{
  gchar *name;

  g_return_val_if_fail (bst_xkb_desc != NULL, NULL);

  if (physical)
    name = bst_xkb_desc->names->phys_symbols ? XGetAtomName (bst_xkb_display, bst_xkb_desc->names->phys_symbols) : "";
  else
    name = bst_xkb_desc->names->symbols ? XGetAtomName (bst_xkb_display, bst_xkb_desc->names->symbols) : "";

  return name;
}

void
bst_xkb_dump (void)
{
  g_return_if_fail (bst_xkb_desc != NULL);
  
  g_message ("XKB: keycodes: %s types: %s "
	     "symbols: %s phys_symbols: %s "
	     "geo-name: %s",
	     bst_xkb_desc->names->keycodes ? XGetAtomName (bst_xkb_display, bst_xkb_desc->names->keycodes) : "<>",
	     bst_xkb_desc->names->types ? XGetAtomName (bst_xkb_display, bst_xkb_desc->names->types) : "<>",
	     bst_xkb_desc->names->symbols ? XGetAtomName (bst_xkb_display, bst_xkb_desc->names->symbols) : "<>",
	     bst_xkb_desc->names->phys_symbols ? XGetAtomName (bst_xkb_display, bst_xkb_desc->names->phys_symbols) : "<>",
	     bst_xkb_desc->geom->name ? XGetAtomName (bst_xkb_display, bst_xkb_desc->geom->name) : "<>");
  
  /* Tim Janik <timj@gtk.org>:
   *   keycodes: xfree86 types: complete
   *   symbols: en_US(pc102)_de(nodeadkeys) phys_symbols: en_US(pc102)_de(nodeadkeys)
   *   geo-name: pc(pc102)
   * Francisco Bustamante <bit@linuxfan.com>:
   *   keycodes: xfree86 types: complete
   *   symbols: en_US(pc102)_es phys_symbols: en_US(pc102)_es
   *   geo-name: pc(pc101)
   * Will LaShell <wlashell@cland.net>:
   *   keycodes: xfree86 types: complete
   *   symbols: us(pc101) phys_symbols: us(pc101)
   *   geo-name: pc(pc101)
   * Craig Small <csmall@debian.org>:
   *   keycodes: xfree86 types: default
   *   symbols: us(pc105) phys_symbols: us(pc105)
   *   geo-name: pc
   * James Atwill <james-gnome@tainted.org> (laptop):
   *   keycodes: xfree86 types: complete
   *   symbols: us(pc101) phys_symbols: us(pc101)
   *   geo-name: pc(pc101)
   */
}

#else  /* !BST_WITH_XKB */
gboolean
bst_xkb_open  (const gchar *display,
	       gboolean     sync)
{
  return FALSE;
}
void
bst_xkb_close (void)
{
}
void
bst_xkb_dump (void)
{
}
const gchar*
bst_xkb_get_symbol (gboolean physical)
{
  return NULL;
}

#endif /* !BST_WITH_XKB */

void
bst_xkb_parse_symbol (const gchar *const_symbol,
		      gchar      **encoding_p,
		      gchar      **model_p,
		      gchar      **layout_p,
		      gchar      **variant_p)
{
  gchar *s, *e, *symbol = const_symbol ? (gchar*) const_symbol : "";

  e = symbol + strlen (symbol);

  s = strchr (symbol, '(');
  if (encoding_p)
    *encoding_p = s ? g_strndup (symbol, s - symbol) : *symbol ? g_strdup (symbol) : NULL;
  symbol = s ? s + 1 : e;
  s = strchr (symbol, ')');
  if (model_p)
    *model_p = s ? g_strndup (symbol, s - symbol) : NULL;
  symbol = s ? s + 1 : e;
  if (*symbol == '_' || *symbol == '+')
    symbol++;
  s = strchr (symbol, '(');
  if (layout_p)
    *layout_p = s ? g_strndup (symbol, s - symbol) : *symbol ? g_strdup (symbol) : NULL;;
  symbol = s ? s + 1 : e;
  s = strchr (symbol, ')');
  if (variant_p)
    *variant_p = s ? g_strndup (symbol, s - symbol) : NULL;
}
