/* BEAST - Bedevilled Audio System
 * Copyright (C) 2003 Tim Janik
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
#ifndef __BST_APP_CONTROL_H__
#define __BST_APP_CONTROL_H__

#include	<gxk/gxk.h>

G_BEGIN_DECLS

/* stop pause prev rew play fwd next */

/* --- typedefs & structures --- */
typedef struct {
  GtkWidget	*box;
  GxkLed	*led;
  GtkWidget	*power;
  GtkWidget	*stop;
  GtkWidget	*first;
  GtkWidget	*prev;
  GtkWidget	*rew;
  GtkWidget	*play;
  GtkWidget	*pause;
  GtkWidget	*fwd;
  GtkWidget	*next;
  GtkWidget	*last;
  GtkWidget	*record;
} BstAppControl;


/* --- prototypes --- */
BstAppControl*		bst_app_control_new	(void);


G_END_DECLS

#endif /* __BST_APP_CONTROL_H__ */
