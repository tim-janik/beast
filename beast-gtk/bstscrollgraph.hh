// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_SCROLLGRAPH_H__
#define __BST_SCROLLGRAPH_H__

#include "bstutils.hh"

/* --- type macros --- */
#define BST_TYPE_SCROLLGRAPH		  (bst_scrollgraph_get_type ())
#define BST_SCROLLGRAPH(object)		  (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_SCROLLGRAPH, BstScrollgraph))
#define BST_SCROLLGRAPH_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_SCROLLGRAPH, BstScrollgraphClass))
#define BST_IS_SCROLLGRAPH(object)	  (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_SCROLLGRAPH))
#define BST_IS_SCROLLGRAPH_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_SCROLLGRAPH))
#define BST_SCROLLGRAPH_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((obj), BST_TYPE_SCROLLGRAPH, BstScrollgraphClass))

/* --- structures & typedefs --- */
typedef struct _BstScrollgraph      BstScrollgraph;
typedef struct _BstScrollgraphClass BstScrollgraphClass;
struct _BstScrollgraph
{
  GtkBin          parent_instance;
  Bst::Direction  direction;
  uint            window_size;
  guint           flip : 1;
  guint           delete_toplevel : 1;  /* upon proxy::release */
  guint           mix_freq;
  gdouble         boost;
  guint           n_points;
  guint           n_bars;
  guint           bar_offset; /* start of ring-buffer */
  gfloat         *values;     /* [n_points * n_bars] */
  GdkPixbuf      *pixbuf;     /* n_points wide or high */
  GdkWindow      *canvas;
  Bse::SourceH    source;
  size_t          probes_handler = 0;
  guint           ochannel;
};
struct _BstScrollgraphClass
{
  GtkBinClass parent_class;
  void  (*resize_values) (BstScrollgraph *self, Bst::Direction direction);
};

/* --- public methods --- */
GType	        bst_scrollgraph_get_type	(void);
void            bst_scrollgraph_clear           (BstScrollgraph *self);
void            bst_scrollgraph_set_source      (BstScrollgraph *self, Bse::SourceH source, uint ochannel);
GtkWidget*      bst_scrollgraph_build_dialog    (GtkWidget *alive_object, Bse::SourceH source, uint ochannel);

#endif /* __BST_SCROLLGRAPH_H__ */
