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


/* --- status percentages --- */
#define	BST_STATUS_PROGRESS	(+200.0)
#define	BST_STATUS_DONE		(+100.0)
#define	BST_STATUS_IDLE_HINT	(-0.4)
#define	BST_STATUS_IDLE		(-0.5)
#define	BST_STATUS_WAIT		(-1.0)
#define	BST_STATUS_ERROR	(-2.0)
/* 0..+100 is normal progression percentage */


/* --- auxillary structure --- */
typedef struct
{
  GtkWidget      *sbar;
  GtkProgressBar *pbar;
  GtkProgress    *prog;
  GtkLabel       *message;
  GtkLabel       *status;
  guint           is_idle : 1;
  guint		  timer_id;
} BstStatusBar;


/* --- prototypes --- */
GtkWidget* bst_status_bar_create		(void);
void	   bst_status_set			(gfloat		 percentage,
						 const gchar	*message,
						 const gchar	*status_msg);
void	   bst_status_printf			(gfloat		 percentage,
						 const gchar	*status_msg,
						 const gchar	*message_fmt,
						 ...) G_GNUC_PRINTF (3, 4);
void	   bst_status_eprintf			(BswErrorType	 error,
						 const gchar	*message_fmt,
						 ...) G_GNUC_PRINTF (2, 3);
void	   bst_status_errnoprintf		(gint		 libc_errno,
						 const gchar	*message_fmt,
						 ...) G_GNUC_PRINTF (2, 3);
void	   bst_status_clear			(void);
void	   bst_status_bar_catch_procs		(void);
void	   bst_status_bar_uncatch_procs		(void);
void	   bst_status_bar_listen_exec_status	(void);
void	   bst_status_window_push		(gpointer        widget);
void	   bst_status_window_pop		(void);
void	   bst_status_push_progress_window	(gpointer        widget);
void	   bst_status_pop_progress_window	(void);
void	   bst_status_set_script_control_window (BswProxy	script_control,
						 GtkWindow     *window);
void	   bst_status_delete_script_control	(BswProxy	script_control);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif	/* __BST_STATUS_BAR_H__ */
