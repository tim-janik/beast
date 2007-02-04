/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2005 Tim Janik
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
#ifndef __GXK_NOTEBOOK_H__
#define __GXK_NOTEBOOK_H__

#include "gxkassortment.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_NOTEBOOK              (gxk_notebook_get_type ())
#define GXK_NOTEBOOK(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_NOTEBOOK, GxkNotebook))
#define GXK_NOTEBOOK_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_NOTEBOOK, GxkNotebookClass))
#define GXK_IS_NOTEBOOK(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_NOTEBOOK))
#define GXK_IS_NOTEBOOK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_NOTEBOOK))
#define GXK_NOTEBOOK_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_NOTEBOOK, GxkNotebookClass))


/* --- structures --- */
typedef struct {
  GtkNotebook    parent_instance;
  gchar         *assortment_name;
  GxkAssortment *assortment;
} GxkNotebook;
typedef struct {
  GtkNotebookClass parent_class;
} GxkNotebookClass;


/* --- prototypes --- */
GType		gxk_notebook_get_type		(void);
void            gxk_notebook_set_assortment     (GxkNotebook    *self,
                                                 GxkAssortment  *assortment);

G_END_DECLS

#endif  /* __GXK_NOTEBOOK_H__ */
