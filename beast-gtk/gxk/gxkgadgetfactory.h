/* GXK - Gtk+ Extension Kit
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
#ifndef __GXK_GADGET_FACTORY_H__
#define __GXK_GADGET_FACTORY_H__

#include "gxkgadget.h"
#include "gxkaction.h"

G_BEGIN_DECLS


/* --- type macros --- */
#define GXK_TYPE_GADGET_FACTORY              (gxk_gadget_factory_get_type ())
#define GXK_GADGET_FACTORY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_GADGET_FACTORY, GxkGadgetFactory))
#define GXK_GADGET_FACTORY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_GADGET_FACTORY, GxkGadgetFactoryClass))
#define GXK_IS_GADGET_FACTORY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_GADGET_FACTORY))
#define GXK_IS_GADGET_FACTORY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_GADGET_FACTORY))
#define GXK_GADGET_FACTORY_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_GADGET_FACTORY, GxkGadgetFactoryClass))


/* --- structures --- */
typedef struct {
  GxkActionFactory parent_instance;
  GtkWindow       *window;
  GxkGadget       *gadget;
  GxkGadget       *xdef_gadget;
  guint            cslot;
  gulong           timer;
  gchar           *action_root;
  gchar           *per_list;
  gchar           *per_branch;
  gchar           *per_action;
  gchar           *name;
  gchar           *action_list;
  gchar           *activatable;
  gchar           *regulate;
  GxkGadgetOpt    *pass_options;
  GData           *branches;
} GxkGadgetFactory;
typedef GxkActionFactoryClass GxkGadgetFactoryClass;


/* --- public API --- */
GType   gxk_gadget_factory_get_type             (void);
void    gxk_gadget_factory_check_anchored       (GxkGadgetFactory       *self);
void    gxk_gadget_factory_attach               (GxkGadgetFactory       *self,
                                                 GxkGadget              *gadget);
void    gxk_gadget_factory_match                (GxkGadgetFactory       *self,
                                                 const gchar            *prefix,
                                                 GxkActionList          *alist);


/* --- implementation details --- */
extern const GxkGadgetType *_gxk_gadget_factory_def;


G_END_DECLS

#endif /* __GXK_GADGET_FACTORY_H__ */
