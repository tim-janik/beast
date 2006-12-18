/* BseConstant - BSE Constant
 * Copyright (C) 1999-2002 Tim Janik
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
#include "bseconstant.h"

#include <bse/bsecategories.h>
#include <bse/bseengine.h>

#include <string.h>

#define	BSE_DFL_CONSTANT_VOLUME_dB	(BSE_DFL_MASTER_VOLUME_dB)

/* --- parameters --- */
enum
{
  PARAM_0,
  /* don't add params after here */
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
						 BseTrans         *trans);
static void	 bse_constant_update_modules	(BseConstant	  *constant,
						 BseTrans         *trans);


/* --- variables --- */
static gpointer	       parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseConstant)
{
  static const GTypeInfo type_info = {
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
#include "./icons/const.c"
  GType type_id;
  
  type_id = bse_type_register_static (BSE_TYPE_SOURCE,
				      "BseConstant",
				      "This module provides constant signal outputs",
                                      __FILE__, __LINE__,
                                      &type_info);
  bse_categories_register_stock_module (N_("/Other Sources/Constant"), type_id, const_pixstream);
  
  return type_id;
}

static void
bse_constant_class_init (BseConstantClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ochannel, i;
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_constant_set_property;
  gobject_class->get_property = bse_constant_get_property;
  
  source_class->context_create = bse_constant_context_create;
  
  for (i = 1; i <= BSE_CONSTANT_N_OUTPUTS; i++)
    {
      gchar *ident, *label, *blurb, *group = g_strdup_printf (_("Constant Output %u"), i);
      
      ident = g_strdup_printf ("value_%u", i);
      label = g_strdup_printf (_("Value [float]"));
      bse_object_class_add_param (object_class, group, PARAM_VALUE + (i - 1) * 3,
				  sfi_pspec_real (ident, label, _("Constant signal value"),
						  1.0, -1.0, 1.0, 0.01,
						  SFI_PARAM_STANDARD ":dial"));
      g_free (ident);
      g_free (label);
      ident = g_strdup_printf ("frequency_%u", i);
      label = g_strdup_printf (_("Frequency"));
      bse_object_class_add_param (object_class, group, PARAM_FREQ + (i - 1) * 3,
				  sfi_pspec_log_scale (ident, label, _("Constant signal value interpreted as frequency value in Hertz"),
						       BSE_MAX_FREQUENCY,
						       0, BSE_MAX_FREQUENCY,
						       10.0,
						       BSE_KAMMER_FREQUENCY * 2, 2, 4,
						       SFI_PARAM_GUI ":dial"));
      g_free (ident);
      g_free (label);
      ident = g_strdup_printf ("note_%u", i);
      label = g_strdup_printf (_("Note"));
      bse_object_class_add_param (object_class, group, PARAM_NOTE + (i - 1) * 3,
				  sfi_pspec_note (ident, label, _("Constant signal value as note, converted to Hertz according to the current musical tuning"),
						  SFI_KAMMER_NOTE, SFI_MIN_NOTE, SFI_MAX_NOTE,
						  TRUE, SFI_PARAM_GUI));
      g_free (ident);
      g_free (label);
      ident = g_strdup_printf ("const-out%u", i);
      label = g_strdup_printf (_("Const Out%u"), i);
      blurb = g_strdup_printf (_("Constant Output %u"), i);
      ochannel = bse_source_class_add_ochannel (source_class, ident, label, blurb);
      g_assert (ochannel == i - 1);
      g_free (ident);
      g_free (label);
      g_free (blurb);
      g_free (group);
    }
}

static void
bse_constant_init (BseConstant *constant)
{
  guint i;
  
  for (i = 0; i < BSE_CONSTANT_N_OUTPUTS; i++)
    constant->constants[i] = 1.0;
}

static void
bse_constant_set_property (GObject      *object,
			   guint         param_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  BseConstant *self = BSE_CONSTANT (object);
  
  switch (param_id)
    {
      guint indx, n;
    default:
      indx = (param_id - PARAM_VALUE) % 3;
      n = (param_id - PARAM_VALUE) / 3;
      switch (indx)
	{
	  gchar *prop;
	  SfiNote note;
	case PARAM_VALUE - PARAM_VALUE:
	  self->constants[n] = sfi_value_get_real (value);
	  bse_constant_update_modules (self, NULL);
	  prop = g_strdup_printf ("frequency_%u", n + 1);
	  g_object_notify (object, prop);
	  g_free (prop);
	  prop = g_strdup_printf ("note_%u", n + 1);
          g_object_notify (object, prop);
	  g_free (prop);
	  break;
	case PARAM_FREQ - PARAM_VALUE:
	  self->constants[n] = BSE_VALUE_FROM_FREQ (sfi_value_get_real (value));
          bse_constant_update_modules (self, NULL);
          prop = g_strdup_printf ("value_%u", n + 1);
	  g_object_notify (object, prop);
	  g_free (prop);
	  prop = g_strdup_printf ("note_%u", n + 1);
	  g_object_notify (object, prop);
	  g_free (prop);
	  break;
	case PARAM_NOTE - PARAM_VALUE:
	  note = sfi_value_get_note (value);
	  if (note != SFI_NOTE_VOID)
	    {
	      self->constants[n] = BSE_VALUE_FROM_FREQ (bse_note_to_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), note));
	      bse_constant_update_modules (self, NULL);
	      prop = g_strdup_printf ("value_%u", n + 1);
	      g_object_notify (object, prop);
	      g_free (prop);
	      prop = g_strdup_printf ("frequency_%u", n + 1);
	      g_object_notify (object, prop);
	      g_free (prop);
	    }
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
	  break;
	}
    }
}

static void
bse_constant_get_property (GObject     *object,
			   guint        param_id,
			   GValue      *value,
			   GParamSpec  *pspec)
{
  BseConstant *self = BSE_CONSTANT (object);
  
  switch (param_id)
    {
      guint indx, n;
    default:
      indx = (param_id - PARAM_VALUE) % 3;
      n = (param_id - PARAM_VALUE) / 3;
      switch (indx)
	{
        case PARAM_VALUE - PARAM_VALUE:
	  sfi_value_set_real (value, self->constants[n]);
	  break;
        case PARAM_FREQ - PARAM_VALUE:
	  sfi_value_set_real (value, BSE_FREQ_FROM_VALUE (self->constants[n]));
	  break;
        case PARAM_NOTE - PARAM_VALUE:
	  sfi_value_set_note (value, bse_note_from_freq (bse_item_current_musical_tuning (BSE_ITEM (self)), BSE_FREQ_FROM_VALUE (self->constants[n])));
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
	  break;
	}
    }
}

typedef struct
{
  gfloat constants[BSE_CONSTANT_N_OUTPUTS];
} ConstantModule;

static void
bse_constant_update_modules (BseConstant *constant,
			     BseTrans    *trans)
{
  if (BSE_SOURCE_PREPARED (constant))
    bse_source_update_modules (BSE_SOURCE (constant),
			       G_STRUCT_OFFSET (ConstantModule, constants),
			       constant->constants,
			       sizeof (constant->constants),
			       trans);
}

typedef struct {
  guint  index;
  guint  n_values;
  gfloat constants[BSE_CONSTANT_N_OUTPUTS];
} FlowAccessData;

static void
constant_process (BseModule *module,
		  guint      n_values)
{
  ConstantModule *cmod = module->user_data;
  guint i;
  
  for (i = 0; i < BSE_CONSTANT_N_OUTPUTS; i++)
    if (BSE_MODULE_OSTREAM (module, i).connected)
      BSE_MODULE_OSTREAM (module, i).values = bse_engine_const_values (cmod->constants[i]);
}

static void
bse_constant_context_create (BseSource *source,
			     guint      context_handle,
			     BseTrans  *trans)
{
  static const BseModuleClass constant_class = {
    0,				/* n_istreams */
    0,                          /* n_jstreams */
    BSE_CONSTANT_N_OUTPUTS,	/* n_ostreams */
    constant_process,		/* process */
    NULL,                       /* process_defer */
    NULL,                       /* reset */
    (BseModuleFreeFunc) g_free,	/* free */
    BSE_COST_CHEAP,		/* flags */
  };
  ConstantModule *constant = g_new0 (ConstantModule, 1);
  BseModule *module;
  
  module = bse_module_new (&constant_class, constant);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  bse_trans_add (trans, bse_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
  
  /* update (initialize) module data */
  bse_constant_update_modules (BSE_CONSTANT (source), trans);
}
