/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
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
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sfi/gbsearcharray.h>
#include <gsl/gslengine.h>
#include <gsl/gslcommon.h>
#include "bsesnet.h"

#include "bseproject.h"
#include "bsecategories.h"
#include "bsestorage.h"
#include "bsemain.h"
#include "bsecontextmerger.h"
#include "bsemidireceiver.h"


typedef struct
{
  guint            context_id;
  BseMidiReceiver *midi_receiver;
  guint            midi_channel;
  guint		   n_branches;
  guint		  *branches;
  guint		   parent_context;
} ContextData;

/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_AUTO_ACTIVATE
};


/* --- prototypes --- */
static void      bse_snet_class_init             (BseSNetClass   *class);
static void      bse_snet_init                   (BseSNet        *snet);
static void      bse_snet_dispose                (GObject        *object);
static void      bse_snet_finalize               (GObject        *object);
static void      bse_snet_set_property           (GObject	 *object,
						  guint           param_id,
						  const GValue   *value,
						  GParamSpec     *pspec);
static void      bse_snet_get_property           (GObject        *object,
						  guint           param_id,
						  GValue         *value,
						  GParamSpec     *pspec);
static void      bse_snet_add_item               (BseContainer   *container,
						  BseItem        *item);
static void      bse_snet_forall_items           (BseContainer   *container,
						  BseForallItemsFunc func,
						  gpointer        data);
static void      bse_snet_remove_item            (BseContainer   *container,
						  BseItem        *item);
static void      bse_snet_release_children       (BseContainer   *container);
static void      bse_snet_prepare                (BseSource      *source);
static void      bse_snet_reset                  (BseSource      *source);
static gint	 snet_ports_compare              (gconstpointer   bsearch_node1, /* key */
						  gconstpointer   bsearch_node2);
static void      bse_snet_context_create	 (BseSource      *source,
						  guint           context_handle,
						  GslTrans       *trans);
static void      bse_snet_context_connect	 (BseSource      *source,
						  guint           context_handle,
						  GslTrans       *trans);
static void      bse_snet_context_dismiss	 (BseSource      *source,
						  guint           context_handle,
						  GslTrans       *trans);
static GSList*	 snet_context_children		 (BseContainer	 *container);


/* --- variables --- */
static GTypeClass          *parent_class = NULL;
static guint		    signal_port_unregistered = 0;
static const GBSearchConfig port_array_config = {
  sizeof (BseSNetPort),
  snet_ports_compare,
  0, /* G_BSEARCH_ARRAY_ALIGN_POWER2 */
};


/* --- functions --- */
BSE_BUILTIN_TYPE (BseSNet)
{
  GType   snet_type;
  
  static const GTypeInfo snet_info = {
    sizeof (BseSNetClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_snet_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseSNet),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_snet_init,
  };
  
  snet_type = bse_type_register_static (BSE_TYPE_SUPER,
                                        "BseSNet",
                                        "BSE Synthesis (Filter) Network",
                                        &snet_info);
  g_assert (BSE_SNET_FLAGS_USHIFT <= 32);
  
  return snet_type;
}

static void
bse_snet_class_init (BseSNetClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (class);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = bse_snet_set_property;
  gobject_class->get_property = bse_snet_get_property;
  gobject_class->dispose = bse_snet_dispose;
  gobject_class->finalize = bse_snet_finalize;
  
  source_class->prepare = bse_snet_prepare;
  source_class->context_create = bse_snet_context_create;
  source_class->context_connect = bse_snet_context_connect;
  source_class->context_dismiss = bse_snet_context_dismiss;
  source_class->reset = bse_snet_reset;
  
  container_class->add_item = bse_snet_add_item;
  container_class->remove_item = bse_snet_remove_item;
  container_class->forall_items = bse_snet_forall_items;
  container_class->context_children = snet_context_children;
  container_class->release_children = bse_snet_release_children;
  
  bse_object_class_add_param (object_class, "Playback Settings",
			      PARAM_AUTO_ACTIVATE,
			      sfi_pspec_bool ("auto_activate", "Auto Activate",
					      "Automatic activation only needs to be enabled for synthesis networks "
					      "that don't use virtual ports for their input and output",
					      FALSE, SFI_PARAM_DEFAULT));
  signal_port_unregistered = bse_object_class_add_signal (object_class, "port_unregistered",
							  G_TYPE_NONE, 0);
}

static void
bse_snet_init (BseSNet *snet)
{
  BSE_OBJECT_SET_FLAGS (snet, BSE_SNET_FLAG_USER_SYNTH);
  snet->sources = NULL;
  snet->iport_names = NULL;
  snet->oport_names = NULL;
  snet->port_array = NULL;
  snet->port_unregistered_id = 0;
  snet->cid_counter = 0;
  snet->n_cids = 0;
  snet->cids = NULL;
}

static void
bse_snet_release_children (BseContainer *container)
{
  BseSNet *snet = BSE_SNET (container);
  GList *list;

  list = snet->sources;
  while (list)
    {
      GList *next = list->next;
      if (!BSE_ITEM_AGGREGATE (list->data))
	bse_container_remove_item (container, list->data);
      list = next;
    }
  if (snet->iport_names)
    g_warning ("%s: leaking %cport \"%s\"", G_STRLOC, 'i', (gchar*) snet->iport_names->data);
  if (snet->oport_names)
    g_warning ("%s: leaking %cport \"%s\"", G_STRLOC, 'o', (gchar*) snet->oport_names->data);
  
  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->release_children (container);
}

static void
bse_snet_dispose (GObject *object)
{
  // BseSNet *snet = BSE_SNET (object);
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_snet_finalize (GObject *object)
{
  BseSNet *snet = BSE_SNET (object);
  
  if (snet->port_unregistered_id)
    {
      bse_idle_remove (snet->port_unregistered_id);
      snet->port_unregistered_id = 0;
    }
  
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
  
  g_return_if_fail (snet->port_unregistered_id == 0);
}

static gboolean
snet_notify_port_unregistered (gpointer data)
{
  BseSNet *snet = BSE_SNET (data);
  
  BSE_THREADS_ENTER ();
  snet->port_unregistered_id = 0;
  g_signal_emit (snet, signal_port_unregistered, 0);
  BSE_THREADS_LEAVE ();
  
  return FALSE;
}

static void
bse_snet_queue_port_unregistered (BseSNet *snet)
{
  if (!snet->port_unregistered_id)
    snet->port_unregistered_id = bse_idle_notify (snet_notify_port_unregistered, snet);
}

static void
bse_snet_set_property (GObject      *object,
		       guint         param_id,
		       const GValue *value,
		       GParamSpec   *pspec)
{
  BseSNet *self = BSE_SNET (object);
  
  switch (param_id)
    {
    case PARAM_AUTO_ACTIVATE:
      if (sfi_value_get_bool (value))
	BSE_OBJECT_SET_FLAGS (self, BSE_SUPER_FLAG_NEEDS_SEQUENCER_CONTEXT);
      else
	BSE_OBJECT_UNSET_FLAGS (self, BSE_SUPER_FLAG_NEEDS_SEQUENCER_CONTEXT);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_snet_get_property (GObject    *object,
		       guint       param_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
  BseSNet *self = BSE_SNET (object);
  
  switch (param_id)
    {
    case PARAM_AUTO_ACTIVATE:
      sfi_value_set_bool (value, BSE_SUPER_NEEDS_SEQUENCER_CONTEXT (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
bse_snet_add_item (BseContainer *container,
                   BseItem      *item)
{
  BseSNet *snet = BSE_SNET (container);
  
  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOURCE))
    snet->sources = g_list_append (snet->sources, item);
  else if (BSE_SNET_USER_SYNTH (snet))
    g_warning ("BseSNet: cannot hold non-source item type `%s'",
               BSE_OBJECT_TYPE_NAME (item));
  
  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_snet_forall_items (BseContainer      *container,
                       BseForallItemsFunc func,
                       gpointer           data)
{
  BseSNet *snet = BSE_SNET (container);
  GList *list;
  
  list = snet->sources;
  while (list)
    {
      BseItem *item;
      
      item = list->data;
      list = list->next;
      if (!func (item, data))
        return;
    }
}

static void
bse_snet_remove_item (BseContainer *container,
                      BseItem      *item)
{
  BseSNet *snet;
  
  snet = BSE_SNET (container);
  
  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOURCE))
    snet->sources = g_list_remove (snet->sources, item);
  else if (BSE_SNET_USER_SYNTH (snet))
    g_warning ("BseSNet: cannot hold non-source item type `%s'",
               BSE_OBJECT_TYPE_NAME (item));
  
  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}

static GSList*
snet_find_port_name (BseSNet     *snet,
		     const gchar *name,
		     gboolean     in_port)
{
  GSList *slist;
  
  for (slist = in_port ? snet->iport_names : snet->oport_names; slist; slist = slist->next)
    if (strcmp (name, slist->data) == 0)
      return slist;
  return NULL;
}

const gchar*
bse_snet_iport_name_register (BseSNet     *snet,
			      const gchar *tmpl_name)
{
  GSList *slist;
  gchar *name;
  guint i;
  
  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  g_return_val_if_fail (tmpl_name != NULL, NULL);
  
  slist = snet_find_port_name (snet, tmpl_name, TRUE);
  name = NULL;
  i = 1;
  while (slist)
    {
      g_free (name);
      name = g_strdup_printf ("%s-%u", tmpl_name, i++);
      slist = snet_find_port_name (snet, name, TRUE);
    }
  if (!name)
    name = g_strdup (tmpl_name);
  snet->iport_names = g_slist_prepend (snet->iport_names, name);
  
  return name;
}

gboolean
bse_snet_iport_name_registered (BseSNet     *snet,
				const gchar *name)
{
  GSList *slist;
  
  g_return_val_if_fail (BSE_IS_SNET (snet), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);
  
  slist = snet_find_port_name (snet, name, TRUE);
  
  return slist != NULL;
}

void
bse_snet_iport_name_unregister (BseSNet     *snet,
				const gchar *name)
{
  GSList *slist;
  
  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (name != NULL);
  
  slist = snet_find_port_name (snet, name, TRUE);
  g_return_if_fail (slist != NULL);
  
  g_free (slist->data);
  snet->iport_names = g_slist_delete_link (snet->iport_names, slist);
  bse_snet_queue_port_unregistered (snet);
}

const gchar*
bse_snet_oport_name_register (BseSNet     *snet,
			      const gchar *tmpl_name)
{
  GSList *slist;
  gchar *name;
  guint i;
  
  g_return_val_if_fail (BSE_IS_SNET (snet), NULL);
  g_return_val_if_fail (tmpl_name != NULL, NULL);
  
  slist = snet_find_port_name (snet, tmpl_name, FALSE);
  name = NULL;
  i = 1;
  while (slist)
    {
      g_free (name);
      name = g_strdup_printf ("%s-%u", tmpl_name, i++);
      slist = snet_find_port_name (snet, name, FALSE);
    }
  if (!name)
    name = g_strdup (tmpl_name);
  snet->oport_names = g_slist_prepend (snet->oport_names, name);
  
  return name;
}

gboolean
bse_snet_oport_name_registered (BseSNet     *snet,
				const gchar *name)
{
  GSList *slist;
  
  g_return_val_if_fail (BSE_IS_SNET (snet), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);
  
  slist = snet_find_port_name (snet, name, FALSE);
  
  return slist != NULL;
}

void
bse_snet_oport_name_unregister (BseSNet     *snet,
				const gchar *name)
{
  GSList *slist;
  
  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (name != NULL);
  
  slist = snet_find_port_name (snet, name, FALSE);
  g_return_if_fail (slist != NULL);
  
  g_free (slist->data);
  snet->oport_names = g_slist_delete_link (snet->oport_names, slist);
  bse_snet_queue_port_unregistered (snet);
}

static gint
snet_ports_compare (gconstpointer bsearch_node1, /* key */
		    gconstpointer bsearch_node2)
{
  const BseSNetPort *p1 = bsearch_node1;
  const BseSNetPort *p2 = bsearch_node2;
  gint cmp;
  
  cmp = G_BSEARCH_ARRAY_CMP (p1->context, p2->context);
  if (!cmp)
    cmp = G_BSEARCH_ARRAY_CMP (p1->input, p2->input);
  if (!cmp)
    cmp = strcmp (p1->name, p2->name);
  
  return cmp;
}

static BseSNetPort*
port_lookup (BseSNet     *snet,
	     const gchar *name,
	     guint        snet_context,
	     gboolean	  is_input)
{
  BseSNetPort key;
  
  key.name = (gchar*) name;
  key.context = snet_context;
  key.input = is_input != FALSE;
  return g_bsearch_array_lookup (snet->port_array, &port_array_config, &key);
}

static BseSNetPort*
port_insert (BseSNet     *snet,
	     const gchar *name,
	     guint        snet_context,
	     gboolean     is_input)
{
  BseSNetPort key = { NULL, }, *port;
  
  key.name = (gchar*) name;
  key.context = snet_context;
  key.input = is_input != FALSE;
  
  port = g_bsearch_array_lookup (snet->port_array, &port_array_config, &key);
  g_return_val_if_fail (port == NULL, port);	/* shouldn't fail */
  
  key.name = g_strdup (key.name);
  key.src_omodule = NULL;
  key.src_ostream = G_MAXUINT;
  key.dest_imodule = NULL;
  key.dest_istream = G_MAXUINT;
  snet->port_array = g_bsearch_array_insert (snet->port_array, &port_array_config, &key);
  return g_bsearch_array_lookup (snet->port_array, &port_array_config, &key);
}

static void
port_delete (BseSNet     *snet,
	     BseSNetPort *port)
{
  guint index = g_bsearch_array_get_index (snet->port_array, &port_array_config, port);
  
  g_return_if_fail (index < g_bsearch_array_get_n_nodes (snet->port_array));
  g_return_if_fail (port->src_omodule == NULL && port->dest_imodule == NULL);
  
  g_free (port->name);
  g_bsearch_array_remove (snet->port_array, &port_array_config, index);
}

void
bse_snet_set_iport_src (BseSNet     *snet,
			const gchar *name,
			guint        snet_context,
			GslModule   *omodule,
			guint        ostream,
			GslTrans    *trans)
{
  BseSNetPort *port;
  
  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (name != NULL);
  g_return_if_fail (bse_source_has_context (BSE_SOURCE (snet), snet_context));
  if (omodule)
    g_return_if_fail (ostream < GSL_MODULE_N_OSTREAMS (omodule));
  g_return_if_fail (trans != NULL);
  
  port = port_lookup (snet, name, snet_context, TRUE);
  if (!port && !omodule)
    return;
  else if (!port)
    port = port_insert (snet, name, snet_context, TRUE);
  else if (!omodule)
    ostream = G_MAXUINT;
  
  if (port->src_omodule && port->dest_imodule)
    gsl_trans_add (trans, gsl_job_disconnect (port->dest_imodule, port->dest_istream));
  port->src_omodule = omodule;
  port->src_ostream = ostream;
  if (port->src_omodule && port->dest_imodule)
    gsl_trans_add (trans, gsl_job_connect (port->src_omodule, port->src_ostream,
					   port->dest_imodule, port->dest_istream));
  if (!port->dest_imodule && !port->src_omodule)
    port_delete (snet, port);
}

void
bse_snet_set_iport_dest (BseSNet     *snet,
			 const gchar *name,
			 guint        snet_context,
			 GslModule   *imodule,
			 guint        istream,
			 GslTrans    *trans)
{
  BseSNetPort *port;
  
  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (name != NULL);
  g_return_if_fail (bse_source_has_context (BSE_SOURCE (snet), snet_context));
  if (imodule)
    g_return_if_fail (istream < GSL_MODULE_N_ISTREAMS (imodule));
  g_return_if_fail (trans != NULL);
  
  port = port_lookup (snet, name, snet_context, TRUE);
  if (!port && !imodule)
    return;
  else if (!port)
    port = port_insert (snet, name, snet_context, TRUE);
  else if (!imodule)
    istream = G_MAXUINT;
  
  if (port->src_omodule && port->dest_imodule)
    gsl_trans_add (trans, gsl_job_disconnect (port->dest_imodule, port->dest_istream));
  port->dest_imodule = imodule;
  port->dest_istream = istream;
  if (port->src_omodule && port->dest_imodule)
    gsl_trans_add (trans, gsl_job_connect (port->src_omodule, port->src_ostream,
					   port->dest_imodule, port->dest_istream));
  if (!port->dest_imodule && !port->src_omodule)
    port_delete (snet, port);
}

void
bse_snet_set_oport_src (BseSNet     *snet,
			const gchar *name,
			guint        snet_context,
			GslModule   *omodule,
			guint        ostream,
			GslTrans    *trans)
{
  BseSNetPort *port;
  
  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (name != NULL);
  g_return_if_fail (bse_source_has_context (BSE_SOURCE (snet), snet_context));
  if (omodule)
    g_return_if_fail (ostream < GSL_MODULE_N_OSTREAMS (omodule));
  g_return_if_fail (trans != NULL);
  
  port = port_lookup (snet, name, snet_context, FALSE);
  if (!port && !omodule)
    return;
  else if (!port)
    port = port_insert (snet, name, snet_context, FALSE);
  else if (!omodule)
    ostream = G_MAXUINT;
  
  if (port->src_omodule && port->dest_imodule)
    gsl_trans_add (trans, gsl_job_disconnect (port->dest_imodule, port->dest_istream));
  port->src_omodule = omodule;
  port->src_ostream = ostream;
  if (port->src_omodule && port->dest_imodule)
    gsl_trans_add (trans, gsl_job_connect (port->src_omodule, port->src_ostream,
					   port->dest_imodule, port->dest_istream));
  if (!port->dest_imodule && !port->src_omodule)
    port_delete (snet, port);
}

void
bse_snet_set_oport_dest (BseSNet     *snet,
			 const gchar *name,
			 guint        snet_context,
			 GslModule   *imodule,
			 guint        istream,
			 GslTrans    *trans)
{
  BseSNetPort *port;
  
  g_return_if_fail (BSE_IS_SNET (snet));
  g_return_if_fail (name != NULL);
  g_return_if_fail (bse_source_has_context (BSE_SOURCE (snet), snet_context));
  if (imodule)
    g_return_if_fail (istream < GSL_MODULE_N_ISTREAMS (imodule));
  g_return_if_fail (trans != NULL);
  
  port = port_lookup (snet, name, snet_context, FALSE);
  if (!port && !imodule)
    return;
  else if (!port)
    port = port_insert (snet, name, snet_context, FALSE);
  else if (!imodule)
    istream = G_MAXUINT;
  
  if (port->src_omodule && port->dest_imodule)
    gsl_trans_add (trans, gsl_job_disconnect (port->dest_imodule, port->dest_istream));
  port->dest_imodule = imodule;
  port->dest_istream = istream;
  if (port->src_omodule && port->dest_imodule)
    gsl_trans_add (trans, gsl_job_connect (port->src_omodule, port->src_ostream,
					   port->dest_imodule, port->dest_istream));
  if (!port->dest_imodule && !port->src_omodule)
    port_delete (snet, port);
}

static void
bse_snet_free_cid (BseSNet *snet,
		   guint    cid)
{
  guint i;
  
  g_return_if_fail (BSE_SOURCE_PREPARED (snet));
  
  i = snet->n_cids++;
  snet->cids = g_renew (guint, snet->cids, snet->n_cids);
  snet->cids[i] = cid;
}

static guint
bse_snet_alloc_cid (BseSNet *snet)
{
  guint i, cid;
  
  g_return_val_if_fail (BSE_SOURCE_PREPARED (snet), 0);
  
  while (snet->n_cids < 2)
    bse_snet_free_cid (snet, ++snet->cid_counter);
  
  i = rand () % snet->n_cids;
  cid = snet->cids[i];
  snet->n_cids--;
  snet->cids[i] = snet->cids[snet->n_cids];
  return cid;
}

static inline ContextData*
find_context_data (BseSNet *self,
		   guint    context_id)
{
  gpointer data = bse_source_get_context_data (BSE_SOURCE (self), context_id);
  return data;
}

static ContextData*
create_context_data (BseSNet         *self,
		     guint            context_id,
		     guint            parent_context,
		     BseMidiReceiver *midi_receiver,
		     guint            midi_channel)
{
  ContextData *cdata = g_new0 (ContextData, 1);
  
  cdata->context_id = context_id;
  cdata->midi_receiver = bse_midi_receiver_ref (midi_receiver);
  cdata->midi_channel = midi_channel;
  cdata->n_branches = 0;
  cdata->branches = NULL;
  if (parent_context)
    {
      ContextData *pdata = find_context_data (self, parent_context);
      guint i;
      
      i = pdata->n_branches++;
      pdata->branches = g_renew (guint, pdata->branches, pdata->n_branches);
      pdata->branches[i] = context_id;
      cdata->parent_context = parent_context;
    }
  else
    cdata->parent_context = 0;
  
  return cdata;
}

static void
free_context_data (BseSource *source,
		   gpointer   data,
		   GslTrans  *trans)
{
  BseSNet *self = BSE_SNET (source);
  ContextData *cdata = data;
  
  g_return_if_fail (cdata->n_branches == 0);
  
  bse_midi_receiver_unref (cdata->midi_receiver);
  bse_snet_free_cid (self, cdata->context_id);
  if (cdata->parent_context)
    {
      ContextData *pdata = find_context_data (self, cdata->parent_context);
      guint i, swap_context;
      
      g_return_if_fail (pdata->n_branches > 0);
      
      pdata->n_branches--;
      swap_context = pdata->branches[pdata->n_branches];
      for (i = 0; i < pdata->n_branches; i++)
	if (pdata->branches[i] == cdata->context_id)
	  {
	    pdata->branches[i] = swap_context;
	    break;
	  }
    }
  g_free (cdata->branches);
  g_free (cdata);
}

guint
bse_snet_create_context (BseSNet         *self,
			 BseMidiReceiver *midi_receiver,
			 guint		  midi_channel,
			 GslTrans        *trans)
{
  ContextData *cdata;
  guint cid;
  
  g_return_val_if_fail (BSE_IS_SNET (self), 0);
  g_return_val_if_fail (BSE_SOURCE_PREPARED (self), 0);
  g_return_val_if_fail (midi_receiver != NULL, 0);
  g_return_val_if_fail (trans != NULL, 0);
  
  cid = bse_snet_alloc_cid (self);
  g_return_val_if_fail (cid > 0, 0);
  g_return_val_if_fail (bse_source_has_context (BSE_SOURCE (self), cid) == FALSE, 0);
  
  cdata = create_context_data (self, cid, 0, midi_receiver, midi_channel);
  bse_source_create_context_with_data (BSE_SOURCE (self), cid, cdata, free_context_data, trans);
  
  return cid;
}

guint
bse_snet_context_clone_branch (BseSNet         *self,
			       guint            context,
			       BseSource       *context_merger,
			       BseMidiReceiver *midi_receiver,
			       guint            midi_channel,
			       GslTrans        *trans)
{
  SfiRing *ring;
  guint bcid = 0;
  
  g_return_val_if_fail (BSE_IS_SNET (self), 0);
  g_return_val_if_fail (BSE_SOURCE_PREPARED (self), 0);
  g_return_val_if_fail (bse_source_has_context (BSE_SOURCE (self), context), 0);
  g_return_val_if_fail (BSE_IS_CONTEXT_MERGER (context_merger), 0);
  g_return_val_if_fail (bse_source_has_context (context_merger, context), 0);
  g_return_val_if_fail (BSE_ITEM (context_merger)->parent == BSE_ITEM (self), 0);
  g_return_val_if_fail (midi_receiver != NULL, 0);
  g_return_val_if_fail (trans != NULL, 0);
  
  ring = bse_source_collect_inputs_recursive (context_merger);
  if (!BSE_SOURCE_COLLECTED (context_merger))
    {
      ContextData *cdata;
      SfiRing *node;
      
      g_assert (self->tmp_context_children == NULL);
      for (node = ring; node; node = sfi_ring_walk (node, ring))
	self->tmp_context_children = g_slist_prepend (self->tmp_context_children, node->data);
      self->tmp_context_children = g_slist_prepend (self->tmp_context_children, context_merger);
      bse_source_free_collection (ring);
      bcid = bse_snet_alloc_cid (self);
      cdata = create_context_data (self, bcid, context, midi_receiver, midi_channel);
      bse_source_create_context_with_data (BSE_SOURCE (self), bcid, cdata, free_context_data, trans);
      g_assert (self->tmp_context_children == NULL);
    }
  else
    {
      g_warning ("%s: context merger forms a cycle with it's inputs", G_STRLOC);
      bse_source_free_collection (ring);
    }
  
  
  return bcid;
}

gboolean
bse_snet_context_is_branch (BseSNet *self,
			    guint    context_id)
{
  ContextData *cdata;
  
  g_return_val_if_fail (BSE_IS_SNET (self), FALSE);
  g_return_val_if_fail (BSE_SOURCE_PREPARED (self), FALSE);
  g_return_val_if_fail (context_id > 0, FALSE);
  
  cdata = find_context_data (self, context_id);
  return cdata ? cdata->parent_context > 0 : FALSE;
}

static GSList*
snet_context_children (BseContainer *container)
{
  BseSNet *self = BSE_SNET (container);
  GSList *slist;
  
  if (self->tmp_context_children)
    {
      slist = self->tmp_context_children;
      self->tmp_context_children = NULL;
    }
  else
    slist = BSE_CONTAINER_CLASS (parent_class)->context_children (container);
  
  return slist;
}

BseMidiReceiver*
bse_snet_get_midi_receiver (BseSNet *self,
			    guint    context_handle,
			    guint   *midi_channel)
{
  ContextData *cdata;
  
  g_return_val_if_fail (BSE_IS_SNET (self), 0);
  
  cdata = find_context_data (self, context_handle);
  if (midi_channel)
    *midi_channel = cdata ? cdata->midi_channel : 0;
  return cdata ? cdata->midi_receiver : NULL;
}

static void
bse_snet_prepare (BseSource *source)
{
  BseSNet *snet = BSE_SNET (source);
  
  g_return_if_fail (snet->port_array == NULL);
  
  bse_object_lock (BSE_OBJECT (snet));
  snet->port_array = g_bsearch_array_create (&port_array_config);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
bse_snet_reset (BseSource *source)
{
  BseSNet *self = BSE_SNET (source);
  guint i;
  
  g_return_if_fail (self->port_array != NULL);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);
  
  if (g_bsearch_array_get_n_nodes (self->port_array))
    {
      BseSNetPort *port = g_bsearch_array_get_nth (self->port_array, &port_array_config, 0);
      
      g_warning ("%s: %cport \"%s\" still active: context=%u src=%p dest=%p",
		 G_STRLOC, port->input ? 'i' : 'o', port->name,
		 port->context, port->src_omodule, port->dest_imodule);
    }
  g_bsearch_array_free (self->port_array, &port_array_config);
  self->port_array = NULL;
  
  i = self->cid_counter - self->n_cids;
  if (i)
    g_warning ("%s: %u context IDs still in use", G_STRLOC, i);
  g_free (self->cids);
  self->cid_counter = 0;
  self->n_cids = 0;
  self->cids = NULL;
  
  bse_object_unlock (BSE_OBJECT (self));
}

static void
bse_snet_context_create (BseSource *source,
			 guint      context_handle,
			 GslTrans  *trans)
{
  BseSNet *self = BSE_SNET (source);
  
  if (self->tmp_context_children)
    {
      BseContextMerger *context_merger = self->tmp_context_children->data;
      ContextData *cdata = find_context_data (self, context_handle);
      
      g_assert (BSE_IS_CONTEXT_MERGER (context_merger));
      
      bse_context_merger_set_merge_context (context_merger, cdata->parent_context);
      /* chain parent class' handler */
      BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
      bse_context_merger_set_merge_context (context_merger, 0);
    }
  else
    {
      /* chain parent class' handler */
      BSE_SOURCE_CLASS (parent_class)->context_create (source, context_handle, trans);
    }
}

static void
bse_snet_context_connect (BseSource *source,
			  guint      context_handle,
			  GslTrans  *trans)
{
  BseSNet *self = BSE_SNET (source);
  ContextData *cdata = find_context_data (self, context_handle);
  guint i;
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_connect (source, context_handle, trans);
  
  for (i = 0; i < cdata->n_branches; i++)
    bse_source_connect_context (source, cdata->branches[i], trans);
}

static void
bse_snet_context_dismiss (BseSource *source,
			  guint      context_handle,
			  GslTrans  *trans)
{
  BseSNet *self = BSE_SNET (source);
  ContextData *cdata = find_context_data (self, context_handle);
  
  while (cdata->n_branches)
    bse_source_dismiss_context (source, cdata->branches[cdata->n_branches - 1], trans);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}
