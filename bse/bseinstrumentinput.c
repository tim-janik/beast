/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2002 Tim Janik
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
#include "bsesubkeyboard.h"

#include "bsecategories.h"
#include "bsesnet.h"
#include "gslengine.h"

#include <string.h>

/* --- parameters --- */
enum
{
  PROP_0,
  PROP_IPORT_NAME
};


/* --- prototypes --- */
static void	 bse_sub_keyboard_init			(BseSubKeyboard		*self);
static void	 bse_sub_keyboard_class_init		(BseSubKeyboardClass	*class);
static void	 bse_sub_keyboard_set_parent		(BseItem		*item,
							 BseItem		*parent);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
#include "./icons/keyboard.c"
BSE_BUILTIN_TYPE (BseSubKeyboard)
{
  static const GTypeInfo type_info = {
    sizeof (BseSubKeyboardClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_sub_keyboard_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSubKeyboard),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_sub_keyboard_init,
  };
  static const BsePixdata pixdata = {
    KEYBOARD_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    KEYBOARD_IMAGE_WIDTH, KEYBOARD_IMAGE_HEIGHT,
    KEYBOARD_IMAGE_RLE_PIXEL_DATA,
  };
  guint type_id;
  
  type_id = bse_type_register_static (BSE_TYPE_SUB_IPORT,
				      "BseSubKeyboard",
				      "Virtual input module for synthesis networks which "
				      "implement instruments",
				      &type_info);
  bse_categories_register_icon ("/Modules/Virtualization/Keyboard Input",
				type_id,
				&pixdata);
  return type_id;
}

static void
bse_sub_keyboard_class_init (BseSubKeyboardClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  BseSubIPortClass *iport_class = BSE_SUB_IPORT_CLASS (class);
  guint i, ochannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  item_class->set_parent = bse_sub_keyboard_set_parent;
  
  /* override parent properties with NOP properties */
  for (i = 0; i < iport_class->n_input_ports; i++)
    {
      gchar *string;
      
      string = g_strdup_printf ("in_port_%u", i + 1);
      bse_object_class_add_param (object_class, NULL, PROP_IPORT_NAME + i * 2,
				  sfi_pspec_string (string, NULL, NULL, NULL, 0));
      g_free (string);
    }
  
  /* assert parent class introduced enough ports */
  g_assert (iport_class->n_input_ports >= 4);
  
  iport_class->n_input_ports = 4;
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "Frequency", "Note Frequency");
  g_assert (ochannel_id == BSE_SUB_KEYBOARD_OCHANNEL_FREQUENCY);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Gate", "High if the note is currently being pressed");
  g_assert (ochannel_id == BSE_SUB_KEYBOARD_OCHANNEL_GATE);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Velocity", "Velocity of the note press");
  g_assert (ochannel_id == BSE_SUB_KEYBOARD_OCHANNEL_VELOCITY);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Aftertouch", NULL);
  g_assert (ochannel_id == BSE_SUB_KEYBOARD_OCHANNEL_AFTERTOUCH);
}

static void
bse_sub_keyboard_reset_names (BseSubKeyboard *self)
{
  BseSubIPort *iport = BSE_SUB_IPORT (self);
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = item->parent ? BSE_SNET (item->parent) : NULL;
  const gchar *name;
  
  g_object_freeze_notify (G_OBJECT (self));
  name = BSE_SOURCE_OCHANNEL_IDENT (self, 0);
  if (strcmp (iport->input_ports[0], name) != 0 &&
      (!snet || !bse_snet_iport_name_registered (snet, name)))
    g_object_set (self, "BseSubIPort::in_port_1", name, NULL);
  name = BSE_SOURCE_OCHANNEL_IDENT (self, 1);
  if (strcmp (iport->input_ports[1], name) != 0 &&
      (!snet || !bse_snet_iport_name_registered (snet, name)))
    g_object_set (self, "BseSubIPort::in_port_2", name, NULL);
  name = BSE_SOURCE_OCHANNEL_IDENT (self, 2);
  if (strcmp (iport->input_ports[2], name) != 0 &&
      (!snet || !bse_snet_iport_name_registered (snet, name)))
    g_object_set (self, "BseSubIPort::in_port_3", name, NULL);
  name = BSE_SOURCE_OCHANNEL_IDENT (self, 3);
  if (strcmp (iport->input_ports[3], name) != 0 &&
      (!snet || !bse_snet_iport_name_registered (snet, name)))
    g_object_set (self, "BseSubIPort::in_port_4", name, NULL);
  g_object_thaw_notify (G_OBJECT (self));
}

static void
bse_sub_keyboard_init (BseSubKeyboard *self)
{
  bse_sub_keyboard_reset_names (self);
}

static void
bse_sub_keyboard_set_parent (BseItem *item,
			     BseItem *parent)
{
  BseSubKeyboard *self = BSE_SUB_KEYBOARD (item);
  
  if (item->parent)
    g_signal_handlers_disconnect_by_func (item->parent, bse_sub_keyboard_reset_names, self);
  
  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);
  
  if (item->parent)
    g_signal_connect_swapped (item->parent, "port_unregistered",
			      G_CALLBACK (bse_sub_keyboard_reset_names), self);
  else
    bse_sub_keyboard_reset_names (self);
}
