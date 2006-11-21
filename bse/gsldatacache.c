/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2004 Tim Janik
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
#include "gsldatacache.h"
#include "bsemain.h"
#include "gslcommon.h"
#include "gsldatahandle.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>


/* --- macros --- */
#define	NODEP_INDEX(dcache, node_p)	((node_p) - (dcache)->nodes)
#define	UPPER_POWER2(n)			(sfi_alloc_upper_power2 (MAX (n, 4)))
#define	CONFIG_NODE_SIZE()		(BSE_CONFIG (dcache_block_size))
#define	AGE_EPSILON			(3)	/* must be < resident set */
#define	LOW_PERSISTENCY_RESIDENT_SET    (5)

/* we use one global lock to protect the dcache list, the list
 * count (length) and the number of aged (unused) nodes.
 * also, each dcache has its own mutext to protect updates in
 * the reference count, nodes or node data blocks.
 * in order to avoid deadlocks, if both locks need
 * to be held, they always have to be acquired in the order
 * 1) global lock, 2) dcache lock.
 * asyncronous data block filling for a new node occours without
 * the dcache lock being held (as most calls to GslDataHandle
 * functions).
 * however, assignment of the newly acquired data is again
 * protected by the dcache lock. concurrent API entries
 * which require demand loading of such data will wait on
 * a global condition which is always signaled once a new data
 * block read has been completed. using one global condition
 * is considered sufficient until shown otherwise by further
 * profiling/debugging measures.
 */


/* --- prototypes --- */
static void			dcache_free		(GslDataCache	*dcache);
static GslDataCacheNode*	data_cache_new_node_L	(GslDataCache	*dcache,
							 int64		 offset,
							 guint		 pos,
							 gboolean	 demand_load);


/* --- variables --- */
static BirnetMutex	   global_dcache_mutex = { 0, };
static BirnetCond	   global_dcache_cond_node_filled = { 0, };
static SfiRing	  *global_dcache_list = NULL;
static guint	   global_dcache_count = 0;
static guint       global_dcache_n_aged_nodes = 0;


/* --- functions --- */
void
_gsl_init_data_caches (void)
{
  static gboolean initialized = FALSE;

  g_assert (initialized == FALSE);
  initialized++;

  BIRNET_STATIC_ASSERT (AGE_EPSILON < LOW_PERSISTENCY_RESIDENT_SET);
  sfi_cond_init (&global_dcache_cond_node_filled);
  sfi_mutex_init (&global_dcache_mutex);
}

GslDataCache*
gsl_data_cache_new (GslDataHandle *dhandle,
		    guint	   padding)
{
  guint node_size = CONFIG_NODE_SIZE () / sizeof (GslDataType);
  GslDataCache *dcache;

  g_return_val_if_fail (dhandle != NULL, NULL);
  g_return_val_if_fail (padding > 0, NULL);
  g_return_val_if_fail (dhandle->name != NULL, NULL);
  g_assert (node_size == sfi_alloc_upper_power2 (node_size));
  g_return_val_if_fail (padding < node_size / 2, NULL);

  /* allocate new closed dcache if necessary */
  dcache = sfi_new_struct (GslDataCache, 1);
  dcache->dhandle = gsl_data_handle_ref (dhandle);
  dcache->open_count = 0;
  sfi_mutex_init (&dcache->mutex);
  dcache->ref_count = 1;
  dcache->node_size = node_size;
  dcache->padding = padding;
  dcache->max_age = 0;
  dcache->high_persistency = FALSE;
  dcache->n_nodes = 0;
  dcache->nodes = g_renew (GslDataCacheNode*, NULL, UPPER_POWER2 (dcache->n_nodes));

  GSL_SPIN_LOCK (&global_dcache_mutex);
  global_dcache_list = sfi_ring_append (global_dcache_list, dcache);
  global_dcache_count++;
  GSL_SPIN_UNLOCK (&global_dcache_mutex);

  return dcache;
}

void
gsl_data_cache_open (GslDataCache *dcache)
{
  g_return_if_fail (dcache != NULL);
  g_return_if_fail (dcache->ref_count > 0);

  GSL_SPIN_LOCK (&dcache->mutex);
  if (!dcache->open_count)
    {
      BseErrorType error;

      error = gsl_data_handle_open (dcache->dhandle);
      if (error)
	{
	  /* FIXME: this is pretty fatal, throw out zero blocks now? */
	  sfi_diag ("%s: failed to open \"%s\": %s", G_STRLOC, dcache->dhandle->name, bse_error_blurb (error));
	}
      else
	{
          dcache->high_persistency = gsl_data_handle_needs_cache (dcache->dhandle);
	  dcache->open_count = 1;
	  dcache->ref_count++;
	}
    }
  else
    dcache->open_count++;
  GSL_SPIN_UNLOCK (&dcache->mutex);
}

void
gsl_data_cache_close (GslDataCache *dcache)
{
  gboolean need_unref;

  g_return_if_fail (dcache != NULL);
  g_return_if_fail (dcache->ref_count > 0);
  g_return_if_fail (dcache->open_count > 0);

  GSL_SPIN_LOCK (&dcache->mutex);
  dcache->open_count--;
  need_unref = !dcache->open_count;
  if (!dcache->open_count)
    {
      dcache->high_persistency = FALSE;
      gsl_data_handle_close (dcache->dhandle);
    }
  GSL_SPIN_UNLOCK (&dcache->mutex);
  if (need_unref)
    gsl_data_cache_unref (dcache);
}

GslDataCache*
gsl_data_cache_ref (GslDataCache *dcache)
{
  g_return_val_if_fail (dcache != NULL, NULL);
  g_return_val_if_fail (dcache->ref_count > 0, NULL);

  /* we might get invoked with global_dcache_mutex locked */
  GSL_SPIN_LOCK (&dcache->mutex);
  dcache->ref_count++;
  GSL_SPIN_UNLOCK (&dcache->mutex);

  return dcache;
}

static void
dcache_free (GslDataCache *dcache)
{
  guint i;

  g_return_if_fail (dcache->ref_count == 0);
  g_return_if_fail (dcache->open_count == 0);

  gsl_data_handle_unref (dcache->dhandle);
  sfi_mutex_destroy (&dcache->mutex);
  for (i = 0; i < dcache->n_nodes; i++)
    {
      GslDataCacheNode *node = dcache->nodes[i];
      guint size;

      size = dcache->node_size + (dcache->padding << 1);
      sfi_delete_structs (GslDataType, size, node->data - dcache->padding);
      sfi_delete_struct (GslDataCacheNode, node);
    }
  g_free (dcache->nodes);
  sfi_delete_struct (GslDataCache, dcache);
}

void
gsl_data_cache_unref (GslDataCache *dcache)
{
  g_return_if_fail (dcache != NULL);
 restart:
  g_return_if_fail (dcache->ref_count > 0);

  if (dcache->ref_count == 1)	/* possible destruction, need global lock */
    {
      g_return_if_fail (dcache->open_count == 0);

      GSL_SPIN_LOCK (&global_dcache_mutex);
      GSL_SPIN_LOCK (&dcache->mutex);
      if (dcache->ref_count != 1)
	{
	  /* damn, some other thread trapped in, restart */
	  GSL_SPIN_UNLOCK (&dcache->mutex);
	  GSL_SPIN_UNLOCK (&global_dcache_mutex);
	  goto restart;
	}
      dcache->ref_count = 0;
      global_dcache_list = sfi_ring_remove (global_dcache_list, dcache);
      GSL_SPIN_UNLOCK (&dcache->mutex);
      global_dcache_count--;
      global_dcache_n_aged_nodes -= dcache->n_nodes;
      GSL_SPIN_UNLOCK (&global_dcache_mutex);
      dcache_free (dcache);
    }
  else
    {
      GSL_SPIN_LOCK (&dcache->mutex);
      if (dcache->ref_count < 2)
	{
	  /* damn, some other thread trapped in, restart */
	  GSL_SPIN_UNLOCK (&dcache->mutex);
	  goto restart;
	}
      dcache->ref_count--;
      GSL_SPIN_UNLOCK (&dcache->mutex);
    }
}

static inline GslDataCacheNode**
data_cache_lookup_nextmost_node_L (GslDataCache *dcache,
				   int64         offset)
{
  if (dcache->n_nodes > 0)
    {
      GslDataCacheNode **check, **nodes = dcache->nodes;
      guint n_nodes = dcache->n_nodes, node_size = dcache->node_size;

      /* caller has to figure himself whether we return nextmost vs. exact match */
      nodes -= 1;
      do
	{
	  register gint cmp;
	  register guint i;
	  
	  i = (n_nodes + 1) >> 1;
	  check = nodes + i;
	  cmp = offset < (*check)->offset ? -1 : offset >= (*check)->offset + node_size;
	  if (cmp == 0)
	    return check;	/* exact match */
	  else if (cmp > 0)
	    {
	      n_nodes -= i;
	      nodes = check;
	    }
	  else /* if (cmp < 0) */
	    n_nodes = i - 1;
	}
      while (n_nodes);
      
      return check; /* nextmost */
    }
  return NULL;
}

static inline GslDataCacheNode*
data_cache_new_node_L (GslDataCache *dcache,
		       int64	     offset,
		       guint	     pos,
		       gboolean	     demand_load)
{
  GslDataCacheNode **node_p, *dnode;
  GslDataType *data, *node_data;
  guint new_node_array_size, old_node_array_size = UPPER_POWER2 (dcache->n_nodes);
  int64 dhandle_length;
  guint i, size;
  gint result;

  i = dcache->n_nodes++;
  new_node_array_size = UPPER_POWER2 (dcache->n_nodes);
  if (old_node_array_size != new_node_array_size)
    dcache->nodes = g_renew (GslDataCacheNode*, dcache->nodes, new_node_array_size);
  node_p = dcache->nodes + pos;
  g_memmove (node_p + 1, node_p, (i - pos) * sizeof (*node_p));
  dnode = sfi_new_struct (GslDataCacheNode, 1);
  (*node_p) = dnode;
  dnode->offset = offset & ~(dcache->node_size - 1);
  dnode->ref_count = 1;
  dnode->age = 0;
  dnode->data = NULL;
  GSL_SPIN_UNLOCK (&dcache->mutex);

  size = dcache->node_size + (dcache->padding << 1);
  data = sfi_new_struct (GslDataType, size);
  node_data = data + dcache->padding;
  offset = dnode->offset;
  if (dcache->padding > offset)		/* pad out bytes before data start */
    {
      guint short_pad = dcache->padding - offset;

      memset (data, 0, short_pad * sizeof (GslDataType));
      size -= short_pad;
      data += short_pad;
      offset -= (dcache->padding - short_pad); /* should always result in offset=0 */
    }
  else
    offset -= dcache->padding;
  if (!demand_load)
    g_message (G_STRLOC ":FIXME: lazy data loading not yet supported");
  
  /* copy over data from previous node */
  GslDataCacheNode *prev_node = pos ? dcache->nodes[pos - 1] : NULL;
  if (prev_node)
    {
      int64 prev_node_size = dcache->node_size;
      int64 prev_node_offset = prev_node->offset;
      GslDataType *prev_node_data = prev_node->data;
      
      /* padding around prev_node */
      prev_node_size += dcache->padding << 1;
      prev_node_offset -= dcache->padding;
      prev_node_data -= dcache->padding;
      
      /* check for overlap */
      if (offset < prev_node_offset + prev_node_size)
        {
          int64 overlap = prev_node_offset + prev_node_size - offset;
          memcpy (data, prev_node_data + offset - prev_node_offset, overlap * sizeof (data[0]));
          size -= overlap;
          offset += overlap;
          data += overlap;
        }
    }

  /* fill from data handle */
  dhandle_length = gsl_data_handle_length (dcache->dhandle);
  do
    {
      if (offset >= dhandle_length)
	break;
      size = MIN (size, dhandle_length - offset);
      result = gsl_data_handle_read (dcache->dhandle, offset, size, data);
      if (result < 0)
	{
          sfi_diag ("ReadAhead: failed to read from \"%s\"", dcache->dhandle->name);
	  break;
	}
      else
	{
	  offset += result;
	  size -= result;
	  data += result;
	}
    }
  while (size && result > 0);
  memset (data, 0, size * sizeof (data[0]));

  GSL_SPIN_LOCK (&dcache->mutex);
  dnode->data = node_data;
  sfi_cond_broadcast (&global_dcache_cond_node_filled);
  
  return dnode;
}

GslDataCacheNode*
gsl_data_cache_ref_node (GslDataCache       *dcache,
			 int64               offset,
			 GslDataCacheRequest load_request)
{
  GslDataCacheNode **node_p, *node;
  guint insertion_pos;

  g_return_val_if_fail (dcache != NULL, NULL);
  g_return_val_if_fail (dcache->ref_count > 0, NULL);
  g_return_val_if_fail (dcache->open_count > 0, NULL);
  g_return_val_if_fail (offset < gsl_data_handle_length (dcache->dhandle), NULL);

  GSL_SPIN_LOCK (&dcache->mutex);
  node_p = data_cache_lookup_nextmost_node_L (dcache, offset);
  if (node_p)
    {
      node = *node_p;
      if (offset >= node->offset && offset < node->offset + dcache->node_size)
	{
	  gboolean rejuvenate_node = !node->ref_count;

	  if (load_request == GSL_DATA_CACHE_PEEK)
	    {
	      if (node->data)
		node->ref_count++;
	      else
		node = NULL;
	      GSL_SPIN_UNLOCK (&dcache->mutex);
	      if (node && rejuvenate_node)
		{
		  GSL_SPIN_LOCK (&global_dcache_mutex); /* different lock */
		  global_dcache_n_aged_nodes--;
		  GSL_SPIN_UNLOCK (&global_dcache_mutex);
		}
	      return node;
	    }

	  node->ref_count++;
	  if (load_request == GSL_DATA_CACHE_DEMAND_LOAD)
	    while (!node->data)
	      sfi_cond_wait (&global_dcache_cond_node_filled, &dcache->mutex);
	  GSL_SPIN_UNLOCK (&dcache->mutex);
	  /* g_printerr ("hit: %d :%d: %d\n", node->offset, offset, node->offset + dcache->node_size); */

	  if (rejuvenate_node)
	    {
	      GSL_SPIN_LOCK (&global_dcache_mutex); /* different lock */
	      global_dcache_n_aged_nodes--;
	      GSL_SPIN_UNLOCK (&global_dcache_mutex);
	    }
	  
	  return node;					/* exact match */
	}
      insertion_pos = NODEP_INDEX (dcache, node_p);	/* insert before neighbour */
      if (offset > node->offset)			/* insert after neighbour */
	insertion_pos += 1;
      /* g_printerr ("mis: %d :%d: %d\n", node->offset, offset, node->offset + dcache->node_size); */
    }
  else
    insertion_pos = 0;	/* insert at start */

  if (load_request != GSL_DATA_CACHE_PEEK)
    node = data_cache_new_node_L (dcache, offset, insertion_pos, load_request == GSL_DATA_CACHE_DEMAND_LOAD);
  else
    node = NULL;

  GSL_SPIN_UNLOCK (&dcache->mutex);

  return node;
}

static gboolean /* still locked */
data_cache_free_olders_Lunlock (GslDataCache *dcache,
				guint         max_lru)	/* how many lru nodes to keep */
{
  GslDataCacheNode **slot_p;
  guint i, rejuvenate, size;
  guint n_freed = 0;

  g_return_val_if_fail (dcache != NULL, TRUE);

  /* it doesn't make sense to free nodes below the jitter that
   * AGE_EPSILON attempts to prevent.
   */
  max_lru = MAX (AGE_EPSILON, max_lru);
  if (max_lru >= dcache->max_age)
    return TRUE;

  rejuvenate = dcache->max_age - max_lru;
  if (0)
    g_print ("start sweep: dcache (%p) with %u nodes, max_age: %u, rejuvenate: %u (max_lru: %u)\n",
	     dcache, dcache->n_nodes, dcache->max_age, rejuvenate, max_lru);
  size = dcache->node_size + (dcache->padding << 1);
  slot_p = NULL;
  for (i = 0; i < dcache->n_nodes; i++)
    {
      GslDataCacheNode *node = dcache->nodes[i];

      if (!node->ref_count && node->age <= rejuvenate)
	{
	  sfi_delete_structs (GslDataType, size, node->data - dcache->padding);
	  sfi_delete_struct (GslDataCacheNode, node);
	  if (!slot_p)
	    slot_p = dcache->nodes + i;
	  n_freed++;
	}
      else
	{
	  node->age -= MIN (rejuvenate, node->age);
	  if (slot_p)
	    {
	      *slot_p = node;
	      slot_p++;
	    }
	}
    }
  dcache->max_age = max_lru;
  if (slot_p)
    dcache->n_nodes = NODEP_INDEX (dcache, slot_p);
  GSL_SPIN_UNLOCK (&dcache->mutex);

  if (n_freed)
    {
      GSL_SPIN_LOCK (&global_dcache_mutex);
      global_dcache_n_aged_nodes -= n_freed;
      GSL_SPIN_UNLOCK (&global_dcache_mutex);
    }
  if (0)
    g_printerr ("freed %u nodes (%u bytes) remaining %u bytes (this dcache: n_nodes=%u)\n",
		n_freed, n_freed * CONFIG_NODE_SIZE (),
		global_dcache_n_aged_nodes * CONFIG_NODE_SIZE (),
		dcache->n_nodes);
  return FALSE;
}

void
gsl_data_cache_unref_node (GslDataCache     *dcache,
			   GslDataCacheNode *node)
{
  GslDataCacheNode **node_p;
  gboolean check_cache;

  g_return_if_fail (dcache != NULL);
  g_return_if_fail (node != NULL);
  g_return_if_fail (node->ref_count > 0);

  GSL_SPIN_LOCK (&dcache->mutex);
  node_p = data_cache_lookup_nextmost_node_L (dcache, node->offset);
  g_assert (node_p && *node_p == node);	/* paranoid check lookup, yeah! */
  node->ref_count -= 1;
  check_cache = !node->ref_count;
  if (!node->ref_count &&
      (node->age + AGE_EPSILON <= dcache->max_age ||
       dcache->max_age < AGE_EPSILON))
    node->age = ++dcache->max_age;
  GSL_SPIN_UNLOCK (&dcache->mutex);

  if (check_cache)
    {
      guint node_size = CONFIG_NODE_SIZE ();
      guint cache_mem = BSE_CONFIG (dcache_cache_memory);
      guint current_mem;

      GSL_SPIN_LOCK (&global_dcache_mutex);
      global_dcache_n_aged_nodes++;
      current_mem = node_size * global_dcache_n_aged_nodes;
      if (current_mem > cache_mem)              /* round-robin cache trashing */
	{
	  guint dcache_count, needs_unlock;
	  dcache = sfi_ring_pop_head (&global_dcache_list);
	  GSL_SPIN_LOCK (&dcache->mutex);
	  dcache->ref_count++;
	  global_dcache_list = sfi_ring_append (global_dcache_list, dcache);
	  dcache_count = global_dcache_count;
	  GSL_SPIN_UNLOCK (&global_dcache_mutex);
#define DEBUG_TRASHING 0
#if DEBUG_TRASHING
          gint debug_gnaged = global_dcache_n_aged_nodes;
#endif
          if (dcache->high_persistency) /* hard/slow to refill */
            {
              /* try to free the actual cache overflow from the
               * dcache we just picked, but don't free more than
               * 25% of its nodes yet.
               * overflow is actual overhang + ~6% of cache size,
               * so cache sweeps are triggered less frequently.
               */
              current_mem -= cache_mem;		/* overhang */
              current_mem += cache_mem >> 4;	/* overflow = overhang + 6% */
              current_mem /= node_size;		/* n_nodes to free */
              current_mem = MIN (current_mem, dcache->n_nodes);
              guint max_lru = dcache->n_nodes;
              max_lru >>= 1;                    /* keep at least 75% of n_nodes */
              max_lru += max_lru >> 1;
              max_lru = MAX (max_lru, dcache->n_nodes - current_mem);
              needs_unlock = data_cache_free_olders_Lunlock (dcache, MAX (max_lru, LOW_PERSISTENCY_RESIDENT_SET));
            }
          else  /* low persistency - easy to refill */
            {
              guint max_lru = dcache->n_nodes;
              max_lru >>= 2;                    /* keep only 25% of n_nodes */
              needs_unlock = data_cache_free_olders_Lunlock (dcache, MAX (max_lru, LOW_PERSISTENCY_RESIDENT_SET));
            }
#if DEBUG_TRASHING
          if (dcache->dhandle->open_count)
            g_printerr ("shrunk dcache by: dhandle=%p - %s - highp=%d: %d bytes (kept: %d)\n",
                        dcache->dhandle, gsl_data_handle_name (dcache->dhandle),
                        dcache->high_persistency,
                        -(gint) node_size * (debug_gnaged - global_dcache_n_aged_nodes),
                        node_size * dcache->n_nodes);
#endif
	  if (needs_unlock)
	    GSL_SPIN_UNLOCK (&dcache->mutex);
	}
      else
	GSL_SPIN_UNLOCK (&global_dcache_mutex);
    }
}

void
gsl_data_cache_free_olders (GslDataCache *dcache,
			    guint         max_age)
{
  g_return_if_fail (dcache != NULL);

  GSL_SPIN_LOCK (&dcache->mutex);
  gboolean needs_unlock = data_cache_free_olders_Lunlock (dcache, max_age);
  if (needs_unlock)
    GSL_SPIN_UNLOCK (&dcache->mutex);
}

GslDataCache*
gsl_data_cache_from_dhandle (GslDataHandle *dhandle,
			     guint          min_padding)
{
  SfiRing *ring;

  g_return_val_if_fail (dhandle != NULL, NULL);

  GSL_SPIN_LOCK (&global_dcache_mutex);
  for (ring = global_dcache_list; ring; ring = sfi_ring_walk (ring, global_dcache_list))
    {
      GslDataCache *dcache = ring->data;

      if (dcache->dhandle == dhandle && dcache->padding >= min_padding)
	{
	  gsl_data_cache_ref (dcache);
	  GSL_SPIN_UNLOCK (&global_dcache_mutex);
	  return dcache;
	}
    }
  GSL_SPIN_UNLOCK (&global_dcache_mutex);

  return gsl_data_cache_new (dhandle, min_padding);
}
