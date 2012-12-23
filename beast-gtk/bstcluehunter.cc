// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstcluehunter.hh"
#include "bstmarshal.h"
#include <gdk/gdkkeysyms.h>
#include <string.h>


/* --- signals --- */
enum {
  SIGNAL_ACTIVATE,
  SIGNAL_POPUP,
  SIGNAL_POPDOWN,
  SIGNAL_SELECT_ON,
  SIGNAL_POLL_REFRESH,
  SIGNAL_LAST
};


/* --- arguments --- */
enum {
  PROP_0,
  PROP_PATTERN_MATCHING,
  PROP_ALIGN_WIDGET,
  PROP_KEEP_HISTORY,
  PROP_ENTRY
};


/* --- prototypes --- */
static void	bst_clue_hunter_class_init	(BstClueHunterClass	*klass);
static void	bst_clue_hunter_init		(BstClueHunter		*clue_hunter);
static void	bst_clue_hunter_destroy		(GtkObject		*object);
static void	bst_clue_hunter_finalize	(GObject		*object);
static void     bst_clue_hunter_set_property	(GObject                *object,
						 guint                   prop_id,
						 const GValue           *value,
						 GParamSpec             *pspec);
static void     bst_clue_hunter_get_property	(GObject                *object,
						 guint                   prop_id,
						 GValue                 *value,
						 GParamSpec             *pspec);
static void	bst_clue_hunter_entry_changed	(BstClueHunter		*clue_hunter);
static gint	bst_clue_hunter_entry_key_press	(BstClueHunter		*clue_hunter,
						 GdkEventKey		*event,
						 GtkEntry		*entry);
static gint	bst_clue_hunter_clist_click	(BstClueHunter		*clue_hunter,
						 GdkEvent		*event,
						 GtkCList		*clist);
static gint	bst_clue_hunter_event           (GtkWidget		*widget,
						 GdkEvent		*event);
static void	bst_clue_hunter_do_activate	(BstClueHunter		*clue_hunter);
static void	bst_clue_hunter_do_popup	(BstClueHunter       	*clue_hunter);
static void	bst_clue_hunter_do_popdown      (BstClueHunter       	*clue_hunter);
static void	bst_clue_hunter_add_history	(BstClueHunter		*clue_hunter,
						 const gchar   		*string);
static void	bst_clue_hunter_do_select_on	(BstClueHunter		*clue_hunter,
						 const gchar		*string);
static void	bst_clue_hunter_popdown		(BstClueHunter		*clue_hunter);


/* --- variables --- */
static GtkWindowClass	  *parent_class = NULL;
static BstClueHunterClass *bst_clue_hunter_class = NULL;
static guint		   clue_hunter_signals[SIGNAL_LAST] = { 0, };


/* --- functions --- */
GtkType
bst_clue_hunter_get_type (void)
{
  static GtkType clue_hunter_type = 0;
  
  if (!clue_hunter_type)
    {
      GtkTypeInfo clue_hunter_info =
      {
	(char*) "BstClueHunter",
	sizeof (BstClueHunter),
	sizeof (BstClueHunterClass),
	(GtkClassInitFunc) bst_clue_hunter_class_init,
	(GtkObjectInitFunc) bst_clue_hunter_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      clue_hunter_type = gtk_type_unique (GTK_TYPE_WINDOW, &clue_hunter_info);
    }
  return clue_hunter_type;
}

static void
bst_clue_hunter_class_init (BstClueHunterClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  bst_clue_hunter_class = klass;
  parent_class = (GtkWindowClass*) g_type_class_peek_parent (klass);
  
  gobject_class->set_property = bst_clue_hunter_set_property;
  gobject_class->get_property = bst_clue_hunter_get_property;
  gobject_class->finalize = bst_clue_hunter_finalize;

  object_class->destroy = bst_clue_hunter_destroy;
  
  widget_class->event = bst_clue_hunter_event;
  
  klass->activate = bst_clue_hunter_do_activate;
  klass->popup = bst_clue_hunter_do_popup;
  klass->popdown = bst_clue_hunter_do_popdown;
  klass->select_on = bst_clue_hunter_do_select_on;

  /* override GtkWindow::type property */
  g_object_class_install_property (gobject_class,
				   1024,
				   g_param_spec_enum ("type", NULL, NULL,
						      GTK_TYPE_WINDOW_TYPE,
						      GTK_WINDOW_POPUP,
						      /* should be 0 */ G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				   PROP_PATTERN_MATCHING,
				   g_param_spec_boolean ("pattern_matching", NULL, NULL,
							 TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
				   PROP_ALIGN_WIDGET,
				   g_param_spec_object ("align-widget", NULL, NULL,
                                                        GTK_TYPE_WIDGET, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
				   PROP_KEEP_HISTORY,
				   g_param_spec_boolean ("keep_history", NULL, NULL,
							 FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
				   PROP_ENTRY,
				   g_param_spec_object ("entry", NULL, NULL,
							GTK_TYPE_ENTRY, G_PARAM_READWRITE));

  clue_hunter_signals[SIGNAL_ACTIVATE] = g_signal_new ("activate", G_OBJECT_CLASS_TYPE (klass),
						       G_SIGNAL_RUN_LAST,
						       G_STRUCT_OFFSET (BstClueHunterClass, activate),
						       NULL, NULL,
						       gtk_signal_default_marshaller,
						       G_TYPE_NONE, 0);
  widget_class->activate_signal = clue_hunter_signals[SIGNAL_ACTIVATE];
  clue_hunter_signals[SIGNAL_POPUP] = g_signal_new ("popup", G_OBJECT_CLASS_TYPE (klass),
						    G_SIGNAL_RUN_LAST,
						    G_STRUCT_OFFSET (BstClueHunterClass, popup),
						    NULL, NULL,
						    gtk_signal_default_marshaller,
						    G_TYPE_NONE, 0);
  clue_hunter_signals[SIGNAL_POPDOWN] = g_signal_new ("popdown", G_OBJECT_CLASS_TYPE (klass),
						      G_SIGNAL_RUN_LAST,
						      G_STRUCT_OFFSET (BstClueHunterClass, popdown),
						      NULL, NULL,
						      gtk_signal_default_marshaller,
						      G_TYPE_NONE, 0);
  clue_hunter_signals[SIGNAL_SELECT_ON] = g_signal_new ("select-on", G_OBJECT_CLASS_TYPE (klass),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET (BstClueHunterClass, select_on),
							NULL, NULL,
							g_cclosure_marshal_VOID__STRING,
							G_TYPE_NONE, 1, G_TYPE_STRING);
  clue_hunter_signals[SIGNAL_POLL_REFRESH] = g_signal_new ("poll-refresh", G_OBJECT_CLASS_TYPE (klass),
							   G_SIGNAL_RUN_LAST,
							   G_STRUCT_OFFSET (BstClueHunterClass, poll_refresh),
							   NULL, NULL,
							   gtk_signal_default_marshaller,
							   G_TYPE_NONE, 0);
}

static void
bst_clue_hunter_init (BstClueHunter *self)
{
  GtkWidget *parent;
  GtkCList *clist;
  
  self->popped_up = FALSE;
  self->completion_tag = FALSE;
  self->pattern_matching = TRUE;
  self->align_widget = NULL;
  self->keep_history = FALSE;
  self->cstring = NULL;

  GTK_WINDOW (self)->type = GTK_WINDOW_POPUP;

  g_object_set (self,
		"allow_shrink", FALSE,
		"allow_grow", FALSE,
		NULL);
  parent = (GtkWidget*) g_object_new (GTK_TYPE_FRAME,
                                      "visible", TRUE,
                                      "label", NULL,
                                      "shadow", GTK_SHADOW_OUT,
                                      "parent", self,
                                      NULL);
  self->scw = (GtkWidget*) g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                                         "visible", TRUE,
                                         "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
                                         "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
                                         "parent", parent,
                                         NULL);
  self->clist = NULL;
  self->entry = NULL;
  clist = (GtkCList*) g_object_new (GTK_TYPE_CLIST,
                                    "n_columns", 1,
                                    "titles_active", FALSE,
                                    NULL);
  gtk_clist_set_auto_sort (GTK_CLIST (clist), TRUE);
  gtk_clist_set_sort_type (GTK_CLIST (clist), GTK_SORT_ASCENDING);
  gtk_clist_column_titles_hide (GTK_CLIST (clist));
  bst_clue_hunter_set_clist (self, clist, 0);
}

static void
bst_clue_hunter_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  BstClueHunter *self = BST_CLUE_HUNTER (object);

  switch (prop_id)
    {
    case PROP_PATTERN_MATCHING:
      self->pattern_matching = g_value_get_boolean (value);
      break;
    case PROP_ALIGN_WIDGET:
      if (self->align_widget)
        g_object_unref (self->align_widget);
      self->align_widget = (GtkWidget*) g_value_get_object (value);
      if (self->align_widget)
        g_object_ref (self->align_widget);
      break;
    case PROP_KEEP_HISTORY:
      self->keep_history = g_value_get_boolean (value);
      break;
    case PROP_ENTRY:
      bst_clue_hunter_set_entry (self, (GtkEntry*) g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
      break;
    }
}

static void
bst_clue_hunter_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
  BstClueHunter *self = BST_CLUE_HUNTER (object);

  switch (prop_id)
    {
    case PROP_PATTERN_MATCHING:
      g_value_set_boolean (value, self->pattern_matching);
      break;
    case PROP_ALIGN_WIDGET:
      g_value_set_object (value, self->align_widget);
      break;
    case PROP_KEEP_HISTORY:
      g_value_set_boolean (value, self->keep_history);
      break;
    case PROP_ENTRY:
      g_value_set_object (value, self->entry);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
      break;
    }
}

static void
bst_clue_hunter_destroy (GtkObject *object)
{
  BstClueHunter *self = BST_CLUE_HUNTER (object);

  if (self->align_widget)
    {
      g_object_unref (self->align_widget);
      self->align_widget = NULL;
    }
  if (self->popped_up)
    bst_clue_hunter_popdown (self);

  self->scw = NULL;
  if (self->clist)
    g_object_unref (self->clist);
  self->clist = NULL;
  
  if (self->entry)
    bst_clue_hunter_set_entry (self, NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_clue_hunter_finalize (GObject *object)
{
  BstClueHunter *self = BST_CLUE_HUNTER (object);

  if (self->align_widget)
    g_object_unref (self->align_widget);
  g_free (self->cstring);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint
bst_clue_hunter_clist_click (BstClueHunter *self,
			     GdkEvent	   *event,
			     GtkCList	   *clist)
{
  gboolean handled = FALSE;

  if (event->type == GDK_2BUTTON_PRESS &&
      event->button.button == 1 && clist->selection)
    {
      gchar *string;

      handled = TRUE;
      string = bst_clue_hunter_try_complete (self);
      gtk_entry_set_text (GTK_ENTRY (self->entry), string ? string : "");
      g_free (string);

      bst_clue_hunter_popdown (self);
      gtk_widget_activate (GTK_WIDGET (self->entry));
    }

  return handled;
}

static gboolean
intercept_innermost_signal (GObject *object)
{
  GSignalInvocationHint *ihint = g_signal_get_invocation_hint (object);
  g_signal_stop_emission (object, ihint->signal_id, ihint->detail);
  return FALSE;
}

void
bst_clue_hunter_set_clist (BstClueHunter *self,
			   GtkCList      *clist,
			   guint16	  column)
{
  GtkWidget *clist_parent;

  g_return_if_fail (BST_IS_CLUE_HUNTER (self));
  g_return_if_fail (GTK_IS_CLIST (clist));
  clist_parent = GTK_WIDGET (clist)->parent;
  g_return_if_fail (clist_parent == NULL);
  g_return_if_fail (column < GTK_CLIST (clist)->columns);

  if (self->clist)
    {
      if (clist_parent)
	gtk_container_remove (GTK_CONTAINER (clist_parent), GTK_WIDGET (self->clist));
      if (self->clist)
	g_object_disconnect (self->clist,
			     "any_signal", bst_clue_hunter_clist_click, self,
			     "any_signal", intercept_innermost_signal, self,
			     NULL);
      g_object_unref (self->clist);
    }
  self->clist = clist;
  g_object_ref (self->clist);
  g_object_set (self->clist,
		"visible", TRUE,
		"selection_mode", GTK_SELECTION_EXTENDED,
		"parent", self->scw,
		NULL);
  g_object_connect (self->clist,
		    "swapped_signal::event-after", bst_clue_hunter_clist_click, self,
		    "signal::button_press_event", intercept_innermost_signal, self,
		    "signal::button_release_event", intercept_innermost_signal, self,
		    NULL);
  self->clist_column = column;
}

static void
bst_clue_hunter_popdown (BstClueHunter *self)
{
  g_return_if_fail (BST_IS_CLUE_HUNTER (self));

  if (self->popped_up)
    g_signal_emit (self, clue_hunter_signals[SIGNAL_POPDOWN], 0);
}

void
bst_clue_hunter_popup (BstClueHunter *self)
{
  g_return_if_fail (BST_IS_CLUE_HUNTER (self));

  if (self->popped_up == FALSE &&
      self->entry && GTK_WIDGET_DRAWABLE (self->entry))
    g_signal_emit (self, clue_hunter_signals[SIGNAL_POPUP], 0);
}

void
bst_clue_hunter_select_on (BstClueHunter *self,
			   const gchar   *string)
{
  g_return_if_fail (BST_IS_CLUE_HUNTER (self));
  g_return_if_fail (string != NULL);

  g_signal_emit (self, clue_hunter_signals[SIGNAL_SELECT_ON], 0, string);
}

void
bst_clue_hunter_popup_if_editable (BstClueHunter *self)
{
  if (self->entry && gtk_editable_get_editable (GTK_EDITABLE (self->entry)))
    bst_clue_hunter_popup (self);
}

GtkWidget*
bst_clue_hunter_create_arrow (BstClueHunter *self,
                              gboolean       require_editable)
{
  GtkWidget *button, *arrow;

  g_return_val_if_fail (BST_IS_CLUE_HUNTER (self), NULL);

  button = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
                                      "visible", TRUE,
                                      "can_focus", FALSE,
                                      NULL);
  arrow = (GtkWidget*) g_object_new (GTK_TYPE_ARROW,
                                     "arrow_type", GTK_ARROW_DOWN,
                                     "shadow_type", GTK_SHADOW_ETCHED_IN,
                                     "visible", TRUE,
                                     "parent", button,
                                     NULL);
  (void) arrow;
  g_object_connect (button,
		    "swapped_object_signal::clicked", (require_editable ?
                                                       bst_clue_hunter_popup_if_editable :
                                                       bst_clue_hunter_popup), self,
		    NULL);
  return button;
}

static void
bst_clue_hunter_entry_changed (BstClueHunter *self)
{
  self->completion_tag = FALSE;
  g_free (self->cstring);
  self->cstring = g_strdup (gtk_entry_get_text (self->entry));
  bst_clue_hunter_select_on (self, self->cstring);
}

static gint
bst_clue_hunter_entry_key_press (BstClueHunter *self,
				 GdkEventKey   *event,
				 GtkEntry      *entry)
{
  gboolean handled = FALSE;
  
  if ((event->keyval == GDK_Tab || event->keyval == GDK_ISO_Left_Tab) &&
      !(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)))
    {
      handled = TRUE;
      if (event->type == GDK_KEY_PRESS)
	{
	  gchar *cstring;
	  cstring = bst_clue_hunter_try_complete (self);
	  const char *ostring = gtk_entry_get_text (self->entry);
	  if (!ostring)
	    ostring = "";
	  if (cstring && strcmp (ostring, cstring))
	    {
	      gtk_entry_set_text (self->entry, cstring);
	      self->completion_tag = self->popped_up;
	      gtk_entry_set_position (entry, -1);
	    }
	  else
	    {
	      if (self->completion_tag)
		gtk_widget_activate (GTK_WIDGET (self));
	      else
		self->completion_tag = TRUE;
	    }
	  g_free (cstring);
	}
    }
  return handled;
}

static void
bst_clue_hunter_entry_destroyed (GtkObject *clue_hunter)
{
  gtk_object_destroy (clue_hunter);
}

void
bst_clue_hunter_set_entry (BstClueHunter *self,
			   GtkEntry      *entry)
{
  g_return_if_fail (BST_IS_CLUE_HUNTER (self));
  if (entry)
    {
      g_return_if_fail (GTK_IS_ENTRY (entry));
      g_return_if_fail (bst_clue_hunter_from_entry (entry) == NULL);
    }

  bst_clue_hunter_popdown (self);
  if (self->entry)
    {
      if (!self->entry)
	{
	  g_object_disconnect (self->entry,
			       "any_signal", bst_clue_hunter_entry_changed, self,
			       "any_signal", bst_clue_hunter_entry_key_press, self,
			       "any_signal", bst_clue_hunter_entry_destroyed, self,
			       "any_signal", bst_clue_hunter_poll_refresh, self,
			       NULL);
	}
      g_object_set_data (G_OBJECT (self->entry), "BstClueHunter", NULL);
      g_object_unref (self->entry);
    }
  self->entry = entry;
  if (self->entry)
    {
      g_object_ref (self->entry);
      g_object_set_data (G_OBJECT (self->entry), "BstClueHunter", self);
      g_object_connect (self->entry,
			"swapped_object_signal::destroy", bst_clue_hunter_entry_destroyed, self,
			"swapped_object_signal::changed", bst_clue_hunter_entry_changed, self,
			"swapped_object_signal::key_press_event", bst_clue_hunter_entry_key_press, self,
			"swapped_object_signal::focus_in_event", bst_clue_hunter_poll_refresh, self,
			NULL);
    }
  self->completion_tag = FALSE;
}

gpointer
bst_clue_hunter_from_entry (gpointer entry)
{
  g_return_val_if_fail (GTK_IS_ENTRY (entry), NULL);

  return g_object_get_data (G_OBJECT (entry), "BstClueHunter");
}

void
bst_clue_hunter_add_string (BstClueHunter *self,
			    const gchar   *string)
{
  gchar **text;
  
  g_return_if_fail (BST_IS_CLUE_HUNTER (self));
  g_return_if_fail (string != NULL);

  text = g_new0 (gchar*, self->clist->columns);
  text[self->clist_column] = (gchar*) string;
  gtk_clist_insert (self->clist, 0, text);
  g_free (text);
}

void
bst_clue_hunter_remove_string (BstClueHunter *self,
			       const gchar   *string)
{
  GList *list;
  guint n = 0;
  
  g_return_if_fail (BST_IS_CLUE_HUNTER (self));
  g_return_if_fail (string != NULL);

  for (list = self->clist->row_list; list; list = list->next)
    {
      GtkCListRow *clist_row = (GtkCListRow*) list->data;
      gchar *ctext = clist_row->cell[self->clist_column].u.text;

      if (ctext && strcmp (string, ctext) == 0)
	{
	  gtk_clist_remove (self->clist, n);
	  break;
	}
      n++;
    }
}

void
bst_clue_hunter_remove_matches (BstClueHunter *self,
				const gchar   *pattern)
{
  GPatternSpec *pspec;
  GList *list;
  guint n = 0;
  
  g_return_if_fail (BST_IS_CLUE_HUNTER (self));
  if (!pattern)
    pattern = "*";

  pspec = g_pattern_spec_new (pattern);

  gtk_clist_freeze (self->clist);
  list = self->clist->row_list;
  while (list)
    {
      GtkCListRow *clist_row = (GtkCListRow*) list->data;
      gchar *ctext = clist_row->cell[self->clist_column].u.text;

      list = list->next;
      if (!ctext || g_pattern_match_string (pspec, ctext))
	gtk_clist_remove (self->clist, n);
      else
	n++;
    }
  g_pattern_spec_free (pspec);
  gtk_clist_thaw (self->clist);
}

static gchar*
string_list_intersect (guint   max_len,
		       GSList *strings)
{
  gchar *completion;
  guint l = 0;
  
  if (!strings || !max_len)
    return NULL;
  
  completion = g_new (gchar, max_len + 1);
  
  while (l < max_len)
    {
      gchar *s = (gchar*) strings->data;
      GSList *slist;
      
      s += l;
      completion[l] = *s;
      
      for (slist = strings->next; slist; slist = slist->next)
	{
	  s = (char*) slist->data;
	  s += l;
	  if (completion[l] != *s)
	    completion[l] = 0;
	}
      if (!completion[l])
	break;
      l++;
    }
  completion[l] = 0;
  
  return g_renew (gchar, completion, completion[0] ? l + 1 : 0);
}

gchar*
bst_clue_hunter_try_complete (BstClueHunter *self)
{
  GList *list;
  gchar *completion;
  GSList *strings = NULL;
  guint max_len = 0, n = 0;
  
  g_return_val_if_fail (BST_IS_CLUE_HUNTER (self), NULL);

  for (list = self->clist->row_list; list; list = list->next)
    {
      GtkCListRow *clist_row = (GtkCListRow*) list->data;
      
      if (g_list_find (self->clist->selection, GINT_TO_POINTER (n)))
	{
	  gchar *ctext = clist_row->cell[self->clist_column].u.text;
	  guint l = ctext ? strlen (ctext) : 0;

	  max_len = MAX (max_len, l);
	  if (ctext)
	    strings = g_slist_prepend (strings, ctext);
	}
      n++;
    }

  completion = string_list_intersect (max_len, strings);
  g_slist_free (strings);
  
  return completion;
}

void
bst_clue_hunter_poll_refresh (BstClueHunter *self)
{
  g_return_if_fail (BST_IS_CLUE_HUNTER (self));

  if (self->entry && GTK_WIDGET_HAS_FOCUS (self->entry))
    g_signal_emit (self, clue_hunter_signals[SIGNAL_POLL_REFRESH], 0);
}

static void
bst_clue_hunter_do_activate (BstClueHunter *self)
{
  if (self->popped_up)
    bst_clue_hunter_popdown (self);
  else if (self->entry)
    bst_clue_hunter_popup_if_editable (self);
}

static void
bst_clue_hunter_do_popup (BstClueHunter *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkWidget *wlist = GTK_WIDGET (self->clist);
  GtkWidget *wentry = GTK_WIDGET (self->entry);
  GtkScrolledWindow *scw = GTK_SCROLLED_WINDOW (self->scw);
  gint sheight = gdk_screen_height ();
  gint swidth = gdk_screen_width ();
  gint x = 0, y = 0, width = 0, height = 0;

  g_return_if_fail (!self->popped_up);

  gtk_widget_grab_focus (GTK_WIDGET (self->entry));
  if (!self->cstring)
    self->cstring = g_strdup ("");

  /* work around clist and scrolled window resizing misbehaviour */
  gtk_clist_columns_autosize (self->clist);
  gtk_widget_queue_resize (wlist);	/* work around gtk+ optimizations */
  gtk_widget_queue_resize (self->scw);	/* work around gtk+ optimizations */
  gtk_widget_size_request (self->scw, NULL);
  gtk_widget_set_size_request (self->scw,
                               wlist->requisition.width + 2 * wlist->style->xthickness
                               + 3 /* gtkscrolledwindow.c hardcoded spacing */
                               + scw->vscrollbar->requisition.width,
                               wlist->requisition.height + 2 * wlist->style->ythickness
                               + 3 /* gtkscrolledwindow.c hardcoded spacing */
                               + scw->hscrollbar->requisition.height);
  gtk_widget_size_request (widget, NULL);

  if (self->align_widget && GTK_WIDGET_DRAWABLE (self->align_widget))
    {
      gdk_window_get_origin (self->align_widget->window, &x, &y);
      if (GTK_WIDGET_NO_WINDOW (self->align_widget))
        {
          x += self->align_widget->allocation.x;
          y += self->align_widget->allocation.y;
        }
      width = self->align_widget->allocation.width;
      height = self->align_widget->allocation.height;
    }
  else
    {
      gdk_window_get_origin (wentry->window, &x, &y);
      width = wentry->allocation.width;
      height = wentry->allocation.height;
    }
  height = MIN (height, sheight);
  if (y < 0)
    {
      height = MAX (0, height + y);
      y = 0;
    }
  else if (y > sheight)
    {
      height = 0;
      y = sheight;
    }
  else if (y + height > sheight)
    height = sheight - y;
  width = MIN (width, swidth);
  x = CLAMP (x, 0, swidth);
  if (widget->requisition.height > sheight - (y + height))
    {
      if (y + height / 2 > sheight / 2)
	{
	  height = MIN (y, widget->requisition.height);
	  y -= height;
	}
      else
	{
	  y += height;
	  height = sheight - y;
	}
    }
  else
    {
      y += height;
      height = -1;
    }
  width = MIN (swidth - x, width);
  gtk_widget_set_size_request (widget, width, height);

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move (widget->window, x, y);
  gtk_window_move (GTK_WINDOW (widget), x, y);

  gtk_widget_grab_focus (wlist);

  gtk_widget_show (widget);
  if (gxk_grab_pointer_and_keyboard (widget->window, TRUE,
                                     GDK_POINTER_MOTION_HINT_MASK |
                                     GDK_BUTTON1_MOTION_MASK |
                                     GDK_BUTTON2_MOTION_MASK |
                                     GDK_BUTTON3_MOTION_MASK |
                                     GDK_BUTTON_PRESS_MASK |
                                     GDK_BUTTON_RELEASE_MASK,
                                     NULL, NULL, GDK_CURRENT_TIME))
    {
      gtk_grab_add (widget);
      self->popped_up = TRUE;
      self->completion_tag = FALSE;
      bst_clue_hunter_select_on (self, self->cstring);
    }
  else
    gtk_widget_hide (widget);
}

static void
bst_clue_hunter_do_popdown (BstClueHunter *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  g_return_if_fail (self->popped_up);

  gtk_widget_hide (widget);
  gdk_flush ();	/* remove pointer instantly */
  gtk_grab_remove (widget);

  self->popped_up = FALSE;
  self->completion_tag = FALSE;
}

static void
bst_clue_hunter_add_history (BstClueHunter *self,
			     const gchar   *string)
{
  GList *list;

  for (list = self->clist->row_list; list; list = list->next)
    {
      GtkCListRow *clist_row = (GtkCListRow*) list->data;
      gchar *ctext = clist_row->cell[self->clist_column].u.text;

      if (ctext && strcmp (string, ctext) == 0)
	return;
    }
  bst_clue_hunter_add_string (self, string);
}

static void
bst_clue_hunter_do_select_on (BstClueHunter *self,
			      const gchar   *cstring)
{
  GList *list;
  guint len = strlen (cstring);

  gtk_clist_freeze (self->clist);
  gtk_clist_undo_selection (self->clist);
  gtk_clist_unselect_all (self->clist);

  if (len && self->pattern_matching)
    {
      guint n = 0;
      gboolean check_visibility = TRUE;
      gchar *pattern = g_strconcat (cstring, "*", NULL);
      GPatternSpec *pspec = g_pattern_spec_new (pattern);

      g_free (pattern);
      for (list = self->clist->row_list; list; list = list->next)
	{
	  GtkCListRow *clist_row = (GtkCListRow*) list->data;
	  gchar *ctext = clist_row->cell[self->clist_column].u.text;

	  if (ctext && g_pattern_match_string (pspec, ctext))
	    {
	      gtk_clist_select_row (self->clist, n, 0);
	      if (check_visibility &&
		  gtk_clist_row_is_visible (self->clist, n) != GTK_VISIBILITY_FULL)
		gtk_clist_moveto (self->clist, n, -1, 0.5, 0);
	      check_visibility = FALSE;
	    }
	  n++;
	}
      g_pattern_spec_free (pspec);
    }
  else if (len)
    {
      guint n = 0;
      gboolean check_visibility = TRUE;
      
      for (list = self->clist->row_list; list; list = list->next)
	{
	  GtkCListRow *clist_row = (GtkCListRow*) list->data;
	  gchar *ctext = clist_row->cell[self->clist_column].u.text;
	  if (ctext && strncmp (cstring, ctext, len) == 0)
	    {
	      gtk_clist_select_row (self->clist, n, 0);
	      if (check_visibility &&
		  gtk_clist_row_is_visible (self->clist, n) != GTK_VISIBILITY_FULL)
		gtk_clist_moveto (self->clist, n, -1, 0.5, 0);
	      check_visibility = FALSE;
	    }
	  n++;
	}
    }
  gtk_clist_thaw (self->clist);
}

static gint
bst_clue_hunter_event (GtkWidget *widget,
		       GdkEvent  *event)
{
  BstClueHunter *self = BST_CLUE_HUNTER (widget);
  gboolean handled = FALSE;

  switch (event->type)
    {
      GtkWidget *ev_widget;
    case GDK_KEY_PRESS:
      if (event->key.keyval == GDK_Escape)
	{
	  handled = TRUE;
	  bst_clue_hunter_popdown (self);
	}
      else if (event->key.keyval == GDK_Return ||
	       event->key.keyval == GDK_KP_Enter)
	{
          char *string;
	  handled = TRUE;
	  string = bst_clue_hunter_try_complete (self);
	  if (string)
	    {
	      if (string[0])
		gtk_entry_set_text (self->entry, string);
	      g_free (string);
	    }
	  else if (self->keep_history)
	    {
	      const char *cstring = gtk_entry_get_text (self->entry);
	      if (cstring && cstring[0])
		bst_clue_hunter_add_history (self, cstring);
	    }
	  bst_clue_hunter_popdown (self);
	  /* if (string) */
	  gtk_widget_activate (GTK_WIDGET (self->entry));
	}
      else
	handled = gtk_widget_event (GTK_WIDGET (self->entry), event);
      break;
    case GDK_KEY_RELEASE:
      if (event->key.keyval == GDK_Escape ||
	  event->key.keyval == GDK_Return ||
	  event->key.keyval == GDK_KP_Enter)
	handled = TRUE;
      else
	handled = gtk_widget_event (GTK_WIDGET (self->entry), event);
      break;
      
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      if (event->button.window == self->clist->clist_window)
	{
	  handled = TRUE;
	  if (event->button.button == 1)
	    {
	      gint row, on_row = gtk_clist_get_selection_info (self->clist,
							       event->button.x, event->button.y,
							       &row, NULL);
	      if (event->type == GDK_BUTTON_RELEASE &&
		  on_row && self->clist->selection &&
		  GPOINTER_TO_UINT (self->clist->selection->data) == row)
		{
		  gchar *string = bst_clue_hunter_try_complete (self);
		  gtk_entry_set_text (GTK_ENTRY (self->entry), string ? string : "");
		  g_free (string);
		  bst_clue_hunter_popdown (self);
		  gtk_widget_activate (GTK_WIDGET (self->entry));
		}
	      else if (on_row && event->type == GDK_BUTTON_PRESS)
		{
		  gtk_clist_unselect_all (self->clist);
		  gtk_clist_select_row (self->clist, row, self->clist_column);
		}
	    }
	  break;
	}
      ev_widget = gtk_get_event_widget (event);
      if (ev_widget == widget && event->type == GDK_BUTTON_PRESS)
	{
	  gint w, h;
	  
	  gdk_window_get_size (widget->window, &w, &h);
	  if (event->button.x > w || event->button.y > h ||
	      event->button.x < 0 || event->button.y < 0)
	    ev_widget = NULL;
	}
      else if (ev_widget)
	while (ev_widget->parent)
	  ev_widget = ev_widget->parent;
      if (ev_widget != widget)
	{
	  bst_clue_hunter_popdown (self);
	  handled = TRUE;
	}
      break;
    case GDK_DELETE:
      bst_clue_hunter_popdown (self);
      handled = TRUE;
      break;
    default:
      break;
    }
  
  return handled;
}
