#!/bin/bash

GLE=/usr/src/gle/gle
SOURCES="gtkwrapbox gtkcluehunter gtkhwrapbox gtkvwrapbox"

# pull together glewidgets.h body
echo >glewidgets.tmp
for i in $SOURCES; do
	cat $GLE/$i.h >>glewidgets.tmp
done

# put guarding header with includes
cat >glewidgets.h <<EOF
#ifndef __GLE_WIDGETS_H__
#define __GLE_WIDGETS_H__
#include <gtk/gtk.h>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
EOF

# disable includes and put body
sed >>glewidgets.h <glewidgets.tmp -e 's,^\(#include.*\),/* \1 */,'

# put special feature prototypes
cat >>glewidgets.h <<EOF
void gtk_idle_show_widget (GtkWidget *widget);
void gtk_file_selection_heal (GtkFileSelection *fs);
EOF

# put trailer
cat >>glewidgets.h <<EOF
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __GLE_WIDGETS_H__ */
EOF

# pull together glewidgets.c body
echo >glewidgets.tmp
for i in $SOURCES; do
	sed <$GLE/$i.c >>glewidgets.tmp \
	  -e 's,parent_class,parent_'$i'_class,g' \
	  -e 's,\([^_]ARG_\),\1_'$i'_,g' \
	  -e 's,gtk_widget_get_child_requisition,gtk_widget_get__child_requisition,g' \
	  -e 's,get_child_requisition,get_'$i'_child_requisition,g' \
	  -e 's,gtk_widget_get__child_requisition,gtk_widget_get_child_requisition,g' \
	  -e 's,get_layout_size,get_'$i'_layout_size,g' \
	  -e 's,Line,Line_'$i'_,g'
done

# put header with includes
cat >glewidgets.c <<EOF
#include "glewidgets.h"
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
EOF

# disable includes and put body
sed >>glewidgets.c <glewidgets.tmp -e 's,^\(#include.*\),/* \1 */,'

# remove temporary
rm glewidgets.tmp

# put special feature functions
cat >>glewidgets.c <<EOF
void
gtk_file_selection_heal (GtkFileSelection *fs)
{
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *any;

  g_return_if_fail (fs != NULL);
  g_return_if_fail (GTK_IS_FILE_SELECTION (fs));

  /* button placement
   */
  gtk_container_set_border_width (GTK_CONTAINER (fs), 0);
  gtk_file_selection_hide_fileop_buttons (fs);
  gtk_widget_ref (fs->main_vbox);
  gtk_container_remove (GTK_CONTAINER (fs), fs->main_vbox);
  gtk_box_set_spacing (GTK_BOX (fs->main_vbox), 0);
  gtk_container_set_border_width (GTK_CONTAINER (fs->main_vbox), 5);
  main_vbox =
    gtk_widget_new (GTK_TYPE_VBOX,
		    "homogeneous", FALSE,
		    "spacing", 0,
		    "border_width", 0,
		    "parent", fs,
		    "visible", TRUE,
		    NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), fs->main_vbox, TRUE, TRUE, 0);
  gtk_widget_unref (fs->main_vbox);
  gtk_widget_hide (fs->ok_button->parent);
  hbox =
    gtk_widget_new (GTK_TYPE_HBOX,
		    "homogeneous", TRUE,
		    "spacing", 0,
		    "border_width", 5,
		    "visible", TRUE,
		    NULL);
  gtk_box_pack_end (GTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_reparent (fs->ok_button, hbox);
  gtk_widget_reparent (fs->cancel_button, hbox);
  gtk_widget_grab_default (fs->ok_button);
  gtk_label_set_text (GTK_LABEL (GTK_BIN (fs->ok_button)->child), "Ok");
  gtk_label_set_text (GTK_LABEL (GTK_BIN (fs->cancel_button)->child), "Cancel");

  /* heal the action_area packing so we can customize children
   */
  gtk_box_set_child_packing (GTK_BOX (fs->action_area->parent),
			     fs->action_area,
			     FALSE, TRUE,
			     5, GTK_PACK_START);

  any =
    gtk_widget_new (gtk_hseparator_get_type (),
		    "GtkWidget::visible", TRUE,
		    NULL);
  gtk_box_pack_end (GTK_BOX (main_vbox), any, FALSE, TRUE, 0);
  gtk_widget_grab_focus (fs->selection_entry);
}

static gint
idle_shower (GtkWidget **widget_p)
{
  GDK_THREADS_ENTER ();
  
  if (GTK_IS_WIDGET (*widget_p) && !GTK_OBJECT_DESTROYED (*widget_p))
    {
      gtk_signal_disconnect_by_func (GTK_OBJECT (*widget_p),
				     GTK_SIGNAL_FUNC (gtk_widget_destroyed),
				     widget_p);
      gtk_widget_show (*widget_p);
    }

  g_free (widget_p);

  GDK_THREADS_LEAVE ();

  return FALSE;
}

void
gtk_idle_show_widget (GtkWidget *widget)
{
  GtkWidget **widget_p;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  if (GTK_OBJECT_DESTROYED (widget))
    return;

  widget_p = g_new (GtkWidget*, 1);
  *widget_p = widget;
  gtk_signal_connect (GTK_OBJECT (widget),
		      "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed),
		      widget_p);
  gtk_idle_add_priority (G_PRIORITY_LOW, (GtkFunction) idle_shower, widget_p);
}

EOF
