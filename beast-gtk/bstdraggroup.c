/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000, 2001 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */


typedef struct _BstDragGroup BstDragGroup;
struct _BstDragGroup
{
  BsePatternGroup *pattern_group;
  BseSong         *song;
  guint            position;      /* monitored from song */
  GtkWidget       *widget;        /* to add to parent */
  GtkWidget       *name;
  GtkWidget       *hbox;
  guint		   ignore_first_insert : 1;
};

/* a drag group supports BST_DRAG_BUTTON_COPY (+link),
 * BST_DRAG_BUTTON_MOVE and BST_DRAG_BUTTON_CONTEXT out
 * of the box.
 * it monitors ::pattern_group_inserted and ::pattern_group_removed
 * on the song to update its position field and to auto-destruct.
 * it also monitors ::set_parent on the pattern_group for
 * auto-destruction.
 * it also updates its pattern widgets, according to
 * ::pattern_inserted and ::pattern_removed on the pattern_group.
 * furthermore, it accepts arbitrary drops of drag patterns and
 * updates pattern_group according to those drops.
 */


/* --- prototypes --- */
static BstDragGroup*   bst_drag_group_new      (BsePatternGroup        *pattern_group,
						guint                   position,
						gboolean                ignore_first_insert);
static void            bst_drag_group_destroy  (BstDragGroup           *drag_group);
static gboolean	       drag_group_drag_motion  (GtkWidget              *group_list,
						GdkDragContext         *context,
						gint                    x,
						gint                    y,
						guint                   time,
						BstDragGroup	       *drag_group);
static void	       drag_group_drag_leave   (GtkWidget              *group_list,
						GdkDragContext         *context,
						guint                   time,
						BstDragGroup           *drag_group);
static gboolean	       drag_group_drag_drop	(GtkWidget              *group_list,
						 GdkDragContext         *context,
						 gint                    x,
						 gint                    y,
						 guint                   time,
						 BstDragGroup           *drag_group);
static void	drag_group_drag_data_received  (GtkWidget              *group_list,
						GdkDragContext         *context,
						gint                    x,
						gint                    y,
						GtkSelectionData       *selection_data,
						guint                   info,
						guint                   time,
						BstDragGroup           *drag_group);


/* --- functions --- */
static void
drag_group_changed (BstDragGroup *drag_group)
{
  if (drag_group->name)
    gtk_label_set_text (GTK_LABEL (drag_group->name), bsw_item_get_name (BSE_OBJECT_ID (drag_group->pattern_group)));
}

static void
drag_group_pattern_group_inserted (BseSong         *song,
				   BsePatternGroup *pgroup,
				   guint            position,
				   BstDragGroup    *drag_group)
{
  g_return_if_fail (drag_group != NULL);
  g_return_if_fail (drag_group->song == song);

  if (drag_group->ignore_first_insert)
    drag_group->ignore_first_insert = FALSE;
  else if (position <= drag_group->position)
    drag_group->position++;

  g_return_if_fail (song->pgroups[drag_group->position] == drag_group->pattern_group);
}

static void
drag_group_pattern_group_removed (BseSong         *song,
				  BsePatternGroup *pgroup,
				  guint            position,
				  BstDragGroup    *drag_group)
{
  g_return_if_fail (drag_group != NULL);
  g_return_if_fail (drag_group->song == song);

  if (position < drag_group->position)
    {
      drag_group->position--;

      g_return_if_fail (song->pgroups[drag_group->position] == drag_group->pattern_group);
    }
  else if (position == drag_group->position)
    {
      g_return_if_fail (drag_group->pattern_group == pgroup);

      bst_drag_group_destroy (drag_group);
    }
}

static void
drag_group_pattern_inserted (BsePatternGroup *pattern_group,
			     BsePattern      *pattern,
			     guint            position,
			     BstDragGroup    *drag_group)
{
  BstDragPattern *drag_pattern;

  g_return_if_fail (drag_group != NULL);
  g_return_if_fail (drag_group->pattern_group == pattern_group);

  drag_pattern = bst_drag_pattern_new (pattern, pattern_group, position, FALSE); /* ignore_first_insert: TRUE */
  
  gtk_container_add_with_properties (GTK_CONTAINER (drag_group->hbox), drag_pattern->widget,
				     "expand", FALSE,
				     "position", position,
				     NULL);
}

static void
bst_drag_group_destroy (BstDragGroup *drag_group)
{
  g_return_if_fail (drag_group != NULL);

  gtk_object_remove_no_notify (GTK_OBJECT (drag_group->widget), "BstDragGroup");
  gtk_signal_disconnect_by_func (GTK_OBJECT (drag_group->widget),
				 G_CALLBACK (bst_drag_group_destroy),
				 drag_group);
  
  g_object_disconnect (drag_group->pattern_group,
		       "any_signal", drag_group_changed, drag_group,
		       "any_signal", bst_drag_group_destroy, drag_group,
		       "any_signal", drag_group_pattern_inserted, drag_group,
		       NULL);
  g_object_disconnect (drag_group->song,
		       "any_signal", drag_group_pattern_group_inserted, drag_group,
		       "any_signal", drag_group_pattern_group_removed, drag_group,
		       NULL);
  
  gtk_widget_destroy (drag_group->widget);
  g_free (drag_group);
}

static void
drag_group_drag_begin (GtkWidget      *widget,
		       GdkDragContext *context,
		       BstDragGroup   *drag_group)
{
  g_return_if_fail (drag_group != NULL);
  g_return_if_fail (drag_group->widget == widget);

  gtk_drag_set_icon_widget (context, bst_play_list_drag_window_pattern_group_icon, 0, 0);
}

static void
drag_group_drag_data_delete (GtkWidget      *widget,
			     GdkDragContext *context,
			     BstDragGroup   *drag_group)
{
  g_return_if_fail (drag_group != NULL);
  g_return_if_fail (drag_group->widget == widget);

  bse_song_remove_pattern_group_entry (drag_group->song, drag_group->position);
}

static void
drag_group_drag_data_get (GtkWidget        *widget,
			  GdkDragContext   *context,
			  GtkSelectionData *selection_data,
			  guint             info,
			  guint             time,
			  BstDragGroup     *drag_group)
{
  g_return_if_fail (drag_group != NULL);
  g_return_if_fail (drag_group->widget == widget);

  gtk_selection_data_set (selection_data,
			  atom_beast_pattern_group_pointer,
			  8,
			  (gpointer) &drag_group->pattern_group,
			  sizeof (drag_group->pattern_group));
}

static gint
drag_group_button_press (GtkWidget       *widget,
			 GdkEventButton  *event,
			 BstDragGroup     *drag_group)
{
  if (event->type == GDK_BUTTON_PRESS &&
      (event->button == BST_DRAG_BUTTON_COPY ||
       event->button == BST_DRAG_BUTTON_MOVE))
    {
      static const GtkTargetEntry target = { "_BEAST_PATTERN_GROUP_POINTER", };
      GtkTargetList *tlist = gtk_target_list_new (&target, 1);
      
      gtk_drag_begin (widget, tlist,
		      (event->button == BST_DRAG_BUTTON_COPY) * (GDK_ACTION_COPY | GDK_ACTION_LINK) |
		      (event->button == BST_DRAG_BUTTON_MOVE) * GDK_ACTION_MOVE,
		      event->button, (GdkEvent*) event);
      gtk_target_list_unref (tlist);

      if (event->button == BST_DRAG_BUTTON_MOVE)
	gtk_widget_hide (widget);
      
      return TRUE;
    }
  else if (event->type == GDK_BUTTON_PRESS && event->button == BST_DRAG_BUTTON_CONTEXT)
    {
      GtkWidget *choice;
      gchar *string = g_strconcat ("Group: ", bsw_item_get_name (BSE_OBJECT_ID (drag_group->pattern_group)), NULL);

      choice = bst_choice_menu_createv ("<BEAST-DragGroup>/Popup",
					BST_CHOICE_TITLE (string),
					BST_CHOICE_SEPERATOR,
					BST_CHOICE (2, "Duplicate", NONE),
					BST_CHOICE (3, "Duplicate-Link", NONE),
					BST_CHOICE_SEPERATOR,
					BST_CHOICE_S (1, "Delete", DELETE, drag_group->song->n_pgroups > 1),
					BST_CHOICE_END);
      g_free (string);
      switch (bst_choice_modal (choice, event->button, event->time))
	{
	case 1:
	  bse_song_remove_pattern_group_entry (drag_group->song, drag_group->position);
	  break;
	case 2:
	  bse_song_insert_pattern_group_copy (drag_group->song, drag_group->pattern_group, drag_group->position + 1);
	  break;
	case 3:
	  bse_song_insert_pattern_group_link (drag_group->song, drag_group->pattern_group, drag_group->position + 1);
	  break;
	}
      bst_choice_destroy (choice);
      
      return TRUE;
    }
  
  return FALSE;
}

static BstDragGroup*
bst_drag_group_new (BsePatternGroup *pattern_group,
		    guint            position,
		    gboolean         ignore_first_insert)
{
  static const GtkTargetEntry target = { "_BEAST_PATTERN_POINTER", GTK_TARGET_SAME_APP, 0 };
  BseSong *song;
  GtkWidget *frame, *hbox, *any;
  BstDragGroup *drag_group;
  guint i;

  g_return_val_if_fail (BSE_IS_PATTERN_GROUP (pattern_group), NULL);
  song = BSE_SONG (BSE_ITEM (pattern_group)->parent);
  g_return_val_if_fail (position < song->n_pgroups, NULL);
  g_return_val_if_fail (song->pgroups[position] == pattern_group, NULL);
  
  drag_group = g_new0 (BstDragGroup, 1);
  drag_group->pattern_group = pattern_group;
  g_object_connect (BSE_OBJECT (pattern_group),
		    "swapped_signal::notify", drag_group_changed, drag_group,
		    "swapped_signal::set_parent", bst_drag_group_destroy, drag_group,
		    "signal::pattern_inserted", drag_group_pattern_inserted, drag_group,
		    NULL);
  
  drag_group->song = song;
  drag_group->position = position;
  g_object_connect (BSE_OBJECT (song),
		    "signal::pattern_group_inserted", drag_group_pattern_group_inserted, drag_group,
		    "signal::pattern_group_removed", drag_group_pattern_group_removed, drag_group,
		    NULL);
  drag_group->ignore_first_insert = ignore_first_insert;
  
  /* build outer containers
   */
  drag_group->widget = g_object_connect (gtk_widget_new (GTK_TYPE_EVENT_BOX,
							 "visible", TRUE,
							 NULL),
					 "swapped_signal::destroy", bst_drag_group_destroy, drag_group,
					 NULL);
  gtk_object_set_data_full (GTK_OBJECT (drag_group->widget),
			    "BstDragGroup",
			    drag_group,
			    (GtkDestroyNotify) bst_drag_group_destroy);
  frame = gtk_widget_new (GTK_TYPE_FRAME,
			  "visible", TRUE,
			  "shadow", GTK_SHADOW_OUT,
			  "parent", drag_group->widget,
			  NULL);
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "spacing", 0,
			 "parent", frame,
			 NULL);

  /* pattern name widget
   */
  drag_group->name = g_object_connect (gtk_widget_new (GTK_TYPE_LABEL,
						       "visible", TRUE,
						       "xalign", 0.0,
						       NULL),
				       "swapped_signal::destroy", bse_nullify_pointer, &drag_group->name,
				       NULL);
  gtk_box_pack_start (GTK_BOX (hbox), drag_group->name, FALSE, TRUE, 5);

  
  /* seperator and pattern container
   */
  any = gtk_widget_new (GTK_TYPE_VSEPARATOR,
			"visible", TRUE,
			NULL);
  gtk_box_pack_start (GTK_BOX (hbox), any, FALSE, TRUE, 0);
  drag_group->hbox = g_object_connect (gtk_widget_new (GTK_TYPE_HBOX,
						       "visible", TRUE,
						       "parent", hbox,
						       NULL),
				       "swapped_signal::destroy", bse_nullify_pointer, &drag_group->hbox,
				       NULL);
  

  /* update contents
   */
  drag_group_changed (drag_group);
  for (i = 0; i < pattern_group->n_entries; i++)
    {
      BstDragPattern *drag_pattern = bst_drag_pattern_new (pattern_group->entries[i].pattern,
							   pattern_group, i, FALSE);

      gtk_container_add_with_properties (GTK_CONTAINER (drag_group->hbox), drag_pattern->widget,
					 "expand", FALSE,
					 "position", i,
					 NULL);
    }
  
  /* setup as drag source
   */
  g_object_connect (drag_group->widget,
		    "signal::button_press_event", drag_group_button_press, drag_group,
		    "signal::drag_begin", drag_group_drag_begin, drag_group,
		    "signal::drag_data_get", drag_group_drag_data_get, drag_group,
		    "signal::drag_data_delete", drag_group_drag_data_delete, drag_group,
		    "signal::drag_end", gtk_widget_show, NULL,
		    NULL);

  /* setup as drag destination
   */
  gtk_drag_dest_set (drag_group->hbox,
		     0,
		     &target, 1,
		     GDK_ACTION_COPY | GDK_ACTION_MOVE);
  g_object_connect (drag_group->hbox,
		    "signal::drag_motion", drag_group_drag_motion, drag_group,
		    "signal::drag_leave", drag_group_drag_leave, drag_group,
		    "signal::drag_data_received", drag_group_drag_data_received, drag_group,
		    "signal::drag_drop", drag_group_drag_drop, drag_group,
		    NULL);

  return drag_group;
}

static gboolean
drag_group_drag_motion (GtkWidget      *hbox,
			GdkDragContext *context,
			gint            x,
			gint            y,
			guint           time,
			BstDragGroup   *drag_group)
{
  GtkWidget *drag_source = gtk_drag_get_source_widget (context);
  GtkWidget *plist = gtk_widget_get_ancestor (hbox, BST_TYPE_PLAY_LIST);

  if (drag_source &&                                            /* check SAME_APP */
      gtk_widget_is_ancestor (drag_source, plist) &&		/* check widget branch */
      context->targets &&                                       /* check target type */
      context->targets->data == atom_beast_pattern_pointer)
    {
      guint action = context->actions & context->suggested_action;
      gint old_pos, new_pos;

      if (action & GDK_ACTION_COPY)
	gdk_drag_status (context, action & GDK_ACTION_COPY, time);
      else
	gdk_drag_status (context, action & GDK_ACTION_MOVE, time);

      if (action & (GDK_ACTION_COPY | GDK_ACTION_MOVE))
	gtk_widget_show (bst_play_list_drop_spot_pattern);
      else
	gtk_widget_hide (bst_play_list_drop_spot_pattern);

      if (!bst_play_list_drop_spot_pattern->parent)
	gtk_box_pack_start (GTK_BOX (hbox), bst_play_list_drop_spot_pattern, FALSE, TRUE, 1);

      new_pos = bst_container_get_insertion_position (GTK_CONTAINER (hbox),
						      TRUE, x,
						      bst_play_list_drop_spot_pattern,
						      &old_pos);
      if (new_pos != old_pos)
	gtk_box_reorder_child (GTK_BOX (hbox), bst_play_list_drop_spot_pattern, new_pos);

      return TRUE;
    }

  container_remove (bst_play_list_drop_spot_pattern);

  return FALSE;
}

static void
drag_group_drag_leave (GtkWidget      *hbox,
		       GdkDragContext *context,
		       guint           time,
		       BstDragGroup   *drag_group)
{
  container_remove (bst_play_list_drop_spot_pattern);
}

static gboolean
drag_group_drag_drop (GtkWidget      *hbox,
		      GdkDragContext *context,
		      gint            x,
		      gint            y,
		      guint           time,
		      BstDragGroup   *drag_group)
{
  GtkWidget *drag_source = gtk_drag_get_source_widget (context);
  GtkWidget *plist = gtk_widget_get_ancestor (hbox, BST_TYPE_PLAY_LIST);

  if (drag_source &&                                            /* check SAME_APP */
      gtk_widget_is_ancestor (drag_source, plist) &&            /* check widget branch */
      context->targets &&                                       /* check target type */
      context->targets->data == atom_beast_pattern_pointer)
    {
      gtk_drag_get_data (hbox, context, context->targets->data, time);

      return TRUE;
    }

  return FALSE;
}

static void
drag_group_drag_data_received (GtkWidget        *hbox,
			       GdkDragContext   *context,
			       gint              x,
			       gint              y,
			       GtkSelectionData *selection_data,
			       guint             info,
			       guint             time,
			       BstDragGroup     *drag_group)
{
  gpointer drag_contents;
  gint position;
  guint action;

  g_return_if_fail (selection_data->type == atom_beast_pattern_pointer);
  g_return_if_fail (selection_data->format == 8);
  g_return_if_fail (selection_data->length == sizeof (gpointer));

  action = context->actions & context->suggested_action;
  g_return_if_fail ((action & (GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK)) != 0);

  drag_contents = *((gpointer*) selection_data->data);
  g_return_if_fail (BSE_IS_PATTERN (drag_contents));

  position = bst_container_get_insertion_position (GTK_CONTAINER (hbox),
						   TRUE, x,
						   bst_play_list_drop_spot_pattern, NULL);

  if (bst_play_list_drop_spot_pattern->parent) // FIXME GTKFIX
    {
      container_remove (bst_play_list_drop_spot_pattern);
      if (bst_play_list_drop_spot_pattern->parent)
	gtk_box_reorder_child (GTK_BOX (bst_play_list_drop_spot_pattern->parent),
			       bst_play_list_drop_spot_pattern, -1);
    }

  if (context->actions & (GDK_ACTION_COPY | GDK_ACTION_MOVE))
    bse_pattern_group_insert_pattern (drag_group->pattern_group, drag_contents, position);
}
