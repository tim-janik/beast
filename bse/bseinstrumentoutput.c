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
#include "bseinstrumentoutput.h"

#include "bsecategories.h"
#include "bsesnet.h"
#include "gslengine.h"

#include <string.h>

/* --- parameters --- */
enum
{
  PROP_0,
  PROP_OPORT_NAME
};


/* --- prototypes --- */
static void bse_instrument_output_init       (BseInstrumentOutput      *self);
static void bse_instrument_output_class_init (BseInstrumentOutputClass *class);
static void bse_instrument_output_set_parent (BseItem                  *item,
                                              BseItem                  *parent);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
#include "./icons/instrument.c"
BSE_BUILTIN_TYPE (BseInstrumentOutput)
{
  static const GTypeInfo type_info = {
    sizeof (BseInstrumentOutputClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_instrument_output_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseInstrumentOutput),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_instrument_output_init,
  };
  static const BsePixdata pixdata = {
    INSTRUMENT_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    INSTRUMENT_IMAGE_WIDTH, INSTRUMENT_IMAGE_HEIGHT,
    INSTRUMENT_IMAGE_RLE_PIXEL_DATA,
  };
  GType type = bse_type_register_static (BSE_TYPE_SUB_OPORT,
                                         "BseInstrumentOutput",
                                         "Virtual output module for synthesis networks which "
                                         "implement instruments",
                                         &type_info);
  bse_categories_register_icon ("/Modules/Input & Output/Instrument Output", type, &pixdata);
  return type;
}

static void
bse_instrument_output_class_init (BseInstrumentOutputClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  BseSubOPortClass *oport_class = BSE_SUB_OPORT_CLASS (class);
  guint i, ichannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  item_class->set_parent = bse_instrument_output_set_parent;
  
  /* override parent properties with NOP properties */
  for (i = 0; i < oport_class->n_output_ports; i++)
    {
      gchar *string;
      
      string = g_strdup_printf ("out_port_%u", i + 1);
      bse_object_class_add_param (object_class, NULL, PROP_OPORT_NAME + i * 2,
				  sfi_pspec_string (string, NULL, NULL, NULL, NULL));
      g_free (string);
    }
  
  /* assert parent class introduced enough ports */
  g_assert (oport_class->n_output_ports >= 4);
  
  oport_class->n_output_ports = 4;
  
  ichannel_id = bse_source_class_add_ichannel (source_class, "left-audio", _("Left Audio"), _("Left Channel Output"));
  g_assert (ichannel_id == BSE_INSTRUMENT_OUTPUT_ICHANNEL_LEFT);
  ichannel_id = bse_source_class_add_ichannel (source_class, "right-audio", _("Right Audio"), _("Right Channel Output"));
  g_assert (ichannel_id == BSE_INSTRUMENT_OUTPUT_ICHANNEL_RIGHT);
  ichannel_id = bse_source_class_add_ichannel (source_class, "unused", _("Unused"), NULL);
  g_assert (ichannel_id == BSE_INSTRUMENT_OUTPUT_ICHANNEL_UNUSED);
  ichannel_id = bse_source_class_add_ichannel (source_class, "synth-done", _("Synth Done"), _("High indicates the instrument is done synthesizing"));
  g_assert (ichannel_id == BSE_INSTRUMENT_OUTPUT_ICHANNEL_DONE);
}

static void
bse_instrument_output_reset_names (BseInstrumentOutput *self)
{
  BseSubOPort *oport = BSE_SUB_OPORT (self);
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = item->parent ? BSE_SNET (item->parent) : NULL;
  const gchar *name;
  
  g_object_freeze_notify (G_OBJECT (self));
  name = BSE_SOURCE_ICHANNEL_IDENT (self, 0);
  if (strcmp (oport->output_ports[0], name) != 0 &&
      (!snet || !bse_snet_oport_name_registered (snet, name)))
    g_object_set (self, /* no undo */
                  "BseSubOPort::out_port_1", name, NULL);
  name = BSE_SOURCE_ICHANNEL_IDENT (self, 1);
  if (strcmp (oport->output_ports[1], name) != 0 &&
      (!snet || !bse_snet_oport_name_registered (snet, name)))
    g_object_set (self, /* no undo */
                  "BseSubOPort::out_port_2", name, NULL);
  name = BSE_SOURCE_ICHANNEL_IDENT (self, 2);
  if (strcmp (oport->output_ports[2], name) != 0 &&
      (!snet || !bse_snet_oport_name_registered (snet, name)))
    g_object_set (self, /* no undo */
                  "BseSubOPort::out_port_3", name, NULL);
  name = BSE_SOURCE_ICHANNEL_IDENT (self, 3);
  if (strcmp (oport->output_ports[3], name) != 0 &&
      (!snet || !bse_snet_oport_name_registered (snet, name)))
    g_object_set (self, /* no undo */
                  "BseSubOPort::out_port_4", name, NULL);
  g_object_thaw_notify (G_OBJECT (self));
}

static void
bse_instrument_output_init (BseInstrumentOutput *self)
{
  bse_instrument_output_reset_names (self);
}


static void
bse_instrument_output_set_parent (BseItem *item,
                                  BseItem *parent)
{
  BseInstrumentOutput *self = BSE_INSTRUMENT_OUTPUT (item);
  
  if (item->parent)
    g_signal_handlers_disconnect_by_func (item->parent, bse_instrument_output_reset_names, self);
  
  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);
  
  if (item->parent)
    g_signal_connect_swapped (item->parent, "port_unregistered",
			      G_CALLBACK (bse_instrument_output_reset_names), self);
  else
    bse_instrument_output_reset_names (self);
}
