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
#include	"bstplaylist.h"

#include	"bstmenus.h"


/* --- global play_list variables --- */
static GtkWidget        *bst_play_list_drag_window_pattern_icon = NULL;
static GtkWidget        *bst_play_list_drag_window_pattern_group_icon = NULL;
static GtkWidget        *bst_play_list_drop_spot_pattern = NULL;
static GtkWidget        *bst_play_list_drop_spot_pattern_group = NULL;
static GdkAtom		 atom_beast_pattern_group_pointer = 0;
static GdkAtom		 atom_beast_pattern_pointer = 0;


/* --- bug workarounds --- */
static void
container_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (widget->parent)
    gtk_container_remove (GTK_CONTAINER (widget->parent), widget);
}

#include	"bstdragpattern.c"	/* draggable pattern widgets */
#include	"bstdraggroup.c"	/* draggable pattern group widgets */

/* --- prototypes --- */
static void	bst_play_list_class_init	(BstPlayListClass	*klass);
static void	bst_play_list_init		(BstPlayList		*play_list);
static void	bst_play_list_destroy		(GtkObject		*object);
static gboolean	group_list_drag_motion		(GtkWidget      	*group_list,
						 GdkDragContext 	*context,
						 gint            	 x,
						 gint            	 y,
						 guint           	 time,
						 BstPlayList		*plist);
static void	group_list_drag_leave		(GtkWidget              *group_list,
						 GdkDragContext 	*context,
						 guint                   time,
						 BstPlayList		*plist);
static gboolean	group_list_drag_drop		(GtkWidget      	*group_list,
						 GdkDragContext 	*context,
						 gint            	 x,
						 gint            	 y,
						 guint           	 time,
						 BstPlayList		*plist);
static void	group_list_drag_data_received	(GtkWidget              *group_list,
						 GdkDragContext         *context,
						 gint                    x,
						 gint                    y,
						 GtkSelectionData       *selection_data,
						 guint                   info,
						 guint                   time,
						 BstPlayList            *plist);


/* --- static variables --- */
static gpointer		 parent_class = NULL;
static BstPlayListClass *bst_play_list_class = NULL;
static guint             drag_windows_ref_count = 0;


/* --- functions --- */
GtkType
bst_play_list_get_type (void)
{
  static GtkType play_list_type = 0;
  
  if (!play_list_type)
    {
      GtkTypeInfo play_list_info =
      {
	"BstPlayList",
	sizeof (BstPlayList),
	sizeof (BstPlayListClass),
	(GtkClassInitFunc) bst_play_list_class_init,
	(GtkObjectInitFunc) bst_play_list_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      play_list_type = gtk_type_unique (GTK_TYPE_VPANED, &play_list_info);
    }
  
  return play_list_type;
}

static void
bst_play_list_class_init (BstPlayListClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = GTK_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  bst_play_list_class = class;
  parent_class = gtk_type_class (GTK_TYPE_VPANED);

  object_class->destroy = bst_play_list_destroy;

  atom_beast_pattern_pointer = gdk_atom_intern ("_BEAST_PATTERN_POINTER", FALSE);
  atom_beast_pattern_group_pointer = gdk_atom_intern ("_BEAST_PATTERN_GROUP_POINTER", FALSE);
}

static void
bst_play_list_init (BstPlayList *plist)
{
  static const GtkTargetEntry target = { "_BEAST_PATTERN_GROUP_POINTER", GTK_TARGET_SAME_APP, 0 };
  GtkWidget *sw, *vbox, *any;

  plist->song = NULL;

  /* setup pattern list
   */
  vbox = gtk_widget_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "parent", plist,
			 "spacing", 0,
			 "border_width", 5,
			 NULL);
  any = gtk_widget_new (GTK_TYPE_LABEL,
			"visible", TRUE,
			"label", "Patterns:",
			"xalign", 0.0,
			NULL);
  gtk_box_pack_start (GTK_BOX (vbox), any, FALSE, TRUE, 0);
  plist->pattern_list = g_object_new (GTK_TYPE_VBOX,
				      "visible", TRUE,
				      NULL);
  g_object_connect (plist->pattern_list,
		    "signal::destroy", gtk_widget_destroyed, &plist->pattern_list,
		    NULL);
  sw = gtk_widget_new (GTK_TYPE_SCROLLED_WINDOW,
		       "visible", TRUE,
		       "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
		       "vscrollbar_policy", GTK_POLICY_ALWAYS,
		       "parent", vbox,
		       NULL);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sw), plist->pattern_list);
  bst_widget_modify_bg_as_base (plist->pattern_list->parent);

  /* setup group list
   */
  vbox = gtk_widget_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "parent", plist,
			 "spacing", 0,
			 "border_width", 5,
			 NULL);
  any = gtk_widget_new (GTK_TYPE_LABEL,
			"visible", TRUE,
			"label", "Arranger Groups:",
			"xalign", 0.0,
			NULL);
  gtk_box_pack_start (GTK_BOX (vbox), any, FALSE, TRUE, 0);
  plist->group_list = g_object_connect (gtk_widget_new (GTK_TYPE_VBOX,
							"visible", TRUE,
							NULL),
					"signal::destroy", gtk_widget_destroyed, &plist->group_list,
					NULL);
  sw = gtk_widget_new (GTK_TYPE_SCROLLED_WINDOW,
		       "visible", TRUE,
		       "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
		       "vscrollbar_policy", GTK_POLICY_ALWAYS,
		       "parent", vbox,
		       NULL);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (sw), plist->group_list);
  bst_widget_modify_bg_as_base (plist->group_list->parent);

  /* setup group list as drag destination
   */
  gtk_drag_dest_set (plist->group_list,
		     0,
		     &target, 1,
		     GDK_ACTION_COPY | GDK_ACTION_LINK | GDK_ACTION_MOVE);
  g_object_connect (plist->group_list,
		    "signal::drag_motion", group_list_drag_motion, plist,
		    "signal::drag_leave", group_list_drag_leave, plist,
		    "signal::drag_data_received", group_list_drag_data_received, plist,
		    "signal::drag_drop", group_list_drag_drop, plist,
		    NULL);
  
  
  /* build insertion spots and drag_windows
   */
  if (!drag_windows_ref_count)
    {
      GtkWidget *drag_widget, *hbox, *frame;

      drag_widget = gtk_widget_new (GTK_TYPE_FRAME,
				    "visible", TRUE,
				    "shadow", GTK_SHADOW_IN,
				    "child", bst_forest_from_bse_icon (bst_icon_from_stock (BST_ICON_TARGET),
								       BST_DRAG_ICON_WIDTH / 2,
								       BST_DRAG_ICON_HEIGHT / 2),
				    NULL);
      gtk_widget_ref (drag_widget);
      gtk_object_sink (GTK_OBJECT (drag_widget));
      bst_play_list_drop_spot_pattern = drag_widget;

      hbox = gtk_widget_new (GTK_TYPE_HBOX,
			     "visible", TRUE,
			     NULL);
      frame = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "shadow", GTK_SHADOW_OUT,
			      "child", bst_forest_from_bse_icon (bst_icon_from_stock (BST_ICON_TARGET),
								 BST_DRAG_ICON_WIDTH / 2,
								 BST_DRAG_ICON_HEIGHT / 2),
			      NULL);
      gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
      frame = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "shadow", GTK_SHADOW_OUT,
			      "child", bst_forest_from_bse_icon (bst_icon_from_stock (BST_ICON_TARGET),
								 BST_DRAG_ICON_WIDTH / 2,
								 BST_DRAG_ICON_HEIGHT / 2),
			      NULL);
      gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);
      frame = gtk_widget_new (GTK_TYPE_FRAME,
			      "visible", TRUE,
			      "shadow", GTK_SHADOW_OUT,
			      "child", bst_forest_from_bse_icon (bst_icon_from_stock (BST_ICON_TARGET),
								 BST_DRAG_ICON_WIDTH / 2,
								 BST_DRAG_ICON_HEIGHT / 2),
			      NULL);
      gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
      drag_widget = gtk_widget_new (GTK_TYPE_FRAME,
				    "visible", TRUE,
				    "shadow", GTK_SHADOW_IN,
				    "child", hbox,
				    NULL);
      gtk_widget_ref (drag_widget);
      gtk_object_sink (GTK_OBJECT (drag_widget));
      bst_play_list_drop_spot_pattern_group = drag_widget;

      drag_widget = bst_drag_window_from_icon (bst_icon_from_stock (BST_ICON_PATTERN));
      gtk_widget_ref (drag_widget);
      gtk_object_sink (GTK_OBJECT (drag_widget));
      bst_play_list_drag_window_pattern_icon = drag_widget;

      drag_widget = bst_drag_window_from_icon (bst_icon_from_stock (BST_ICON_PATTERN_GROUP));
      gtk_widget_ref (drag_widget);
      gtk_object_sink (GTK_OBJECT (drag_widget));
      bst_play_list_drag_window_pattern_group_icon = drag_widget;
    }
  drag_windows_ref_count += 1;
  plist->group_name_kennel = gtk_kennel_new (GTK_KENNEL_TO_MAXIMUM, 0);
}

static void
bst_play_list_destroy (GtkObject *object)
{
  BstPlayList *plist = BST_PLAY_LIST (object);

  bst_play_list_set_song (plist, NULL);

  if (plist->group_name_kennel) /* catch first destroy only */
    {
      gtk_kennel_unref (plist->group_name_kennel);
      plist->group_name_kennel = NULL;
      
      /* destroy drag_windows
       */
      drag_windows_ref_count -= 1;
      if (!drag_windows_ref_count)
	{
	  GtkWidget *drag_widget;
	  
	  drag_widget = bst_play_list_drop_spot_pattern_group;
	  bst_play_list_drop_spot_pattern_group = NULL;
	  gtk_widget_destroy (drag_widget);
	  gtk_widget_unref (drag_widget);
	  
	  drag_widget = bst_play_list_drop_spot_pattern;
	  bst_play_list_drop_spot_pattern = NULL;
	  gtk_widget_destroy (drag_widget);
	  gtk_widget_unref (drag_widget);
	  
	  drag_widget = bst_play_list_drag_window_pattern_icon;
	  bst_play_list_drag_window_pattern_icon = NULL;
	  gtk_widget_destroy (drag_widget);
	  gtk_widget_unref (drag_widget);
	  
	  drag_widget = bst_play_list_drag_window_pattern_group_icon;
	  bst_play_list_drag_window_pattern_group_icon = NULL;
	  gtk_widget_destroy (drag_widget);
	  gtk_widget_unref (drag_widget);
	}
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static gboolean
group_list_drag_motion (GtkWidget      *group_list,
			GdkDragContext *context,
			gint            x,
			gint            y,
			guint           time,
			BstPlayList    *plist)
{
  GtkWidget *drag_source = gtk_drag_get_source_widget (context);

  if (drag_source &&							/* check SAME_APP */
      gtk_widget_is_ancestor (drag_source, GTK_WIDGET (plist)) &&	/* check widget branch */
      context->targets &&						/* check target type */
      context->targets->data == atom_beast_pattern_group_pointer)
    {
      guint action = context->actions & context->suggested_action;
      gint old_pos, new_pos;

      if (action & GDK_ACTION_LINK)
	gdk_drag_status (context, action & GDK_ACTION_LINK, time);
      else if (action & GDK_ACTION_COPY)
	gdk_drag_status (context, action & GDK_ACTION_COPY, time);
      else
	gdk_drag_status (context, action & GDK_ACTION_MOVE, time);

      if (action & (GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK))
	gtk_widget_show (bst_play_list_drop_spot_pattern_group);
      else
	gtk_widget_hide (bst_play_list_drop_spot_pattern_group);

      if (!bst_play_list_drop_spot_pattern_group->parent)
	gtk_box_pack_start (GTK_BOX (group_list), bst_play_list_drop_spot_pattern_group, FALSE, TRUE, 1);

      new_pos = bst_container_get_insertion_position (GTK_CONTAINER (group_list),
						      FALSE, y,
						      bst_play_list_drop_spot_pattern_group,
						      &old_pos);
      if (new_pos != old_pos)
	gtk_box_reorder_child (GTK_BOX (group_list), bst_play_list_drop_spot_pattern_group, new_pos);

      return TRUE;
    }

  container_remove (bst_play_list_drop_spot_pattern_group); /* FIXME: GTKFIX, DND breaks on tree alterations */

  return FALSE;
}

static void
group_list_drag_leave (GtkWidget      *group_list,
		       GdkDragContext *context,
		       guint           time,
		       BstPlayList    *plist)
{
  container_remove (bst_play_list_drop_spot_pattern_group);
}

static gboolean
group_list_drag_drop (GtkWidget      *group_list,
		      GdkDragContext *context,
		      gint            x,
		      gint            y,
		      guint           time,
		      BstPlayList    *plist)
{
  GtkWidget *drag_source = gtk_drag_get_source_widget (context);

  if (drag_source &&							/* check SAME_APP */
      gtk_widget_is_ancestor (drag_source, GTK_WIDGET (plist)) &&	/* check widget branch */
      context->targets &&						/* check target type */
      context->targets->data == atom_beast_pattern_group_pointer)
    {
      gtk_drag_get_data (group_list, context, context->targets->data, time);

      return TRUE;
    }

  return FALSE;
}

static void
group_list_drag_data_received (GtkWidget        *group_list,
			       GdkDragContext   *context,
			       gint              x,
			       gint              y,
			       GtkSelectionData *selection_data,
			       guint             info,
			       guint             time,
			       BstPlayList      *plist)
{
  gpointer drag_contents;
  gint position;
  guint action;

  g_return_if_fail (selection_data->type == atom_beast_pattern_group_pointer);
  g_return_if_fail (selection_data->format = 8);
  g_return_if_fail (selection_data->length == sizeof (gpointer));

  action = context->actions & context->suggested_action;
  g_return_if_fail ((action & (GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK)) != 0);
  
  drag_contents = *((gpointer*) selection_data->data);
  g_return_if_fail (BSE_IS_PATTERN_GROUP (drag_contents));

  position = bst_container_get_insertion_position (GTK_CONTAINER (group_list),
						   FALSE, y,
						   bst_play_list_drop_spot_pattern_group, NULL);

  if (bst_play_list_drop_spot_pattern_group->parent) // FIXME GTKFIX (1.2.7 bug)
    {
      container_remove (bst_play_list_drop_spot_pattern_group);
      if (bst_play_list_drop_spot_pattern_group->parent)
	gtk_box_reorder_child (GTK_BOX (bst_play_list_drop_spot_pattern_group->parent),
			       bst_play_list_drop_spot_pattern_group, -1);
    }

  if (context->actions & GDK_ACTION_COPY)
    bse_song_insert_pattern_group_copy (plist->song, drag_contents, position);
  else if (context->actions & GDK_ACTION_MOVE)
    bse_song_insert_pattern_group_link (plist->song, drag_contents, position);
  else if (context->actions & GDK_ACTION_LINK)
    bse_song_insert_pattern_group_link (plist->song, drag_contents, position);
}

static void
song_item_added (BseSong     *song,
		 BseItem     *item,
		 BstPlayList *plist)
{
  if (BSE_IS_PATTERN (item))
    {
      BstDragPattern *drag_pattern = bst_drag_pattern_new (BSE_PATTERN (item), NULL, 0, FALSE);

      gtk_container_add_with_properties (GTK_CONTAINER (plist->pattern_list), drag_pattern->widget,
					 "expand", FALSE,
					 NULL);
      gtk_widget_queue_resize (plist->pattern_list);
    }
}

static void
song_pattern_group_insert (BseSong         *song,
			   BsePatternGroup *pgroup,
			   guint            position,
			   BstPlayList     *plist,
			   gboolean         ignore_first_insert)
{
  BstDragGroup *drag_group = bst_drag_group_new (pgroup, position, ignore_first_insert);

  gtk_kennel_add (plist->group_name_kennel, drag_group->name);
  gtk_container_add_with_properties (GTK_CONTAINER (plist->group_list), drag_group->widget,
				     "expand", FALSE,
				     "position", position,
				     NULL);
}

static void
song_pattern_group_inserted (BseSong         *song,
			     BsePatternGroup *pgroup,
			     guint            position,
			     BstPlayList     *plist)
{
  song_pattern_group_insert (song, pgroup, position, plist, FALSE); /* ignore_first_insert: TRUE */
}

void
bst_play_list_set_song (BstPlayList *plist,
			BseSong     *song)
{
  g_return_if_fail (BST_IS_PLAY_LIST (plist));
  if (song)
    g_return_if_fail (BSE_IS_SONG (song));

  if (plist->song)
    {
      g_object_disconnect (BSE_OBJECT (plist->song),
			   "any_signal", song_item_added, plist,
			   "any_signal", song_pattern_group_inserted, plist,
			   NULL);
      gtk_container_foreach (GTK_CONTAINER (plist->pattern_list), (GtkCallback) gtk_widget_destroy, NULL);
      gtk_container_foreach (GTK_CONTAINER (plist->group_list), (GtkCallback) gtk_widget_destroy, NULL);
    }
  plist->song = song;
  if (plist->song)
    {
      GList *free_list, *list;
      guint i;

      /* setup pattern list
       */
      g_object_connect (BSE_OBJECT (plist->song),
			"signal::item_added", song_item_added, plist,
			NULL);
      free_list = g_list_reverse (bse_container_list_items (BSE_CONTAINER (plist->song)));
      for (list = free_list; list; list = list->next)
	song_item_added (plist->song, list->data, plist);
      g_list_free (free_list);

      /* setup pattern groups
       */
      g_object_connect (BSE_OBJECT (plist->song),
			"signal::pattern_group_inserted", song_pattern_group_inserted, plist,
			NULL);
      for (i = 0; i < plist->song->n_pgroups; i++)
	song_pattern_group_insert (plist->song, plist->song->pgroups[i], i, plist, FALSE);
    }
}

void
bst_play_list_rebuild (BstPlayList *plist)
{
  g_return_if_fail (BST_IS_PLAY_LIST (plist));

  if (plist->song)
    {
      BseSong *song = plist->song;

      bse_object_ref (BSE_OBJECT (song));

      bst_play_list_set_song (plist, NULL);
      bst_play_list_set_song (plist, song);

      bse_object_unref (BSE_OBJECT (song));
    }
}

GtkWidget*
bst_play_list_new (BseSong *song)
{
  GtkWidget *play_list;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  
  play_list = gtk_widget_new (BST_TYPE_PLAY_LIST, NULL);
  bst_play_list_set_song (BST_PLAY_LIST (play_list), song);
  
  return play_list;
}
