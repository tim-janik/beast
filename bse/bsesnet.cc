// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseproject.hh"
#include "bsecategories.hh"
#include "bsestorage.hh"
#include "bsemain.hh"
#include "bsecontextmerger.hh"
#include "bsemidireceiver.hh"
#include "bse/internal.hh"
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <bse/bseengine.hh>
#include <bse/gslcommon.hh>
#include "bsesnet.hh"

typedef struct
{
  guint            context_id;
  BseMidiReceiver *midi_receiver;
  guint            midi_channel;
  guint		   n_branches;
  guint		  *branches;
  guint		   parent_context;
} ContextData;

/* --- prototypes --- */
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
						  BseTrans       *trans);
static void      bse_snet_context_connect	 (BseSource      *source,
						  guint           context_handle,
						  BseTrans       *trans);
static void      bse_snet_context_dismiss	 (BseSource      *source,
						  guint           context_handle,
						  BseTrans       *trans);
static GSList*	 snet_context_children		 (BseContainer	 *container);


/* --- variables --- */
static GTypeClass          *parent_class = NULL;
static const GBSearchConfig port_array_config = {
  sizeof (BseSNetPort),
  snet_ports_compare,
  0, /* G_BSEARCH_ARRAY_ALIGN_POWER2 */
};


/* --- functions --- */
static void
bse_snet_init (BseSNet *self)
{
  self->unset_flag (BSE_SNET_FLAG_USER_SYNTH);
  self->set_flag (BSE_SUPER_FLAG_NEEDS_CONTEXT);
  self->sources = NULL;
  self->isources = NULL;
  self->iport_names = NULL;
  self->oport_names = NULL;
  self->port_array = NULL;
  self->port_unregistered_id = 0;
}

/**
 * @param self	valid BseSNet
 * @param child	valid BseItem, which is a child of @a self
 *
 * Mark @a child as internal via bse_item_set_internal() and
 * add special protection so to not destroy it automatically
 * upon g_object_run_dispose().
 */
void
bse_snet_intern_child (BseSNet *self,
                       gpointer child)
{
  BseItem *item = (BseItem*) child;

  assert_return (BSE_IS_SNET (self));
  assert_return (BSE_IS_ITEM (item));
  assert_return (item->parent == (BseItem*) self);
  assert_return (sfi_ring_find (self->sources, child) != NULL);

  self->sources = sfi_ring_remove (self->sources, child);
  self->isources = sfi_ring_append (self->isources, child);
  bse_item_set_internal (child, TRUE);
}

static void
bse_snet_release_children (BseContainer *container)
{
  BseSNet *snet = BSE_SNET (container);

  while (snet->sources)
    bse_container_remove_item (container, (BseItem*) sfi_ring_pop_head (&snet->sources));

  /* chain parent class' handler */
  BSE_CONTAINER_CLASS (parent_class)->release_children (container);
}

static void
bse_snet_dispose (GObject *object)
{
  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bse_snet_finalize (GObject *object)
{
  BseSNet *snet = BSE_SNET (object);
  BseContainer *container = BSE_CONTAINER (object);

  while (snet->sources)
    bse_container_remove_item (container, (BseItem*) sfi_ring_pop_head (&snet->sources));
  while (snet->isources)
    bse_container_remove_item (container, (BseItem*) sfi_ring_pop_head (&snet->isources));
  if (snet->port_unregistered_id)
    {
      bse_idle_remove (snet->port_unregistered_id);
      snet->port_unregistered_id = 0;
    }
  if (snet->iport_names)
    Bse::warning ("%s: %s: leaking %cport \"%s\"", G_STRLOC, G_OBJECT_TYPE_NAME (snet), 'i', (gchar*) snet->iport_names->data);
  if (snet->oport_names)
    Bse::warning ("%s: %s: leaking %cport \"%s\"", G_STRLOC, G_OBJECT_TYPE_NAME (snet), 'o', (gchar*) snet->oport_names->data);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);

  assert_return (snet->port_unregistered_id == 0);
}

static gboolean
snet_notify_port_unregistered (gpointer data)
{
  BseSNet *snet = BSE_SNET (data);
  Bse::SNetImpl *impl = snet->as<Bse::SNetImpl*>();

  BSE_THREADS_ENTER ();
  snet->port_unregistered_id = 0;
  impl->emit_event ("portunregistered");
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
bse_snet_add_item (BseContainer *container,
                   BseItem      *item)
{
  BseSNet *snet = BSE_SNET (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOURCE))
    snet->sources = sfi_ring_append (snet->sources, item);
  else if (BSE_SNET_USER_SYNTH (snet))
    Bse::warning ("BseSNet: cannot hold non-source item type `%s'", BSE_OBJECT_TYPE_NAME (item));

  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_snet_forall_items (BseContainer      *container,
                       BseForallItemsFunc func,
                       gpointer           data)
{
  BseSNet *snet = BSE_SNET (container);
  SfiRing *node;

  node = snet->sources;
  while (node)
    {
      BseItem *item = (BseItem*) node->data;
      node = sfi_ring_walk (node, snet->sources);
      if (!func (item, data))
        return;
    }

  node = snet->isources;
  while (node)
    {
      BseItem *item = (BseItem*) node->data;
      node = sfi_ring_walk (node, snet->isources);
      if (!func (item, data))
        return;
    }
}

static void
bse_snet_remove_item (BseContainer *container,
                      BseItem      *item)
{
  BseSNet *self = BSE_SNET (container);

  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_SOURCE))
    {
      SfiRing *node = sfi_ring_find (self->isources, item);
      if (node)
        self->isources = sfi_ring_remove_node (self->isources, node);
      else
        self->sources = sfi_ring_remove (self->sources, item);
    }
  else if (BSE_SNET_USER_SYNTH (self))
    Bse::warning ("BseSNet: cannot hold non-source item type `%s'", BSE_OBJECT_TYPE_NAME (item));

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
    if (strcmp (name, (const char*) slist->data) == 0)
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

  assert_return (BSE_IS_SNET (snet), NULL);
  assert_return (tmpl_name != NULL, NULL);

  slist = snet_find_port_name (snet, tmpl_name, TRUE);
  name = NULL;
  i = 1;
  while (slist)
    {
      g_free (name);
      name = g_strdup_format ("%s-%u", tmpl_name, i++);
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

  assert_return (BSE_IS_SNET (snet), FALSE);
  assert_return (name != NULL, FALSE);

  slist = snet_find_port_name (snet, name, TRUE);

  return slist != NULL;
}

void
bse_snet_iport_name_unregister (BseSNet     *snet,
				const gchar *name)
{
  GSList *slist;

  assert_return (BSE_IS_SNET (snet));
  assert_return (name != NULL);

  slist = snet_find_port_name (snet, name, TRUE);
  assert_return (slist != NULL);

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

  assert_return (BSE_IS_SNET (snet), NULL);
  assert_return (tmpl_name != NULL, NULL);

  slist = snet_find_port_name (snet, tmpl_name, FALSE);
  name = NULL;
  i = 1;
  while (slist)
    {
      g_free (name);
      name = g_strdup_format ("%s-%u", tmpl_name, i++);
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

  assert_return (BSE_IS_SNET (snet), FALSE);
  assert_return (name != NULL, FALSE);

  slist = snet_find_port_name (snet, name, FALSE);

  return slist != NULL;
}

void
bse_snet_oport_name_unregister (BseSNet     *snet,
				const gchar *name)
{
  GSList *slist;

  assert_return (BSE_IS_SNET (snet));
  assert_return (name != NULL);

  slist = snet_find_port_name (snet, name, FALSE);
  assert_return (slist != NULL);

  g_free (slist->data);
  snet->oport_names = g_slist_delete_link (snet->oport_names, slist);
  bse_snet_queue_port_unregistered (snet);
}

static gint
snet_ports_compare (gconstpointer bsearch_node1, /* key */
		    gconstpointer bsearch_node2)
{
  const BseSNetPort *p1 = (const BseSNetPort*) bsearch_node1;
  const BseSNetPort *p2 = (const BseSNetPort*) bsearch_node2;
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
  return (BseSNetPort*) g_bsearch_array_lookup (snet->port_array, &port_array_config, &key);
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

  port = (BseSNetPort*) g_bsearch_array_lookup (snet->port_array, &port_array_config, &key);
  assert_return (port == NULL, port);	/* shouldn't fail */

  key.name = g_strdup (key.name);
  key.src_omodule = NULL;
  key.src_ostream = G_MAXUINT;
  key.dest_imodule = NULL;
  key.dest_istream = G_MAXUINT;
  snet->port_array = g_bsearch_array_insert (snet->port_array, &port_array_config, &key);
  return (BseSNetPort*) g_bsearch_array_lookup (snet->port_array, &port_array_config, &key);
}

static void
port_delete (BseSNet     *snet,
	     BseSNetPort *port)
{
  guint index = g_bsearch_array_get_index (snet->port_array, &port_array_config, port);

  assert_return (index < g_bsearch_array_get_n_nodes (snet->port_array));
  assert_return (port->src_omodule == NULL && port->dest_imodule == NULL);

  g_free (port->name);
  g_bsearch_array_remove (snet->port_array, &port_array_config, index);
}

void
bse_snet_set_iport_src (BseSNet     *snet,
			const gchar *name,
			guint        snet_context,
			BseModule   *omodule,
			guint        ostream,
			BseTrans    *trans)
{
  BseSNetPort *port;

  assert_return (BSE_IS_SNET (snet));
  assert_return (name != NULL);
  assert_return (bse_source_has_context (BSE_SOURCE (snet), snet_context));
  if (omodule)
    assert_return (ostream < BSE_MODULE_N_OSTREAMS (omodule));
  assert_return (trans != NULL);

  port = port_lookup (snet, name, snet_context, TRUE);
  if (!port && !omodule)
    return;
  else if (!port)
    port = port_insert (snet, name, snet_context, TRUE);
  else if (!omodule)
    ostream = G_MAXUINT;

  if (port->src_omodule && port->dest_imodule)
    bse_trans_add (trans, bse_job_disconnect (port->dest_imodule, port->dest_istream));
  port->src_omodule = omodule;
  port->src_ostream = ostream;
  if (port->src_omodule && port->dest_imodule)
    bse_trans_add (trans, bse_job_connect (port->src_omodule, port->src_ostream,
					   port->dest_imodule, port->dest_istream));
  if (!port->dest_imodule && !port->src_omodule)
    port_delete (snet, port);
}

void
bse_snet_set_iport_dest (BseSNet     *snet,
			 const gchar *name,
			 guint        snet_context,
			 BseModule   *imodule,
			 guint        istream,
			 BseTrans    *trans)
{
  BseSNetPort *port;

  assert_return (BSE_IS_SNET (snet));
  assert_return (name != NULL);
  assert_return (bse_source_has_context (BSE_SOURCE (snet), snet_context));
  if (imodule)
    assert_return (istream < BSE_MODULE_N_ISTREAMS (imodule));
  assert_return (trans != NULL);

  port = port_lookup (snet, name, snet_context, TRUE);
  if (!port && !imodule)
    return;
  else if (!port)
    port = port_insert (snet, name, snet_context, TRUE);
  else if (!imodule)
    istream = G_MAXUINT;

  if (port->src_omodule && port->dest_imodule)
    bse_trans_add (trans, bse_job_disconnect (port->dest_imodule, port->dest_istream));
  port->dest_imodule = imodule;
  port->dest_istream = istream;
  if (port->src_omodule && port->dest_imodule)
    bse_trans_add (trans, bse_job_connect (port->src_omodule, port->src_ostream,
					   port->dest_imodule, port->dest_istream));
  if (!port->dest_imodule && !port->src_omodule)
    port_delete (snet, port);
}

void
bse_snet_set_oport_src (BseSNet     *snet,
			const gchar *name,
			guint        snet_context,
			BseModule   *omodule,
			guint        ostream,
			BseTrans    *trans)
{
  BseSNetPort *port;

  assert_return (BSE_IS_SNET (snet));
  assert_return (name != NULL);
  assert_return (bse_source_has_context (BSE_SOURCE (snet), snet_context));
  if (omodule)
    assert_return (ostream < BSE_MODULE_N_OSTREAMS (omodule));
  assert_return (trans != NULL);

  port = port_lookup (snet, name, snet_context, FALSE);
  if (!port && !omodule)
    return;
  else if (!port)
    port = port_insert (snet, name, snet_context, FALSE);
  else if (!omodule)
    ostream = G_MAXUINT;

  if (port->src_omodule && port->dest_imodule)
    bse_trans_add (trans, bse_job_disconnect (port->dest_imodule, port->dest_istream));
  port->src_omodule = omodule;
  port->src_ostream = ostream;
  if (port->src_omodule && port->dest_imodule)
    bse_trans_add (trans, bse_job_connect (port->src_omodule, port->src_ostream,
					   port->dest_imodule, port->dest_istream));
  if (!port->dest_imodule && !port->src_omodule)
    port_delete (snet, port);
}

void
bse_snet_set_oport_dest (BseSNet     *snet,
			 const gchar *name,
			 guint        snet_context,
			 BseModule   *imodule,
			 guint        istream,
			 BseTrans    *trans)
{
  BseSNetPort *port;

  assert_return (BSE_IS_SNET (snet));
  assert_return (name != NULL);
  assert_return (bse_source_has_context (BSE_SOURCE (snet), snet_context));
  if (imodule)
    assert_return (istream < BSE_MODULE_N_ISTREAMS (imodule));
  assert_return (trans != NULL);

  port = port_lookup (snet, name, snet_context, FALSE);
  if (!port && !imodule)
    return;
  else if (!port)
    port = port_insert (snet, name, snet_context, FALSE);
  else if (!imodule)
    istream = G_MAXUINT;

  if (port->src_omodule && port->dest_imodule)
    bse_trans_add (trans, bse_job_disconnect (port->dest_imodule, port->dest_istream));
  port->dest_imodule = imodule;
  port->dest_istream = istream;
  if (port->src_omodule && port->dest_imodule)
    bse_trans_add (trans, bse_job_connect (port->src_omodule, port->src_ostream,
					   port->dest_imodule, port->dest_istream));
  if (!port->dest_imodule && !port->src_omodule)
    port_delete (snet, port);
}

static inline ContextData*
find_context_data (BseSNet *self,
		   guint    context_id)
{
  return (ContextData*) bse_source_get_context_data (BSE_SOURCE (self), context_id);
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
		   BseTrans  *trans)
{
  BseSNet *self = BSE_SNET (source);
  ContextData *cdata = (ContextData*) data;

  assert_return (cdata->n_branches == 0);

  bse_midi_receiver_unref (cdata->midi_receiver);
  bse_id_free (cdata->context_id);
  if (cdata->parent_context)
    {
      ContextData *pdata = find_context_data (self, cdata->parent_context);
      guint i, swap_context;

      assert_return (pdata->n_branches > 0);

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
                         BseMidiContext   mcontext,
			 BseTrans        *trans)
{
  ContextData *cdata;
  guint cid;

  assert_return (BSE_IS_SNET (self), 0);
  assert_return (BSE_SOURCE_PREPARED (self), 0);
  assert_return (mcontext.midi_receiver != NULL, 0);
  assert_return (trans != NULL, 0);

  cid = bse_id_alloc ();
  assert_return (bse_source_has_context (BSE_SOURCE (self), cid) == FALSE, 0);

  cdata = create_context_data (self, cid, 0, mcontext.midi_receiver, mcontext.midi_channel);
  bse_source_create_context_with_data (BSE_SOURCE (self), cid, cdata, free_context_data, trans);

  return cid;
}

guint
bse_snet_context_clone_branch (BseSNet         *self,
			       guint            context,
			       BseSource       *context_merger,
                               BseMidiContext   mcontext,
			       BseTrans        *trans)
{
  SfiRing *ring;
  guint bcid = 0;

  assert_return (BSE_IS_SNET (self), 0);
  assert_return (BSE_SOURCE_PREPARED (self), 0);
  assert_return (bse_source_has_context (BSE_SOURCE (self), context), 0);
  assert_return (BSE_IS_CONTEXT_MERGER (context_merger), 0);
  assert_return (bse_source_has_context (context_merger, context), 0);
  assert_return (BSE_ITEM (context_merger)->parent == BSE_ITEM (self), 0);
  assert_return (mcontext.midi_receiver != NULL, 0);
  assert_return (trans != NULL, 0);

  ring = bse_source_collect_inputs_recursive (context_merger);
  if (!BSE_SOURCE_COLLECTED (context_merger))
    {
      ContextData *cdata;
      SfiRing *node;

      assert_return (self->tmp_context_children == NULL, 0);
      for (node = ring; node; node = sfi_ring_walk (node, ring))
	self->tmp_context_children = g_slist_prepend (self->tmp_context_children, node->data);
      self->tmp_context_children = g_slist_prepend (self->tmp_context_children, context_merger);
      bse_source_free_collection (ring);
      bcid = bse_id_alloc ();
      cdata = create_context_data (self, bcid, context, mcontext.midi_receiver, mcontext.midi_channel);
      bse_source_create_context_with_data (BSE_SOURCE (self), bcid, cdata, free_context_data, trans);
      assert_return (self->tmp_context_children == NULL, 0);
    }
  else
    {
      Bse::warning ("%s: context merger forms a cycle with it's inputs", G_STRLOC);
      bse_source_free_collection (ring);
    }


  return bcid;
}

gboolean
bse_snet_context_is_branch (BseSNet *self,
			    guint    context_id)
{
  ContextData *cdata;

  assert_return (BSE_IS_SNET (self), FALSE);
  assert_return (BSE_SOURCE_PREPARED (self), FALSE);
  assert_return (context_id > 0, FALSE);

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

BseMidiContext
bse_snet_get_midi_context (BseSNet *self,
                           guint    context_handle)
{
  BseMidiContext mcontext = { 0, };
  ContextData *cdata;

  assert_return (BSE_IS_SNET (self), mcontext);

  cdata = find_context_data (self, context_handle);
  if (cdata)
    {
      mcontext.midi_receiver = cdata->midi_receiver;
      mcontext.midi_channel = cdata->midi_channel;
    }
  return mcontext;
}

static void
bse_snet_prepare (BseSource *source)
{
  BseSNet *snet = BSE_SNET (source);

  assert_return (snet->port_array == NULL);

  bse_object_lock (BSE_OBJECT (snet));
  snet->port_array = g_bsearch_array_create (&port_array_config);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->prepare (source);
}

static void
bse_snet_reset (BseSource *source)
{
  BseSNet *self = BSE_SNET (source);

  assert_return (self->port_array != NULL);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->reset (source);

  if (g_bsearch_array_get_n_nodes (self->port_array))
    {
      BseSNetPort *port = (BseSNetPort*) g_bsearch_array_get_nth (self->port_array, &port_array_config, 0);

      Bse::warning ("%s: %cport \"%s\" still active: context=%u src=%p dest=%p", G_STRLOC,
                    port->input ? 'i' : 'o', port->name,
                    port->context, port->src_omodule, port->dest_imodule);
    }
  g_bsearch_array_free (self->port_array, &port_array_config);
  self->port_array = NULL;

  bse_object_unlock (BSE_OBJECT (self));
}

static void
bse_snet_context_create (BseSource *source,
			 guint      context_handle,
			 BseTrans  *trans)
{
  BseSNet *self = BSE_SNET (source);

  if (self->tmp_context_children)
    {
      BseContextMerger *context_merger = (BseContextMerger*) self->tmp_context_children->data;
      ContextData *cdata = find_context_data (self, context_handle);

      assert_return (BSE_IS_CONTEXT_MERGER (context_merger));

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
			  BseTrans  *trans)
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
			  BseTrans  *trans)
{
  BseSNet *self = BSE_SNET (source);
  ContextData *cdata = find_context_data (self, context_handle);

  while (cdata->n_branches)
    bse_source_dismiss_context (source, cdata->branches[cdata->n_branches - 1], trans);

  /* chain parent class' handler */
  BSE_SOURCE_CLASS (parent_class)->context_dismiss (source, context_handle, trans);
}

static void
bse_snet_class_init (BseSNetClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BseSourceClass *source_class = BSE_SOURCE_CLASS (klass);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (klass);

  parent_class = (GTypeClass*) g_type_class_peek_parent (klass);

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
}

BSE_BUILTIN_TYPE (BseSNet)
{
  static const GTypeInfo type_info = {
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
  assert_return (BSE_SNET_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT, 0);
  return bse_type_register_abstract (BSE_TYPE_SUPER, "BseSNet", "BSE Synthesis (Filter) Network", __FILE__, __LINE__, &type_info);
}

namespace Bse {

SNetImpl::SNetImpl (BseObject *bobj) :
  SuperImpl (bobj)
{}

SNetImpl::~SNetImpl ()
{}

bool
SNetImpl::supports_user_synths ()
{
  BseSNet *self = as<BseSNet*>();
  return BSE_SNET_USER_SYNTH (self);
}

bool
SNetImpl::auto_activate () const
{
  BseSNet *self = const_cast<SNetImpl*> (this)->as<BseSNet*>();
  return BSE_SUPER_NEEDS_CONTEXT (self);
}

void
SNetImpl::auto_activate (bool v)
{
  BseSNet *self = as<BseSNet*>();
  if (v)
    self->set_flag (BSE_SUPER_FLAG_NEEDS_CONTEXT);
  else
    self->unset_flag (BSE_SUPER_FLAG_NEEDS_CONTEXT);
  notify ("auto_activate");
}

Error
SNetImpl::can_create_source (const String &module_type)
{
  BseSNet *self = as<BseSNet*>();
  GType type = g_type_from_name (module_type.c_str());
  Error error = Error::NONE;
  if (!BSE_SNET_USER_SYNTH (self) && !BSE_DBG_EXT)
    error = Error::NOT_OWNER;
  else if (!g_type_is_a (type, BSE_TYPE_SOURCE) ||
	   g_type_is_a (type, BSE_TYPE_CONTAINER))
    error = Error::SOURCE_TYPE_INVALID;
  return error;
}

SourceIfaceP
SNetImpl::create_source (const String &module_type)
{
  BseSNet *self = as<BseSNet*>();
  if (can_create_source (module_type) != Error::NONE)
    return NULL;
  BseUndoStack *ustack = bse_item_undo_open (self, __func__);
  BseSource *child = (BseSource*) bse_container_new_child (self, g_type_from_name (module_type.c_str()), NULL);
  if (child)
    {
      // an undo lambda is needed for wrapping object argument references
      UndoDescriptor<SourceImpl> child_descriptor = undo_descriptor (*child->as<SourceImpl*>());
      auto lambda = [child_descriptor] (SNetImpl &self, BseUndoStack *ustack) -> Error {
        return self.remove_source (self.undo_resolve (child_descriptor));
      };
      push_undo (__func__, *this, lambda);
    }
  bse_item_undo_close (ustack);
  return child ? child->as<SourceIfaceP>() : NULL;
}

Error
SNetImpl::remove_source (SourceIface &module)
{
  BseSNet *self = as<BseSNet*>();
  BseSource *child = module.as<BseSource*>();
  Bse::Error error = Bse::Error::NONE;
  if (!BSE_IS_SOURCE (child) || child->parent != self || (!BSE_SNET_USER_SYNTH (self) && !BSE_DBG_EXT))
    return Error::PROC_PARAM_INVAL;
  BseUndoStack *ustack = bse_item_undo_open (self, string_format ("%s: %s", __func__, bse_object_debug_name (child)).c_str());
  bse_container_uncross_undoable (self, child);
  {
    // an undo lambda is needed for wrapping object argument references
    UndoDescriptor<SourceImpl> child_descriptor = undo_descriptor (*child->as<SourceImpl*>());
    auto lambda = [child_descriptor] (SNetImpl &self, BseUndoStack *ustack) -> Error {
      return self.remove_source (self.undo_resolve (child_descriptor));
    };
    push_undo_to_redo (__func__, *this, lambda); // how to get rid of the item once backed up
  }
  bse_container_remove_backedup (BSE_CONTAINER (self), child, ustack); // remove (without redo queueing)
  bse_item_undo_close (ustack);
  return error;
}

} // Bse
