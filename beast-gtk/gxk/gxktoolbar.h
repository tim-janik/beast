/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2002 Tim Janik
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
#ifndef __GXK_TOOLBAR_H__
#define __GXK_TOOLBAR_H__

#include "gxkutils.h"

G_BEGIN_DECLS


/* --- type macros --- */
#define GXK_TYPE_TOOLBAR		(gxk_toolbar_get_type ())
#define GXK_TOOLBAR(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_TOOLBAR, GxkToolbar))
#define GXK_TOOLBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_TOOLBAR, GxkToolbarClass))
#define GXK_IS_TOOLBAR(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_TOOLBAR))
#define GXK_IS_TOOLBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_TOOLBAR))
#define GXK_TOOLBAR_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GXK_TYPE_TOOLBAR, GxkToolbarClass))


/* --- toolbar widget --- */
typedef struct _GxkToolbar	     GxkToolbar;
typedef struct _GxkToolbarClass GxkToolbarClass;
struct _GxkToolbar
{
  GtkFrame       parent_object;

  GtkReliefStyle relief_style;
  GtkIconSize	 icon_size;
  gint		 icon_width;
  gint		 icon_height;
  GtkSizeGroup  *size_group;
  GtkWidget	*box;
};
struct _GxkToolbarClass
{
  GtkFrameClass parent_class;
};
typedef enum {
  GXK_TOOLBAR_SPACE,
  GXK_TOOLBAR_SEPARATOR,
  GXK_TOOLBAR_BUTTON,
  GXK_TOOLBAR_TRUNC_BUTTON,
  GXK_TOOLBAR_EXTRA_BUTTON,
  GXK_TOOLBAR_TOGGLE,
  GXK_TOOLBAR_TRUNC_TOGGLE,
  GXK_TOOLBAR_EXTRA_TOGGLE,
  GXK_TOOLBAR_WIDGET,
  GXK_TOOLBAR_TRUNC_WIDGET,
  GXK_TOOLBAR_EXTRA_WIDGET,
  GXK_TOOLBAR_FILL_WIDGET
  /* TRUNC => clip the label
   * EXTRA => free form widget size (not in size group)
   * FILL  => expand & fill widget with toolbar resizes
   */
} GxkToolbarChild;
typedef void (*GxkToolbarChoiceFunc)	(gpointer	data,
					 guint		choice);


/* --- API --- */
GType		gxk_toolbar_get_type		(void);
GxkToolbar*	gxk_toolbar_new			(gpointer	 nullify_pointer);
GtkWidget*	gxk_toolbar_append		(GxkToolbar	*self,
						 GxkToolbarChild child_type,
						 const gchar	*name,
						 const gchar	*tooltip,
						 GtkWidget	*icon);
GtkWidget*	gxk_toolbar_append_stock	(GxkToolbar	*self,
						 GxkToolbarChild child_type,
						 const gchar	*name,
						 const gchar	*tooltip,
						 const gchar    *stock_id);
#define	gxk_toolbar_append_space(self)		gxk_toolbar_append ((self), GXK_TOOLBAR_SPACE, 0, 0, 0)
#define	gxk_toolbar_append_separator(self)	gxk_toolbar_append ((self), GXK_TOOLBAR_SEPARATOR, 0, 0, 0)
GtkWidget*	gxk_toolbar_append_choice	(GxkToolbar	*self,
						 GxkToolbarChild child_type,
						 GxkToolbarChoiceFunc choice_func,
						 gpointer        data,
						 GDestroyNotify  data_free);
void		gxk_toolbar_choice_add		(GtkWidget	*widget,
						 const gchar	*name,
						 const gchar	*tooltip,
						 GtkWidget	*icon,
						 guint           choice);
void		gxk_toolbar_choice_set		(GtkWidget	*widget,
						 const gchar	*name,
						 const gchar	*tooltip,
						 GtkWidget	*icon,
						 guint           choice);

G_END_DECLS

#endif /* __GXK_TOOLBAR_H__ */

