/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BST_ACTIVATABLE_H__
#define __BST_ACTIVATABLE_H__

#include "bstutils.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_ACTIVATABLE            (bst_activatable_get_type ())
#define BST_ACTIVATABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BST_TYPE_ACTIVATABLE, BstActivatable))
#define BST_ACTIVATABLE_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), BST_TYPE_ACTIVATABLE, BstActivatableIface))
#define BST_IS_ACTIVATABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BST_TYPE_ACTIVATABLE))
#define BST_ACTIVATABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BST_TYPE_ACTIVATABLE, BstActivatableIface))


/* --- typedefs --- */
typedef struct _BstActivatable      BstActivatable;     /* dummy typedef */
typedef struct _BstActivatableIface BstActivatableIface;


/* --- structures --- */
struct _BstActivatableIface
{
  GTypeInterface g_iface;
  
  void     (*activate)       (BstActivatable *activatable,
                              gulong          action);
  gboolean (*can_activate)   (BstActivatable *activatable,
                              gulong          action);
  void     (*update)         (BstActivatable *activatable);
  void     (*request_update) (BstActivatable *activatable);
};


/* --- functions --- */
GType     bst_activatable_get_type                (void);
void      bst_activatable_activate                (BstActivatable *self,
                                                   gulong          action);
gboolean  bst_activatable_can_activate            (BstActivatable *self,
                                                   gulong          action);
void      bst_widget_update_activatable           (gpointer        widget);

/* --- internal --- */
void      bst_activatable_request_update          (BstActivatable *self);
void      bst_activatable_update_enqueue          (BstActivatable *self);
void      bst_activatable_update_dequeue          (BstActivatable *self);
void      bst_activatable_default_request_update  (BstActivatable *self);

/* --- convenience --- */
void      bst_type_implement_activatable          (GType           type,
                                                   void          (*activate) (BstActivatable *, gulong),
                                                   gboolean      (*can_activate) (BstActivatable *, gulong),
                                                   void          (*update)       (BstActivatable *));


G_END_DECLS

#endif  /* __BST_ACTIVATABLE_H__ */
