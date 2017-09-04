// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseinstrumentinput.hh"

#include "bsecategories.hh"
#include "bsesnet.hh"
#include "bseengine.hh"

#include <string.h>

/* --- parameters --- */
enum
{
  PROP_0,
  PROP_IPORT_NAME
};


/* --- variables --- */
static void *parent_class = NULL;


/* --- functions --- */
static void
bse_instrument_input_reset_names (BseInstrumentInput *self)
{
  BseSubIPort *iport = BSE_SUB_IPORT (self);
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = item->parent ? BSE_SNET (item->parent) : NULL;
  const char *name;

  g_object_freeze_notify (G_OBJECT (self));
  name = BSE_SOURCE_OCHANNEL_IDENT (self, 0);
  if (strcmp (iport->input_ports[0], name) != 0 &&
      (!snet || !bse_snet_iport_name_registered (snet, name)))
    g_object_set (self, /* no undo */
                  "BseSubIPort::in_port_1", name, NULL);
  name = BSE_SOURCE_OCHANNEL_IDENT (self, 1);
  if (strcmp (iport->input_ports[1], name) != 0 &&
      (!snet || !bse_snet_iport_name_registered (snet, name)))
    g_object_set (self, /* no undo */
                  "BseSubIPort::in_port_2", name, NULL);
  name = BSE_SOURCE_OCHANNEL_IDENT (self, 2);
  if (strcmp (iport->input_ports[2], name) != 0 &&
      (!snet || !bse_snet_iport_name_registered (snet, name)))
    g_object_set (self, /* no undo */
                  "BseSubIPort::in_port_3", name, NULL);
  name = BSE_SOURCE_OCHANNEL_IDENT (self, 3);
  if (strcmp (iport->input_ports[3], name) != 0 &&
      (!snet || !bse_snet_iport_name_registered (snet, name)))
    g_object_set (self, /* no undo */
                  "BseSubIPort::in_port_4", name, NULL);
  g_object_thaw_notify (G_OBJECT (self));
}

static void
bse_instrument_input_init (BseInstrumentInput *self)
{
  bse_instrument_input_reset_names (self);
}

static void
bse_instrument_input_set_parent (BseItem *item,
                                 BseItem *parent)
{
  BseInstrumentInput *self = BSE_INSTRUMENT_INPUT (item);

  if (item->parent)
    g_signal_handlers_disconnect_by_func (item->parent, (void*) bse_instrument_input_reset_names, self);

  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);

  if (item->parent)
    g_signal_connect_swapped (item->parent, "port_unregistered",
			      G_CALLBACK (bse_instrument_input_reset_names), self);
  else
    bse_instrument_input_reset_names (self);
}

static void
bse_instrument_input_get_property (GObject *object, uint param_id, GValue *value, GParamSpec *pspec)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bse_instrument_input_class_init (BseInstrumentInputClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseItemClass *item_class = BSE_ITEM_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  uint i, ochannel_id;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = bse_instrument_input_get_property;
  item_class->set_parent = bse_instrument_input_set_parent;

  /* assert parent class introduced enough ports */
  assert_return (BSE_SUB_IPORT_N_PORTS >= 4);
  /* override parent properties with NOP properties */
  for (i = 0; i < BSE_SUB_IPORT_N_PORTS; i++)
    {
      char *string = g_strdup_format ("in_port_%u", i + 1);
      bse_object_class_add_param (object_class, NULL, PROP_IPORT_NAME + i * 2,
				  sfi_pspec_string (string, NULL, NULL, NULL,
                                                    /* override parent property: 0 */ "r"));
      g_free (string);
    }

  ochannel_id = bse_source_class_add_ochannel (source_class, "frequency", _("Frequency"), _("Note Frequency"));
  assert_return (ochannel_id == BSE_INSTRUMENT_INPUT_OCHANNEL_FREQUENCY);
  ochannel_id = bse_source_class_add_ochannel (source_class, "gate", _("Gate"), _("High if the note is currently being pressed"));
  assert_return (ochannel_id == BSE_INSTRUMENT_INPUT_OCHANNEL_GATE);
  ochannel_id = bse_source_class_add_ochannel (source_class, "velocity", _("Velocity"), _("Velocity of the note press"));
  assert_return (ochannel_id == BSE_INSTRUMENT_INPUT_OCHANNEL_VELOCITY);
  ochannel_id = bse_source_class_add_ochannel (source_class, "aftertouch", _("Aftertouch"), _("Velocity while the note is pressed"));
  assert_return (ochannel_id == BSE_INSTRUMENT_INPUT_OCHANNEL_AFTERTOUCH);
}

BSE_BUILTIN_TYPE (BseInstrumentInput)
{
  static const GTypeInfo type_info = {
    sizeof (BseInstrumentInputClass),

    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_instrument_input_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    sizeof (BseInstrumentInput),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_instrument_input_init,
  };
#include "./icons/keyboard.c"
  GType type = bse_type_register_static (BSE_TYPE_SUB_IPORT,
                                         "BseInstrumentInput",
                                         "Virtual input module for synthesis networks which "
                                         "implement instruments",
                                         __FILE__, __LINE__,
                                         &type_info);
  bse_categories_register_stock_module (N_("/Input & Output/Instrument Voice Input"), type, keyboard_pixstream);
  return type;
}
