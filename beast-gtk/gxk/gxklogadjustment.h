/* GXK - Gtk+ Extension Kit
 * Copyright (C) 1998-2003 Tim Janik
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
#ifndef __GXK_LOG_ADJUSTMENT_H__
#define __GXK_LOG_ADJUSTMENT_H__

#include <gxk/gxkutils.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_LOG_ADJUSTMENT              (gxk_log_adjustment_get_type ())
#define GXK_LOG_ADJUSTMENT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_LOG_ADJUSTMENT, GxkLogAdjustment))
#define GXK_LOG_ADJUSTMENT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_LOG_ADJUSTMENT, GxkLogAdjustmentClass))
#define GXK_IS_LOG_ADJUSTMENT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_LOG_ADJUSTMENT))
#define GXK_IS_LOG_ADJUSTMENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_LOG_ADJUSTMENT))
#define GXK_LOG_ADJUSTMENT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_LOG_ADJUSTMENT, GxkLogAdjustmentClass))

/* --- structures & typedefs --- */
typedef	struct	_GxkLogAdjustment	GxkLogAdjustment;
typedef	struct	_GxkLogAdjustmentClass	GxkLogAdjustmentClass;
struct _GxkLogAdjustment
{
  GtkAdjustment	parent_instance;
  
  /* settings */
  gdouble	 center;
  gdouble	 n_steps;
  gdouble	 base;
  GtkAdjustment *client;
  
  guint		 block_client;
  gdouble	 base_ln;
  gdouble	 llimit;
  gdouble	 ulimit;
};
struct _GxkLogAdjustmentClass
{
  GtkAdjustmentClass parent_class;
};

/* --- prototypes --- */
GType          gxk_log_adjustment_get_type   (void);
void           gxk_log_adjustment_set_client (GxkLogAdjustment *self,
                                              GtkAdjustment    *client);
GtkAdjustment* gxk_log_adjustment_from_adj   (GtkAdjustment    *client);
void           gxk_log_adjustment_setup      (GxkLogAdjustment *self,
                                              gdouble           center,
                                              gdouble           base,
                                              gdouble           n_steps);
G_END_DECLS

#endif /* __GXK_LOG_ADJUSTMENT_H__ */
