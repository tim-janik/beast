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
#include "bseoutport.h"

#include "bsecategories.h"
#include "bsesnet.h"
#include "./icons/outport.c"
#include "gslengine.h"


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_PORT_NAME
};


/* --- prototypes --- */
static void	 bse_out_port_init		(BseOutPort		*scard);
static void	 bse_out_port_class_init	(BseOutPortClass	*class);
static void	 bse_out_port_class_finalize	(BseOutPortClass	*class);
static void	 bse_out_port_set_property	(GObject                *object,
						 guint			 param_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	 bse_out_port_get_property	(GObject                *object,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	 bse_out_port_do_destroy	(BseObject		*object);
static void	 bse_out_port_set_parent	(BseItem		*item,
						 BseItem		*parent);
static void	 bse_out_port_context_create	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);
static void      bse_out_port_context_connect   (BseSource              *source,
						 guint                   context_handle,
						 GslTrans               *trans);
static void      bse_out_port_context_dismiss   (BseSource              *source,
						 guint                   context_handle,
						 GslTrans               *trans);
static void   bse_out_port_update_port_contexts (BseOutPort             *self,
						 const gchar            *old_name,
						 const gchar            *new_name,
						 guint                   port);


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseOutPort)
{
  static const GTypeInfo out_port_info = {
    sizeof (BseOutPortClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_out_port_class_init,
    (GClassFinalizeFunc) bse_out_port_class_finalize,
    NULL /* class_data */,
    
    sizeof (BseOutPort),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_out_port_init,
  };
  static const BsePixdata pixdata = {
    OUTPORT_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
    OUTPORT_IMAGE_WIDTH, OUTPORT_IMAGE_HEIGHT,
    OUTPORT_IMAGE_RLE_PIXEL_DATA,
  };
  guint out_port_type_id;
  
  out_port_type_id = bse_type_register_static (BSE_TYPE_SOURCE,
					       "BseOutPort",
					       "Virtual output port connector, used to interface synthesis network "
					       "output to other synthesis networks (as inputs)",
					       &out_port_info);
  bse_categories_register_icon ("/Modules/Plug/Virtual Output",
				out_port_type_id,
				&pixdata);
  return out_port_type_id;
}

static void
bse_out_port_class_init (BseOutPortClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ichannel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_out_port_set_property;
  gobject_class->get_property = bse_out_port_get_property;
  
  object_class->destroy = bse_out_port_do_destroy;
  
  item_class->set_parent = bse_out_port_set_parent;
  
  source_class->context_create = bse_out_port_context_create;
  source_class->context_connect = bse_out_port_context_connect;
  source_class->context_dismiss = bse_out_port_context_dismiss;
  
  bse_object_class_add_param (object_class, "Assignments",
			      PARAM_PORT_NAME,
			      bse_param_spec_string ("port_name", "Output port",
						     "The port name is a unique name to establish output<->input "
						     "port relationships",
						     "synth_out",
						     BSE_PARAM_DEFAULT));
  
  ichannel_id = bse_source_class_add_ichannel (source_class, "Input", "Virtual synthesis network output connection");
  g_assert (ichannel_id == BSE_OUT_PORT_ICHANNEL_VOUT);
}

static void
bse_out_port_class_finalize (BseOutPortClass *class)
{
}

static void
bse_out_port_init (BseOutPort *self)
{
  self->port_name = g_strdup ("synth_out");
}

static void
bse_out_port_do_destroy (BseObject *object)
{
  BseOutPort *self = BSE_OUT_PORT (object);
  
  g_free (self->port_name);
  self->port_name = NULL;
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_out_port_set_property (GObject      *object,
			   guint         param_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  BseOutPort *self = BSE_OUT_PORT (object);
  BseItem *item = BSE_ITEM (self);
  
  switch (param_id)
    {
      const gchar *name;
    case PARAM_PORT_NAME:
      name = g_value_get_string (value);
      if (item->parent)
	{
	  bse_snet_oport_name_unregister (BSE_SNET (item->parent), self->port_name);
	  name = bse_snet_oport_name_register (BSE_SNET (item->parent), name);
	}
      if (BSE_SOURCE_PREPARED (self))
	bse_out_port_update_port_contexts (self, self->port_name, name, 0);
      g_free (self->port_name);
      self->port_name = g_strdup (name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_out_port_get_property (GObject    *object,
			   guint       param_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  BseOutPort *self = BSE_OUT_PORT (object);

  switch (param_id)
    {
    case PARAM_PORT_NAME:
      g_value_set_string (value, self->port_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_out_port_set_parent (BseItem *item,
			 BseItem *parent)
{
  BseOutPort *self = BSE_OUT_PORT (item);
  
  /* remove port name from old parent */
  if (item->parent)
    bse_snet_oport_name_unregister (BSE_SNET (item->parent), self->port_name);
  
  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);
  
  /* add port name to new parent */
  if (item->parent)
    {
      const gchar *name;
      
      name = bse_snet_oport_name_register (BSE_SNET (item->parent), self->port_name);
      if (strcmp (name, self->port_name) != 0)
	{
	  g_free (self->port_name);
	  self->port_name = g_strdup (name);
	  g_object_notify (G_OBJECT (item), "port_name");
	}
    }
}

static void
out_port_process (GslModule *module,
		  guint      n_values)
{
  const BseSampleValue *src = GSL_MODULE_IBUFFER (module, 0);
  
  GSL_MODULE_OBUFFER (module, 0) = (gfloat*) src;
}

static void
bse_out_port_context_create (BseSource *source,
			     guint      context_handle,
			     GslTrans  *trans)
{
  static const GslClass out_port_mclass = {
    1,			/* n_istreams */
    0,			/* n_jstreams */
    1,			/* n_ostreams */
    out_port_process,	/* process */
    NULL,		/* free */
    GSL_COST_CHEAP,	/* cost */
  };
  GslModule *module = gsl_module_new (&out_port_mclass, NULL);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_imodule (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_out_port_context_connect (BseSource *source,
			      guint      context_handle,
			      GslTrans  *trans)
{
  BseOutPort *self = BSE_OUT_PORT (source);
  BseItem *item = BSE_ITEM (self);
  GslModule *module = bse_source_get_context_imodule (source, context_handle);
  
  bse_snet_set_oport_src (BSE_SNET (item->parent), self->port_name, context_handle,
			  module, 0, trans);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_out_port_context_dismiss (BseSource *source,
			      guint      context_handle,
			      GslTrans  *trans)
{
  BseOutPort *self = BSE_OUT_PORT (source);
  BseItem *item = BSE_ITEM (self);
  
  bse_snet_set_oport_src (BSE_SNET (item->parent), self->port_name, context_handle,
			  NULL, 0, trans);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}

static void
bse_out_port_update_port_contexts (BseOutPort   *self,
				   const gchar *old_name,
				   const gchar *new_name,
				   guint        port)
{
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = BSE_SNET (item->parent);
  BseSource *source = BSE_SOURCE (self);
  GslTrans *trans = gsl_trans_open ();
  guint *cids, n, i;
  
  g_return_if_fail (BSE_SOURCE_PREPARED (self));
  
  cids = bse_source_context_ids (source, &n);
  for (i = 0; i < n; i++)
    {
      GslModule *imodule = bse_source_get_context_imodule (source, cids[i]);
      
      bse_snet_set_oport_src (snet, old_name, cids[i], NULL, port, trans);
      bse_snet_set_oport_src (snet, new_name, cids[i], imodule, port, trans);
    }
  g_free (cids);
  gsl_trans_commit (trans);
}
