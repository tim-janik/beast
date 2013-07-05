// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstutils.hh"
#include "bstqsampler.hh"
// #include "bstcellptrack.h"
// #include <bse/gsldatacache.hh>
// #include <bse/gsldatahandle.hh>
#include <string.h>

#include <math.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() { return 0; }

#if 0

#define	QSAMPLER_SELECTION_TIMEOUT	(33)

typedef struct _WaveView WaveView;
struct _WaveView
{
  BstQSampler *qsampler;
  GslDataHandle *handle;
  WaveView *next;
};
static WaveView *wave_views = NULL;


static gulong qsampler_selection_timeout_id = 0;

static void
qsampler_set_selection (BstQSampler *qsampler,
			gint         m1,
			gint         m2,
			gboolean     visible_mark)
{
  bst_qsampler_set_mark (qsampler, 1, m1, BST_QSAMPLER_SELECTED);
  bst_qsampler_set_mark (qsampler, 2, m2, BST_QSAMPLER_SELECTED);
  bst_qsampler_set_region (qsampler, 1, MIN (m1, m2), 1 + MAX (m1, m2) - MIN (m1, m2), BST_QSAMPLER_SELECTED);
  if (visible_mark == 1)
    bst_qsampler_scroll_show (qsampler, m1);
  else if (visible_mark == 2)
    bst_qsampler_scroll_show (qsampler, m2);
}

static gboolean
qsampler_selection_timeout (gpointer data)
{
  BstQSampler *qsampler = BST_QSAMPLER (data);
  gboolean retain = FALSE;

  if (GTK_WIDGET_DRAWABLE (qsampler))
    {
      gint m1 = bst_qsampler_get_mark_offset (qsampler, 1);
      gint m2 = bst_qsampler_get_mark_offset (qsampler, 2);
      gint x;

      gdk_window_get_pointer (GTK_WIDGET (qsampler)->window, &x, NULL, NULL);
      if (!bst_qsampler_get_offset_at (qsampler, &x))
	{
	  gint b;

	  if (x < 0)
	    {
	      bst_qsampler_get_bounds (qsampler, &b, NULL);
	      x = MAX (0, b + x);
	    }
	  else
	    {
	      bst_qsampler_get_bounds (qsampler, NULL, &b);
	      x = MIN (qsampler->pcm_length - 1, b + x);
	    }
	  retain = TRUE;
	}
      if (ABS (m1 - x) < ABS (m2 - x))
	qsampler_set_selection (qsampler, m2, x, 2);
      else
	qsampler_set_selection (qsampler, m1, x, 2);
    }

  if (retain && qsampler_selection_timeout_id)
    return TRUE;
  else
    {
      qsampler_selection_timeout_id = 0;
      g_object_unref (qsampler);
      return FALSE;
    }
}

static gboolean
qsampler_button_event (BstQSampler    *qsampler,
		       GdkEventButton *event)
{
  gboolean handled = FALSE;

  if (event->button == 1)
    {
      gint m1 = bst_qsampler_get_mark_offset (qsampler, 1);
      gint m2 = bst_qsampler_get_mark_offset (qsampler, 2);
      gint x = event->x;

      handled = TRUE;
      if (!bst_qsampler_get_offset_at (qsampler, &x))
	{
	  if (x < 0)
	    bst_qsampler_get_bounds (qsampler, &x, NULL);
	  else
	    bst_qsampler_get_bounds (qsampler, NULL, &x);
	}

      if (event->type == GDK_BUTTON_PRESS && (event->state & GDK_SHIFT_MASK) &&
	  m1 >= 0 && m2 >= 0)
	{
	  if (ABS (m1 - x) < ABS (m2 - x))
	    qsampler_set_selection (qsampler, m2, x, 2);
	  else
	    qsampler_set_selection (qsampler, m1, x, 2);
	}
      else if (event->type == GDK_BUTTON_PRESS)
	qsampler_set_selection (qsampler, x, x, 2);
      else if (event->type == GDK_BUTTON_RELEASE)
	{
	  if (qsampler_selection_timeout_id)
	    {
	      gtk_timeout_remove (qsampler_selection_timeout_id);
	      qsampler_selection_timeout_id = 0;
	    }
	}
    }

  return handled;
}

static gboolean
qsampler_motion_event (BstQSampler    *qsampler,
		       GdkEventMotion *event)
{
  gboolean handled = FALSE;

  if (event->type == GDK_MOTION_NOTIFY)
    {
      gint m1 = bst_qsampler_get_mark_offset (qsampler, 1);
      // gint m2 = bst_qsampler_get_mark_offset (qsampler, 2);
      gint x = event->x;

      handled = TRUE;
      if (bst_qsampler_get_offset_at (qsampler, &x))
	qsampler_set_selection (qsampler, m1, x, 2);
      else if (!qsampler_selection_timeout_id)
	qsampler_selection_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT + 1,
							    QSAMPLER_SELECTION_TIMEOUT,
							    qsampler_selection_timeout,
							    g_object_ref (qsampler), NULL);
    }

  return handled;
}


static void
selection_to_loop (BstQSampler *qsampler)
{
  gint m1 = bst_qsampler_get_mark_offset (qsampler, 1);
  gint m2 = bst_qsampler_get_mark_offset (qsampler, 2);

  if (m2 < 0 || m1 < 0)
    return;

  if (m2 < m1)
    {
      gint t = m2;
      m2 = m1;
      m1 = t;
    }
  bst_qsampler_set_mark (qsampler, 3, m1, BST_QSAMPLER_ACTIVE);
  bst_qsampler_set_mark (qsampler, 4, m2, BST_QSAMPLER_ACTIVE);
  bst_qsampler_set_region (qsampler, 2, m1, 1 + m2 - m1, BST_QSAMPLER_ACTIVE);
  bst_qsampler_set_region (qsampler, 1, 0, 0, 0);
  bst_qsampler_set_mark (qsampler, 1, 0, 0);
  bst_qsampler_set_mark (qsampler, 2, 0, 0);
}

static void
loop_to_selection (BstQSampler *qsampler)
{
  gint m1, m2;

  m1 = bst_qsampler_get_mark_offset (qsampler, 3);
  m2 = bst_qsampler_get_mark_offset (qsampler, 4);
  if (m2 < 0 || m1 < 0)
    return;

  qsampler_set_selection (qsampler, MIN (m1, m2), MAX (m1, m2), 0);
}

static void
zoom_selection (BstQSampler *qsampler)
{
  gint m1, m2;

  m1 = bst_qsampler_get_mark_offset (qsampler, 1);
  m2 = bst_qsampler_get_mark_offset (qsampler, 2);
  if (m2 < 0 || m1 < 0)
    return;
  if (m1 > m2)
    {
      gint t = m2;
      m2 = m1;
      m1 = t;
    }
  m2 = MAX (m2, m1 + 1);

  bst_qsampler_scroll_to (qsampler, m1);
  bst_qsampler_set_zoom (qsampler, qsampler->n_pixels / (gdouble) (m2 - m1) * 100.);
}

static void
adjust_zoom (GtkAdjustment *adjustment,
	     BstQSampler   *qsampler)
{
  bst_qsampler_set_zoom (qsampler, adjustment->value);
}
static void
adjust_vscale (GtkAdjustment *adjustment,
	       BstQSampler   *qsampler)
{
  bst_qsampler_set_vscale (qsampler, adjustment->value);
}

static void
qsampler_dcache_filler (gpointer     data,
			guint        voffset,
			guint        n_values,
			gint16      *values,
			BstQSampler *qsampler)
{
  GslDataCache *dcache = data;
  GslDataCacheNode *dnode;
  glong dcache_length, dnode_length;
  gint i;

  dnode = gsl_data_cache_ref_node (dcache, voffset, TRUE);
  dcache_length = gsl_data_handle_length (dcache->dhandle);
  dnode_length = dcache->node_size;
  for (i = 0; i < n_values; i++)
    {
      glong offset = voffset + i;

      if (offset < 0 || offset >= dcache_length)
	values[i] = 0;
      else
	{
	  if (offset < dnode->offset || offset >= dnode->offset + dnode_length)
	    {
	      gsl_data_cache_unref_node (dcache, dnode);
	      dnode = gsl_data_cache_ref_node (dcache, offset, TRUE);
	    }
	  values[i] = dnode->data[offset - dnode->offset] * 32768.0;
	}
    }
  gsl_data_cache_unref_node (dcache, dnode);
}

static void
qsampler_set_handle (BstQSampler   *qsampler,
		     GslDataHandle *handle)
{
  GslDataCache *dcache = gsl_data_cache_new (handle, 1);

  gsl_data_cache_open (dcache);
  bst_qsampler_set_source (qsampler, gsl_data_handle_length (dcache->dhandle),
			   qsampler_dcache_filler, dcache, (GDestroyNotify) gsl_data_cache_close);
  gsl_data_cache_unref (dcache);
}

GslDataHandle *global_handle = NULL;

static void
unset_loop (BstQSampler *qsampler)
{
  qsampler_set_handle (qsampler, global_handle);
}

static void
set_loop (BstQSampler *qsampler)
{
  gint m1 = bst_qsampler_get_mark_offset (qsampler, 3);
  gint m2 = bst_qsampler_get_mark_offset (qsampler, 4);

  if (m2 < 0 || m1 < 0)
    return;

  if (m2 < m1)
    {
      gint t = m2;
      m2 = m1;
      m1 = t;
    }

  if (m1 >= 0 && m2 > m1)
    {
      GslDataHandle *handle = gsl_data_handle_new_looped (global_handle, m1, m2);

      qsampler_set_handle (qsampler, handle);
      gsl_data_handle_unref (handle);
      qsampler_set_selection (qsampler, m1, m2, 0);
    }
}

static void
score (BstQSampler *qsampler)
{
  GslDataCache *dcache = qsampler->src_data;
  GslDataHandle *shandle = global_handle;
  GslDataHandle *dhandle = dcache->dhandle;
  GslLong l, length = MIN (gsl_data_handle_length (shandle), gsl_data_handle_length (dhandle));
  gdouble score = 0;

  for (l = 0; l < length; )
    {
      GslLong b = 8192;
      gfloat v1[b], v2[b];

      b = MIN (b, length - l);
      b = gsl_data_handle_read (shandle, l, b, v1);
      b = gsl_data_handle_read (dhandle, l, b, v2);
      g_assert (b >= 0);
      g_assert (b >= 1);
      l += b;

      while (b--)
	score += (v1[b] - v2[b]) * (v1[b] - v2[b]);
      // g_print ("0x%08lx) %10.3f\n", l, score);
    }
  g_print ("total score: %10.3f\n", score);
}

static gdouble
score_loop (GslDataHandle *shandle,
	    GslDataHandle *dhandle)
{
  GslLong l, length = MIN (gsl_data_handle_length (shandle), gsl_data_handle_length (dhandle));
  gdouble score = 0;

  for (l = 0; l < length; )
    {
      GslLong b = 8192;
      gfloat v1[b], v2[b];

      b = MIN (b, length - l);
      b = gsl_data_handle_read (shandle, l, b, v1);
      b = gsl_data_handle_read (dhandle, l, b, v2);
      g_assert (b >= 0);
      g_assert (b >= 1);
      l += b;

      while (b--)
	score += (v1[b] - v2[b]) * (v1[b] - v2[b]);
    }
  return score;
}

#include <bse/gsldatautils.hh>

static void
find (WaveView *view)
{
  GslLong start, end;
  GslLong length = gsl_data_handle_length (view->handle);
  GslLoopSpec loop_spec = { 0, length / 3, 44100.0/15., length / 3.5 };

  gsl_data_find_tailmatch (view->handle, &loop_spec, &start, &end);
  qsampler_set_selection (view->qsampler, start, end, 2);
  selection_to_loop (view->qsampler);
}

static void
mark_signalh (WaveView *view)
{
  GslLong mark;

  mark = gsl_data_find_sample (view->handle,
			       1. / 32768. * +16.,
			       1. / 32768. * -16.,
			       0, +1);
  bst_qsampler_set_mark (view->qsampler, 5, MAX (mark, 0), mark < 0 ? 0 : BST_QSAMPLER_PRELIGHT);
}

static void
mark_signalt (WaveView *view)
{
  GslLong mark;

  mark = gsl_data_find_sample (view->handle,
			       1. / 32768. * +16.,
			       1. / 32768. * -16.,
			       -1, -1);
  bst_qsampler_set_mark (view->qsampler, 5, MAX (mark, 0), mark < 0 ? 0 : BST_QSAMPLER_PRELIGHT);
}

static void
findx ()
{
  GslDataCache *dcache = gsl_data_cache_new (global_handle, 1);
  GslDataHandle *shandle = gsl_data_handle_new_dcached (dcache);
  GslLong length = gsl_data_handle_length (shandle);
  GslLong l, start = 0, end = 0, lsize = gsl_data_handle_length (shandle) / 2;
  gdouble score = 0, least = GSL_MAXLONG;

  gsl_data_cache_unref (dcache);
  gsl_data_handle_open (shandle);

  while (lsize)
    {
      for (l = 0; l < length - lsize; l++)
	{
	  GslDataHandle *dhandle = gsl_data_handle_new_looped (shandle, l, l + lsize);

	  gsl_data_handle_open (dhandle);
	  score = score_loop (shandle, dhandle);
	  gsl_data_handle_close (dhandle);
	  gsl_data_handle_unref (dhandle);
	  if (score < least)
	    {
	      start = l;
	      end = l + lsize;
	      g_print ("\nimproved: %f < %f: [0x%lx..0x%lx] (%lu)\n", score, least, start, end, lsize);
	      least = score;
	    }
	  g_print ("\rprocessed: %lu / %lu\r", l, length - lsize);
	}
      lsize -= 1;
    }
  gsl_data_handle_close (shandle);
  gsl_data_handle_unref (shandle);
}

static GtkWidget*	pack_test_widget (void);

int
main (int   argc,
      char *argv[])
{
  GtkWidget *window, *vbox, *hbox, *sbar, *spin, *button;
  GslConfigValue gslconfig[] = {
    { "wave_chunk_padding",     1, },
    { "dcache_block_size",      8192, },
    { "dcache_cache_memory",	5 * 1024 * 1024, },
    { NULL, },
  };
  WaveView *view, *first_view = NULL;
  guint i;

  g_thread_init (NULL);
  g_type_init ();
  birnet_init (&argc, &argv, NULL);
  gsl_init (gslconfig);
  bse_init_inprocess (&argc, &argv, NULL, NULL);
  gtk_init (&argc, &argv);
  gxk_init ();
  _bst_init_utils ();

  if (argc < 2)
    g_error ("need filenames");

  vbox = gtk_widget_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "border_width", 10,
			 NULL);
  gtk_box_pack_start (GTK_BOX (vbox), pack_test_widget (), TRUE, TRUE, 0);

  sbar = gtk_widget_new (GTK_TYPE_HSCROLLBAR,
			 "visible", TRUE,
			 NULL);
  gtk_box_pack_start (GTK_BOX (vbox), sbar, FALSE, TRUE, 0);

  for (i = 1; i < argc; i++)
    {
      view = g_new (WaveView, 1);
      view->handle = gsl_wave_handle_new (argv[i], 1, 44100, 440, GSL_WAVE_FORMAT_SIGNED_16, G_LITTLE_ENDIAN, 0, -1);
      if (!view->handle)
	g_error ("failed to create handle for \"%s\": stat() failed", argv[i]);
      view->qsampler = g_object_new (BST_TYPE_QSAMPLER,
				     "visible", TRUE,
				     "events", (GDK_BUTTON_PRESS_MASK |
						GDK_BUTTON_RELEASE_MASK |
						GDK_BUTTON1_MOTION_MASK),
				     NULL);
      g_object_connect (view->qsampler,
			"signal::button_press_event", qsampler_button_event, view,
			"signal::button_release_event", qsampler_button_event, view,
			"signal::motion_notify_event", qsampler_motion_event, view,
			NULL);
      qsampler_set_handle (view->qsampler, view->handle);
      bst_qsampler_set_adjustment (view->qsampler, gtk_range_get_adjustment (GTK_RANGE (sbar)));
      gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (view->qsampler), TRUE, TRUE, 2);
      view->next = wave_views;
      wave_views = view;
      if (i == 1)
	first_view = view;
    }

  spin = gtk_spin_button_new (GTK_ADJUSTMENT (gtk_adjustment_new (100, 1e-16, 1e+16, 0.1, 10, 0)), 0, 5);
  gtk_widget_set (spin,
		  "visible", TRUE,
		  "width_request", 40,
		  NULL);
  for (view = wave_views; view; view = view->next)
    g_object_connect (GTK_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin))),
		      "signal::value_changed", adjust_zoom, view->qsampler,
		      NULL);
  gtk_box_pack_start (GTK_BOX (vbox), spin, FALSE, TRUE, 0);

  spin = gtk_spin_button_new (GTK_ADJUSTMENT (gtk_adjustment_new (100, 1e-16, 1e+16, 1, 10, 0)), 0, 5);
  gtk_widget_set (spin,
		  "visible", TRUE,
		  "width_request", 40,
		  NULL);
  for (view = wave_views; view; view = view->next)
    g_object_connect (GTK_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin))),
		      "signal::value_changed", adjust_vscale, view->qsampler,
		      NULL);
  gtk_box_pack_start (GTK_BOX (vbox), spin, FALSE, TRUE, 0);

  hbox = gtk_widget_new (GTK_TYPE_HBOX,
			 "visible", TRUE,
			 "border_width", 10,
			 NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  button = g_object_connect (g_object_new (GTK_TYPE_BUTTON,
					   "visible", TRUE,
					   "label", _("Selection to Loop"),
					   NULL),
			     "swapped_signal::clicked", selection_to_loop, first_view->qsampler,
			     NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  button = g_object_connect (g_object_new (GTK_TYPE_BUTTON,
					   "visible", TRUE,
					   "label", _("Loop to Selection"),
					   NULL),
			     "swapped_signal::clicked", loop_to_selection, first_view->qsampler,
			     NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  button = g_object_connect (g_object_new (GTK_TYPE_BUTTON,
					   "visible", TRUE,
					   "label", _("Zoom Selection"),
					   NULL),
			     "swapped_signal::clicked", zoom_selection, first_view->qsampler,
			     NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  button = g_object_connect (g_object_new (GTK_TYPE_BUTTON,
					   "visible", TRUE,
					   "label", _("Apply Loop"),
					   NULL),
			     "swapped_signal::clicked", set_loop, first_view->qsampler,
			     NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  button = g_object_connect (g_object_new (GTK_TYPE_BUTTON,
					   "visible", TRUE,
					   "label", _("Reset Loop"),
					   NULL),
			     "swapped_signal::clicked", unset_loop, first_view->qsampler,
			     NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  button = g_object_connect (g_object_new (GTK_TYPE_BUTTON,
					   "visible", TRUE,
					   "label", _("Score"),
					   NULL),
			     "swapped_signal::clicked", score, first_view->qsampler,
			     NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  button = g_object_connect (g_object_new (GTK_TYPE_BUTTON,
					   "visible", TRUE,
					   "label", _("Find"),
					   NULL),
			     "swapped_signal::clicked", find, first_view,
			     NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  window = g_object_connect (gtk_widget_new (GTK_TYPE_WINDOW,
					     "child", vbox,
					     "visible", TRUE,
					     "height_request", 450,
					     "width_request", 1400,
					     "allow_shrink", TRUE,
					     NULL),
			     "signal::destroy", gtk_main_quit, NULL,
			     NULL);

  button = g_object_new (GTK_TYPE_BUTTON,
			 "visible", TRUE,
			 "label", _("Mark Signal (Head)"),
			 NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  for (view = wave_views; view; view = view->next)
     g_object_connect (GTK_OBJECT (button),
		       "swapped_signal::clicked", mark_signalh, view,
		       NULL);

  button = g_object_new (GTK_TYPE_BUTTON,
			 "visible", TRUE,
			 "label", _("Mark Signal (Tail)"),
			 NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  for (view = wave_views; view; view = view->next)
     g_object_connect (GTK_OBJECT (button),
		       "swapped_signal::clicked", mark_signalt, view,
		       NULL);


  gtk_main ();

  return 0;
}

#if 0	/* test code */
static void
plist_fill_value (gpointer  da_NULL,
		  guint           column,
		  guint           row,
		  GValue         *value)
{
  switch (column)
    {
    case 0:
      g_value_set_string (value, "Foo");
      break;
    case 1:
      g_value_set_string (value, "Foozy");
      break;
    case 2:
      g_value_set_string (value, "Fuzy");
      break;
    }
}

static gboolean
tree_event (GtkTreeView *tree,
	    GdkEvent    *event)
{
  GtkTreeViewColumn *column;
  gint cx, cy, atrow;
  switch (event->type)
    {
      GdkRectangle rect;
      gint wx, wy;
    case GDK_BUTTON_PRESS:
      atrow = gtk_tree_view_get_path_at_pos (tree, event->button.x, event->button.y,
					     NULL, &column, &cx, &cy);
      g_print ("got button_press at %f,%f [%u] (%p %d %d)\n", event->button.x, event->button.y, atrow, column, cx, cy);
      break;
    case GDK_BUTTON_RELEASE:
      atrow = gtk_tree_view_get_path_at_pos (tree, event->button.x, event->button.y,
					     NULL, &column, &cx, &cy);
      g_print ("got button_release at %f,%f [%u] (%p %d %d)\n", event->button.x, event->button.y, atrow, column, cx, cy);
      break;
    case GDK_MOTION_NOTIFY:
      atrow = gtk_tree_view_get_path_at_pos (tree, event->motion.x, event->motion.y,
					     NULL, &column, &cx, &cy);
      gtk_tree_view_get_visible_rect (tree, &rect);
      gtk_tree_view_tree_to_widget_coords (tree, rect.x, rect.y, &wx, &wy);
      g_print ("got motion at %f,%f [%u] (%p %d %d) widget(%d %d)\n", event->motion.x, event->motion.y, atrow, column,
	       cx, cy, wx, wy);
      break;
    default:
    }
  return FALSE;
}

static GtkWidget*
pack_test_widget (void)
{
  GtkWidget *scwin, *tree;
  GtkTreeSelection *tsel;
  GxkListWrapper *plist = gxk_list_wrapper_new (3 /* N_COLS */,
						G_TYPE_STRING,  /* 0 */
						G_TYPE_STRING,  /* 1 */
						G_TYPE_STRING   /* 2 */
						);
  g_signal_connect_object (plist, "fill-value",
			   G_CALLBACK (plist_fill_value),
			   NULL, G_CONNECT_SWAPPED);
  gxk_list_wrapper_notify_prepend (plist, 200);

  scwin = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
			"visible", TRUE,
			"hscrollbar_policy", GTK_POLICY_AUTOMATIC,
			"vscrollbar_policy", GTK_POLICY_ALWAYS,
			"height_request", 320,
			"width_request", 320,
			"border_width", 5,
			"shadow_type", GTK_SHADOW_IN,
			NULL);
  tree = g_object_new (GTK_TYPE_TREE_VIEW,
		       "visible", TRUE,
		       "can_focus", TRUE,
		       "model", plist,
		       "border_width", 10,
		       "parent", scwin,
		       NULL);
  gxk_tree_view_append_text_columns (GTK_TREE_VIEW (tree), 3 /* N_COLS */,
				     0, "", 0.0, "Foo Name",
				     1, "", 0.0, "Bar Name",
				     2, "", 0.0, "Baz Type"
				     );
  gtk_tree_view_add_column (GTK_TREE_VIEW (tree), -1,
			    g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
					  "title", "PTrack",
					  "sizing", GTK_TREE_VIEW_COLUMN_GROW_ONLY,
					  "resizable", TRUE,
					  NULL),
			    g_object_new (BST_TYPE_CELL_PTRACK,
					  "xalign", 0.0,
					  NULL),
			    // "text", 3,
			    NULL);
  g_object_connect (tree,
		    "signal::event", tree_event, NULL,
		    NULL);
#if 0
  g_object_connect (tree,
		    "swapped_object_signal::row_activated", tree_row_activated, self,
		    NULL);
#endif

  /* ensure selection
   */
  tsel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (tsel, GTK_SELECTION_SINGLE);
  gxk_tree_selection_select_spath (tsel, "0");

  return scwin;
}
#else
static GtkWidget*
pack_test_widget (void)
{
  return g_object_new (GTK_TYPE_ALIGNMENT, NULL);
}
#endif


#endif
