/* BEAST - Bedevilled Audio System
 * Copyright (C) 2000-2001 Tim Janik
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
#ifndef __BST_QSAMPLER_H__
#define __BST_QSAMPLER_H__

#include        <gtk/gtk.h>


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define BST_TYPE_QSAMPLER            (bst_qsampler_get_type ())
#define BST_QSAMPLER(object)         (GTK_CHECK_CAST ((object), BST_TYPE_QSAMPLER, BstQSampler))
#define BST_QSAMPLER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_QSAMPLER, BstQSamplerClass))
#define BST_IS_QSAMPLER(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_QSAMPLER))
#define BST_IS_QSAMPLER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_QSAMPLER))
#define BST_QSAMPLER_GET_CLASS(obj)  ((BstQSamplerClass*) ((GtkObject*) (obj))->klass)

#define	BST_QSAMPLER_READ_PRIORITY   (G_PRIORITY_LOW)


/* --- typedefs --- */
typedef struct _BstQSampler       BstQSampler;
typedef struct _BstQSamplerClass  BstQSamplerClass;
typedef struct _BstQSamplerSource BstQSamplerSource;
typedef struct _BstQSamplerBlock  BstQSamplerBlock;
typedef struct _BstQSamplerMark	  BstQSamplerMark;
typedef struct _BstQSamplerRegion BstQSamplerRegion;
typedef void  (*BstQSamplerFill) (gpointer	 data,
				  guint		 voffset,
				  guint		 n_values,
				  gint16	*values,
				  BstQSampler	*qsampler);
typedef enum
{
  /* regions & marks */
  BST_QSAMPLER_ACTIVE		= 1 << 1,
  BST_QSAMPLER_SELECTED		= 1 << 2,
  /* marks only */
  BST_QSAMPLER_PRELIGHT		= 1 << 3,

  /*< private >*/
  BST_QSAMPLER_MARK_MASK	= (BST_QSAMPLER_SELECTED | BST_QSAMPLER_ACTIVE | BST_QSAMPLER_PRELIGHT),
  BST_QSAMPLER_REGION_MASK	= (BST_QSAMPLER_SELECTED | BST_QSAMPLER_ACTIVE),
  BST_QSAMPLER_MARK		= 1 << 4,
  BST_QSAMPLER_MASK		= 0x1f,
  BST_QSAMPLER_SKIP		= 1 << 5,
  BST_QSAMPLER_NEEDS_DRAW	= 1 << 6,
  BST_QSAMPLER_DIRTY		= 1 << 7
} BstQSamplerType;
typedef enum
{
  BST_QSAMPLER_DRAW_CRANGE,
  BST_QSAMPLER_DRAW_CSHAPE,
  BST_QSAMPLER_DRAW_ZERO_SHAPE,
  BST_QSAMPLER_DRAW_MINIMUM_LINE,
  BST_QSAMPLER_DRAW_MIDDLE_LINE,
  BST_QSAMPLER_DRAW_MAXIMUM_LINE,
  BST_QSAMPLER_DRAW_MINIMUM_SHAPE,
  BST_QSAMPLER_DRAW_MAXIMUM_SHAPE,
  BST_QSAMPLER_DRAW_MODE_LAST	/*< skip >*/
} BstQSamplerDrawMode;


/* --- structures --- */
struct _BstQSampler
{
  GtkWidget parent_instance;

  GtkAdjustment	    *adjustment;
  guint		     n_total_samples;
  guint		     sample_offset;
  guint		     n_area_samples; /* visible */
  gdouble	     zoom_factor;
  gdouble	     vscale_factor;
  gdouble	     offset2peak_factor;
  guint		     n_peaks;
  gint16	    *peaks, *mpeaks; /* max, min */
  guint8	    *peak_types;
  guint		     n_marks;
  BstQSamplerMark   *marks;
  guint		     n_regions;
  BstQSamplerRegion *regions;
  GdkColor	     red, green;
  GdkGC		    *red_gc, *green_gc;
  guint		     vread_handler;
  guint		     expose_handler;
  guint		     expose_frame : 1;
  guint		     draw_mode : 16;
  guint16	     join_vreads;
  /* source */
  BstQSamplerFill    src_filler;
  gpointer	     src_data;
  GDestroyNotify     src_destroy;
};
struct _BstQSamplerClass
{
  GtkWidgetClass parent_class;
};
struct _BstQSamplerMark
{
  guint			index;
  BstQSamplerType	type;
  guint			offset;
};
struct _BstQSamplerRegion
{
  guint			index;
  BstQSamplerType	type;
  guint			offset;
  guint			length;
};


/* --- prototypes --- */
GtkType    bst_qsampler_get_type	(void);
void	   bst_qsampler_set_source	(BstQSampler			*qsampler,
					 guint				 n_total_samples,
					 BstQSamplerFill		 fill_func,
					 gpointer			 data,
					 GDestroyNotify			 destroy);
void	   bst_qsampler_get_bounds	(BstQSampler			*qsampler,
					 gint				*first_offset,
					 gint				*last_offset);
gboolean   bst_qsampler_get_offset_at	(BstQSampler			*qsampler,
					 gint				*x_coord_p);
void	   bst_qsampler_scroll_show	(BstQSampler			*qsampler,
					 guint				 offset);
void	   bst_qsampler_scroll_to	(BstQSampler			*qsampler,
					 guint				 offset);
void	   bst_qsampler_set_mark	(BstQSampler			*qsampler,
					 guint				 mark_index,
					 guint				 offset,
					 BstQSamplerType		 type);
gint	   bst_qsampler_get_mark_offset	(BstQSampler			*qsampler,
					 guint				 mark_index);
void	   bst_qsampler_set_region	(BstQSampler			*qsampler,
					 guint				 region_index,
					 guint				 offset,
					 guint				 length,
					 BstQSamplerType		 type);
void	   bst_qsampler_set_zoom	(BstQSampler			*qsampler,
					 gdouble			 zoom);
void	   bst_qsampler_set_vscale	(BstQSampler			*qsampler,
					 gdouble			 vscale);
void	   bst_qsampler_set_draw_mode	(BstQSampler			*qsampler,
					 BstQSamplerDrawMode		 dmode);
void	   bst_qsampler_set_adjustment	(BstQSampler			*qsampler,
					 GtkAdjustment			*adjustment);
     

#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_QSAMPLER_H__ */
