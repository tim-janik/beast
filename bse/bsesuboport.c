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
#include "bsesuboport.h"

#include "bsecategories.h"
#include "bsesnet.h"

#include <string.h>

/* --- parameters --- */
enum
{
  PROP_0,
  /* don't add properties after here */
  PROP_OPORT_NAME
};


/* --- prototypes --- */
static void      bse_sub_oport_init             (BseSubOPort            *self);
static void      bse_sub_oport_class_init       (BseSubOPortClass       *class);
static void      bse_sub_oport_set_property     (GObject                *object,
                                                 guint                   param_id,
                                                 const GValue           *value,
                                                 GParamSpec             *pspec);
static void      bse_sub_oport_get_property     (GObject                *object,
                                                 guint                   param_id,
                                                 GValue                 *value,
                                                 GParamSpec             *pspec);
static void      bse_sub_oport_finalize         (GObject                *object);
static void      bse_sub_oport_set_parent       (BseItem                *item,
                                                 BseItem                *parent);
static void      bse_sub_oport_context_create   (BseSource              *source,
                                                 guint                   instance_id,
                                                 GslTrans               *trans);
static void      bse_sub_oport_context_connect  (BseSource              *source,
                                                 guint                   context_handle,
                                                 GslTrans               *trans);
static void      bse_sub_oport_context_dismiss  (BseSource              *source,
                                                 guint                   context_handle,
                                                 GslTrans               *trans);
static void      bse_sub_oport_update_modules   (BseSubOPort            *self,
                                                 const gchar            *old_name,
                                                 const gchar            *new_name,
                                                 guint                   port);


/* --- variables --- */
static gpointer          parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSubOPort)
{
  static const GTypeInfo type_info = {
    sizeof (BseSubOPortClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_sub_oport_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSubOPort),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_sub_oport_init,
  };
#include "./icons/virtual-output.c"
  GType type = bse_type_register_static (BSE_TYPE_SOURCE,
                                         "BseSubOPort",
                                         "Virtual output port connector, used to provide a synthesis network "
                                         "with output signals from other synthesis networks",
                                         &type_info);
  bse_categories_register_stock_module (N_("/Virtualization/Virtual Output"), type, virtual_output_pixstream);
  return type;
}

static void
bse_sub_oport_class_init (BseSubOPortClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint i, channel_id;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_sub_oport_set_property;
  gobject_class->get_property = bse_sub_oport_get_property;
  gobject_class->finalize = bse_sub_oport_finalize;
  
  item_class->set_parent = bse_sub_oport_set_parent;
  
  source_class->context_create = bse_sub_oport_context_create;
  source_class->context_connect = bse_sub_oport_context_connect;
  source_class->context_dismiss = bse_sub_oport_context_dismiss;
  
  class->n_output_ports = BSE_SUB_OPORT_N_PORTS;
  
  for (i = 0; i < class->n_output_ports; i++)
    {
      gchar *ident, *label, *value;
      
      ident = g_strdup_printf ("out_port_%u", i + 1);
      label = g_strdup_printf (_("Output Port %u"), i + 1);
      value = g_strdup_printf ("synth_out_%u", i + 1);
      bse_object_class_add_param (object_class, _("Assignments"), PROP_OPORT_NAME + i * 2,
                                  sfi_pspec_string (ident, label,
						    _("The port name is a unique name to establish input<->output "
                                                      "port relationships"),
						    value, SFI_PARAM_STANDARD ":skip-default"));
      g_free (ident);
      g_free (label);
      g_free (value);
      
      ident = g_strdup_printf ("input-%u", i + 1);
      label = g_strdup_printf (_("Virtual output %u"), i + 1);
      channel_id = bse_source_class_add_ichannel (source_class, ident, label, NULL);
      g_assert (channel_id == i);
      g_free (ident);
      g_free (label);
    }
}

static void
bse_sub_oport_init (BseSubOPort *self)
{
  BseSubOPortClass *class = BSE_SUB_OPORT_GET_CLASS (self);
  guint i;
  
  self->output_ports = g_new (gchar*, class->n_output_ports);
  for (i = 0; i < class->n_output_ports; i++)
    self->output_ports[i] = g_strdup_printf ("synth_out_%u", i + 1);
}

static void
bse_sub_oport_finalize (GObject *object)
{
  BseSubOPort *self = BSE_SUB_OPORT (object);
  BseSubOPortClass *class = BSE_SUB_OPORT_GET_CLASS (self);
  guint i;
  
  for (i = 0; i < class->n_output_ports; i++)
    g_free (self->output_ports[i]);
  g_free (self->output_ports);
  self->output_ports = NULL;
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bse_sub_oport_set_property (GObject      *object,
                            guint         param_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BseSubOPort *self = BSE_SUB_OPORT (object);
  BseItem *item = BSE_ITEM (self);
  
  switch (param_id)
    {
      guint indx, n;
    default:
      indx = (param_id - PROP_OPORT_NAME) % 2 + PROP_OPORT_NAME;
      n = (param_id - PROP_OPORT_NAME) / 2;
      switch (indx)
        {
          const gchar *name;
        case PROP_OPORT_NAME:
          name = g_value_get_string (value);
          if (item->parent)
            {
              bse_snet_oport_name_unregister (BSE_SNET (item->parent), self->output_ports[n]);
              name = bse_snet_oport_name_register (BSE_SNET (item->parent), name);
            }
	  if (BSE_SOURCE_PREPARED (self))
	    bse_sub_oport_update_modules (self, self->output_ports[n], name, n);
          g_free (self->output_ports[n]);
          self->output_ports[n] = g_strdup (name);
          break;
        default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
          break;
        }
    }
}

static void
bse_sub_oport_get_property (GObject     *object,
                            guint        param_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
  BseSubOPort *self = BSE_SUB_OPORT (object);
  
  switch (param_id)
    {
      guint indx, n;
    default:
      indx = (param_id - PROP_OPORT_NAME) % 2 + PROP_OPORT_NAME;
      n = (param_id - PROP_OPORT_NAME) / 2;
      switch (indx)
        {
        case PROP_OPORT_NAME:
          g_value_set_string (value, self->output_ports[n]);
          break;
        default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
          break;
        }
      break;
    }
}

static void
bse_sub_oport_set_parent (BseItem *item,
                          BseItem *parent)
{
  BseSubOPort *self = BSE_SUB_OPORT (item);
  BseSubOPortClass *class = BSE_SUB_OPORT_GET_CLASS (self);
  guint i;
  
  /* remove port name from old parent */
  if (item->parent)
    for (i = 0; i < class->n_output_ports; i++)
      bse_snet_oport_name_unregister (BSE_SNET (item->parent), self->output_ports[i]);
  
  /* chain parent class' handler */
  BSE_ITEM_CLASS (parent_class)->set_parent (item, parent);
  
  /* add port name to new parent */
  if (item->parent)
    for (i = 0; i < class->n_output_ports; i++)
      {
        const gchar *name = bse_snet_oport_name_register (BSE_SNET (item->parent), self->output_ports[i]);
        if (strcmp (name, self->output_ports[i]) != 0)
          {
            gchar *string;
            
            g_free (self->output_ports[i]);
            self->output_ports[i] = g_strdup (name);
            string = g_strdup_printf ("out_port_%u", i + 1);
            g_object_notify (G_OBJECT (item), string);
            g_free (string);
          }
      }
}

static void
sub_oport_process (GslModule *module,
                   guint      n_values)
{
  guint i, n = GSL_MODULE_N_ISTREAMS (module);
  
  for (i = 0; i < n; i++)
    GSL_MODULE_OBUFFER (module, i) = (gfloat*) GSL_MODULE_IBUFFER (module, i);
}

static void
bse_sub_oport_context_create (BseSource *source,
                              guint      context_handle,
                              GslTrans  *trans)
{
  BseSubOPortClass *class = BSE_SUB_OPORT_GET_CLASS (source);
  GslModule *module;
  
  if (class->gsl_class.process == NULL)
    {
      class->gsl_class.n_istreams = class->n_output_ports;
      class->gsl_class.n_jstreams = 0;
      class->gsl_class.n_ostreams = class->n_output_ports;
      class->gsl_class.process = sub_oport_process;
      class->gsl_class.free = NULL;
      class->gsl_class.mflags = GSL_COST_CHEAP;
    }
  module = gsl_module_new (&class->gsl_class, NULL);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_imodule (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_sub_oport_context_connect (BseSource *source,
                               guint      context_handle,
                               GslTrans  *trans)
{
  BseSubOPort *self = BSE_SUB_OPORT (source);
  BseSubOPortClass *class = BSE_SUB_OPORT_GET_CLASS (self);
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = BSE_SNET (item->parent);
  GslModule *module = bse_source_get_context_imodule (source, context_handle);
  guint i;
  
  for (i = 0; i < class->n_output_ports; i++)
    bse_snet_set_oport_src (snet, self->output_ports[i], context_handle,
			    module, i, trans);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_sub_oport_context_dismiss (BseSource *source,
                               guint      context_handle,
                               GslTrans  *trans)
{
  BseSubOPort *self = BSE_SUB_OPORT (source);
  BseSubOPortClass *class = BSE_SUB_OPORT_GET_CLASS (self);
  BseItem *item = BSE_ITEM (self);
  BseSNet *snet = BSE_SNET (item->parent);
  guint i;
  
  for (i = 0; i < class->n_output_ports; i++)
    bse_snet_set_oport_src (snet, self->output_ports[i], context_handle,
			    NULL, i, trans);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}

static void
bse_sub_oport_update_modules (BseSubOPort *self,
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
      GslModule *module = bse_source_get_context_imodule (source, cids[i]);
      
      bse_snet_set_oport_src (snet, old_name, cids[i], NULL, port, trans);
      bse_snet_set_oport_src (snet, new_name, cids[i], module, port, trans);
    }
  g_free (cids);
  gsl_trans_commit (trans);
}
