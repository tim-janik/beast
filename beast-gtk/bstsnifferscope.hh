/* BEAST - Better Audio System
 * Copyright (C) 2003 Tim Janik
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
#ifndef __BST_SNIFFER_SCOPE_H__
#define __BST_SNIFFER_SCOPE_H__

#include "bstutils.hh"

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
  guint     n_values;
  float    *lvalues;
  float    *rvalues;
  GdkGC    *oshoot_gc;
} BstSnifferScope;
typedef GtkWidgetClass BstSnifferScopeClass;
GType      bst_sniffer_scope_get_type       (void);
GtkWidget* bst_sniffer_scope_new            (void);
void       bst_sniffer_scope_set_sniffer    (BstSnifferScope    *scope,
                                             SfiProxy            proxy);

typedef enum {
  BST_SOURCE_PROBE_RANGE   = 0x01,
  BST_SOURCE_PROBE_ENERGIE = 0x02,
  BST_SOURCE_PROBE_SAMPLES = 0x04,
  BST_SOURCE_PROBE_FFT     = 0x08,
} BstSourceProbeFeature;

void bst_source_queue_probe_request (SfiProxy              source,
				     guint                 ochannel_id,
				     BstSourceProbeFeature pfeature,
				     gfloat                frequency);

G_END_DECLS

#endif /* __BST_SNIFFER_SCOPE_H__ */
