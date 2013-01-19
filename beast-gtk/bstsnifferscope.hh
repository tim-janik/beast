// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
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
