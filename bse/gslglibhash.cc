/* GSL Glib Hashtable implementation
 * Copyright (C) 2001 Stefan Westerfeld
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
#include "gslglib.h"
#include <list>
#include <map>

using std::list;
using std::pair;
using std::map;

/*
 * this uses a map of list to emulate somewhat hashtable like behaviour - note
 * that insert and remove are O(log N) due to the use of a map, instead of
 * ~ O(1) which they would be for a "real" hashtable
 */
struct _GHashTable
{
  GHashFunc hashFunc;
  GEqualFunc equalFunc;
  map<guint /*hashvalue for key*/, list< pair<gpointer,gpointer> > /*(key,value) pairs*/> nodes;

  /*Con*/ _GHashTable (GHashFunc hf, GEqualFunc ef) :
    hashFunc(hf), equalFunc(ef) { }
};


GHashTable* g_hash_table_new               (GHashFunc       hash_func,
					    GEqualFunc      key_equal_func)
{
  return new GHashTable (hash_func?hash_func:g_direct_hash, key_equal_func?key_equal_func:g_direct_equal);
}
void        g_hash_table_destroy           (GHashTable     *hash_table)
{
  g_return_if_fail (hash_table != NULL);

  delete hash_table;
}
void        g_hash_table_insert            (GHashTable     *hash_table,
					    gpointer        key,
					    gpointer        value)
{
  g_return_if_fail (hash_table != NULL);
  guint hashvalue = hash_table->hashFunc (key);

  list< pair<gpointer,gpointer> >& bucket = hash_table->nodes[hashvalue];
  list< pair<gpointer,gpointer> >::iterator i;
  
  for (i = bucket.begin(); i != bucket.end(); i++)
    {
      if (hash_table->equalFunc(i->first, key))
      	{
	  if (value || TRUE)
	    {
	      i->second = value; /* overwrite old hash value */
	      return;
	    }
	  else
	    {
	      bucket.erase(i); /* remove value */

	      if (bucket.empty()) /* remove bucket if this was the only value */
	        hash_table->nodes.erase (hashvalue);
	      return;
	    }
	}
    }

  if (value)
    hash_table->nodes[hashvalue].push_back(std::make_pair (key, value));
}

gpointer    g_hash_table_lookup            (GHashTable     *hash_table,
					    gconstpointer   key)
{
  g_return_val_if_fail (hash_table != NULL, NULL);

  guint hashvalue = hash_table->hashFunc (key);

  list< pair<gpointer,gpointer> >& bucket = hash_table->nodes[hashvalue];
  list< pair<gpointer,gpointer> >::iterator i;
  
  for (i = bucket.begin(); i != bucket.end(); i++)
    {
      if (hash_table->equalFunc(i->first, key))
      	return i->second;
    }

  return 0;
}
gboolean    g_hash_table_remove            (GHashTable     *hash_table,
					    gconstpointer   key)
{
  g_return_val_if_fail (hash_table != NULL, FALSE);

  guint hashvalue = hash_table->hashFunc (key);

  list< pair<gpointer,gpointer> >& bucket = hash_table->nodes[hashvalue];
  list< pair<gpointer,gpointer> >::iterator i;
  
  for (i = bucket.begin(); i != bucket.end(); i++)
    {
      if (hash_table->equalFunc(i->first, key))
      	{
      	  bucket.erase (i);

	  if (bucket.empty()) /* remove bucket if this was the only value */
	    hash_table->nodes.erase (hashvalue);
	  return true;
	}
    }

  return false;
}

void        g_hash_table_foreach           (GHashTable     *hash_table,
					    GHFunc          func,
					    gpointer        user_data)
{
  map<guint, list< pair<gpointer,gpointer> > >::iterator bi;

  g_return_if_fail (hash_table != NULL);

  /* for all buckets */
  for (bi = hash_table->nodes.begin (); bi != hash_table->nodes.end (); bi++)
    {
      list< pair<gpointer,gpointer> >& bucket = bi->second;
      list< pair<gpointer,gpointer> >::iterator i;
 
      /* for each element in the current bucket */
      for (i = bucket.begin(); i != bucket.end(); i++)
        func ((void*) i->first, (void*) i->second, user_data);
   }
}

/* vim:set ts=8 sw=2 sts=2: */
