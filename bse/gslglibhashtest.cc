/* GSL Glib Hashtable test
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
#include "gsldefs.h"
#include <string.h>

/* cause hashvalue collisions for the test */
guint g_str_narrow_hash (gconstpointer key) {
	return g_str_hash (key) & 15;
}

struct Test {
	char *key;
	char *value;
	int count;
} test[50];

void print_func (gpointer key, gpointer value, gpointer user_data) {
#if 0	 /* <- enable to get printing  of some entries */
	g_print ("%s: %s = %s\n", user_data, key, value);
#endif
}

void fail_func (gpointer key, gpointer value, gpointer user_data) {
	g_error ("*this* should not have happened");
}


void count_func (gpointer key, gpointer value, gpointer user_data) {
  g_assert (value != NULL && key != NULL);
	for(int i=0;i<50;i++)
	{
		if(strcmp((char *)key, test[i].key) == 0)
		{
			g_assert(strcmp((char *)value, test[i].value) == 0);
			test[i].count++;
		}
	}
}


int main()
{
	GHashTable *t = g_hash_table_new (g_str_narrow_hash, g_str_equal);

	for(int i=0;i<50;i++)
	{
		test[i].key = g_strdup_printf ("key-%d",i);
		test[i].value = g_strdup_printf ("value-%d",i);
		test[i].count = 0;

		g_hash_table_insert (t, test[i].key, test[i].value);
	}

	g_assert (strcmp ((char *)g_hash_table_lookup (t, "key-24"), "value-24") == 0);
	g_hash_table_foreach(t, print_func, g_strdup("all-keys-50"));
	g_hash_table_foreach(t, count_func, 0);

	for(int i=0;i<50;i++)
	{
		/* each key should have been found once in the hash table now */
		g_assert(test[i].count == 1);
	}

	for(int i=0;i<25;i++)
	{
		test[i].key = g_strdup_printf ("key-%d",i);
		test[i].value = g_strdup_printf ("another-value-%d",i);

		g_hash_table_insert (t, test[i].key, test[i].value);
	}

	g_assert (strcmp ((char *)g_hash_table_lookup (t, "key-24"), "another-value-24") == 0);
	g_hash_table_foreach(t, print_func, g_strdup("all-keys-new-25-old-25"));
	g_hash_table_foreach(t, count_func, 0);

	for(int i=0;i<50;i++)
	{
		/* each key should have been found once (with the new value) */
		g_assert(test[i].count == 2);
	}

	for(int i=0;i<50;i+=2)
	{
		/* remove even valued keys */
		g_hash_table_remove (t, test[i].key);
	}

	g_assert (g_hash_table_lookup (t, "key-24") == 0);
	g_hash_table_foreach(t, print_func, g_strdup("only-odd-keys-25"));
	g_hash_table_foreach(t, count_func, 0);

	for(int i=0;i<50;i++)
	{
		/* only odd keys should be there */
		g_assert((test[i].count == 3 && (i & 1))
		      || (test[i].count == 2 && !(i & 1)));
	}

	for(int i=1;i<50;i+=2)
	{
		/* remove odd valued keys */
		g_hash_table_remove (t, g_strdup(test[i].key));
	}

	g_hash_table_foreach(t, fail_func, 0);
	return 0;
}
