/* GSL Engine - Flow module operation engine
 * Copyright (C) 2001 Tim Janik
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
#include "gslopschedule.h"


#include "gslcommon.h"	


/* --- functions --- */
OpSchedule*
_op_schedule_new (void)
{
  OpSchedule *sched = gsl_new_struct0 (OpSchedule, 1);

  sched->n_items = 0;
  sched->leaf_levels = 0;
  sched->nodes = NULL;
  sched->cycles = NULL;
  sched->secured = FALSE;
  sched->in_pqueue = FALSE;
  sched->cur_leaf_level = ~0;
  sched->cur_node = NULL;
  sched->cur_cycle = NULL;
  
  return sched;
}

static inline void
unschedule_node (OpSchedule *sched,
		 OpNode     *node)
{
  guint leaf_level;

  g_return_if_fail (OP_NODE_IS_SCHEDULED (node) == TRUE);
  leaf_level = node->sched_leaf_level;
  g_return_if_fail (leaf_level <= sched->leaf_levels);
  g_return_if_fail (sched->n_items > 0);
  
  OP_DEBUG (SCHED, "unschedule_node(%p,%u)", node, leaf_level);
  sched->nodes[leaf_level] = gsl_ring_remove (sched->nodes[leaf_level], node);
  node->sched_tag = FALSE;
  node->sched_leaf_level = 0;
  sched->n_items--;
}

static inline void
unschedule_cycle (OpSchedule *sched,
		  GslRing     *ring)
{
  guint leaf_level;
  GslRing *walk;

  g_return_if_fail (OP_NODE_IS_SCHEDULED (OP_NODE (ring->data)) == TRUE);
  leaf_level = OP_NODE (ring->data)->sched_leaf_level;
  g_return_if_fail (leaf_level <= sched->leaf_levels);
  g_return_if_fail (sched->n_items > 0);

  OP_DEBUG (SCHED, "unschedule_cycle(%p,%u,%p)", ring->data, leaf_level, ring);
  sched->nodes[leaf_level] = gsl_ring_remove (sched->nodes[leaf_level], ring);
  for (walk = ring; walk; walk = gsl_ring_walk (ring, walk))
    {
      OpNode *node = walk->data;

      if (!OP_NODE_IS_SCHEDULED (node))
	g_warning ("node(%p) in schedule ring(%p) is untagged", node, ring);
      node->sched_tag = FALSE;
      node->sched_leaf_level = 0;
    }
  sched->n_items--;
}

static void
_op_schedule_debug_dump (OpSchedule *sched)
{
  g_print ("sched(%p) = {\n", sched);
  if (sched)
    {
      guint i;

      g_print ("  n_items=%u, leaf_levels=%u, secured=%u,\n",
	       sched->n_items, sched->leaf_levels, sched->secured);
      g_print ("  in_pqueue=%u, cur_leaf_level=%u,\n",
	       sched->in_pqueue, sched->cur_leaf_level);
      g_print ("  cur_node=%p, cur_cycle=%p,\n",
	       sched->cur_node, sched->cur_cycle);
      for (i = 0; i < sched->leaf_levels; i++)
	{
	  GslRing *ring, *head = sched->nodes[i];

	  if (!head)
	    continue;
	  g_print ("  { leaf_level=%u:", i);
	  for (ring = head; ring; ring = gsl_ring_walk (head, ring))
	    g_print (" node(%p(tag:%u))", ring->data, ((OpNode*) ring->data)->sched_tag);
	  g_print (" },\n");
	}
    }
  g_print ("};\n");
}


void
_op_schedule_clear (OpSchedule *sched)
{
  guint i;

  g_return_if_fail (sched != NULL);
  g_return_if_fail (sched->secured == FALSE);
  g_return_if_fail (sched->in_pqueue == FALSE);

  _op_schedule_debug_dump (sched);

  for (i = 0; i < sched->leaf_levels; i++)
    {
      while (sched->nodes[i])
	unschedule_node (sched, sched->nodes[i]->data);
      while (sched->cycles[i])
	unschedule_cycle (sched, sched->cycles[i]->data);
    }
  g_return_if_fail (sched->n_items == 0);
}

void
_op_schedule_destroy (OpSchedule *sched)
{
  g_return_if_fail (sched != NULL);
  g_return_if_fail (sched->secured == FALSE);
  g_return_if_fail (sched->in_pqueue == FALSE);

  _op_schedule_clear (sched);
  g_free (sched->nodes);
  g_free (sched->cycles);
  gsl_delete_struct (OpSchedule, 1, sched);
}

static void
_op_schedule_grow (OpSchedule *sched,
		  guint       leaf_level)
{
  guint ll = 1 << g_bit_storage (leaf_level);	/* power2 growth alignment, ll >= leaf_level+1 */
  
  if (sched->leaf_levels < ll)
    {
      guint i = sched->leaf_levels;

      sched->leaf_levels = ll;
      sched->nodes = g_renew (GslRing*, sched->nodes, sched->leaf_levels);
      sched->cycles = g_renew (GslRing*, sched->cycles, sched->leaf_levels);
      for (; i < sched->leaf_levels; i++)
	{
	  sched->nodes[i] = NULL;
	  sched->cycles[i] = NULL;
	}
    }
}

void
_op_schedule_node (OpSchedule *sched,
		  OpNode     *node,
		  guint       leaf_level)
{
  g_return_if_fail (sched != NULL);
  g_return_if_fail (sched->secured == FALSE);
  g_return_if_fail (node != NULL);
  g_return_if_fail (!OP_NODE_IS_SCHEDULED (node));

  OP_DEBUG (SCHED, "schedule_node(%p,%u)", node, leaf_level);
  node->sched_tag = TRUE;
  node->sched_leaf_level = leaf_level;
  _op_schedule_grow (sched, leaf_level);
  /* could do 3-stage scheduling by expensiveness */
  sched->nodes[leaf_level] = (OP_NODE_IS_EXPENSIVE (node) ? gsl_ring_prepend : gsl_ring_append) (sched->nodes[leaf_level], node);
  sched->n_items++;
}

void
_op_schedule_cycle (OpSchedule *sched,
		   GslRing     *cycle_nodes,
		   guint       leaf_level)
{
  GslRing *walk;

  g_return_if_fail (sched != NULL);
  g_return_if_fail (sched->secured == FALSE);
  g_return_if_fail (cycle_nodes != NULL);

  for (walk = cycle_nodes; walk; walk = gsl_ring_walk (cycle_nodes, walk))
    {
      OpNode *node = walk->data;

      g_return_if_fail (!OP_NODE_IS_SCHEDULED (node));
      node->sched_tag = TRUE;
      node->sched_leaf_level = leaf_level;
    }
  _op_schedule_grow (sched, leaf_level);
  sched->cycles[leaf_level] = gsl_ring_prepend (sched->cycles[leaf_level], cycle_nodes);
  sched->n_items++;
}

void
_op_schedule_restart (OpSchedule *sched)
{
  g_return_if_fail (sched != NULL);
  g_return_if_fail (sched->secured == TRUE);
  g_return_if_fail (sched->cur_leaf_level == sched->leaf_levels);
  g_return_if_fail (sched->cur_node == NULL);
  g_return_if_fail (sched->cur_cycle == NULL);

  sched->cur_leaf_level = 0;
  if (sched->leaf_levels > 0)
    {
      sched->cur_node = sched->nodes[0];
      sched->cur_cycle = sched->cycles[0];
    }
}

void
_op_schedule_secure (OpSchedule *sched)
{
  g_return_if_fail (sched != NULL);
  g_return_if_fail (sched->secured == FALSE);

  sched->secured = TRUE;
  sched->cur_leaf_level = sched->leaf_levels;
}

static void
schedule_advance (OpSchedule *sched)
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

OpNode*
_op_schedule_pop_node (OpSchedule *sched)
{
  g_return_val_if_fail (sched != NULL, NULL);
  g_return_val_if_fail (sched->secured == TRUE, NULL);
  g_return_val_if_fail (sched->cur_leaf_level <= sched->leaf_levels, NULL);

  do
    {
      guint leaf_level = sched->cur_leaf_level;

      if (sched->cur_node)
	{
	  OpNode *node = sched->cur_node->data;
	  
	  sched->cur_node = gsl_ring_walk (sched->nodes[leaf_level], sched->cur_node);
	  return node;
	}
      schedule_advance (sched);
    }
  while (sched->cur_node);

  /* nothing to hand out, either we're empty or still have cycles pending */
  return NULL;
}

GslRing*
_op_schedule_pop_cycle (OpSchedule *sched)
{
  g_return_val_if_fail (sched != NULL, NULL);
  g_return_val_if_fail (sched->secured == TRUE, NULL);
  g_return_val_if_fail (sched->cur_leaf_level <= sched->leaf_levels, NULL);

  do
    {
      guint leaf_level = sched->cur_leaf_level;

      if (sched->cur_cycle)
	{
	  GslRing *cycle = sched->cur_cycle->data;
	  
	  sched->cur_cycle = gsl_ring_walk (sched->cycles[leaf_level], sched->cur_cycle);
	  return cycle;
	}
      schedule_advance (sched);
    }
  while (sched->cur_cycle);

  /* nothing to hand out, either we're empty or still have nodes pending */
  return NULL;
}

void
_op_schedule_unsecure (OpSchedule *sched)
{
  g_return_if_fail (sched != NULL);
  g_return_if_fail (sched->secured == TRUE);
  g_return_if_fail (sched->in_pqueue == FALSE);
  g_return_if_fail (sched->cur_leaf_level == sched->leaf_levels);
  g_return_if_fail (sched->cur_node == NULL);
  g_return_if_fail (sched->cur_cycle == NULL);

  sched->secured = FALSE;
  sched->cur_leaf_level = ~0;
}


/* --- depth scheduling --- */
static GslRing*
merge_untagged_node_lists_uniq (GslRing *ring1,
				GslRing *ring2)
{
  GslRing *walk;

  /* paranoid, ensure all nodes are untagged */
  for (walk = ring2; walk; walk = gsl_ring_walk (ring2, walk))
    {
      OpNode *node = walk->data;

      g_assert (node->sched_router_tag == FALSE);
    }

  /* tag all nodes in list first */
  for (walk = ring1; walk; walk = gsl_ring_walk (ring1, walk))
    {
      OpNode *node = walk->data;

      g_assert (node->sched_router_tag == FALSE);	/* paranoid check */
      node->sched_router_tag = TRUE;
    }

  /* merge list with missing (untagged) nodes */
  for (walk = ring2; walk; walk = gsl_ring_walk (ring2, walk))
    {
      OpNode *node = walk->data;

      if (node->sched_router_tag == FALSE)
	ring1 = gsl_ring_append (ring1, node);
    }

  /* untag all nodes */
  for (walk = ring1; walk; walk = gsl_ring_walk (ring1, walk))
    {
      OpNode *node = walk->data;

      node->sched_router_tag = FALSE;
    }
  for (walk = ring2; walk; walk = gsl_ring_walk (ring2, walk))
    {
      OpNode *node = walk->data;

      node->sched_router_tag = FALSE;
    }
  gsl_ring_free (ring2);
  return ring1;
}

static gboolean
resolve_cycle (OpCycle *cycle,
	       OpNode  *node,
	       GslRing **cycle_nodes_p)
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
master_resolve_cycles (OpQuery *query,
		       OpNode  *node)
{
  GslRing *walk;
  gboolean all_resolved = TRUE;

  g_assert (query->cycles != NULL);	/* paranoid */

  walk = query->cycles;
  while (walk)
    {
      GslRing *next = gsl_ring_walk (query->cycles, walk);
      OpCycle *cycle = walk->data;
      
      if (resolve_cycle (cycle, node, &query->cycle_nodes))
	{
	  g_assert (cycle->last == NULL);	/* paranoid */
	  g_assert (cycle->nodes == NULL);	/* paranoid */

	  gsl_delete_struct (OpCycle, 1, cycle);
	  query->cycles = gsl_ring_remove_node (query->cycles, walk);
	}
      else
	all_resolved = FALSE;
      walk = next;
    }
  if (all_resolved)
    g_assert (query->cycles == NULL);	/* paranoid */
  return all_resolved;
}

static void
query_add_cycle (OpQuery *query,
		 OpNode  *dep,
		 OpNode  *node)
{
  OpCycle *cycle = gsl_new_struct0 (OpCycle, 1);

  cycle->last = dep;
  cycle->nodes = gsl_ring_prepend (NULL, node);
  cycle->seen_deferred_node = OP_NODE_IS_DEFERRED (node); /* dep will be checked when added to nodes */
  query->cycles = gsl_ring_append (query->cycles, cycle);
}

static void
query_merge_cycles (OpQuery *query,
		    OpQuery *child_query,
		    OpNode  *node)
{
  GslRing *walk;

  g_assert (child_query->cycles != NULL);	/* paranoid */

  /* add node to all child cycles */
  for (walk = child_query->cycles; walk; walk = gsl_ring_walk (child_query->cycles, walk))
    {
      OpCycle *cycle = walk->data;
      
      cycle->nodes = gsl_ring_prepend (cycle->nodes, node);
      cycle->seen_deferred_node |= OP_NODE_IS_DEFERRED (node);
    }

  /* merge child cycles into ours */
  query->cycles = gsl_ring_concat (query->cycles, child_query->cycles);
  child_query->cycles = NULL;

  /* merge childs cycle nodes from resolved cycles into ours */
  query->cycle_nodes = merge_untagged_node_lists_uniq (query->cycle_nodes, child_query->cycle_nodes);
  child_query->cycle_nodes = NULL;
}

static void
subschedule_query_node (OpSchedule *schedule,
			OpNode	   *node,
			OpQuery	   *query)
{
  guint i, leaf_level = 0;

  g_return_if_fail (node->sched_router_tag == FALSE);

  OP_DEBUG (SCHED, "start_query(%p)", node);
  node->sched_router_tag = TRUE;
  for (i = 0; i < OP_NODE_N_ISTREAMS (node); i++)
    {
      OpNode *child = node->inputs[i].src_node;

      if (!child)
	continue;
      else if (OP_NODE_IS_SCHEDULED (child))
	{
	  leaf_level = MAX (leaf_level, child->sched_leaf_level + 1);
	  continue;
	}
      else if (child->sched_router_tag)	/* cycle */
	{
	  query_add_cycle (query, child, node);
	}
      else			/* nice boy ;) */
	{
	  OpQuery child_query = { 0, };

	  subschedule_query_node (schedule, child, &child_query);
	  leaf_level = MAX (leaf_level, child_query.leaf_level + 1);
	  if (!child_query.cycles)
	    {
	      g_assert (child_query.cycle_nodes == NULL);	/* paranoid */
	      _op_schedule_node (schedule, child, child_query.leaf_level);
	    }
	  else if (master_resolve_cycles (&child_query, child))
	    {
	      g_assert (child == child_query.cycle_nodes->data);	/* paranoid */
	      _op_schedule_cycle (schedule, child_query.cycle_nodes, child_query.leaf_level);
	      child_query.cycle_nodes = NULL;
	    }
	  else
	    query_merge_cycles (query, &child_query, node);
	  g_assert (child_query.cycles == NULL);	/* paranoid */
	  g_assert (child_query.cycle_nodes == NULL);	/* paranoid */
	}
    }
  query->leaf_level = leaf_level;
  node->counter = gsl_engine_last_counter ();
  node->sched_router_tag = FALSE;
  OP_DEBUG (SCHED, "end_query(%p)", node);
}

void
_op_schedule_consumer_node (OpSchedule *schedule,
			   OpNode     *node)
{
  OpQuery query = { 0, };

  g_return_if_fail (schedule != NULL);
  g_return_if_fail (schedule->secured == FALSE);
  g_return_if_fail (node != NULL);
  g_return_if_fail (OP_NODE_IS_CONSUMER (node));

  subschedule_query_node (schedule, node, &query);
  g_assert (query.cycles == NULL);	/* paranoid */
  g_assert (query.cycle_nodes == NULL);	/* paranoid */
  _op_schedule_node (schedule, node, query.leaf_level);
}
