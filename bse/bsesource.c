/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include        "bsesource.h"

#include        "bsecontainer.h"
#include        "bsestorage.h"
#include        "bsemarshal.h"
#include        "gslengine.h"


enum {
  SIGNAL_IO_CHANGED,
  SIGNAL_LAST
};


/* --- prototypes --- */
static void         bse_source_class_base_init		(BseSourceClass	*class);
static void         bse_source_class_base_finalize	(BseSourceClass	*class);
static void         bse_source_class_init		(BseSourceClass	*class);
static void         bse_source_init			(BseSource	*source,
							 BseSourceClass	*class);
static void         bse_source_real_destroy		(BseObject	*object);
static void         bse_source_real_prepare		(BseSource	*source);
static void	    bse_source_real_context_create	(BseSource      *source,
							 guint           context_handle,
							 GslTrans       *trans);
static void	    bse_source_real_context_connect	(BseSource      *source,
							 guint           context_handle,
							 GslTrans       *trans);
static void	    bse_source_real_context_dismiss	(BseSource      *source,
							 guint           context_handle,
							 GslTrans       *trans);
static void         bse_source_real_reset		(BseSource	*source);
static void	    bse_source_real_add_input		(BseSource	*source,
							 guint     	 ichannel,
							 BseSource 	*osource,
							 guint     	 ochannel);
static void	    bse_source_real_remove_input	(BseSource	*source,
							 guint		 ichannel,
							 BseSource      *osource,
							 guint           ochannel);
static void	    bse_source_real_store_private	(BseObject	*object,
							 BseStorage	*storage);
static BseTokenType bse_source_real_restore_private	(BseObject      *object,
							 BseStorage     *storage);


/* --- variables --- */
static GTypeClass *parent_class = NULL;
static GQuark      quark_deferred_input = 0;
static guint       source_signals[SIGNAL_LAST] = { 0, };


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSource)
{
  static const GTypeInfo source_info = {
    sizeof (BseSourceClass),
    
    (GBaseInitFunc) bse_source_class_base_init,
    (GBaseFinalizeFunc) bse_source_class_base_finalize,
    (GClassInitFunc) bse_source_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSource),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_source_init,
  };

  g_assert (BSE_SOURCE_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT);
  
  return bse_type_register_static (BSE_TYPE_ITEM,
				   "BseSource",
				   "Base type for sound sources",
				   &source_info);
}

static void
bse_source_class_base_init (BseSourceClass *class)
{
  class->channel_defs.n_ichannels = 0;
  class->channel_defs.ichannel_names = NULL;
  class->channel_defs.ichannel_cnames = NULL;
  class->channel_defs.ichannel_blurbs = NULL;
  class->channel_defs.jchannel_flags = NULL;
  class->channel_defs.n_ochannels = 0;
  class->channel_defs.ochannel_names = NULL;
  class->channel_defs.ochannel_cnames = NULL;
  class->channel_defs.ochannel_blurbs = NULL;
}

static void
bse_source_class_base_finalize (BseSourceClass *class)
{
  guint i;
  
  for (i = 0; i < class->channel_defs.n_ichannels; i++)
    {
      g_free (class->channel_defs.ichannel_cnames[i]);
      g_free (class->channel_defs.ichannel_names[i]);
      g_free (class->channel_defs.ichannel_blurbs[i]);
    }
  g_free (class->channel_defs.ichannel_names);
  g_free (class->channel_defs.ichannel_cnames);
  g_free (class->channel_defs.ichannel_blurbs);
  g_free (class->channel_defs.jchannel_flags);
  class->channel_defs.n_ichannels = 0;
  class->channel_defs.ichannel_names = NULL;
  class->channel_defs.ichannel_cnames = NULL;
  class->channel_defs.ichannel_blurbs = NULL;
  class->channel_defs.jchannel_flags = NULL;
  for (i = 0; i < class->channel_defs.n_ochannels; i++)
    {
      g_free (class->channel_defs.ochannel_cnames[i]);
      g_free (class->channel_defs.ochannel_names[i]);
      g_free (class->channel_defs.ochannel_blurbs[i]);
    }
  g_free (class->channel_defs.ochannel_names);
  g_free (class->channel_defs.ochannel_cnames);
  g_free (class->channel_defs.ochannel_blurbs);
  class->channel_defs.n_ochannels = 0;
  class->channel_defs.ochannel_names = NULL;
  class->channel_defs.ochannel_cnames = NULL;
  class->channel_defs.ochannel_blurbs = NULL;
}

static void
bse_source_class_init (BseSourceClass *class)
{
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  object_class->store_private = bse_source_real_store_private;
  object_class->restore_private = bse_source_real_restore_private;
  object_class->destroy = bse_source_real_destroy;

  class->prepare = bse_source_real_prepare;
  class->context_create = bse_source_real_context_create;
  class->context_connect = bse_source_real_context_connect;
  class->context_dismiss = bse_source_real_context_dismiss;
  class->reset = bse_source_real_reset;
  class->add_input = bse_source_real_add_input;
  class->remove_input = bse_source_real_remove_input;

  source_signals[SIGNAL_IO_CHANGED] = bse_object_class_add_signal (object_class, "io_changed",
								   bse_marshal_VOID__NONE,
								   G_TYPE_NONE, 0);
}

static void
bse_source_init (BseSource      *source,
		 BseSourceClass *class)
{
  source->channel_defs = &BSE_SOURCE_CLASS (class)->channel_defs;
  source->inputs = g_new0 (BseSourceInput, BSE_SOURCE_N_ICHANNELS (source));
  source->outputs = NULL;
  source->n_contexts = 0;
  source->contexts = NULL;
}

static void
bse_source_real_destroy (BseObject *object)
{
  BseSource *source;
  guint i;

  source = BSE_SOURCE (object);

  if (bse_object_get_qdata (object, quark_deferred_input))
    g_warning (G_STRLOC ": source still contains deferred_input data");

  _bse_source_clear_ochannels (source);
  if (BSE_SOURCE_PREPARED (source))
    {
      g_warning (G_STRLOC ": source still prepared during destruction");
      bse_source_reset (source);
    }

  _bse_source_clear_ichannels (source);
  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, i))
      g_free (BSE_SOURCE_INPUT (source, i)->jdata.joints);
  g_free (source->inputs);
  source->inputs = NULL;

  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static gchar*
channel_dup_canonify (const gchar *name)
{
  gchar *cname = g_new (gchar, strlen (name) + 1);
  const gchar *s;
  gchar *c = cname;

  for (s = name; *s; s++)
    if ((*s >= '0' && *s <= '9') ||
	(*s >= 'a' && *s <= 'z'))
      *c++ = *s;
    else if (*s >= 'A' && *s <= 'Z')
      *c++ = *s - 'A' + 'a';
    else
      *c++ = '_';
  *c++ = 0;
  return cname;
}

static guint
bse_source_class_add_ijchannel (BseSourceClass *source_class,
				const gchar    *name,
				const gchar    *blurb,
				gboolean        is_joint_channel)
{
  BseSourceChannelDefs *defs;
  guint i;
  gchar *cname;

  g_return_val_if_fail (BSE_IS_SOURCE_CLASS (source_class), 0);
  g_return_val_if_fail (name != NULL, 0);
  if (!blurb)
    blurb = name;

  cname = channel_dup_canonify (name);
  i = 0; // FIXME: bse_source_find_ichannel (source, cname);
  if (i == ~0)
    {
      g_warning ("source class `%s' already has a channel \"%s\" with canonical name \"%s\"",
		 G_OBJECT_CLASS_NAME (source_class),
		 name, cname);
      g_free (cname);
      return ~0;
    }
  defs = &source_class->channel_defs;
  i = defs->n_ichannels++;
  defs->ichannel_names = g_renew (gchar*, defs->ichannel_names, defs->n_ichannels);
  defs->ichannel_cnames = g_renew (gchar*, defs->ichannel_cnames, defs->n_ichannels);
  defs->ichannel_blurbs = g_renew (gchar*, defs->ichannel_blurbs, defs->n_ichannels);
  defs->jchannel_flags = g_renew (guint8, defs->jchannel_flags, defs->n_ichannels);
  defs->ichannel_names[i] = g_strdup (name);
  defs->ichannel_cnames[i] = cname;
  defs->ichannel_blurbs[i] = g_strdup (blurb);
  defs->jchannel_flags[i] = is_joint_channel != FALSE;
  
  return i;
}

guint
bse_source_class_add_ichannel (BseSourceClass *source_class,
			       const gchar    *name,
			       const gchar    *blurb)
{
  return bse_source_class_add_ijchannel (source_class, name, blurb, FALSE);
}

guint
bse_source_class_add_jchannel (BseSourceClass *source_class,
			       const gchar    *name,
			       const gchar    *blurb)
{
  return bse_source_class_add_ijchannel (source_class, name, blurb, TRUE);
}

guint
bse_source_find_ichannel (BseSource   *source,
			  const gchar *ichannel_cname)
{
  guint i;

  g_return_val_if_fail (BSE_IS_SOURCE (source), ~0);
  g_return_val_if_fail (ichannel_cname != NULL, ~0);

  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    if (strcmp (BSE_SOURCE_ICHANNEL_CNAME (source, i), ichannel_cname) == 0)
      return i;
  return ~0;
}

guint
bse_source_class_add_ochannel (BseSourceClass *source_class,
			       const gchar    *name,
			       const gchar    *blurb)
{
  BseSourceChannelDefs *defs;
  guint i;
  gchar *cname;
  
  g_return_val_if_fail (BSE_IS_SOURCE_CLASS (source_class), 0);
  g_return_val_if_fail (name != NULL, 0);
  if (!blurb)
    blurb = name;
  
  cname = channel_dup_canonify (name);
  i = 0; // FIXME: bse_source_find_ochannel (source, cname);
  if (i == ~0)
    {
      g_warning ("source class `%s' already has a channel \"%s\" with canonical name \"%s\"",
		 G_OBJECT_CLASS_NAME (source_class),
		 name, cname);
      g_free (cname);
      return ~0;
    }
  defs = &source_class->channel_defs;
  i = defs->n_ochannels++;
  defs->ochannel_names = g_renew (gchar*, defs->ochannel_names, defs->n_ochannels);
  defs->ochannel_cnames = g_renew (gchar*, defs->ochannel_cnames, defs->n_ochannels);
  defs->ochannel_blurbs = g_renew (gchar*, defs->ochannel_blurbs, defs->n_ochannels);
  defs->ochannel_names[i] = g_strdup (name);
  defs->ochannel_cnames[i] = cname;
  defs->ochannel_blurbs[i] = g_strdup (blurb);
  
  return i;
}

guint
bse_source_find_ochannel (BseSource   *source,
			  const gchar *ochannel_cname)
{
  guint i;

  g_return_val_if_fail (BSE_IS_SOURCE (source), ~0);
  g_return_val_if_fail (ochannel_cname != NULL, ~0);

  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    if (strcmp (BSE_SOURCE_OCHANNEL_CNAME (source, i), ochannel_cname) == 0)
      return i;
  return ~0;
}

static void
bse_source_real_prepare (BseSource *source)
{
}

void
bse_source_prepare (BseSource *source)
{
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (!BSE_SOURCE_PREPARED (source));
  g_return_if_fail (source->n_contexts == 0);
  
  bse_object_ref (BSE_OBJECT (source));
  BSE_OBJECT_SET_FLAGS (source, BSE_SOURCE_FLAG_PREPARED);
  BSE_SOURCE_GET_CLASS (source)->prepare (source);
  bse_object_unref (BSE_OBJECT (source));
}

static void
bse_source_real_reset (BseSource *source)
{
}

void
bse_source_reset (BseSource *source)
{
  GslTrans *trans;

  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_PREPARED (source));
  
  bse_object_ref (BSE_OBJECT (source));
  trans = gsl_trans_open ();
  while (source->n_contexts)
    bse_source_dismiss_context (source, source->n_contexts - 1, trans);
  gsl_trans_commit (trans);
  gsl_engine_wait_on_trans ();
  BSE_OBJECT_UNSET_FLAGS (source, BSE_SOURCE_FLAG_PREPARED);
  BSE_SOURCE_GET_CLASS (source)->reset (source);
  bse_object_unref (BSE_OBJECT (source));
}

static void
bse_source_real_context_create	(BseSource *source,
				 guint      context_handle,
				 GslTrans  *trans)
{
}

guint
bse_source_create_context (BseSource *source,
			   GslTrans  *trans)
{
  BseSourceContext *context;
  guint i, c;

  g_return_val_if_fail (BSE_IS_SOURCE (source), ~0);
  g_return_val_if_fail (BSE_SOURCE_PREPARED (source), ~0);
  g_return_val_if_fail (trans != NULL, ~0);

  g_object_ref (source);
  c = source->n_contexts++;
  source->contexts = g_renew (BseSourceContext, source->contexts, source->n_contexts);
  context = source->contexts + c;
  context->ichannel_modules = g_new0 (GslModule*, BSE_SOURCE_N_ICHANNELS (source));
  context->module_istreams = g_new0 (guint, BSE_SOURCE_N_ICHANNELS (source));
  context->ochannel_modules = g_new0 (GslModule*, BSE_SOURCE_N_OCHANNELS (source));
  context->module_ostreams = g_new0 (guint, BSE_SOURCE_N_OCHANNELS (source));
  BSE_SOURCE_GET_CLASS (source)->context_create (source, c, trans);
  context = source->contexts + c;
  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    if (!context->ichannel_modules[i])
      g_warning (G_STRLOC ": source `%s' failed to create %s module for channel \"%s\" (%d)",
		 G_OBJECT_TYPE_NAME (source), "input", BSE_SOURCE_ICHANNEL_NAME (source, i), i);
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    if (!context->ochannel_modules[i])
      g_warning (G_STRLOC ": source `%s' failed to create %s module for channel \"%s\" (%d)",
		 G_OBJECT_TYPE_NAME (source), "output", BSE_SOURCE_OCHANNEL_NAME (source, i), i);
  g_object_unref (source);
  return c;
}

static void
bse_source_context_connect_ichannel (BseSource *source,
				     guint      ichannel,
				     guint      context_handle,
				     GslTrans  *trans)
{
  BseSourceContext *context = source->contexts + context_handle;
  BseSourceInput *input = BSE_SOURCE_INPUT (source, ichannel);

  /* keep this function in sync with bse_sub_synth_context_dismiss()
   * as it overrides our behaviour completely
   */

  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      g_message ("can't connect joint channels yet"); // FIXME: bse_sub_synth_context_dismiss()
    }
  else
    {
      if (input->idata.osource)
	{
	  guint module_ostream;
	  GslModule *omodule = bse_source_get_ochannel_module (input->idata.osource,
							       input->idata.ochannel,
							       context_handle,
							       &module_ostream);
	  
	  gsl_trans_add (trans,
			 gsl_job_connect (omodule, module_ostream,
					  context->ichannel_modules[ichannel],
					  context->module_istreams[ichannel]));
	}
    }
}

static void
bse_source_real_context_connect	(BseSource *source,
				 guint      context_handle,
				 GslTrans  *trans)
{
  guint i;

  /* keep this function in sync with bse_sub_synth_context_dismiss()
   * as it overrides our behaviour completely
   */

  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    bse_source_context_connect_ichannel (source, i, context_handle, trans);
}

void
bse_source_connect_context (BseSource *source,
			    guint      context_handle,
			    GslTrans  *trans)
{
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_PREPARED (source));
  g_return_if_fail (context_handle < source->n_contexts);
  g_return_if_fail (trans != NULL);

  g_object_ref (source);
  BSE_SOURCE_GET_CLASS (source)->context_connect (source, context_handle, trans);
  g_object_unref (source);
}

static void
bse_source_real_context_dismiss	(BseSource *source,
				 guint      context_handle,
				 GslTrans  *trans)
{
  BseSourceContext *context = source->contexts + context_handle;
  GSList *modules = NULL;
  guint i;

  /* keep this function in sync with bse_sub_synth_context_dismiss()
   * as it overrides our behaviour completely
   */

  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    {
      GslModule *module = context->ichannel_modules[i];

      if (!g_slist_find (modules, module))
	{
	  gsl_trans_add (trans, gsl_job_discard (module));
	  modules = g_slist_prepend (modules, module);
	}
      context->ichannel_modules[i] = NULL;
    }
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    {
      GslModule *module = context->ochannel_modules[i];

      if (!g_slist_find (modules, module))
	{
	  gsl_trans_add (trans, gsl_job_discard (module));
	  modules = g_slist_prepend (modules, module);
	}
      context->ochannel_modules[i] = NULL;
    }
  g_slist_free (modules);
}

void
bse_source_dismiss_context (BseSource *source,
			    guint      context_handle,
			    GslTrans  *trans)
{
  BseSourceContext *context;

  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_PREPARED (source));
  g_return_if_fail (context_handle < source->n_contexts);
  g_return_if_fail (trans != NULL);
  g_return_if_fail (context_handle == source->n_contexts - 1);	// FIXME
  
  g_object_ref (source);
  BSE_SOURCE_GET_CLASS (source)->context_dismiss (source, context_handle, trans);
  context = source->contexts + context_handle;
  g_free (context->ichannel_modules);
  g_free (context->module_istreams);
  g_free (context->ochannel_modules);
  g_free (context->module_ostreams);
  source->n_contexts -= 1;	// FIXME
  g_object_unref (source);
}

void
bse_source_set_context_imodule (BseSource *source,
				guint      ichannel,
				guint	   context_handle,
				GslModule *imodule,
				guint      istream)
{
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (ichannel < BSE_SOURCE_N_ICHANNELS (source));
  g_return_if_fail (context_handle < source->n_contexts);
  g_return_if_fail (imodule != NULL);
  g_return_if_fail (istream < GSL_MODULE_N_ISTREAMS (imodule));
  g_return_if_fail (source->contexts[context_handle].ichannel_modules[ichannel] == NULL);

  source->contexts[context_handle].ichannel_modules[ichannel] = imodule;
  source->contexts[context_handle].module_istreams[ichannel] = istream;
}

GslModule*
bse_source_get_ichannel_module (BseSource *source,
				guint      ichannel,
				guint      context_handle,
				guint     *module_istream_p)
{
  GslModule *imodule;

  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);
  g_return_val_if_fail (ichannel < BSE_SOURCE_N_ICHANNELS (source), NULL);
  g_return_val_if_fail (context_handle < source->n_contexts, NULL);

  imodule = source->contexts[context_handle].ichannel_modules[ichannel];
  if (module_istream_p)
    *module_istream_p = imodule ? source->contexts[context_handle].module_istreams[ichannel] : ~0;
  return imodule;
}

void
bse_source_set_context_omodule (BseSource *source,
				guint      ochannel,
				guint	   context_handle,
				GslModule *omodule,
				guint      ostream)
{
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (ochannel < BSE_SOURCE_N_OCHANNELS (source));
  g_return_if_fail (context_handle < source->n_contexts);
  g_return_if_fail (omodule != NULL);
  g_return_if_fail (ostream < GSL_MODULE_N_OSTREAMS (omodule));
  g_return_if_fail (source->contexts[context_handle].ochannel_modules[ochannel] == NULL);

  source->contexts[context_handle].ochannel_modules[ochannel] = omodule;
  source->contexts[context_handle].module_ostreams[ochannel] = ostream;
}

GslModule*
bse_source_get_ochannel_module (BseSource *source,
				guint      ochannel,
				guint      context_handle,
				guint     *module_ostream_p)
{
  GslModule *omodule;

  g_return_val_if_fail (BSE_IS_SOURCE (source), NULL);
  g_return_val_if_fail (ochannel < BSE_SOURCE_N_OCHANNELS (source), NULL);
  g_return_val_if_fail (context_handle < source->n_contexts, NULL);

  omodule = source->contexts[context_handle].ochannel_modules[ochannel];
  if (module_ostream_p)
    *module_ostream_p = omodule ? source->contexts[context_handle].module_ostreams[ochannel] : ~0;
  return omodule;
}

void
bse_source_set_context_module (BseSource *source,
			       guint      context_handle,
			       GslModule *module)
{
  guint i;

  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (context_handle < source->n_contexts);
  g_return_if_fail (module != NULL);
  g_return_if_fail (BSE_SOURCE_N_ICHANNELS (source) <= GSL_MODULE_N_ISTREAMS (module));
  g_return_if_fail (BSE_SOURCE_N_OCHANNELS (source) <= GSL_MODULE_N_OSTREAMS (module));

  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    bse_source_set_context_imodule (source, i, context_handle, module, i);
  for (i = 0; i < BSE_SOURCE_N_OCHANNELS (source); i++)
    bse_source_set_context_omodule (source, i, context_handle, module, i);
}

static void
source_access_modules (BseSource    *source,
		       guint	     channel,
		       gboolean	     is_ichannel,
		       GslAccessFunc access_func,
		       gpointer      data,
		       GslFreeFunc   data_free_func,
		       GslTrans     *trans)
{
  GSList *modules = NULL;
  guint c;
  
  for (c = 0; c < source->n_contexts; c++)
    {
      GslModule *module;
      
      if (is_ichannel)
	module = bse_source_get_ichannel_module (source, channel, c, NULL);
      else
	module = bse_source_get_ochannel_module (source, channel, c, NULL);
      
      if (module)
	modules = g_slist_prepend (modules, module);
    }
  if (modules)
    {
      GslTrans *my_trans = gsl_trans_open ();
      GSList *slist;
      
      if (!trans)
	trans = my_trans;
      for (slist = modules; slist; slist = slist->next)
	gsl_trans_add (trans, gsl_job_access (slist->data, access_func, data, slist->next ? NULL : data_free_func));
      gsl_trans_commit (my_trans);
      g_slist_free (modules);
    }
}

void
bse_source_access_imodules (BseSource    *source,
			    guint	  ichannel,
			    GslAccessFunc access_func,
			    gpointer      data,
			    GslFreeFunc   data_free_func,
			    GslTrans     *trans)
{
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_PREPARED (source));
  g_return_if_fail (ichannel < BSE_SOURCE_N_ICHANNELS (source));
  g_return_if_fail (access_func != NULL);

  source_access_modules (source, ichannel, TRUE, access_func, data, data_free_func, trans);
}

void
bse_source_access_omodules (BseSource    *source,
			    guint	  ochannel,
			    GslAccessFunc access_func,
			    gpointer      data,
			    GslFreeFunc   data_free_func,
			    GslTrans     *trans)
{
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_PREPARED (source));
  g_return_if_fail (ochannel < BSE_SOURCE_N_OCHANNELS (source));
  g_return_if_fail (access_func != NULL);

  source_access_modules (source, ochannel, FALSE, access_func, data, data_free_func, trans);
}

typedef struct {
  guint  member_offset;
  guint  member_size;
} AccessData;

static void
op_access_update (GslModule *module,
		  gpointer   data)
{
  AccessData *adata = data;
  guint8 *m = module->user_data;

  memcpy (m + adata->member_offset, adata + 1, adata->member_size);
}

void
bse_source_update_imodules (BseSource *source,
			    guint      ichannel,
			    guint      member_offset,
			    gpointer   member_p,
			    guint      member_size,
			    GslTrans  *trans)
{
  AccessData *adata;
  
  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_PREPARED (source));
  g_return_if_fail (ichannel < BSE_SOURCE_N_ICHANNELS (source));
  g_return_if_fail (member_p != NULL);
  g_return_if_fail (member_size > 0);

  adata = g_malloc (sizeof (AccessData) + member_size);
  adata->member_offset = member_offset;
  adata->member_size = member_size;
  memcpy (adata + 1, member_p, member_size);
  source_access_modules (source, ichannel, TRUE, op_access_update, adata, g_free, trans);
}

void
bse_source_update_omodules (BseSource *source,
			    guint      ochannel,
			    guint      member_offset,
			    gpointer   member_p,
			    guint      member_size,
			    GslTrans  *trans)
{
  AccessData *adata;

  g_return_if_fail (BSE_IS_SOURCE (source));
  g_return_if_fail (BSE_SOURCE_PREPARED (source));
  g_return_if_fail (ochannel < BSE_SOURCE_N_OCHANNELS (source));
  g_return_if_fail (member_p != NULL);
  g_return_if_fail (member_size > 0);

  adata = g_malloc (sizeof (AccessData) + member_size);
  adata->member_offset = member_offset;
  adata->member_size = member_size;
  memcpy (adata + 1, member_p, member_size);
  source_access_modules (source, ochannel, FALSE, op_access_update, adata, g_free, trans);
}

static void
bse_source_real_add_input (BseSource *source,
			   guint      ichannel,
			   BseSource *osource,
			   guint      ochannel)
{
  BseSourceInput *input = BSE_SOURCE_INPUT (source, ichannel);

  if (!BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    g_return_if_fail (input->idata.osource == NULL);

  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      guint j = input->jdata.n_joints++;

      input->jdata.joints = g_renew (BseSourceOutput, input->jdata.joints, input->jdata.n_joints);
      input->jdata.joints[j].osource = osource;
      input->jdata.joints[j].ochannel = ochannel;
      osource->outputs = g_slist_prepend (osource->outputs, source);

      if (source->n_contexts)   /* only if BSE_SOURCE_PREPARED() */
	{
	  GslTrans *trans = gsl_trans_open ();
	  guint c;

	  for (c = 0; c < source->n_contexts; c++)
	    bse_source_context_connect_ichannel (source, ichannel, c, trans);
	  gsl_trans_commit (trans);
	}
    }
  else
    {
      input->idata.osource = osource;
      input->idata.ochannel = ochannel;
      osource->outputs = g_slist_prepend (osource->outputs, source);
      
      if (source->n_contexts)	/* only if BSE_SOURCE_PREPARED() */
	{
	  GslTrans *trans = gsl_trans_open ();
	  guint c;
	  
	  for (c = 0; c < source->n_contexts; c++)
	    bse_source_context_connect_ichannel (source, ichannel, c, trans);
	  gsl_trans_commit (trans);
	}
    }
}

static gint
check_jchannel_connection (BseSource *source,
			   guint      ichannel,
			   BseSource *osource,
			   guint      ochannel)
{
  BseSourceInput *input = BSE_SOURCE_INPUT (source, ichannel);

  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      guint j;

      for (j = 0; j < input->jdata.n_joints; j++)
	if (input->jdata.joints[j].osource == osource &&
	    input->jdata.joints[j].ochannel == ochannel)
	  break;
      return j < input->jdata.n_joints ? j : -1;
    }
  else
    return ochannel == 0 && osource == input->idata.osource ? 0 : -1;
}

BseErrorType
_bse_source_set_input (BseSource *source,
		       guint      ichannel,
		       BseSource *osource,
		       guint      ochannel)
{
  g_return_val_if_fail (BSE_IS_SOURCE (source), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_IS_SOURCE (osource), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_ITEM (source)->parent == BSE_ITEM (osource)->parent, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (source->n_contexts == osource->n_contexts, BSE_ERROR_INTERNAL); /* paranoid, checked parent */

  if (ichannel >= BSE_SOURCE_N_ICHANNELS (source))
    return BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL;
  if (ochannel >= BSE_SOURCE_N_OCHANNELS (osource))
    return BSE_ERROR_SOURCE_NO_SUCH_OCHANNEL;
  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      if (check_jchannel_connection (source, ichannel, osource, ochannel) >= 0)
	return BSE_ERROR_SOURCE_CHANNELS_CONNECTED;
    }
  else if (BSE_SOURCE_INPUT (source, ichannel)->idata.osource)
    return BSE_ERROR_SOURCE_ICHANNEL_IN_USE;

  g_object_ref (source);
  g_object_ref (osource);
  BSE_SOURCE_GET_CLASS (source)->add_input (source, ichannel, osource, ochannel);
  g_signal_emit (source, source_signals[SIGNAL_IO_CHANGED], 0);
  g_signal_emit (osource, source_signals[SIGNAL_IO_CHANGED], 0);
  g_object_unref (source);
  g_object_unref (osource);
  
  return BSE_ERROR_NONE;
}

static void
bse_source_real_remove_input (BseSource *source,
			      guint      ichannel,
			      BseSource *osource,
			      guint      ochannel)
{
  BseSourceInput *input = BSE_SOURCE_INPUT (source, ichannel);
  GslTrans *trans = NULL;
  gint j = ~0;

  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      j = check_jchannel_connection (source, ichannel, osource, ochannel);
      g_return_if_fail (j >= 0);
    }
  else
    g_return_if_fail (osource == BSE_SOURCE_INPUT (source, ichannel)->idata.osource);

  if (source->n_contexts)	/* only if BSE_SOURCE_PREPARED() */
    {
      if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
	{
	  g_message ("can't disconnect jstreams yet "); // FIXME
	}
      else
	{
	  guint c;
	  
	  trans = gsl_trans_open ();
	  for (c = 0; c < source->n_contexts; c++)
	    {
	      guint istream;
	      GslModule *module = bse_source_get_ichannel_module (source, ichannel, c, &istream);
	      
	      gsl_trans_add (trans, gsl_job_disconnect (module, istream));
	    }
	}
    }

  if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, ichannel))
    {
      guint k = --input->jdata.n_joints;

      input->jdata.joints[j].osource = input->jdata.joints[k].osource;
      input->jdata.joints[j].ochannel = input->jdata.joints[k].ochannel;
    }
  else
    {
      input->idata.osource = NULL;
      input->idata.ochannel = 0;
    }
  osource->outputs = g_slist_remove (osource->outputs, source);

  if (trans)
    gsl_trans_commit (trans);
}

BseErrorType
_bse_source_unset_input (BseSource *source,
			 guint      ichannel,
			 BseSource *osource,
			 guint      ochannel)
{
  g_return_val_if_fail (BSE_IS_SOURCE (source), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_IS_SOURCE (osource), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (BSE_ITEM (source)->parent == BSE_ITEM (osource)->parent, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (source->n_contexts == osource->n_contexts, BSE_ERROR_INTERNAL); /* paranoid, checked parent */

  if (ichannel >= BSE_SOURCE_N_ICHANNELS (source))
    return BSE_ERROR_SOURCE_NO_SUCH_ICHANNEL;
  if (ochannel >= BSE_SOURCE_N_OCHANNELS (osource))
    return BSE_ERROR_SOURCE_NO_SUCH_OCHANNEL;
  if (check_jchannel_connection (source, ichannel, osource, ochannel) < 0)
    return BSE_ERROR_SOURCE_NO_SUCH_CONNECTION;

  g_object_ref (source);
  g_object_ref (osource);
  BSE_SOURCE_GET_CLASS (source)->remove_input (source, ichannel, osource, ochannel);
  g_signal_emit (source, source_signals[SIGNAL_IO_CHANGED], 0);
  g_signal_emit (osource, source_signals[SIGNAL_IO_CHANGED], 0);
  g_object_unref (osource);
  g_object_unref (source);

  return BSE_ERROR_NONE;
}

void
_bse_source_clear_ichannels (BseSource *source)
{
  gboolean io_changed = FALSE;
  guint i;

  g_return_if_fail (BSE_IS_SOURCE (source));

  g_object_ref (source);
  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    {
      BseSourceInput *input = BSE_SOURCE_INPUT (source, i);
      BseSource *osource;

      if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, i))
	{
	  guint ochannel;

	  while (input->jdata.n_joints)
	    {
	      osource = input->jdata.joints[0].osource;
	      ochannel = input->jdata.joints[0].ochannel;

	      io_changed = TRUE;
	      g_object_ref (osource);
	      BSE_SOURCE_GET_CLASS (source)->remove_input (source, i, osource, ochannel);
	      g_signal_emit (osource, source_signals[SIGNAL_IO_CHANGED], 0);
	      g_object_unref (osource);
	    }
	}
      else if (input->idata.osource)
	{
	  osource = input->idata.osource;

	  io_changed = TRUE;
	  g_object_ref (osource);
	  BSE_SOURCE_GET_CLASS (source)->remove_input (source, i, osource, input->idata.ochannel);
	  g_signal_emit (osource, source_signals[SIGNAL_IO_CHANGED], 0);
	  g_object_unref (osource);
	}
    }
  if (io_changed)
    g_signal_emit (source, source_signals[SIGNAL_IO_CHANGED], 0);
  g_object_unref (source);
}

void
_bse_source_clear_ochannels (BseSource *source)
{
  gboolean io_changed = FALSE;
  
  g_return_if_fail (BSE_IS_SOURCE (source));

  g_object_ref (source);
  while (source->outputs)
    {
      BseSource *isource = source->outputs->data;
      guint i;
      
      g_object_ref (isource);
      for (i = 0; i < BSE_SOURCE_N_ICHANNELS (isource); i++)
	{
	  BseSourceInput *input = BSE_SOURCE_INPUT (isource, i);

	  if (BSE_SOURCE_IS_JOINT_ICHANNEL (isource, i))
	    {
	      guint j;

	      for (j = 0; j < input->jdata.n_joints; j++)
		if (input->jdata.joints[j].osource == source)
		  break;
	      if (j < input->jdata.n_joints)
		{
		  io_changed = TRUE;
		  BSE_SOURCE_GET_CLASS (isource)->remove_input (isource, i,
								source, input->jdata.joints[j].ochannel);
		  g_signal_emit (isource, source_signals[SIGNAL_IO_CHANGED], 0);
		  break;
		}
	    }
	  else if (input->idata.osource == source)
	    {
	      io_changed = TRUE;
	      BSE_SOURCE_GET_CLASS (isource)->remove_input (isource, i,
							    source, input->idata.ochannel);
	      g_signal_emit (isource, source_signals[SIGNAL_IO_CHANGED], 0);
	      break;
	    }
	}
      g_object_unref (isource);
    }
  if (io_changed)
    g_signal_emit (source, source_signals[SIGNAL_IO_CHANGED], 0);
  g_object_unref (source);
}

static void
bse_source_real_store_private (BseObject  *object,
			       BseStorage *storage)
{
  BseSource *source = BSE_SOURCE (object);
  BseProject *project = bse_item_get_project (BSE_ITEM (source));
  guint i, j;

  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->store_private)
    BSE_OBJECT_CLASS (parent_class)->store_private (object, storage);
  
  for (i = 0; i < BSE_SOURCE_N_ICHANNELS (source); i++)
    {
      BseSourceInput *input = BSE_SOURCE_INPUT (source, i);
      GSList *slist, *outputs = NULL;
      
      if (BSE_SOURCE_IS_JOINT_ICHANNEL (source, i))
	for (j = 0; j < input->jdata.n_joints; j++)
	  outputs = g_slist_append (outputs, input->jdata.joints + j);
      else if (input->idata.osource)
	outputs = g_slist_append (outputs, &input->idata);
      
      for (slist = outputs; slist; slist = slist->next)
	{
	  BseSourceOutput *output = slist->data;
	  gchar *path = bse_container_make_item_path (BSE_CONTAINER (project),
						      BSE_ITEM (output->osource),
						      FALSE);
	  
	  bse_storage_break (storage);
	  
	  bse_storage_printf (storage,
			      "(source-input \"%s\" ",
			      BSE_SOURCE_ICHANNEL_CNAME (source, i));
	  bse_storage_push_level (storage);
	  bse_storage_printf (storage, "%s \"%s\"",
			      path,
			      BSE_SOURCE_OCHANNEL_CNAME (output->osource, output->ochannel));
	  bse_storage_pop_level (storage);
	  bse_storage_handle_break (storage);
	  bse_storage_putc (storage, ')');
	  g_free (path);
	}
      g_slist_free (outputs);
    }
}

typedef struct _DeferredInput DeferredInput;
struct _DeferredInput
{
  DeferredInput *next;
  guint          ichannel;
  gchar         *osource_path;
  gchar         *ochannel_name;
};

static void
deferred_input_free (gpointer data)
{
  do
    {
      DeferredInput *dinput = data;
      
      data = dinput->next;
      g_free (dinput->osource_path);
      g_free (dinput->ochannel_name);
      g_free (dinput);
    }
  while (data);
}

static void
resolve_dinput (BseSource  *source,
		BseStorage *storage,
		gboolean    aborted,
		BseProject *project)
{
  BseObject *object = BSE_OBJECT (source);
  DeferredInput *dinput = bse_object_get_qdata (object, quark_deferred_input);
  
  g_object_disconnect (BSE_OBJECT (project),
		       "any_signal", resolve_dinput, source,
		       NULL);
  
  for (; dinput; dinput = dinput->next)
    {
      BseItem *item;
      BseErrorType error;
      
      item = bse_container_item_from_path (BSE_CONTAINER (project), dinput->osource_path);
      if (!item || !BSE_IS_SOURCE (item))
	{
	  if (!aborted)
	    bse_storage_warn (storage,
			      "%s: unable to determine input source from \"%s\"",
			      BSE_OBJECT_NAME (source),
			      dinput->osource_path);
	  continue;
	}
      error = _bse_source_set_input (source,
				    dinput->ichannel,
				    BSE_SOURCE (item),
				    bse_source_find_ochannel (BSE_SOURCE (item),
							      dinput->ochannel_name));
      if (error && !aborted)
	{
	  bse_storage_warn (storage,
			    "failed to connect input \"%s\" of \"%s\" to output \"%s\" of \"%s\": %s",
			  dinput->ichannel < BSE_SOURCE_N_ICHANNELS (source) ? BSE_SOURCE_ICHANNEL_CNAME (source, dinput->ichannel) : "???",
			  BSE_OBJECT_NAME (source),
			  dinput->ochannel_name,
			  BSE_OBJECT_NAME (item),
			    bse_error_blurb (error));
	  item = bse_container_item_from_path (BSE_CONTAINER (project), dinput->osource_path);
	}
    }
  
  bse_object_set_qdata (object, quark_deferred_input, NULL);
}

static BseTokenType
bse_source_real_restore_private (BseObject  *object,
				 BseStorage *storage)
{
  BseSource *source = BSE_SOURCE (object);
  GScanner *scanner = storage->scanner;
  GTokenType expected_token = BSE_TOKEN_UNMATCHED;
  DeferredInput *dinput;
  BseProject *project;
  guint ichannel;
  gchar *osource_path;
  
  /* chain parent class' handler */
  if (BSE_OBJECT_CLASS (parent_class)->restore_private)
    expected_token = BSE_OBJECT_CLASS (parent_class)->restore_private (object, storage);
  
  /* feature source-input keywords */
  if (expected_token != BSE_TOKEN_UNMATCHED ||
      g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER ||
      !bse_string_equals ("source-input", scanner->next_value.v_identifier))
    return expected_token;
  
  g_scanner_get_next_token (scanner); /* eat "source-input" */
  project = bse_item_get_project (BSE_ITEM (source));
  
  /* parse ichannel name */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    return G_TOKEN_STRING;
  ichannel = bse_source_find_ichannel (source, scanner->value.v_string);
  
  /* parse osource path */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return G_TOKEN_IDENTIFIER;
  osource_path = g_strdup (scanner->value.v_identifier);
  
  /* parse ochannel name */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    {
      g_free (osource_path);
      
      return G_TOKEN_STRING;
    }
  
  /* ok, we got the link information, save it away into our list
   * and make sure we get notified from our project when to
   * complete restoration
   */
  if (!quark_deferred_input)
    quark_deferred_input = g_quark_from_static_string ("BseSourceDeferredInput");
  dinput = g_new (DeferredInput, 1);
  dinput->next = bse_object_steal_qdata (object, quark_deferred_input);
  dinput->ichannel = ichannel;
  dinput->osource_path = osource_path;
  dinput->ochannel_name = g_strdup (scanner->value.v_string);
  bse_object_set_qdata_full (object, quark_deferred_input, dinput, deferred_input_free);
  if (!dinput->next)
    g_object_connect (BSE_OBJECT (project),
		      "swapped_signal::complete_restore", resolve_dinput, source,
		      NULL);
  
  /* read closing brace */
  return g_scanner_get_next_token (scanner) == ')' ? G_TOKEN_NONE : ')';
}
