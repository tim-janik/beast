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
#include "bseinport.h"

#include "bsecategories.h"
#include "bsesnet.h"
#include "./icons/inport.c"
#include "gslengine.h"



/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_PORT_NAME
};


/* --- prototypes --- */
static void	 bse_in_port_init		(BseInPort		*scard);
static void	 bse_in_port_class_init		(BseInPortClass		*class);
static void	 bse_in_port_class_finalize	(BseInPortClass		*class);
static void	 bse_in_port_set_property	(BseInPort		*scard,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	 bse_in_port_get_property	(BseInPort		*scard,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	 bse_in_port_do_destroy		(BseObject		*object);
static void	 bse_in_port_set_parent		(BseItem		*item,
						 BseItem		*parent);
static void	 bse_in_port_context_create	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseInPort)
{
  static const GTypeInfo in_port_info = {
    sizeof (BseInPortClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_in_port_class_init,
    (GClassFinalizeFunc) bse_in_port_class_finalize,
    NULL /* class_data */,
    
    sizeof (BseInPort),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_in_port_init,
  };
  static const BsePixdata pixdata = {
    INPORT_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    INPORT_IMAGE_WIDTH, INPORT_IMAGE_HEIGHT,
    INPORT_IMAGE_RLE_PIXEL_DATA,
  };
  guint in_port_type_id;
  
  in_port_type_id = bse_type_register_static (BSE_TYPE_SOURCE,
					      "BseInPort",
					      "Virtual input port connector, used to provide a synthesis network "
					      "with input signals from other synthesis networks",
					      &in_port_info);
  bse_categories_register_icon ("/Source/Plug/Virtual Input",
				in_port_type_id,
				&pixdata);
  return in_port_type_id;
}

static void
bse_in_port_class_init (BseInPortClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_in_port_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_in_port_get_property;
  
  object_class->destroy = bse_in_port_do_destroy;
  
  item_class->set_parent = bse_in_port_set_parent;
  
  source_class->context_create = bse_in_port_context_create;
  
  bse_object_class_add_param (object_class, "Assignments",
			      PARAM_PORT_NAME,
			      bse_param_spec_string ("port_name", "Input port",
						     "The port name is a unique name to establish input<->output "
						     "port relationships",
						     "synth_in",
						     BSE_PARAM_DEFAULT));
  
  ochannel_id = bse_source_class_add_ochannel (source_class, "Output", "Virtual synthesis network input connection");
  g_assert (ochannel_id == BSE_IN_PORT_OCHANNEL_VIN);
}

static void
bse_in_port_class_finalize (BseInPortClass *class)
{
}

static void
bse_in_port_init (BseInPort *oport)
{
  oport->port_name = g_strdup ("synth_in");
}

static void
bse_in_port_do_destroy (BseObject *object)
{
  BseInPort *oport = BSE_IN_PORT (object);
  
  g_free (oport->port_name);
  oport->port_name = NULL;
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_in_port_set_property (BseInPort  *oport,
			  guint        param_id,
			  GValue      *value,
			  GParamSpec  *pspec)
{
  BseItem *item = BSE_ITEM (oport);
  
  switch (param_id)
    {
      const gchar *name;
    case PARAM_PORT_NAME:
      name = g_value_get_string (value);
      if (item->parent)
	{
	  bse_snet_remove_in_port (BSE_SNET (item->parent), oport->port_name);
	  name = bse_snet_add_in_port (BSE_SNET (item->parent), name,
				       BSE_SOURCE (item), BSE_IN_PORT_OCHANNEL_VIN, 0);
	}
      g_free (oport->port_name);
      oport->port_name = g_strdup (name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (oport, param_id, pspec);
      break;
    }
}

static void
bse_in_port_get_property (BseInPort  *oport,
			  guint        param_id,
			  GValue      *value,
			  GParamSpec  *pspec)
{
  switch (param_id)
    {
    case PARAM_PORT_NAME:
      g_value_set_string (value, oport->port_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (oport, param_id, pspec);
      break;
    }
}

static void
bse_in_port_set_parent (BseItem *item,
			BseItem *parent)
{
  BseInPort *oport = BSE_IN_PORT (item);
  
  /* remove from old parent */
  if (item->parent)
    bse_snet_remove_in_port (BSE_SNET (item->parent), oport->port_name);
  
  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);
  
  /* add to new parent */
  if (item->parent)
    {
      const gchar *name;
      
      name = bse_snet_add_in_port (BSE_SNET (item->parent), oport->port_name,
				   BSE_SOURCE (item), BSE_IN_PORT_OCHANNEL_VIN, 0);
      if (strcmp (name, oport->port_name) != 0)
	{
	  g_free (oport->port_name);
	  oport->port_name = g_strdup (name);
	  g_object_notify (G_OBJECT (item), "port_name");
	}
    }
}

static void
in_port_process (GslModule *module,
		 guint      n_values)
{
  const BseSampleValue *src = GSL_MODULE_IBUFFER (module, 0);
  
  GSL_MODULE_OBUFFER (module, 0) = (gfloat*) src;
}

static void
bse_in_port_context_create (BseSource *source,
			    guint      context_handle,
			    GslTrans  *trans)
{
  static const GslClass in_port_mclass = {
    1,			/* n_istreams */
    0,			/* n_jstreams */
    1,			/* n_ostreams */
    in_port_process,	/* process */
    NULL,		/* free */
    GSL_COST_CHEAP,	/* cost */
  };
  GslModule *module = gsl_module_new (&in_port_mclass, NULL);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}
