#ifndef __GLE_WIDGETS_H__
#define __GLE_WIDGETS_H__
#include <gtk/gtk.h>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
#ifndef __GTK_WRAP_BOX_H__
#define __GTK_WRAP_BOX_H__


/* #include <gdk/gdk.h> */
/* #include <gtk/gtkcontainer.h> */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define GTK_TYPE_WRAP_BOX	     (gtk_wrap_box_get_type ())
#define GTK_WRAP_BOX(obj)	     (GTK_CHECK_CAST ((obj), GTK_TYPE_WRAP_BOX, GtkWrapBox))
#define GTK_WRAP_BOX_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_WRAP_BOX, GtkWrapBoxClass))
#define GTK_IS_WRAP_BOX(obj)	     (GTK_CHECK_TYPE ((obj), GTK_TYPE_WRAP_BOX))
#define GTK_IS_WRAP_BOX_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_WRAP_BOX))
#define GTK_WRAP_BOX_GET_CLASS(obj)  (GTK_WRAP_BOX_CLASS (((GtkObject*) (obj))->klass))


/* --- typedefs --- */
typedef struct _GtkWrapBox      GtkWrapBox;
typedef struct _GtkWrapBoxClass GtkWrapBoxClass;
typedef struct _GtkWrapBoxChild GtkWrapBoxChild;

/* --- GtkWrapBox --- */
struct _GtkWrapBox
{
  GtkContainer     container;
  
  guint            homogeneous : 1;
  guint            justify : 4;
  guint            line_justify : 4;
  guint8           hspacing;
  guint8           vspacing;
  guint16          n_children;
  GtkWrapBoxChild *children;
  gfloat           aspect_ratio; /* 1/256..256 */
  guint            child_limit;
};
struct _GtkWrapBoxClass
{
  GtkContainerClass parent_class;

  GSList* (*rlist_line_children) (GtkWrapBox       *wbox,
				  GtkWrapBoxChild **child_p,
				  GtkAllocation    *area,
				  guint            *max_child_size,
				  gboolean         *expand_line);
};
struct _GtkWrapBoxChild
{
  GtkWidget *widget;
  guint      hexpand : 1;
  guint      hfill : 1;
  guint      vexpand : 1;
  guint      vfill : 1;
  guint      wrapped : 1;
  
  GtkWrapBoxChild *next;
};
#define GTK_JUSTIFY_TOP    GTK_JUSTIFY_LEFT
#define GTK_JUSTIFY_BOTTOM GTK_JUSTIFY_RIGHT


/* --- prototypes --- */
GtkType	   gtk_wrap_box_get_type            (void);
void	   gtk_wrap_box_set_homogeneous     (GtkWrapBox      *wbox,
					     gboolean         homogeneous);
void	   gtk_wrap_box_set_hspacing        (GtkWrapBox      *wbox,
					     guint            hspacing);
void	   gtk_wrap_box_set_vspacing        (GtkWrapBox      *wbox,
					     guint            vspacing);
void	   gtk_wrap_box_set_justify         (GtkWrapBox      *wbox,
					     GtkJustification justify);
void	   gtk_wrap_box_set_line_justify    (GtkWrapBox      *wbox,
					     GtkJustification line_justify);
void	   gtk_wrap_box_set_aspect_ratio    (GtkWrapBox      *wbox,
					     gfloat           aspect_ratio);
void	   gtk_wrap_box_pack	            (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gboolean         hexpand,
					     gboolean         hfill,
					     gboolean         vexpand,
					     gboolean         vfill);
void	   gtk_wrap_box_pack_wrapped        (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gboolean         hexpand,
					     gboolean         hfill,
					     gboolean         vexpand,
					     gboolean         vfill,
					     gboolean         wrapped);
void       gtk_wrap_box_reorder_child       (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gint             position);
void       gtk_wrap_box_query_child_packing (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gboolean        *hexpand,
					     gboolean        *hfill,
					     gboolean        *vexpand,
					     gboolean        *vfill,
					     gboolean        *wrapped);
void       gtk_wrap_box_set_child_packing   (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gboolean         hexpand,
					     gboolean         hfill,
					     gboolean         vexpand,
					     gboolean         vfill,
					     gboolean         wrapped);
guint*	   gtk_wrap_box_query_line_lengths  (GtkWrapBox	     *wbox,
					     guint           *n_lines);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_WRAP_BOX_H__ */
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
#ifndef __GTK_CLUE_HUNTER_H__
#define __GTK_CLUE_HUNTER_H__


/* #include	<gtk/gtk.h> */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define	GTK_TYPE_CLUE_HUNTER		(gtk_clue_hunter_get_type ())
#define	GTK_CLUE_HUNTER(object)	        (GTK_CHECK_CAST ((object), GTK_TYPE_CLUE_HUNTER, GtkClueHunter))
#define	GTK_CLUE_HUNTER_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_CLUE_HUNTER, GtkClueHunterClass))
#define	GTK_IS_CLUE_HUNTER(object)	(GTK_CHECK_TYPE ((object), GTK_TYPE_CLUE_HUNTER))
#define GTK_IS_CLUE_HUNTER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CLUE_HUNTER))
 
/* --- typedefs --- */
typedef	struct	_GtkClueHunterClass GtkClueHunterClass;
typedef	struct	_GtkClueHunter	    GtkClueHunter;


/* --- structures --- */
struct	_GtkClueHunter
{
  GtkWindow	 window;

  guint		 popped_up : 1;
  guint		 completion_tag : 1;
  guint		 pattern_matching : 1;
  guint		 keep_history : 1;
  guint		 align_width : 1;
  guint		 clist_column : 16;

  gchar		*cstring;

  GtkWidget	*scw;
  GtkWidget	*clist;
  GtkWidget	*entry;
};
struct	_GtkClueHunterClass
{
  GtkWindowClass	parent_class;

  void	(*activate)	(GtkClueHunter	*clue_hunter);
  void	(*popup)	(GtkClueHunter	*clue_hunter);
  void	(*popdown)	(GtkClueHunter	*clue_hunter);
  void	(*select_on)	(GtkClueHunter	*clue_hunter,
			 const gchar	*string);
};


/* --- prototypes --- */
GtkType	   gtk_clue_hunter_get_type	        (void);
void	   gtk_clue_hunter_popup	        (GtkClueHunter	*clue_hunter);
void	   gtk_clue_hunter_set_clist	        (GtkClueHunter	*clue_hunter,
						 GtkWidget	*clist,
						 guint16	 column);
void	   gtk_clue_hunter_set_entry	        (GtkClueHunter	*clue_hunter,
						 GtkWidget	*entry);
void	   gtk_clue_hunter_add_string	        (GtkClueHunter	*clue_hunter,
						 const gchar	*string);
void	   gtk_clue_hunter_remove_string	(GtkClueHunter	*clue_hunter,
						 const gchar	*string);
void	   gtk_clue_hunter_remove_matches	(GtkClueHunter	*clue_hunter,
						 const gchar	*pattern);
void	   gtk_clue_hunter_set_pattern_matching (GtkClueHunter	*clue_hunter,
						 gboolean	 on_off);
void	   gtk_clue_hunter_set_keep_history     (GtkClueHunter	*clue_hunter,
						 gboolean	 on_off);
void	   gtk_clue_hunter_set_align_width      (GtkClueHunter	*clue_hunter,
						 gboolean	 on_off);
void	   gtk_clue_hunter_select_on	        (GtkClueHunter	*clue_hunter,
						 const gchar	*string);
gchar*	   gtk_clue_hunter_try_complete	        (GtkClueHunter	*clue_hunter);
GtkWidget* gtk_clue_hunter_create_arrow		(GtkClueHunter	*clue_hunter);
GtkClueHunter* gtk_clue_hunter_from_entry	(GtkWidget	*entry);





#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif	/* __GTK_CLUE_HUNTER_H__ */
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
#ifndef __GTK_HWRAP_BOX_H__
#define __GTK_HWRAP_BOX_H__


/* #include <gle/gtkwrapbox.h> */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define GTK_TYPE_HWRAP_BOX	      (gtk_hwrap_box_get_type ())
#define GTK_HWRAP_BOX(obj)	      (GTK_CHECK_CAST ((obj), GTK_TYPE_HWRAP_BOX, GtkHWrapBox))
#define GTK_HWRAP_BOX_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_HWRAP_BOX, GtkHWrapBoxClass))
#define GTK_IS_HWRAP_BOX(obj)	      (GTK_CHECK_TYPE ((obj), GTK_TYPE_HWRAP_BOX))
#define GTK_IS_HWRAP_BOX_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_HWRAP_BOX))
#define GTK_HWRAP_BOX_GET_CLASS(obj)  (GTK_HWRAP_BOX_CLASS (((GtkObject*) (obj))->klass))


/* --- typedefs --- */
typedef struct _GtkHWrapBox      GtkHWrapBox;
typedef struct _GtkHWrapBoxClass GtkHWrapBoxClass;


/* --- GtkHWrapBox --- */
struct _GtkHWrapBox
{
  GtkWrapBox parent_widget;
  
  /*<h2v-off>*/
  guint16    max_child_width;
  guint16    max_child_height;
  /*<h2v-on>*/
};

struct _GtkHWrapBoxClass
{
  GtkWrapBoxClass parent_class;
};


/* --- prototypes --- */
GtkType	   gtk_hwrap_box_get_type           (void);
GtkWidget* gtk_hwrap_box_new                (gboolean homogeneous);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_HWRAP_BOX_H__ */
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
#ifndef __GTK_VWRAP_BOX_H__
#define __GTK_VWRAP_BOX_H__


/* #include <gle/gtkwrapbox.h> */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define GTK_TYPE_VWRAP_BOX	      (gtk_vwrap_box_get_type ())
#define GTK_VWRAP_BOX(obj)	      (GTK_CHECK_CAST ((obj), GTK_TYPE_VWRAP_BOX, GtkVWrapBox))
#define GTK_VWRAP_BOX_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_VWRAP_BOX, GtkVWrapBoxClass))
#define GTK_IS_VWRAP_BOX(obj)	      (GTK_CHECK_TYPE ((obj), GTK_TYPE_VWRAP_BOX))
#define GTK_IS_VWRAP_BOX_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_VWRAP_BOX))
#define GTK_VWRAP_BOX_GET_CLASS(obj)  (GTK_VWRAP_BOX_CLASS (((GtkObject*) (obj))->klass))


/* --- typedefs --- */
typedef struct _GtkVWrapBox      GtkVWrapBox;
typedef struct _GtkVWrapBoxClass GtkVWrapBoxClass;


/* --- GtkVWrapBox --- */
struct _GtkVWrapBox
{
  GtkWrapBox parent_widget;
  
  /*<h2v-off>*/
  guint16    max_child_width;
  guint16    max_child_height;
  /*<h2v-on>*/
};

struct _GtkVWrapBoxClass
{
  GtkWrapBoxClass parent_class;
};


/* --- prototypes --- */
GtkType	   gtk_vwrap_box_get_type           (void);
GtkWidget* gtk_vwrap_box_new                (gboolean homogeneous);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_VWRAP_BOX_H__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __GLE_WIDGETS_H__ */
