/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1998, 1999, 2000 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#undef		G_LOG_DOMAIN
#define		G_LOG_DOMAIN	"GOSys"
#include	"glib-gobject.h"


/* --- prototypes --- */
static void	g_object_do_class_init	(GObjectClass	*class);
static void	g_object_do_init	(GObject	*object);
static void	g_object_do_last_unref	(GObject	*object);
static void	g_object_do_finalize	(GObject	*object);


/* --- functions --- */
void
g_object_init (void)
{
  static gboolean initialized = FALSE;
  static const GTypeFundamentalInfo finfo = {
    G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE | G_TYPE_FLAG_DERIVABLE | G_TYPE_FLAG_DEEP_DERIVABLE,
    0       /* n_collect_bytes */,
    NULL    /* GTypeParamCollector */,
  };
  static GTypeInfo info = {
    sizeof (GObjectClass),
    NULL    /* base_init */,
    NULL    /* base_destroy */,
    (GClassInitFunc) g_object_do_class_init,
    NULL	/* class_destroy */,
    NULL    /* class_data */,
    sizeof (GObject),
    0	/* n_preallocs */,
    (GInstanceInitFunc) g_object_do_init,
  };
  GType type;
  
  g_return_if_fail (initialized == FALSE);
  initialized = TRUE;
  
  /* G_TYPE_OBJECT
   */
  type = g_type_register_fundamental (G_TYPE_OBJECT, "GObject", &finfo, &info);
  g_assert (type == G_TYPE_OBJECT);
}

static void
g_object_do_class_init (GObjectClass *class)
{
  class->last_unref = g_object_do_last_unref;
  class->finalize = g_object_do_finalize;
}

static void
g_object_do_init (GObject *object)
{
  object->ref_count = 1;
  object->qdata = NULL;
}

static void
g_object_do_last_unref (GObject *object)
{
  g_return_if_fail (object->ref_count > 0);

  object->ref_count -= 1;
  if (object->ref_count == 0)	/* may have been re-referenced meanwhile */
    G_OBJECT_GET_CLASS (object)->finalize (object);
}

static void
g_object_do_finalize (GObject *object)
{
  g_datalist_clear (&object->qdata);

  g_type_free_instance ((GTypeInstance*) object);
}

gpointer
g_object_new (GType    object_type,
	      gpointer _null)
{
  GObject *object;

  g_return_val_if_fail (G_TYPE_IS_OBJECT (object_type), NULL);

  object = (GObject*) g_type_create_instance (object_type);

  return object;
}

GObject*
g_object_ref (GObject *object)
{
  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (object->ref_count > 0, NULL);

  object->ref_count += 1;

  return object;
}

void
g_object_unref (GObject *object)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);

  if (object->ref_count > 1)
    object->ref_count -= 1;
  else
    G_OBJECT_GET_CLASS (object)->last_unref (object);
}

gpointer
g_object_get_qdata (GObject *object,
		    GQuark   quark)
{
  g_return_val_if_fail (G_IS_OBJECT (object), NULL);

  return quark ? g_datalist_id_get_data (&object->qdata, quark) : NULL;
}

void
g_object_set_qdata (GObject *object,
		    GQuark   quark,
		    gpointer data)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (quark > 0);
  
  g_datalist_id_set_data (&object->qdata, quark, data);
}

void
g_object_set_qdata_full (GObject       *object,
			 GQuark         quark,
			 gpointer       data,
			 GDestroyNotify destroy)
{
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (quark > 0);
  
  g_datalist_id_set_data_full (&object->qdata, quark, data, data ? destroy : NULL);
}

gpointer
g_object_steal_qdata (GObject *object,
		      GQuark   quark)
{
  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (quark > 0, NULL);
  
  return g_datalist_id_remove_no_notify (&object->qdata, quark);
}
