/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2002 Tim Janik
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
#ifndef __GSL_DATA_CACHE_H__
#define __GSL_DATA_CACHE_H__

#include <gsl/gslcommon.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- macros --- */
#define	GSL_DATA_CACHE_NODE_SIZE(dcache)	(((GslDataCache*) (dcache))->node_size)


/* --- typedefs & structures --- */
typedef gfloat                     GslDataType;
typedef struct _GslDataCacheNode   GslDataCacheNode;
struct _GslDataCache
{
  GslDataHandle	       *dhandle;
  guint			open_count;
  GslMutex		mutex;
  guint			ref_count;
  guint			node_size;	/* power of 2, const for all dcaches */
  guint			padding;	/* n_values around blocks */
  guint			max_age;
  gboolean		low_persistency;
  guint			n_nodes;
  GslDataCacheNode    **nodes;
};
struct _GslDataCacheNode
{
  gsize		offset;
  guint		ref_count;
  guint		age;
  GslDataType  *data;	/* NULL while busy */
};
typedef enum
{
  GSL_DATA_CACHE_REQUEST     = FALSE, /* node->data may be NULL and will be filled */
  GSL_DATA_CACHE_DEMAND_LOAD = TRUE,  /* blocks until node->data != NULL */
  GSL_DATA_CACHE_PEEK	     = 2      /* may return NULL node, data != NULL otherwise */
} GslDataCacheRequest;


/* --- prototypes --- */
GslDataCache*	  gsl_data_cache_new		(GslDataHandle	    *dhandle,
						 guint		     padding);
GslDataCache*	  gsl_data_cache_ref		(GslDataCache	    *dcache);
void		  gsl_data_cache_unref		(GslDataCache	    *dcache);
void		  gsl_data_cache_open		(GslDataCache	    *dcache);
void		  gsl_data_cache_close		(GslDataCache	    *dcache);
GslDataCacheNode* gsl_data_cache_ref_node	(GslDataCache	    *dcache,
						 gsize		     offset,
						 GslDataCacheRequest load_request);
void		  gsl_data_cache_unref_node	(GslDataCache	    *dcache,
						 GslDataCacheNode   *node);
void		  gsl_data_cache_free_olders	(GslDataCache	    *dcache,
						 guint		     max_age);
GslDataCache*	  gsl_data_cache_from_dhandle	(GslDataHandle	    *dhandle,
						 guint		     min_padding);
						 
						 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_DATA_CACHE_H__ */
