#include "bstutils.h"
#include "gnomeforest.h"







/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1998 Raph Levien
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
#include	<libart_lgpl/art_misc.h>
#include	<libart_lgpl/art_svp.h>
#include	<libart_lgpl/art_svp_vpath.h>
#include	<libart_lgpl/art_svp_point.h>

#include <math.h>

#define EPSILON 1e-6

#define POINTS_PREALLOC	     (32)

typedef enum
{
  D_EAST,
  D_NORTH,
  D_WEST,
  D_SOUTH
} Direction;

typedef struct
{
  int x, y;
  Direction dir;
  
  int width, height;
  int alpha_threshold, rowstride;
  art_u8 *pixels;
  
  int n_points, n_alloced_points;
  ArtVpath *vpath;
  ArtSVP *svp;
} VC;


/* --- short hand macros --- */
#define pixel_offset(vc, x, y) ((vc)->pixels + (y) * (vc)->rowstride + (x) * 4)
#define is_pixel(vc, x, y)     (pixel_offset ((vc), (x), (y)) [3] >= (vc)->alpha_threshold)
#define dir_left(dir)	       ((dir) == D_SOUTH ? D_EAST : (dir) + 1)
#define dir_right(dir)	       ((dir) == D_EAST ? D_SOUTH : (dir) - 1)
#define turn_left(vc)          ((void) ((vc)->dir = dir_left ((vc)->dir)))
#define turn_right(vc)         ((void) ((vc)->dir = dir_right ((vc)->dir)))
#define	dir_opposite(dir)      ((dir) > D_NORTH ? (dir) - 2 : (dir) + 2)
#define	dir_name(dir)	       ((dir) == D_EAST  ? "EAST" : \
                                (dir) == D_NORTH ? "NORTH" : \
                                (dir) == D_WEST  ? "WEST" : \
                                                   "SOUTH")


/* --- functions --- */
static inline int
move_towards (VC *vc, Direction dir, int *x_p, int *y_p)
{
  switch (dir)
    {
    case D_EAST:  *x_p += 1; break;
    case D_NORTH: *y_p -= 1; break;
    case D_WEST:  *x_p -= 1; break;
    case D_SOUTH: *y_p += 1; break;
    }
  return *y_p >= 0 && *x_p >= 0 && *y_p < vc->height && *x_p < vc->width;
}

static inline int
pixel_ahead (VC *vc)
{
  int x = vc->x, y = vc->y;
  
  return move_towards (vc, vc->dir, &x, &y) && is_pixel (vc, x, y);
}
static inline int
pixel_left (VC *vc)
{
  int x = vc->x, y = vc->y;
  
  return move_towards (vc, dir_left (vc->dir), &x, &y) && is_pixel (vc, x, y);
}
static inline int
pixel_right (VC *vc)
{
  int x = vc->x, y = vc->y;
  
  return move_towards (vc, dir_right (vc->dir), &x, &y) && is_pixel (vc, x, y);
}
static inline void
add_point (VC *vc, double x, double y)
{
  int i = vc->n_points;
  
  vc->n_points++;
  if (vc->n_points == vc->n_alloced_points)
    art_expand (vc->vpath, ArtVpath, vc->n_alloced_points);
  vc->vpath[i].code = ART_LINETO;
  vc->vpath[i].x = x;
  vc->vpath[i].y = y;
}

static inline void
add_rear (VC *vc)
{
  double x = vc->x, y = vc->y;
  int dir = vc->dir;
  
  /* depending on the direction we are facing, we have to add the coordinates
   * of the inner pixel corner at our rear.
   * offsets are as follows:
   *
   * dir facing  rear coords: x  y
   * D_EAST:		     +0 +0
   * D_NORTH:		     +0 +1
   * D_WEST:		     +1 +1
   * D_SOUTH:		     +1 +0
   */
  
  x += dir > D_NORTH;
  y += dir && dir < D_SOUTH;
  
  add_point (vc, x, y);
}

static void
vectorize_outline (VC *vc, int i_x, int i_y)
{
  double x = --i_x, y = i_y;
  int i, start;
  
  /* this function needs to be given a pixel that has no neighbour above
   * or left to it. after surrounding this pixel (plus adherents), the
   * path can only be closed with a left turn.
   */
  
  /* setup vc */
  vc->x = x;
  vc->y = y;
  vc->dir = D_SOUTH;
  vc->n_points -= 1;
  start = vc->n_points;
  
  /* setup starting point */
  add_rear (vc);
  vc->vpath[start].code = ART_MOVETO;
  
  /* vectorize until path is closed */
  do
    {
      while (pixel_left (vc))
	if (pixel_ahead (vc))
	  {
	    turn_right (vc);
	    add_rear (vc);
	    continue;
	  }
	else
	  move_towards (vc, vc->dir, &vc->x, &vc->y);
      
      turn_left (vc);
      move_towards (vc, vc->dir, &vc->x, &vc->y);
      i = vc->n_points;
      add_rear (vc);
    }
  while (fabs (vc->vpath[i].x - vc->vpath[start].x) >= EPSILON ||
	 fabs (vc->vpath[i].y - vc->vpath[start].y) >= EPSILON);
  
  /* end mark */
  i = vc->n_points;
  add_point (vc, 0, 0);
  vc->vpath[i].code = ART_END;
}

ArtVpath*
art_vpath_outline_from_pixbuf (const ArtPixBuf *pixbuf,
			       int              alpha_threshold)
{
  VC vectorization_context = { 0, }, *vc = &vectorization_context;
  int x, y;
  
  /* sanity checks */
  if (pixbuf->format != ART_PIX_RGB ||
      !pixbuf->has_alpha ||
      pixbuf->n_channels != 4 ||
      pixbuf->bits_per_sample != 8 ||
      !pixbuf->width ||
      !pixbuf->height)
    {
      /* printf ("unable to vectorize pixbuf\n"); */
      
      return NULL;
    }
  
  /* initial vc setup */
  vc->width = pixbuf->width;
  vc->height = pixbuf->height;
  vc->alpha_threshold = alpha_threshold;
  vc->rowstride = pixbuf->rowstride;
  vc->pixels = pixbuf->pixels;
  vc->n_alloced_points = 4;
  vc->n_points = 1;
  vc->vpath = art_new (ArtVpath, vc->n_alloced_points);
  vc->vpath->code = ART_END;
  vc->vpath->x = 0;
  vc->vpath->y = 0;
  vc->svp = NULL;
  
  /* find first pixel */
  for (y = 0; y < vc->height; y++)
    {
      for (x = 0; x < vc->width; x++)
	if (is_pixel (vc, x, y))
	  {
	    int xn;
	    
	    /* check if this point is already contained */
	    if (!vc->svp ||
		!art_svp_point_wind (vc->svp, x + 0.5, y + 0.5))
	      {
		/* pixel not yet contained, vectorize from here on */
		vectorize_outline (vc, x, y);
		if (vc->svp)
		  art_svp_free (vc->svp);
		vc->svp = art_svp_from_vpath (vc->vpath);
	      }
	    
	    /* eat contiguous pixels */
	    xn = x + 1;
	    while (xn < vc->width && is_pixel (vc, xn, y))
	      x = xn++;
	  }
    }
  
  /* if (vc->n_points == 1)
   *   printf ("unable to vectorize empty pixbuf\n");
   */
  
  if (vc->svp)
    {
      art_svp_free (vc->svp);
      vc->svp = NULL;
    }
  
  /* adjust size of closed outline path */
  vc->vpath = art_renew (vc->vpath, ArtVpath, vc->n_points);
  
  return vc->vpath;
}

int
art_affine_from_triplets (const ArtTriplet *src_triplet,
			  const ArtTriplet *transformed_triplet,
			  double matrix[6])
{
  double Ax = src_triplet->ax, Ay = src_triplet->ay;
  double Bx = src_triplet->bx, By = src_triplet->by;
  double Cx = src_triplet->cx, Cy = src_triplet->cy;
  double ax = transformed_triplet->ax, ay = transformed_triplet->ay;
  double bx = transformed_triplet->bx, by = transformed_triplet->by;
  double cx = transformed_triplet->cx, cy = transformed_triplet->cy;
  
  /* solve the linear equation system:
   *   ax = Ax * matrix[0] + Ay * matrix[2] + matrix[4];
   *   ay = Ax * matrix[1] + Ay * matrix[3] + matrix[5];
   *   bx = Bx * matrix[0] + By * matrix[2] + matrix[4];
   *   by = Bx * matrix[1] + By * matrix[3] + matrix[5];
   *   cx = Cx * matrix[0] + Cy * matrix[2] + matrix[4];
   *   cy = Cx * matrix[1] + Cy * matrix[3] + matrix[5];
   * for matrix[0..5]
   */
  double AxBy = Ax * By, AyBx = Ay * Bx;
  double AxCy = Ax * Cy, AyCx = Ay * Cx;
  double BxCy = Bx * Cy, ByCx = By * Cx;
  double com_div = AxBy - AyBx - AxCy + AyCx + BxCy - ByCx;
  
  if (fabs (com_div) < EPSILON)
    return ART_FALSE;
  
  matrix[0] = By * ax - Ay * bx + Ay * cx - Cy * ax - By * cx + Cy * bx;
  matrix[1] = By * ay - Ay * by + Ay * cy - Cy * ay - By * cy + Cy * by;
  matrix[2] = Ax * bx - Bx * ax - Ax * cx + Cx * ax + Bx * cx - Cx * bx;
  matrix[3] = Ax * by - Bx * ay - Ax * cy + Cx * ay + Bx * cy - Cx * by;
  matrix[4] = AxBy * cx - AxCy * bx - AyBx * cx + AyCx * bx + BxCy * ax - ByCx * ax;
  matrix[5] = AxBy * cy - AxCy * by - AyBx * cy + AyCx * by + BxCy * ay - ByCx * ay;
  matrix[0] /= com_div;
  matrix[1] /= com_div;
  matrix[2] /= com_div;
  matrix[3] /= com_div;
  matrix[4] /= com_div;
  matrix[5] /= com_div;
  
  return ART_TRUE;
}

double
art_vpath_area (const ArtVpath *vpath)
{
  double area = 0;
  
  while (vpath[0].code != ART_END)
    {
      int n_points, i;
      
      for (n_points = 1; vpath[n_points].code == ART_LINETO; n_points++)
	;
      
      for (i = 0; i < n_points; i++)
	{
	  int j = (i + 1) % n_points;
	  
	  area += vpath[j].x * vpath[i].y - vpath[i].x * vpath[j].y;
	}
      
      vpath += n_points;
    }
  
  return area * 0.5;
}








/* GnomeForest - Gnome Sprite Engine
 * Copyright (C) 1999 Tim Janik
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "gnomeforest.h"

#include <libart_lgpl/art_rgb_pixbuf_affine.h>
#include <libart_lgpl/art_rgb.h>
#include <libart_lgpl/art_rgb_svp.h>
#include <libart_lgpl/art_uta_rect.h>
#include <libart_lgpl/art_uta_ops.h>
#include <libart_lgpl/art_uta_vpath.h>
#include <libart_lgpl/art_rect_uta.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_svp_ops.h>
#include <libart_lgpl/art_vpath_svp.h>
#include <math.h>

#define EPSILON 1e-6

enum {
  ARG_0,
  ARG_EXPAND_FOREST,
};

/* --- signals --- */
enum
{
  SIGNAL_COLLISION,
  SIGNAL_LAST
};
typedef void (*SignalCollision) (GnomeForest          *forest,
				 guint                 n_collisions,
				 GnomeSpriteCollision *collisions,
				 gpointer              func_data);


/* --- prototypes --- */
static void	gnome_forest_class_init		(GnomeForestClass	*class);
static void	gnome_forest_init		(GnomeForest		*forest);
static void	gnome_forest_set_arg		(GtkObject	   	*object,
						 GtkArg	   		*arg,
						 guint	   		 arg_id);
static void	gnome_forest_get_arg		(GtkObject		*object,
						 GtkArg	   		*arg,
						 guint	   		 arg_id);
static void	gnome_forest_finalize		(GObject		*object);
static void	gnome_forest_size_request	(GtkWidget		*widget,
						 GtkRequisition		*requisition);
static void	gnome_forest_size_allocate	(GtkWidget		*widget,
						 GtkAllocation		*allocation);
static void     gnome_forest_state_changed      (GtkWidget              *widget,
						 GtkStateType            previous_state);
static void     gnome_forest_style_set          (GtkWidget              *widget,
						 GtkStyle               *previous_style);

static void	gnome_forest_realize		(GtkWidget		*widget);
static gboolean	gnome_forest_expose		(GtkWidget		*widget,
						 GdkEventExpose		*event);
static void     gnome_forest_queue_update	(GnomeForest 		*forest);
static void	gnome_forest_queue_vpath	(GnomeForest		*forest,
						 const ArtVpath		*vpath);
static void	gnome_forest_queue_area		(GnomeForest		*forest,
						 gboolean  		 rerender,
						 gint      		 x,
						 gint      		 y,
						 gint      		 width,
						 gint      		 height);
static gboolean gnome_forest_collisions		(GnomeForest		*forest);
static gboolean gnome_sprite_check_update 	(GnomeForest		*forest,
						 GnomeSprite		*sprite);
static gboolean sprite_ensure_vpath		(GnomeForest		*forest,
						 GnomeSprite		*sprite);


/* --- variables --- */
static gpointer parent_class = NULL;
static guint	forest_signals[SIGNAL_LAST] = { 0 };
static GQuark	quark_animators = 0;


/* --- prototypes --- */
GtkType
gnome_forest_get_type (void)
{
  static GtkType forest_type = 0;
  
  if (!forest_type)
    {
      static const GtkTypeInfo forest_info =
      {
	"GnomeForest",
	sizeof (GnomeForest),
	sizeof (GnomeForestClass),
	(GtkClassInitFunc) gnome_forest_class_init,
	(GtkObjectInitFunc) gnome_forest_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      forest_type = gtk_type_unique (GTK_TYPE_WIDGET, &forest_info);
    }
  
  return forest_type;
}

static void
gnome_forest_marshal_collision (GtkObject    *object,
				GtkSignalFunc func,
				gpointer      func_data,
				GtkArg       *args)
{
  SignalCollision sfunc = (SignalCollision) func;
  
  sfunc ((GnomeForest*) object,
	 GTK_VALUE_UINT (args[0]),
	 GTK_VALUE_POINTER (args[1]),
	 func_data);
}

static void
gnome_forest_class_init (GnomeForestClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  widget_class = GTK_WIDGET_CLASS (class);
  
  parent_class = gtk_type_class (GTK_TYPE_WIDGET);
  
  quark_animators = g_quark_from_static_string ("gnome-forest-animators");
  
  G_OBJECT_CLASS (object_class)->finalize = gnome_forest_finalize;

  object_class->set_arg = gnome_forest_set_arg;
  object_class->get_arg = gnome_forest_get_arg;
  
  widget_class->size_request = gnome_forest_size_request;
  widget_class->size_allocate = gnome_forest_size_allocate;
  widget_class->realize = gnome_forest_realize;
  widget_class->state_changed = gnome_forest_state_changed;
  widget_class->style_set = gnome_forest_style_set;
  widget_class->expose_event = gnome_forest_expose;
  
  class->collision = NULL;
  
  gtk_object_add_arg_type ("GnomeForest::expand_forest", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_EXPAND_FOREST);
  
  forest_signals[SIGNAL_COLLISION] =
    gtk_signal_new ("collision",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (GnomeForestClass, collision),
		    bst_marshal_NONE__UINT_POINTER,
		    GTK_TYPE_NONE, 2, GTK_TYPE_UINT, GTK_TYPE_POINTER);
}

static void
gnome_forest_init (GnomeForest *forest)
{
  forest->n_sprites = 0;
  forest->sprites = NULL;
  forest->update_queued = 0;
  forest->expand_forest = TRUE;
  forest->buffer_size = 0;
  forest->buffer = NULL;
  forest->render_uta = NULL;
  forest->paint_uta = NULL;
  g_datalist_init (&forest->animdata);
  gtk_widget_set_double_buffered (GTK_WIDGET (forest), FALSE);
}

static void
gnome_forest_set_arg (GtkObject	  *object,
		      GtkArg	  *arg,
		      guint	   arg_id)
{
  GnomeForest *forest = GNOME_FOREST (object);
  
  switch (arg_id)
    {
    case ARG_EXPAND_FOREST:
      forest->expand_forest = GTK_VALUE_BOOL (*arg);
      gtk_widget_queue_resize (GTK_WIDGET (forest));
      break;
    default:
      break;
    }
}

static void
gnome_forest_get_arg (GtkObject	  *object,
		      GtkArg	  *arg,
		      guint	   arg_id)
{
  GnomeForest *forest;
  
  forest = GNOME_FOREST (object);
  
  switch (arg_id)
    {
    case ARG_EXPAND_FOREST:
      GTK_VALUE_BOOL (*arg) = forest->expand_forest;
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static void
gnome_forest_finalize (GObject *object)
{
  GnomeForest *forest = GNOME_FOREST (object);
  guint i;
  
  g_datalist_clear (&forest->animdata);
  
  for (i = 0; i < forest->n_sprites; i++)
    {
      GnomeSprite *sprite = forest->sprites + i;
      
      if (sprite->svp)
	art_svp_free (sprite->svp);
      if (sprite->outline)
	art_free (sprite->outline);
      if (sprite->vpath)
	art_free (sprite->vpath);
      art_pixbuf_free (sprite->pixbuf);
    }
  g_free (forest->sprites);
  
  g_free (forest->buffer);
  if (forest->render_uta)
    art_uta_free (forest->render_uta);
  if (forest->paint_uta)
    art_uta_free (forest->paint_uta);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gnome_forest_size_request (GtkWidget      *widget,
			   GtkRequisition *requisition)
{
  GnomeForest *forest;
  guint i;
  
  g_return_if_fail (requisition != NULL);
  
  forest = GNOME_FOREST (widget);
  
  requisition->width = 1;
  requisition->height = 1;

  /* walk all visible sprites and request a region that is
   * big enough to fit them all in
   */
  for (i = 0; i < forest->n_sprites; i++)
    {
      GnomeSprite *sprite = forest->sprites + i;
      ArtVpath *vpath;

      /* ignore invisible sprites */
      if (!sprite->visible)
	continue;
      sprite_ensure_vpath (forest, sprite);
      vpath = sprite->vpath;
      while (vpath->code != ART_END)
	{
	  /* ignore dimensions that get scaled to allocation */
	  if (sprite->width > 0)
	    {
	      guint c = ceil (vpath->x);

	      requisition->width = MAX (requisition->width, MIN (32766, c));
	    }
	  if (sprite->height > 0)
	    {
	      guint c = ceil (vpath->y);

	      requisition->height = MAX (requisition->height, MIN (32766, c));
	    }
	  vpath++;
	}
    }
}

static void
gnome_forest_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
  GnomeForest *forest = GNOME_FOREST (widget);
  GtkRequisition requisition;
  guint i;

  g_return_if_fail (allocation != NULL);

  gtk_widget_get_child_requisition (widget, &requisition);

  /* assign new allocation */
  widget->allocation = *allocation;
  if (!forest->expand_forest)
    {
      if (widget->allocation.width > requisition.width)
	{
	  widget->allocation.x += (widget->allocation.width - requisition.width) / 2;
	  widget->allocation.width = requisition.width;
	}
      if (widget->allocation.height > requisition.height)
	{
	  widget->allocation.y += (widget->allocation.height - requisition.height) / 2;
	  widget->allocation.height = requisition.height;
	}
    }
  allocation = &widget->allocation;
  
  /* adjust gdk window */
  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (widget->window,
			    allocation->x,
			    allocation->y,
			    allocation->width,
			    allocation->height);
  
  /* resize rendering buffer */
  forest->buffer_size = allocation->width * allocation->height * 3;
  g_free (forest->buffer);
  forest->buffer = g_new (guint8, forest->buffer_size);
  
  /* rerender complete buffer */
  gnome_forest_queue_area (forest, TRUE,
			   0, 0,
			   allocation->width,
			   allocation->height);
  
  /* update sprite layouts (some are widget->allocation dependant) */
  for (i = 0; i < forest->n_sprites; i++)
    gnome_sprite_check_update (forest, forest->sprites + i);
}

static void
gnome_forest_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  guint attributes_mask;
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);
  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_WIDGET_STATE (widget));
  gdk_window_set_static_gravities (widget->window, TRUE);
}

static void
gnome_forest_state_changed (GtkWidget   *widget,
                            GtkStateType previous_state)
{
  /* rerender complete buffer */
  gnome_forest_queue_area (GNOME_FOREST (widget), TRUE,
                           0, 0,
                           widget->allocation.width,
                           widget->allocation.height);
  if (GTK_WIDGET_REALIZED (widget))
    gtk_style_set_background (widget->style, widget->window, GTK_WIDGET_STATE (widget));
  
  if (GTK_WIDGET_CLASS (parent_class)->state_changed)
    GTK_WIDGET_CLASS (parent_class)->state_changed (widget, previous_state);
}

static void
gnome_forest_style_set (GtkWidget *widget,
                        GtkStyle  *previous_style)
{
  /* rerender complete buffer */
  gnome_forest_queue_area (GNOME_FOREST (widget), TRUE,
                           0, 0,
                           widget->allocation.width,
                           widget->allocation.height);
  if (GTK_WIDGET_REALIZED (widget))
    gtk_style_set_background (widget->style, widget->window, GTK_WIDGET_STATE (widget));
  
  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);
}

static gboolean
gnome_forest_expose (GtkWidget      *widget,
		     GdkEventExpose *event)
{
  GnomeForest *forest = GNOME_FOREST (widget);
  
  g_return_val_if_fail (event != NULL, FALSE);
  
  if (GTK_WIDGET_DRAWABLE (forest))
    gnome_forest_queue_area (forest, FALSE,
			     event->area.x, event->area.y,
			     event->area.width,
			     event->area.height);
  
  return TRUE;
}

GtkWidget*
gnome_forest_new (void)
{
  return gtk_widget_new (GNOME_TYPE_FOREST, NULL);
}

void
gnome_forest_rerender (GnomeForest *forest)
{
  GtkWidget *widget;

  g_return_if_fail (GNOME_IS_FOREST (forest));

  widget = GTK_WIDGET (forest);

  /* rerender complete buffer */
  gnome_forest_queue_area (forest, TRUE,
			   0, 0,
			   widget->allocation.width,
			   widget->allocation.height);
}

GnomeSprite*
gnome_forest_peek_sprite (GnomeForest *forest,
			  guint        id)
{
  guint i;
  
  g_return_val_if_fail (GNOME_IS_FOREST (forest), NULL);
  
  for (i = 0; i < forest->n_sprites; i++)
    if (id == forest->sprites[i].id)
      return forest->sprites + i;
  
  return NULL;
}

guint
gnome_forest_put_sprite (GnomeForest *forest,
			 guint        id,
			 ArtPixBuf   *image)
{
  GnomeSprite *sprite = NULL;
  gboolean visible;
  guint i;
  
  g_return_val_if_fail (GNOME_IS_FOREST (forest), 0);
  g_return_val_if_fail (image != NULL, 0);
  
  for (i = 0; i < forest->n_sprites; i++)
    {
      if (id == forest->sprites[i].id)
	{
	  sprite = forest->sprites + i;
	  break;
	}
    }
  if (sprite)
    {
      visible = sprite->visible;
      sprite->visible = FALSE;
      gnome_forest_queue_vpath (forest, sprite->vpath);
      if (sprite->outline)
	art_free (sprite->outline);
      if (sprite->vpath)
	art_free (sprite->vpath);
      if (sprite->svp)
	art_svp_free (sprite->svp);
      art_pixbuf_free (sprite->pixbuf);
    }
  else
    {
      visible = TRUE;
      forest->n_sprites++;
      forest->sprites = g_renew (GnomeSprite, forest->sprites, forest->n_sprites);
      sprite = forest->sprites + forest->n_sprites - 1;
      sprite->id = id;
      sprite->visible = FALSE;
      sprite->can_collide = FALSE + 1;
      sprite->hflip = FALSE;
      sprite->vflip = FALSE;
      sprite->x = 0;
      sprite->y = 0;
      sprite->width = image->width;
      sprite->height = image->height;
      sprite->rotate = 0;
      sprite->shear = 0;
      art_affine_identity (sprite->affine);
    }
  sprite->outline = NULL;
  sprite->vpath = NULL;
  sprite->svp = NULL;
  sprite->pixbuf = image;
  gnome_sprite_check_update (forest, sprite); /* update matrix */
  if (visible)
    {
      sprite->visible = TRUE;
      gnome_forest_queue_update (forest);
    }
  
  return sprite->id;
}

void
gnome_forest_show_sprite (GnomeForest *forest,
			  guint        id)
{
  GnomeSprite *sprite;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  sprite = gnome_forest_peek_sprite (forest, id);
  g_return_if_fail (sprite != NULL);
  
  if (sprite && !sprite->visible)
    {
      sprite->visible = TRUE;
      gnome_forest_queue_update (forest);
    }
}

void
gnome_forest_hide_sprite (GnomeForest *forest,
			  guint      id)
{
  GnomeSprite *sprite;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  sprite = gnome_forest_peek_sprite (forest, id);
  g_return_if_fail (sprite != NULL);
  
  if (sprite && sprite->visible)
    {
      sprite->visible = FALSE;
      gnome_forest_queue_vpath (forest, sprite->vpath);
    }
}

void
gnome_forest_set_sprite_pos (GnomeForest *forest,
			     guint        id,
			     gint         x,
			     gint         y)
{
  GnomeSprite *sprite;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  sprite = gnome_forest_peek_sprite (forest, id);
  g_return_if_fail (sprite != NULL);
  
  if (sprite)
    {
      sprite->x = x;
      sprite->y = y;
      gnome_sprite_check_update (forest, sprite);
    }
}

void
gnome_forest_move_sprite (GnomeForest *forest,
			  guint        id,
			  gint         hdelta,
			  gint         vdelta)
{
  GnomeSprite *sprite;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  sprite = gnome_forest_peek_sprite (forest, id);
  g_return_if_fail (sprite != NULL);
  
  if (sprite)
    {
      sprite->x += hdelta;
      sprite->y += vdelta;
      gnome_sprite_check_update (forest, sprite);
    }
}

void
gnome_forest_set_sprite_size (GnomeForest *forest,
			      guint        id,
			      guint        width,
			      guint        height)
{
  GnomeSprite *sprite;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  sprite = gnome_forest_peek_sprite (forest, id);
  g_return_if_fail (sprite != NULL);
  
  if (sprite)
    {
      sprite->width = width;
      sprite->height = height;
      gnome_sprite_check_update (forest, sprite);
    }
}

void
gnome_forest_set_sprite_rot (GnomeForest *forest,
			     guint        id,
			     gfloat       angle)
{
  GnomeSprite *sprite;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  sprite = gnome_forest_peek_sprite (forest, id);
  g_return_if_fail (sprite != NULL);
  
  if (sprite)
    {
      sprite->rotate = angle;
      gnome_sprite_check_update (forest, sprite);
    }
}

void
gnome_forest_set_sprite_shear (GnomeForest *forest,
			       guint        id,
			       gfloat       angle)
{
  GnomeSprite *sprite;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  sprite = gnome_forest_peek_sprite (forest, id);
  g_return_if_fail (sprite != NULL);
  
  if (sprite)
    {
      sprite->shear = angle;
      gnome_sprite_check_update (forest, sprite);
    }
}

void
gnome_forest_set_sprite_hflip (GnomeForest *forest,
			       guint        id,
			       gboolean     hflip)
{
  GnomeSprite *sprite;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  sprite = gnome_forest_peek_sprite (forest, id);
  g_return_if_fail (sprite != NULL);
  
  if (sprite)
    {
      sprite->hflip = hflip != FALSE;
      gnome_sprite_check_update (forest, sprite);
    }
}

void
gnome_forest_set_sprite_vflip (GnomeForest *forest,
			       guint        id,
			       gboolean     vflip)
{
  GnomeSprite *sprite;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  sprite = gnome_forest_peek_sprite (forest, id);
  g_return_if_fail (sprite != NULL);
  
  if (sprite)
    {
      sprite->vflip = vflip != FALSE;
      gnome_sprite_check_update (forest, sprite);
    }
}

static gboolean
gnome_sprite_check_update (GnomeForest *forest,
			   GnomeSprite *sprite)
{
  GtkAllocation *allocation = &GTK_WIDGET (forest)->allocation;
  gdouble affine[6], transform[6], sx, sy;
  
  /* identity transformation */
  art_affine_identity (affine);
  
  /* scale to fixed size or to fit allocation */
  if (sprite->width > 0)
    sx = ((gdouble) sprite->width) / ((gdouble) sprite->pixbuf->width);
  else
    sx = ((gdouble) allocation->width) / ((gdouble) sprite->pixbuf->width);
  if (sprite->height > 0)
    sy = ((gdouble) sprite->height) / ((gdouble) sprite->pixbuf->height);
  else
    sy = ((gdouble) allocation->height) / ((gdouble) sprite->pixbuf->height);
  if (sx != 1 || sy != 1)
    {
      art_affine_scale (transform, sx, sy);
      art_affine_multiply (affine, affine, transform);
    }
  
  if (fabs (sprite->rotate) > EPSILON ||
      fabs (sprite->shear) > EPSILON ||
      sprite->hflip || sprite->vflip)
    {
      gdouble mid_x = sprite->pixbuf->width;
      gdouble mid_y = sprite->pixbuf->height;
      
      /* figure center, adjusted to previous scaling */
      mid_x /= 2.0;
      mid_y /= 2.0;
      mid_x *= sx;
      mid_y *= sy;
      
      /* offset into center */
      art_affine_translate (transform, -mid_x, -mid_y);
      art_affine_multiply (affine, affine, transform);
      
      /* rotate */
      if (fabs (sprite->rotate) > EPSILON)
	{
	  art_affine_rotate (transform, sprite->rotate);
	  art_affine_multiply (affine, affine, transform);
	}
      
      /* shear */
      if (fabs (sprite->shear) > EPSILON)
	{
	  art_affine_shear (transform, sprite->shear);
	  art_affine_multiply (affine, affine, transform);
	}
      
      /* flip */
      if (sprite->hflip || sprite->vflip)
	art_affine_flip (affine, affine, sprite->hflip, sprite->vflip);
      
      /* offset to origin */
      art_affine_translate (transform, mid_x, mid_y);
      art_affine_multiply (affine, affine, transform);
    }
  
  /* translate to offset */
  if (fabs (sprite->x) > EPSILON ||
      fabs (sprite->y) > EPSILON)
    {
      art_affine_translate (transform, sprite->x, sprite->y);
      art_affine_multiply (affine, affine, transform);
    }
  
  /* update where neccessary */
  if (!art_affine_equal (sprite->affine, affine))
    {
      if (sprite->visible)
	gnome_forest_queue_vpath (forest, sprite->vpath);
      
      /* set sprite matrix, invalidate vpath */
      art_affine_flip (sprite->affine, affine, FALSE, FALSE);
      
      if (sprite->vpath)
	{
	  art_free (sprite->vpath);
	  sprite->vpath = NULL;
	}
      if (sprite->svp)
	{
	  art_svp_free (sprite->svp);
	  sprite->svp = NULL;
	}
      
      return TRUE;
    }
  
  return FALSE;
}

static gboolean
sprite_ensure_vpath (GnomeForest *forest,
		     GnomeSprite *sprite)
{
  gboolean vpath_altered = FALSE;
  
  if (!sprite->outline)
    sprite->outline = art_vpath_outline_from_pixbuf (sprite->pixbuf, TRUE);
  
  if (!sprite->vpath && sprite->outline)
    {
      sprite->vpath = art_vpath_affine_transform (sprite->outline, sprite->affine);
      vpath_altered = TRUE;
    }
  
  if (!sprite->vpath)
    {
      ArtPixBuf *image = sprite->pixbuf;
      gdouble *matrix = sprite->affine;
      
      sprite->vpath = art_new (ArtVpath, 6);
      sprite->vpath[0].code = ART_MOVETO;
      sprite->vpath[0].x = matrix[4];
      sprite->vpath[0].y = matrix[5];
      sprite->vpath[1].code = ART_LINETO;
      sprite->vpath[1].x = matrix[2] * image->height + matrix[4];
      sprite->vpath[1].y = matrix[3] * image->height + matrix[5];
      sprite->vpath[2].code = ART_LINETO;
      sprite->vpath[2].x = matrix[0] * image->width + matrix[2] * image->height + matrix[4];
      sprite->vpath[2].y = matrix[1] * image->width + matrix[3] * image->height + matrix[5];
      sprite->vpath[3].code = ART_LINETO;
      sprite->vpath[3].x = matrix[0] * image->width + matrix[4];
      sprite->vpath[3].y = matrix[1] * image->width + matrix[5];
      sprite->vpath[4].code = ART_LINETO;
      sprite->vpath[4].x = matrix[4];
      sprite->vpath[4].y = matrix[5];
      sprite->vpath[5].code = ART_END;
      
      vpath_altered = TRUE;
    }
  
  if (vpath_altered && sprite->visible)
    gnome_forest_queue_vpath (forest, sprite->vpath);
  
  return vpath_altered;
}

static void
sprite_ensure_svp (GnomeForest *forest,
		   GnomeSprite *sprite)
{
  if (!sprite->svp)
    sprite->svp = art_svp_from_vpath (sprite->vpath);
}

static void
gnome_forest_render (GnomeForest *forest,
		     GdkColor    *bg_color)
{
  GtkWidget *widget = GTK_WIDGET (forest);
  GtkAllocation *allocation = &widget->allocation;
  ArtIRect arect = { 0, 0, allocation->width, allocation->height };
  
  if (forest->render_uta)
    {
      ArtIRect *rects, *irect, *irect_end;
      guint n_rects;
      guint8 *b, *e;
      
      /* rectangle list of render areas */
      rects = art_rect_list_from_uta (forest->render_uta,
				      allocation->width,
				      allocation->height,
				      &n_rects);
      if (forest->paint_uta)
	{
          ArtUta *uta2;
	  
	  uta2 = art_uta_union (forest->paint_uta, forest->render_uta);
	  art_uta_free (forest->render_uta);
	  art_uta_free (forest->paint_uta);
	  forest->paint_uta = uta2;
	}
      else
	forest->paint_uta = forest->render_uta;
      forest->render_uta = NULL;
      
      /* render rectangles */
      irect_end = rects + n_rects;
      for (irect = rects; irect < irect_end; irect++)
	{
	  gint width, height;
	  guint i, rowstride = allocation->width * 3;
	  
	  /* sanity clip rectangle against allocation */
	  art_irect_intersect (irect, irect, &arect);
	  width = irect->x1 - irect->x0;
	  height = irect->y1 - irect->y0;
	  
	  /* render only sane rectangles */
	  if (width < 1 || height < 1)
	    continue;
	  
	  /* paint buffer with background color */
	  b = forest->buffer + irect->y0 * rowstride + irect->x0 * 3;
	  for (e = b + height * rowstride; b < e; b += rowstride)
	    art_rgb_fill_run (b,
			      bg_color->red >> 8,
			      bg_color->green >> 8,
			      bg_color->blue >> 8,
			      width);
	  
	  /* render sprites to buffer */
	  for (i = 0; i < forest->n_sprites; i++)
	    {
	      GnomeSprite *sprite = forest->sprites + i;
	      
	      if (sprite->visible)
		art_rgb_pixbuf_affine (forest->buffer + irect->y0 * rowstride + irect->x0 * 3,
				       irect->x0, irect->y0, irect->x1, irect->y1,
				       rowstride,
				       sprite->pixbuf,
				       sprite->affine,
				       ART_FILTER_NEAREST,
				       NULL);
	      if (forest->shade_svps && sprite->visible)
		{
		  sprite_ensure_svp (forest, sprite);
		  art_rgb_svp_alpha (sprite->svp,
				     irect->x0, irect->y0, irect->x1, irect->y1,
				     0xffffff80,
				     forest->buffer + irect->y0 * rowstride + irect->x0 * 3,
				     rowstride,
				     NULL);
		}
	    }
	}
      if (n_rects)
	art_free (rects);
    }
}

static void
gnome_forest_paint (GnomeForest *forest,
		    GdkColor    *bg_color)
{
  GtkWidget *widget = GTK_WIDGET (forest);
  GtkAllocation *allocation = &widget->allocation;
  ArtIRect arect = { 0, 0, allocation->width, allocation->height };
  ArtIRect *rects, *irect, *irect_end;
  guint n_rects;

  /* assume rendering finished */
  g_return_if_fail (forest->render_uta == NULL);
  
  /* rectangle list of redraw areas */
  rects = art_rect_list_from_uta (forest->paint_uta,
				  allocation->width,
				  allocation->height,
				  &n_rects);
  art_uta_free (forest->paint_uta);
  forest->paint_uta = NULL;
  
  /* repaint rectangles */
  irect_end = rects + n_rects;
  for (irect = rects; irect < irect_end; irect++)
    {
      gint width, height;
      guint rowstride = widget->allocation.width * 3;
      
      /* sanity clip rectangle against allocation */
      art_irect_intersect (irect, irect, &arect);
      width = irect->x1 - irect->x0;
      height = irect->y1 - irect->y0;
      
      /* paint only sane rectangles */
      if (width < 1 || height < 1)
	continue;
      
      /* draw buffer */
      gdk_draw_rgb_image (widget->window,
			  widget->style->black_gc,
			  irect->x0, irect->y0,
			  width, height,
			  GDK_RGB_DITHER_NONE,
			  forest->buffer + irect->y0 * rowstride + irect->x0 * 3,
			  rowstride);
      if (forest->show_utas)
	{
	  gdk_draw_line (widget->window, widget->style->white_gc,
			 irect->x0, irect->y0, irect->x1 - 1, irect->y0);
	  gdk_draw_line (widget->window, widget->style->white_gc,
			 irect->x1 - 1, irect->y0, irect->x1 - 1, irect->y1 - 1);
	  gdk_draw_line (widget->window, widget->style->white_gc,
			 irect->x1 - 1, irect->y1 - 1, irect->x0, irect->y1 - 1);
	  gdk_draw_line (widget->window, widget->style->white_gc,
			 irect->x0, irect->y1 - 1, irect->x0, irect->y0);
	  gdk_draw_line (widget->window,
			 widget->style->black_gc,
			 irect->x0, irect->y0,
			 irect->x1 - 1, irect->y1 - 1);
	  gdk_draw_line (widget->window,
			 widget->style->black_gc,
			 irect->x1, irect->y0,
			 irect->x0 - 1, irect->y1 - 1);
	}
    }
  
  if (n_rects)
    art_free (rects);
}

static gboolean
forest_idle_update (gpointer func_data)
{
  GnomeForest *forest;

  GDK_THREADS_ENTER ();

  forest = GNOME_FOREST (func_data);

  forest->update_queued = 0;
  
  if (GTK_WIDGET_VISIBLE (forest) && forest->buffer && !GTK_OBJECT_DESTROYED (forest))
    {
      gboolean need_cd = FALSE;
      
      do
	{
	  guint i;
	  
	  /* handle modified sprites (finish sprite update) */
	  for (i = 0; i < forest->n_sprites; i++)
	    {
	      GnomeSprite *sprite = forest->sprites + i;
	      
	      if (sprite->visible)
		need_cd |= sprite_ensure_vpath (forest, sprite) && sprite->can_collide;
	    }
	  
	  if (need_cd)
	    need_cd = gnome_forest_collisions (forest);
	}
      while (need_cd && GTK_WIDGET_VISIBLE (forest));
      
      if (GTK_WIDGET_VISIBLE (forest))
	{
	  GtkWidget *widget = GTK_WIDGET (forest);
	  GtkStateType state;
	  
	  state = GTK_WIDGET_STATE (widget);
	  if (forest->render_uta)
	    gnome_forest_render (forest, &widget->style->bg[state]);
	  if (GTK_WIDGET_DRAWABLE (forest) && forest->paint_uta)
	    gnome_forest_paint (forest, &widget->style->bg[state]);
	}
      /*FIXME*/ gnome_forest_collisions (forest);
    }
  GDK_THREADS_LEAVE ();

  return FALSE;
}

static void
gnome_forest_queue_update (GnomeForest *forest)
{
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  if (!forest->update_queued && GTK_WIDGET_VISIBLE (forest))
    {
      gtk_object_ref (GTK_OBJECT (forest));
      forest->update_queued = g_idle_add_full (GNOME_FOREST_PRIORITY - 1,
					       forest_idle_update,
					       forest,
					       (GDestroyNotify) gtk_object_unref);
    }
}

void
gnome_forest_render_now (GnomeForest *forest)
{
  GtkWidget *widget;

  g_return_if_fail (GNOME_IS_FOREST (forest));

  widget = GTK_WIDGET (forest);

  if (forest->update_queued)
    {
      g_source_remove (forest->update_queued);
      forest->update_queued = 0;

      GDK_THREADS_LEAVE ();
      forest_idle_update (forest);
      GDK_THREADS_ENTER ();
    }
}

guint8*
gnome_forest_bitmap_data (GnomeForest *forest,
			  gint        *width_p,
			  gint        *height_p)
{
  GtkWidget *widget;
  guint8 *data;
  gint x, y, width, height, red, green, blue, rowstride;

  g_return_val_if_fail (GNOME_IS_FOREST (forest), NULL);
  g_return_val_if_fail (width_p != NULL, NULL);
  g_return_val_if_fail (height_p != NULL, NULL);
  g_return_val_if_fail (GTK_WIDGET_VISIBLE (forest), NULL);

  widget = GTK_WIDGET (forest);

  gtk_widget_ensure_style (widget);

  /* force size allocation, only
   * necessary for unrealized forest
   */
  if (!forest->buffer)
    {
      GtkRequisition requisition = { 0, 0, };
      GtkAllocation allocation = { 0, 0, 0, 0, };
      
      gtk_widget_size_request (widget, &requisition);
      allocation.width = requisition.width;
      allocation.height = requisition.height;
      gtk_widget_size_allocate (widget, &allocation);
    }

  gnome_forest_render_now (forest);

  width = widget->allocation.width;
  rowstride = widget->allocation.width * 3;
  height = widget->allocation.height;
  red = widget->style->bg[GTK_WIDGET_STATE (widget)].red >> 8;
  green = widget->style->bg[GTK_WIDGET_STATE (widget)].green >> 8;
  blue = widget->style->bg[GTK_WIDGET_STATE (widget)].blue >> 8;

  data = g_new0 (guint8, (width + 7) / 8 * height);
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      {
	guint8 *d = data + (width + 7) / 8 * y + x / 8;
	guint8 *buf = forest->buffer + y * rowstride + x * 3;

	if (buf[0] != red || buf[1] != green || buf[2] != blue)
	  *d |= 1 << (x % 8);
      }

  *width_p = width;
  *height_p = height;

  return data;
}

static void
gnome_forest_queue_vpath (GnomeForest    *forest,
			  const ArtVpath *vpath)
{
  if (vpath && vpath[0].code == ART_MOVETO)
    {
      ArtUta *uta;
      
      uta = art_uta_from_vpath (vpath);
      if (forest->render_uta)
	{
	  ArtUta *uta2;
	  
	  uta2 = art_uta_union (forest->render_uta, uta);
	  art_uta_free (uta);
	  art_uta_free (forest->render_uta);
	  forest->render_uta = uta2;
	}
      else
	forest->render_uta = uta;
      
      gnome_forest_queue_update (forest);
    }
}

static void
gnome_forest_queue_area (GnomeForest *forest,
			 gboolean     rerender,
			 gint         x,
			 gint         y,
			 gint         width,
			 gint         height)
{
  if (width > 0 && height > 0)
    {
      ArtUta *uta, **uta_p = rerender ? &forest->render_uta : &forest->paint_uta;
      ArtIRect irect;
      
      irect.x0 = x;
      irect.y0 = y;
      irect.x1 = x + width;
      irect.y1 = y + height;
      
      uta = art_uta_from_irect (&irect);
      if (*uta_p)
	{
	  ArtUta *uta2;
	  
	  uta2 = art_uta_union (*uta_p, uta);
	  art_uta_free (uta);
	  art_uta_free (*uta_p);
	  *uta_p = uta2;
	}
      else
	*uta_p = uta;
      
      gnome_forest_queue_update (forest);
    }
}

static gboolean
gnome_forest_collisions (GnomeForest *forest)
{
  guint v;
  
  /* all visible sprites have a vpath */
  
  for (v = 0; v < forest->n_sprites; v++)
    {
      GnomeSprite *sprite = forest->sprites + v;
      guint u;
      
      if (!sprite->can_collide || !sprite->visible)
	continue;
      if (forest->disable_cd)
	continue;
      
      sprite_ensure_svp (forest, sprite);
      for (u = v + 1; u < forest->n_sprites; u++)
	{
	  GnomeSprite *collidor = forest->sprites + u;
	  ArtVpath *vpath;
	  ArtSVP *svp;
	  guint i;
	  
	  if (!collidor->can_collide || !collidor->visible)
	    continue;
	  
	  sprite_ensure_svp (forest, collidor);
	  
	  svp = art_svp_intersect (sprite->svp, collidor->svp);
	  vpath = art_vpath_from_svp (svp);
	  art_svp_free (svp);

	  if (forest->debug_cd)
	    {
	      for (i = 0; vpath[i].code != ART_END; i++)
		gdk_draw_arc (GTK_WIDGET (forest)->window,
			      GTK_WIDGET (forest)->style->white_gc,
			      FALSE,
			      vpath[i].x, vpath[i].y,
			      10, 10,
			      0, 360);
	      g_print ("%d<->%d: %d points, area %f\n",
		       sprite->id, collidor->id, i, art_vpath_area (vpath));
	    }
	  
	  art_free (vpath);
	}
    }
  
  return FALSE;
}


/* --- sprite animation --- */
typedef struct _AnimData AnimData;
typedef struct _AnimCtrl AnimCtrl;
struct _AnimData
{
  AnimData *next;
  
  guint	    	id;
  guint		delay;
  guint		n_steps;
  
  guint		absolute_move : 1;
  guint		relative_move : 1;
  guint		absolute_size : 1;
  guint		relative_size : 1;
  guint		absolute_rotation : 1;
  guint		relative_rotation : 1;
  guint		absolute_shear : 1;
  guint		relative_shear : 1;
  
  gdouble	x, y, width, height, rotation, shear;
};
struct _AnimCtrl
{
  guint     sprite_id;
  AnimData *adata;
  guint     n_adatas : 28;
  guint     loop : 1;
  gint      nth_adata;
  guint     next_step;
  guint     handler_id;
};

static void
animctrl_destroy (gpointer data)
{
  AnimCtrl *actrl = data;
  AnimData *adata = actrl->adata;
  
  while (adata)
    {
      AnimData *tmp = adata->next;
      
      g_free (adata);
      adata = tmp;
    }
  if (actrl->handler_id)
    {
      g_source_remove (actrl->handler_id);
      actrl->handler_id = 0;
    }
  g_free (actrl);
}

static AnimCtrl*
sprite_animctrl (GnomeForest *forest,
		 guint        sprite_id,
		 gboolean     force_actrl)
{
  AnimCtrl *actrl = g_datalist_id_get_data (&forest->animdata, sprite_id);
  
  if (!actrl && force_actrl)
    {
      actrl = g_new (AnimCtrl, 1);
      actrl->sprite_id = sprite_id;
      actrl->adata = NULL;
      actrl->n_adatas = 0;
      actrl->loop = FALSE;
      actrl->nth_adata = -1;
      actrl->next_step = 1;
      actrl->handler_id = 0;
      g_datalist_id_set_data_full (&forest->animdata,
				   sprite_id,
				   actrl,
				   animctrl_destroy);
    }
  
  return actrl;
}

static inline AnimData*
nth_animdata (AnimCtrl *actrl,
	      gint      n)
{
  if (n >= 0)
    {
      AnimData *adata = actrl->adata;
      
      while (n-- > 0 && adata)
	adata = adata->next;
      
      return adata;
    }
  
  return NULL;
}

static AnimData*
new_animdata (AnimCtrl *actrl)
{
  AnimData adata_tmpl = {
    NULL,
    0, 0, 0,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
  };
  AnimData *adata = g_memdup (&adata_tmpl, sizeof (AnimData));
  
  adata->id = ++actrl->n_adatas;
  
  if (actrl->adata)
    {
      AnimData *last = actrl->adata;
      
      while (last->next)
	last = last->next;
      last->next = adata;
    }
  else
    actrl->adata = adata;
  
  return adata;
}

static void
animdata_step (GnomeForest *forest,
	       AnimData    *adata,
	       GnomeSprite *sprite,
	       gdouble      a_step_f,
	       gdouble      r_step_f)
{
  gdouble dx = 0, dy = 0, dwidth = 0, dheight = 0, drotation = 0, dshear = 0;
  
  if (adata->absolute_move)
    {
      dx = adata->x - sprite->x;
      dy = adata->y - sprite->y;
      dx *= a_step_f;
      dy *= a_step_f;
    }
  if (adata->relative_move)
    {
      dx = adata->x * r_step_f;
      dy = adata->y * r_step_f;
    }
  if (adata->absolute_size)
    {
      dwidth = adata->width < 0 ? sprite->pixbuf->width : adata->width;
      dheight = adata->height < 0 ? sprite->pixbuf->height : adata->height;
      dwidth -= sprite->width;
      dheight -= sprite->height;
      dwidth *= a_step_f;
      dheight *= a_step_f;
    }
  if (adata->relative_size)
    {
      dwidth = adata->width * r_step_f;
      dheight = adata->height * r_step_f;
    }
  if (adata->absolute_rotation)
    {
      drotation = adata->rotation - sprite->rotate;
      drotation *= a_step_f;
    }
  if (adata->relative_rotation)
    drotation = adata->rotation * r_step_f;
  if (adata->absolute_shear)
    {
      dshear = adata->shear - sprite->shear;
      dshear *= a_step_f;
    }
  if (adata->relative_shear)
    dshear = adata->shear * r_step_f;
  
  sprite->x += dx;
  sprite->y += dy;
  sprite->width += dwidth;
  sprite->height += dheight;
  sprite->rotate += drotation;
  sprite->shear += dshear;
  
  gnome_sprite_check_update (forest, sprite);
}

static void
sa_destroy (gpointer data)
{
  gpointer *sa_data = data;
  
  gtk_object_unref (sa_data[0]);
  g_free (sa_data);
}

static gboolean sprite_animator (gpointer data);

static inline void
queue_animator (GnomeForest *forest,
		AnimCtrl    *actrl)
{
  g_return_if_fail (actrl->handler_id == 0);
  
  if (!GTK_OBJECT_DESTROYED (forest))
    {
      AnimData *adata = nth_animdata (actrl, actrl->nth_adata);
      
      if (adata && actrl->next_step > adata->n_steps)
	{
	  actrl->nth_adata++;
	  actrl->next_step = 1;
	  adata = nth_animdata (actrl, actrl->nth_adata);
	}
      if (!adata)
	{
	  actrl->nth_adata = actrl->n_adatas && actrl->loop ? 0 : -1;
	  actrl->next_step = 1;
	  adata = nth_animdata (actrl, actrl->nth_adata);
	}
      if (adata)
	{
	  gpointer sa_data[2] = { forest, actrl };
	  
	  gtk_object_ref (GTK_OBJECT (forest));
	  if (adata->delay)
	    actrl->handler_id = g_timeout_add_full (GNOME_FOREST_PRIORITY - 2,
						    adata->delay,
						    sprite_animator,
						    g_memdup (sa_data, sizeof (sa_data)),
						    sa_destroy);
	  else
	    actrl->handler_id = g_idle_add_full (GNOME_FOREST_PRIORITY - 2,
						 sprite_animator,
						 g_memdup (sa_data, sizeof (sa_data)),
						 sa_destroy);
	}
    }
}

static gboolean
sprite_animator (gpointer data)
{
  gpointer *sa_data = data;
  GnomeForest *forest;
  AnimCtrl *actrl;
  GnomeSprite *sprite;
  AnimData *adata;

  GDK_THREADS_ENTER ();

  forest = GNOME_FOREST (sa_data[0]);
  actrl = sa_data[1];
  sprite = gnome_forest_peek_sprite (forest, actrl->sprite_id);
  adata = nth_animdata (actrl, actrl->nth_adata);
  
  if (sprite && adata)
    {
      gdouble step_frac = actrl->next_step;
      
      step_frac /= adata->n_steps;
      actrl->next_step++;
      
      animdata_step (forest,
		     adata,
		     sprite,
		     step_frac,
		     ((gdouble) 1) / (gdouble) adata->n_steps);
      
      if (actrl->handler_id)
	{
	  actrl->handler_id = 0;
	  queue_animator (forest, actrl);
	}
    }

  GDK_THREADS_LEAVE ();
  
  return FALSE;
}

guint
gnome_forest_animate_sprite (GnomeForest        *forest,
			     guint               sprite_id,
			     guint               step_delay,
			     guint               n_steps,
			     GnomeSpriteAnimType a_type,
			     ...)
{
  GnomeSprite *sprite;
  AnimCtrl *actrl;
  AnimData *adata;
  va_list args;
  
  g_return_val_if_fail (GNOME_IS_FOREST (forest), 0);
  
  sprite = gnome_forest_peek_sprite (forest, sprite_id);
  g_return_val_if_fail (sprite != NULL, 0);
  
  n_steps = MAX (n_steps, 1);
  actrl = sprite_animctrl (forest, sprite_id, TRUE);
  adata = new_animdata (actrl);
  adata->delay = step_delay;
  adata->n_steps = n_steps;
  
  va_start (args, a_type);
  while (a_type != GNOME_SPRITE_DONE)
    {
      switch (a_type)
	{
	case GNOME_SPRITE_MOVE_TO:
	case GNOME_SPRITE_MOVE_BY:
	  adata->relative_move = a_type & 1;
	  adata->absolute_move = !adata->relative_move;
	  adata->x = va_arg (args, int);
	  adata->y = va_arg (args, int);
	  break;
	case GNOME_SPRITE_RESIZE_TO:
	case GNOME_SPRITE_RESIZE_BY:
	  adata->relative_size = a_type & 1;
	  adata->absolute_size = !adata->relative_size;
	  adata->width = va_arg (args, int);
	  adata->height = va_arg (args, int);
	  break;
	case GNOME_SPRITE_ROTATE_TO:
	case GNOME_SPRITE_ROTATE_BY:
	  adata->relative_rotation = a_type & 1;
	  adata->absolute_rotation = !adata->relative_rotation;
	  adata->rotation = va_arg (args, gdouble);
	  break;
	case GNOME_SPRITE_SHEAR_TO:
	case GNOME_SPRITE_SHEAR_BY:
	  adata->relative_shear = a_type & 1;
	  adata->absolute_shear = !adata->relative_shear;
	  adata->shear = va_arg (args, gdouble);
	  break;
	case GNOME_SPRITE_LOOP_ALWAYS:
	case GNOME_SPRITE_LOOP_NEVER:
	  actrl->loop = a_type == GNOME_SPRITE_LOOP_ALWAYS;
	  break;
	default:
	  g_warning (G_GNUC_PRETTY_FUNCTION "(): invalid GnomeSpriteAnimType (%d)", a_type);
	  a_type = GNOME_SPRITE_DONE;
	  break;
	}
      if (a_type != GNOME_SPRITE_DONE)
	a_type = va_arg (args, GnomeSpriteAnimType);
    }
  va_end (args);
  
  if (actrl->nth_adata < 0)
    gnome_forest_restart_sprite_animations (forest, sprite->id);
  
  return adata->id;
}

void
gnome_forest_restart_sprite_animations (GnomeForest *forest,
					guint        sprite_id)
{
  AnimCtrl *actrl;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  actrl = sprite_animctrl (forest, sprite_id, FALSE);
  if (actrl)
    {
      actrl->nth_adata = actrl->n_adatas ? 0 : -1;
      actrl->next_step = 1;
      if (actrl->handler_id)
	g_source_remove (actrl->handler_id);
      actrl->handler_id = 0;
      queue_animator (forest, actrl);
    }
}

void
gnome_forest_continue_sprite_animations (GnomeForest *forest,
					 guint        sprite_id)
{
  AnimCtrl *actrl;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  actrl = sprite_animctrl (forest, sprite_id, FALSE);
  if (actrl && !actrl->handler_id)
    queue_animator (forest, actrl);
}

void
gnome_forest_stop_sprite_animations (GnomeForest *forest,
				     guint        sprite_id)
{
  AnimCtrl *actrl;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  actrl = sprite_animctrl (forest, sprite_id, FALSE);
  if (actrl)
    {
      if (actrl->handler_id)
	g_source_remove (actrl->handler_id);
      actrl->handler_id = 0;
    }
}

void
gnome_forest_kill_sprite_animations (GnomeForest *forest,
				     guint        sprite_id)
{
  AnimCtrl *actrl;
  
  g_return_if_fail (GNOME_IS_FOREST (forest));
  
  actrl = sprite_animctrl (forest, sprite_id, FALSE);
  if (actrl)
    {
      if (actrl->handler_id)
	g_source_remove (actrl->handler_id);
      actrl->handler_id = 0;
      g_datalist_id_set_data (&forest->animdata, sprite_id, NULL);
    }
}
