/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1997-1999, 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#define	_GNU_SOURCE
#include	"glib-extra.h"

#include	<string.h>



/* --- string functions --- */
gchar**
g_straddv (gchar      **str_array,
	   const gchar *new_str)
{
  if (new_str)
    {
      if (!str_array)
	{
	  str_array = g_new (gchar*, 2);
	  str_array[0] = g_strdup (new_str);
	  str_array[1] = NULL;
	}
      else
	{
	  guint i = 0;

	  while (str_array[i])
	    i++;
	  str_array = g_renew (gchar*, str_array, i + 1 + 1);
	  str_array[i] = g_strdup (new_str);
	  i++;
	  str_array[i] = NULL;
	}
    }
  return str_array;
}

guint
g_strlenv (gchar **str_array)
{
  guint i = 0;

  if (str_array)
    while (str_array[i])
      i++;

  return i;
}

gchar**
g_strslistv (GSList *slist)
{
  gchar **str_array;
  guint i;

  if (!slist)
    return NULL;

  i = g_slist_length (slist);
  str_array = g_new (gchar*, i + 1);
  i = 0;
  while (slist)
    {
      str_array[i++] = g_strdup (slist->data);
      slist = slist->next;
    }
  str_array[i] = NULL;

  return str_array;
}

GDArray*
g_darray_new (guint prealloc)
{
  GDArray *darray = g_new0 (GDArray, 1);

  darray->n_prealloced = prealloc;
  darray->values = g_new0 (gdouble, darray->n_prealloced);

  return darray;
}

void
g_darray_free (GDArray *darray)
{
  g_return_if_fail (darray != NULL);

  g_free (darray->values);
  g_free (darray);
}

gdouble
g_darray_get (GDArray *darray,
	      guint    index)
{
  g_return_val_if_fail (darray != NULL, 0);
  g_return_val_if_fail (index < darray->n_values, 0);

  return darray->values[index];
}

void
g_darray_insert (GDArray *darray,
		 guint    index,
		 gdouble  value)
{
  guint i;

  g_return_if_fail (darray != NULL);
  g_return_if_fail (index <= darray->n_values);

  i = darray->n_values;
  i = darray->n_values += 1;
  if (darray->n_values > darray->n_prealloced)
    {
      darray->n_prealloced = darray->n_values;
      darray->values = g_renew (gdouble, darray->values, darray->n_prealloced);
    }
  g_memmove (darray->values + index + 1,
	     darray->values + index,
	     i - index);
  darray->values[index] = value;
}

void
g_darray_append (GDArray *darray,
		 gdouble  value)
{
  g_darray_insert (darray, darray->n_values, value);
}

void
g_darray_set (GDArray *darray,
	      guint    index,
	      gdouble  value)
{
  g_return_if_fail (darray != NULL);
  g_return_if_fail (index < darray->n_values);

  darray->values[index] = value;
}


#if 0

/* GLib main loop reentrant signal queue
 */
typedef struct _GUSignalData GUSignalData;
struct _GUSignalData
{
  guint8       index;
  guint8       shift;
  GUSignalFunc callback;
};

static gboolean g_usignal_prepare  (gpointer  source_data,
			 	    GTimeVal *current_time,
				    gint     *timeout,
				    gpointer  user_data);
static gboolean g_usignal_check    (gpointer  source_data,
				    GTimeVal *current_time,
				    gpointer  user_data);
static gboolean g_usignal_dispatch (gpointer  source_data,
				    GTimeVal *dispatch_time,
				    gpointer  user_data);

static GSourceFuncs usignal_funcs = {
  g_usignal_prepare,
  g_usignal_check,
  g_usignal_dispatch,
  g_free
};
static	guint32	usignals_notified[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

static gboolean
g_usignal_prepare (gpointer  source_data,
		   GTimeVal *current_time,
		   gint     *timeout,
		   gpointer  user_data)
{
  GUSignalData *usignal_data = source_data;
  
  return usignals_notified[usignal_data->index] & (1 << usignal_data->shift);
}

static gboolean
g_usignal_check (gpointer  source_data,
		 GTimeVal *current_time,
		 gpointer  user_data)
{
  GUSignalData *usignal_data = source_data;
  
  return usignals_notified[usignal_data->index] & (1 << usignal_data->shift);
}

static gboolean
g_usignal_dispatch (gpointer  source_data,
		    GTimeVal *dispatch_time,
		    gpointer  user_data)
{
  GUSignalData *usignal_data = source_data;
  
  usignals_notified[usignal_data->index] &= ~(1 << usignal_data->shift);
  
  return usignal_data->callback (-128 + usignal_data->index * 32 + usignal_data->shift, user_data);
}

guint
g_usignal_add (gint8	    usignal,
	       GUSignalFunc function,
	       gpointer     data)
{
  return g_usignal_add_full (G_PRIORITY_DEFAULT, usignal, function, data, NULL);
}

guint
g_usignal_add_full (gint           priority,
		    gint8          usignal,
		    GUSignalFunc   function,
		    gpointer       data,
		    GDestroyNotify destroy)
{
  GUSignalData *usignal_data;
  guint s = 128 + usignal;
  
  g_return_val_if_fail (function != NULL, 0);
  
  usignal_data = g_new (GUSignalData, 1);
  usignal_data->index = s / 32;
  usignal_data->shift = s % 32;
  usignal_data->callback = function;
  
  return g_source_add (priority, TRUE, &usignal_funcs, usignal_data, data, destroy);
}

void
g_usignal_notify (gint8 usignal)
{
  guint index, shift;
  guint s = 128 + usignal;
  
  index = s / 32;
  shift = s % 32;
  
  usignals_notified[index] |= 1 << shift;
}
#endif
