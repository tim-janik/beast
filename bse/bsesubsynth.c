/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1999, 2000-2003 Tim Janik
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
#include "bsesubsynth.h"

#include "bsecategories.h"
#include "bsecsynth.h"
#include "bseproject.h"
#include "bsemidireceiver.h"
#include "bseengine.h"

#include <string.h>

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
static void	 bse_sub_synth_do_dispose	(GObject		*object);
static void	 bse_sub_synth_context_create	(BseSource		*source,
						 guint			 instance_id,
						 BseTrans		*trans);
static void	 bse_sub_synth_context_connect	(BseSource		*source,
						 guint			 instance_id,
						 BseTrans		*trans);
static void	 bse_sub_synth_context_dismiss	(BseSource		*source,
						 guint			 instance_id,
						 BseTrans		*trans);
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
#include "./icons/virtual-synth.c"
  GType type = bse_type_register_static (BSE_TYPE_SOURCE,
                                         "BseSubSynth",
                                         "This module encapsulates whole synthesizer networks, by "
                                         "interfacing to/from their virtual input and output ports",
                                         &sub_synth_info);
  bse_categories_register_stock_module (N_("/Virtualization/Virtual Sub Synth"), type, virtual_synth_pixstream);
  return type;
}

static void
bse_sub_synth_init (BseSubSynth *synth)
{
  guint i;
  
  synth->snet = NULL;
  synth->null_shortcut = FALSE;
  
  for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
    {
      synth->input_ports[i] = g_strdup_printf ("synth_in_%u", i + 1);
      synth->output_ports[i] = g_strdup_printf ("synth_out_%u", i + 1);
    }
}

static void
bse_sub_synth_do_dispose (GObject *object)
{
  BseSubSynth *self = BSE_SUB_SYNTH (object);
  guint i;
  
  if (self->snet)
    {
      g_object_unref (self->snet);
      self->snet = NULL;
    }
  for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
    {
      g_free (self->input_ports[i]);
      self->input_ports[i] = NULL;
      g_free (self->output_ports[i]);
      self->output_ports[i] = NULL;
    }
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_sub_synth_get_candidates (BseItem               *item,
                              guint                  param_id,
                              BsePropertyCandidates *pc,
                              GParamSpec            *pspec)
{
  BseSubSynth *self = BSE_SUB_SYNTH (item);
  switch (param_id)
    {
    case PARAM_SNET:
      bse_property_candidate_relabel (pc, _("Available Synthesizers"), _("List of available synthesis networks to choose a sub network from"));
      bse_item_gather_items_typed (item, pc->items, BSE_TYPE_CSYNTH, BSE_TYPE_PROJECT, FALSE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
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
sub_synth_uncross_snet (BseItem *owner,
                        BseItem *ref_item)
{
  BseSubSynth *self = BSE_SUB_SYNTH (owner);
  bse_item_set (self, "snet", NULL, NULL);
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
      if (!BSE_SOURCE_PREPARED (self))
	{
          if (self->snet)
            {
              bse_object_unproxy_notifies (self->snet, self, "notify::snet");
              bse_item_cross_unlink (BSE_ITEM (self), BSE_ITEM (self->snet), sub_synth_uncross_snet);
              self->snet = NULL;
            }
	  self->snet = bse_value_get_object (value);
	  if (self->snet)
            {
              bse_item_cross_link (BSE_ITEM (self), BSE_ITEM (self->snet), sub_synth_uncross_snet);
              bse_object_proxy_notifies (self->snet, self, "notify::snet");
            }
	}
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
      bse_value_set_object (value, self->snet);
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
bse_sub_synth_set_midi_channel (BseSubSynth     *self,
                                guint            midi_channel)
{
  g_return_if_fail (BSE_IS_SUB_SYNTH (self));
  
  self->midi_channel = midi_channel;
}

void
bse_sub_synth_set_null_shortcut (BseSubSynth *self,
                                 gboolean     enabled)
{
  g_return_if_fail (BSE_IS_SUB_SYNTH (self));

  self->null_shortcut = enabled != FALSE;
}

typedef struct {
  guint  synth_context_handle;
} ModData;

static void
bse_sub_synth_context_create (BseSource *source,
			      guint      context_handle,
			      BseTrans  *trans)
{
  static GSList *recursion_stack = NULL;
  BseSubSynth *self = BSE_SUB_SYNTH (source);
  BseSNet *snet = self->snet;
  ModData *mdata_in = g_new0 (ModData, 1);
  ModData *mdata_out = g_new0 (ModData, 1);
  BseModule *imodule = bse_module_new_virtual (BSE_SUB_SYNTH_N_IOPORTS, mdata_in, g_free);
  BseModule *omodule = bse_module_new_virtual (BSE_SUB_SYNTH_N_IOPORTS, mdata_out, g_free);
  guint foreign_context_handle = 0, shortcut = FALSE;

  /* create new context for foreign synth */
  if (snet && g_slist_find (recursion_stack, source))
    {
      g_warning ("%s: not creating modules for %s due to infinite recursion",
		 bse_object_debug_name (self), bse_object_debug_name (snet));
    }
  else if (snet)
    {
      BseItem *parent = BSE_ITEM (self)->parent;
      BseMidiContext mcontext = bse_snet_get_midi_context (BSE_SNET (parent), context_handle);
      if (self->midi_channel)
	mcontext.midi_channel = self->midi_channel;
      recursion_stack = g_slist_prepend (recursion_stack, self);
      foreign_context_handle = bse_snet_create_context (snet, mcontext, trans);
      recursion_stack = g_slist_remove (recursion_stack, self);
      g_assert (foreign_context_handle > 0);
    }
  else
    shortcut = self->null_shortcut;

  mdata_in->synth_context_handle = foreign_context_handle;
  mdata_out->synth_context_handle = foreign_context_handle;
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_imodule (source, context_handle, imodule);
  bse_source_set_context_omodule (source, context_handle, omodule);
  
  /* commit modules to engine */
  bse_trans_add (trans, bse_job_integrate (imodule));
  bse_trans_add (trans, bse_job_integrate (omodule));
  
  if (shortcut)
    {
      guint i;
      for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
        bse_trans_add (trans, bse_job_connect (imodule, i, omodule, i));
    }

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_sub_synth_context_connect (BseSource *source,
			       guint      context_handle,
			       BseTrans  *trans)
{
  BseSubSynth *self = BSE_SUB_SYNTH (source);
  BseSNet *snet = self->snet;
  guint i;
  
  /* connect module to sub synthesizer */
  if (snet)
    {
      BseModule *imodule = bse_source_get_context_imodule (source, context_handle);
      BseModule *omodule = bse_source_get_context_omodule (source, context_handle);
      ModData *mdata_in = imodule->user_data;
      guint foreign_context_handle = mdata_in->synth_context_handle;
      
      if (foreign_context_handle)
	{
	  bse_source_connect_context (BSE_SOURCE (snet), foreign_context_handle, trans);
	  for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
	    {
	      bse_snet_set_iport_src (snet, self->input_ports[i], foreign_context_handle,
				      imodule, i, trans);
	      bse_snet_set_oport_dest (snet, self->output_ports[i], foreign_context_handle,
				       omodule, i, trans);
	    }
	}
    }
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
}

static void
bse_sub_synth_context_dismiss (BseSource *source,
			       guint      context_handle,
			       BseTrans  *trans)
{
  BseSubSynth *self = BSE_SUB_SYNTH (source);
  BseSNet *snet = self->snet;
  
  if (snet)
    {
      BseModule *imodule = bse_source_get_context_imodule (source, context_handle);
      ModData *mdata_in = imodule->user_data;
      guint i, foreign_context_handle = mdata_in->synth_context_handle;
      
      if (foreign_context_handle)
	{
	  for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
	    {
	      bse_snet_set_iport_src (snet, self->input_ports[i], foreign_context_handle, NULL, i, trans);
	      bse_snet_set_oport_dest (snet, self->output_ports[i], foreign_context_handle, NULL, i, trans);
	    }
	  bse_source_dismiss_context (BSE_SOURCE (snet), foreign_context_handle, trans);
	}
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
  BseTrans *trans = bse_trans_open ();
  guint *cids, n, i;
  
  g_return_if_fail (BSE_SOURCE_PREPARED (self));
  
  cids = bse_source_context_ids (source, &n);
  for (i = 0; i < n; i++)
    if (is_input)
      {
	BseModule *imodule = bse_source_get_context_imodule (source, cids[i]);
	ModData *mdata_in = imodule->user_data;
	guint foreign_context_handle = mdata_in->synth_context_handle;
	
	if (foreign_context_handle)
	  {
	    bse_snet_set_iport_src (snet, old_name, foreign_context_handle, NULL, port, trans);
	    bse_snet_set_iport_src (snet, new_name, foreign_context_handle, imodule, port, trans);
	  }
      }
    else
      {
	BseModule *omodule = bse_source_get_context_omodule (source, cids[i]);
	ModData *mdata_in = omodule->user_data;
        guint foreign_context_handle = mdata_in->synth_context_handle;
	
        if (foreign_context_handle)
	  {
	    bse_snet_set_oport_dest (snet, old_name, foreign_context_handle, NULL, port, trans);
	    bse_snet_set_oport_dest (snet, new_name, foreign_context_handle, omodule, port, trans);
	  }
      }
  g_free (cids);
  bse_trans_commit (trans);
}

static void
bse_sub_synth_class_init (BseSubSynthClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseItemClass *item_class = BSE_ITEM_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel_id, i;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_sub_synth_set_property;
  gobject_class->get_property = bse_sub_synth_get_property;
  gobject_class->dispose = bse_sub_synth_do_dispose;
  
  item_class->get_candidates = bse_sub_synth_get_candidates;
  
  source_class->context_create = bse_sub_synth_context_create;
  source_class->context_connect = bse_sub_synth_context_connect;
  source_class->context_dismiss = bse_sub_synth_context_dismiss;
  
  bse_object_class_add_param (object_class, _("Assignments"),
			      PARAM_SNET,
			      bse_param_spec_object ("snet", _("Synthesizer"),
                                                     _("Synthesis network to use as embedded sub network"),
						     BSE_TYPE_CSYNTH, SFI_PARAM_STANDARD));
  for (i = 0; i < BSE_SUB_SYNTH_N_IOPORTS; i++)
    {
      gchar *ident, *label, *value;
      
      ident = g_strdup_printf ("in_port_%u", i + 1);
      label = g_strdup_printf (_("Input Port %u"), i + 1);
      value = g_strdup_printf ("synth_in_%u", i + 1);
      bse_object_class_add_param (object_class, _("Input Assignments"), PARAM_IPORT_NAME + i * 2,
				  sfi_pspec_string (ident, label, _("Output port name to interface from"),
						    value, SFI_PARAM_STANDARD ":skip-default"));
      g_free (ident);
      g_free (label);
      g_free (value);
      
      ident = g_strdup_printf ("out_port_%u", i + 1);
      label = g_strdup_printf (_("Output Port %u"), i + 1);
      value = g_strdup_printf ("synth_out_%u", i + 1);
      bse_object_class_add_param (object_class, _("Output Assignments"), PARAM_OPORT_NAME + i * 2,
				  sfi_pspec_string (ident, label, _("Input port name to interface to"),
						    value, SFI_PARAM_STANDARD ":skip-default"));
      g_free (ident);
      g_free (label);
      g_free (value);
      
      ident = g_strdup_printf ("input-%u", i + 1);
      label = g_strdup_printf (_("Virtual input %u"), i + 1);
      channel_id = bse_source_class_add_ichannel (source_class, ident, label, NULL);
      g_assert (channel_id == i);
      g_free (ident);
      g_free (label);
      
      ident = g_strdup_printf ("output-%u", i + 1);
      label = g_strdup_printf (_("Virtual output %u"), i + 1);
      channel_id = bse_source_class_add_ochannel (source_class, ident, label, NULL);
      g_assert (channel_id == i);
      g_free (ident);
      g_free (label);
    }
}
