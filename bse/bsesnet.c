/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik
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
#include        "bsesnet.h"

#include        "bseproject.h"
#include        "bsecategories.h"
#include        "bsestorage.h"
#include        "bsemarshal.h"
#include        "bsemain.h"
#include        <string.h>
#include        <time.h>
#include        <fcntl.h>
#include        <unistd.h>
#include        <stdlib.h>
#include        "gbsearcharray.h"
#include        <gsl/gslengine.h>


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_AUTO_ACTIVATE
};


/* --- prototypes --- */
static void      bse_snet_class_init             (BseSNetClass   *class);
static void      bse_snet_init                   (BseSNet        *snet);
static void      bse_snet_do_destroy             (BseObject      *object);
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
static void      bse_snet_prepare                (BseSource      *source);
static void      bse_snet_reset                  (BseSource      *source);
static gint	 snet_ports_compare              (gconstpointer   bsearch_node1, /* key */
						  gconstpointer   bsearch_node2);
static void      bse_snet_context_dismiss	 (BseSource      *source,
						  guint           context_handle,
						  GslTrans       *trans);


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
  gobject_class->finalize = bse_snet_finalize;
  
  object_class->destroy = bse_snet_do_destroy;
  
  source_class->prepare = bse_snet_prepare;
  source_class->context_dismiss = bse_snet_context_dismiss;
  source_class->reset = bse_snet_reset;
  
  container_class->add_item = bse_snet_add_item;
  container_class->remove_item = bse_snet_remove_item;
  container_class->forall_items = bse_snet_forall_items;
  
  bse_object_class_add_param (object_class, "Playback Settings",
			      PARAM_AUTO_ACTIVATE,
			      g_param_spec_boolean ("auto_activate", "Auto Activate",
						    "Automatic activation only needs to be enabled for synthesis networks "
						    "that don't use virtual ports for their input or output",
						    FALSE, BSE_PARAM_DEFAULT));
  signal_port_unregistered = bse_object_class_add_signal (object_class, "port_unregistered",
							  bse_marshal_VOID__NONE, NULL,
							  G_TYPE_NONE, 0);
}

static void
bse_snet_init (BseSNet *snet)
{
  BSE_OBJECT_SET_FLAGS (snet, BSE_SNET_FLAG_USER_SYNTH);
  BSE_SUPER (snet)->auto_activate = FALSE;
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
bse_snet_do_destroy (BseObject *object)
{
  BseSNet *snet = BSE_SNET (object);
  
  while (snet->sources)
    bse_container_remove_item (BSE_CONTAINER (snet), snet->sources->data);
  if (snet->iport_names)
    g_warning ("%s: leaking %cport \"%s\"", G_STRLOC, 'i', (gchar*) snet->iport_names->data);
  if (snet->oport_names)
    g_warning ("%s: leaking %cport \"%s\"", G_STRLOC, 'o', (gchar*) snet->oport_names->data);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_snet_finalize (GObject *object)
{
  BseSNet *snet = BSE_SNET (object);
  
  if (snet->port_unregistered_id)
    {
      g_source_remove (snet->port_unregistered_id);
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
    snet->port_unregistered_id = g_idle_add_full (BSE_NOTIFY_PRIORITY, snet_notify_port_unregistered, snet, NULL);
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
      BSE_SUPER (self)->auto_activate = g_value_get_boolean (value);
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
      g_value_set_boolean (value, BSE_SUPER (self)->auto_activate);
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

guint
bse_snet_create_context (BseSNet  *snet,
			 GslTrans *trans)
{
  guint cid;
  
  g_return_val_if_fail (BSE_IS_SNET (snet), 0);
  g_return_val_if_fail (BSE_SOURCE_PREPARED (snet), 0);
  g_return_val_if_fail (trans != NULL, 0);
  
  cid = bse_snet_alloc_cid (snet);
  g_return_val_if_fail (cid > 0, 0);
  g_return_val_if_fail (bse_source_has_context (BSE_SOURCE (snet), cid) == FALSE, 0);
  
  bse_source_create_context (BSE_SOURCE (snet), cid, trans);
  
  return cid;
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
bse_snet_context_dismiss (BseSource *source,
			  guint      context_handle,
			  GslTrans  *trans)
{
  BseSNet *self = BSE_SNET (source);
  
  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
  
  bse_snet_free_cid (self, context_handle);
}
