#ifndef __GTK_DIAL_H__
#define __GTK_DIAL_H__

#include <gtk/gtkadjustment.h>
#include <gtk/gtkwidget.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_DIAL			(gtk_dial_get_type ())
#define GTK_DIAL(object)		(GTK_CHECK_CAST ((object), GTK_TYPE_DIAL, GtkDial))
#define GTK_DIAL_CLASS(klass)		(GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_DIAL, GtkDialClass))
#define GTK_IS_DIAL(object)		(GTK_CHECK_TYPE ((object), GTK_TYPE_DIAL))
#define GTK_IS_DIAL_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_DIAL))
#define GTK_DIAL_GET_CLASS(object)	((GtkDialClass*) (((GtkObject*) (object))->klass))


typedef struct _GtkDial	     GtkDial;
typedef struct _GtkDialClass GtkDialClass;
struct _GtkDial
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
  gfloat angle;
  
  /* Old values from GtkAdjustment, stored so we know when something changed */
  gfloat old_value;
  gfloat old_lower;
  gfloat old_upper;
  
  /* The adjustment object that stores the data for this dial */
  GtkObject *adjustment;
};
struct _GtkDialClass
{
  GtkWidgetClass parent_class;
};
  

GtkType	       gtk_dial_get_type	       (void);
GtkWidget*     gtk_dial_new                    (GtkAdjustment *adjustment);
void           gtk_dial_set_adjustment         (GtkDial       *dial,
                                                GtkAdjustment *adjustment);
GtkAdjustment* gtk_dial_get_adjustment         (GtkDial       *dial);
void           gtk_dial_set_update_policy      (GtkDial       *dial,
                                                GtkUpdateType  policy);
void	       bst_dial_set_align_widget       (GtkDial	      *dial,
						GtkWidget     *widget,
						gboolean       width_align,
						gboolean       height_align);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GTK_DIAL_H__ */

