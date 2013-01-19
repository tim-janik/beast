// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gxk/gxk.hh"
#include "gxkracktable.hh"
#include "gxkrackeditor.hh"
#include "gxkrackitem.hh"
#include "gxkscrollcanvas.hh"
/* --- prototype --- */
static void     rack_test (void);
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
/* --- prototypes --- */
static void scroll_canvas_test (void);
/* --- functions --- */
static gint
led_timer (gpointer data)
{
  GxkLed *led = (GxkLed*) data;
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
  GtkWidget *led, *polygon1, *polygon2, *polygon, *box, *box2;
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
  scroll_canvas_test ();
  rack_test ();
  /* test polygons */
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
    polygon1 = (GtkWidget*) gxk_polygon_new (&graph);
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
    polygon2 = (GtkWidget*) gxk_polygon_new (&graph);
  }
  i = 0;
  arcs[i++] = (GxkPolygonArc) { 0.5, 0.5, 0.3, 0.3, 0, +360 }; /* O */
  arcs[i++] = (GxkPolygonArc) { 0.5, 0.5, 0.1, 0.1, 0, -360 }; /* o */
  gxk_polygon_set_arcs (GXK_POLYGON (polygon2), i, arcs);
  box = (GtkWidget*) g_object_new (GTK_TYPE_VBOX,
                                   "visible", TRUE,
                                   NULL);
  gtk_box_pack_start (GTK_BOX (box), polygon1, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX (box), polygon2, TRUE, TRUE, 5);
  box2 = (GtkWidget*) g_object_new (GTK_TYPE_HBOX,
                                    "visible", TRUE,
                                    NULL);
  gtk_box_pack_start (GTK_BOX (box), box2, FALSE, FALSE, 5);
  polygon = (GtkWidget*) gxk_polygon_new (&gxk_polygon_power);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  polygon = (GtkWidget*) gxk_polygon_new (&gxk_polygon_stop);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  polygon = (GtkWidget*) gxk_polygon_new (&gxk_polygon_first);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  polygon = (GtkWidget*) gxk_polygon_new (&gxk_polygon_previous);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  polygon = (GtkWidget*) gxk_polygon_new (&gxk_polygon_rewind);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  polygon = (GtkWidget*) gxk_polygon_new (&gxk_polygon_play);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  polygon = (GtkWidget*) gxk_polygon_new (&gxk_polygon_pause);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  polygon = (GtkWidget*) gxk_polygon_new (&gxk_polygon_forward);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  polygon = (GtkWidget*) gxk_polygon_new (&gxk_polygon_next);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  polygon = (GtkWidget*) gxk_polygon_new (&gxk_polygon_last);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  {
    GxkPolygonLine lines[] = {
      { 0.25, 0.25, 0.75, 0.25 }, /* right */
      { 0.75, 0.25, 0.75, 0.75 }, /* up */
      { 0.75, 0.75, 0.25, 0.75 }, /* left */
      { 0.25, 0.75, 0.25, 0.25 }, /* down */
    };
    GxkPolygonGraph graph = { G_N_ELEMENTS (lines), lines, 0, NULL, 30 };
    polygon = (GtkWidget*) gxk_polygon_new (&graph);
  }
  // gxk_polygon_set_length (GXK_POLYGON (polygon), 21);
  gtk_box_pack_start (GTK_BOX (box2), polygon, FALSE, FALSE, 5);
  led = (GtkWidget*) gxk_led_new (0x13f4e5);
  gtk_box_pack_start (GTK_BOX (box), led, TRUE, TRUE, 5);
  g_object_new (GTK_TYPE_WINDOW,
                "border_width", 20,
                "child", box,
                "default_width", 400,
                "default_height", 400,
                "visible", TRUE,
                "sensitive", FALSE,
                NULL);
  g_timeout_add (400, led_timer, led);
  /* start main loop */
  gtk_main ();
  return 0;
}
static void
toggle_edit_mode (GtkToggleButton *tb,
                  GxkRackTable    *rtable)
{
  gxk_rack_table_set_edit_mode (rtable, tb->active);
  if (tb->active)
    gxk_rack_table_uncover (rtable);
  else
    gxk_rack_table_cover_up (rtable);
}
static void G_GNUC_NORETURN
exit_program ()
{
  exit (0);
}
static void
rack_test (void)
{
  GtkWidget *win, *box = (GtkWidget*) g_object_new (GTK_TYPE_VBOX, "visible", TRUE, NULL);
  GtkWidget *button = (GtkWidget*) g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                                                 "visible", TRUE,
                                                 "use_underline", TRUE,
                                                 "label", "_Edit",
                                                 NULL);
  GxkRackTable *rtable = (GxkRackTable*) g_object_new (GXK_TYPE_RACK_TABLE, NULL);
  gtk_table_resize (GTK_TABLE (rtable), 20, 30);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (rtable), TRUE, TRUE, 0);
  g_object_connect (button, "signal::clicked", toggle_edit_mode, rtable, NULL);
  gtk_table_attach_defaults (GTK_TABLE (rtable),
                             (GtkWidget*) g_object_new (GTK_TYPE_BUTTON, "visible", 1, "label", "Huhu", NULL),
                             3, 20, 3, 5);
  gtk_container_add (GTK_CONTAINER (rtable),
                     (GtkWidget*) g_object_new (GXK_TYPE_RACK_ITEM,
                                                "child", g_object_new (GTK_TYPE_LABEL, "visible", 1, "label", "RackItem", NULL),
                                                NULL));
  gtk_container_add (GTK_CONTAINER (rtable),
                     (GtkWidget*) g_object_new (GXK_TYPE_RACK_ITEM, NULL));
  gtk_container_add (GTK_CONTAINER (rtable),
                     (GtkWidget*) g_object_new (GXK_TYPE_RACK_ITEM,
                                                "shadow-type", GTK_SHADOW_ETCHED_IN,
                                                "label-widget", g_object_new (GXK_TYPE_SIMPLE_LABEL,
                                                                              "visible", 1,
                                                                              "label", "Huhu, this is an opverlong label text",
                                                                              "auto-cut", TRUE,
                                                                              NULL),
                                                NULL));
  win = (GtkWidget*) g_object_new (GTK_TYPE_WINDOW,
                                   "border_width", 20,
                                   "child", box,
                                   "sensitive", TRUE,
                                   "visible", TRUE,
                                   NULL);
  g_object_connect (win,
                    "signal::hide", exit_program, NULL,
                    "signal::delete-event", gtk_widget_hide_on_delete, NULL,
                    NULL);
  /* start main loop */
  gtk_main ();
}
static GxkScrollCanvasLayout faked_layout = { 10, 10, 10, 10, 32, 320, 20, 200 };
static void
toggle_panel_sizes (GtkToggleButton *tb,
                    GxkScrollCanvas *scc)
{
  gint sz = tb->active ? 10 : 0;
  GxkScrollCanvasLayout *layout = &faked_layout; // scc->layout;
  glong l = g_object_get_long (tb, "user_data");
  switch (l)
    {
    case 1:     layout->top_panel_height = sz; break;
    case 2:     layout->left_panel_width = sz; break;
    case 3:     layout->right_panel_width = sz; break;
    case 4:     layout->bottom_panel_height = sz; break;
    case 5:
      gtk_container_set_border_width ((GtkContainer*) scc, sz);
      break;
    }
  gtk_widget_queue_resize ((GtkWidget*) scc);
}
static void
faked_scroll_canvas_get_layout (GxkScrollCanvas        *self,
                                GxkScrollCanvasLayout  *layout)
{
  *layout = faked_layout;
}
static void
scroll_canvas_test (void)
{
  GtkWidget *win, *box = (GtkWidget*) g_object_new (GTK_TYPE_VBOX, "visible", TRUE, NULL);
  GtkWidget *btn1 = (GtkWidget*) g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                                               "visible", TRUE,
                                               "use_underline", TRUE,
                                               "label", "_Top",
                                               "active", 1,
                                               "user_data", 1,
                                               NULL);
  GtkWidget *btn2 = (GtkWidget*) g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                                               "visible", TRUE,
                                               "use_underline", TRUE,
                                               "label", "_Left",
                                               "active", 1,
                                               "user_data", 2,
                                               NULL);
  GtkWidget *btn3 = (GtkWidget*) g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                                               "visible", TRUE,
                                               "use_underline", TRUE,
                                               "label", "_Right",
                                               "active", 1,
                                               "user_data", 3,
                                               NULL);
  GtkWidget *btn4 = (GtkWidget*) g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                                               "visible", TRUE,
                                               "use_underline", TRUE,
                                               "label", "_Bottom",
                                               "active", 1,
                                               "user_data", 4,
                                               NULL);
  GtkWidget *btn5 = (GtkWidget*) g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                                               "visible", TRUE,
                                               "use_underline", TRUE,
                                               "label", "Border _Width",
                                               "active", 1,
                                               "user_data", 5,
                                               NULL);
  GtkWidget *frame = (GtkWidget*) g_object_new (GTK_TYPE_FRAME,
                                                "visible", TRUE,
                                                "shadow_type", GTK_SHADOW_IN,
                                                "border_width", 5,
                                                NULL);
  GxkScrollCanvas *scc = (GxkScrollCanvas*) g_object_new (GXK_TYPE_SCROLL_CANVAS,
                                                          "border-width", 10,
                                                          "parent", frame,
                                                          NULL);
  /* patch up scroll-canvas class to have a layout */
  g_type_class_unref (g_type_class_ref (GXK_TYPE_SCROLL_CANVAS)); // ensure class exists
  GXK_SCROLL_CANVAS_CLASS (g_type_class_peek (GXK_TYPE_SCROLL_CANVAS))->get_layout = faked_scroll_canvas_get_layout;
  gtk_box_pack_start (GTK_BOX (box), btn1, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), btn2, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), btn3, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), btn4, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), btn5, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
  g_object_connect (btn1, "signal::clicked", toggle_panel_sizes, scc, NULL);
  g_object_connect (btn2, "signal::clicked", toggle_panel_sizes, scc, NULL);
  g_object_connect (btn3, "signal::clicked", toggle_panel_sizes, scc, NULL);
  g_object_connect (btn4, "signal::clicked", toggle_panel_sizes, scc, NULL);
  g_object_connect (btn5, "signal::clicked", toggle_panel_sizes, scc, NULL);
  win = (GtkWidget*) g_object_new (GTK_TYPE_WINDOW,
                                   "border_width", 20,
                                   "child", box,
                                   "sensitive", TRUE,
                                   "default-height", 320,
                                   "default-width", 400,
                                   "visible", TRUE,
                                   NULL);
  g_object_connect (win,
                    "signal::hide", exit_program, NULL,
                    "signal::delete-event", gtk_widget_hide_on_delete, NULL,
                    NULL);
  /* start main loop */
  gtk_main ();
}
