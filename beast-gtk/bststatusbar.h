/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999, 2000, 2001 Tim Janik and Red Hat, Inc.
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
 *
 * bststatusbar.h: general code for status bar maintenance
 */
#ifndef __BST_STATUS_BAR_H__
#define __BST_STATUS_BAR_H__


#include	"bstdefs.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define	BST_STATUS_DONE		(+100.0)
#define	BST_STATUS_IDLE		(-0.5)
#define	BST_STATUS_WAIT		(-1.0)
#define	BST_STATUS_ERROR	(-2.0)


/* --- prototypes --- */
GtkWidget*	bst_status_bar_create		(void);
GtkWidget*	bst_status_bar_get_current	(void);
void		bst_status_bar_catch_procedure	(void);
void		bst_status_bar_uncatch_procedure (void);
GtkWidget*	bst_status_bar_from_widget	(GtkWidget	*widget);
void		bst_status_bar_queue_clear	(GtkWidget	*sbar,
						 guint		 msecs);
void		bst_status_bar_set		(GtkWidget	*sbar,
						 gfloat		 percentage,
						 const gchar	*message,
						 const gchar	*status_msg);
guint		bst_status_bar_set_permanent	(GtkWidget	*sbar,
						 gfloat		 percentage,
						 const gchar	*message,
						 const gchar	*status_msg);
void		bst_status_set			(gfloat		 percentage,
						 const gchar	*message,
						 const gchar	*status_msg);
void		bst_status_printf		(gfloat		 percentage,
						 const gchar	*status_msg,
						 const gchar	*message_fmt,
						 ...) G_GNUC_PRINTF (3, 4);
void		bst_status_clear		(void);
void		bst_status_window_push		(gpointer        widget);
void		bst_status_window_pop		(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif	/* __BST_STATUS_BAR_H__ */
