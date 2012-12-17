/* BseAdder - BSE Adder
 * Copyright (C) 1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bseadder.h"

#include <bse/bseengine.h>

#include <string.h>


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_SUBTRACT,
};


/* --- prototypes --- */
static void	 bse_adder_init			(BseAdder	*self);
static void	 bse_adder_class_init		(BseAdderClass	*klass);
static void      bse_adder_class_finalize       (BseAdderClass  *klass);
static void	 bse_adder_set_property		(GObject        *object,
						 guint           param_id,
						 const GValue   *value,
						 GParamSpec     *pspec);
static void	 bse_adder_get_property		(GObject        *object,
						 guint           param_id,
						 GValue         *value,
						 GParamSpec     *pspec);
static BseIcon*	 bse_adder_do_get_icon		(BseObject	*object);
static void      bse_adder_context_create       (BseSource      *source,
						 guint           context_handle,
						 BseTrans       *trans);
static void	 bse_adder_update_modules	(BseAdder	*self,
						 BseTrans	*trans);


/* --- Export to BSE --- */
#include "./icons/sum.c"
BSE_REGISTER_OBJECT (BseAdder, BseSource, "/Modules/Routing/Adder", "deprecated",
                     "The Adder is a very simplisitic prototype mixer that just sums up "
                     "incoming signals (it does allow for switching to subtract mode though)",
                     sum_icon,
                     bse_adder_class_init, bse_adder_class_finalize, bse_adder_init);
BSE_DEFINE_EXPORTS ();


/* --- variables --- */
static gpointer		 parent_class = NULL;


/* --- functions --- */
static void
bse_adder_class_init (BseAdderClass *klass)
{
#include "./icons/sub.c"
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  guint channel;

  parent_class = g_type_class_peek (BSE_TYPE_SOURCE);
  
  gobject_class->set_property = bse_adder_set_property;
  gobject_class->get_property = bse_adder_get_property;
  
  object_class->get_icon = bse_adder_do_get_icon;
  
  source_class->context_create = bse_adder_context_create;
  
  klass->sub_icon = bse_icon_from_pixstream (sub_pixstream);
  
  bse_object_class_add_param (object_class, "Features",
			      PARAM_SUBTRACT,
			      sfi_pspec_bool ("subtract", "Subtract instead",
						   "Use subtraction to combine sample"
						   "values (instead of addition)",
						   FALSE,
						   SFI_PARAM_STANDARD ":skip-default"));
  
  channel = bse_source_class_add_jchannel (source_class, "audio-in1", _("Audio In1"), _("Audio Input 1"));
  g_assert (channel == BSE_ADDER_JCHANNEL_AUDIO1);
  channel = bse_source_class_add_jchannel (source_class, "audio-in2", _("Audio In2"), _("Audio Input 2"));
  g_assert (channel == BSE_ADDER_JCHANNEL_AUDIO2);
  channel = bse_source_class_add_ochannel (source_class, "audio-out", _("Audio Out"), _("Audio Output"));
  g_assert (channel == BSE_ADDER_OCHANNEL_AUDIO_OUT);
}

static void
bse_adder_class_finalize (BseAdderClass *klass)
{
  bse_icon_free (klass->sub_icon);
  klass->sub_icon = NULL;
}

static void
bse_adder_init (BseAdder *self)
{
  self->subtract = FALSE;
}

static BseIcon*
bse_adder_do_get_icon (BseObject *object)
{
  BseAdder *self = BSE_ADDER (object);
  
  if (self->subtract)
    return BSE_ADDER_GET_CLASS (self)->sub_icon;
  else /* chain parent class' handler */
    return BSE_OBJECT_CLASS (parent_class)->get_icon (object);
}

static void
bse_adder_set_property (GObject      *object,
			guint         param_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  BseAdder *self = BSE_ADDER (object);

  switch (param_id)
    {
    case PARAM_SUBTRACT:
      self->subtract = sfi_value_get_bool (value);
      bse_adder_update_modules (self, NULL);
      bse_object_notify_icon_changed (BSE_OBJECT (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_adder_get_property (GObject    *object,
			guint       param_id,
			GValue     *value,
			GParamSpec *pspec)
{
  BseAdder *self = BSE_ADDER (object);

  switch (param_id)
    {
    case PARAM_SUBTRACT:
      sfi_value_set_bool (value, self->subtract);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

typedef struct
{
  gboolean subtract;
} Adder;

static void
bse_adder_update_modules (BseAdder *self,
			  BseTrans *trans)
{
  if (BSE_SOURCE_PREPARED (self))
    bse_source_update_modules (BSE_SOURCE (self),
			       G_STRUCT_OFFSET (Adder, subtract),
			       &self->subtract,
			       sizeof (self->subtract),
			       trans);
}

static void
adder_process (BseModule *module,
	       guint      n_values)
{
  Adder *adder = (Adder*) module->user_data;
  guint n_au1 = BSE_MODULE_JSTREAM (module, BSE_ADDER_JCHANNEL_AUDIO1).n_connections;
  guint n_au2 = BSE_MODULE_JSTREAM (module, BSE_ADDER_JCHANNEL_AUDIO2).n_connections;
  gfloat *out, *audio_out = BSE_MODULE_OBUFFER (module, BSE_ADDER_OCHANNEL_AUDIO_OUT);
  gfloat *bound = audio_out + n_values;
  const gfloat *auin;
  guint i;

  if (!n_au1 && !n_au2)
    {
      module->ostreams[BSE_ADDER_OCHANNEL_AUDIO_OUT].values = bse_engine_const_values (0);
      return;
    }
  if (n_au1)	/* sum up audio1 inputs */
    {
      auin = BSE_MODULE_JBUFFER (module, BSE_ADDER_JCHANNEL_AUDIO1, 0);
      out = audio_out;
      do
	*out++ = *auin++;
      while (out < bound);
      for (i = 1; i < n_au1; i++)
	{
	  auin = BSE_MODULE_JBUFFER (module, BSE_ADDER_JCHANNEL_AUDIO1, i);
	  out = audio_out;
	  do
	    *out++ += *auin++;
	  while (out < bound);
	}
    }
  else
    memset (audio_out, 0, n_values * sizeof (audio_out[0]));

  if (n_au2 && !adder->subtract)	/* sum up audio2 inputs */
    for (i = 0; i < n_au2; i++)
      {
	auin = BSE_MODULE_JBUFFER (module, BSE_ADDER_JCHANNEL_AUDIO2, i);
	out = audio_out;
	do
	  *out++ += *auin++;
	while (out < bound);
      }
  else if (n_au2)		/*  subtract audio2 inputs */
    for (i = 0; i < n_au2; i++)
      {
	auin = BSE_MODULE_JBUFFER (module, BSE_ADDER_JCHANNEL_AUDIO2, i);
	out = audio_out;
	do
	  *out++ -= *auin++;
	while (out < bound);
      }
}

static void
bse_adder_context_create (BseSource *source,
			  guint      context_handle,
			  BseTrans  *trans)
{
  static const BseModuleClass add_class = {
    0,				/* n_istreams */
    BSE_ADDER_N_JCHANNELS,	/* n_jstreams */
    BSE_ADDER_N_OCHANNELS,	/* n_ostreams */
    adder_process,		/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    (BseModuleFreeFunc) g_free,	/* free */
    BSE_COST_CHEAP,		/* cost */
  };
  BseAdder *adder = BSE_ADDER (source);
  Adder *add = g_new0 (Adder, 1);
  BseModule *module;

  module = bse_module_new (&add_class, add);

  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);

  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);

  /* update module data */
  bse_adder_update_modules (adder, trans);
}
