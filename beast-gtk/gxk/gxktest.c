/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2003 Tim Janik
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
#include "gxk/gxk.h"


/* --- variables --- */
static guint led_colors[] = {
  GXK_LED_OFF,
  GXK_LED_MAGENTA,
  GXK_LED_OFF,
  GXK_LED_RED,
  GXK_LED_OFF,
  GXK_LED_YELLOW,
  GXK_LED_OFF,
  GXK_LED_GREEN,
  GXK_LED_OFF,
  GXK_LED_CYAN,
  GXK_LED_OFF,
  GXK_LED_BLUE,
};


/* --- functions --- */
static gint
led_timer (gpointer data)
{
  GxkLed *led = data;
  static guint led_cindex = 0;

  GDK_THREADS_ENTER ();
  led_cindex++;
  led_cindex %= G_N_ELEMENTS (led_colors);
  gxk_led_set_color (led, led_colors[led_cindex]);
  GDK_THREADS_LEAVE ();
  return TRUE;
}

int
main (int   argc,
      char *argv[])
{
  GtkWidget *window, *led, *polygon1, *polygon2, *polygon, *box, *box2;
  GxkPolygonArc arcs[64];
  guint i;
  
  /* GLib's thread and object systems */
  g_thread_init (NULL);
  g_type_init ();
  
  /* initialize Gtk+ and go into threading mode */
  gtk_init (&argc, &argv);
  g_set_prgname ("GxkTest");	/* overriding Gdk's program name */
  GDK_THREADS_ENTER ();
  
  /* initialize Gtk+ Extension Kit */
  gxk_init ();
  
  {
    GxkPolygonLine lines[] = {
      { 0.5, 0.5, 0.0, 0.9 }, /* \ */
      { 0.5, 0.5, 0.0, 1.0 }, /* \ */
      { 0.5, 0.5, 0.1, 1.0 }, /* \ */
      { 0.5, 0.5, 0.9, 1.0 }, /* / */
      { 0.5, 0.5, 1.0, 1.0 }, /* / */
      { 0.5, 0.5, 1.0, 0.9 }, /* / */
      { 0.5, 0.5, 1.0, 0.1 }, /* \ */
      { 0.5, 0.5, 1.0, 0.0 }, /* \ */
      { 0.5, 0.5, 0.9, 0.0 }, /* \ */
      { 0.5, 0.5, 0.1, 0.0 }, /* / */
      { 0.5, 0.5, 0.0, 0.0 }, /* / */
      { 0.5, 0.5, 0.0, 0.1 }, /* / */
    };
    GxkPolygonGraph graph = { G_N_ELEMENTS (lines), lines, 0, NULL };
    polygon1 = gxk_polygon_new (&graph);
  }
  
  {
    GxkPolygonLine lines[] = {
      { 0.0, 0.9, 0.5, 0.5 }, /* \ */
      { 0.0, 1.0, 0.5, 0.5 }, /* \ */
      { 0.1, 1.0, 0.5, 0.5 }, /* \ */
      { 0.9, 1.0, 0.5, 0.5 }, /* / */
      { 1.0, 1.0, 0.5, 0.5 }, /* / */
      { 1.0, 0.9, 0.5, 0.5 }, /* / */
      { 1.0, 0.1, 0.5, 0.5 }, /* \ */
      { 1.0, 0.0, 0.5, 0.5 }, /* \ */
      { 0.9, 0.0, 0.5, 0.5 }, /* \ */
      { 0.1, 0.0, 0.5, 0.5 }, /* / */
      { 0.0, 0.0, 0.5, 0.5 }, /* / */
      { 0.0, 0.1, 0.5, 0.5 }, /* / */
    };
    GxkPolygonGraph graph = { G_N_ELEMENTS (lines), lines, 0, NULL };
    polygon2 = gxk_polygon_new (&graph);
  }
  i = 0;
  arcs[i++] = (GxkPolygonArc) { 0.5, 0.5, 0.3, 0.3, 0, +360 }; /* O */
  arcs[i++] = (GxkPolygonArc) { 0.5, 0.5, 0.1, 0.1, 0, -360 }; /* o */
  gxk_polygon_set_arcs (GXK_POLYGON (polygon2), i, arcs);
  
  box = g_object_new (GTK_TYPE_VBOX,
		      "visible", TRUE,
		      NULL);
  gtk_box_pack_start (GTK_BOX (box), polygon1, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (box), polygon2, TRUE, TRUE, 5);
  box2 = g_object_new (GTK_TYPE_HBOX,
		       "visible", TRUE,
		       NULL);
  gtk_box_pack_start (GTK_BOX (box), box2, FALSE, FALSE, 5);

  polygon = gxk_polygon_new (&gxk_polygon_power);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  
  polygon = gxk_polygon_new (&gxk_polygon_stop);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);

  polygon = gxk_polygon_new (&gxk_polygon_first);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);

  polygon = gxk_polygon_new (&gxk_polygon_previous);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  
  polygon = gxk_polygon_new (&gxk_polygon_rewind);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  
  polygon = gxk_polygon_new (&gxk_polygon_play);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  
  polygon = gxk_polygon_new (&gxk_polygon_pause);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  
  polygon = gxk_polygon_new (&gxk_polygon_forward);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  
  polygon = gxk_polygon_new (&gxk_polygon_next);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);

  polygon = gxk_polygon_new (&gxk_polygon_last);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  
  {
    GxkPolygonLine lines[] = {
      { 0.25, 0.25, 0.75, 0.25 }, /* right */
      { 0.75, 0.25, 0.75, 0.75 }, /* up */
      { 0.75, 0.75, 0.25, 0.75 }, /* left */
      { 0.25, 0.75, 0.25, 0.25 }, /* down */
    };
    GxkPolygonGraph graph = { G_N_ELEMENTS (lines), lines, 0, NULL, 30 };
    polygon = gxk_polygon_new (&graph);
  }
  // gxk_polygon_set_length (GXK_POLYGON (polygon), 21);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  
  led = gxk_led_new (0x13f4e5);
  gtk_box_pack_start (GTK_BOX (box), led, TRUE, TRUE, 5);
  window = g_object_new (GTK_TYPE_WINDOW,
			 "border_width", 20,
			 "child", box,
			 "default_width", 400,
			 "default_height", 400,
			 "visible", TRUE,
			 NULL);
  g_timeout_add (400, led_timer, led);

  /* start main loop */
  gtk_main ();

  return 0;
}
