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
  CHILD_PROP_0,
  CHILD_PROP_POSITION,
  CHILD_PROP_HEXPAND,
  CHILD_PROP_HFILL,
  CHILD_PROP_VEXPAND,
  CHILD_PROP_VFILL,
  CHILD_PROP_WRAPPED
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
static void gtk_wrap_box_set_child_property (GtkContainer    *container,
					     GtkWidget       *child,
					     guint            property_id,
					     const GValue    *value,
					     GParamSpec      *pspec);
static void gtk_wrap_box_get_child_property (GtkContainer    *container,
					     GtkWidget       *child,
					     guint            property_id,
					     GValue          *value,
					     GParamSpec      *pspec);
static void gtk_wrap_box_map           (GtkWidget          *widget);
static void gtk_wrap_box_unmap         (GtkWidget          *widget);
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
  
  parent_gtkwrapbox_class = g_type_class_peek_parent (class);
  
  object_class->set_arg = gtk_wrap_box_set_arg;
  object_class->get_arg = gtk_wrap_box_get_arg;
  
  widget_class->map = gtk_wrap_box_map;
  widget_class->unmap = gtk_wrap_box_unmap;
  widget_class->expose_event = gtk_wrap_box_expose;
  
  container_class->add = gtk_wrap_box_add;
  container_class->remove = gtk_wrap_box_remove;
  container_class->forall = gtk_wrap_box_forall;
  container_class->child_type = gtk_wrap_box_child_type;
  container_class->set_child_property = gtk_wrap_box_set_child_property;
  container_class->get_child_property = gtk_wrap_box_get_child_property;

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

  gtk_container_class_install_child_property (container_class, CHILD_PROP_POSITION,
					      g_param_spec_int ("position", NULL, NULL,
								-1, G_MAXINT, 0,
								G_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class, CHILD_PROP_HEXPAND,
					      g_param_spec_boolean ("hexpand", NULL, NULL,
								    FALSE,
								    G_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class, CHILD_PROP_HFILL,
					      g_param_spec_boolean ("hfill", NULL, NULL,
								    FALSE,
								    G_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class, CHILD_PROP_VEXPAND,
					      g_param_spec_boolean ("vexpand", NULL, NULL,
								    FALSE,
								    G_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class, CHILD_PROP_VFILL,
					      g_param_spec_boolean ("vfill", NULL, NULL,
								    FALSE,
								    G_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class, CHILD_PROP_VFILL,
					      g_param_spec_boolean ("wrapped", NULL, NULL,
								    FALSE,
								    G_PARAM_READWRITE));
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
gtk_wrap_box_set_child_property (GtkContainer    *container,
				 GtkWidget       *child,
				 guint            property_id,
				 const GValue    *value,
				 GParamSpec      *pspec)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  gboolean hexpand = FALSE, hfill = FALSE, vexpand = FALSE, vfill = FALSE, wrapped = FALSE;
  
  if (property_id != CHILD_PROP_POSITION)
    gtk_wrap_box_query_child_packing (wbox, child, &hexpand, &hfill, &vexpand, &vfill, &wrapped);
  
  switch (property_id)
    {
    case CHILD_PROP_POSITION:
      gtk_wrap_box_reorder_child (wbox, child, g_value_get_int (value));
      break;
    case CHILD_PROP_HEXPAND:
      gtk_wrap_box_set_child_packing (wbox, child,
				      g_value_get_boolean (value), hfill,
				      vexpand, vfill,
				      wrapped);
      break;
    case CHILD_PROP_HFILL:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, g_value_get_boolean (value),
				      vexpand, vfill,
				      wrapped);
      break;
    case CHILD_PROP_VEXPAND:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, hfill,
				      g_value_get_boolean (value), vfill,
				      wrapped);
      break;
    case CHILD_PROP_VFILL:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, hfill,
				      vexpand, g_value_get_boolean (value),
				      wrapped);
      break;
    case CHILD_PROP_WRAPPED:
      gtk_wrap_box_set_child_packing (wbox, child,
				      hexpand, hfill,
				      vexpand, vfill,
				      g_value_get_boolean (value));
      break;
    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
gtk_wrap_box_get_child_property (GtkContainer    *container,
				 GtkWidget       *child,
				 guint            property_id,
				 GValue 	 *value,
				 GParamSpec      *pspec)
{
  GtkWrapBox *wbox = GTK_WRAP_BOX (container);
  gboolean hexpand = FALSE, hfill = FALSE, vexpand = FALSE, vfill = FALSE, wrapped = FALSE;
  
  if (property_id != CHILD_PROP_POSITION)
    gtk_wrap_box_query_child_packing (wbox, child, &hexpand, &hfill, &vexpand, &vfill, &wrapped);
  
  switch (property_id)
    {
      GtkWrapBoxChild *child_info;
      guint i;
    case CHILD_PROP_POSITION:
      i = 0;
      for (child_info = wbox->children; child_info; child_info = child_info->next)
	{
	  if (child_info->widget == child)
	    break;
	  i += 1;
	}
      g_value_set_int (value, child_info ? i : -1);
      break;
    case CHILD_PROP_HEXPAND:
      g_value_set_boolean (value, hexpand);
      break;
    case CHILD_PROP_HFILL:
      g_value_set_boolean (value, hfill);
      break;
    case CHILD_PROP_VEXPAND:
      g_value_set_boolean (value, vexpand);
      break;
    case CHILD_PROP_VFILL:
      g_value_set_boolean (value, vfill);
      break;
    case CHILD_PROP_WRAPPED:
      g_value_set_boolean (value, wrapped);
      break;
    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
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
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == NULL);

  gtk_wrap_box_pack_wrapped (wbox, child, hexpand, hfill, vexpand, vfill, FALSE);
}

void
gtk_wrap_box_pack_wrapped (GtkWrapBox *wbox,
			   GtkWidget  *child,
			   gboolean    hexpand,
			   gboolean    hfill,
			   gboolean    vexpand,
			   gboolean    vfill,
			   gboolean    wrapped)
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
  child_info->wrapped = wrapped ? TRUE : FALSE;
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
				  gboolean   *vfill,
				  gboolean   *wrapped)
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
      if (wrapped)
	*wrapped = child_info->wrapped;
    }
}

void
gtk_wrap_box_set_child_packing (GtkWrapBox *wbox,
				GtkWidget  *child,
				gboolean    hexpand,
				gboolean    hfill,
				gboolean    vexpand,
				gboolean    vfill,
				gboolean    wrapped)
{
  GtkWrapBoxChild *child_info;
  
  g_return_if_fail (GTK_IS_WRAP_BOX (wbox));
  g_return_if_fail (GTK_IS_WIDGET (child));
  
  hexpand = hexpand != FALSE;
  hfill = hfill != FALSE;
  vexpand = vexpand != FALSE;
  vfill = vfill != FALSE;
  wrapped = wrapped != FALSE;

  for (child_info = wbox->children; child_info; child_info = child_info->next)
    if (child_info->widget == child)
      break;
  
  if (child_info &&
      (child_info->hexpand != hexpand || child_info->vexpand != vexpand ||
       child_info->hfill != hfill || child_info->vfill != vfill ||
       child_info->wrapped != wrapped))
    {
      child_info->hexpand = hexpand;
      child_info->hfill = hfill;
      child_info->vexpand = vexpand;
      child_info->vfill = vfill;
      child_info->wrapped = wrapped;
      
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

static gint
gtk_wrap_box_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
  return GTK_WIDGET_CLASS (parent_gtkwrapbox_class)->expose_event (widget, event);
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
	      if (width + wbox->hspacing + child_requisition.width > row_width ||
		  child->wrapped)
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
  guint n_children = 0, n_expand_children = 0, have_expand_children = 0;
  gint total_width = 0;
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
  else if (have_expand_children && wbox->justify != GTK_JUSTIFY_FILL)
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
  gint border = GTK_CONTAINER (wbox)->border_width; /*<h2v-skip>*/
  
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
	      if (height + wbox->vspacing + child_requisition.height > col_height ||
		  child->wrapped)
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
  guint n_children = 0, n_expand_children = 0, have_expand_children = 0;
  gint total_height = 0;
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
  else if (have_expand_children && wbox->justify != GTK_JUSTIFY_FILL)
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
  gint border = GTK_CONTAINER (wbox)->border_width; /*<h2v-skip>*/
  
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
