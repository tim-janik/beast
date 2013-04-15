// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseinstrumentoutput.hh"

#include "bsecategories.hh"
#include "bsesnet.hh"
#include "bseengine.hh"

#include <string.h>

/* --- parameters --- */
enum
{
  PROP_0,
  PROP_OPORT_NAME
};


/* --- variables --- */
static void *parent_class = NULL;


/* --- functions --- */
static void
bse_instrument_output_reset_names (BseInstrumentOutput *self)
{
  BseSubOPort *oport = BSE_SUB_OPORT (self);
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = item->parent ? BSE_SNET (item->parent) : NULL;
  const char *name;

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
    g_signal_handlers_disconnect_by_func (item->parent, (void*) bse_instrument_output_reset_names, self);

  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);

  if (item->parent)
    g_signal_connect_swapped (item->parent, "port_unregistered",
			      G_CALLBACK (bse_instrument_output_reset_names), self);
  else
    bse_instrument_output_reset_names (self);
}

static void
bse_instrument_output_get_property (GObject *object, uint param_id, GValue *value, GParamSpec *pspec)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_instrument_output_class_init (BseInstrumentOutputClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  uint i, ichannel_id;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = bse_instrument_output_get_property;
  item_class->set_parent = bse_instrument_output_set_parent;

  /* assert parent class introduced enough ports */
  g_assert (BSE_SUB_OPORT_N_PORTS >= 4);
  /* override parent properties with NOP properties */
  for (i = 0; i < BSE_SUB_OPORT_N_PORTS; i++)
    {
      char *string = g_strdup_printf ("out_port_%u", i + 1);
      bse_object_class_add_param (object_class, NULL, PROP_OPORT_NAME + i * 2,
				  sfi_pspec_string (string, NULL, NULL, NULL,
                                                    /* override parent property: 0 */ "r"));
      g_free (string);
    }

  ichannel_id = bse_source_class_add_ichannel (source_class, "left-audio", _("Left Audio"), _("Left Channel Output"));
  g_assert (ichannel_id == BSE_INSTRUMENT_OUTPUT_ICHANNEL_LEFT);
  ichannel_id = bse_source_class_add_ichannel (source_class, "right-audio", _("Right Audio"), _("Right Channel Output"));
  g_assert (ichannel_id == BSE_INSTRUMENT_OUTPUT_ICHANNEL_RIGHT);
  ichannel_id = bse_source_class_add_ichannel (source_class, "unused", _("Unused"), NULL);
  g_assert (ichannel_id == BSE_INSTRUMENT_OUTPUT_ICHANNEL_UNUSED);
  ichannel_id = bse_source_class_add_ichannel (source_class, "synth-done", _("Synth Done"), _("High indicates the instrument is done synthesizing"));
  g_assert (ichannel_id == BSE_INSTRUMENT_OUTPUT_ICHANNEL_DONE);
}

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
#include "./icons/instrument.c"
  GType type = bse_type_register_static (BSE_TYPE_SUB_OPORT,
                                         "BseInstrumentOutput",
                                         "Virtual output module for synthesis networks which "
                                         "implement instruments",
                                         __FILE__, __LINE__,
                                         &type_info);
  bse_categories_register_stock_module (N_("/Input & Output/Instrument Output"), type, instrument_pixstream);
  return type;
}
