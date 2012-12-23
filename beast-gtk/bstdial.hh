// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_DIAL_H__
#define __BST_DIAL_H__

#include <gtk/gtkadjustment.h>
#include <gtk/gtkwidget.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define BST_TYPE_DIAL			(bst_dial_get_type ())
#define BST_DIAL(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_DIAL, BstDial))
#define BST_DIAL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_DIAL, BstDialClass))
#define BST_IS_DIAL(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_DIAL))
#define BST_IS_DIAL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_DIAL))
#define BST_DIAL_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BST_TYPE_DIAL, BstDialClass))


typedef struct _BstDial	     BstDial;
typedef struct _BstDialClass BstDialClass;
struct _BstDial
{
  GtkWidget parent_object;
  
  /* The update policy: GTK_UPDATE_CONTINUOUS,
   * GTK_UPDATE_DISCONTINUOUS or GTK_UPDATE_DELAYED
   */
  GtkUpdateType update_policy;
  
  GtkWidget *align_widget;
  guint      align_width : 1;
  
  /* The button currently pressed or 0 if none */
  guint8 button;
  
  /* Dimensions of dial components */
  gint radius;
  gint pointer_width;
  
  /* ID of update timer for delayed updates, or 0 if none */
  guint timer;
  
  /* Current angle of the pointer */
  gdouble angle;
  
  /* Old values from GtkAdjustment, stored so we know when something changed */
  gdouble old_value;
  gdouble old_lower;
  gdouble old_upper;
  gdouble old_page_size;
  
  /* The adjustment object that stores the data for this dial */
  GtkObject *adjustment;
};
struct _BstDialClass
{
  GtkWidgetClass parent_class;
};
  

GtkType	       bst_dial_get_type	       (void);
GtkWidget*     bst_dial_new                    (GtkAdjustment *adjustment);
void           bst_dial_set_adjustment         (BstDial       *dial,
                                                GtkAdjustment *adjustment);
GtkAdjustment* bst_dial_get_adjustment         (BstDial       *dial);
void           bst_dial_set_update_policy      (BstDial       *dial,
                                                GtkUpdateType  policy);
void	       bst_dial_set_align_widget       (BstDial	      *dial,
						GtkWidget     *widget,
						gboolean       width_align,
						gboolean       height_align);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_DIAL_H__ */

