/* GSL - Generic Sound Layer
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
#include "gsldatacache.h"

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
#define	UPPER_POWER2(n)			(gsl_alloc_upper_power2 (MAX (n, 4)))
#define	CONFIG_NODE_SIZE()		(gsl_get_config ()->dcache_block_size)
#define	DFL_SWEEP			(32 * 1024 * 1024)


/* we use one global lock to protect the dcache_ht hash table and
 * closed_dcaches ring. also, each dcache has its own mutext
 * to protect updates in the reference count, nodes or node
 * data blocks. in order to avoid deadlocks, if both locks need
 * to be held, they always have to be acquired in the order
 * 1) global lock, 2) dcache lock.
 * parallel data block filling for a new node occours without
 * the dcache lock being held (as all calls to GslDataReader
 * functions). however, assignment of the newly acquired data
 * is again protected by the dcache lock. concurrent API entries
 * which require demand loading of such data will wait on a global
 * condition which is always signaled once a new data block read
 * has been completed. using one global condition should be
 * sufficient as there shouldn't normaly be more than one user-thread
 * waiting for demand load completion.
 */


/* --- prototypes --- */
static void			dcache_free		(GslDataCache	*dcache);
static GslDataCacheNode*	data_cache_new_node_L	(GslDataCache	*dcache,
							 gsize		 offset,
							 guint		 pos,
							 gboolean	 demand_load);


/* --- variables --- */
static GslMutex	   dcache_global = { 0, };
static GslCond	  *dcache_cond_node_filled = NULL;
static GslRing	  *dcache_list = NULL;
static guint       n_aged_nodes = 0;


/* --- functions --- */
void
_gsl_init_data_caches (void)
{
  g_assert (dcache_cond_node_filled == NULL);

  dcache_cond_node_filled = gsl_cond_new ();
  gsl_mutex_init (&dcache_global);
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
  g_return_val_if_fail (dhandle->mtime > 0, NULL);
  g_return_val_if_fail (dhandle->n_values > 0, NULL);
  g_assert (node_size == gsl_alloc_upper_power2 (node_size));
  g_return_val_if_fail (padding < node_size / 2, NULL);

  /* allocate new closed dcache if necessary */
  dcache = gsl_new_struct (GslDataCache, 1);
  dcache->handle = gsl_data_handle_ref (dhandle);
  dcache->open_count = 0;
  gsl_mutex_init (&dcache->mutex);
  dcache->ref_count = 1;
  dcache->node_size = node_size;
  dcache->padding = padding;
  dcache->max_age = 0;
  dcache->n_nodes = 0;
  dcache->nodes = g_renew (GslDataCacheNode*, NULL, UPPER_POWER2 (dcache->n_nodes));

  GSL_SPIN_LOCK (&dcache_global);
  dcache_list = gsl_ring_append (dcache_list, dcache);
  GSL_SPIN_UNLOCK (&dcache_global);

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
      gint error;

      error = gsl_data_handle_open (dcache->handle);
      if (error)
	{
	  /* FIXME: this is pretty fatal, throw out zero blocks now? */
	  gsl_message_send (GSL_MSG_DATA_CACHE,
			    GSL_ERROR_IO,
			    "failed to open \"%s\": %s",
			    dcache->handle->name,
			    g_strerror (error));
	}
      else
	{
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
    gsl_data_handle_close (dcache->handle);
  GSL_SPIN_UNLOCK (&dcache->mutex);
  if (need_unref)
    gsl_data_cache_unref (dcache);
}

GslDataCache*
gsl_data_cache_ref (GslDataCache *dcache)
{
  g_return_val_if_fail (dcache != NULL, NULL);
  g_return_val_if_fail (dcache->ref_count > 0, NULL);

  /* we might get invoked with dcache_global locked */
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

  gsl_data_handle_unref (dcache->handle);
  gsl_mutex_destroy (&dcache->mutex);
  for (i = 0; i < dcache->n_nodes; i++)
    {
      GslDataCacheNode *node = dcache->nodes[i];
      guint size;

      size = dcache->node_size + (dcache->padding << 1);
      gsl_delete_struct (GslDataType, size, node->data - dcache->padding);
      gsl_delete_struct (GslDataCacheNode, 1, node);
    }
  g_free (dcache->nodes);
  gsl_delete_struct (GslDataCache, 1, dcache);
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

      GSL_SPIN_LOCK (&dcache_global);
      GSL_SPIN_LOCK (&dcache->mutex);
      if (dcache->ref_count != 1)
	{
	  /* damn, some other thread trapped in, restart */
	  GSL_SPIN_UNLOCK (&dcache->mutex);
	  GSL_SPIN_UNLOCK (&dcache_global);
	  goto restart;
	}
      dcache->ref_count = 0;
      dcache_list = gsl_ring_remove (dcache_list, dcache);
      GSL_SPIN_UNLOCK (&dcache->mutex);
      n_aged_nodes -= dcache->n_nodes;
      GSL_SPIN_UNLOCK (&dcache_global);
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
				   gsize         offset)
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
		       gsize	     offset,
		       guint	     pos,
		       gboolean	     demand_load)
{
  GslDataCacheNode **node_p, *dnode;
  GslDataType *data, *node_data;
  guint new_node_array_size, old_node_array_size = UPPER_POWER2 (dcache->n_nodes);
  guint i, size;
  gint result;

  i = dcache->n_nodes++;
  new_node_array_size = UPPER_POWER2 (dcache->n_nodes);
  if (old_node_array_size != new_node_array_size)
    dcache->nodes = g_renew (GslDataCacheNode*, dcache->nodes, new_node_array_size);
  node_p = dcache->nodes + pos;
  g_memmove (node_p + 1, node_p, (i - pos) * sizeof (*node_p));
  dnode = gsl_new_struct (GslDataCacheNode, 1);
  (*node_p) = dnode;
  dnode->offset = offset & ~(dcache->node_size - 1);
  dnode->ref_count = 1;
  dnode->age = 0;
  dnode->data = NULL;
  GSL_SPIN_UNLOCK (&dcache->mutex);

  size = dcache->node_size + (dcache->padding << 1);
  data = gsl_new_struct (GslDataType, size);
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
  do
    {
      if (offset >= dcache->handle->n_values)
	break;
      size = MIN (size, dcache->handle->n_values - offset);
      result = gsl_data_handle_read (dcache->handle, offset, size, data);
      if (result < 0)
	{
	  gsl_message_send (GSL_MSG_DATA_CACHE, GSL_ERROR_READ_FAILED,
			    "reading from \"%s\"", dcache->handle->name);
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
  gsl_cond_broadcast (dcache_cond_node_filled);
  
  return dnode;
}

GslDataCacheNode*
gsl_data_cache_ref_node (GslDataCache *dcache,
			 gsize         offset,
			 gboolean      demand_load)
{
  GslDataCacheNode **node_p, *node;
  guint insertion_pos;

  g_return_val_if_fail (dcache != NULL, NULL);
  g_return_val_if_fail (dcache->ref_count > 0, NULL);
  g_return_val_if_fail (offset < dcache->handle->n_values, NULL);

  GSL_SPIN_LOCK (&dcache->mutex);
  node_p = data_cache_lookup_nextmost_node_L (dcache, offset);
  if (node_p)
    {
      node = *node_p;
      if (offset >= node->offset && offset < node->offset + dcache->node_size)
	{
	  gboolean rejuvenate_node = !node->ref_count;

	  node->ref_count++;
	  if (demand_load)
	    while (!node->data)
	      gsl_cond_wait (dcache_cond_node_filled, &dcache->mutex);
	  GSL_SPIN_UNLOCK (&dcache->mutex);
	  /* g_print("hit: %d :%d: %d\n", node->offset, offset, node->offset + dcache->node_size); */

	  if (rejuvenate_node)
	    {
	      GSL_SPIN_LOCK (&dcache_global);
	      n_aged_nodes--;
	      GSL_SPIN_UNLOCK (&dcache_global);
	    }
	  
	  return node;					/* exact match */
	}
      insertion_pos = NODEP_INDEX (dcache, node_p);	/* insert before neighbour */
      if (offset > node->offset)			/* insert after neighbour */
	insertion_pos += 1;
      /* g_print("mis: %d :%d: %d\n", node->offset, offset, node->offset + dcache->node_size); */
    }
  else
    insertion_pos = 0;	/* insert at start */
  node = data_cache_new_node_L (dcache, offset, insertion_pos, demand_load);
  GSL_SPIN_UNLOCK (&dcache->mutex);

  return node;
}

static void
data_cache_free_olders_LL (GslDataCache *dcache,
			   guint         oldest)
{
  GslDataCacheNode **slot_p;
  guint i, rejuvenate, size;
  guint n_freed = 0;

  g_return_if_fail (dcache != NULL);

  if (oldest >= dcache->max_age)
    return;

  rejuvenate = dcache->max_age - oldest;
  size = dcache->node_size + (dcache->padding << 1);
  slot_p = NULL;
  for (i = 0; i < dcache->n_nodes; i++)
    {
      GslDataCacheNode *node = dcache->nodes[i];

      if (!node->ref_count && node->age + oldest <= dcache->max_age)
	{
	  gsl_delete_struct (GslDataType, size, node->data - dcache->padding);
	  gsl_delete_struct (GslDataCacheNode, 1, node);
	  if (!slot_p)
	    slot_p = dcache->nodes + i;
	  n_freed++;
	}
      else
	{
	  node->age -= rejuvenate;
	  if (slot_p)
	    {
	      *slot_p = node;
	      slot_p++;
	    }
	}
    }
  dcache->max_age -= rejuvenate;
  if (slot_p)
    dcache->n_nodes = NODEP_INDEX (dcache, slot_p);
  n_aged_nodes -= n_freed;
  if (0)
    g_print ("freed %u nodes (%u bytes) remaining %u bytes\n",
	     n_freed, n_freed * CONFIG_NODE_SIZE (),
	     n_aged_nodes * CONFIG_NODE_SIZE ());
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
  if (!node->ref_count)
    node->age = ++dcache->max_age;
  GSL_SPIN_UNLOCK (&dcache->mutex);

  if (check_cache)
    {
      guint node_size = CONFIG_NODE_SIZE ();
      
      /* FIXME: cache sweeping should note be done from _unref */
      GSL_SPIN_LOCK (&dcache_global);
      n_aged_nodes++;
      if (node_size * n_aged_nodes > gsl_get_config ()->dcache_cache_memory)
	{
	  dcache = gsl_ring_pop_head (&dcache_list);
	  GSL_SPIN_LOCK (&dcache->mutex);
	  dcache_list = gsl_ring_append (dcache_list, dcache);
	  data_cache_free_olders_LL (dcache, MAX (dcache->max_age, DFL_SWEEP) - DFL_SWEEP);
	  GSL_SPIN_UNLOCK (&dcache->mutex);
	}
      GSL_SPIN_UNLOCK (&dcache_global);
    }
}

void
gsl_data_cache_free_olders (GslDataCache *dcache,
			    guint         oldest)
{
  g_return_if_fail (dcache != NULL);

  GSL_SPIN_LOCK (&dcache_global);
  GSL_SPIN_LOCK (&dcache->mutex);
  data_cache_free_olders_LL (dcache, oldest);
  GSL_SPIN_UNLOCK (&dcache->mutex);
  GSL_SPIN_UNLOCK (&dcache_global);
}

GslDataCache*
gsl_data_cache_from_dhandle (GslDataHandle *dhandle,
			     guint          min_padding)
{
  GslRing *ring;

  g_return_val_if_fail (dhandle != NULL, NULL);

  GSL_SPIN_LOCK (&dcache_global);
  for (ring = dcache_list; ring; ring = gsl_ring_walk (dcache_list, ring->next))
    {
      GslDataCache *dcache = ring->data;

      if (dcache->handle == dhandle && dcache->padding >= min_padding)
	{
	  gsl_data_cache_ref (dcache);
	  GSL_SPIN_UNLOCK (&dcache_global);
	  return dcache;
	}
    }
  GSL_SPIN_UNLOCK (&dcache_global);

  return gsl_data_cache_new (dhandle, min_padding);
}
