/* GXK - Gtk+ Extension Kit
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
#ifndef __GXK_LED_H__
#define __GXK_LED_H__

#include "gxkutils.h"

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_LED              (gxk_led_get_type ())
#define GXK_LED(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_LED, GxkLed))
#define GXK_LED_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_LED, GxkLedClass))
#define GXK_IS_LED(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_LED))
#define GXK_IS_LED_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_LED))
#define GXK_LED_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_LED, GxkLedClass))

#define	GXK_LED_OFF	(0x505050)
#define	GXK_LED_MAGENTA	(0xff50ff)
#define	GXK_LED_RED	(0xff5050)
#define	GXK_LED_YELLOW	(0xffff50)
#define	GXK_LED_GREEN	(0x50ff50)
#define	GXK_LED_CYAN	(0x50ffff)
#define	GXK_LED_BLUE	(0x5050ff)


/* --- structures --- */
typedef struct {
  GtkWidget	 parent_instance;
  guint		 color;
  guint	         border_width;
  /* rendering data */
  guint		 radius;
  GdkPixbuf	*pixbuf;
} GxkLed;
typedef struct {
  GtkWidgetClass parent_class;
} GxkLedClass;


/* --- prototypes --- */
GType		gxk_led_get_type		  (void);
gpointer	gxk_led_new			(guint	 color);
void		gxk_led_set_color		(GxkLed	*self,
						 guint	 rgb_colors);
void		gxk_led_set_colors		(GxkLed	*self,
						 guint	 n_colors,
						 ...);
void		gxk_led_set_border_width	(GxkLed	*self,
						 guint	 border_width);


G_END_DECLS

#endif  /* __GXK_LED_H__ */
