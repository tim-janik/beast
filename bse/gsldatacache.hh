/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __GSL_DATA_CACHE_H__
#define __GSL_DATA_CACHE_H__

#include <bse/gslcommon.hh>

G_BEGIN_DECLS

/* --- macros --- */
#define	GSL_DATA_CACHE_NODE_SIZE(dcache)	(((GslDataCache*) (dcache))->node_size)


/* --- typedefs & structures --- */
typedef gfloat                     GslDataType;
typedef struct _GslDataCacheNode   GslDataCacheNode;
struct _GslDataCache
{
  GslDataHandle	       *dhandle;
  guint			open_count;
  BirnetMutex		mutex;
  guint			ref_count;
  guint			node_size;	        /* power of 2, const for all dcaches */
  guint			padding;	        /* n_values around blocks */
  guint			max_age;
  gboolean		high_persistency;       /* valid for opened caches only */
  guint			n_nodes;
  GslDataCacheNode    **nodes;
};
struct _GslDataCacheNode
{
  int64	        offset;
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
						 int64		     offset,
						 GslDataCacheRequest load_request);
void		  gsl_data_cache_unref_node	(GslDataCache	    *dcache,
						 GslDataCacheNode   *node);
void		  gsl_data_cache_free_olders	(GslDataCache	    *dcache,
						 guint		     max_age);
GslDataCache*	  gsl_data_cache_from_dhandle	(GslDataHandle	    *dhandle,
						 guint		     min_padding);
						 
G_END_DECLS

#endif /* __GSL_DATA_CACHE_H__ */
