/* BSE - Better Sound Engine
 * Copyright (C) 2002-2004 Tim Janik
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
#include "bsecontextmerger.h"
#include "bseblockutils.hh"
#include "bsesnet.h"
#include "bseserver.h"
#include "bseengine.h"

#include <string.h>


/* --- prototypes --- */
static void	 bse_context_merger_init		(BseContextMerger	 *self);
static void	 bse_context_merger_class_init		(BseContextMergerClass	 *klass);
static void	 bse_context_merger_context_create	(BseSource		 *source,
							 uint        		  context_handle,
							 BseTrans		 *trans);
static void	 bse_context_merger_context_dismiss	(BseSource		 *source,
							 uint        		  context_handle,
							 BseTrans		 *trans);


/* --- variables --- */
static void *parent_class = NULL;


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
                                   __FILE__, __LINE__,
                                   &type_info);
}

static void
bse_context_merger_class_init (BseContextMergerClass *klass)
{
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  uint channel_id, i;
  
  parent_class = g_type_class_peek_parent (klass);
  
  source_class->context_create = bse_context_merger_context_create;
  source_class->context_dismiss = bse_context_merger_context_dismiss;
  
  for (i = 0; i < BSE_CONTEXT_MERGER_N_IOPORTS; i++)
    {
      char *ident;
      
      ident = g_strdup_printf ("input-%u", i + 1);
      channel_id = bse_source_class_add_jchannel (source_class, ident, NULL, NULL);
      g_assert (channel_id == i);
      g_free (ident);
      
      ident = g_strdup_printf ("output-%u", i + 1);
      channel_id = bse_source_class_add_ochannel (source_class, ident, NULL, NULL);
      g_assert (channel_id == i);
      g_free (ident);
    }
}

static void
bse_context_merger_init (BseContextMerger *self)
{
  self->merge_context = 0;
}

void
bse_context_merger_set_merge_context (BseContextMerger *self,
				      uint              merge_context)
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
  uint real_context;
  uint ref_count;
} ContextModuleData;

static void
context_merger_process (BseModule    *module,
			uint          n_values)
{
  uint i;
  
  for (i = 0; i < BSE_CONTEXT_MERGER_N_IOPORTS; i++)
    if (BSE_MODULE_OSTREAM (module, i).connected)
      {
	uint j, n_cons = BSE_MODULE_JSTREAM (module, i).n_connections;
	
	if (!n_cons)
	  module->ostreams[i].values = bse_engine_const_values (0);
	else if (n_cons == 1)
	  module->ostreams[i].values = (float*) BSE_MODULE_JBUFFER (module, i, 0);
	else
	  {
	    float *sout = BSE_MODULE_OBUFFER (module, i);
	    const float *sin = BSE_MODULE_JBUFFER (module, i, 0);
            bse_block_copy_float (n_values, sout, sin);
	    for (j = 1; j < n_cons; j++)
	      {
		sin = BSE_MODULE_JBUFFER (module, i, j);
                bse_block_add_floats (n_values, sout, sin);
	      }
	  }
      }
}

static void
bse_context_merger_context_create (BseSource    *source,
				   uint          context_handle,
				   BseTrans     *trans)
{
  static const BseModuleClass context_merger_mclass = {
    0,                            /* n_istreams */
    BSE_CONTEXT_MERGER_N_IOPORTS, /* n_jstreams */
    BSE_CONTEXT_MERGER_N_IOPORTS, /* n_ostreams */
    context_merger_process,       /* process */
    NULL,                         /* process_defer */
    NULL,                         /* reset */
    (BseModuleFreeFunc) g_free,	  /* free */
    BSE_COST_CHEAP,               /* cost */
  };
  BseContextMerger *self = BSE_CONTEXT_MERGER (source);
  BseModule *module;
  
  /* merge with existing context if set */
  if (self->merge_context)
    {
      module = bse_source_get_context_imodule (source, self->merge_context);
      if (!module)
	g_warning ("context merger: request to merge context (%u) with non existing context (%u)",
		   context_handle, self->merge_context);
      else
	{
	  ContextModuleData *cmdata = (ContextModuleData*) module->user_data;
	  cmdata->ref_count++;
	}
    }
  else
    {
      ContextModuleData *cmdata = g_new (ContextModuleData, 1);
      cmdata->real_context = context_handle;
      cmdata->ref_count = 1;
      module = bse_module_new (&context_merger_mclass, cmdata);
      /* commit module to engine */
      bse_trans_add (trans, bse_job_integrate (module));
    }
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}

static void
bse_context_merger_context_dismiss (BseSource     *source,
				    uint           context_handle,
				    BseTrans      *trans)
{
  BseModule *module;
  
  /* if the BseModule wasn't created within context_handle, we would
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
      ContextModuleData *cmdata = (ContextModuleData*) module->user_data;
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
