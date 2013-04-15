// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include	"bstsampleeditor.hh"

#include	"bstprocedure.hh"
#include	"bstmenus.hh"
#include	<gdk/gdkkeysyms.h>


/* --- parameters --- */
enum
{
  PARAM_0,
  PARAM_SAMPLE
};


/* --- prototypes --- */
static void	bst_sample_editor_class_init	(BstSampleEditorClass	*klass);
static void	bst_sample_editor_init		(BstSampleEditor	*sample_editor);
static void	bst_sample_editor_destroy	(GtkObject		*object);
static void	bst_sample_editor_finalize	(GObject		*object);
static void	bst_sample_editor_set_property	(GObject		*object,
						 guint			 prop_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	bst_sample_editor_get_property	(GObject		*object,
						 guint			 prop_id,
						 GValue			*value,
						 GParamSpec		*pspec);


/* --- static variables --- */
static gpointer		   parent_class = NULL;


/* --- functions --- */
GtkType
bst_sample_editor_get_type (void)
{
  static GtkType sample_editor_type = 0;

  if (!sample_editor_type)
    {
      GtkTypeInfo sample_editor_info =
      {
	(char*) "BstSampleEditor",
	sizeof (BstSampleEditor),
	sizeof (BstSampleEditorClass),
	(GtkClassInitFunc) bst_sample_editor_class_init,
	(GtkObjectInitFunc) bst_sample_editor_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      sample_editor_type = gtk_type_unique (GTK_TYPE_VBOX, &sample_editor_info);
    }

  return sample_editor_type;
}

static void
bst_sample_editor_class_init (BstSampleEditorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  // GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = bst_sample_editor_set_property;
  gobject_class->get_property = bst_sample_editor_get_property;
  gobject_class->finalize = bst_sample_editor_finalize;

  object_class->destroy = bst_sample_editor_destroy;

  g_object_class_install_property (gobject_class, PARAM_SAMPLE,
				   sfi_pspec_proxy ("sample", NULL, NULL,
						    SFI_PARAM_READWRITE));
}

static void
bst_sample_editor_init (BstSampleEditor *editor)
{
  /* setup main container */
  editor->main_vbox = GTK_WIDGET (editor);
}

static void
bst_sample_editor_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  BstSampleEditor *editor = BST_SAMPLE_EDITOR (object);

  switch (prop_id)
    {
    case PARAM_SAMPLE:
      bst_sample_editor_set_sample (editor, sfi_value_get_proxy (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_sample_editor_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
  BstSampleEditor *editor = BST_SAMPLE_EDITOR (object);

  switch (prop_id)
    {
    case PARAM_SAMPLE:
      sfi_value_set_proxy (value, editor->esample);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
bst_sample_editor_set_sample (BstSampleEditor *editor,
			      SfiProxy	       sample)
{
  g_return_if_fail (BST_IS_SAMPLE_EDITOR (editor));
  if (sample)
    g_return_if_fail (BSE_IS_EDITABLE_SAMPLE (sample));

  if (sample != editor->esample)
    {
      if (editor->esample)
	bse_item_unuse (editor->esample);
      editor->esample = sample;
      editor->n_channels = 0;
      if (editor->esample)
	{
	  bse_item_use (editor->esample);
	  editor->n_channels = bse_editable_sample_get_n_channels (editor->esample);
	}
      bst_sample_editor_rebuild (editor);
      g_object_notify (G_OBJECT (editor), "sample");
    }
}

static void
bst_sample_editor_destroy (GtkObject *object)
{
  BstSampleEditor *editor = BST_SAMPLE_EDITOR (object);

  bst_sample_editor_set_sample (editor, 0);

  if (editor->play_back)
    bst_play_back_handle_destroy (editor->play_back);
  editor->play_back = NULL;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_sample_editor_finalize (GObject *object)
{
  // BstSampleEditor *editor = BST_SAMPLE_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget*
bst_sample_editor_new (SfiProxy sample)
{
  GtkWidget *widget;

  widget = gtk_widget_new (BST_TYPE_SAMPLE_EDITOR, "sample", sample, NULL);

  return widget;
}

#define QSAMPLER_SELECTION_TIMEOUT      (33)
static gulong qsampler_selection_timeout_id = 0;

static void
qsampler_set_selection (BstQSampler *qsampler,
			gint         m1,
			gint         m2,
			gboolean     visible_mark)
{
  BstSampleEditor *editor = BST_SAMPLE_EDITOR (qsampler->owner);
  guint i, length = bse_editable_sample_get_length (editor->esample);

  m1 = CLAMP (m1, 0, (gint) (length / editor->n_channels));
  m2 = CLAMP (m2, 0, (gint) (length / editor->n_channels));
  for (i = 0; i < editor->n_channels; i++)
    {
      BstQSampler *qs = editor->qsampler[i];
      gchar *s;

      bst_qsampler_set_mark (qs, 1, m1, BST_QSAMPLER_SELECTED);
      bst_qsampler_set_mark (qs, 2, m2, BST_QSAMPLER_SELECTED);
      bst_qsampler_set_region (qs, 1, MIN (m1, m2), 1 + MAX (m1, m2) - MIN (m1, m2), BST_QSAMPLER_SELECTED);
      if (visible_mark == 1)
	bst_qsampler_scroll_show (qs, m1);
      else if (visible_mark == 2)
	bst_qsampler_scroll_show (qs, m2);
      s = g_strdup_printf ("%d", MIN (m1, m2));
      gtk_entry_set_text (editor->sstart, s);
      g_free (s);
      s = g_strdup_printf ("%d", MAX (m1, m2));
      gtk_entry_set_text (editor->send, s);
      g_free (s);
    }
}

static gboolean
qsampler_selection_timeout (gpointer data)
{
  BstQSampler *qsampler;
  gboolean retain = FALSE;

  GDK_THREADS_ENTER ();

  qsampler = BST_QSAMPLER (data);
  if (GTK_WIDGET_DRAWABLE (qsampler))
    {
      gint m1 = bst_qsampler_get_mark_offset (qsampler, 1);
      gint m2 = bst_qsampler_get_mark_offset (qsampler, 2);
      gint x;

      gdk_window_get_pointer (GTK_WIDGET (qsampler)->window, &x, NULL, NULL);
      bst_qsampler_get_offset_at (qsampler, &x);
      retain = x < qsampler->pcm_length && x >= 0;
      if (ABS (m1 - x) < ABS (m2 - x))
	qsampler_set_selection (qsampler, m2, x, 2);
      else
	qsampler_set_selection (qsampler, m1, x, 2);
    }

  if (retain && qsampler_selection_timeout_id)
    {
      GDK_THREADS_LEAVE ();
      return TRUE;
    }
  else
    {
      qsampler_selection_timeout_id = 0;
      g_object_unref (qsampler);
      GDK_THREADS_LEAVE ();
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
      bst_qsampler_get_offset_at (qsampler, &x);

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
	qsampler_selection_timeout_id = g_timeout_add_full (GTK_PRIORITY_REDRAW + 1,
							    QSAMPLER_SELECTION_TIMEOUT,
							    qsampler_selection_timeout,
							    g_object_ref (qsampler), NULL);
    }

  return handled;
}

static void
change_draw_mode (BstSampleEditor *editor,
		  GtkOptionMenu   *omenu)
{
  guint i;
  BstQSamplerDrawMode mode = (BstQSamplerDrawMode) bst_choice_get_last (omenu->menu);

  for (i = 0; i < editor->n_channels; i++)
    {
      BstQSampler *qsampler = editor->qsampler[i];

      bst_qsampler_set_draw_mode (qsampler, mode);
    }
}

static void
update_play_back_marks (gpointer data,
			SfiNum   tick_stamp,
			guint    pcm_pos)
{
  BstSampleEditor *editor = (BstSampleEditor*) data;
  guint i;

  pcm_pos /= editor->n_channels;
  for (i = 0; i < editor->n_channels; i++)
    {
      BstQSampler *qsampler = editor->qsampler[i];

      bst_qsampler_set_mark (qsampler, 3, pcm_pos, BstQSamplerType (0));
      bst_qsampler_force_refresh (qsampler);
      bst_qsampler_scroll_rbounded (qsampler, pcm_pos, 0.98, 0.05);
      bst_qsampler_set_mark (qsampler, 3, pcm_pos, BST_QSAMPLER_PRELIGHT);
    }
}

static void
play_back_wchunk (BstSampleEditor *editor)
{
  if (!editor->play_back)
    {
      editor->play_back = bst_play_back_handle_new ();
      bst_play_back_handle_set (editor->play_back,
				editor->esample,
				bse_editable_sample_get_osc_freq (editor->esample));
    }
  bst_play_back_handle_toggle (editor->play_back);
  if (bst_play_back_handle_is_playing (editor->play_back))
    bst_play_back_handle_pcm_notify (editor->play_back, 50, update_play_back_marks, editor);
}

static void
adjustments_changed (BstSampleEditor *editor,
		     GtkAdjustment   *adjustment)
{
  guint i;

  for (i = 0; i < editor->n_channels; i++)
    {
      BstQSampler *qsampler = editor->qsampler[i];

      if (adjustment == editor->zoom_adjustment)
	bst_qsampler_set_zoom (qsampler, adjustment->value);
      else if (adjustment == editor->vscale_adjustment)
	bst_qsampler_set_vscale (qsampler, adjustment->value);
    }
}

void
bst_sample_editor_rebuild (BstSampleEditor *editor)
{
  GtkWidget *qsampler_parent, *sbar, *entry, *mask_parent, *any;
  gpointer gmask;
  guint i;

  g_return_if_fail (BST_IS_SAMPLE_EDITOR (editor));

  gtk_container_foreach (GTK_CONTAINER (editor), (GtkCallback) gtk_widget_destroy, NULL);
  g_free (editor->qsampler);
  editor->qsampler = NULL;
  if (!editor->esample)
    return;

  qsampler_parent = gtk_widget_new (GTK_TYPE_VBOX,
				    "visible", TRUE,
				    "spacing", 1,
				    "parent", editor->main_vbox,
				    "border_width", 0,
				    NULL);
  editor->qsampler = g_renew (BstQSampler*, editor->qsampler, editor->n_channels);


  /* setup qsampler widgets
   */
  sbar = gtk_widget_new (GTK_TYPE_HSCROLLBAR,
			 "visible", TRUE,
			 NULL);
  for (i = 0; i < editor->n_channels; i++)
    {
      GtkWidget *qbox = gtk_widget_new (GTK_TYPE_VBOX,
					"visible", TRUE,
					"border_width", 0,
					"parent", qsampler_parent,
					NULL);
      BstQSampler *qsampler = (BstQSampler*) g_object_new (BST_TYPE_QSAMPLER,
					    "visible", TRUE,
					    "events", (GDK_BUTTON_PRESS_MASK |
						       GDK_BUTTON_RELEASE_MASK |
						       GDK_BUTTON1_MOTION_MASK),
					    "parent", qbox,
					    "height_request", 80,
					    NULL);
      g_object_connect (qsampler,
			"signal::button_press_event", qsampler_button_event, NULL,
			"signal::button_release_event", qsampler_button_event, NULL,
			"signal::motion_notify_event", qsampler_motion_event, NULL,
			NULL);
      bst_qsampler_set_adjustment (qsampler, gtk_range_get_adjustment (GTK_RANGE (sbar)));
      qsampler->owner_index = i;
      qsampler->owner = editor;
      editor->qsampler[i] = qsampler;
      if (!editor->esample)
	bst_qsampler_set_source (qsampler, 0, NULL, NULL, NULL);
      else
	bst_qsampler_set_source_from_esample (qsampler, editor->esample, i);
    }
  gtk_box_pack_start (GTK_BOX (qsampler_parent), sbar, FALSE, TRUE, 0);


  /* setup qsampler zoom and vscale
   */
  mask_parent = bst_gmask_container_create (5, TRUE);
  gtk_box_pack_start (GTK_BOX (editor->main_vbox), mask_parent, FALSE, TRUE, 0);
  editor->zoom_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (100, 1e-16, 1e+16, 0.1, 10, 0));
  g_object_connect (editor->zoom_adjustment,
		    "swapped_signal_after::value_changed", adjustments_changed, editor,
		    "swapped_signal::destroy", g_nullify_pointer, &editor->zoom_adjustment,
		    NULL);
  entry = (GtkWidget*) g_object_new (GTK_TYPE_SPIN_BUTTON,
                                     "adjustment", editor->zoom_adjustment,
                                     "digits", 5,
                                     "visible", TRUE,
                                     NULL);
  gmask = bst_gmask_quick (mask_parent, 0, _("Zoom:"), entry, NULL);
  editor->vscale_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (100, 1e-16, 1e+16, 1, 10, 0));
  g_object_connect (editor->vscale_adjustment,
		    "swapped_signal_after::value_changed", adjustments_changed, editor,
		    "swapped_signal::destroy", g_nullify_pointer, &editor->vscale_adjustment,
		    NULL);
  entry = (GtkWidget*) g_object_new (GTK_TYPE_SPIN_BUTTON,
                                     "adjustment", editor->vscale_adjustment,
                                     "digits", 5,
                                     "visible", TRUE,
                                     NULL);
  gmask = bst_gmask_quick (mask_parent, 1, _("VScale:"), entry, NULL);

  /* setup qsampler selection start and end
   */
  editor->sstart = (GtkEntry*) g_object_new (GTK_TYPE_ENTRY,
                                             "visible", TRUE,
                                             NULL);
  g_object_connect (editor->sstart,
		    "swapped_signal::destroy", g_nullify_pointer, &editor->sstart,
		    NULL);
  gmask = bst_gmask_quick (mask_parent, 0, _("Start:"), editor->sstart, NULL);
  editor->send = (GtkEntry*) g_object_new (GTK_TYPE_ENTRY,
                                           "visible", TRUE,
                                           NULL);
  g_object_connect (editor->send,
		    "swapped_signal::destroy", g_nullify_pointer, &editor->sstart,
		    NULL);
  gmask = bst_gmask_quick (mask_parent, 1, _("End:"), editor->send, NULL);

  /* setup sample display type
   */
  any = (GtkWidget*) g_object_new (GTK_TYPE_OPTION_MENU,
                                   "visible", TRUE,
                                   NULL);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (any),
			    bst_choice_menu_createv ("<BEAST-SampleEditor>/Popup",
						     BST_CHOICE (BST_QSAMPLER_DRAW_CRANGE, _("Shape Range"), NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_ZERO_SHAPE, _("Shape Average"), NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_MINIMUM_SHAPE, _("Shape Minimum"), NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_MAXIMUM_SHAPE, _("Shape Maximum"), NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_CSHAPE, _("Sketch Range"), NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_MIDDLE_LINE, _("Sketch Average"), NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_MINIMUM_LINE, _("Sketch Minimum"), NONE),
						     BST_CHOICE (BST_QSAMPLER_DRAW_MAXIMUM_LINE, _("Sketch Maximum"), NONE),
						     BST_CHOICE_END));
  g_object_connect (any,
		    "swapped_signal::changed", change_draw_mode, editor,
		    NULL);
  gtk_option_menu_set_history (GTK_OPTION_MENU (any), 0);
  gmask = bst_gmask_quick (mask_parent, 2, NULL, any, NULL);

  /* setup preview button
   */
  any = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
                                   "visible", TRUE,
                                   /* TRANSLATORS: here "Preview" is meant audible, i.e. to
                                    * playback the current sample version and listen to it.
                                    */
                                   "label", _("Preview"),
                                   NULL);
  g_object_connect (any,
		    "swapped_signal::clicked", play_back_wchunk, editor,
		    NULL);
  gmask = bst_gmask_quick (mask_parent, 2, NULL, any, NULL);
}
