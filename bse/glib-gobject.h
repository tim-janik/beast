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
#ifndef __G_GOBJECT_H__
#define __G_GOBJECT_H__

#include	<bse/glib-gtype.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define G_TYPE_IS_OBJECT(type)	   (G_TYPE_FUNDAMENTAL (type) == G_TYPE_OBJECT)
#define G_OBJECT(object)	   (G_IS_OBJECT (object) ? ((GObject*) (object)) : \
                                    G_TYPE_CHECK_INSTANCE_CAST ((object), G_TYPE_OBJECT, GObject))
#define G_OBJECT_CLASS(class)	   (G_IS_OBJECT_CLASS (class) ? ((GObjectClass*) (class)) : \
                                    G_TYPE_CHECK_CLASS_CAST ((class), G_TYPE_OBJECT, GObjectClass))
#define G_IS_OBJECT(object)	   (((GObject*) (object)) != NULL && \
                                    G_IS_OBJECT_CLASS (((GTypeInstance*) (object))->g_class))
#define G_IS_OBJECT_CLASS(class)   (((GTypeClass*) (class)) != NULL && \
                                    G_TYPE_IS_OBJECT (((GTypeClass*) (class))->g_type))
#define G_OBJECT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GObjectClass))
#define G_OBJECT_TYPE(object)	   (G_TYPE_FROM_INSTANCE (object))
#define G_OBJECT_TYPE_NAME(object) (g_type_name (G_OBJECT_TYPE (object)))
#define G_OBJECT_CLASS_TYPE(class) (G_TYPE_FROM_CLASS (class))
#define G_OBJECT_CLASS_NAME(class) (g_type_name (G_OBJECT_CLASS_TYPE (class)))


/* --- typedefs & structures --- */
typedef struct _GObject      GObject;
typedef struct _GObjectClass GObjectClass;
struct	_GObject
{
  GTypeInstance g_type_instance;

  /*< private >*/
  guint         ref_count;
  GData	       *qdata;
};
struct	_GObjectClass
{
  GTypeClass g_type_class;

  void	   (*last_unref)	(GObject	*object);
  void	   (*finalize)		(GObject	*object);
};


/* --- prototypes --- */
gpointer	g_object_new		(GType		 object_type,
					 gpointer	 _null);
GObject*	g_object_ref		(GObject	*object);
void		g_object_unref		(GObject	*object);
gpointer	g_object_get_qdata	(GObject	*object,
					 GQuark		 quark);
void		g_object_set_qdata	(GObject	*object,
					 GQuark		 quark,
					 gpointer	 data);
void		g_object_set_qdata_full	(GObject	*object,
					 GQuark		 quark,
					 gpointer	 data,
					 GDestroyNotify	 destroy);
gpointer	g_object_steal_qdata	(GObject	*object,
					 GQuark		 quark);


/* --- implementation details --- */
void		g_object_init		(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __G_GOBJECT_H__ */
