/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GXK_DIALOG_H__
#define __GXK_DIALOG_H__

#include        "gxkutils.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_DIALOG              (gxk_dialog_get_type ())
#define GXK_DIALOG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_DIALOG, GxkDialog))
#define GXK_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_DIALOG, GxkDialogClass))
#define GXK_IS_DIALOG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_DIALOG))
#define GXK_IS_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_DIALOG))
#define GXK_DIALOG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_DIALOG, GxkDialogClass))


/* --- typedefs & enums --- */
typedef struct  _GxkDialog       GxkDialog;
typedef struct  _GxkDialogClass  GxkDialogClass;
typedef enum
{
  GXK_DIALOG_HIDE_ON_DELETE	= 1 << 0,	/* skips destroy upon delete event */
  GXK_DIALOG_STATUS_SHELL	= 1 << 1,
  GXK_DIALOG_MODAL		= 1 << 2,
  GXK_DIALOG_POPUP_POS		= 1 << 3,	/* popup at mouse pointer */
  GXK_DIALOG_DELETE_BUTTON	= 1 << 4,	/* has "Close" button */
  GXK_DIALOG_PRESERVE_STATE     = 1 << 5        /* don't always reset size etc. upon hiding */
} GxkDialogFlags;


/* --- structures --- */
struct _GxkDialog
{
  GtkWindow      window;

  GtkWidget	*vbox;

  /*< private >*/
  GtkObject	*alive_object;	/* dialog is destroyed with this object */
  GxkDialogFlags flags;
  gpointer	*pointer_loc;	/* nullified on destroy */
  GtkWidget	*status_bar;
  GtkWidget	*default_widget;
  GtkWidget	*focus_widget;
  GtkWidget	*sep;
  GtkWidget	*hbox;
  GtkWidget	*mbox;
  GtkWidget	*child;
};
struct _GxkDialogClass
{
  GtkWindowClass        parent_class;
};


/* --- prototypes --- */
GtkType		gxk_dialog_get_type		  (void);
gpointer	gxk_dialog_new			  (gpointer	   pointer_loc,
						   GtkObject	  *alive_object,
						   GxkDialogFlags  flags,
						   const gchar    *title,
						   GtkWidget	  *child);
gpointer        gxk_dialog_new_radget             (gpointer        pointer_loc,
                                                   GtkObject      *alive_object,
                                                   GxkDialogFlags  flags,
                                                   const gchar    *title,
                                                   const gchar    *domain_name,
                                                   const gchar    *radget_name);
void            gxk_dialog_set_sizes              (GxkDialog      *dialog,
                                                   gint            min_width,
                                                   gint            min_height,
                                                   gint            default_width,
                                                   gint            default_height);
void		gxk_dialog_set_title		  (GxkDialog	  *dialog,
						   const gchar	  *title);
void		gxk_dialog_set_focus		  (GxkDialog	  *dialog,
						   GtkWidget	  *widget);
void		gxk_dialog_set_default		  (GxkDialog	  *dialog,
						   GtkWidget	  *widget);
void		gxk_dialog_set_child		  (GxkDialog	  *dialog,
						   GtkWidget	  *child);
GtkWidget*	gxk_dialog_get_child		  (GxkDialog	  *dialog);
GxkDialog*	gxk_dialog_get_status_window	  (void);
void		gxk_dialog_add_flags		  (GxkDialog	  *dialog,
						   GxkDialogFlags  flags);
void		gxk_dialog_clear_flags		  (GxkDialog	  *dialog,
						   GxkDialogFlags  flags);
void		gxk_dialog_remove_actions	  (GxkDialog	  *dialog);
#define		gxk_dialog_action(		   dialog, action, callback, data)	\
                                                  gxk_dialog_action_multi ((dialog), (action), (callback), (data), 0, 0)
#define		gxk_dialog_default_action(	   dialog, action, callback, data)	\
                                                  gxk_dialog_action_multi ((dialog), (action), (callback), (data), 0, GXK_DIALOG_MULTI_DEFAULT)
#define		gxk_dialog_action_swapped(	   dialog, action, callback, data)	\
                                                  gxk_dialog_action_multi ((dialog), (action), (callback), (data), 0, GXK_DIALOG_MULTI_SWAPPED)
#define		gxk_dialog_default_action_swapped( dialog, action, callback, data)	\
                                                  gxk_dialog_action_multi ((dialog), (action), (callback), (data), 0, GXK_DIALOG_MULTI_DEFAULT | GXK_DIALOG_MULTI_SWAPPED)


/* --- internal --- */
typedef enum /*< skip >*/
{
  GXK_DIALOG_MULTI_DEFAULT = 1,
  GXK_DIALOG_MULTI_SWAPPED = 2
} GxkDialogMultiFlags;
GtkWidget*	gxk_dialog_action_multi		(GxkDialog	    *dialog,
						 const gchar	    *action,
						 gpointer	     callback,
						 gpointer	     data,
						 const gchar	    *icon_stock_id,
						 GxkDialogMultiFlags multi_mode);

G_END_DECLS

#endif  /* __GXK_DIALOG_H__ */
