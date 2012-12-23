// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_KNOB_H__
#define __BST_KNOB_H__

#include <gtk/gtkadjustment.h>
#include <gtk/gtkimage.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_KNOB			(bst_knob_get_type ())
#define BST_KNOB(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_KNOB, BstKnob))
#define BST_KNOB_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_KNOB, BstKnobClass))
#define BST_IS_KNOB(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_KNOB))
#define BST_IS_KNOB_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_KNOB))
#define BST_KNOB_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BST_TYPE_KNOB, BstKnobClass))

/* --- structures & typedefs --- */
typedef struct _BstKnob	     BstKnob;
typedef struct _BstKnobClass BstKnobClass;
struct _BstKnob
{
  GtkImage parent_object;

  /* The update policy: GTK_UPDATE_CONTINUOUS,
   * GTK_UPDATE_DISCONTINUOUS or GTK_UPDATE_DELAYED
   */
  GtkUpdateType update_policy;
  
  /* The button currently pressed or 0 if none */
  guint8 button;
  
  /* Dimensions of knob components */
  gfloat furrow_radius, dot_radius, xofs, yofs;
  gfloat arc_start, arc_dist;

  /* ID of update timer for delayed updates, or 0 if none */
  guint timer;
  
  /* Current angle of the pointer */
  gdouble angle_range;

  /* user input */
  gfloat pangle;
  gfloat px, py;
  
  /* Old values from GtkAdjustment, stored so we know when something changed */
  gdouble old_value;
  gdouble old_lower;
  gdouble old_upper;
  gdouble old_page_size;
  
  /* The adjustment object that stores the data for this knob */
  GtkObject *adjustment;
  GdkWindow *iwindow;
  GdkPixbuf *pixbuf;
};
struct _BstKnobClass
{
  GtkImageClass parent_class;
};
  
/* --- public methods --- */
GType	       bst_knob_get_type	       (void);
GtkWidget*     bst_knob_new                    (GtkAdjustment *adjustment);
void           bst_knob_set_adjustment         (BstKnob       *knob,
                                                GtkAdjustment *adjustment);
GtkAdjustment* bst_knob_get_adjustment         (BstKnob       *knob);
void           bst_knob_set_update_policy      (BstKnob       *knob,
                                                GtkUpdateType  policy);
G_END_DECLS

#endif /* __BST_KNOB_H__ */
