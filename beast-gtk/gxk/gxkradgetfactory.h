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
#ifndef __GXK_RADGET_FACTORY_H__
#define __GXK_RADGET_FACTORY_H__

#include "gxkradget.h"
#include "gxkaction.h"

G_BEGIN_DECLS


/* --- type macros --- */
#define GXK_TYPE_RADGET_FACTORY              (gxk_radget_factory_get_type ())
#define GXK_RADGET_FACTORY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_RADGET_FACTORY, GxkRadgetFactory))
#define GXK_RADGET_FACTORY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_RADGET_FACTORY, GxkRadgetFactoryClass))
#define GXK_IS_RADGET_FACTORY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_RADGET_FACTORY))
#define GXK_IS_RADGET_FACTORY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_RADGET_FACTORY))
#define GXK_RADGET_FACTORY_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_RADGET_FACTORY, GxkRadgetFactoryClass))


/* --- structures --- */
typedef struct {
  GObject          parent_instance;
  GtkWindow       *window;
  GxkRadget       *radget;
  GxkRadget       *xdef_radget;
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
  GxkRadgetArgs   *call_args;
  GData           *branch_widgets;
  GSList          *branches;
} GxkRadgetFactory;
typedef GObjectClass GxkRadgetFactoryClass;


/* --- public API --- */
GType   gxk_radget_factory_get_type             (void);
void    gxk_radget_factory_check_anchored       (GxkRadgetFactory       *self);
void    gxk_radget_factory_attach               (GxkRadgetFactory       *self,
                                                 GxkRadget              *radget);
void    gxk_radget_factory_match                (GxkRadgetFactory       *self,
                                                 const gchar            *prefix,
                                                 GxkActionList          *alist);


/* --- GxkFactoryBranch --- */
#define GXK_TYPE_FACTORY_BRANCH              (gxk_factory_branch_get_type ())
#define GXK_FACTORY_BRANCH(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_FACTORY_BRANCH, GxkFactoryBranch))
#define GXK_FACTORY_BRANCH_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_FACTORY_BRANCH, GxkFactoryBranchClass))
#define GXK_IS_FACTORY_BRANCH(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_FACTORY_BRANCH))
#define GXK_IS_FACTORY_BRANCH_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_FACTORY_BRANCH))
#define GXK_FACTORY_BRANCH_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_FACTORY_BRANCH, GxkFactoryBranchClass))
typedef struct {
  GObject        parent_instance;
  gchar         *uline_label;
  gchar         *key_label;
  GxkRadgetArgs *branch_args;
} GxkFactoryBranch;
typedef GObjectClass GxkFactoryBranchClass;
GType   gxk_factory_branch_get_type          (void);


/* --- implementation details --- */
extern const GxkRadgetType *gxk_radget_factory_def;
extern const GxkRadgetType *gxk_factory_branch_def;


G_END_DECLS

#endif /* __GXK_RADGET_FACTORY_H__ */
