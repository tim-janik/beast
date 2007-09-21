/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __GXK_STATUS_BAR_H__
#define __GXK_STATUS_BAR_H__

#include <gxk/gxkutils.h>

G_BEGIN_DECLS

/* --- status percentages --- */
#define	GXK_STATUS_PROGRESS	(+200.0)
#define	GXK_STATUS_DONE		(+100.0)
#define	GXK_STATUS_IDLE_HINT	(-0.4)
#define	GXK_STATUS_IDLE		(-0.5)
#define	GXK_STATUS_WAIT		(-1.0)
#define	GXK_STATUS_ERROR	(-2.0)
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
} GxkStatusBar;


/* --- prototypes --- */
GtkWidget* gxk_status_bar_create		(void);
void       gxk_status_enable_error_bell         (gboolean        enable_error_bell);
void	   gxk_status_set			(gfloat		 percentage,
						 const gchar	*message,
						 const gchar	*status_msg);
void	   gxk_status_printf			(gfloat		 percentage,
						 const gchar	*status_msg,
						 const gchar	*message_fmt,
						 ...) G_GNUC_PRINTF (3, 4);
void	   gxk_status_errnoprintf		(gint		 libc_errno,
						 const gchar	*message_fmt,
						 ...) G_GNUC_PRINTF (2, 3);
void	   gxk_status_clear			(void);
void	   gxk_status_window_push		(gpointer        widget);
void	   gxk_status_window_pop		(void);
void	   gxk_status_push_progress_window	(gpointer        widget);
void	   gxk_status_pop_progress_window	(void);


G_END_DECLS

#endif	/* __GXK_STATUS_BAR_H__ */
