#include "gslglib.h"
#include <hash_map>


struct eqclass
{
  GEqualFunc eq_func;
  bool operator() (gconstpointer a, gconstpointer b)
  {
    return eq_func (a, b);
  }
  /* Con */ eqclass (GEqualFunc f) { eq_func = f; }
};
struct hashclass
{
  GHashFunc hash_func;
  size_t operator() (gconstpointer key) const
  {
    return hash_func (key);
  }
  /* Con */ hashclass (GHashFunc f) { hash_func = f; }
};

typedef hash_map<gconstpointer, gpointer, hashclass, eqclass> MyHash;
struct _GHashTable
{
  MyHash nodes;
  /*Con*/ _GHashTable (GHashFunc hf, GEqualFunc ef) :
    nodes (0, hf, ef) {}
};


GHashTable* g_hash_table_new               (GHashFunc       hash_func,
					    GEqualFunc      key_equal_func)
{
  return new GHashTable (hash_func?hash_func:g_direct_hash, key_equal_func ?key_equal_func:g_direct_equal);
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

  hash_table->nodes[key] = value;
}
gpointer    g_hash_table_lookup            (GHashTable     *hash_table,
					    gconstpointer   key)
{
  g_return_val_if_fail (hash_table != NULL, NULL);

  return hash_table->nodes[key];
}
gboolean    g_hash_table_remove            (GHashTable     *hash_table,
					    gconstpointer   key)
{
  MyHash::iterator i;

  g_return_val_if_fail (hash_table != NULL, FALSE);

  i = hash_table->nodes.find (key);
  if (i == hash_table->nodes.end ())
    return false;
  hash_table->nodes.erase (i);
  return true;
}

void        g_hash_table_foreach           (GHashTable     *hash_table,
					    GHFunc          func,
					    gpointer        user_data)
{
  MyHash::iterator i;

  g_return_if_fail (hash_table != NULL);

  for (i = hash_table->nodes.begin (); i != hash_table->nodes.end (); i++)
    func ((void*) i->first, (void*) i->second, user_data);
}

