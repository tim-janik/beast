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
#ifndef __ART_VPATHO_PIXBUF_H__
#define __ART_VPATHO_PIXBUF_H__

#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_pixbuf.h>

/* create a vpath outline from the objects contained in an RGBA pixbuf */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ArtTriplet ArtTriplet;
struct _ArtTriplet
{
  double ax, ay;
  double bx, by;
  double cx, cy;
};


/* feed only ART_PIX_RGB images with alpha into this function,
 * this will return NULL, for invalid or completely empty pixbufs
 */
ArtVpath* art_vpath_outline_from_pixbuf (const ArtPixBuf *pixbuf,
					 int              alpha_threshold);

int
art_affine_from_triplets (const ArtTriplet *src_triplet,
			  const ArtTriplet *transformed_triplet,
			  double matrix[6]);

double art_vpath_area (const ArtVpath *vpath);


#ifdef __cplusplus
}
#endif

#endif /* __ART_VPATHO_PIXBUF_H__ */
















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
#ifndef __GNOME_FOREST_H__
#define __GNOME_FOREST_H__

#include	<libart_lgpl/libart.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	GNOME_TYPE_FOREST		(gnome_forest_get_type ())
#define	GNOME_FOREST(object)		(GTK_CHECK_CAST ((object), GNOME_TYPE_FOREST, GnomeForest))
#define	GNOME_FOREST_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_FOREST, GnomeForestClass))
#define	GNOME_IS_FOREST(object)		(GTK_CHECK_TYPE ((object), GNOME_TYPE_FOREST))
#define	GNOME_IS_FOREST_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_FOREST))
#define GNOME_FOREST_GET_CLASS(obj)	(GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_FOREST, GnomeForestClass))

#define GNOME_FOREST_PRIORITY	(GTK_PRIORITY_REDRAW)


/* --- structures & typedefs --- */
typedef	struct	_GnomeSprite		GnomeSprite;
typedef	struct	_GnomeSpriteCollision	GnomeSpriteCollision;
typedef	struct	_GnomeForest		GnomeForest;
typedef	struct	_GnomeForestClass	GnomeForestClass;
struct _GnomeSprite
{
  guint		 id;
  guint		 visible : 1, can_collide : 1;
  guint		 hflip : 1, vflip : 1;
  gfloat	 x, y;
  gfloat	 width, height;
  gfloat	 rotate, shear;
  gdouble	 affine[6];
  ArtVpath	*outline;
  ArtVpath	*vpath;
  ArtSVP	*svp;
  ArtPixBuf	*pixbuf;
};
struct _GnomeSpriteCollision
{
  guint		sprite1;
  GtkAllocation	area1;
  guint		sprite2;
  GtkAllocation	area2;
};
struct _GnomeForest
{
  GtkWidget	 parent_object;

  /* debug flags, FIXME: remove */
  guint shade_svps:1, show_utas:1, disable_cd:1, debug_cd:1;

  guint		 n_sprites;
  GnomeSprite	*sprites;

  guint		 update_queued;

  guint		 expand_forest : 1;

  guint		 buffer_size : 28;
  guint8	*buffer;
  ArtUta	*render_uta;
  ArtUta	*paint_uta;

  GData		*animdata;
};
struct _GnomeForestClass
{
  GtkWidgetClass parent_class;

  void  (*collision)	(GnomeForest	      *forest,
			 guint		       n_collisions,
			 GnomeSpriteCollision *collisions);
};


/* --- prototypes --- */
GtkType		gnome_forest_get_type		(void);
GtkWidget*	gnome_forest_new		(void);
void		gnome_forest_rerender		(GnomeForest	  *forest);
void		gnome_forest_render_now		(GnomeForest	  *forest);
guint		gnome_forest_put_sprite		(GnomeForest	  *forest,
						 guint		   id,
						 ArtPixBuf	  *image);
void		gnome_forest_show_sprite	(GnomeForest	  *forest,
						 guint             id);
void		gnome_forest_hide_sprite	(GnomeForest	  *forest,
						 guint             id);
void		gnome_forest_set_sprite_pos	(GnomeForest	  *forest,
						 guint             id,
						 gint              x,
						 gint              y);
void		gnome_forest_move_sprite	(GnomeForest	  *forest,
						 guint             id,
						 gint              hdelta,
						 gint              vdelta);
void		gnome_forest_set_sprite_size	(GnomeForest	  *forest,
						 guint             id,
						 guint             width,
						 guint             height);
void		gnome_forest_set_sprite_rot	(GnomeForest	  *forest,
						 guint             id,
						 gfloat		   angle);
void		gnome_forest_set_sprite_shear	(GnomeForest	  *forest,
						 guint             id,
						 gfloat		   angle);
void		gnome_forest_set_sprite_hflip	(GnomeForest	  *forest,
						 guint             id,
						 gboolean	   hflip);
void		gnome_forest_set_sprite_vflip	(GnomeForest	  *forest,
						 guint             id,
						 gboolean	   vflip);
/* caution: this pointer is short lived */
GnomeSprite*	gnome_forest_peek_sprite	(GnomeForest	  *forest,
						 guint		   id);
guint8*		gnome_forest_bitmap_data	(GnomeForest	  *forest,
						 gint             *width,
						 gint             *height);


/* --- sprite animation --- */
typedef enum
{
  GNOME_SPRITE_DONE,		/* NONE, terminator */
  GNOME_SPRITE_REPEAT_FROM,	/* INT: anim id */
  GNOME_SPRITE_LOOP_ALWAYS,	/* NONE */
  GNOME_SPRITE_LOOP_NEVER,	/* NONE */
  GNOME_SPRITE_MOVE_TO	= 16,	/* INT: x, y */
  GNOME_SPRITE_MOVE_BY,		/* INT: x, y */
  GNOME_SPRITE_RESIZE_TO,	/* INT: width, height */
  GNOME_SPRITE_RESIZE_BY,	/* INT: width, height */
  GNOME_SPRITE_ROTATE_TO,	/* DOUBLE: angle */
  GNOME_SPRITE_ROTATE_BY,	/* DOUBLE: angle */
  GNOME_SPRITE_SHEAR_TO,	/* DOUBLE: angle */
  GNOME_SPRITE_SHEAR_BY,	/* DOUBLE: angle */
} GnomeSpriteAnimType;

guint 	gnome_forest_animate_sprite		(GnomeForest	  *forest,
						 guint             sprite_id,
						 guint		   step_delay,
						 guint		   n_steps,
						 GnomeSpriteAnimType a_type,
						 ...);
void	gnome_forest_stop_sprite_animations	(GnomeForest	  *forest,
						 guint		   sprite_id);
void	gnome_forest_continue_sprite_animations	(GnomeForest	  *forest,
						 guint             sprite_id);
void	gnome_forest_restart_sprite_animations	(GnomeForest	  *forest,
						 guint             sprite_id);
void	gnome_forest_kill_sprite_animations	(GnomeForest	  *forest,
						 guint             sprite_id);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GNOME_FOREST_H__ */
