/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
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
#include "sfiustore.h"

/* --- strcutures --- */
static inline SfiUStore*
scast (register GTree *tree)
{
  return (SfiUStore*) tree;
}
static inline GTree*
tcast (register SfiUStore *store)
{
  return (GTree*) store;
}


/* --- unique ID store --- */
static gint
ustore_cmp (gconstpointer a,
	    gconstpointer b)
{
  gulong u1 = (gulong) a;
  gulong u2 = (gulong) b;
  return u1 < u2 ? -1 : u1 != u2;
}

SfiUStore*
sfi_ustore_new (void)
{
  SfiUStore *store = scast (g_tree_new (ustore_cmp));

  return store;
}

gpointer
sfi_ustore_lookup (SfiUStore *store,
		   gulong     unique_id)
{
  g_return_val_if_fail (store != NULL, NULL);

  return g_tree_lookup (tcast (store), (gpointer) unique_id);
}

void
sfi_ustore_insert (SfiUStore *store,
		   gulong     unique_id,
		   gpointer   value)
{
  g_return_if_fail (store != NULL);

  if (!value)
    g_tree_remove (tcast (store), (gpointer) unique_id);
  else
    g_tree_insert (tcast (store), (gpointer) unique_id, value);
}

void
sfi_ustore_remove (SfiUStore *store,
		   gulong     unique_id)
{
  g_return_if_fail (store != NULL);

  g_tree_remove (tcast (store), (gpointer) unique_id);
}

typedef struct {
  gpointer         data;
  SfiUStoreForeach foreach;
} FData;

static gboolean
foreach_wrapper (gpointer key,
		 gpointer value,
		 gpointer data)
{
  FData *fdata = data;
  /* iterate as long as SfiUStoreForeach() returns TRUE */
  return !fdata->foreach (fdata->data, (gulong) key, value);
}

void
sfi_ustore_foreach (SfiUStore       *store,
		    SfiUStoreForeach foreach,
		    gpointer         data)
{
  FData fdata;

  g_return_if_fail (store != NULL);

  fdata.data = data;
  fdata.foreach = foreach;
  g_tree_foreach (tcast (store), foreach_wrapper, &fdata);
}

void
sfi_ustore_destroy (SfiUStore *store)
{
  g_return_if_fail (store != NULL);

  g_tree_destroy (tcast (store));
}

/* --- unique ID pool --- */
#define UPOOL_TAG ((gpointer) sfi_upool_new)

static inline SfiUPool*
upool_cast (register SfiUStore *ustore)
{
  return (SfiUPool*) ustore;
}

static inline SfiUStore*
ustore_cast (register SfiUPool *upool)
{
  return (SfiUStore*) upool;
}

SfiUPool*
sfi_upool_new (void)
{
  return upool_cast (sfi_ustore_new ());
}

gboolean
sfi_upool_lookup (SfiUPool *pool,
		  gulong    unique_id)
{
  return sfi_ustore_lookup (ustore_cast (pool), unique_id) != NULL;
}

void
sfi_upool_set (SfiUPool *pool,
	       gulong    unique_id)
{
  sfi_ustore_insert (ustore_cast (pool), unique_id, UPOOL_TAG);
}

void
sfi_upool_unset (SfiUPool *pool,
		 gulong    unique_id)
{
  sfi_ustore_remove (ustore_cast (pool), unique_id);
}

void
sfi_upool_foreach (SfiUPool        *pool,
		   SfiUPoolForeach  foreach,
		   gpointer         data)
{
  sfi_ustore_foreach (ustore_cast (pool), (SfiUStoreForeach) foreach, data);
}

void
sfi_upool_destroy (SfiUPool *pool)
{
  sfi_ustore_destroy (ustore_cast (pool));
}
