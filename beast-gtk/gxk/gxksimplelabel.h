/* GxkSimpleLabel - Small text-cutting label
 * Copyright (C) 2003 Tim Janik
 * Copyright (C) 1997-2000 the GTK+ Team and others
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * The GxkSimpleLabel code is derived from the GtkLabel implementation
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GXK_SIMPLE_LABEL_H__
#define __GXK_SIMPLE_LABEL_H__

#include <gxk/gxkutils.h>

G_BEGIN_DECLS

#define GXK_TYPE_SIMPLE_LABEL		  (gxk_simple_label_get_type ())
#define GXK_SIMPLE_LABEL(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GXK_TYPE_SIMPLE_LABEL, GxkSimpleLabel))
#define GXK_SIMPLE_LABEL_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_SIMPLE_LABEL, GxkSimpleLabelClass))
#define GXK_IS_SIMPLE_LABEL(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GXK_TYPE_SIMPLE_LABEL))
#define GXK_IS_SIMPLE_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_SIMPLE_LABEL))
#define GXK_SIMPLE_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GXK_TYPE_SIMPLE_LABEL, GxkSimpleLabelClass))

typedef struct {
  GtkWidget      parent_instance;
  
  gchar         *label;
  guint8         jtype;
  guint          use_underline : 1;
  guint          auto_cut : 1;
  guint          needs_cutting : 1;
  gchar         *text;
  guint          mnemonic_keyval;
  GtkWidget     *mnemonic_widget;
  GtkWindow     *mnemonic_window;
  PangoAttrList *effective_attrs;
  PangoLayout   *layout;
} GxkSimpleLabel;
typedef GtkWidgetClass GxkSimpleLabelClass;

GType   gxk_simple_label_get_type             (void) G_GNUC_CONST;
void    gxk_simple_label_set_mnemonic_widget  (GxkSimpleLabel  *self,
                                               GtkWidget       *widget);

G_END_DECLS

#endif /* __GXK_SIMPLE_LABEL_H__ */
