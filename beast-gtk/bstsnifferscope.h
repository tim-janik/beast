/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_SNIFFER_SCOPE_H__
#define __BST_SNIFFER_SCOPE_H__

#include "bstutils.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_SNIFFER_SCOPE              (bst_sniffer_scope_get_type ())
#define BST_SNIFFER_SCOPE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_SNIFFER_SCOPE, BstSnifferScope))
#define BST_SNIFFER_SCOPE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_SNIFFER_SCOPE, BstSnifferScopeClass))
#define BST_IS_SNIFFER_SCOPE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_SNIFFER_SCOPE))
#define BST_IS_SNIFFER_SCOPE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_SNIFFER_SCOPE))
#define BST_SNIFFER_SCOPE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_SNIFFER_SCOPE, BstSnifferScopeClass))

/* --- API --- */
typedef struct {
  GtkWidget parent_instance;
  SfiProxy  proxy;
} BstSnifferScope;
typedef GtkWidgetClass BstSnifferScopeClass;
GType      bst_sniffer_scope_get_type    (void);
GtkWidget* bst_sniffer_scope_new         (void);
void       bst_sniffer_scope_set_sniffer (BstSnifferScope *scope,
                                          SfiProxy         proxy);
G_END_DECLS

#endif /* __BST_SNIFFER_SCOPE_H__ */
