// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_QSAMPLER_H__
#define __BST_QSAMPLER_H__

#include        "bstutils.hh"

/* --- Gtk+ type macros --- */
#define BST_TYPE_QSAMPLER            (bst_qsampler_get_type ())
#define BST_QSAMPLER(object)         (GTK_CHECK_CAST ((object), BST_TYPE_QSAMPLER, BstQSampler))
#define BST_QSAMPLER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_QSAMPLER, BstQSamplerClass))
#define BST_IS_QSAMPLER(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_QSAMPLER))
#define BST_IS_QSAMPLER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_QSAMPLER))
#define BST_QSAMPLER_GET_CLASS(obj)  ((BstQSamplerClass*) ((GtkObject*) (obj))->klass)



/* --- typedefs --- */
typedef struct _BstQSampler       BstQSampler;
typedef struct _BstQSamplerClass  BstQSamplerClass;
typedef struct _BstQSamplerPeak	  BstQSamplerPeak;
typedef struct _BstQSamplerTPeak  BstQSamplerTPeak;
typedef struct _BstQSamplerSource BstQSamplerSource;
typedef struct _BstQSamplerBlock  BstQSamplerBlock;
typedef struct _BstQSamplerMark	  BstQSamplerMark;
typedef struct _BstQSamplerRegion BstQSamplerRegion;
typedef guint (*BstQSamplerFill) (gpointer	   data,
				  guint		   voffset,
				  gdouble	   offset_scale,
				  guint		   block_size,
				  guint		   n_values,
				  BstQSamplerPeak *values,
				  BstQSampler	  *qsampler);
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
#ifdef __cplusplus
constexpr BstQSamplerType  operator&  (BstQSamplerType  s1, BstQSamplerType s2) { return BstQSamplerType (s1 & (long long unsigned) s2); }
inline    BstQSamplerType& operator&= (BstQSamplerType &s1, BstQSamplerType s2) { s1 = s1 & s2; return s1; }
constexpr BstQSamplerType  operator|  (BstQSamplerType  s1, BstQSamplerType s2) { return BstQSamplerType (s1 | (long long unsigned) s2); }
inline    BstQSamplerType& operator|= (BstQSamplerType &s1, BstQSamplerType s2) { s1 = s1 | s2; return s1; }
constexpr BstQSamplerType  operator~  (BstQSamplerType  s1)                 { return BstQSamplerType (~(long long unsigned) s1); }
#endif // __cplusplus

#define	BST_QSAMPLER_RELOAD_PRIORITY	(GTK_PRIORITY_REDRAW + 5)


/* --- structures --- */
struct _BstQSampler
{
  GtkWidget parent_instance;

  guint		     peak_length;	/* pcm length in peaks */
  guint		     n_peaks;		/* number of cached peaks */
  BstQSamplerTPeak  *peaks;
  guint		     peak_offset;	/* display offset */
  guint		     n_pixels;		/* <= n_peaks */


  /* user settings */
  guint		     n_marks;
  BstQSamplerMark   *marks;
  guint		     n_regions;
  BstQSamplerRegion *regions;
  guint		     pcm_length;
  BstQSamplerFill    src_filler;
  gpointer	     src_data;
  GDestroyNotify     src_destroy;
  gdouble	     zoom_factor;

  GtkAdjustment	    *adjustment;
  gdouble	     vscale_factor;

  GdkColor	     red, green;
  GdkGC		    *red_gc, *green_gc;
  GdkWindow	    *canvas;
  Bst::QSamplerDrawMode draw_mode;
  guint		     expose_frame : 1;
  guint		     ignore_adjustment : 1;
  guint		     refresh_queued : 1;
  guint		     invalid_remains : 1; /* temporary refresh flag */

  /* user data */
  gpointer	     owner;
  guint		     owner_index;
};
struct _BstQSamplerTPeak
{
  gint16 min, max;
  guint8 type;
};
struct _BstQSamplerPeak
{
  gint16 min, max;
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
GType	   bst_qsampler_get_type	(void);
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
void	   bst_qsampler_scroll_rbounded	(BstQSampler			*qsampler,
					 guint				 offset,
					 gfloat				 boundary_padding,
					 gfloat				 padding);
void	   bst_qsampler_scroll_lbounded	(BstQSampler			*qsampler,
					 guint				 offset,
					 gfloat				 boundary_padding,
					 gfloat				 padding);
void	   bst_qsampler_scroll_bounded	(BstQSampler			*qsampler,
					 guint				 offset,
					 gfloat				 boundary_padding,
					 gfloat				 padding);
void	   bst_qsampler_scroll_to	(BstQSampler			*qsampler,
					 guint				 offset);
void	   bst_qsampler_force_refresh	(BstQSampler			*qsampler);
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
void	   bst_qsampler_set_draw_mode	(BstQSampler *qsampler, Bst::QSamplerDrawMode dmode);
void	   bst_qsampler_set_adjustment	(BstQSampler			*qsampler,
					 GtkAdjustment			*adjustment);

void       bst_qsampler_set_source_from_esample (BstQSampler *qsampler, Bse::EditableSampleH esample, uint nth_channel);

#endif /* __BST_QSAMPLER_H__ */
