/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999-2002 Tim Janik
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
#ifndef __BST_KNOB_H__
#define __BST_KNOB_H__

#include <gtk/gtkadjustment.h>
#include <gtk/gtkimage.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define BST_TYPE_KNOB			(bst_knob_get_type ())
#define BST_KNOB(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_KNOB, BstKnob))
#define BST_KNOB_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_KNOB, BstKnobClass))
#define BST_IS_KNOB(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_KNOB))
#define BST_IS_KNOB_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_KNOB))
#define BST_KNOB_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BST_TYPE_KNOB, BstKnobClass))


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
  

GtkType	       bst_knob_get_type	       (void);
GtkWidget*     bst_knob_new                    (GtkAdjustment *adjustment);
void           bst_knob_set_adjustment         (BstKnob       *knob,
                                                GtkAdjustment *adjustment);
GtkAdjustment* bst_knob_get_adjustment         (BstKnob       *knob);
void           bst_knob_set_update_policy      (BstKnob       *knob,
                                                GtkUpdateType  policy);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_KNOB_H__ */

