// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseengineschedule.hh"
#include "bseengineutils.hh"

#define SCHED_DEBUG(...) Bse::debug ("sched", __VA_ARGS__)
#define CHECK_DEBUG()    Bse::debug_enabled ("sched")

/* --- prototypes --- */
static void	schedule_node		(EngineSchedule	*schedule,
					 EngineNode	*node,
					 guint		 leaf_level);
static void	schedule_cycle		(EngineSchedule	*schedule,
					 SfiRing	*cycle_nodes,
					 guint		 leaf_level);
static void 	subschedule_query_node	(EngineSchedule *schedule,
					 EngineNode     *node,
					 EngineQuery    *query);


/* --- functions --- */
EngineSchedule*
_engine_schedule_new (void)
{
  EngineSchedule *sched = sfi_new_struct0 (EngineSchedule, 1);

  sched->n_items = 0;
  sched->leaf_levels = 0;
  sched->nodes = NULL;
  sched->cycles = NULL;
  sched->secured = FALSE;
  sched->in_pqueue = FALSE;
  sched->cur_leaf_level = ~0;
  sched->cur_node = NULL;
  sched->cur_cycle = NULL;
  sched->vnodes = NULL;

  return sched;
}

static inline void
unschedule_virtual (EngineSchedule *sched, EngineNode *vnode)
{
  assert_return (ENGINE_NODE_IS_SCHEDULED (vnode) == TRUE);
  assert_return (sched->n_items > 0);

  /* SCHED_DEBUG ("unschedule_virtual(%p)", vnode); */
  sched->vnodes = sfi_ring_remove (sched->vnodes, vnode);
  vnode->sched_tag = FALSE;
  sched->n_items--;
}

static inline void
unschedule_node (EngineSchedule *sched, EngineNode *node)
{
  guint leaf_level;

  assert_return (ENGINE_NODE_IS_SCHEDULED (node) == TRUE);
  leaf_level = node->sched_leaf_level;
  assert_return (leaf_level <= sched->leaf_levels);
  assert_return (sched->n_items > 0);

  /* SCHED_DEBUG ("unschedule_node(%p,%u)", node, leaf_level); */
  sched->nodes[leaf_level] = sfi_ring_remove (sched->nodes[leaf_level], node);
  node->sched_leaf_level = 0;
  node->sched_tag = FALSE;
  if (node->flow_jobs)
    _engine_mnl_node_changed (node);
  sched->n_items--;
}

static inline void
unschedule_cycle (EngineSchedule *sched,
		  SfiRing        *ring)
{
  guint leaf_level;
  SfiRing *walk;

  assert_return (ENGINE_NODE_IS_SCHEDULED (ENGINE_NODE (ring->data)) == TRUE);
  leaf_level = ENGINE_NODE (ring->data)->sched_leaf_level;
  assert_return (leaf_level <= sched->leaf_levels);
  assert_return (sched->n_items > 0);

  /* SCHED_DEBUG ("unschedule_cycle(%p,%u,%p)", ring->data, leaf_level, ring); */
  sched->nodes[leaf_level] = sfi_ring_remove (sched->nodes[leaf_level], ring);
  for (walk = ring; walk; walk = sfi_ring_walk (walk, ring))
    {
      EngineNode *node = (EngineNode*) walk->data;
      if (!ENGINE_NODE_IS_SCHEDULED (node))
	g_warning ("node(%p) in schedule ring(%p) is untagged", node, ring);
      node->sched_leaf_level = 0;
      node->sched_tag = FALSE;
      if (node->flow_jobs)
	_engine_mnl_node_changed (node);
    }
  sched->n_items--;
}

static void
_engine_schedule_debug_dump (EngineSchedule *sched)
{
  Bse::printerr ("sched(%p) = {\n", sched);
  if (sched)
    {
      guint i;

      Bse::printerr ("  n_items=%u, n_vnodes=%u, leaf_levels=%u, secured=%u,\n",
                     sched->n_items, sfi_ring_length (sched->vnodes), sched->leaf_levels, sched->secured);
      Bse::printerr ("  in_pqueue=%u, cur_leaf_level=%u,\n",
                     sched->in_pqueue, sched->cur_leaf_level);
      Bse::printerr ("  cur_node=%p, cur_cycle=%p,\n",
                     sched->cur_node, sched->cur_cycle);
      for (i = 0; i < sched->leaf_levels; i++)
	{
	  SfiRing *ring, *head = sched->nodes[i];

	  if (!head)
	    continue;
	  Bse::printerr ("  { leaf_level=%u:", i);
	  for (ring = head; ring; ring = sfi_ring_walk (ring, head))
	    Bse::printerr (" node(%p(i:%u,s:%u))", ring->data,
                           ((EngineNode*) ring->data)->integrated,
                           ((EngineNode*) ring->data)->sched_tag);
	  Bse::printerr (" },\n");
	}
      SfiRing *ring;
      Bse::printerr ("  { vnodes:");
      for (ring = sched->vnodes; ring; ring = sfi_ring_walk (ring, sched->vnodes))
        Bse::printerr (" vnode(%p(pj:%u))", ring->data, ((EngineNode*) ring->data)->probe_jobs != 0);
      Bse::printerr (" },\n");
    }
  Bse::printerr ("};\n");
}


void
_engine_schedule_clear (EngineSchedule *sched)
{
  guint i;

  assert_return (sched != NULL);
  assert_return (sched->secured == FALSE);
  assert_return (sched->in_pqueue == FALSE);

  while (sched->vnodes)
    unschedule_virtual (sched, (EngineNode*) sched->vnodes->data);
  for (i = 0; i < sched->leaf_levels; i++)
    {
      while (sched->nodes[i])
	unschedule_node (sched, (EngineNode*) sched->nodes[i]->data);
      while (sched->cycles[i])
	unschedule_cycle (sched, (SfiRing*) sched->cycles[i]->data);
    }
  assert_return (sched->n_items == 0);
}

void
_engine_schedule_destroy (EngineSchedule *sched)
{
  assert_return (sched != NULL);
  assert_return (sched->secured == FALSE);
  assert_return (sched->in_pqueue == FALSE);

  _engine_schedule_clear (sched);
  g_free (sched->nodes);
  g_free (sched->cycles);
  sfi_delete_struct (EngineSchedule, sched);
}

static void
_engine_schedule_grow (EngineSchedule *sched,
		       guint           leaf_level)
{
  guint ll = 1 << g_bit_storage (leaf_level);	/* power2 growth alignment, ll >= leaf_level+1 */

  if (sched->leaf_levels < ll)
    {
      guint i = sched->leaf_levels;

      sched->leaf_levels = ll;
      sched->nodes = g_renew (SfiRing*, sched->nodes, sched->leaf_levels);
      sched->cycles = g_renew (SfiRing*, sched->cycles, sched->leaf_levels);
      for (; i < sched->leaf_levels; i++)
	{
	  sched->nodes[i] = NULL;
	  sched->cycles[i] = NULL;
	}
    }
}

static void
schedule_virtual (EngineSchedule *sched,
		  EngineNode     *vnode)
{
  assert_return (sched != NULL);
  assert_return (sched->secured == FALSE);
  assert_return (vnode != NULL);
  assert_return (ENGINE_NODE_IS_VIRTUAL (vnode));
  assert_return (!ENGINE_NODE_IS_SCHEDULED (vnode));

  /* SCHED_DEBUG ("schedule_virtual(%p)", vnode); */
  vnode->sched_tag = TRUE;
  vnode->cleared_ostreams = FALSE;
  sched->vnodes = sfi_ring_append (sched->vnodes, vnode);
  sched->n_items++;
  guint i;
  for (i = 0; i < ENGINE_NODE_N_ISTREAMS (vnode); i++)
    {
      vnode->inputs[i].real_node = NULL;
      vnode->inputs[i].real_stream = 0;
      /* _used_ virtual inputs are filled later on */
    }
}

static void
schedule_node (EngineSchedule *sched,
	       EngineNode     *node,
	       guint           leaf_level)
{
  assert_return (sched != NULL);
  assert_return (sched->secured == FALSE);
  assert_return (node != NULL);
  assert_return (!ENGINE_NODE_IS_SCHEDULED (node));

  /* SCHED_DEBUG ("schedule_node(%p,%u)", node, leaf_level); */
  node->sched_leaf_level = leaf_level;
  node->sched_tag = TRUE;
  node->cleared_ostreams = FALSE;
  if (node->flow_jobs)
    _engine_mnl_node_changed (node);
  _engine_schedule_grow (sched, leaf_level);
  /* could do 3-stage scheduling by expensiveness */
  sched->nodes[leaf_level] = (ENGINE_NODE_IS_EXPENSIVE (node) ? sfi_ring_prepend : sfi_ring_append) (sched->nodes[leaf_level], node);
  sched->n_items++;
}

static void
schedule_cycle (EngineSchedule *sched,
		SfiRing        *cycle_nodes,
		guint           leaf_level)
{
  SfiRing *walk;
  assert_return (sched != NULL);
  assert_return (sched->secured == FALSE);
  assert_return (cycle_nodes != NULL);

  for (walk = cycle_nodes; walk; walk = sfi_ring_walk (walk, cycle_nodes))
    {
      EngineNode *node = (EngineNode*) walk->data;
      assert_return (!ENGINE_NODE_IS_SCHEDULED (node));
      node->sched_leaf_level = leaf_level;
      node->sched_tag = TRUE;
      node->cleared_ostreams = FALSE;
      if (node->flow_jobs)
	_engine_mnl_node_changed (node);
    }
  _engine_schedule_grow (sched, leaf_level);
  sched->cycles[leaf_level] = sfi_ring_prepend (sched->cycles[leaf_level], cycle_nodes);
  sched->n_items++;
}

void
_engine_schedule_restart (EngineSchedule *sched)
{
  assert_return (sched != NULL);
  assert_return (sched->secured == TRUE);
  assert_return (sched->cur_leaf_level == sched->leaf_levels);
  assert_return (sched->cur_node == NULL);
  assert_return (sched->cur_cycle == NULL);

  sched->cur_leaf_level = 0;
  if (sched->leaf_levels > 0)
    {
      sched->cur_node = sched->nodes[0];
      sched->cur_cycle = sched->cycles[0];
    }
}
void
_engine_schedule_secure (EngineSchedule *sched)
{
  assert_return (sched != NULL);
  assert_return (sched->secured == FALSE);
  sched->secured = TRUE;
  sched->cur_leaf_level = sched->leaf_levels;
  if (CHECK_DEBUG())
    _engine_schedule_debug_dump (sched);
}
static void
schedule_advance (EngineSchedule *sched)
{
  while (!sched->cur_node && !sched->cur_cycle && sched->cur_leaf_level < sched->leaf_levels)
    {
      sched->cur_leaf_level += 1;
      if (sched->cur_leaf_level < sched->leaf_levels)
	{
	  sched->cur_node = sched->nodes[sched->cur_leaf_level];
	  sched->cur_cycle = sched->cycles[sched->cur_leaf_level];
	}
    }
}

EngineNode*
_engine_schedule_pop_node (EngineSchedule *sched)
{
  assert_return (sched != NULL, NULL);
  assert_return (sched->secured == TRUE, NULL);
  assert_return (sched->cur_leaf_level <= sched->leaf_levels, NULL);
  do
    {
      uint leaf_level = sched->cur_leaf_level;
      if (sched->cur_node)
	{
	  EngineNode *node = (EngineNode*) sched->cur_node->data;
	  sched->cur_node = sfi_ring_walk (sched->cur_node, sched->nodes[leaf_level]);
	  return node;
	}
      schedule_advance (sched);
    }
  while (sched->cur_node);
  /* nothing to hand out, either we're empty or still have cycles pending */
  return NULL;
}

SfiRing*
_engine_schedule_pop_cycle (EngineSchedule *sched)
{
  assert_return (sched != NULL, NULL);
  assert_return (sched->secured == TRUE, NULL);
  assert_return (sched->cur_leaf_level <= sched->leaf_levels, NULL);
  do
    {
      guint leaf_level = sched->cur_leaf_level;
      if (sched->cur_cycle)
	{
	  SfiRing *cycle = (SfiRing*) sched->cur_cycle->data;
	  sched->cur_cycle = sfi_ring_walk (sched->cur_cycle, sched->cycles[leaf_level]);
	  return cycle;
	}
      schedule_advance (sched);
    }
  while (sched->cur_cycle);
  /* nothing to hand out, either we're empty or still have nodes pending */
  return NULL;
}

void
_engine_schedule_unsecure (EngineSchedule *sched)
{
  assert_return (sched != NULL);
  assert_return (sched->secured == TRUE);
  assert_return (sched->in_pqueue == FALSE);
  assert_return (sched->cur_leaf_level == sched->leaf_levels);
  assert_return (sched->cur_node == NULL);
  assert_return (sched->cur_cycle == NULL);

  sched->secured = FALSE;
  sched->cur_leaf_level = ~0;
}

void
_engine_schedule_consumer_node (EngineSchedule *schedule,
				EngineNode     *node)
{
  EngineQuery query = { 0, };

  assert_return (schedule != NULL);
  assert_return (schedule->secured == FALSE);
  assert_return (node != NULL);
  assert_return (ENGINE_NODE_IS_CONSUMER (node));
  assert_return (ENGINE_NODE_IS_VIRTUAL (node) == FALSE);

  subschedule_query_node (schedule, node, &query);
  assert_return (query.cycles == NULL);	/* paranoid */
  assert_return (query.cycle_nodes == NULL);	/* paranoid */
  schedule_node (schedule, node, query.leaf_level);
}


/* --- depth scheduling --- */
static gboolean
determine_suspension_reset (EngineNode *node)
{
  assert_return (node->update_suspend == FALSE, FALSE);
  assert_return (node->in_suspend_call == FALSE, FALSE);

  if (!ENGINE_NODE_IS_VIRTUAL (node))
    return node->needs_reset;

  SfiRing *ring;
  gboolean keep_state = FALSE;
  node->in_suspend_call = TRUE;
  for (ring = node->output_nodes; ring && !keep_state; ring = sfi_ring_walk (ring, node->output_nodes))
    {
      EngineNode *dest_node = (EngineNode*) ring->data;
      if (!dest_node->in_suspend_call)          /* break cycles (consisting of purely virtual nodes) */
        keep_state |= !determine_suspension_reset (dest_node);
    }
  node->in_suspend_call = FALSE;

  return !keep_state;
}

static guint64
determine_suspension_state (EngineNode *node,
                            gboolean   *seen_cycle_p,
                            gboolean   *keep_state_p)
{
  gboolean seen_cycle = FALSE;
  guint64 stamp = Bse::TickStamp::max_stamp();
  assert_return (node->in_suspend_call == FALSE, stamp);
  if (node->update_suspend)
    {
      node->in_suspend_call = TRUE;
      SfiRing *ring;    /* calculate outer suspend constraints */
      if (ENGINE_NODE_IS_CONSUMER (node))
        stamp = 0;
      gboolean keep_state = FALSE;
      for (ring = node->output_nodes; ring; ring = sfi_ring_walk (ring, node->output_nodes))
        {
          EngineNode *dest_node = (EngineNode*) ring->data;
          if (!dest_node->in_suspend_call)      /* catch cycles */
            {
              guint64 ostamp = determine_suspension_state (dest_node, &seen_cycle, &keep_state);
              stamp = MIN (ostamp, stamp);
            }
          else
            seen_cycle = TRUE;
        }
      node->needs_reset |= !keep_state;
      /* bound against inner suspend constraint */
      stamp = MAX (stamp, node->local_active);
      if (!seen_cycle)
        {
          node->next_active = stamp;
          node->update_suspend = FALSE;
        }
      node->in_suspend_call = FALSE;
    }
  else
    stamp = node->next_active;
  *keep_state_p |= !determine_suspension_reset (node);
  *seen_cycle_p = *seen_cycle_p || seen_cycle;
  return stamp;
}

static inline void
update_suspension_state (EngineNode *node)
{
  if (node->update_suspend)
    {
      gboolean seen_cycle = FALSE;
      gboolean keep_state = FALSE;
      guint64 stamp = determine_suspension_state (node, &seen_cycle, &keep_state);
      node->needs_reset |= !keep_state;
      if (node->update_suspend)         /* break cycles */
        {
          node->next_active = stamp;
          node->update_suspend = FALSE;
        }
    }
}

static SfiRing*
merge_untagged_node_lists_uniq (SfiRing *ring1,
				SfiRing *ring2)
{
  SfiRing *walk;
  /* paranoid, ensure all nodes are untagged (ring2) */
  for (walk = ring2; walk; walk = sfi_ring_walk (walk, ring2))
    {
      EngineNode *node = (EngineNode*) walk->data;
      assert_return (node->sched_recurse_tag == FALSE, NULL);
    }
  /* tag all nodes in ring1 first */
  for (walk = ring1; walk; walk = sfi_ring_walk (walk, ring1))
    {
      EngineNode *node = (EngineNode*) walk->data;
      assert_return (node->sched_recurse_tag == FALSE, NULL);	/* paranoid check */
      node->sched_recurse_tag = TRUE;
    }
  /* merge list with missing (untagged) nodes */
  for (walk = ring2; walk; walk = sfi_ring_walk (walk, ring2))
    {
      EngineNode *node = (EngineNode*) walk->data;
      if (node->sched_recurse_tag == FALSE)
	ring1 = sfi_ring_append (ring1, node);
    }
  /* untag all nodes */
  for (walk = ring1; walk; walk = sfi_ring_walk (walk, ring1))
    {
      EngineNode *node = (EngineNode*) walk->data;
      node->sched_recurse_tag = FALSE;
    }
  for (walk = ring2; walk; walk = sfi_ring_walk (walk, ring2))
    {
      EngineNode *node = (EngineNode*) walk->data;
      node->sched_recurse_tag = FALSE;
    }
  sfi_ring_free (ring2);
  return ring1;
}

static gboolean
resolve_cycle (EngineCycle *cycle,
	       EngineNode  *node,
	       SfiRing    **cycle_nodes_p)
{
  if (node != cycle->last)
    return FALSE;
  if (!cycle->seen_deferred_node)
    {
      g_error ("cycle without delay module: (%p)", cycle);
    }
  *cycle_nodes_p = merge_untagged_node_lists_uniq (*cycle_nodes_p, cycle->nodes);
  cycle->nodes = NULL;
  cycle->last = NULL;
  return TRUE;
}

static gboolean
master_resolve_cycles (EngineQuery *query,
		       EngineNode  *node)
{
  SfiRing *walk;
  gboolean all_resolved = TRUE;
  assert_return (query->cycles != NULL, FALSE);	/* paranoid */
  walk = query->cycles;
  while (walk)
    {
      SfiRing *next = sfi_ring_walk (walk, query->cycles);
      EngineCycle *cycle = (EngineCycle*) walk->data;
      if (resolve_cycle (cycle, node, &query->cycle_nodes))
	{
	  assert_return (cycle->last == NULL, FALSE);	/* paranoid */
	  assert_return (cycle->nodes == NULL, FALSE);	/* paranoid */
	  sfi_delete_struct (EngineCycle, cycle);
	  query->cycles = sfi_ring_remove_node (query->cycles, walk);
	}
      else
	all_resolved = FALSE;
      walk = next;
    }
  if (all_resolved)
    assert_return (query->cycles == NULL, FALSE);	/* paranoid */
  return all_resolved;
}

static void
query_add_cycle (EngineQuery *query,
		 EngineNode  *dep,
		 EngineNode  *node)
{
  EngineCycle *cycle = sfi_new_struct0 (EngineCycle, 1);

  cycle->last = dep;
  cycle->nodes = sfi_ring_prepend (NULL, node);
  cycle->seen_deferred_node = ENGINE_NODE_IS_DEFERRED (node); /* dep will be checked when added to nodes */
  query->cycles = sfi_ring_append (query->cycles, cycle);
}

static void
query_merge_cycles (EngineQuery *query,
		    EngineQuery *child_query,
		    EngineNode  *node)
{
  SfiRing *walk;
  assert_return (child_query->cycles != NULL);	/* paranoid */
  /* add node to all child cycles */
  for (walk = child_query->cycles; walk; walk = sfi_ring_walk (walk, child_query->cycles))
    {
      EngineCycle *cycle = (EngineCycle*) walk->data;
      cycle->nodes = sfi_ring_prepend (cycle->nodes, node);
      cycle->seen_deferred_node |= ENGINE_NODE_IS_DEFERRED (node);
    }
  /* merge child cycles into our cycle list */
  query->cycles = sfi_ring_concat (query->cycles, child_query->cycles);
  child_query->cycles = NULL;
  /* merge child's cycle nodes from resolved cycles into ours */
  query->cycle_nodes = merge_untagged_node_lists_uniq (query->cycle_nodes, child_query->cycle_nodes);
  child_query->cycle_nodes = NULL;
}

static inline void
clean_ostreams (EngineNode *node)
{
  if (!node->cleared_ostreams && !ENGINE_NODE_IS_SCHEDULED (node))
    {
      guint i;

      for (i = 0; i < ENGINE_NODE_N_OSTREAMS (node); i++)
	node->module.ostreams[i].connected = FALSE;
      node->cleared_ostreams = TRUE;
    }
}

static inline void
subschedule_trace_virtual_input (EngineSchedule *schedule,
                                 EngineNode     *node,
                                 guint           istream)
{
  if (!ENGINE_NODE_IS_SCHEDULED (node))
    schedule_virtual (schedule, node);
  EngineInput *input = node->inputs + istream;
  if (input->src_node && ENGINE_NODE_IS_VIRTUAL (input->src_node))
    {
      subschedule_trace_virtual_input (schedule, input->src_node, input->src_stream);
      EngineInput *src_input = input->src_node->inputs + input->src_stream;
      input->real_node = src_input->real_node;
      input->real_stream = src_input->real_stream;
    }
  else
    {
      input->real_node = input->src_node;
      input->real_stream = input->src_stream;
    }
}

static inline EngineNode*
subschedule_skip_virtuals (EngineSchedule *schedule,
			   EngineNode     *node,
			   guint          *ostream_p)
{
  if (node && ENGINE_NODE_IS_VIRTUAL (node))
    {
      subschedule_trace_virtual_input (schedule, node, *ostream_p);
      EngineInput *input = node->inputs + *ostream_p;
      *ostream_p = input->real_stream;
      node = input->real_node;
    }
  return node;
}

static inline void
subschedule_child (EngineSchedule *schedule,
		   EngineNode     *node,
		   EngineQuery    *query,
		   EngineNode     *child,
		   guint           child_ostream)
{
  assert_return (ENGINE_NODE_IS_VIRTUAL (node) == FALSE);

  /* flag connected ostream */
  clean_ostreams (child);
  child->module.ostreams[child_ostream].connected = TRUE;

  /* schedule away if necessary */
  if (ENGINE_NODE_IS_SCHEDULED (child))
    query->leaf_level = MAX (query->leaf_level, child->sched_leaf_level + 1);
  else if (child->sched_recurse_tag)	/* cycle */
    query_add_cycle (query, child, node);
  else			/* nice boy ;) */
    {
      EngineQuery child_query = { 0, };

      subschedule_query_node (schedule, child, &child_query);
      query->leaf_level = MAX (query->leaf_level, child_query.leaf_level + 1);
      if (!child_query.cycles)
	{
	  assert_return (child_query.cycle_nodes == NULL);	/* paranoid */
	  schedule_node (schedule, child, child_query.leaf_level);
	}
      else if (master_resolve_cycles (&child_query, child))
	{
	  assert_return (child == child_query.cycle_nodes->data);	/* paranoid */
	  schedule_cycle (schedule, child_query.cycle_nodes, child_query.leaf_level);
	  child_query.cycle_nodes = NULL;
	}
      else
	query_merge_cycles (query, &child_query, node);
      assert_return (child_query.cycles == NULL && child_query.cycle_nodes == NULL);	/* paranoid */
    }
}

static void
subschedule_query_node (EngineSchedule *schedule,
			EngineNode     *node,
			EngineQuery    *query)
{
  guint i, j;

  assert_return (ENGINE_NODE_IS_VIRTUAL (node) == FALSE);
  assert_return (node->sched_recurse_tag == FALSE);
  assert_return (query->leaf_level == 0);

  /* update suspension state if necessary */
  update_suspension_state (node);
  /* reset ostream[].connected flags */
  clean_ostreams (node);

  /* SCHED_DEBUG ("sched_query(%p)", node); */
  node->sched_recurse_tag = TRUE;
  /* schedule input stream children */
  for (i = 0; i < ENGINE_NODE_N_ISTREAMS (node); i++)
    {
      EngineNode *child = node->inputs[i].src_node;
      guint child_ostream = node->inputs[i].src_stream;
      child = subschedule_skip_virtuals (schedule, child, &child_ostream);
      if (!child)
	{
	  node->module.istreams[i].connected = FALSE;
	  node->inputs[i].real_node = NULL;
	}
      else
	{
	  node->module.istreams[i].connected = TRUE;
	  node->inputs[i].real_node = child;
	  node->inputs[i].real_stream = child_ostream;
	  subschedule_child (schedule, node, query, child, child_ostream);
	}
    }
  /* eliminate dead virtual ends in jstreams */
  for (j = 0; j < ENGINE_NODE_N_JSTREAMS (node); j++)
    {
      BseJStream *jstream = node->module.jstreams + j;

      /* we check this jstream's connections for virtual dead-ends.
       * valid connections stay at (are moved to) the array front and
       * are counted in n_connections, while dead-ends are moved
       * to the array end and are counted in i. jcount is the number
       * of total connections for this joint stream.
       */
      jstream->n_connections = i = 0;
      while (jstream->n_connections + i < jstream->jcount)
	{
	  EngineJInput *tmp, *jinput = node->jinputs[j] + jstream->n_connections;
	  EngineNode *child = jinput->src_node;
	  guint child_ostream = jinput->src_stream;
	  child = subschedule_skip_virtuals (schedule, child, &child_ostream);
	  if (child)
	    {
	      jstream->n_connections++;		/* valid input */
	      jinput->real_node = child;
	      jinput->real_stream = child_ostream;
	    }
	  else
	    {
	      i++;				/* virtual dead-end */
	      /* swap dead-end out of the way */
	      tmp = node->jinputs[j] + jstream->jcount - i;
	      if (jinput != tmp)
		{
		  child = tmp->src_node;
		  child_ostream = tmp->src_stream;
		  tmp->src_node = jinput->src_node;
		  tmp->src_stream = jinput->src_stream;
		  jinput->src_node = child;
		  jinput->src_stream = child_ostream;
		}
	    }
	}
    }
  /* schedule valid jstream connections */
  for (j = 0; j < ENGINE_NODE_N_JSTREAMS (node); j++)
    for (i = 0; i < node->module.jstreams[j].n_connections; i++)
      {
	EngineNode *child = node->jinputs[j][i].real_node;
	guint child_ostream = node->jinputs[j][i].real_stream;

	subschedule_child (schedule, node, query, child, child_ostream);
      }
  node->counter = Bse::TickStamp::current();
  node->sched_recurse_tag = FALSE;
  /* SCHED_DEBUG ("sched_done(%p)", node); */
}
