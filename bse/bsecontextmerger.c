/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
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
#include "bsecontextmerger.h"

#include "bsesnet.h"
#include "bseserver.h"
#include "gslengine.h"

#include <string.h>


/* --- prototypes --- */
static void	 bse_context_merger_init		(BseContextMerger	 *self);
static void	 bse_context_merger_class_init		(BseContextMergerClass	 *class);
static void	 bse_context_merger_context_create	(BseSource		 *source,
							 guint			  context_handle,
							 GslTrans		 *trans);
static void	 bse_context_merger_context_dismiss	(BseSource		 *source,
							 guint			  context_handle,
							 GslTrans		 *trans);


/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseContextMerger)
{
  static const GTypeInfo type_info = {
    sizeof (BseContextMergerClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_context_merger_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    sizeof (BseContextMerger),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_context_merger_init,
  };
  
  return bse_type_register_static (BSE_TYPE_SOURCE,
				   "BseContextMerger",
				   "Internal CONTEXT Voice glue object (merger)",
				   &type_info);
}

static void
bse_context_merger_class_init (BseContextMergerClass *class)
{
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint channel_id, i;
  
  parent_class = g_type_class_peek_parent (class);
  
  source_class->context_create = bse_context_merger_context_create;
  source_class->context_dismiss = bse_context_merger_context_dismiss;
  
  for (i = 0; i < BSE_CONTEXT_MERGER_N_IOPORTS; i++)
    {
      gchar *string;
      
      string = g_strdup_printf ("Input %u", i + 1);
      channel_id = bse_source_class_add_jchannel (source_class, string, NULL);
      g_assert (channel_id == i);
      g_free (string);
      
      string = g_strdup_printf ("Output %u", i + 1);
      channel_id = bse_source_class_add_ochannel (source_class, string, NULL);
      g_assert (channel_id == i);
      g_free (string);
    }
}

static void
bse_context_merger_init (BseContextMerger *self)
{
  self->merge_context = 0;
}

void
bse_context_merger_set_merge_context (BseContextMerger *self,
				      guint             merge_context)
{
  g_return_if_fail (BSE_CONTEXT_MERGER (self));
  
  if (merge_context)
    {
      g_return_if_fail (self->merge_context == 0);
      g_return_if_fail (bse_source_has_context (BSE_SOURCE (self), merge_context) == TRUE);
    }
  else
    g_return_if_fail (self->merge_context != 0);
  
  self->merge_context = merge_context;
}

typedef struct {
  guint real_context;
  guint ref_count;
} ContextModuleData;

static void
context_merger_process (GslModule *module,
			guint      n_values)
{
  guint i;
  
  for (i = 0; i < BSE_CONTEXT_MERGER_N_IOPORTS; i++)
    if (GSL_MODULE_OSTREAM (module, i).connected)
      {
	guint j, n_cons = GSL_MODULE_JSTREAM (module, i).n_connections;
	
	if (!n_cons)
	  module->ostreams[i].values = gsl_engine_const_values (0);
	else if (n_cons == 1)
	  module->ostreams[i].values = (gfloat*) GSL_MODULE_JBUFFER (module, i, 0);
	else
	  {
	    gfloat *sout = GSL_MODULE_OBUFFER (module, i), *bound = sout + n_values;
	    const gfloat *sin = GSL_MODULE_JBUFFER (module, i, 0);
	    memcpy (sout, sin, n_values * sizeof (sin[0]));
	    for (j = 1; j < n_cons; j++)
	      {
		gfloat *d = sout;
		sin = GSL_MODULE_JBUFFER (module, i, j);
		do
		  *d++ += *sin++;
		while (d < bound);
	      }
	  }
      }
}

static void
bse_context_merger_context_create (BseSource *source,
				   guint      context_handle,
				   GslTrans  *trans)
{
  static const GslClass context_merger_mclass = {
    0,                            /* n_istreams */
    BSE_CONTEXT_MERGER_N_IOPORTS, /* n_jstreams */
    BSE_CONTEXT_MERGER_N_IOPORTS, /* n_ostreams */
    context_merger_process,       /* process */
    NULL,                         /* process_defer */
    NULL,                         /* reset */
    (GslModuleFreeFunc) g_free,	  /* free */
    GSL_COST_CHEAP,               /* cost */
  };
  BseContextMerger *self = BSE_CONTEXT_MERGER (source);
  GslModule *module;
  
  /* merge with existing context if set */
  if (self->merge_context)
    {
      module = bse_source_get_context_imodule (source, self->merge_context);
      if (!module)
	g_warning ("context merger: request to merge context (%u) with non existing context (%u)",
		   context_handle, self->merge_context);
      else
	{
	  ContextModuleData *cmdata = module->user_data;
	  cmdata->ref_count++;
	}
    }
  else
    {
      ContextModuleData *cmdata = g_new (ContextModuleData, 1);
      cmdata->real_context = context_handle;
      cmdata->ref_count = 1;
      module = gsl_module_new (&context_merger_mclass, cmdata);
      /* commit module to engine */
      gsl_trans_add (trans, gsl_job_integrate (module));
    }
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_context_merger_context_dismiss (BseSource *source,
				    guint      context_handle,
				    GslTrans  *trans)
{
  GslModule *module;
  
  /* if the GslModule wasn't created within context_handle, we would
   * just need to disconnect it from connections within this context
   * and not discard it. however, that's somewhat tedious since it
   * requires poking around in BseSource internals which we can't do here.
   * context mergers are meant for internal static use only anyways,
   * so we can simply skip the disconnection, as usually all contexts
   * should be dismissed together.
   */
  
  module = bse_source_get_context_imodule (source, context_handle);
  if (module)
    {
      ContextModuleData *cmdata = module->user_data;
      g_return_if_fail (cmdata->ref_count > 0);
      cmdata->ref_count--;
      if (cmdata->ref_count)	/* prevent discarding from engine */
	{
	  bse_source_set_context_imodule (source, context_handle, NULL);
	  bse_source_set_context_omodule (source, context_handle, NULL);
	}
    }
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}
