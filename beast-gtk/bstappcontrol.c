/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "bstappcontrol.h"

#include <math.h>


/* --- functions --- */
BstAppControl*
bst_app_control_new (void)
{
  BstAppControl *self = g_new0 (BstAppControl, 1);
  GtkWidget *box, *frame;
  GtkSizeGroup *sgroup;
  guint spaceing = 0;
  self->box = box = g_object_new (GTK_TYPE_HBOX,
				  NULL);
  g_object_set_data_full (box, "BstAppControl", self, g_free);
  
  self->power = g_object_new (GTK_TYPE_BUTTON,
			      "child", gxk_polygon_new (&gxk_polygon_power),
			      "can_focus", FALSE,
			      NULL);
  gtk_box_pack_start (GTK_BOX (box), self->power, FALSE, FALSE, spaceing);
  self->led = gxk_led_new (GXK_LED_OFF);
  frame = g_object_new (GTK_TYPE_FRAME,
			"shadow_type", GTK_SHADOW_ETCHED_OUT,
			"child", self->led,
			NULL);
  gxk_led_set_border_width (self->led, 1);
  gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, spaceing);
  sgroup = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
  gtk_size_group_add_widget (sgroup, GTK_WIDGET (self->led));
  gtk_size_group_add_widget (sgroup, self->power);
  g_object_unref (sgroup);
  self->stop = g_object_new (GTK_TYPE_BUTTON,
			     "child", gxk_polygon_new (&gxk_polygon_stop),
			     "can_focus", FALSE,
			     NULL);
  gtk_box_pack_start (GTK_BOX (box), self->stop, FALSE, FALSE, spaceing);
  self->first = g_object_new (GTK_TYPE_BUTTON,
			      "child", gxk_polygon_new (&gxk_polygon_first),
			      "can_focus", FALSE,
			      NULL);
  gtk_box_pack_start (GTK_BOX (box), self->first, FALSE, FALSE, spaceing);
  self->prev = g_object_new (GTK_TYPE_BUTTON,
			     "child", gxk_polygon_new (&gxk_polygon_previous),
			     "can_focus", FALSE,
			     NULL);
  gtk_box_pack_start (GTK_BOX (box), self->prev, FALSE, FALSE, spaceing);
  self->rew = g_object_new (GTK_TYPE_BUTTON,
			    "child", gxk_polygon_new (&gxk_polygon_rewind),
			    "can_focus", FALSE,
			    NULL);
  gtk_box_pack_start (GTK_BOX (box), self->rew, FALSE, FALSE, spaceing);
  self->play = g_object_new (GTK_TYPE_BUTTON,
			     "child", gxk_polygon_new (&gxk_polygon_play),
			     "can_focus", FALSE,
			     NULL);
  gtk_box_pack_start (GTK_BOX (box), self->play, FALSE, FALSE, spaceing);
  self->pause = g_object_new (GTK_TYPE_BUTTON,
			      "child", gxk_polygon_new (&gxk_polygon_pause),
			      "can_focus", FALSE,
			      NULL);
  gtk_box_pack_start (GTK_BOX (box), self->pause, FALSE, FALSE, spaceing);
  self->fwd = g_object_new (GTK_TYPE_BUTTON,
			    "child", gxk_polygon_new (&gxk_polygon_forward),
			    "can_focus", FALSE,
			    NULL);
  gtk_box_pack_start (GTK_BOX (box), self->fwd, FALSE, FALSE, spaceing);
  self->next = g_object_new (GTK_TYPE_BUTTON,
			     "child", gxk_polygon_new (&gxk_polygon_next),
			     "can_focus", FALSE,
			     NULL);
  gtk_box_pack_start (GTK_BOX (box), self->next, FALSE, FALSE, spaceing);
  self->last = g_object_new (GTK_TYPE_BUTTON,
			     "child", gxk_polygon_new (&gxk_polygon_last),
			     "can_focus", FALSE,
			     NULL);
  gtk_box_pack_start (GTK_BOX (box), self->last, FALSE, FALSE, spaceing);
  gtk_widget_show_all (box);
  gtk_widget_set_sensitive (box, FALSE);
  return self;
}
