/* BSE - Bedevilled Sound Engine
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
#include "bsesubsynth.h"

#include "bsecategories.h"
#include "bsesnet.h"
#include "./icons/inoutport.c"
#include "gslengine.h"



/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_SNET,
  PARAM_IPORT_NAME1,
  PARAM_IPORT_NAME2,
  PARAM_IPORT_NAME3,
  PARAM_IPORT_NAME4,
  PARAM_OPORT_NAME1,
  PARAM_OPORT_NAME2,
  PARAM_OPORT_NAME3,
  PARAM_OPORT_NAME4
};


/* --- prototypes --- */
static void	 bse_sub_synth_init		(BseSubSynth		*scard);
static void	 bse_sub_synth_class_init	(BseSubSynthClass	*class);
static void	 bse_sub_synth_class_finalize	(BseSubSynthClass	*class);
static void	 bse_sub_synth_set_property	(BseSubSynth		*scard,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec,
						 const gchar		*trailer);
static void	 bse_sub_synth_get_property	(BseSubSynth		*scard,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec,
						 const gchar		*trailer);
static void	 bse_sub_synth_do_destroy	(BseObject		*object);
static void	 bse_sub_synth_prepare		(BseSource		*source);
static void	 bse_sub_synth_context_create	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);
static void	 bse_sub_synth_context_connect	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);
static void	 bse_sub_synth_context_dismiss	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);
static void	 bse_sub_synth_reset		(BseSource		*source);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSubSynth)
{
  static const GTypeInfo sub_synth_info = {
    sizeof (BseSubSynthClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_sub_synth_class_init,
    (GClassFinalizeFunc) bse_sub_synth_class_finalize,
    NULL /* class_data */,
    
    sizeof (BseSubSynth),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_sub_synth_init,
  };
  static const BsePixdata pixdata = {
    INOUTPORT_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    INOUTPORT_IMAGE_WIDTH, INOUTPORT_IMAGE_HEIGHT,
    INOUTPORT_IMAGE_RLE_PIXEL_DATA,
  };
  guint sub_synth_type_id;
  
  sub_synth_type_id = bse_type_register_static (BSE_TYPE_SOURCE,
						"BseSubSynth",
						"This module encapsulates whole synthesizer networks, by "
						"interfacing to/from their virtual input and output ports",
						&sub_synth_info);
  bse_categories_register_icon ("/Source/Plug/Virtual Sub Synth",
				sub_synth_type_id,
				&pixdata);
  return sub_synth_type_id;
}

static void
bse_sub_synth_class_init (BseSubSynthClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ichannel_id, ochannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_sub_synth_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_sub_synth_get_property;
  
  object_class->destroy = bse_sub_synth_do_destroy;
  
  source_class->prepare = bse_sub_synth_prepare;
  source_class->context_create = bse_sub_synth_context_create;
  source_class->context_connect = bse_sub_synth_context_connect;
  source_class->context_dismiss = bse_sub_synth_context_dismiss;
  source_class->reset = bse_sub_synth_reset;
  
  bse_object_class_add_param (object_class, "Assignments",
			      PARAM_SNET,
			      g_param_spec_object ("snet", "Synthesis Network", "The synthesis network to interface to",
						   BSE_TYPE_SNET, BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Input Assignments",
			      PARAM_IPORT_NAME1,
			      bse_param_spec_string ("in_port_1", "Input Port 1", "Output port name to interface from",
						     "synth_in_1",
						     BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Input Assignments",
			      PARAM_IPORT_NAME2,
			      bse_param_spec_string ("in_port_2", "Input Port 2", "Output port name to interface from",
						     "synth_in_2",
						     BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Input Assignments",
			      PARAM_IPORT_NAME3,
			      bse_param_spec_string ("in_port_3", "Input Port 3", "Output port name to interface from",
						     "synth_in_3",
						     BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Input Assignments",
			      PARAM_IPORT_NAME4,
			      bse_param_spec_string ("in_port_4", "Input Port 4", "Output port name to interface from",
						     "synth_in_4",
						     BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Output Assignments",
			      PARAM_OPORT_NAME1,
			      bse_param_spec_string ("out_port_1", "Output Port 1", "Input port name to interface to",
						     "synth_out_1",
						     BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Output Assignments",
			      PARAM_OPORT_NAME2,
			      bse_param_spec_string ("out_port_2", "Output Port 2", "Input port name to interface to",
						     "synth_out_2",
						     BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Output Assignments",
			      PARAM_OPORT_NAME3,
			      bse_param_spec_string ("out_port_3", "Output Port 3", "Input port name to interface to",
						     "synth_out_3",
						     BSE_PARAM_DEFAULT));
  bse_object_class_add_param (object_class, "Output Assignments",
			      PARAM_OPORT_NAME4,
			      bse_param_spec_string ("out_port_4", "Output Port 4", "Input port name to interface to",
						     "synth_out_4",
						     BSE_PARAM_DEFAULT));
  
  ichannel_id = bse_source_class_add_ichannel (source_class, "Input 1", "Virtual input 1");
  g_assert (ichannel_id == BSE_SUB_SYNTH_ICHANNEL_VIN1);
  ichannel_id = bse_source_class_add_ichannel (source_class, "Input 2", "Virtual input 2");
  g_assert (ichannel_id == BSE_SUB_SYNTH_ICHANNEL_VIN2);
  ichannel_id = bse_source_class_add_ichannel (source_class, "Input 3", "Virtual input 3");
  g_assert (ichannel_id == BSE_SUB_SYNTH_ICHANNEL_VIN3);
  ichannel_id = bse_source_class_add_ichannel (source_class, "Input 4", "Virtual input 4");
  g_assert (ichannel_id == BSE_SUB_SYNTH_ICHANNEL_VIN4);

  ochannel_id = bse_source_class_add_ochannel (source_class, "Output 1", "Virtual output 1");
  g_assert (ochannel_id == BSE_SUB_SYNTH_OCHANNEL_VOUT1);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Output 2", "Virtual output 2");
  g_assert (ochannel_id == BSE_SUB_SYNTH_OCHANNEL_VOUT2);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Output 3", "Virtual output 3");
  g_assert (ochannel_id == BSE_SUB_SYNTH_OCHANNEL_VOUT3);
  ochannel_id = bse_source_class_add_ochannel (source_class, "Output 4", "Virtual output 4");
  g_assert (ochannel_id == BSE_SUB_SYNTH_OCHANNEL_VOUT4);
}

static void
bse_sub_synth_class_finalize (BseSubSynthClass *class)
{
}

static void
bse_sub_synth_init (BseSubSynth *synth)
{
  synth->snet = NULL;
  synth->input_ports[0] = g_strdup ("synth_in_1");
  synth->input_ports[1] = g_strdup ("synth_in_2");
  synth->input_ports[2] = g_strdup ("synth_in_3");
  synth->input_ports[3] = g_strdup ("synth_in_4");
  synth->output_ports[0] = g_strdup ("synth_out_1");
  synth->output_ports[1] = g_strdup ("synth_out_2");
  synth->output_ports[2] = g_strdup ("synth_out_3");
  synth->output_ports[3] = g_strdup ("synth_out_4");
}

static void
bse_sub_synth_do_destroy (BseObject *object)
{
  BseSubSynth *synth = BSE_SUB_SYNTH (object);
  guint i;
  
  if (synth->snet)
    {
      g_object_unref (synth->snet);
      synth->snet = NULL;
    }
  for (i = 0; i < 4; i++)
    {
      g_free (synth->input_ports[i]);
      synth->input_ports[i] = NULL;
      g_free (synth->output_ports[i]);
      synth->output_ports[i] = NULL;
    }
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_sub_synth_set_property (BseSubSynth *synth,
			    guint        param_id,
			    GValue      *value,
			    GParamSpec  *pspec,
			    const gchar *trailer)
{
  switch (param_id)
    {
      guint i;
    case PARAM_SNET:
      if (synth->snet)
	g_object_unref (synth->snet);
      synth->snet = g_value_get_object (value);
      if (synth->snet)
	g_object_ref (synth->snet);
      break;
    case PARAM_IPORT_NAME1:
    case PARAM_IPORT_NAME2:
    case PARAM_IPORT_NAME3:
    case PARAM_IPORT_NAME4:
      i = param_id - PARAM_IPORT_NAME1;
      g_free (synth->input_ports[i]);
      synth->input_ports[i] = g_strdup (g_value_get_string (value));
      break;
    case PARAM_OPORT_NAME1:
    case PARAM_OPORT_NAME2:
    case PARAM_OPORT_NAME3:
    case PARAM_OPORT_NAME4:
      i = param_id - PARAM_OPORT_NAME1;
      g_free (synth->output_ports[i]);
      synth->output_ports[i] = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (synth, param_id, pspec);
      break;
    }
}

static void
bse_sub_synth_get_property (BseSubSynth *synth,
			    guint        param_id,
			    GValue      *value,
			    GParamSpec  *pspec,
			    const gchar *trailer)
{
  switch (param_id)
    {
      guint i;
    case PARAM_SNET:
      g_value_set_object (value, synth->snet);
      break;
    case PARAM_IPORT_NAME1:
    case PARAM_IPORT_NAME2:
    case PARAM_IPORT_NAME3:
    case PARAM_IPORT_NAME4:
      i = param_id - PARAM_IPORT_NAME1;
      g_value_set_string (value, synth->input_ports[i]);
      break;
    case PARAM_OPORT_NAME1:
    case PARAM_OPORT_NAME2:
    case PARAM_OPORT_NAME3:
    case PARAM_OPORT_NAME4:
      i = param_id - PARAM_OPORT_NAME1;
      g_value_set_string (value, synth->output_ports[i]);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (synth, param_id, pspec);
      break;
    }
}

static void
bse_sub_synth_prepare (BseSource *source)
{
  // BseSubSynth *synth = BSE_SUB_SYNTH (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

typedef struct {
  guint  synth_context_handle;
} ModData;

static void
sub_synths_process (GslModule *module,
		    guint      n_values)
{
  GSL_MODULE_OBUFFER (module, 0) = (gfloat*) GSL_MODULE_IBUFFER (module, 0);
  GSL_MODULE_OBUFFER (module, 1) = (gfloat*) GSL_MODULE_IBUFFER (module, 1);
  GSL_MODULE_OBUFFER (module, 2) = (gfloat*) GSL_MODULE_IBUFFER (module, 2);
  GSL_MODULE_OBUFFER (module, 3) = (gfloat*) GSL_MODULE_IBUFFER (module, 3);
}

static void
bse_sub_synth_context_create (BseSource *source,
			      guint      context_handle,
			      GslTrans  *trans)
{
  static const GslClass sub_synth_mclass = {
    4,				/* n_istreams */
    0,				/* n_jstreams */
    4,				/* n_ostreams */
    sub_synths_process,		/* process */
    (GslModuleFreeFunc) g_free,	/* free */
    GSL_COST_CHEAP,		/* cost */
  };
  BseSubSynth *synth = BSE_SUB_SYNTH (source);
  BseSNet *snet = synth->snet;
  ModData *mdata_in = g_new0 (ModData, 1);
  ModData *mdata_out = g_new0 (ModData, 1);
  GslModule *imodule = gsl_module_new (&sub_synth_mclass, mdata_in);
  GslModule *omodule = gsl_module_new (&sub_synth_mclass, mdata_out);
  guint foreign_context_handle = ~0;

  /* create new context for foreign synth */
  if (snet)
    foreign_context_handle = bse_source_create_context (BSE_SOURCE (snet), trans);

  mdata_in->synth_context_handle = foreign_context_handle;
  mdata_out->synth_context_handle = foreign_context_handle;
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_imodule (source, BSE_SUB_SYNTH_ICHANNEL_VIN1, context_handle, imodule, 0);
  bse_source_set_context_imodule (source, BSE_SUB_SYNTH_ICHANNEL_VIN2, context_handle, imodule, 1);
  bse_source_set_context_imodule (source, BSE_SUB_SYNTH_ICHANNEL_VIN3, context_handle, imodule, 2);
  bse_source_set_context_imodule (source, BSE_SUB_SYNTH_ICHANNEL_VIN4, context_handle, imodule, 3);
  bse_source_set_context_omodule (source, BSE_SUB_SYNTH_OCHANNEL_VOUT1, context_handle, omodule, 0);
  bse_source_set_context_omodule (source, BSE_SUB_SYNTH_OCHANNEL_VOUT2, context_handle, omodule, 1);
  bse_source_set_context_omodule (source, BSE_SUB_SYNTH_OCHANNEL_VOUT3, context_handle, omodule, 2);
  bse_source_set_context_omodule (source, BSE_SUB_SYNTH_OCHANNEL_VOUT4, context_handle, omodule, 3);
  
  /* commit modules to engine */
  gsl_trans_add (trans, gsl_job_integrate (imodule));
  gsl_trans_add (trans, gsl_job_integrate (omodule));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_sub_synth_context_connect (BseSource *source,
			       guint      context_handle,
			       GslTrans  *trans)
{
  BseSubSynth *synth = BSE_SUB_SYNTH (source);
  BseSNet *snet = synth->snet;
  GslModule *imodule, *omodule, *fmodule;
  ModData *mdata_in, *mdata_out;
  guint i, stream;
  
  /* get context specific modules */
  imodule = bse_source_get_ichannel_module (source, BSE_SUB_SYNTH_ICHANNEL_VIN1, context_handle, NULL);
  omodule = bse_source_get_ochannel_module (source, BSE_SUB_SYNTH_OCHANNEL_VOUT1, context_handle, NULL);
  mdata_in = imodule->user_data;
  mdata_out = omodule->user_data;
  
  /* connect module to sub synthesizer */
  if (snet)
    {
      guint foreign_context_handle = mdata_in->synth_context_handle;

      bse_source_connect_context (BSE_SOURCE (snet), foreign_context_handle, trans);
      for (i = 0; i < 4; i++)
	{
	  fmodule = bse_snet_get_out_port_module (snet, synth->output_ports[i], foreign_context_handle, &stream);
	  if (fmodule)
	    gsl_trans_add (trans, gsl_job_connect (fmodule, stream, omodule, i));
	  fmodule = bse_snet_get_in_port_module (snet, synth->input_ports[i], foreign_context_handle, &stream);
	  if (fmodule)
	    gsl_trans_add (trans, gsl_job_connect (imodule, i, fmodule, stream));
	}
    }
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_sub_synth_context_dismiss (BseSource *source,
			       guint      context_handle,
			       GslTrans  *trans)
{
  BseSubSynth *synth = BSE_SUB_SYNTH (source);
  BseSNet *snet = synth->snet;
  GslModule *imodule;
  ModData *mdata_in;

  /* get context specific modules */
  imodule = bse_source_get_ichannel_module (source, BSE_SUB_SYNTH_ICHANNEL_VIN1, context_handle, NULL);
  mdata_in = imodule->user_data;

  if (snet)
    bse_source_dismiss_context (BSE_SOURCE (snet), mdata_in->synth_context_handle, trans);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}

static void
bse_sub_synth_reset (BseSource *source)
{
  // BseSubSynth *synth = BSE_SUB_SYNTH (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
}
