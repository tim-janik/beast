/* BseConstant - BSE Constant
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
#include "bseconstant.h"

#include <bse/gslengine.h>

#define	BSE_DFL_CONSTANT_VOLUME_dB	(BSE_DFL_MASTER_VOLUME_dB)

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_VALUE,
  PARAM_FREQ,
  PARAM_NOTE
};


/* --- prototypes --- */
static void	 bse_constant_init		(BseConstant	  *constant);
static void	 bse_constant_class_init	(BseConstantClass *class);
static void	 bse_constant_set_property	(GObject	  *object,
						 guint             param_id,
						 const GValue     *value,
						 GParamSpec       *pspec);
static void	 bse_constant_get_property	(GObject	  *object,
						 guint             param_id,
						 GValue           *value,
						 GParamSpec       *pspec);
static void	 bse_constant_context_create	(BseSource        *source,
						 guint             context_handle,
						 GslTrans         *trans);
static void	 bse_constant_update_modules	(BseConstant	  *constant,
						 GslTrans         *trans);


/* --- variables --- */
static GType	       type_id_constant = 0;
static gpointer	       parent_class = NULL;
static const GTypeInfo type_info_constant = {
  sizeof (BseConstantClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_constant_class_init,
  (GClassFinalizeFunc) NULL,
  NULL /* class_data */,
  
  sizeof (BseConstant),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_constant_init,
};


/* --- functions --- */
static void
bse_constant_class_init (BseConstantClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_constant_set_property;
  gobject_class->get_property = bse_constant_get_property;
  
  source_class->context_create = bse_constant_context_create;
  
  bse_object_class_add_param (object_class, "Constant Value",
			      PARAM_VALUE,
			      bse_param_spec_float ("value", "Value [float]", NULL,
						    -1.0, 1.0, 1.0, 0.01,
						    BSE_PARAM_DEFAULT | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Constant Value",
			      PARAM_FREQ,
			      bse_param_spec_float ("freq", "Frequency", NULL,
						    0, BSE_MAX_FREQUENCY,
						    BSE_MAX_FREQUENCY, 10.0,
						    BSE_PARAM_GUI | BSE_PARAM_HINT_DIAL));
  bse_object_class_add_param (object_class, "Constant Value",
			      PARAM_NOTE,
			      bse_param_spec_note ("note", "Note", NULL,
						   BSE_MIN_NOTE, BSE_MAX_NOTE,
						   BSE_NOTE_VOID, 1, TRUE,
						   BSE_PARAM_GUI));
  ochannel = bse_source_class_add_ochannel (source_class, "Const Out", "Constant Output");
  g_assert (ochannel == BSE_CONSTANT_OCHANNEL_MONO);
}

static void
bse_constant_init (BseConstant *constant)
{
  constant->constant_value = 1.0;
}

static void
bse_constant_set_property (GObject      *object,
			   guint         param_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  BseConstant *constant = BSE_CONSTANT (object);

  switch (param_id)
    {
    case PARAM_VALUE:
      constant->constant_value = g_value_get_float (value);
      bse_constant_update_modules (constant, NULL);
      g_object_notify (object, "freq");
      g_object_notify (object, "note");
      break;
    case PARAM_FREQ:
      constant->constant_value = BSE_VALUE_FROM_FREQ (g_value_get_float (value));
      bse_constant_update_modules (constant, NULL);
      g_object_notify (object, "value");
      g_object_notify (object, "note");
      break;
    case PARAM_NOTE:
      constant->constant_value = BSE_VALUE_FROM_FREQ (bse_note_to_freq (bse_value_get_note (value)));
      bse_constant_update_modules (constant, NULL);
      g_object_notify (object, "value");
      g_object_notify (object, "freq");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (constant, param_id, pspec);
      break;
    }
}

static void
bse_constant_get_property (GObject     *object,
			   guint        param_id,
			   GValue      *value,
			   GParamSpec  *pspec)
{
  BseConstant *constant = BSE_CONSTANT (object);

  switch (param_id)
    {
    case PARAM_VALUE:
      g_value_set_float (value, constant->constant_value);
      break;
    case PARAM_FREQ:
      g_value_set_float (value, BSE_FREQ_FROM_VALUE (constant->constant_value));
      break;
    case PARAM_NOTE:
      bse_value_set_note (value, bse_note_from_freq (BSE_FREQ_FROM_VALUE (constant->constant_value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (constant, param_id, pspec);
      break;
    }
}

typedef struct
{
  gfloat constant_value;
} Constant;

static void
bse_constant_update_modules (BseConstant *constant,
			     GslTrans    *trans)
{
  if (BSE_SOURCE_PREPARED (constant))
    bse_source_update_omodules (BSE_SOURCE (constant),
				BSE_CONSTANT_OCHANNEL_MONO,
				G_STRUCT_OFFSET (Constant, constant_value),
				&constant->constant_value,
				sizeof (constant->constant_value),
				trans);
}

static void
constant_process (GslModule *module,
		  guint      n_values)
{
  Constant *constant = module->user_data;
  
  module->ostreams[0].values = gsl_engine_const_values (constant->constant_value);
}

static void
bse_constant_context_create (BseSource *source,
			     guint      context_handle,
			     GslTrans  *trans)
{
  static const GslClass constant_class = {
    0,				/* n_istreams */
    0,                          /* n_jstreams */
    1,				/* n_ostreams */
    constant_process,		/* process */
    (GslModuleFreeFunc) g_free,	/* free */
    GSL_COST_CHEAP,		/* flags */
  };
  Constant *constant = g_new0 (Constant, 1);
  GslModule *module;
  
  module = gsl_module_new (&constant_class, constant);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
  
  /* update (initialize) module data */
  bse_constant_update_modules (BSE_CONSTANT (source), trans);
}


/* --- Export to BSE --- */
#include "./icons/const.c"
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_constant, "BseConstant", "BseSource",
    "This module provides a constant output value",
    &type_info_constant,
    "/Modules/Constant",
    { CONST_IMAGE_BYTES_PER_PIXEL | BSE_PIXDATA_1BYTE_RLE,
      CONST_IMAGE_WIDTH, CONST_IMAGE_HEIGHT,
      CONST_IMAGE_RLE_PIXEL_DATA, },
  },
  { NULL, },
};
BSE_EXPORTS_END;
