#include "glewidgets.h"
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkWrapBox: Wrapping box widget
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/* #include <math.h> */

/* #include "gtkwrapbox.h" */


/* --- arguments --- */
enum {
  ARG__gtkwrapbox_0,
  ARG__gtkwrapbox_HOMOGENEOUS,
  ARG__gtkwrapbox_JUSTIFY,
  ARG__gtkwrapbox_HSPACING,
  ARG__gtkwrapbox_VSPACING,
  ARG__gtkwrapbox_LINE_JUSTIFY,
  ARG__gtkwrapbox_ASPECT_RATIO,
  ARG__gtkwrapbox_CURRENT_RATIO,
  ARG__gtkwrapbox_CHILD_LIMIT
};
enum {
  CHILD_ARG_0,
  CHILD_ARG_POSITION,
  CHILD_ARG_HEXPAND,
  CHILD_ARG_HFILL,
  CHILD_ARG_VEXPAND,
  CHILD_ARG_VFILL
};


/* --- prototypes --- */
static void gtk_wrap_box_class_init    (GtkWrapBoxClass    *klass);
static void gtk_wrap_box_init          (GtkWrapBox         *wbox);
static void gtk_wrap_box_get_arg       (GtkObject          *object,
					GtkArg             *arg,
					guint               arg_id);
static void gtk_wrap_box_set_arg       (GtkObject          *object,
					GtkArg             *arg,
					guint               arg_id);
static void gtk_wrap_box_set_child_arg (GtkContainer       *container,
					GtkWidget          *child,
					GtkArg             *arg,
					guint               arg_id);
static void gtk_wrap_box_get_child_arg (GtkContainer       *container,
					GtkWidget          *child,
					GtkArg             *arg,
					guint               arg_id);
static void gtk_wrap_box_map           (GtkWidget          *widget);
static void gtk_wrap_box_unmap         (GtkWidget          *widget);
static void gtk_wrap_box_draw          (GtkWidget          *widget,
					GdkRectangle       *area);
static gint gtk_wrap_box_expose        (GtkWidget          *widget,
					GdkEventExpose     *event);
static void gtk_wrap_box_add           (GtkContainer       *container,
					GtkWidget          *widget);
static void gtk_wrap_box_remove        (GtkContainer       *container,
					GtkWidget          *widget);
static void gtk_wrap_box_forall        (GtkContainer       *container,
					gboolean            include_internals,
					GtkCallback         callback,
					gpointer            callback_data);
static GtkType gtk_wrap_box_child_type (GtkContainer       *container);


/* --- variables --- */
static gpointer parent_gtkwrapbox_class = NULL;


/* --- functions --- */
GtkType
gtk_wrap_box_get_type (void)
{
  static GtkType wrap_box_type = 0;
  
  if (!wrap_box_type)
    {
      static const GtkTypeInfo wrap_box_info =
      {
	"GtkWrapBox",
	sizeof (GtkWrapBox),
	sizeof (GtkWrapBoxClass),
	(GtkClassInitFunc) gtk_wrap_box_class_init,
	(GtkObjectInitFunc) gtk_wrap_box_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      wrap_box_type = gtk_type_unique (GTK_TYPE_CONTAINER, &wrap_box_info);
    }
  
  return wrap_box_type;
}

static void
gtk_wrap_box_class_init (GtkWrapBoxClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  container_class = GTK_CONTAINER_CLASS (class);
  
  parent_gtkwrapbox_class = gtk_type_class (GTK_TYPE_CONTAINER);
  
  object_class->set_arg = gtk_wrap_box_set_arg;
  object_class->get_arg = gtk_wrap_box_get_arg;
  
  widget_class->map = gtk_wrap_box_map;
  widget_class->unmap = gtk_wrap_box_unmap;
  widget_class->draw = gtk_wrap_box_draw;
  widget_class->expose_event = gtk_wrap_box_expose;
  
  container_class->add = gtk_wrap_box_add;
  container_class->remove = gtk_wrap_box_remove;
  container_class->forall = gtk_wrap_box_forall;
  container_class->child_type = gtk_wrap_box_child_type;
  container_class->set_child_arg = gtk_wrap_box_set_child_arg;
  container_class->get_child_arg = gtk_wrap_box_get_child_arg;

  class->rlist_line_children = NULL;
  
  gtk_object_add_arg_type ("GtkWrapBox::homogeneous",
			   GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG__gtkwrapbox_HOMOGENEOUS);
  gtk_object_add_arg_type ("GtkWrapBox::justify",
			   GTK_TYPE_JUSTIFICATION, GTK_ARG_READWRITE, ARG__gtkwrapbox_JUSTIFY);
  gtk_object_add_arg_type ("GtkWrapBox::hspacing",
			   GTK_TYPE_UINT, GTK_ARG_READWRITE, ARG__gtkwrapbox_HSPACING);
  gtk_object_add_arg_type ("GtkWrapBox::vspacing",
			   GTK_TYPE_UINT, GTK_ARG_READWRITE, ARG__gtkwrapbox_VSPACING);
  gtk_object_add_arg_type ("GtkWrapBox::line_justify",
			   GTK_TYPE_JUSTIFICATION, GTK_ARG_READWRITE, ARG__gtkwrapbox_LINE_JUSTIFY);
  gtk_object_add_arg_type ("GtkWrapBox::aspect_ratio",
			   GTK_TYPE_FLOAT, GTK_ARG_READWRITE, ARG__gtkwrapbox_ASPECT_RATIO);
  gtk_object_add_arg_type ("GtkWrapBox::current_ratio",
			   GTK_TYPE_FLOAT, GTK_ARG_READABLE, ARG__gtkwrapbox_CURRENT_RATIO);
  gtk_object_add_arg_type ("GtkWrapBox::max_children_per_line",
			   GTK_TYPE_UINT, GTK_ARG_READWRITE, ARG__gtkwrapbox_CHILD_LIMIT);
  gtk_container_add_child_arg_type ("GtkWrapBox::position",
				    GTK_TYPE_INT, GTK_ARG_READWRITE, CHILD_ARG_POSITION);
  gtk_container_add_child_arg_type ("GtkWrapBox::hexpand",
				    GTK_TYPE_BOOL, GTK_ARG_READWRITE, CHILD_ARG_HEXPAND);
  gtk_container_add_child_arg_type ("GtkWrapBox::hfill",
				    GTK_TYPE_BOOL, GTK_ARG_READWRITE, CHILD_ARG_HFILL);
  gtk_container_add_child_arg_type ("GtkWrapBox::vexpand",
				    GTK_TYPE_BOOL, GTK_ARG_READWRITE, CHILD_ARG_VEXPAND);
  gtk_container_add_child_arg_type ("GtkWrapBox::vfill",
				    GTK_TYPE_BOOL, GTK_ARG_READWRITE, CHILD_ARG_VFILL);
}

static void
gtk_wrap_box_init (GtkWrapBox *wbox)
{
  GTK_WIDGET_SET_FLAGS (wbox, GTK_NO_WINDOW);
  
  wbox->homogeneous = FALSE;
  wbox->hspacing = 0;
  wbox->vspacing = 0;
  wbox->justify = GTK_JUSTIFY_LEFT;
  wbox->line_justify = GTK_JUSTIFY_BOTTOM;
  wbox->n_children = 0;
  wbox->children = NULL;
  wbox->aspect_ratio = 1;
  wbox->child_limit = 32767;
}

static void
gtk_wrap_box_set_arg (GtkObject *object,
		      GtkArg    *arg,
		      guint      arg_id)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (object);
  
  switch (arg_id)
    {
    case ARG__gtkwrapbox_HOMOGENEOUS:
      gtk_wrap_box_set_homogeneous (wbox, GTK_VALUE_BOOL (*arg));
      break;
    case ARG__gtkwrapbox_JUSTIFY:
      gtk_wrap_box_set_justify (wbox, GTK_VALUE_ENUM (*arg));
      break;
    case ARG__gtkwrapbox_LINE_JUSTIFY:
      gtk_wrap_box_set_line_justify (wbox, GTK_VALUE_ENUM (*arg));
      break;
    case ARG__gtkwrapbox_HSPACING:
      gtk_wrap_box_set_hspacing (wbox, GTK_VALUE_UINT (*arg));
      break;
    case ARG__gtkwrapbox_VSPACING:
      gtk_wrap_box_set_vspacing (wbox, GTK_VALUE_UINT (*arg));
      break;
    case ARG__gtkwrapbox_ASPECT_RATIO:
      gtk_wrap_box_set_aspect_ratio (wbox, GTK_VALUE_FLOAT (*arg));
      break;
    case ARG__gtkwrapbox_CHILD_LIMIT:
      if (wbox->child_limit != GTK_VALUE_UINT (*arg))
	{
	  wbox->child_limit = CLAMP (GTK_VALUE_UINT (*arg), 1, 32767);
	  gtk_widget_queue_resize (GTK_WIDGET (wbox));
	}
      break;
    }
}

static void
gtk_wrap_box_get_arg (GtkObject *object,
		      GtkArg    *arg,
		      guint      arg_id)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (object);
  GtkWidget *widget = GTK_WIDGET (object);
  
  switch (arg_id)
    {
    case ARG__gtkwrapbox_HOMOGENEOUS:
      GTK_VALUE_BOOL (*arg) = wbox->homogeneous;
      break;
    case ARG__gtkwrapbox_JUSTIFY:
      GTK_VALUE_ENUM (*arg) = wbox->justify;
      break;
    case ARG__gtkwrapbox_LINE_JUSTIFY:
      GTK_VALUE_ENUM (*arg) = wbox->line_justify;
      break;
    case ARG__gtkwrapbox_HSPACING:
      GTK_VALUE_UINT (*arg) = wbox->hspacing;
      break;
    case ARG__gtkwrapbox_VSPACING:
      GTK_VALUE_UINT (*arg) = wbox->vspacing;
      break;
    case ARG__gtkwrapbox_ASPECT_RATIO:
      GTK_VALUE_FLOAT (*arg) = wbox->aspect_ratio;
      break;
    case ARG__gtkwrapbox_CURRENT_RATIO:
      GTK_VALUE_FLOAT (*arg) = (((gfloat) widget->allocation.width) /
				((gfloat) widget->allocation.height));
      break;
    case ARG__gtkwrapbox_CHILD_LIMIT:
      GTK_VALUE_UINT (*arg) = wbox->child_limit;
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static void
gtk_wrap_box_set_child_arg (GtkContainer *container,
			    GtkWidget    *child,
			    GtkArg       *arg,
			    guint         arg_id)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  gboolean hexpand = FALSE, hfill = FALSE, vexpand = FALSE, vfill = FALSE;
  
  if (arg_id != CHILD_ARG_POSITION)
    gtk_wrap_box_query_child_packing (wbox, child, &hexpand, &hfill, &vexpand, &vfill);
  
  switch (arg_id)
    {
    case CHILD_ARG_POSITION:
      gtk_wrap_box_reorder_child (wbox, child, GTK_VALUE_INT (*arg));
      break;
    case CHILD_ARG_HEXPAND:
      gtk_wrap_box_set_child_packing (wbox, child,
				      GTK_VALUE_BOOL (*arg), hfill,
				      vexpand, vfill);
      break;
    case CHILD_ARG_HFILL:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, GTK_VALUE_BOOL (*arg),
				      vexpand, vfill);
      break;
    case CHILD_ARG_VEXPAND:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, hfill,
				      GTK_VALUE_BOOL (*arg), vfill);
      break;
    case CHILD_ARG_VFILL:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, hfill,
				      vexpand, GTK_VALUE_BOOL (*arg));
      break;
    default:
      break;
    }
}

static void
gtk_wrap_box_get_child_arg (GtkContainer *container,
			    GtkWidget    *child,
			    GtkArg       *arg,
			    guint         arg_id)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  gboolean hexpand = FALSE, hfill = FALSE, vexpand = FALSE, vfill = FALSE;
  
  if (arg_id != CHILD_ARG_POSITION)
    gtk_wrap_box_query_child_packing (wbox, child, &hexpand, &hfill, &vexpand, &vfill);
  
  switch (arg_id)
    {
      GtkWrapBoxChild *child_info;
    case CHILD_ARG_POSITION:
      GTK_VALUE_INT (*arg) = 0;
      for (child_info = wbox->children; child_info; child_info = child_info->next)
	{
	  if (child_info->widget == child)
	    break;
	  GTK_VALUE_INT (*arg)++;
	}
      if (!child_info)
	GTK_VALUE_INT (*arg) = -1;
      break;
    case CHILD_ARG_HEXPAND:
      GTK_VALUE_BOOL (*arg) = hexpand;
      break;
    case CHILD_ARG_HFILL:
      GTK_VALUE_BOOL (*arg) = hfill;
      break;
    case CHILD_ARG_VEXPAND:
      GTK_VALUE_BOOL (*arg) = vexpand;
      break;
    case CHILD_ARG_VFILL:
      GTK_VALUE_BOOL (*arg) = vfill;
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static GtkType
gtk_wrap_box_child_type	(GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}

void
gtk_wrap_box_set_homogeneous (GtkWrapBox *wbox,
			      gboolean    homogeneous)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  
  homogeneous = homogeneous != FALSE;
  if (wbox->homogeneous != homogeneous)
    {
      wbox->homogeneous = homogeneous;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_set_hspacing (GtkWrapBox *wbox,
			   guint       hspacing)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  
  if (wbox->hspacing != hspacing)
    {
      wbox->hspacing = hspacing;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_set_vspacing (GtkWrapBox *wbox,
			   guint       vspacing)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  
  if (wbox->vspacing != vspacing)
    {
      wbox->vspacing = vspacing;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_set_justify (GtkWrapBox      *wbox,
			  GtkJustification justify)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (justify <= GTK_JUSTIFY_FILL);
  
  if (wbox->justify != justify)
    {
      wbox->justify = justify;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_set_line_justify (GtkWrapBox      *wbox,
			       GtkJustification line_justify)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (line_justify <= GTK_JUSTIFY_FILL);
  
  if (wbox->line_justify != line_justify)
    {
      wbox->line_justify = line_justify;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_set_aspect_ratio (GtkWrapBox *wbox,
			       gfloat      aspect_ratio)
{
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  
  aspect_ratio = CLAMP (aspect_ratio, 1.0 / 256.0, 256.0);
  
  if (wbox->aspect_ratio != aspect_ratio)
    {
      wbox->aspect_ratio = aspect_ratio;
      gtk_widget_queue_resize (GTK_WIDGET (wbox));
    }
}

void
gtk_wrap_box_pack (GtkWrapBox *wbox,
		   GtkWidget  *child,
		   gboolean    hexpand,
		   gboolean    hfill,
		   gboolean    vexpand,
		   gboolean    vfill)
{
  GtkWrapBoxChild *child_info;
  
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == NULL);
  
  child_info = g_new (GtkWrapBoxChild, 1);
  child_info->widget = child;
  child_info->hexpand = hexpand ? TRUE : FALSE;
  child_info->hfill = hfill ? TRUE : FALSE;
  child_info->vexpand = vexpand ? TRUE : FALSE;
  child_info->vfill = vfill ? TRUE : FALSE;
  child_info->next = NULL;
  if (wbox->children)
    {
      GtkWrapBoxChild *last = wbox->children;
      
      while (last->next)
	last = last->next;
      last->next = child_info;
    }
  else
    wbox->children = child_info;
  wbox->n_children++;
  
  gtk_widget_set_parent (child, GTK_WIDGET (wbox));
  
  if (GTK_WIDGET_REALIZED (wbox))
    gtk_widget_realize (child);
  
  if (GTK_WIDGET_VISIBLE (wbox) && GTK_WIDGET_VISIBLE (child))
    {
      if (GTK_WIDGET_MAPPED (wbox))
	gtk_widget_map (child);
      
      gtk_widget_queue_resize (child);
    }
}

void
gtk_wrap_box_reorder_child (GtkWrapBox *wbox,
			    GtkWidget  *child,
			    gint        position)
{
  GtkWrapBoxChild *child_info, *last = NULL;
  
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  
  for (child_info = wbox->children; child_info; last = child_info, child_info = last->next)
    if (child_info->widget == child)
      break;
  
  if (child_info && wbox->children->next)
    {
      GtkWrapBoxChild *tmp;
      
      if (last)
	last->next = child_info->next;
      else
	wbox->children = child_info->next;
      
      last = NULL;
      tmp = wbox->children;
      while (position && tmp->next)
	{
	  position--;
	  last = tmp;
	  tmp = last->next;
	}
      
      if (position)
	{
	  tmp->next = child_info;
	  child_info->next = NULL;
	}
      else
	{
	  child_info->next = tmp;
	  if (last)
	    last->next = child_info;
	  else
	    wbox->children = child_info;
	}
      
      if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_VISIBLE (wbox))
	gtk_widget_queue_resize (child);
    }
}

void
gtk_wrap_box_query_child_packing (GtkWrapBox *wbox,
				  GtkWidget  *child,
				  gboolean   *hexpand,
				  gboolean   *hfill,
				  gboolean   *vexpand,
				  gboolean   *vfill)
{
  GtkWrapBoxChild *child_info;
  
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  
  for (child_info = wbox->children; child_info; child_info = child_info->next)
    if (child_info->widget == child)
      break;
  
  if (child_info)
    {
      if (hexpand)
	*hexpand = child_info->hexpand;
      if (hfill)
	*hfill = child_info->hfill;
      if (vexpand)
	*vexpand = child_info->vexpand;
      if (vfill)
	*vfill = child_info->vfill;
    }
}

void
gtk_wrap_box_set_child_packing (GtkWrapBox *wbox,
				GtkWidget  *child,
				gboolean    hexpand,
				gboolean    hfill,
				gboolean    vexpand,
				gboolean    vfill)
{
  GtkWrapBoxChild *child_info;
  
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  
  hexpand = hexpand != FALSE;
  hfill = hfill != FALSE;
  vexpand = vexpand != FALSE;
  vfill = vfill != FALSE;
  
  for (child_info = wbox->children; child_info; child_info = child_info->next)
    if (child_info->widget == child)
      break;
  
  if (child_info &&
      (child_info->hexpand != hexpand || child_info->vexpand != vexpand ||
       child_info->hfill != hfill || child_info->vfill != vfill))
    {
      child_info->hexpand = hexpand;
      child_info->hfill = hfill;
      child_info->vexpand = vexpand;
      child_info->vfill = vfill;
      
      if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_VISIBLE (wbox))
	gtk_widget_queue_resize (child);
    }
}

guint*
gtk_wrap_box_query_line_lengths (GtkWrapBox *wbox,
				 guint      *_n_lines)
{
  GtkWrapBoxChild *next_child = NULL;
  GtkAllocation area, *allocation;
  gboolean expand_line;
  GSList *slist;
  guint max_child_size, border, n_lines = 0, *lines = NULL;

  if (_n_lines)
    *_n_lines = 0;
  g_return_val_if_fail (GTK_IS_WRAP_BOX (wbox), NULL);

  allocation = &GTK_WIDGET (wbox)->allocation;
  border = GTK_CONTAINER (wbox)->border_width;
  area.x = allocation->x + border;
  area.y = allocation->y + border;
  area.width = MAX (1, (gint) allocation->width - border * 2);
  area.height = MAX (1, (gint) allocation->height - border * 2);

  next_child = wbox->children;
  slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
							      &next_child,
							      &area,
							      &max_child_size,
							      &expand_line);
  while (slist)
    {
      guint l = n_lines++;

      lines = g_renew (guint, lines, n_lines);
      lines[l] = g_slist_length (slist);
      g_slist_free (slist);

      slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
								  &next_child,
								  &area,
								  &max_child_size,
								  &expand_line);
    }

  if (_n_lines)
    *_n_lines = n_lines;

  return lines;
}

static void
gtk_wrap_box_map (GtkWidget *widget)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  
  GTK_WIDGET_SET_FLAGS (wbox, GTK_MAPPED);
  
  for (child = wbox->children; child; child = child->next)
    if (GTK_WIDGET_VISIBLE (child->widget) &&
	!GTK_WIDGET_MAPPED (child->widget))
      gtk_widget_map (child->widget);
}

static void
gtk_wrap_box_unmap (GtkWidget *widget)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  
  GTK_WIDGET_UNSET_FLAGS (wbox, GTK_MAPPED);
  
  for (child = wbox->children; child; child = child->next)
    if (GTK_WIDGET_VISIBLE (child->widget) &&
	GTK_WIDGET_MAPPED (child->widget))
      gtk_widget_unmap (child->widget);
}

static void
gtk_wrap_box_draw (GtkWidget    *widget,
		   GdkRectangle *area)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  GdkRectangle child_area;
  
  if (GTK_WIDGET_DRAWABLE (widget))
    for (child = wbox->children; child; child = child->next)
      if (GTK_WIDGET_DRAWABLE (child->widget) &&
	  gtk_widget_intersect (child->widget, area, &child_area))
	gtk_widget_draw (child->widget, &child_area);
}

static gint
gtk_wrap_box_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  GdkEventExpose child_event = *event;
  
  g_return_val_if_fail (event != NULL, FALSE);
  
  if (GTK_WIDGET_DRAWABLE (widget))
    for (child = wbox->children; child; child = child->next)
      if (GTK_WIDGET_DRAWABLE (child->widget) &&
	  GTK_WIDGET_NO_WINDOW (child->widget) &&
	  gtk_widget_intersect (child->widget, &event->area, &child_event.area))
	gtk_widget_event (child->widget, (GdkEvent*) &child_event);
  
  return TRUE;
}

static void
gtk_wrap_box_add (GtkContainer *container,
		  GtkWidget    *widget)
{
  gtk_wrap_box_pack (GTK_WRAP_BOX (container), widget, FALSE, TRUE, FALSE, TRUE);
}

static void
gtk_wrap_box_remove (GtkContainer *container,
		     GtkWidget    *widget)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  GtkWrapBoxChild *child, *last = NULL;
  
  child = wbox->children;
  while (child)
    {
      if (child->widget == widget)
	{
	  gboolean was_visible;
	  
	  was_visible = GTK_WIDGET_VISIBLE (widget);
	  gtk_widget_unparent (widget);
	  
	  if (last)
	    last->next = child->next;
	  else
	    wbox->children = child->next;
	  g_free (child);
	  wbox->n_children--;
	  
	  if (was_visible)
	    gtk_widget_queue_resize (GTK_WIDGET (container));
	  
	  break;
	}
      
      last = child;
      child = last->next;
    }
}

static void
gtk_wrap_box_forall (GtkContainer *container,
		     gboolean      include_internals,
		     GtkCallback   callback,
		     gpointer      callback_data)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  GtkWrapBoxChild *child;
  
  child = wbox->children;
  while (child)
    {
      GtkWidget *widget = child->widget;
      
      child = child->next;
      
      callback (widget, callback_data);
    }
}
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkClueHunter: Completion popup with pattern matching for GtkEntry
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/* #include	"gtkcluehunter.h" */
/* #include	<gdk/gdkkeysyms.h> */
/* #include	<string.h> */


/* --- signals --- */
enum {
  SIGNAL_ACTIVATE,
  SIGNAL_POPUP,
  SIGNAL_POPDOWN,
  SIGNAL_SELECT_ON,
  SIGNAL_LAST
};


/* --- arguments --- */
enum {
  ARG__gtkcluehunter_0,
  ARG__gtkcluehunter_PATTERN_MATCHING,
  ARG__gtkcluehunter_KEEP_HISTORY,
  ARG__gtkcluehunter_ALIGN_WIDTH,
  ARG__gtkcluehunter_ENTRY
};


/* --- prototypes --- */
static void	gtk_clue_hunter_class_init	(GtkClueHunterClass	*class);
static void	gtk_clue_hunter_init		(GtkClueHunter		*clue_hunter);
static void	gtk_clue_hunter_destroy		(GtkObject		*object);
static void	gtk_clue_hunter_finalize	(GtkObject		*object);
static void	gtk_clue_hunter_set_arg		(GtkObject      	*object,
						 GtkArg         	*arg,
						 guint          	 arg_id);
static void	gtk_clue_hunter_get_arg		(GtkObject      	*object,
						 GtkArg         	*arg,
						 guint          	 arg_id);
static void	gtk_clue_hunter_entry_changed	(GtkClueHunter		*clue_hunter);
static gint	gtk_clue_hunter_entry_key_press	(GtkClueHunter		*clue_hunter,
						 GdkEventKey		*event,
						 GtkEntry		*entry);
static gint	gtk_clue_hunter_clist_click	(GtkClueHunter		*clue_hunter,
						 GdkEventButton		*event,
						 GtkCList		*clist);
static gint	gtk_clue_hunter_event           (GtkWidget		*widget,
						 GdkEvent		*event);
static void	gtk_clue_hunter_do_activate	(GtkClueHunter		*clue_hunter);
static void	gtk_clue_hunter_do_popup	(GtkClueHunter       	*clue_hunter);
static void	gtk_clue_hunter_do_popdown      (GtkClueHunter       	*clue_hunter);
static void	gtk_clue_hunter_add_history	(GtkClueHunter		*clue_hunter,
						 const gchar   		*string);
static void	gtk_clue_hunter_do_select_on	(GtkClueHunter		*clue_hunter,
						 const gchar		*string);
static void	gtk_clue_hunter_popdown		(GtkClueHunter		*clue_hunter);


/* --- variables --- */
static GtkWindowClass	  *parent_gtkcluehunter_class = NULL;
static GtkClueHunterClass *gtk_clue_hunter_class = NULL;
static guint		   clue_hunter_signals[SIGNAL_LAST] = { 0, };


/* --- functions --- */
GtkType
gtk_clue_hunter_get_type (void)
{
  static GtkType clue_hunter_type = 0;
  
  if (!clue_hunter_type)
    {
      GtkTypeInfo clue_hunter_info =
      {
	"GtkClueHunter",
	sizeof (GtkClueHunter),
	sizeof (GtkClueHunterClass),
	(GtkClassInitFunc) gtk_clue_hunter_class_init,
	(GtkObjectInitFunc) gtk_clue_hunter_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      clue_hunter_type = gtk_type_unique (GTK_TYPE_WINDOW, &clue_hunter_info);
    }
  
  return clue_hunter_type;
}

static void
gtk_clue_hunter_class_init (GtkClueHunterClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  GtkWindowClass *window_class;
  
  gtk_clue_hunter_class = class;
  parent_gtkcluehunter_class = gtk_type_class (GTK_TYPE_WINDOW);
  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  container_class = (GtkContainerClass*) class;
  window_class = (GtkWindowClass*) class;
  
  object_class->set_arg = gtk_clue_hunter_set_arg;
  object_class->get_arg = gtk_clue_hunter_get_arg;
  object_class->destroy = gtk_clue_hunter_destroy;
  object_class->finalize = gtk_clue_hunter_finalize;
  
  widget_class->event = gtk_clue_hunter_event;
  
  class->activate = gtk_clue_hunter_do_activate;
  class->popup = gtk_clue_hunter_do_popup;
  class->popdown = gtk_clue_hunter_do_popdown;
  class->select_on = gtk_clue_hunter_do_select_on;
  
  gtk_object_add_arg_type ("GtkClueHunter::pattern_matching", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG__gtkcluehunter_PATTERN_MATCHING);
  gtk_object_add_arg_type ("GtkClueHunter::keep_history", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG__gtkcluehunter_KEEP_HISTORY);
  gtk_object_add_arg_type ("GtkClueHunter::align_width", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG__gtkcluehunter_ALIGN_WIDTH);
  gtk_object_add_arg_type ("GtkClueHunter::entry", GTK_TYPE_ENTRY, GTK_ARG_READWRITE, ARG__gtkcluehunter_ENTRY);
  
  clue_hunter_signals[SIGNAL_ACTIVATE] =
    gtk_signal_new ("activate",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkClueHunterClass, activate),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  clue_hunter_signals[SIGNAL_POPUP] =
    gtk_signal_new ("popup",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkClueHunterClass, popup),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  clue_hunter_signals[SIGNAL_POPDOWN] =
    gtk_signal_new ("popdown",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkClueHunterClass, popdown),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  clue_hunter_signals[SIGNAL_SELECT_ON] =
    gtk_signal_new ("select_on",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkClueHunterClass, select_on),
		    gtk_marshal_NONE__POINTER,
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_STRING);
  widget_class->activate_signal = clue_hunter_signals[SIGNAL_ACTIVATE];
}

static void
gtk_clue_hunter_set_arg (GtkObject *object,
			 GtkArg    *arg,
			 guint      arg_id)
{
  GtkClueHunter *clue_hunter;
  
  clue_hunter = GTK_CLUE_HUNTER (object);
  
  switch (arg_id)
    {
    case ARG__gtkcluehunter_PATTERN_MATCHING:
      gtk_clue_hunter_set_pattern_matching (clue_hunter, GTK_VALUE_BOOL (*arg));
      break;
    case ARG__gtkcluehunter_KEEP_HISTORY:
      gtk_clue_hunter_set_keep_history (clue_hunter, GTK_VALUE_BOOL (*arg));
      break;
    case ARG__gtkcluehunter_ALIGN_WIDTH:
      gtk_clue_hunter_set_align_width (clue_hunter, GTK_VALUE_BOOL (*arg));
      break;
    case ARG__gtkcluehunter_ENTRY:
      gtk_clue_hunter_set_entry (clue_hunter, GTK_VALUE_POINTER (*arg));
      break;
    default:
      break;
    }
}

static void
gtk_clue_hunter_get_arg (GtkObject *object,
			 GtkArg    *arg,
			 guint      arg_id)
{
  GtkClueHunter *clue_hunter;
  
  clue_hunter = GTK_CLUE_HUNTER (object);
  
  switch (arg_id)
    {
    case ARG__gtkcluehunter_PATTERN_MATCHING:
      GTK_VALUE_BOOL (*arg) = clue_hunter->pattern_matching;
      break;
    case ARG__gtkcluehunter_KEEP_HISTORY:
      GTK_VALUE_BOOL (*arg) = clue_hunter->keep_history;
      break;
    case ARG__gtkcluehunter_ALIGN_WIDTH:
      GTK_VALUE_BOOL (*arg) = clue_hunter->align_width;
      break;
    case ARG__gtkcluehunter_ENTRY:
      GTK_VALUE_POINTER (*arg) = clue_hunter->entry;
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static void
gtk_clue_hunter_init (GtkClueHunter *clue_hunter)
{
  GtkWidget *parent;
  GtkWidget *clist;
  
  clue_hunter->popped_up = FALSE;
  clue_hunter->completion_tag = FALSE;
  clue_hunter->pattern_matching = TRUE;
  clue_hunter->keep_history = TRUE;
  clue_hunter->align_width = TRUE;
  clue_hunter->clist_column = 0;
  clue_hunter->cstring = NULL;
  
  gtk_widget_set (GTK_WIDGET (clue_hunter),
		  "type", GTK_WINDOW_POPUP,
		  "auto_shrink", TRUE,
		  "allow_shrink", FALSE,
		  "allow_grow", FALSE,
		  NULL);
  parent = GTK_WIDGET (clue_hunter);
  parent = gtk_widget_new (GTK_TYPE_FRAME,
			   "visible", TRUE,
			   "label", NULL,
			   "shadow", GTK_SHADOW_OUT,
			   "parent", parent,
			   NULL);
  clue_hunter->scw = gtk_widget_new (GTK_TYPE_SCROLLED_WINDOW,
				     "visible", TRUE,
				     "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
				     "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
				     "parent", parent,
				     NULL);
  clue_hunter->clist = NULL;
  clue_hunter->entry = NULL;
  clist = gtk_widget_new (GTK_TYPE_CLIST,
			  "n_columns", 1,
			  "titles_active", FALSE,
			  NULL);
  gtk_clist_set_auto_sort (GTK_CLIST (clist), TRUE);
  gtk_clist_set_sort_type (GTK_CLIST (clist), GTK_SORT_ASCENDING);
  gtk_clist_column_titles_hide (GTK_CLIST (clist));
  gtk_clue_hunter_set_clist (clue_hunter, clist, 0);
}

static void
gtk_clue_hunter_destroy (GtkObject *object)
{
  GtkClueHunter *clue_hunter;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (object));
  
  clue_hunter = GTK_CLUE_HUNTER (object);
  
  if (clue_hunter->popped_up)
    gtk_clue_hunter_popdown (clue_hunter);
  
  clue_hunter->scw = NULL;
  if (clue_hunter->clist)
    gtk_widget_unref (clue_hunter->clist);
  clue_hunter->clist = NULL;
  
  if (clue_hunter->entry)
    gtk_clue_hunter_set_entry (clue_hunter, NULL);
  
  GTK_OBJECT_CLASS (parent_gtkcluehunter_class)->destroy (object);
}

static gint
gtk_clue_hunter_clist_click (GtkClueHunter  *clue_hunter,
			     GdkEventButton *event,
			     GtkCList	    *clist)
{
  gboolean handled = FALSE;

  if (event->type == GDK_2BUTTON_PRESS &&
      event->button == 1 && clist->selection)
    {
      gchar *string;

      handled = TRUE;
      string = gtk_clue_hunter_try_complete (clue_hunter);
      gtk_entry_set_text (GTK_ENTRY (clue_hunter->entry), string ? string : "");
      g_free (string);

      gtk_clue_hunter_popdown (clue_hunter);
      gtk_widget_activate (clue_hunter->entry);
    }

  return handled;
}

void
gtk_clue_hunter_set_clist (GtkClueHunter *clue_hunter,
			   GtkWidget     *clist,
			   guint16	  column)
{
  g_return_if_fail (clue_hunter != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter));
  g_return_if_fail (clist != NULL);
  g_return_if_fail (GTK_IS_CLIST (clist));
  g_return_if_fail (clist->parent == NULL);
  g_return_if_fail (column < GTK_CLIST (clist)->columns);

  if (clue_hunter->clist)
    {
      if (clue_hunter->clist->parent)
	gtk_container_remove (GTK_CONTAINER (clue_hunter->clist->parent), clue_hunter->clist);
      if (!GTK_OBJECT_DESTROYED (clue_hunter->clist))
	gtk_signal_disconnect_by_func (GTK_OBJECT (clue_hunter->clist),
				       GTK_SIGNAL_FUNC (gtk_clue_hunter_clist_click),
				       clue_hunter);
      gtk_widget_unref (clue_hunter->clist);
    }
  clue_hunter->clist = clist;
  gtk_widget_ref (clue_hunter->clist);
  gtk_widget_set (clue_hunter->clist,
		  "visible", TRUE,
		  "selection_mode", GTK_SELECTION_EXTENDED,
		  "parent", clue_hunter->scw,
		  "object_signal_after::button_press_event", gtk_clue_hunter_clist_click, clue_hunter,
		  NULL);
  clue_hunter->clist_column = column;
}

static void
gtk_clue_hunter_popdown (GtkClueHunter *clue_hunter)
{
  g_return_if_fail (clue_hunter != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter));
  g_return_if_fail (clue_hunter->popped_up == TRUE);
  
  gtk_signal_emit (GTK_OBJECT (clue_hunter), clue_hunter_signals[SIGNAL_POPDOWN]);
}

static void
gtk_clue_hunter_finalize (GtkObject *object)
{
  GtkClueHunter *clue_hunter;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (object));
  
  clue_hunter = GTK_CLUE_HUNTER (object);
  
  g_free (clue_hunter->cstring);
  
  GTK_OBJECT_CLASS (parent_gtkcluehunter_class)->finalize (object);
}

void
gtk_clue_hunter_popup (GtkClueHunter *clue_hunter)
{
  g_return_if_fail (clue_hunter != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter));
  g_return_if_fail (clue_hunter->popped_up == FALSE);
  
  if (clue_hunter->entry && GTK_WIDGET_DRAWABLE (clue_hunter->entry))
    gtk_signal_emit (GTK_OBJECT (clue_hunter), clue_hunter_signals[SIGNAL_POPUP]);
}

void
gtk_clue_hunter_select_on (GtkClueHunter *clue_hunter,
			   const gchar   *string)
{
  g_return_if_fail (clue_hunter != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter));
  g_return_if_fail (string != NULL);
  
  gtk_signal_emit (GTK_OBJECT (clue_hunter), clue_hunter_signals[SIGNAL_SELECT_ON], string);
}

GtkWidget*
gtk_clue_hunter_create_arrow (GtkClueHunter *clue_hunter)
{
  GtkWidget *button, *arrow;

  g_return_val_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter), NULL);

  arrow = gtk_widget_new (GTK_TYPE_ARROW,
			  "arrow_type", GTK_ARROW_DOWN,
			  "shadow_type", GTK_SHADOW_ETCHED_IN,
			  "visible", TRUE,
			  NULL);
  button = gtk_widget_new (GTK_TYPE_BUTTON,
			   "child", arrow,
			   "visible", TRUE,
			   "can_focus", FALSE,
			   NULL);
  gtk_signal_connect_object_while_alive (GTK_OBJECT (button),
					 "clicked",
					 GTK_SIGNAL_FUNC (gtk_clue_hunter_popup),
					 GTK_OBJECT (clue_hunter));

  return button;
}

static void
gtk_clue_hunter_entry_changed (GtkClueHunter *clue_hunter)
{
  clue_hunter->completion_tag = FALSE;
  g_free (clue_hunter->cstring);
  clue_hunter->cstring = g_strdup (gtk_entry_get_text (GTK_ENTRY (clue_hunter->entry)));
  gtk_clue_hunter_select_on (clue_hunter, clue_hunter->cstring);
}

static gint
gtk_clue_hunter_entry_key_press (GtkClueHunter *clue_hunter,
				 GdkEventKey   *event,
				 GtkEntry      *entry)
{
  gboolean handled = FALSE;
  
  if ((event->keyval == GDK_Tab || event->keyval == GDK_ISO_Left_Tab) &&
      !(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)))
    {
      handled = TRUE;
      
      if (event->type == GDK_KEY_PRESS)
	{
	  gchar *cstring;
	  gchar *ostring;
	  
	  cstring = gtk_clue_hunter_try_complete (clue_hunter);
	  ostring = gtk_entry_get_text (GTK_ENTRY (clue_hunter->entry));
	  if (!ostring)
	    ostring = "";
	  if (cstring && strcmp (ostring, cstring))
	    {
	      gtk_entry_set_text (GTK_ENTRY (clue_hunter->entry), cstring);
	      clue_hunter->completion_tag = clue_hunter->popped_up;
	    }
	  else
	    {
	      if (clue_hunter->completion_tag)
		gtk_widget_activate (GTK_WIDGET (clue_hunter));
	      else
		clue_hunter->completion_tag = TRUE;
	    }
	  g_free (cstring);
	  
	  gtk_signal_emit_stop_by_name (GTK_OBJECT (entry), "key-press-event");
	}
    }
  
  return handled;
}

static void
gtk_clue_hunter_entry_destroyed (GtkClueHunter *clue_hunter)
{
  gtk_object_destroy (GTK_OBJECT (clue_hunter));
}

void
gtk_clue_hunter_set_entry (GtkClueHunter *clue_hunter,
			   GtkWidget     *entry)
{
  g_return_if_fail (clue_hunter != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter));
  if (entry)
    {
      g_return_if_fail (GTK_IS_ENTRY (entry));
      g_return_if_fail (GTK_OBJECT_DESTROYED (entry) == FALSE);
      g_return_if_fail (gtk_clue_hunter_from_entry (entry) == NULL);
    }
  
  if (clue_hunter->entry)
    {
      if (!GTK_OBJECT_DESTROYED (clue_hunter->entry))
	{
	  gtk_signal_disconnect_by_func (GTK_OBJECT (clue_hunter->entry),
					 GTK_SIGNAL_FUNC (gtk_clue_hunter_entry_changed),
					 clue_hunter);
	  gtk_signal_disconnect_by_func (GTK_OBJECT (clue_hunter->entry),
					 GTK_SIGNAL_FUNC (gtk_clue_hunter_entry_key_press),
					 clue_hunter);
	  gtk_signal_disconnect_by_func (GTK_OBJECT (clue_hunter->entry),
					 GTK_SIGNAL_FUNC (gtk_clue_hunter_entry_destroyed),
					 clue_hunter);
	}
      gtk_object_set_data (GTK_OBJECT (clue_hunter->entry), "GtkClueHunter", NULL);
      gtk_widget_unref (clue_hunter->entry);
    }
  clue_hunter->entry = entry;
  if (clue_hunter->entry)
    {
      gtk_widget_ref (clue_hunter->entry);
      gtk_object_set_data (GTK_OBJECT (clue_hunter->entry), "GtkClueHunter", clue_hunter);
      gtk_signal_connect_object (GTK_OBJECT (clue_hunter->entry),
				 "destroy",
				 GTK_SIGNAL_FUNC (gtk_clue_hunter_entry_destroyed),
				 GTK_OBJECT (clue_hunter));
      gtk_signal_connect_object (GTK_OBJECT (clue_hunter->entry),
				 "changed",
				 GTK_SIGNAL_FUNC (gtk_clue_hunter_entry_changed),
				 GTK_OBJECT (clue_hunter));
      gtk_signal_connect_object (GTK_OBJECT (clue_hunter->entry),
				 "key_press_event",
				 GTK_SIGNAL_FUNC (gtk_clue_hunter_entry_key_press),
				 GTK_OBJECT (clue_hunter));
    }
  clue_hunter->completion_tag = FALSE;
}

GtkClueHunter*
gtk_clue_hunter_from_entry (GtkWidget *entry)
{
  g_return_val_if_fail (GTK_IS_ENTRY (entry), NULL);

  return gtk_object_get_data (GTK_OBJECT (entry), "GtkClueHunter");
}

void
gtk_clue_hunter_add_string (GtkClueHunter *clue_hunter,
			    const gchar   *string)
{
  GtkCList *clist;
  gchar **text;
  
  g_return_if_fail (clue_hunter != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter));
  g_return_if_fail (string != NULL);
  
  clist = GTK_CLIST (clue_hunter->clist);
  
  text = g_new0 (gchar*, clist->columns);
  text[clue_hunter->clist_column] = (gchar*) string;

  gtk_clist_insert (clist, 0, text);
  g_free (text);
}

void
gtk_clue_hunter_remove_string (GtkClueHunter *clue_hunter,
			       const gchar   *string)
{
  GtkCList *clist;
  GList *list;
  guint n = 0;
  
  g_return_if_fail (clue_hunter != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter));
  g_return_if_fail (string != NULL);
  
  clist = GTK_CLIST (clue_hunter->clist);
  
  for (list = clist->row_list; list; list = list->next)
    {
      GtkCListRow *clist_row = list->data;
      gint cmp;
      
      cmp = strcmp (string, clist_row->cell[clue_hunter->clist_column].u.text);
      if (cmp == 0)
	{
	  gtk_clist_remove (clist, n);
	  break;
	}
      n++;
    }
}

void
gtk_clue_hunter_remove_matches (GtkClueHunter *clue_hunter,
				const gchar   *pattern)
{
  GtkPatternSpec pspec;
  GtkCList *clist;
  GList *list;
  guint n = 0;
  
  g_return_if_fail (clue_hunter != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter));
  if (!pattern)
    pattern = "*";
  
  gtk_pattern_spec_init (&pspec, pattern);
  
  clist = GTK_CLIST (clue_hunter->clist);
  
  gtk_clist_freeze (clist);

  list = clist->row_list;
  while (list)
    {
      GtkCListRow *clist_row = list->data;

      list = list->next;
      
      if (gtk_pattern_match_string (&pspec, clist_row->cell[clue_hunter->clist_column].u.text))
	gtk_clist_remove (clist, n);
      else
	n++;
    }

  gtk_clist_thaw (clist);

  gtk_pattern_spec_free_segs (&pspec);
}

static gchar*
gtk_clue_hunter_intersect (guint   max_len,
			   GSList *strings)
{
  gchar *completion;
  guint l = 0;
  
  if (!strings || !max_len)
    return NULL;
  
  completion = g_new (gchar, max_len + 1);
  
  while (l < max_len)
    {
      gchar *s = strings->data;
      GSList *slist;
      
      s += l;
      completion[l] = *s;
      
      for (slist = strings->next; slist; slist = slist->next)
	{
	  s = slist->data;
	  s += l;
	  if (completion[l] != *s)
	    completion[l] = 0;
	}
      if (!completion[l])
	break;
      l++;
    }
  completion[l] = 0;
  
  return g_renew (gchar, completion, completion[0] ? l + 1 : 0);
}

gchar*
gtk_clue_hunter_try_complete (GtkClueHunter *clue_hunter)
{
  GtkCList *clist;
  GList *list;
  GSList *strings;
  guint max_len, n;
  gchar *completion;
  
  g_return_val_if_fail (clue_hunter != NULL, NULL);
  g_return_val_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter), NULL);
  
  clist = GTK_CLIST (clue_hunter->clist);
  
  strings = NULL;
  max_len = 0;
  n = 0;
  for (list = clist->row_list; list; list = list->next)
    {
      GtkCListRow *clist_row = list->data;
      
      if (g_list_find (clist->selection, GINT_TO_POINTER (n)))
	{
	  guint l;
	  
	  l = strlen (clist_row->cell[clue_hunter->clist_column].u.text);
	  max_len = MAX (max_len, l);
	  strings = g_slist_prepend (strings, clist_row->cell[clue_hunter->clist_column].u.text);
	}
      n++;
    }
  
  completion = gtk_clue_hunter_intersect (max_len, strings);
  g_slist_free (strings);
  
  return completion;
}

void
gtk_clue_hunter_set_pattern_matching (GtkClueHunter *clue_hunter,
				      gboolean       on_off)
{
  g_return_if_fail (clue_hunter != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter));
  
  clue_hunter->pattern_matching = on_off != FALSE;
}

void
gtk_clue_hunter_set_keep_history (GtkClueHunter *clue_hunter,
				  gboolean       on_off)
{
  g_return_if_fail (clue_hunter != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter));
  
  clue_hunter->keep_history = on_off != FALSE;
}

void
gtk_clue_hunter_set_align_width (GtkClueHunter *clue_hunter,
				 gboolean       on_off)
{
  g_return_if_fail (clue_hunter != NULL);
  g_return_if_fail (GTK_IS_CLUE_HUNTER (clue_hunter));
  
  clue_hunter->align_width = on_off != FALSE;
}

static void
gtk_clue_hunter_do_activate (GtkClueHunter *clue_hunter)
{
  if (clue_hunter->popped_up)
    gtk_clue_hunter_popdown (clue_hunter);
  else if (clue_hunter->entry)
    gtk_clue_hunter_popup (clue_hunter);
}

static void
gtk_clue_hunter_do_popup (GtkClueHunter *clue_hunter)
{
  GtkWidget *widget;
  gint x = 0, y = 0, width = 0, height = 0;
  
  g_return_if_fail (!clue_hunter->popped_up);
  
  widget = GTK_WIDGET (clue_hunter);
  
  gtk_widget_grab_focus (clue_hunter->entry);

  if (!clue_hunter->cstring)
    clue_hunter->cstring = g_strdup ("");
  
  gtk_clist_columns_autosize (GTK_CLIST (clue_hunter->clist));
  gtk_widget_size_request (clue_hunter->clist, NULL);
  gtk_widget_set_usize (clue_hunter->clist,
			clue_hunter->clist->requisition.width,
			clue_hunter->clist->requisition.height);
  gtk_widget_size_request (widget, NULL);
  
  gdk_window_get_origin (clue_hunter->entry->window, &x, &y);
  gdk_window_get_size (clue_hunter->entry->window, &width, &height);

  height = MIN (height, gdk_screen_height ());
  if (y < 0)
    {
      height = MAX (0, height + y);
      y = 0;
    }
  else if (y > gdk_screen_height ())
    {
      height = 0;
      y = gdk_screen_height ();
    }
  else if (y + height > gdk_screen_height ())
    height = gdk_screen_height () - y;
  width = MIN (width, gdk_screen_width ());
  x = CLAMP (x, 0, gdk_screen_width () - width);
  
  if (widget->requisition.height > gdk_screen_height () - (y + height))
    {
      if (y + height / 2 > gdk_screen_height () / 2)
	{
	  height = MIN (y, widget->requisition.height);
	  y -= height;
	}
      else
	{
	  y += height;
	  height = gdk_screen_height () - y;
	}
    }
  else
    {
      y += height;
      height = 0;
    }
  
  if (!clue_hunter->align_width && widget->requisition.width > width)
    {
      if (widget->requisition.width <= gdk_screen_width () - x)
	width = widget->requisition.width;
      else if (x + width / 2 > gdk_screen_width () / 2)
	{
	  x += width;
	  width = MIN (x, widget->requisition.width);
	  x -= width;
	}
      else
	width = MIN (gdk_screen_width () - x, widget->requisition.width);
    }

  gtk_widget_set_uposition (widget, x, y);
  gtk_widget_set_usize (widget, width, height);

  gtk_grab_add (widget);
  
  gtk_widget_grab_focus (clue_hunter->clist);
  
  clue_hunter->popped_up = TRUE;
  clue_hunter->completion_tag = FALSE;
  
  gtk_clue_hunter_select_on (clue_hunter, clue_hunter->cstring);
  
  gtk_widget_show (widget);
  
  while (gdk_pointer_grab (widget->window, TRUE,
			   (GDK_POINTER_MOTION_HINT_MASK |
			    GDK_BUTTON1_MOTION_MASK |
			    GDK_BUTTON2_MOTION_MASK |
			    GDK_BUTTON3_MOTION_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK),
			   NULL,
			   NULL,
			   GDK_CURRENT_TIME) != 0)
    ;
}

static void
gtk_clue_hunter_do_popdown (GtkClueHunter *clue_hunter)
{
  GtkWidget *widget;
  
  g_return_if_fail (clue_hunter->popped_up);
  
  widget = GTK_WIDGET (clue_hunter);
  
  /* gdk_pointer_ungrab (GDK_CURRENT_TIME); */
  gtk_widget_hide (widget);

  gdk_flush ();
  
  gtk_grab_remove (widget);
  
  clue_hunter->popped_up = FALSE;
  clue_hunter->completion_tag = FALSE;
}

static void
gtk_clue_hunter_add_history (GtkClueHunter *clue_hunter,
			     const gchar   *string)
{
  GtkCList *clist;
  GList *list;

  clist = GTK_CLIST (clue_hunter->clist);

  for (list = clist->row_list; list; list = list->next)
    {
      GtkCListRow *clist_row = list->data;
      
      if (strcmp (string, clist_row->cell[clue_hunter->clist_column].u.text) == 0)
	return;
    }
  gtk_clue_hunter_add_string (clue_hunter, string);
}

static void
gtk_clue_hunter_do_select_on (GtkClueHunter *clue_hunter,
			      const gchar   *cstring)
{
  GtkCList *clist;
  GList *list;
  guint len;
  
  clist = GTK_CLIST (clue_hunter->clist);
  
  len = strlen (cstring);
  
  gtk_clist_freeze (clist);
  
  gtk_clist_undo_selection (clist);
  gtk_clist_unselect_all (clist);
  
  if (len && clue_hunter->pattern_matching)
    {
      GtkPatternSpec pspec = { 0, };
      guint n = 0;
      gboolean check_visibility = TRUE;
      gchar *pattern;
      
      pattern = g_strconcat (cstring, "*", NULL);
      gtk_pattern_spec_init (&pspec, pattern);
      g_free (pattern);
      
      for (list = clist->row_list; list; list = list->next)
	{
	  GtkCListRow *clist_row = list->data;
	  
	  if (gtk_pattern_match_string (&pspec, clist_row->cell[clue_hunter->clist_column].u.text))
	    {
	      gtk_clist_select_row (clist, n, 0);
	      
	      if (check_visibility &&
		  gtk_clist_row_is_visible (clist, n) != GTK_VISIBILITY_FULL)
		gtk_clist_moveto (clist, n, -1, 0.5, 0);
	      check_visibility = FALSE;
	    }
	  n++;
	}
      gtk_pattern_spec_free_segs (&pspec);
    }
  else if (len)
    {
      guint n = 0;
      gboolean check_visibility = TRUE;
      
      for (list = clist->row_list; list; list = list->next)
	{
	  GtkCListRow *clist_row = list->data;
	  gint cmp;
	  
	  cmp = strncmp (cstring, clist_row->cell[clue_hunter->clist_column].u.text, len);
	  if (cmp == 0)
	    {
	      gtk_clist_select_row (clist, n, 0);
	      
	      if (check_visibility &&
		  gtk_clist_row_is_visible (clist, n) != GTK_VISIBILITY_FULL)
		gtk_clist_moveto (clist, n, -1, 0.5, 0);
	      check_visibility = FALSE;
	    }
	  n++;
	}
    }
  
  gtk_clist_thaw (clist);
}

static gint
gtk_clue_hunter_event (GtkWidget *widget,
		       GdkEvent  *event)
{
  GtkClueHunter *clue_hunter;
  gboolean handled = FALSE;
  
  clue_hunter = GTK_CLUE_HUNTER (widget);
  switch (event->type)
    {
      GtkWidget *ev_widget;
    case GDK_KEY_PRESS:
      if (event->key.keyval == GDK_Escape)
	{
	  handled = TRUE;
	  gtk_clue_hunter_popdown (clue_hunter);
	}
      else if (event->key.keyval == GDK_Return ||
	       event->key.keyval == GDK_KP_Enter)
	{
	  gchar *string;
	  
	  handled = TRUE;
	  string = gtk_clue_hunter_try_complete (clue_hunter);
	  if (string)
	    {
	      if (string[0])
		gtk_entry_set_text (GTK_ENTRY (clue_hunter->entry), string);
	      g_free (string);
	    }
	  else if (clue_hunter->keep_history)
	    {
	      string = gtk_entry_get_text (GTK_ENTRY (clue_hunter->entry));
	      if (string && string[0])
		gtk_clue_hunter_add_history (clue_hunter, string);
	    }
	  gtk_clue_hunter_popdown (clue_hunter);
	  gtk_widget_activate (clue_hunter->entry);
	}
      else
	handled = gtk_widget_event (clue_hunter->entry, event);
      break;
      
    case GDK_KEY_RELEASE:
      if (event->key.keyval == GDK_Escape ||
	  event->key.keyval == GDK_Return ||
	  event->key.keyval == GDK_KP_Enter)
	handled = TRUE;
      else
	handled = gtk_widget_event (clue_hunter->entry, event);
      break;
      
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      while (gdk_pointer_grab (widget->window, TRUE,
			       (GDK_POINTER_MOTION_HINT_MASK |
				GDK_BUTTON1_MOTION_MASK |
				GDK_BUTTON2_MOTION_MASK |
				GDK_BUTTON3_MOTION_MASK |
				GDK_BUTTON_PRESS_MASK |
				GDK_BUTTON_RELEASE_MASK),
			       NULL,
			       NULL,
			       GDK_CURRENT_TIME) != 0)
	;
      ev_widget = gtk_get_event_widget (event);
      if (ev_widget == widget &&
	  event->type == GDK_BUTTON_PRESS)
	{
	  gint w, h;
	  
	  gdk_window_get_size (widget->window, &w, &h);
	  if (event->button.x > w || event->button.y > h ||
	      event->button.x < 0 || event->button.y < 0)
	    ev_widget = NULL;
	}
      else if (ev_widget)
	while (ev_widget->parent)
	  ev_widget = ev_widget->parent;
      if (ev_widget != widget)
	{
	  gtk_clue_hunter_popdown (clue_hunter);
	  handled = TRUE;
	}
      break;
      
    case GDK_DELETE:
      gtk_clue_hunter_popdown (clue_hunter);
      handled = TRUE;
      break;
      
    default:
      break;
    }
  
  return handled;
}
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkHWrapBox: Horizontal wrapping box widget
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/* #include "gtkhwrapbox.h" */
/* #include <math.h> */


/* --- prototypes --- */
static void    gtk_hwrap_box_class_init    (GtkHWrapBoxClass   *klass);
static void    gtk_hwrap_box_init          (GtkHWrapBox        *hwbox);
static void    gtk_hwrap_box_size_request  (GtkWidget          *widget,
					    GtkRequisition     *requisition);
static void    gtk_hwrap_box_size_allocate (GtkWidget          *widget,
					    GtkAllocation      *allocation);
static GSList* reverse_list_row_children   (GtkWrapBox         *wbox,
					    GtkWrapBoxChild   **child_p,
					    GtkAllocation      *area,
					    guint              *max_height,
					    gboolean           *can_vexpand);


/* --- variables --- */
static gpointer parent_gtkhwrapbox_class = NULL;


/* --- functions --- */
GtkType
gtk_hwrap_box_get_type (void)
{
  static GtkType hwrap_box_type = 0;
  
  if (!hwrap_box_type)
    {
      static const GtkTypeInfo hwrap_box_info =
      {
	"GtkHWrapBox",
	sizeof (GtkHWrapBox),
	sizeof (GtkHWrapBoxClass),
	(GtkClassInitFunc) gtk_hwrap_box_class_init,
	(GtkObjectInitFunc) gtk_hwrap_box_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      hwrap_box_type = gtk_type_unique (GTK_TYPE_WRAP_BOX, &hwrap_box_info);
    }
  
  return hwrap_box_type;
}

static void
gtk_hwrap_box_class_init (GtkHWrapBoxClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  GtkWrapBoxClass *wrap_box_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  container_class = GTK_CONTAINER_CLASS (class);
  wrap_box_class = GTK_WRAP_BOX_CLASS (class);
  
  parent_gtkhwrapbox_class = gtk_type_class (GTK_TYPE_WRAP_BOX);
  
  widget_class->size_request = gtk_hwrap_box_size_request;
  widget_class->size_allocate = gtk_hwrap_box_size_allocate;

  wrap_box_class->rlist_line_children = reverse_list_row_children;
}

static void
gtk_hwrap_box_init (GtkHWrapBox *hwbox)
{
  hwbox->max_child_width = 0;
  hwbox->max_child_height = 0;
}

GtkWidget*
gtk_hwrap_box_new (gboolean homogeneous)
{
  GtkHWrapBox *hwbox;

  hwbox = GTK_HWRAP_BOX (gtk_widget_new (GTK_TYPE_HWRAP_BOX, NULL));

  GTK_WRAP_BOX (hwbox)->homogeneous = homogeneous ? TRUE : FALSE;

  return GTK_WIDGET (hwbox);
}

static inline void
get_gtkhwrapbox_child_requisition (GtkWrapBox     *wbox,
		       GtkWidget      *child,
		       GtkRequisition *child_requisition)
{
  if (wbox->homogeneous)
    {
      GtkHWrapBox *hwbox = GTK_HWRAP_BOX (wbox);
      
      child_requisition->width = hwbox->max_child_width;
      child_requisition->height = hwbox->max_child_height;
    }
  else
    gtk_widget_get_child_requisition (child, child_requisition);
}

static void
_gtk_hwrap_box_size_request (GtkWidget      *widget,
			     GtkRequisition *requisition)
{
  GtkHWrapBox *this = GTK_HWRAP_BOX (widget);
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  guint area = 0;
  
  g_return_if_fail (requisition != NULL);
  
  /*<h2v-off>*/
  requisition->width = 0;
  requisition->height = 0;
  this->max_child_width = 0;
  this->max_child_height = 0;
  
  for (child = wbox->children; child; child = child->next)
    if (GTK_WIDGET_VISIBLE (child->widget))
      {
	GtkRequisition child_requisition;
	
	gtk_widget_size_request (child->widget, &child_requisition);
	
	area += child_requisition.width * child_requisition.height;
	this->max_child_width = MAX (this->max_child_width, child_requisition.width);
	this->max_child_height = MAX (this->max_child_height, child_requisition.height);
      }
  if (wbox->homogeneous)
    area = this->max_child_width * this->max_child_height * wbox->n_children;
  
  if (area)
    {
      requisition->width = sqrt (area * wbox->aspect_ratio);
      requisition->height = area / requisition->width;
    }
  else
    {
      requisition->width = 0;
      requisition->height = 0;
    }
  
  requisition->width += GTK_CONTAINER (wbox)->border_width * 2;
  requisition->height += GTK_CONTAINER (wbox)->border_width * 2;
  /*<h2v-on>*/
}

static gfloat
get_gtkhwrapbox_layout_size (GtkHWrapBox *this,
		 guint        max_width,
		 guint       *width_inc)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (this);
  GtkWrapBoxChild *child;
  guint n_rows, left_over = 0, total_height = 0;
  gboolean last_row_filled = TRUE;

  *width_inc = this->max_child_width + 1;

  n_rows = 0;
  for (child = wbox->children; child; child = child->next)
    {
      GtkWrapBoxChild *row_child;
      GtkRequisition child_requisition;
      guint row_width, row_height, n = 1;

      if (!GTK_WIDGET_VISIBLE (child->widget))
	continue;

      get_gtkhwrapbox_child_requisition (wbox, child->widget, &child_requisition);
      if (!last_row_filled)
	*width_inc = MIN (*width_inc, child_requisition.width - left_over);
      row_width = child_requisition.width;
      row_height = child_requisition.height;
      for (row_child = child->next; row_child && n < wbox->child_limit; row_child = row_child->next)
	{
	  if (GTK_WIDGET_VISIBLE (row_child->widget))
	    {
	      get_gtkhwrapbox_child_requisition (wbox, row_child->widget, &child_requisition);
	      if (row_width + wbox->hspacing + child_requisition.width > max_width)
		break;
	      row_width += wbox->hspacing + child_requisition.width;
	      row_height = MAX (row_height, child_requisition.height);
	      n++;
	    }
	  child = row_child;
	}
      last_row_filled = n >= wbox->child_limit;
      left_over = last_row_filled ? 0 : max_width - (row_width + wbox->hspacing);
      total_height += (n_rows ? wbox->vspacing : 0) + row_height;
      n_rows++;
    }

  if (*width_inc > this->max_child_width)
    *width_inc = 0;
  
  return MAX (total_height, 1);
}

static void
gtk_hwrap_box_size_request (GtkWidget      *widget,
			    GtkRequisition *requisition)
{
  GtkHWrapBox *this = GTK_HWRAP_BOX (widget);
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  gfloat ratio_dist, layout_width = 0;
  guint row_inc = 0;
  
  g_return_if_fail (requisition != NULL);
  
  requisition->width = 0;
  requisition->height = 0;
  this->max_child_width = 0;
  this->max_child_height = 0;

  /* size_request all children */
  for (child = wbox->children; child; child = child->next)
    if (GTK_WIDGET_VISIBLE (child->widget))
      {
	GtkRequisition child_requisition;
	
	gtk_widget_size_request (child->widget, &child_requisition);

	this->max_child_width = MAX (this->max_child_width, child_requisition.width);
	this->max_child_height = MAX (this->max_child_height, child_requisition.height);
      }

  /* figure all possible layouts */
  ratio_dist = 32768;
  layout_width = this->max_child_width;
  do
    {
      gfloat layout_height;
      gfloat ratio, dist;

      layout_width += row_inc;
      layout_height = get_gtkhwrapbox_layout_size (this, layout_width, &row_inc);
      ratio = layout_width / layout_height;		/*<h2v-skip>*/
      dist = MAX (ratio, wbox->aspect_ratio) - MIN (ratio, wbox->aspect_ratio);
      if (dist < ratio_dist)
	{
	  ratio_dist = dist;
	  requisition->width = layout_width;
	  requisition->height = layout_height;
	}
      
      /* g_print ("ratio for width %d height %d = %f\n",
	 (gint) layout_width,
	 (gint) layout_height,
	 ratio);
      */
    }
  while (row_inc);

  requisition->width += GTK_CONTAINER (wbox)->border_width * 2; /*<h2v-skip>*/
  requisition->height += GTK_CONTAINER (wbox)->border_width * 2; /*<h2v-skip>*/
  /* g_print ("choosen: width %d, height %d\n",
     requisition->width,
     requisition->height);
  */
}

static GSList*
reverse_list_row_children (GtkWrapBox       *wbox,
			   GtkWrapBoxChild **child_p,
			   GtkAllocation    *area,
			   guint            *max_child_size,
			   gboolean         *expand_line)
{
  GSList *slist = NULL;
  guint width = 0, row_width = area->width;
  GtkWrapBoxChild *child = *child_p;
  
  *max_child_size = 0;
  *expand_line = FALSE;
  
  while (child && !GTK_WIDGET_VISIBLE (child->widget))
    {
      *child_p = child->next;
      child = *child_p;
    }
  
  if (child)
    {
      GtkRequisition child_requisition;
      guint n = 1;
      
      get_gtkhwrapbox_child_requisition (wbox, child->widget, &child_requisition);
      width += child_requisition.width;
      *max_child_size = MAX (*max_child_size, child_requisition.height);
      *expand_line |= child->vexpand;
      slist = g_slist_prepend (slist, child);
      *child_p = child->next;
      child = *child_p;
      
      while (child && n < wbox->child_limit)
	{
	  if (GTK_WIDGET_VISIBLE (child->widget))
	    {
	      get_gtkhwrapbox_child_requisition (wbox, child->widget, &child_requisition);
	      if (width + wbox->hspacing + child_requisition.width > row_width)
		break;
	      width += wbox->hspacing + child_requisition.width;
	      *max_child_size = MAX (*max_child_size, child_requisition.height);
	      *expand_line |= child->vexpand;
	      slist = g_slist_prepend (slist, child);
	      n++;
	    }
	  *child_p = child->next;
	  child = *child_p;
	}
    }
  
  return slist;
}

static void
layout_row (GtkWrapBox    *wbox,
	    GtkAllocation *area,
	    GSList        *children,
	    guint          children_per_line,
	    gboolean       vexpand)
{
  GSList *slist;
  guint n_children = 0, n_expand_children = 0, have_expand_children = 0, total_width = 0;
  gfloat x, width, extra;
  GtkAllocation child_allocation;
  
  for (slist = children; slist; slist = slist->next)
    {
      GtkWrapBoxChild *child = slist->data;
      GtkRequisition child_requisition;
      
      n_children++;
      if (child->hexpand)
	n_expand_children++;
      
      get_gtkhwrapbox_child_requisition (wbox, child->widget, &child_requisition);
      total_width += child_requisition.width;
    }
  
  width = MAX (1, area->width - (n_children - 1) * wbox->hspacing);
  if (width > total_width)
    extra = width - total_width;
  else
    extra = 0;
  have_expand_children = n_expand_children && extra;
  
  x = area->x;
  if (wbox->homogeneous)
    {
      width = MAX (1, area->width - (children_per_line - 1) * wbox->hspacing);
      width /= ((gdouble) children_per_line);
      extra = 0;
    }
  else if (have_expand_children)
    {
      width = extra;
      extra /= ((gdouble) n_expand_children);
    }
  else
    {
      if (wbox->justify == GTK_JUSTIFY_FILL)
	{
	  width = extra;
	  have_expand_children = TRUE;
	  n_expand_children = n_children;
	  extra /= ((gdouble) n_expand_children);
	}
      else if (wbox->justify == GTK_JUSTIFY_CENTER)
	{
	  x += extra / 2;
	  width = 0;
	  extra = 0;
	}
      else if (wbox->justify == GTK_JUSTIFY_LEFT)
	{
	  width = 0;
	  extra = 0;
	}
      else if (wbox->justify == GTK_JUSTIFY_RIGHT)
	{
	  x += extra;
	  width = 0;
	  extra = 0;
	}
    }
  
  n_children = 0;
  for (slist = children; slist; slist = slist->next)
    {
      GtkWrapBoxChild *child = slist->data;
      
      child_allocation.x = x;
      child_allocation.y = area->y;
      if (wbox->homogeneous)
	{
	  child_allocation.height = area->height;
	  child_allocation.width = width;
	  x += child_allocation.width + wbox->hspacing;
	}
      else
	{
	  GtkRequisition child_requisition;
	  
	  get_gtkhwrapbox_child_requisition (wbox, child->widget, &child_requisition);
	  
	  if (child_requisition.height >= area->height)
	    child_allocation.height = area->height;
	  else
	    {
	      child_allocation.height = child_requisition.height;
	      if (wbox->line_justify == GTK_JUSTIFY_FILL || child->vfill)
		child_allocation.height = area->height;
	      else if (child->vexpand || wbox->line_justify == GTK_JUSTIFY_CENTER)
		child_allocation.y += (area->height - child_requisition.height) / 2;
	      else if (wbox->line_justify == GTK_JUSTIFY_BOTTOM)
		child_allocation.y += area->height - child_requisition.height;
	    }
	  
	  if (have_expand_children)
	    {
	      child_allocation.width = child_requisition.width;
	      if (child->hexpand || wbox->justify == GTK_JUSTIFY_FILL)
		{
		  guint space;
		  
		  n_expand_children--;
		  space = extra * n_expand_children;
		  space = width - space;
		  width -= space;
		  if (child->hfill)
		    child_allocation.width += space;
		  else
		    {
		      child_allocation.x += space / 2;
		      x += space;
		    }
		}
	    }
	  else
	    {
	      /* g_print ("child_allocation.x %d += %d * %f ",
		       child_allocation.x, n_children, extra); */
	      child_allocation.x += n_children * extra;
	      /* g_print ("= %d\n",
		       child_allocation.x); */
	      child_allocation.width = MIN (child_requisition.width,
					    area->width - child_allocation.x + area->x);
	    }
	}
      
      x += child_allocation.width + wbox->hspacing;
      gtk_widget_size_allocate (child->widget, &child_allocation);
      n_children++;
    }
}

typedef struct _Line_gtkhwrapbox_ Line_gtkhwrapbox_;
struct _Line_gtkhwrapbox_
{
  GSList  *children;
  guint16  min_size;
  guint    expand : 1;
  Line_gtkhwrapbox_     *next;
};

static void
layout_rows (GtkWrapBox    *wbox,
	     GtkAllocation *area)
{
  GtkWrapBoxChild *next_child;
  guint min_height;
  gboolean vexpand;
  GSList *slist;
  Line_gtkhwrapbox_ *line_list = NULL;
  guint total_height = 0, n_expand_lines = 0, n_lines = 0;
  gfloat shrink_height;
  guint children_per_line;
  
  next_child = wbox->children;
  slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
							      &next_child,
							      area,
							      &min_height,
							      &vexpand);
  slist = g_slist_reverse (slist);

  children_per_line = g_slist_length (slist);
  while (slist)
    {
      Line_gtkhwrapbox_ *line = g_new (Line_gtkhwrapbox_, 1);
      
      line->children = slist;
      line->min_size = min_height;
      total_height += min_height;
      line->expand = vexpand;
      if (vexpand)
	n_expand_lines++;
      line->next = line_list;
      line_list = line;
      n_lines++;
      
      slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
								  &next_child,
								  area,
								  &min_height,
								  &vexpand);
      slist = g_slist_reverse (slist);
    }
  
  if (total_height > area->height)
    shrink_height = total_height - area->height;
  else
    shrink_height = 0;
  
  if (1) /* reverse lines and shrink */
    {
      Line_gtkhwrapbox_ *prev = NULL, *last = NULL;
      gfloat n_shrink_lines = n_lines;
      
      while (line_list)
	{
	  Line_gtkhwrapbox_ *tmp = line_list->next;

	  if (shrink_height)
	    {
	      Line_gtkhwrapbox_ *line = line_list;
	      guint shrink_fract = shrink_height / n_shrink_lines + 0.5;

	      if (line->min_size > shrink_fract)
		{
		  shrink_height -= shrink_fract;
		  line->min_size -= shrink_fract;
		}
	      else
		{
		  shrink_height -= line->min_size - 1;
		  line->min_size = 1;
		}
	    }
	  n_shrink_lines--;

	  last = line_list;
	  line_list->next = prev;
	  prev = line_list;
	  line_list = tmp;
	}
      line_list = last;
    }
  
  if (n_lines)
    {
      Line_gtkhwrapbox_ *line;
      gfloat y, height, extra = 0;
      
      height = area->height;
      height = MAX (n_lines, height - (n_lines - 1) * wbox->vspacing);
      
      if (wbox->homogeneous)
	height /= ((gdouble) n_lines);
      else if (n_expand_lines)
	{
	  height = MAX (0, height - total_height);
	  extra = height / ((gdouble) n_expand_lines);
	}
      else
	height = 0;
      
      y = area->y;
      line = line_list;
      while (line)
	{
	  GtkAllocation row_allocation;
	  Line_gtkhwrapbox_ *next_line = line->next;
	  
	  row_allocation.x = area->x;
	  row_allocation.width = area->width;
	  if (wbox->homogeneous)
	    row_allocation.height = height;
	  else
	    {
	      row_allocation.height = line->min_size;
	      
	      if (line->expand)
		row_allocation.height += extra;
	    }
	  
	  row_allocation.y = y;
	  
	  y += row_allocation.height + wbox->vspacing;
	  layout_row (wbox,
		      &row_allocation,
		      line->children,
		      children_per_line,
		      line->expand);
	  
	  g_slist_free (line->children);
	  g_free (line);
	  line = next_line;
	}
    }
}

static void
gtk_hwrap_box_size_allocate (GtkWidget     *widget,
			     GtkAllocation *allocation)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkAllocation area;
  guint border = GTK_CONTAINER (wbox)->border_width; /*<h2v-skip>*/
  
  widget->allocation = *allocation;
  area.x = allocation->x + border;
  area.y = allocation->y + border;
  area.width = MAX (1, (gint) allocation->width - border * 2);
  area.height = MAX (1, (gint) allocation->height - border * 2);
  
  /*<h2v-off>*/
  /* g_print ("got: width %d, height %d\n",
     allocation->width,
     allocation->height);
  */
  /*<h2v-on>*/
  
  layout_rows (wbox, &area);
}
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkVWrapBox: Vertical wrapping box widget
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/* #include "gtkvwrapbox.h" */
/* #include <math.h> */


/* --- prototypes --- */
static void    gtk_vwrap_box_class_init    (GtkVWrapBoxClass   *klass);
static void    gtk_vwrap_box_init          (GtkVWrapBox        *vwbox);
static void    gtk_vwrap_box_size_request  (GtkWidget          *widget,
					    GtkRequisition     *requisition);
static void    gtk_vwrap_box_size_allocate (GtkWidget          *widget,
					    GtkAllocation      *allocation);
static GSList* reverse_list_col_children   (GtkWrapBox         *wbox,
					    GtkWrapBoxChild   **child_p,
					    GtkAllocation      *area,
					    guint              *max_width,
					    gboolean           *can_hexpand);


/* --- variables --- */
static gpointer parent_gtkvwrapbox_class = NULL;


/* --- functions --- */
GtkType
gtk_vwrap_box_get_type (void)
{
  static GtkType vwrap_box_type = 0;
  
  if (!vwrap_box_type)
    {
      static const GtkTypeInfo vwrap_box_info =
      {
	"GtkVWrapBox",
	sizeof (GtkVWrapBox),
	sizeof (GtkVWrapBoxClass),
	(GtkClassInitFunc) gtk_vwrap_box_class_init,
	(GtkObjectInitFunc) gtk_vwrap_box_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      vwrap_box_type = gtk_type_unique (GTK_TYPE_WRAP_BOX, &vwrap_box_info);
    }
  
  return vwrap_box_type;
}

static void
gtk_vwrap_box_class_init (GtkVWrapBoxClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  GtkWrapBoxClass *wrap_box_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  container_class = GTK_CONTAINER_CLASS (class);
  wrap_box_class = GTK_WRAP_BOX_CLASS (class);
  
  parent_gtkvwrapbox_class = gtk_type_class (GTK_TYPE_WRAP_BOX);
  
  widget_class->size_request = gtk_vwrap_box_size_request;
  widget_class->size_allocate = gtk_vwrap_box_size_allocate;

  wrap_box_class->rlist_line_children = reverse_list_col_children;
}

static void
gtk_vwrap_box_init (GtkVWrapBox *vwbox)
{
  vwbox->max_child_height = 0;
  vwbox->max_child_width = 0;
}

GtkWidget*
gtk_vwrap_box_new (gboolean homogeneous)
{
  GtkVWrapBox *vwbox;

  vwbox = GTK_VWRAP_BOX (gtk_widget_new (GTK_TYPE_VWRAP_BOX, NULL));

  GTK_WRAP_BOX (vwbox)->homogeneous = homogeneous ? TRUE : FALSE;

  return GTK_WIDGET (vwbox);
}

static inline void
get_gtkvwrapbox_child_requisition (GtkWrapBox     *wbox,
		       GtkWidget      *child,
		       GtkRequisition *child_requisition)
{
  if (wbox->homogeneous)
    {
      GtkVWrapBox *vwbox = GTK_VWRAP_BOX (wbox);
      
      child_requisition->height = vwbox->max_child_height;
      child_requisition->width = vwbox->max_child_width;
    }
  else
    gtk_widget_get_child_requisition (child, child_requisition);
}

static void
_gtk_vwrap_box_size_request (GtkWidget      *widget,
			     GtkRequisition *requisition)
{
  GtkVWrapBox *this = GTK_VWRAP_BOX (widget);
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  guint area = 0;
  
  g_return_if_fail (requisition != NULL);
  
  /*<h2v-off>*/
  requisition->width = 0;
  requisition->height = 0;
  this->max_child_width = 0;
  this->max_child_height = 0;
  
  for (child = wbox->children; child; child = child->next)
    if (GTK_WIDGET_VISIBLE (child->widget))
      {
	GtkRequisition child_requisition;
	
	gtk_widget_size_request (child->widget, &child_requisition);
	
	area += child_requisition.width * child_requisition.height;
	this->max_child_width = MAX (this->max_child_width, child_requisition.width);
	this->max_child_height = MAX (this->max_child_height, child_requisition.height);
      }
  if (wbox->homogeneous)
    area = this->max_child_width * this->max_child_height * wbox->n_children;
  
  if (area)
    {
      requisition->width = sqrt (area * wbox->aspect_ratio);
      requisition->height = area / requisition->width;
    }
  else
    {
      requisition->width = 0;
      requisition->height = 0;
    }
  
  requisition->width += GTK_CONTAINER (wbox)->border_width * 2;
  requisition->height += GTK_CONTAINER (wbox)->border_width * 2;
  /*<h2v-on>*/
}

static gfloat
get_gtkvwrapbox_layout_size (GtkVWrapBox *this,
		 guint        max_height,
		 guint       *height_inc)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (this);
  GtkWrapBoxChild *child;
  guint n_cols, left_over = 0, total_width = 0;
  gboolean last_col_filled = TRUE;

  *height_inc = this->max_child_height + 1;

  n_cols = 0;
  for (child = wbox->children; child; child = child->next)
    {
      GtkWrapBoxChild *col_child;
      GtkRequisition child_requisition;
      guint col_height, col_width, n = 1;

      if (!GTK_WIDGET_VISIBLE (child->widget))
	continue;

      get_gtkvwrapbox_child_requisition (wbox, child->widget, &child_requisition);
      if (!last_col_filled)
	*height_inc = MIN (*height_inc, child_requisition.height - left_over);
      col_height = child_requisition.height;
      col_width = child_requisition.width;
      for (col_child = child->next; col_child && n < wbox->child_limit; col_child = col_child->next)
	{
	  if (GTK_WIDGET_VISIBLE (col_child->widget))
	    {
	      get_gtkvwrapbox_child_requisition (wbox, col_child->widget, &child_requisition);
	      if (col_height + wbox->vspacing + child_requisition.height > max_height)
		break;
	      col_height += wbox->vspacing + child_requisition.height;
	      col_width = MAX (col_width, child_requisition.width);
	      n++;
	    }
	  child = col_child;
	}
      last_col_filled = n >= wbox->child_limit;
      left_over = last_col_filled ? 0 : max_height - (col_height + wbox->vspacing);
      total_width += (n_cols ? wbox->hspacing : 0) + col_width;
      n_cols++;
    }

  if (*height_inc > this->max_child_height)
    *height_inc = 0;
  
  return MAX (total_width, 1);
}

static void
gtk_vwrap_box_size_request (GtkWidget      *widget,
			    GtkRequisition *requisition)
{
  GtkVWrapBox *this = GTK_VWRAP_BOX (widget);
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkWrapBoxChild *child;
  gfloat ratio_dist, layout_height = 0;
  guint col_inc = 0;
  
  g_return_if_fail (requisition != NULL);
  
  requisition->height = 0;
  requisition->width = 0;
  this->max_child_height = 0;
  this->max_child_width = 0;

  /* size_request all children */
  for (child = wbox->children; child; child = child->next)
    if (GTK_WIDGET_VISIBLE (child->widget))
      {
	GtkRequisition child_requisition;
	
	gtk_widget_size_request (child->widget, &child_requisition);

	this->max_child_height = MAX (this->max_child_height, child_requisition.height);
	this->max_child_width = MAX (this->max_child_width, child_requisition.width);
      }

  /* figure all possible layouts */
  ratio_dist = 32768;
  layout_height = this->max_child_height;
  do
    {
      gfloat layout_width;
      gfloat ratio, dist;

      layout_height += col_inc;
      layout_width = get_gtkvwrapbox_layout_size (this, layout_height, &col_inc);
      ratio = layout_width / layout_height;		/*<h2v-skip>*/
      dist = MAX (ratio, wbox->aspect_ratio) - MIN (ratio, wbox->aspect_ratio);
      if (dist < ratio_dist)
	{
	  ratio_dist = dist;
	  requisition->height = layout_height;
	  requisition->width = layout_width;
	}
      
      /* g_print ("ratio for height %d width %d = %f\n",
	 (gint) layout_height,
	 (gint) layout_width,
	 ratio);
      */
    }
  while (col_inc);

  requisition->width += GTK_CONTAINER (wbox)->border_width * 2; /*<h2v-skip>*/
  requisition->height += GTK_CONTAINER (wbox)->border_width * 2; /*<h2v-skip>*/
  /* g_print ("choosen: height %d, width %d\n",
     requisition->height,
     requisition->width);
  */
}

static GSList*
reverse_list_col_children (GtkWrapBox       *wbox,
			   GtkWrapBoxChild **child_p,
			   GtkAllocation    *area,
			   guint            *max_child_size,
			   gboolean         *expand_line)
{
  GSList *slist = NULL;
  guint height = 0, col_height = area->height;
  GtkWrapBoxChild *child = *child_p;
  
  *max_child_size = 0;
  *expand_line = FALSE;
  
  while (child && !GTK_WIDGET_VISIBLE (child->widget))
    {
      *child_p = child->next;
      child = *child_p;
    }
  
  if (child)
    {
      GtkRequisition child_requisition;
      guint n = 1;
      
      get_gtkvwrapbox_child_requisition (wbox, child->widget, &child_requisition);
      height += child_requisition.height;
      *max_child_size = MAX (*max_child_size, child_requisition.width);
      *expand_line |= child->hexpand;
      slist = g_slist_prepend (slist, child);
      *child_p = child->next;
      child = *child_p;
      
      while (child && n < wbox->child_limit)
	{
	  if (GTK_WIDGET_VISIBLE (child->widget))
	    {
	      get_gtkvwrapbox_child_requisition (wbox, child->widget, &child_requisition);
	      if (height + wbox->vspacing + child_requisition.height > col_height)
		break;
	      height += wbox->vspacing + child_requisition.height;
	      *max_child_size = MAX (*max_child_size, child_requisition.width);
	      *expand_line |= child->hexpand;
	      slist = g_slist_prepend (slist, child);
	      n++;
	    }
	  *child_p = child->next;
	  child = *child_p;
	}
    }
  
  return slist;
}

static void
layout_col (GtkWrapBox    *wbox,
	    GtkAllocation *area,
	    GSList        *children,
	    guint          children_per_line,
	    gboolean       hexpand)
{
  GSList *slist;
  guint n_children = 0, n_expand_children = 0, have_expand_children = 0, total_height = 0;
  gfloat y, height, extra;
  GtkAllocation child_allocation;
  
  for (slist = children; slist; slist = slist->next)
    {
      GtkWrapBoxChild *child = slist->data;
      GtkRequisition child_requisition;
      
      n_children++;
      if (child->vexpand)
	n_expand_children++;
      
      get_gtkvwrapbox_child_requisition (wbox, child->widget, &child_requisition);
      total_height += child_requisition.height;
    }
  
  height = MAX (1, area->height - (n_children - 1) * wbox->vspacing);
  if (height > total_height)
    extra = height - total_height;
  else
    extra = 0;
  have_expand_children = n_expand_children && extra;
  
  y = area->y;
  if (wbox->homogeneous)
    {
      height = MAX (1, area->height - (children_per_line - 1) * wbox->vspacing);
      height /= ((gdouble) children_per_line);
      extra = 0;
    }
  else if (have_expand_children)
    {
      height = extra;
      extra /= ((gdouble) n_expand_children);
    }
  else
    {
      if (wbox->justify == GTK_JUSTIFY_FILL)
	{
	  height = extra;
	  have_expand_children = TRUE;
	  n_expand_children = n_children;
	  extra /= ((gdouble) n_expand_children);
	}
      else if (wbox->justify == GTK_JUSTIFY_CENTER)
	{
	  y += extra / 2;
	  height = 0;
	  extra = 0;
	}
      else if (wbox->justify == GTK_JUSTIFY_LEFT)
	{
	  height = 0;
	  extra = 0;
	}
      else if (wbox->justify == GTK_JUSTIFY_RIGHT)
	{
	  y += extra;
	  height = 0;
	  extra = 0;
	}
    }
  
  n_children = 0;
  for (slist = children; slist; slist = slist->next)
    {
      GtkWrapBoxChild *child = slist->data;
      
      child_allocation.y = y;
      child_allocation.x = area->x;
      if (wbox->homogeneous)
	{
	  child_allocation.width = area->width;
	  child_allocation.height = height;
	  y += child_allocation.height + wbox->vspacing;
	}
      else
	{
	  GtkRequisition child_requisition;
	  
	  get_gtkvwrapbox_child_requisition (wbox, child->widget, &child_requisition);
	  
	  if (child_requisition.width >= area->width)
	    child_allocation.width = area->width;
	  else
	    {
	      child_allocation.width = child_requisition.width;
	      if (wbox->line_justify == GTK_JUSTIFY_FILL || child->hfill)
		child_allocation.width = area->width;
	      else if (child->hexpand || wbox->line_justify == GTK_JUSTIFY_CENTER)
		child_allocation.x += (area->width - child_requisition.width) / 2;
	      else if (wbox->line_justify == GTK_JUSTIFY_BOTTOM)
		child_allocation.x += area->width - child_requisition.width;
	    }
	  
	  if (have_expand_children)
	    {
	      child_allocation.height = child_requisition.height;
	      if (child->vexpand || wbox->justify == GTK_JUSTIFY_FILL)
		{
		  guint space;
		  
		  n_expand_children--;
		  space = extra * n_expand_children;
		  space = height - space;
		  height -= space;
		  if (child->vfill)
		    child_allocation.height += space;
		  else
		    {
		      child_allocation.y += space / 2;
		      y += space;
		    }
		}
	    }
	  else
	    {
	      /* g_print ("child_allocation.y %d += %d * %f ",
		       child_allocation.y, n_children, extra); */
	      child_allocation.y += n_children * extra;
	      /* g_print ("= %d\n",
		       child_allocation.y); */
	      child_allocation.height = MIN (child_requisition.height,
					    area->height - child_allocation.y + area->y);
	    }
	}
      
      y += child_allocation.height + wbox->vspacing;
      gtk_widget_size_allocate (child->widget, &child_allocation);
      n_children++;
    }
}

typedef struct _Line_gtkvwrapbox_ Line_gtkvwrapbox_;
struct _Line_gtkvwrapbox_
{
  GSList  *children;
  guint16  min_size;
  guint    expand : 1;
  Line_gtkvwrapbox_     *next;
};

static void
layout_cols (GtkWrapBox    *wbox,
	     GtkAllocation *area)
{
  GtkWrapBoxChild *next_child;
  guint min_width;
  gboolean hexpand;
  GSList *slist;
  Line_gtkvwrapbox_ *line_list = NULL;
  guint total_width = 0, n_expand_lines = 0, n_lines = 0;
  gfloat shrink_width;
  guint children_per_line;
  
  next_child = wbox->children;
  slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
							      &next_child,
							      area,
							      &min_width,
							      &hexpand);
  slist = g_slist_reverse (slist);

  children_per_line = g_slist_length (slist);
  while (slist)
    {
      Line_gtkvwrapbox_ *line = g_new (Line_gtkvwrapbox_, 1);
      
      line->children = slist;
      line->min_size = min_width;
      total_width += min_width;
      line->expand = hexpand;
      if (hexpand)
	n_expand_lines++;
      line->next = line_list;
      line_list = line;
      n_lines++;
      
      slist = GTK_WRAP_BOX_GET_CLASS (wbox)->rlist_line_children (wbox,
								  &next_child,
								  area,
								  &min_width,
								  &hexpand);
      slist = g_slist_reverse (slist);
    }
  
  if (total_width > area->width)
    shrink_width = total_width - area->width;
  else
    shrink_width = 0;
  
  if (1) /* reverse lines and shrink */
    {
      Line_gtkvwrapbox_ *prev = NULL, *last = NULL;
      gfloat n_shrink_lines = n_lines;
      
      while (line_list)
	{
	  Line_gtkvwrapbox_ *tmp = line_list->next;

	  if (shrink_width)
	    {
	      Line_gtkvwrapbox_ *line = line_list;
	      guint shrink_fract = shrink_width / n_shrink_lines + 0.5;

	      if (line->min_size > shrink_fract)
		{
		  shrink_width -= shrink_fract;
		  line->min_size -= shrink_fract;
		}
	      else
		{
		  shrink_width -= line->min_size - 1;
		  line->min_size = 1;
		}
	    }
	  n_shrink_lines--;

	  last = line_list;
	  line_list->next = prev;
	  prev = line_list;
	  line_list = tmp;
	}
      line_list = last;
    }
  
  if (n_lines)
    {
      Line_gtkvwrapbox_ *line;
      gfloat x, width, extra = 0;
      
      width = area->width;
      width = MAX (n_lines, width - (n_lines - 1) * wbox->hspacing);
      
      if (wbox->homogeneous)
	width /= ((gdouble) n_lines);
      else if (n_expand_lines)
	{
	  width = MAX (0, width - total_width);
	  extra = width / ((gdouble) n_expand_lines);
	}
      else
	width = 0;
      
      x = area->x;
      line = line_list;
      while (line)
	{
	  GtkAllocation col_allocation;
	  Line_gtkvwrapbox_ *next_line = line->next;
	  
	  col_allocation.y = area->y;
	  col_allocation.height = area->height;
	  if (wbox->homogeneous)
	    col_allocation.width = width;
	  else
	    {
	      col_allocation.width = line->min_size;
	      
	      if (line->expand)
		col_allocation.width += extra;
	    }
	  
	  col_allocation.x = x;
	  
	  x += col_allocation.width + wbox->hspacing;
	  layout_col (wbox,
		      &col_allocation,
		      line->children,
		      children_per_line,
		      line->expand);
	  
	  g_slist_free (line->children);
	  g_free (line);
	  line = next_line;
	}
    }
}

static void
gtk_vwrap_box_size_allocate (GtkWidget     *widget,
			     GtkAllocation *allocation)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (widget);
  GtkAllocation area;
  guint border = GTK_CONTAINER (wbox)->border_width; /*<h2v-skip>*/
  
  widget->allocation = *allocation;
  area.y = allocation->y + border;
  area.x = allocation->x + border;
  area.height = MAX (1, (gint) allocation->height - border * 2);
  area.width = MAX (1, (gint) allocation->width - border * 2);
  
  /*<h2v-off>*/
  /* g_print ("got: width %d, height %d\n",
     allocation->width,
     allocation->height);
  */
  /*<h2v-on>*/
  
  layout_cols (wbox, &area);
}
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

