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
  /* don't add params after here */
  PARAM_IPORT_NAME,
  PARAM_OPORT_NAME
};


/* --- prototypes --- */
static void	 bse_sub_synth_init		(BseSubSynth		*scard);
static void	 bse_sub_synth_class_init	(BseSubSynthClass	*class);
static void	 bse_sub_synth_set_property	(GObject                *object,
						 guint                   param_id,
						 const GValue           *value,
						 GParamSpec             *pspec);
static void	 bse_sub_synth_get_property	(GObject                *object,
						 guint                   param_id,
						 GValue                 *value,
						 GParamSpec             *pspec);
static void	 bse_sub_synth_do_destroy	(BseObject		*object);
static void	 bse_sub_synth_context_create	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);
static void	 bse_sub_synth_context_connect	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);
static void	 bse_sub_synth_context_dismiss	(BseSource		*source,
						 guint			 instance_id,
						 GslTrans		*trans);
static void  bse_sub_synth_update_port_contexts (BseSubSynth		*self,
						 const gchar		*old_name,
						 const gchar		*new_name,
						 gboolean		 is_input,
						 guint			 port);


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
    (GClassFinalizeFunc) NULL,
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
  bse_categories_register_icon ("/Modules/Virtualization/Virtual Sub Synth",
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
  guint channel_id, i;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_sub_synth_set_property;
  gobject_class->get_property = bse_sub_synth_get_property;
  
  object_class->destroy = bse_sub_synth_do_destroy;
  
  source_class->context_create = bse_sub_synth_context_create;
  source_class->context_connect = bse_sub_synth_context_connect;
  source_class->context_dismiss = bse_sub_synth_context_dismiss;
  
  bse_object_class_add_param (object_class, "Assignments",
			      PARAM_SNET,
			      g_param_spec_object ("snet", "Synthesis Network", "The synthesis network to interface to",
						   BSE_TYPE_SNET, BSE_PARAM_DEFAULT));
  for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
    {
      gchar *string, *name, *value;

      string = g_strdup_printf ("in_port_%u", i + 1);
      name = g_strdup_printf ("Input Port %u", i + 1);
      value = g_strdup_printf ("synth_in_%u", i + 1);
      bse_object_class_add_param (object_class, "Input Assignments", PARAM_IPORT_NAME + i * 2,
				  bse_param_spec_string (string, name, "Output port name to interface from",
							 value, BSE_PARAM_DEFAULT));
      g_free (string);
      g_free (name);
      g_free (value);

      string = g_strdup_printf ("out_port_%u", i + 1);
      name = g_strdup_printf ("Output Port %u", i + 1);
      value = g_strdup_printf ("synth_out_%u", i + 1);
      bse_object_class_add_param (object_class, "Output Assignments", PARAM_OPORT_NAME + i * 2,
				  bse_param_spec_string (string, name, "Input port name to interface to",
							 value, BSE_PARAM_DEFAULT));
      g_free (string);
      g_free (name);
      g_free (value);

      string = g_strdup_printf ("Input %u", i + 1);
      name = g_strdup_printf ("Virtual input %u", i + 1);
      channel_id = bse_source_class_add_ichannel (source_class, string, name);
      g_assert (channel_id == i);
      g_free (string);
      g_free (name);

      string = g_strdup_printf ("Output %u", i + 1);
      name = g_strdup_printf ("Virtual output %u", i + 1);
      channel_id = bse_source_class_add_ochannel (source_class, string, name);
      g_assert (channel_id == i);
      g_free (string);
      g_free (name);
    }
}

static void
bse_sub_synth_init (BseSubSynth *synth)
{
  guint i;

  synth->snet = NULL;

  for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
    {
      synth->input_ports[i] = g_strdup_printf ("synth_in_%u", i + 1);
      synth->output_ports[i] = g_strdup_printf ("synth_out_%u", i + 1);
    }
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
  for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
    {
      g_free (synth->input_ports[i]);
      synth->input_ports[i] = NULL;
      g_free (synth->output_ports[i]);
      synth->output_ports[i] = NULL;
    }
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static gboolean
find_name (BseSubSynth *self,
	   const gchar *name,
	   gboolean     is_input)
{
  guint i;

  for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
    {
      gchar *test = is_input ? self->input_ports[i] : self->output_ports[i];

      if (test && strcmp (test, name) == 0)
	return TRUE;
    }
  return FALSE;
}

static gchar*
dup_name_unique (BseSubSynth *self,
		 const gchar *tmpl,
		 gboolean     is_input)
{
  gchar *name = g_strdup (tmpl);
  guint i = 1;

  while (find_name (self, name, is_input))
    {
      g_free (name);
      name = g_strdup_printf ("%s-%u", name, i++);
    }
  return name;
}

static void
bse_sub_synth_set_property (GObject      *object,
			    guint         param_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  BseSubSynth *self = BSE_SUB_SYNTH (object);

  switch (param_id)
    {
      guint indx, n;
    case PARAM_SNET:
      if (self->snet)
	g_object_unref (self->snet);
      self->snet = g_value_get_object (value);
      if (self->snet)
	g_object_ref (self->snet);	// FIXME: use cross-refs
      break;
    default:
      indx = (param_id - PARAM_IPORT_NAME) % 2 + PARAM_IPORT_NAME;
      n = (param_id - PARAM_IPORT_NAME) / 2;
      switch (indx)
	{
	  gchar *old_name;
	case PARAM_IPORT_NAME:
	  old_name = self->input_ports[n];
	  self->input_ports[n] = NULL;	/* exempt from checks */
	  self->input_ports[n] = dup_name_unique (self, g_value_get_string (value), TRUE);
	  if (BSE_SOURCE_PREPARED (self))
	    bse_sub_synth_update_port_contexts (self, old_name, self->input_ports[n], TRUE, n);
	  g_free (old_name);
	  break;
	case PARAM_OPORT_NAME:
	  old_name = self->output_ports[n];
	  self->output_ports[n] = NULL; /* exempt from checks */
	  self->output_ports[n] = dup_name_unique (self, g_value_get_string (value), TRUE);
	  if (BSE_SOURCE_PREPARED (self))
	    bse_sub_synth_update_port_contexts (self, old_name, self->output_ports[n], FALSE, n);
	  g_free (old_name);
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
	  break;
	}
    }
}

static void
bse_sub_synth_get_property (GObject    *object,
			    guint       param_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  BseSubSynth *self = BSE_SUB_SYNTH (object);

  switch (param_id)
    {
      guint indx, n;
    case PARAM_SNET:
      g_value_set_object (value, self->snet);
      break;
    default:
      indx = (param_id - PARAM_IPORT_NAME) % 2 + PARAM_IPORT_NAME;
      n = (param_id - PARAM_IPORT_NAME) / 2;
      switch (indx)
	{
	case PARAM_IPORT_NAME:
	  g_value_set_string (value, self->input_ports[n]);
	  break;
	case PARAM_OPORT_NAME:
	  g_value_set_string (value, self->output_ports[n]);
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
	  break;
	}
      break;
    }
}

void
bse_sub_synth_set_snet (BseSubSynth *sub_synth,
			BseSNet     *snet)
{
  g_return_if_fail (BSE_IS_SUB_SYNTH (sub_synth));

  if (sub_synth->snet != snet)
    g_object_set (sub_synth, "snet", snet, NULL);
}

typedef struct {
  guint  synth_context_handle;
} ModData;

static void
sub_synths_process (GslModule *module,
		    guint      n_values)
{
  guint i;

  for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
    GSL_MODULE_OBUFFER (module, i) = (gfloat*) GSL_MODULE_IBUFFER (module, i);
}

static void
bse_sub_synth_context_create (BseSource *source,
			      guint      context_handle,
			      GslTrans  *trans)
{
  static const GslClass sub_synth_mclass = {
    BSE_SUB_SYNTH_N_IOPORTS,	/* n_istreams */
    0,				/* n_jstreams */
    BSE_SUB_SYNTH_N_IOPORTS,	/* n_ostreams */
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
    foreign_context_handle = bse_snet_create_context (snet, trans);

  mdata_in->synth_context_handle = foreign_context_handle;
  mdata_out->synth_context_handle = foreign_context_handle;
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_imodule (source, context_handle, imodule);
  bse_source_set_context_omodule (source, context_handle, omodule);

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
  BseSubSynth *self = BSE_SUB_SYNTH (source);
  BseSNet *snet = self->snet;
  guint i;
  
  /* connect module to sub synthesizer */
  if (snet)
    {
      GslModule *imodule = bse_source_get_context_imodule (source, context_handle);
      GslModule *omodule = bse_source_get_context_omodule (source, context_handle);
      ModData *mdata_in = imodule->user_data;
      guint foreign_context_handle = mdata_in->synth_context_handle;

      bse_source_connect_context (BSE_SOURCE (snet), foreign_context_handle, trans);
      for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
	{
	  bse_snet_set_iport_src (snet, self->input_ports[i], foreign_context_handle,
				  imodule, i, trans);
	  bse_snet_set_oport_dest (snet, self->output_ports[i], foreign_context_handle,
				   omodule, i, trans);
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
  BseSubSynth *self = BSE_SUB_SYNTH (source);
  BseSNet *snet = self->snet;

  if (snet)
    {
      GslModule *imodule = bse_source_get_context_imodule (source, context_handle);
      ModData *mdata_in = imodule->user_data;
      guint i, foreign_context_handle = mdata_in->synth_context_handle;
      
      for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
	{
	  bse_snet_set_iport_src (snet, self->input_ports[i], foreign_context_handle, NULL, i, trans);
	  bse_snet_set_oport_dest (snet, self->output_ports[i], foreign_context_handle, NULL, i, trans);
	}
      bse_source_dismiss_context (BSE_SOURCE (snet), mdata_in->synth_context_handle, trans);
    }
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}

static void
bse_sub_synth_update_port_contexts (BseSubSynth *self,
				    const gchar *old_name,
				    const gchar *new_name,
				    gboolean     is_input,
				    guint	 port)
{
  BseSNet *snet = self->snet;
  BseSource *source = BSE_SOURCE (self);
  GslTrans *trans = gsl_trans_open ();
  guint *cids, n, i;

  g_return_if_fail (BSE_SOURCE_PREPARED (self));

  cids = bse_source_context_ids (source, &n);
  for (i = 0; i < n; i++)
    if (is_input)
      {
	GslModule *imodule = bse_source_get_context_imodule (source, cids[i]);
	ModData *mdata_in = imodule->user_data;

	bse_snet_set_iport_src (snet, old_name, mdata_in->synth_context_handle, NULL, port, trans);
	bse_snet_set_iport_src (snet, new_name, mdata_in->synth_context_handle, imodule, port, trans);
      }
  else
    {
	GslModule *omodule = bse_source_get_context_omodule (source, cids[i]);
	ModData *mdata_in = omodule->user_data;

	bse_snet_set_oport_dest (snet, old_name, mdata_in->synth_context_handle, NULL, port, trans);
	bse_snet_set_oport_dest (snet, new_name, mdata_in->synth_context_handle, omodule, port, trans);
      }
  g_free (cids);
  gsl_trans_commit (trans);
}
