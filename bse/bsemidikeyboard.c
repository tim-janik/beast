/* BseMidiKeyboard - BSE MIDI destination module
 * Copyright (C) 1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bsemidikeyboard.h"

#include "bsecategories.h"
#include "bsesnet.h"
#include "./icons/keyboard.c"
#include "gslengine.h"



/* --- parameters --- */
enum
{
  PARAM_0
};


/* --- prototypes --- */
static void	 bse_midi_keyboard_init			(BseMidiKeyboard	*scard);
static void	 bse_midi_keyboard_class_init		(BseMidiKeyboardClass	*class);
static void	 bse_midi_keyboard_class_finalize	(BseMidiKeyboardClass	*class);
static void	 bse_midi_keyboard_set_property		(BseMidiKeyboard	*scard,
							 guint			 param_id,
							 GValue			*value,
							 GParamSpec		*pspec,
							 const gchar		*trailer);
static void	 bse_midi_keyboard_get_property		(BseMidiKeyboard	*scard,
							 guint			 param_id,
							 GValue			*value,
							 GParamSpec		*pspec,
							 const gchar		*trailer);
static void	 bse_midi_keyboard_do_destroy		(BseObject		*object);
static void	 bse_midi_keyboard_set_parent		(BseItem		*item,
							 BseItem		*parent);
static void	 bse_midi_keyboard_prepare		(BseSource		*source);
static void	 bse_midi_keyboard_context_create	(BseSource		*source,
							 guint			 instance_id,
							 GslTrans		*trans);
static void	 bse_midi_keyboard_reset		(BseSource		*source);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseMidiKeyboard)
{
  static const GTypeInfo midi_keyboard_info = {
    sizeof (BseMidiKeyboardClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_keyboard_class_init,
    (GClassFinalizeFunc) bse_midi_keyboard_class_finalize,
    NULL /* class_data */,
    
    sizeof (BseMidiKeyboard),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_midi_keyboard_init,
  };
  static const BsePixdata pixdata = {
    KEYBOARD_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    KEYBOARD_IMAGE_WIDTH, KEYBOARD_IMAGE_HEIGHT,
    KEYBOARD_IMAGE_RLE_PIXEL_DATA,
  };
  guint midi_keyboard_type_id;
  
  midi_keyboard_type_id = bse_type_register_static (BSE_TYPE_SOURCE,
						    "BseMidiKeyboard",
						    "Virtual input module for synthesis networks which are "
						    "used as subnetworks for MIDI synthesis",
						    &midi_keyboard_info);
  bse_categories_register_icon ("/Source/MIDI/Keyboard",
				midi_keyboard_type_id,
				&pixdata);
  return midi_keyboard_type_id;
}

static void
bse_midi_keyboard_class_init (BseMidiKeyboardClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_midi_keyboard_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_midi_keyboard_get_property;
  
  object_class->destroy = bse_midi_keyboard_do_destroy;

  item_class->set_parent = bse_midi_keyboard_set_parent;
  
  source_class->prepare = bse_midi_keyboard_prepare;
  source_class->context_create = bse_midi_keyboard_context_create;
  source_class->reset = bse_midi_keyboard_reset;

  ochannel_id = bse_source_class_add_ochannel (source_class, "Frequency", "MIDI Note Frequency");
  g_assert (ochannel_id == BSE_MIDI_KEYBOARD_OCHANNEL_FREQUENCY);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Gate", "High if the note is currently being pressed");
  g_assert (ochannel_id == BSE_MIDI_KEYBOARD_OCHANNEL_GATE);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Velocity", "Velocity of the note press");
  g_assert (ochannel_id == BSE_MIDI_KEYBOARD_OCHANNEL_VELOCITY);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Aftertouch", NULL);
  g_assert (ochannel_id == BSE_MIDI_KEYBOARD_OCHANNEL_AFTERTOUCH);
}

static void
bse_midi_keyboard_class_finalize (BseMidiKeyboardClass *class)
{
}

static void
bse_midi_keyboard_init (BseMidiKeyboard *keyb)
{
  keyb->cname1 = BSE_SOURCE_OCHANNEL_CNAME (keyb, BSE_MIDI_KEYBOARD_OCHANNEL_FREQUENCY);
  keyb->cname2 = BSE_SOURCE_OCHANNEL_CNAME (keyb, BSE_MIDI_KEYBOARD_OCHANNEL_GATE);
  keyb->cname3 = BSE_SOURCE_OCHANNEL_CNAME (keyb, BSE_MIDI_KEYBOARD_OCHANNEL_VELOCITY);
  keyb->cname4 = BSE_SOURCE_OCHANNEL_CNAME (keyb, BSE_MIDI_KEYBOARD_OCHANNEL_AFTERTOUCH);
}

static void
bse_midi_keyboard_do_destroy (BseObject *object)
{
  BseMidiKeyboard *keyb;
  
  keyb = BSE_MIDI_KEYBOARD (object);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_midi_keyboard_set_property (BseMidiKeyboard *keyb,
				guint            param_id,
				GValue          *value,
				GParamSpec      *pspec,
				const gchar     *trailer)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (keyb, param_id, pspec);
      break;
    }
}

static void
bse_midi_keyboard_get_property (BseMidiKeyboard *keyb,
				guint            param_id,
				GValue          *value,
				GParamSpec      *pspec,
				const gchar     *trailer)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (keyb, param_id, pspec);
      break;
    }
}

static void
bse_midi_keyboard_set_parent (BseItem *item,
			      BseItem *parent)
{
  BseMidiKeyboard *keyb = BSE_MIDI_KEYBOARD (item);

  /* remove from old parent */
  if (item->parent)
    {
      bse_snet_remove_in_port (BSE_SNET (item->parent), keyb->cname1);
      bse_snet_remove_in_port (BSE_SNET (item->parent), keyb->cname2);
      bse_snet_remove_in_port (BSE_SNET (item->parent), keyb->cname3);
      bse_snet_remove_in_port (BSE_SNET (item->parent), keyb->cname4);
      keyb->cname1 = BSE_SOURCE_OCHANNEL_CNAME (item, BSE_MIDI_KEYBOARD_OCHANNEL_FREQUENCY);
      keyb->cname2 = BSE_SOURCE_OCHANNEL_CNAME (item, BSE_MIDI_KEYBOARD_OCHANNEL_GATE);
      keyb->cname3 = BSE_SOURCE_OCHANNEL_CNAME (item, BSE_MIDI_KEYBOARD_OCHANNEL_VELOCITY);
      keyb->cname4 = BSE_SOURCE_OCHANNEL_CNAME (item, BSE_MIDI_KEYBOARD_OCHANNEL_AFTERTOUCH);
    }

  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);

  /* add to new parent */
  if (item->parent)
    {
      keyb->cname1 = bse_snet_add_in_port (BSE_SNET (item->parent), keyb->cname1,
					   BSE_SOURCE (item), BSE_MIDI_KEYBOARD_OCHANNEL_FREQUENCY, 0);
      keyb->cname2 = bse_snet_add_in_port (BSE_SNET (item->parent), keyb->cname2,
					   BSE_SOURCE (item), BSE_MIDI_KEYBOARD_OCHANNEL_GATE, 1);
      keyb->cname3 = bse_snet_add_in_port (BSE_SNET (item->parent), keyb->cname3,
					   BSE_SOURCE (item), BSE_MIDI_KEYBOARD_OCHANNEL_VELOCITY, 2);
      keyb->cname4 = bse_snet_add_in_port (BSE_SNET (item->parent), keyb->cname4,
					   BSE_SOURCE (item), BSE_MIDI_KEYBOARD_OCHANNEL_AFTERTOUCH, 3);
    }
}

static void
bse_midi_keyboard_prepare (BseSource *source)
{
  // BseMidiKeyboard *keyb = BSE_MIDI_KEYBOARD (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
midi_keyboard_process (GslModule *module,
		       guint      n_values)
{
  const BseSampleValue *freq_in = GSL_MODULE_IBUFFER (module, 0);
  const BseSampleValue *gate_in = GSL_MODULE_IBUFFER (module, 1);
  const BseSampleValue *vel_in = GSL_MODULE_IBUFFER (module, 2);
  const BseSampleValue *aftertouch_in = GSL_MODULE_IBUFFER (module, 3);

  GSL_MODULE_OBUFFER (module, 0) = (gfloat*) freq_in;
  GSL_MODULE_OBUFFER (module, 1) = (gfloat*) gate_in;
  GSL_MODULE_OBUFFER (module, 2) = (gfloat*) vel_in;
  GSL_MODULE_OBUFFER (module, 3) = (gfloat*) aftertouch_in;
}

static void
bse_midi_keyboard_context_create (BseSource *source,
				  guint      context_handle,
				  GslTrans  *trans)
{
  static const GslClass midi_keyboard_mclass = {
    4,				/* n_istreams */
    0,				/* n_jstreams */
    4,				/* n_ostreams */
    midi_keyboard_process,	/* process */
    NULL,			/* free */
    GSL_COST_CHEAP,		/* cost */
  };
  // BseMidiKeyboard *keyb = BSE_MIDI_KEYBOARD (source);
  GslModule *module = gsl_module_new (&midi_keyboard_mclass, NULL);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_midi_keyboard_reset (BseSource *source)
{
  // BseMidiKeyboard *keyb = BSE_MIDI_KEYBOARD (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
