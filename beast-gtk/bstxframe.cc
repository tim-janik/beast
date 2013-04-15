// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstutils.hh"	/* for GScanner */
#include "bstxframe.hh"


enum {
  PARAM_0,
  PARAM_COVER,
  PARAM_STEAL_BUTTON
};


/* --- prototypes --- */
static void     bst_xframe_destroy                    (GtkObject	*object);
static void     bst_xframe_set_property		      (GObject		*object,
						       guint		 prop_id,
						       const GValue	*value,
						       GParamSpec	*pspec);
static void	bst_xframe_get_property		      (GObject		*object,
						       guint		 prop_id,
						       GValue		*value,
						       GParamSpec	*pspec);
static void     bst_xframe_realize                    (GtkWidget	*widget);
static void     bst_xframe_unrealize                  (GtkWidget	*widget);
static void     bst_xframe_map		              (GtkWidget	*widget);
static void     bst_xframe_unmap	              (GtkWidget	*widget);
static void     bst_xframe_resize		      (BstXFrame	*xframe);
static void     bst_xframe_size_allocate              (GtkWidget	*widget,
						       GtkAllocation	*allocation);
static gint     bst_xframe_button_press		      (GtkWidget	*widget,
						       GdkEventButton	*event);
static gint     bst_xframe_button_release             (GtkWidget	*widget,
						       GdkEventButton	*event);
static gint     bst_xframe_enter_notify		      (GtkWidget	*widget,
						       GdkEventCrossing *event);
static gint     bst_xframe_leave_notify		      (GtkWidget	*widget,
						       GdkEventCrossing *event);
static void     bst_xframe_size_allocate_cover        (GtkWidget	*widget,
						       GtkAllocation	*allocation);


/* --- variables --- */
static guint    signal_button_check = 0;


/* --- functions --- */
G_DEFINE_TYPE (BstXFrame, bst_xframe, GTK_TYPE_FRAME);

static void
bst_xframe_class_init (BstXFrameClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->set_property = bst_xframe_set_property;
  gobject_class->get_property = bst_xframe_get_property;
  object_class->destroy = bst_xframe_destroy;

  widget_class->size_allocate = bst_xframe_size_allocate;
  widget_class->realize = bst_xframe_realize;
  widget_class->unrealize = bst_xframe_unrealize;
  widget_class->map = bst_xframe_map;
  widget_class->unmap = bst_xframe_unmap;
  widget_class->button_press_event = bst_xframe_button_press;
  widget_class->button_release_event = bst_xframe_button_release;
  widget_class->enter_notify_event = bst_xframe_enter_notify;
  widget_class->leave_notify_event = bst_xframe_leave_notify;

  g_object_class_install_property (gobject_class, PARAM_COVER,
				   g_param_spec_object ("cover", "Cover", NULL,
							GTK_TYPE_WIDGET,
							G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PARAM_STEAL_BUTTON,
				   g_param_spec_boolean ("steal_button", "Steal Button3", "Whether to steal button2 clicks from cover widget",
							 FALSE,
							 G_PARAM_READWRITE));
  signal_button_check = g_signal_new ("button_check",
				      G_OBJECT_CLASS_TYPE (klass),
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (BstXFrameClass, button_check),
				      NULL, NULL,
				      bst_marshal_BOOLEAN__UINT,
				      G_TYPE_BOOLEAN, 1, G_TYPE_UINT);
}

static void
bst_xframe_init (BstXFrame *xframe)
{
  xframe->iwindow = NULL;
  xframe->cover = NULL;
  xframe->button_down = 0;
  xframe->allocation_valid = FALSE;
  xframe->entered = FALSE;
  g_object_set (xframe, "visible", TRUE, "shadow_type", GTK_SHADOW_NONE, NULL);
}

static void
bst_xframe_destroy (GtkObject *object)
{
  BstXFrame *xframe = BST_XFRAME (object);

  if (xframe->cover)
    bst_xframe_set_cover_widget (xframe, NULL, FALSE);

  GTK_OBJECT_CLASS (bst_xframe_parent_class)->destroy (object);
}

static void
bst_xframe_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  BstXFrame *xframe = BST_XFRAME (object);

  switch (prop_id)
    {
    case PARAM_COVER:
      bst_xframe_set_cover_widget (xframe, (GtkWidget*) g_value_get_object (value), xframe->steal_button);
      break;
    case PARAM_STEAL_BUTTON:
      bst_xframe_set_cover_widget (xframe, xframe->cover, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_xframe_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
  BstXFrame *xframe = BST_XFRAME (object);

  switch (prop_id)
    {
    case PARAM_COVER:
      g_value_set_object (value, xframe->cover);
      break;
    case PARAM_STEAL_BUTTON:
      g_value_set_boolean (value, xframe->steal_button);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bst_xframe_resize (BstXFrame *xframe)
{
  if (GTK_WIDGET_REALIZED (xframe))
    {
      GtkAllocation a = GTK_WIDGET (xframe)->allocation;

      if (FALSE && xframe->allocation_valid)
	{
	  a.width = MAX (a.x + a.width, xframe->cover->allocation.x + xframe->cover->allocation.width);
	  a.width -= a.x;
	  a.height = MAX (a.y + a.height, xframe->cover->allocation.y + xframe->cover->allocation.height);
	  a.height -= a.y;
	  a.x = MIN (a.x, xframe->cover->allocation.x);
	  a.y = MIN (a.x, xframe->cover->allocation.y);
	}
      gdk_window_move_resize (xframe->iwindow, a.x, a.y, a.width, a.height);
      gdk_window_lower (xframe->iwindow);
    }
}

static void
bst_xframe_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation)
{
  BstXFrame *xframe = BST_XFRAME (widget);

  GTK_WIDGET_CLASS (bst_xframe_parent_class)->size_allocate (widget, allocation);

  bst_xframe_resize (xframe);
}

static void
bst_xframe_realize (GtkWidget *widget)
{
  BstXFrame *xframe = BST_XFRAME (widget);
  GdkWindowAttr attributes;
  gint attributes_mask;

  GTK_WIDGET_CLASS (bst_xframe_parent_class)->realize (widget);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  xframe->iwindow = gdk_window_new (gtk_widget_get_parent_window (widget),
				    &attributes, attributes_mask);
  gdk_window_set_user_data (xframe->iwindow, xframe);
}

static void
bst_xframe_unrealize (GtkWidget *widget)
{
  BstXFrame *xframe = BST_XFRAME (widget);

  gdk_window_set_user_data (xframe->iwindow, NULL);
  gdk_window_destroy (xframe->iwindow);
  xframe->iwindow = NULL;

  GTK_WIDGET_CLASS (bst_xframe_parent_class)->unrealize (widget);
}

static void
bst_xframe_map (GtkWidget *widget)
{
  BstXFrame *xframe = BST_XFRAME (widget);

  GTK_WIDGET_CLASS (bst_xframe_parent_class)->map (widget);

  gdk_window_show (xframe->iwindow);
  gdk_window_lower (xframe->iwindow);
}

static void
bst_xframe_unmap (GtkWidget *widget)
{
  BstXFrame *xframe = BST_XFRAME (widget);

  gdk_window_hide (xframe->iwindow);

  GTK_WIDGET_CLASS (bst_xframe_parent_class)->unmap (widget);
}

static gint
bst_xframe_button_press (GtkWidget      *widget,
			 GdkEventButton *event)
{
  BstXFrame *xframe = BST_XFRAME (widget);

  if (!xframe->button_down && xframe->entered)
    {
      xframe->button_down = event->button;
      return TRUE;
    }

  return FALSE;
}

static gint
bst_xframe_button_release (GtkWidget      *widget,
			   GdkEventButton *event)
{
  BstXFrame *xframe = BST_XFRAME (widget);

  if (event->button == xframe->button_down)
    {
      gboolean valid = FALSE;

      if (xframe->entered)
	g_signal_emit (widget, signal_button_check, 0, xframe->button_down, &valid);
      xframe->button_down = 0;
      return TRUE;
    }

  return FALSE;
}

static gint
bst_xframe_enter_notify (GtkWidget        *widget,
			 GdkEventCrossing *event)
{
  BstXFrame *xframe = BST_XFRAME (widget);

  if (GTK_WIDGET_IS_SENSITIVE (widget) &&
      (!GTK_BIN (widget)->child || GTK_WIDGET_IS_SENSITIVE (GTK_BIN (widget)->child)))
    {
      gboolean valid = FALSE;

      g_signal_emit (widget, signal_button_check, 0, 0, &valid);

      if (valid)
	{
	  g_object_set (xframe, "shadow_type", GTK_SHADOW_ETCHED_OUT, NULL);
	  xframe->entered = TRUE;
	}
    }

  return FALSE;
}

static gint
bst_xframe_leave_notify (GtkWidget        *widget,
			 GdkEventCrossing *event)
{
  BstXFrame *xframe = BST_XFRAME (widget);

  g_object_set (xframe, "shadow_type", GTK_SHADOW_NONE, NULL);
  xframe->entered = FALSE;

  return FALSE;
}

static void
bst_xframe_size_allocate_cover (GtkWidget     *widget,
				GtkAllocation *allocation)
{
  BstXFrame *xframe = BST_XFRAME (widget);

  xframe->allocation_valid = (GTK_WIDGET_VISIBLE (xframe->cover) &&
			      xframe->cover->window &&
			      gdk_window_is_viewable (xframe->cover->window));
  xframe->allocation = xframe->cover->allocation;
  bst_xframe_resize (xframe);
}

void
bst_xframe_set_cover_widget (BstXFrame *xframe,
			     GtkWidget *widget,
			     gboolean   steal_button)
{
  g_return_if_fail (BST_IS_XFRAME (xframe));
  if (widget)
    g_return_if_fail (GTK_IS_WIDGET (widget));

  if (xframe->cover)
    {
      g_signal_handlers_disconnect_by_func (xframe->cover, (void*) bst_xframe_size_allocate_cover, xframe);
      g_signal_handlers_disconnect_by_func (xframe->cover, (void*) bst_xframe_button_press, xframe);
      g_signal_handlers_disconnect_by_func (xframe->cover, (void*) bst_xframe_button_release, xframe);
      g_signal_handlers_disconnect_by_func (xframe->cover, (void*) bst_xframe_enter_notify, xframe);
      g_signal_handlers_disconnect_by_func (xframe->cover, (void*) bst_xframe_leave_notify, xframe);
      g_object_unref (xframe->cover);
    }
  xframe->cover = widget;
  xframe->steal_button = steal_button != FALSE;
  if (xframe->cover)
    {
      g_object_connect (xframe->cover,
			"swapped_signal_after::size_allocate", bst_xframe_size_allocate_cover, xframe,
			"swapped_signal::enter_notify_event", bst_xframe_enter_notify, xframe,
			"swapped_signal::leave_notify_event", bst_xframe_leave_notify, xframe,
			!steal_button ? NULL :
			"swapped_signal::button_press_event", bst_xframe_button_press, xframe,
			"swapped_signal::button_release_event", bst_xframe_button_release, xframe,
			NULL);
      g_object_ref (xframe->cover);
    }
  g_object_notify (G_OBJECT (xframe), "cover");
  g_object_notify (G_OBJECT (xframe), "steal_button");
}
