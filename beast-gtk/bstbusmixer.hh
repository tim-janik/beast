// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_BUS_MIXER_H__
#define __BST_BUS_MIXER_H__

#include	"bstitemview.hh"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_BUS_MIXER              (bst_bus_mixer_get_type ())
#define BST_BUS_MIXER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_BUS_MIXER, BstBusMixer))
#define BST_BUS_MIXER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_BUS_MIXER, BstBusMixerClass))
#define BST_IS_BUS_MIXER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_BUS_MIXER))
#define BST_IS_BUS_MIXER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_BUS_MIXER))
#define BST_BUS_MIXER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_BUS_MIXER, BstBusMixerClass))


/* --- structures & typedefs --- */
typedef	struct	_BstBusMixer      BstBusMixer;
typedef	struct	_BstBusMixerClass BstBusMixerClass;
struct _BstBusMixer
{
  BstItemView      parent_object;
  GSList          *unlisteners;
  GtkBox          *hbox;
};
struct _BstBusMixerClass
{
  BstItemViewClass parent_class;
};


/* --- prototypes --- */
GType		bst_bus_mixer_get_type  (void);
GtkWidget*      bst_bus_mixer_new       (SfiProxy        song);

G_END_DECLS

#endif /* __BST_BUS_MIXER_H__ */
