/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2002 Tim Janik
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
#include "bsemidiicontroller.h"

#include "bsecategories.h"
#include "bseserver.h"
#include "./icons/midiictrl.c"
#include "gslengine.h"



/* --- properties --- */
enum
{
  PROP_0,
  PROP_MIDI_CHANNEL,
  PROP_CONTROL_1,
  PROP_CONTROL_2,
  PROP_CONTROL_3,
  PROP_CONTROL_4
};


/* --- prototypes --- */
static void	 bse_midi_icontroller_init		(BseMidiIController	 *self);
static void	 bse_midi_icontroller_class_init	(BseMidiIControllerClass *class);
static void	 bse_midi_icontroller_set_property	(GObject		 *object,
							 guint			  param_id,
							 const GValue		 *value,
							 GParamSpec		 *pspec);
static void	 bse_midi_icontroller_get_property	(GObject		 *object,
							 guint			  param_id,
							 GValue			 *value,
							 GParamSpec		 *pspec);
static void      bse_midi_icontroller_prepare	        (BseSource		 *source);
static void	 bse_midi_icontroller_context_create	(BseSource		 *source,
							 guint			  instance_id,
							 GslTrans		 *trans);
static void	 bse_midi_icontroller_context_connect	(BseSource		 *source,
							 guint			  instance_id,
							 GslTrans		 *trans);
static void      bse_midi_icontroller_reset	        (BseSource		 *source);
static void	 bse_midi_icontroller_update_modules	(BseMidiIController	 *self);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseMidiIController)
{
  static const GTypeInfo midi_icontroller_info = {
    sizeof (BseMidiIControllerClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_midi_icontroller_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseMidiIController),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_midi_icontroller_init,
  };
  static const BsePixdata pixdata = {
    MIDIICTRL_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    MIDIICTRL_IMAGE_WIDTH, MIDIICTRL_IMAGE_HEIGHT,
    MIDIICTRL_IMAGE_RLE_PIXEL_DATA,
  };
  guint midi_icontroller_type_id;
  
  midi_icontroller_type_id = bse_type_register_static (BSE_TYPE_SOURCE,
						       "BseMidiIController",
						       "MIDI controller input module. With this module, "
						       "MIDI control signals can be used in synthesis networks.",
						       &midi_icontroller_info);
  bse_categories_register_icon ("/Modules/MIDI/Control Input",
				midi_icontroller_type_id,
				&pixdata);
  return midi_icontroller_type_id;
}

static void
bse_midi_icontroller_class_init (BseMidiIControllerClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_midi_icontroller_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_midi_icontroller_get_property;
  
  source_class->prepare = bse_midi_icontroller_prepare;
  source_class->context_create = bse_midi_icontroller_context_create;
  source_class->context_connect = bse_midi_icontroller_context_connect;
  source_class->reset = bse_midi_icontroller_reset;

  bse_object_class_add_param (object_class, "MIDI Controls",
			      PROP_MIDI_CHANNEL,
			      bse_param_spec_uint ("midi_channel", "MIDI Channel", NULL,
						   1, BSE_MIDI_MAX_CHANNELS,
						   1, 1,
						   BSE_PARAM_GUI | BSE_PARAM_STORAGE | BSE_PARAM_HINT_SCALE));
  bse_object_class_add_param (object_class, "MIDI Controls",
			      PROP_CONTROL_1,
			      bse_param_spec_enum ("control_1", "Control1", NULL,
						   BSE_TYPE_MIDI_CONTROL_TYPE,
						   BSE_MIDI_CONTROL_PITCH_BEND,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "MIDI Controls",
			      PROP_CONTROL_2,
			      bse_param_spec_enum ("control_2", "Control2", NULL,
						   BSE_TYPE_MIDI_CONTROL_TYPE,
						   BSE_MIDI_CONTROL_1,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "MIDI Controls",
			      PROP_CONTROL_3,
			      bse_param_spec_enum ("control_3", "Control3", NULL,
						   BSE_TYPE_MIDI_CONTROL_TYPE,
						   BSE_MIDI_CONTROL_7,
						   BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "MIDI Controls",
			      PROP_CONTROL_4,
			      bse_param_spec_enum ("control_4", "Control4", NULL,
						   BSE_TYPE_MIDI_CONTROL_TYPE,
						   BSE_MIDI_CONTROL_PRESSURE,
						   BSE_PARAM_DEFAULT));

  ochannel_id = bse_source_class_add_ochannel (source_class, "ctrl_1", "MIDI Control 1");
  g_assert (ochannel_id == BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL1);
  ochannel_id = bse_source_class_add_ochannel (source_class, "ctrl_2", "MIDI Control 2");
  g_assert (ochannel_id == BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL2);
  ochannel_id = bse_source_class_add_ochannel (source_class, "ctrl_3", "MIDI Control 3");
  g_assert (ochannel_id == BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL3);
  ochannel_id = bse_source_class_add_ochannel (source_class, "ctrl_4", "MIDI Control 4");
  g_assert (ochannel_id == BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL4);
}

static void
bse_midi_icontroller_init (BseMidiIController *self)
{
  self->midi_channel_id = 1;
  self->controls[0] = BSE_MIDI_CONTROL_PITCH_BEND;
  self->controls[1] = BSE_MIDI_CONTROL_1;
  self->controls[2] = BSE_MIDI_CONTROL_7;
  self->controls[3] = BSE_MIDI_CONTROL_PRESSURE;
  self->midi_input_module = NULL;
}

static void
bse_midi_icontroller_set_property (GObject      *object,
				   guint         param_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
  BseMidiIController *self = BSE_MIDI_ICONTROLLER (object);
  
  switch (param_id)
    {
    case PROP_MIDI_CHANNEL:
      self->midi_channel_id = g_value_get_uint (value);
      bse_midi_icontroller_update_modules (self);
      break;
    case PROP_CONTROL_1:
      self->controls[0] = g_value_get_enum (value);
      bse_midi_icontroller_update_modules (self);
      break;
    case PROP_CONTROL_2:
      self->controls[1] = g_value_get_enum (value);
      bse_midi_icontroller_update_modules (self);
      break;
    case PROP_CONTROL_3:
      self->controls[2] = g_value_get_enum (value);
      bse_midi_icontroller_update_modules (self);
      break;
    case PROP_CONTROL_4:
      self->controls[3] = g_value_get_enum (value);
      bse_midi_icontroller_update_modules (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_midi_icontroller_get_property (GObject    *object,
				   guint       param_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
  BseMidiIController *self = BSE_MIDI_ICONTROLLER (object);
  
  switch (param_id)
    {
    case PROP_MIDI_CHANNEL:
      g_value_set_uint (value, self->midi_channel_id);
      break;
    case PROP_CONTROL_1:
      g_value_set_enum (value, self->controls[0]);
      break;
    case PROP_CONTROL_2:
      g_value_set_enum (value, self->controls[1]);
      break;
    case PROP_CONTROL_3:
      g_value_set_enum (value, self->controls[2]);
      break;
    case PROP_CONTROL_4:
      g_value_set_enum (value, self->controls[3]);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_midi_icontroller_prepare (BseSource *source)
{
  BseMidiIController *self = BSE_MIDI_ICONTROLLER (source);
  
  self->midi_input_module = bse_server_retrive_midi_input_module (bse_server_get (),
								  "MidiIController",
								  self->midi_channel_id,
								  0, /* nth_note */
								  self->controls);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
midi_icontroller_process (GslModule *module,
			  guint      n_values)
{
  const BseSampleValue *c1_in = GSL_MODULE_IBUFFER (module, BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL1);
  const BseSampleValue *c2_in = GSL_MODULE_IBUFFER (module, BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL2);
  const BseSampleValue *c3_in = GSL_MODULE_IBUFFER (module, BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL3);
  const BseSampleValue *c4_in = GSL_MODULE_IBUFFER (module, BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL4);
  
  GSL_MODULE_OBUFFER (module, BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL1) = (gfloat*) c1_in;
  GSL_MODULE_OBUFFER (module, BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL2) = (gfloat*) c2_in;
  GSL_MODULE_OBUFFER (module, BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL3) = (gfloat*) c3_in;
  GSL_MODULE_OBUFFER (module, BSE_MIDI_ICONTROLLER_OCHANNEL_CONTROL4) = (gfloat*) c4_in;
}

static void
bse_midi_icontroller_context_create (BseSource *source,
				     guint      context_handle,
				     GslTrans  *trans)
{
  static const GslClass midi_icontroller_mclass = {
    BSE_MIDI_ICONTROLLER_N_OCHANNELS, /* n_istreams */
    0,				     /* n_jstreams */
    BSE_MIDI_ICONTROLLER_N_OCHANNELS, /* n_ostreams */
    midi_icontroller_process,	     /* process */
    NULL,			     /* free */
    GSL_COST_CHEAP,		     /* cost */
  };
  GslModule *module = gsl_module_new (&midi_icontroller_mclass, NULL);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_omodule (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_midi_icontroller_context_connect (BseSource *source,
				      guint      context_handle,
				      GslTrans  *trans)
{
  BseMidiIController *self = BSE_MIDI_ICONTROLLER (source);
  GslModule *module = bse_source_get_context_omodule (source, context_handle);
  GslModule *midictrl = self->midi_input_module;

  /* connect module to midi control uplink */
  gsl_trans_add (trans, gsl_job_connect (midictrl, 0, module, 0));
  gsl_trans_add (trans, gsl_job_connect (midictrl, 1, module, 1));
  gsl_trans_add (trans, gsl_job_connect (midictrl, 2, module, 2));
  gsl_trans_add (trans, gsl_job_connect (midictrl, 3, module, 3));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_midi_icontroller_update_modules (BseMidiIController *self)
{
  if (BSE_SOURCE_PREPARED (self))
    {
      BseSource *source = BSE_SOURCE (self);
      GslModule *old_midictrl = self->midi_input_module;
      GslTrans *trans;
      guint *cids, n, i;

      /* retrive module for new setup */
      self->midi_input_module = bse_server_retrive_midi_input_module (bse_server_get (),
								      "MidiIController",
								      self->midi_channel_id,
								      0, /* nth_note */
								      self->controls);
      /* reconnect all contexts */
      trans = gsl_trans_open ();
      cids = bse_source_context_ids (source, &n);
      for (i = 0; i < n; i++)
	{
	  GslModule *module = bse_source_get_context_omodule (source, cids[i]);
	  GslModule *midictrl = self->midi_input_module;
	  
	  /* disconnect from old module */
	  gsl_trans_add (trans, gsl_job_disconnect (module, 0));
	  gsl_trans_add (trans, gsl_job_disconnect (module, 1));
	  gsl_trans_add (trans, gsl_job_disconnect (module, 2));
	  gsl_trans_add (trans, gsl_job_disconnect (module, 3));
	  /* connect to new module */
	  gsl_trans_add (trans, gsl_job_connect (midictrl, 0, module, 0));
	  gsl_trans_add (trans, gsl_job_connect (midictrl, 1, module, 1));
	  gsl_trans_add (trans, gsl_job_connect (midictrl, 2, module, 2));
	  gsl_trans_add (trans, gsl_job_connect (midictrl, 3, module, 3));
	}
      g_free (cids);

      /* make sure the changes have taken place already */
      gsl_trans_commit (trans);
      gsl_engine_wait_on_trans ();

      /* discard old module */
      bse_server_discard_midi_input_module (bse_server_get (), old_midictrl);
    }
}

static void
bse_midi_icontroller_reset (BseSource *source)
{
  BseMidiIController *self = BSE_MIDI_ICONTROLLER (source);
  
  bse_server_discard_midi_input_module (bse_server_get (), self->midi_input_module);
  self->midi_input_module = NULL;
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
