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


typedef struct _BstDragPattern BstDragPattern;
struct _BstDragPattern
{
  BsePattern      *pattern;
  BsePatternGroup *pattern_group; /* optional */
  guint            position;      /* monitored from pgroup */
  GtkWidget       *widget;        /* to add to parent */
  GtkWidget       *seq_id;
  GtkWidget       *name;
  guint            ignore_first_insert : 1;
};

/* a drag pattern supports DRAG_BUTTON_COPY out of
 * the box, and features DRAG_BUTTON_MOVE and
 * DRAG_BUTTON_CONTEXT if pattern_group is given.
 * it monitors ::pattern_inserted and ::pattern_removed
 * on the pgroup to update its position field and to auto-destruct.
 * it also monitors ::set_parent on the pattern for
 * auto-destruction.
 */


/* --- prototypes --- */
static BstDragPattern* bst_drag_pattern_new     (BsePattern      *pattern,
						 BsePatternGroup *pattern_group,
						 guint            position,
						 gboolean         ignore_first_insert);
static void            bst_drag_pattern_destroy (BstDragPattern  *drag_pattern);


/* --- functions --- */
static void
drag_pattern_changed (BstDragPattern *drag_pattern)
{
  if (drag_pattern->seq_id)
    {
      gchar *string = g_strdup_printf ("%03u", bse_item_get_seqid (BSE_ITEM (drag_pattern->pattern)));

      gtk_label_set_text (GTK_LABEL (drag_pattern->seq_id), string);
      g_free (string);
    }

  if (drag_pattern->name)
    gtk_label_set_text (GTK_LABEL (drag_pattern->name), bsw_item_get_name (BSE_OBJECT_ID (drag_pattern->pattern)));
}

static void
drag_pattern_pattern_inserted (BsePatternGroup *pgroup,
			       BsePattern      *pattern,
			       guint            position,
			       BstDragPattern  *drag_pattern)
{
  g_return_if_fail (drag_pattern != NULL);
  g_return_if_fail (drag_pattern->pattern_group == pgroup);

  if (drag_pattern->ignore_first_insert)
    drag_pattern->ignore_first_insert = FALSE;
  else if (position <= drag_pattern->position)
    drag_pattern->position++;

  g_return_if_fail (pgroup->entries[drag_pattern->position].pattern == drag_pattern->pattern);
}

static void
drag_pattern_pattern_removed (BsePatternGroup *pgroup,
			      BsePattern      *pattern,
			      guint            position,
			      BstDragPattern  *drag_pattern)
{
  g_return_if_fail (drag_pattern != NULL);
  g_return_if_fail (drag_pattern->pattern_group == pgroup);

  if (position < drag_pattern->position)
    {
      drag_pattern->position--;

      g_return_if_fail (pgroup->entries[drag_pattern->position].pattern == drag_pattern->pattern);
    }
  else if (position == drag_pattern->position)
    {
      g_return_if_fail (drag_pattern->pattern == pattern);

      bst_drag_pattern_destroy (drag_pattern);
    }
}

static void
bst_drag_pattern_destroy (BstDragPattern *drag_pattern)
{
  g_return_if_fail (drag_pattern != NULL);

  gtk_object_remove_no_notify (GTK_OBJECT (drag_pattern->widget), "BstDragPattern");
  gtk_signal_disconnect_by_func (GTK_OBJECT (drag_pattern->widget),
				 G_CALLBACK (bst_drag_pattern_destroy),
				 drag_pattern);
  
  g_object_disconnect (drag_pattern->pattern,
		       "any_signal", drag_pattern_changed, drag_pattern,
		       "any_signal", bst_drag_pattern_destroy, drag_pattern,
		       NULL);
  if (drag_pattern->pattern_group)
    g_object_disconnect (drag_pattern->pattern_group,
			 "any_signal", drag_pattern_pattern_inserted, drag_pattern,
			 "any_signal", drag_pattern_pattern_removed, drag_pattern,
			 NULL);
  
  gtk_widget_destroy (drag_pattern->widget);
  g_free (drag_pattern);
}

static void
drag_pattern_drag_begin (GtkWidget      *widget,
			 GdkDragContext *context,
			 BstDragPattern *drag_pattern)
{
  g_return_if_fail (drag_pattern != NULL);
  g_return_if_fail (drag_pattern->widget == widget);

  gtk_drag_set_icon_widget (context, bst_play_list_drag_window_pattern_icon, 0, 0);
}

static void
drag_pattern_drag_data_delete (GtkWidget      *widget,
			       GdkDragContext *context,
			       BstDragPattern *drag_pattern)
{
  g_return_if_fail (drag_pattern != NULL);
  g_return_if_fail (drag_pattern->widget == widget);

  bse_pattern_group_remove_entry (drag_pattern->pattern_group, drag_pattern->position);
}

static void
drag_pattern_drag_data_get (GtkWidget        *widget,
			    GdkDragContext   *context,
			    GtkSelectionData *selection_data,
			    guint             info,
			    guint             time,
			    BstDragPattern   *drag_pattern)
{
  g_return_if_fail (drag_pattern != NULL);
  g_return_if_fail (drag_pattern->widget == widget);


  gtk_selection_data_set (selection_data,
			  atom_beast_pattern_pointer,
			  8,
			  (gpointer) &drag_pattern->pattern,
			  sizeof (drag_pattern->pattern));
}

static gint
drag_pattern_button_press (GtkWidget       *widget,
			   GdkEventButton  *event,
			   BstDragPattern  *drag_pattern)
{
  if (event->type == GDK_BUTTON_PRESS &&
      (event->button == BST_DRAG_BUTTON_COPY ||
       (drag_pattern->pattern_group &&  event->button == BST_DRAG_BUTTON_MOVE)))
    {
      static const GtkTargetEntry target = { "_BEAST_PATTERN_POINTER", };
      GtkTargetList *tlist = gtk_target_list_new (&target, 1);
      
      gtk_drag_begin (widget, tlist,
		      (event->button == BST_DRAG_BUTTON_COPY) * GDK_ACTION_COPY |
		      (drag_pattern->pattern_group && event->button == BST_DRAG_BUTTON_MOVE) * GDK_ACTION_MOVE,
		      event->button, (GdkEvent*) event);
      gtk_target_list_unref (tlist);

      if (event->button == BST_DRAG_BUTTON_MOVE)
	gtk_widget_hide (widget);
      
      return TRUE;
    }
  else if (event->type == GDK_BUTTON_PRESS &&
	   event->button == BST_DRAG_BUTTON_CONTEXT &&
	   drag_pattern->pattern_group)
    {
      GtkWidget *choice;
      gchar *string = g_strconcat ("Pattern: ", bsw_item_get_name (BSE_OBJECT_ID (drag_pattern->pattern)), NULL);

      choice = bst_choice_menu_createv ("<BEAST-DragPattern>/Popup",
					BST_CHOICE_TITLE (string),
					BST_CHOICE_SEPERATOR,
					BST_CHOICE (2, "Duplicate", NONE),
					BST_CHOICE_SEPERATOR,
					BST_CHOICE (1, "Delete", DELETE),
					BST_CHOICE_END);
      g_free (string);
      switch (bst_choice_modal (choice, event->button, event->time))
	{
	case 1:
	  bse_pattern_group_remove_entry (drag_pattern->pattern_group, drag_pattern->position);
	  break;
	case 2:
	  bse_pattern_group_insert_pattern (drag_pattern->pattern_group, drag_pattern->pattern, drag_pattern->position);
	  break;
	}
      bst_choice_destroy (choice);
      
      return TRUE;
    }
  
  return FALSE;
}

static BstDragPattern*
bst_drag_pattern_new (BsePattern      *pattern,
		      BsePatternGroup *pattern_group,
		      guint            position,
		      gboolean         ignore_first_insert)
{
  GtkWidget *frame, *hbox, *any;
  BstDragPattern *drag_pattern;

  g_return_val_if_fail (BSE_IS_PATTERN (pattern), NULL);

  drag_pattern = g_new0 (BstDragPattern, 1);
  drag_pattern->pattern = pattern;
  g_object_connect (BSE_OBJECT (pattern),
		    "swapped_signal::seqid_changed", drag_pattern_changed, drag_pattern,
		    "swapped_signal::notify", drag_pattern_changed, drag_pattern,
		    "swapped_signal::set_parent", bst_drag_pattern_destroy, drag_pattern,
		    NULL);
  if (pattern_group)
    {
      g_return_val_if_fail (BSE_IS_PATTERN_GROUP (pattern_group), NULL);
      g_return_val_if_fail (position < pattern_group->n_entries, NULL);
      g_return_val_if_fail (pattern_group->entries[position].pattern == pattern, NULL);

      drag_pattern->pattern_group = pattern_group;
      drag_pattern->position = position;
      g_object_connect (BSE_OBJECT (pattern_group),
			"signal::pattern_inserted", drag_pattern_pattern_inserted, drag_pattern,
			"signal::pattern_removed", drag_pattern_pattern_removed, drag_pattern,
			NULL);
      drag_pattern->ignore_first_insert = ignore_first_insert;
    }
  
  /* build outer containers
   */
  drag_pattern->widget = g_object_connect (gtk_widget_new (GTK_TYPE_EVENT_BOX,
							   "visible", TRUE,
							   NULL),
					   "swapped_signal::destroy", bst_drag_pattern_destroy, drag_pattern,
					   NULL);
  gtk_object_set_data_full (GTK_OBJECT (drag_pattern->widget),
			    "BstDragPattern",
			    drag_pattern,
			    (GtkDestroyNotify) bst_drag_pattern_destroy);
  frame = gtk_widget_new (GTK_TYPE_FRAME,
			  "visible", TRUE,
			  "shadow", GTK_SHADOW_OUT,
			  "parent", drag_pattern->widget,
			  NULL);
  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "spacing", 0,
			 "parent", frame,
			 NULL);

  /* sequential id widget
   */
  drag_pattern->seq_id = g_object_connect (gtk_widget_new (GTK_TYPE_LABEL,
							   "visible", TRUE,
							   NULL),
					   "swapped_signal::destroy", g_nullify_pointer, &drag_pattern->seq_id,
					   NULL);
  gtk_box_pack_start (GTK_BOX (hbox), drag_pattern->seq_id, FALSE, TRUE, 5);

  /* pattern name widget
   */
  if (!pattern_group)
    {
      any = gtk_widget_new (GTK_TYPE_LABEL,
			    "visible", TRUE,
			    "label", " - ",
			    NULL);
      gtk_box_pack_start (GTK_BOX (hbox), any, FALSE, TRUE, 5);
      drag_pattern->name = g_object_connect (gtk_widget_new (GTK_TYPE_LABEL,
							     "visible", TRUE,
							     NULL),
					     "swapped_signal::destroy", g_nullify_pointer, &drag_pattern->name,
					     NULL);
      gtk_box_pack_start (GTK_BOX (hbox), drag_pattern->name, FALSE, TRUE, 5);
    }

  /* update contents
   */
  drag_pattern_changed (drag_pattern);


  /* setup as drag source
   */
  g_object_connect (drag_pattern->widget,
		    "signal::button_press_event", drag_pattern_button_press, drag_pattern,
		    "signal::drag_begin", drag_pattern_drag_begin, drag_pattern,
		    "signal::drag_data_get", drag_pattern_drag_data_get, drag_pattern,
		    "signal::drag_data_delete", drag_pattern_drag_data_delete, drag_pattern,
		    "signal::drag_end", gtk_widget_show, NULL,
		    NULL);
  
  return drag_pattern;
}
