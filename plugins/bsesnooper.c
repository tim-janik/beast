/* BseSnooper - BSE Snooper
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
#include "bsesnooper.h"

#include <bse/gslengine.h>


/* --- prototypes --- */
static void	 bse_snooper_init		(BseSnooper		*snooper);
static void	 bse_snooper_class_init		(BseSnooperClass	*class);
static void	 bse_snooper_context_create	(BseSource		*source,
						 guint			 context_handle,
						 GslTrans		*trans);


/* --- variables --- */
static GType		 type_id_snooper = 0;
static gpointer		 parent_class = NULL;
static const GTypeInfo type_info_snooper = {
  sizeof (BseSnooperClass),
  
  (GBaseInitFunc) NULL,
  (GBaseFinalizeFunc) NULL,
  (GClassInitFunc) bse_snooper_class_init,
  (GClassFinalizeFunc) NULL,
  NULL /* class_data */,
  
  sizeof (BseSnooper),
  0 /* n_preallocs */,
  (GInstanceInitFunc) bse_snooper_init,
};


/* --- functions --- */
static void
bse_snooper_class_init (BseSnooperClass *class)
{
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  guint ichannel;
  
  parent_class = g_type_class_peek_parent (class);
  
  source_class->context_create = bse_snooper_context_create;

  ichannel = bse_source_class_add_ichannel (source_class, "Mono In", "Snoop Signal");
  g_assert (ichannel == BSE_SNOOPER_ICHANNEL_MONO);
}

static void
bse_snooper_init (BseSnooper *snooper)
{
}

static void
snooper_process (GslModule *module,
		 guint      n_values)
{
  const BseSampleValue *wave_in = GSL_MODULE_IBUFFER (module, 0);

  if (module->istreams[0].connected)
    {
      gfloat min = wave_in[0], max = wave_in[0];
      gfloat avg = wave_in[0], first = wave_in[0], last = wave_in[n_values - 1];
      guint i;

      for (i = 1; i < n_values; i++)
	{
	  gfloat v = wave_in[i];

	  max = MAX (max, v);
	  min = MIN (min, v);
	  avg += v;
	  max = MAX (max, v);
	}
      avg /= (gdouble) n_values;
      g_print ("V: max=%+1.5f min=%+1.5f avg=%+1.5f freq=%+1.2f %u[%+1.5f,..,%+1.5f]\r",
	       max, min, avg, BSE_FREQ_FROM_VALUE (avg),
	       n_values,
	       first, last);
    }
}

static void
bse_snooper_context_create (BseSource *source,
			    guint      context_handle,
			    GslTrans  *trans)
{
  static const GslClass snooper_class = {
    1,			/* n_istreams */
    0,                  /* n_jstreams */
    0,			/* n_ostreams */
    snooper_process,	/* process */
    NULL,		/* free */
    GSL_COST_CHEAP,	/* flags */
  };
  GslModule *module;
  
  module = gsl_module_new (&snooper_class, NULL);
  
  /* setup module i/o streams with BseSource i/o channels */
  bse_source_set_context_module (source, context_handle, module);
  
  /* commit module to engine */
  gsl_trans_add (trans, gsl_job_integrate (module));
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
}


/* --- Export to BSE --- */
BSE_EXPORTS_BEGIN (BSE_PLUGIN_NAME);
BSE_EXPORT_OBJECTS = {
  { &type_id_snooper, "BseSnooper", "BseSource",
    "The Snooper module prints statistics about the incoming signal",
    &type_info_snooper,
    "/Source/Snooper",
  },
  { NULL, },
};
BSE_EXPORTS_END;
