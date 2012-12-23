// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include	"bsteffectview.hh"

#include	"bstparamview.hh"



/* --- prototypes --- */
static void	bst_effect_view_class_init	(BstEffectViewClass	*klass);
static void	bst_effect_view_init		(BstEffectView		*effect_view);
static void     bst_effect_view_destroy         (GtkObject              *object);
static void     bst_effect_view_finalize        (GObject                *object);
static void	bst_effect_view_note_changed    (BstEffectView		*effect_view,
						 guint                   channel,
						 guint                   row,
						 BsePattern             *pattern);
static void	update_effect_lists		(BstEffectView		*effect_view);
static void	add_effect			(BstEffectView		*effect_view);
static void	remove_effect			(BstEffectView		*effect_view);
static void	alist_selection_changed		(BstEffectView		*effect_view);
static void	plist_selection_changed		(BstEffectView		*effect_view);


/* --- static variables --- */
static gpointer		   parent_class = NULL;
static BstEffectViewClass *bst_effect_view_class = NULL;


/* --- functions --- */
GtkType
bst_effect_view_get_type (void)
{
  static GtkType effect_view_type = 0;
  
  if (!effect_view_type)
    {
      GtkTypeInfo effect_view_info =
      {
	"BstEffectView",
	sizeof (BstEffectView),
	sizeof (BstEffectViewClass),
	(GtkClassInitFunc) bst_effect_view_class_init,
	(GtkObjectInitFunc) bst_effect_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      effect_view_type = gtk_type_unique (GTK_TYPE_ALIGNMENT, &effect_view_info);
    }
  
  return effect_view_type;
}

static void
bst_effect_view_class_init (BstEffectViewClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  
  bst_effect_view_class = klass;
  parent_class = gtk_type_class (GTK_TYPE_ALIGNMENT);
  
  G_OBJECT_CLASS (klass)->finalize = bst_effect_view_finalize;

  object_class->destroy = bst_effect_view_destroy;
  
  klass->default_param_view_height = 60;
}

static void
bst_effect_view_init (BstEffectView *effect_view)
{
  GtkWidget *alist_box, *pbox, *bbox, *sc_win;
  GtkCList *clist;
  
  /* setup containers */
  effect_view->paned = gtk_widget_new (GTK_TYPE_HPANED,
				       "visible", TRUE,
				       "border_width", 5,
				       "parent", effect_view,
				       NULL);
  gtk_widget_ref (effect_view->paned);
  pbox = gtk_widget_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "spacing", 5,
			 "border_width", 0,
			 NULL);
  gtk_paned_pack2 (GTK_PANED (effect_view->paned), pbox, TRUE, TRUE);
  alist_box = gtk_widget_new (GTK_TYPE_HBOX,
			      "visible", TRUE,
			      "spacing", 5,
			      "border_width", 0,
			      NULL);
  gtk_box_pack_start (GTK_BOX (pbox), alist_box, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (pbox), gtk_widget_new (GTK_TYPE_HSEPARATOR, "visible", TRUE, NULL), FALSE, FALSE, 0);
  bbox = gtk_widget_new (GTK_TYPE_VBOX,
			 "visible", TRUE,
			 "homogeneous", TRUE,
			 "spacing", 5,
			 "border_width", 0,
			 NULL);
  gtk_box_pack_start (GTK_BOX (alist_box),
		      gtk_widget_new (GTK_TYPE_ALIGNMENT, /* don't want vexpand */
				      "visible", TRUE,
				      "child", bbox,
				      "xscale", 0.0,
				      "yscale", 0.0,
				      "xalign", 0.0,
				      "yalign", 0.5,
				      NULL),
		      FALSE, TRUE, 0);
  
  /* setup lists */
  sc_win = gtk_widget_new (GTK_TYPE_SCROLLED_WINDOW,
			   "visible", TRUE,
			   "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
			   "vscrollbar_policy", GTK_POLICY_ALWAYS,
			   NULL);
  gtk_box_pack_start (GTK_BOX (alist_box), sc_win, FALSE, TRUE, 0);
  effect_view->clist_aeffects = g_object_connect (gtk_widget_new (GTK_TYPE_CLIST,
								  "visible", TRUE,
								  "can_focus", FALSE,
								  "n_columns", 1,
								  "selection_mode", GTK_SELECTION_BROWSE,
								  "border_width", 0,
								  "height_request", 60,
								  "width_request", 150,
								  "shadow_type", GTK_SHADOW_ETCHED_OUT,
								  "parent", sc_win,
								  NULL),
						  "swapped_signal::select_row", alist_selection_changed, effect_view,
						  "signal_after::size_allocate", gtk_clist_moveto_selection, NULL,
						  "signal_after::map", gtk_clist_moveto_selection, NULL,
						  NULL);
  clist = GTK_CLIST (effect_view->clist_aeffects);
  gtk_widget_ref (effect_view->clist_aeffects);
  gtk_clist_column_titles_hide (clist);
  gtk_clist_column_titles_passive (clist);
  gtk_clist_set_column_title (clist, 0, "Available Effects");
  gtk_clist_set_column_auto_resize (clist, 0, TRUE);
  sc_win = gtk_widget_new (GTK_TYPE_SCROLLED_WINDOW,
			   "visible", TRUE,
			   "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
			   "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
			   NULL);
  effect_view->clist_peffects = g_object_connect (gtk_widget_new (GTK_TYPE_CLIST,
								  "visible", TRUE,
								  "n_columns", 1,
								  "selection_mode", GTK_SELECTION_BROWSE,
								  "border_width", 0,
								  "width_request", 100,
								  "parent", sc_win,
								  NULL),
						  "swapped_signal::select_row", plist_selection_changed, effect_view,
						  "signal_after::size_allocate", gtk_clist_moveto_selection, NULL,
						  "signal_after::map", gtk_clist_moveto_selection, NULL,
						  NULL);
  gtk_paned_pack1 (GTK_PANED (effect_view->paned), sc_win, FALSE, FALSE);
  clist = GTK_CLIST (effect_view->clist_peffects);
  gtk_widget_ref (effect_view->clist_peffects);
  gtk_clist_set_column_title (clist, 0, "Note Effects");
  gtk_clist_set_column_resizeable (clist, 0, FALSE);
  gtk_clist_set_column_auto_resize (clist, 0, TRUE);
  gtk_clist_column_titles_show (clist);
  gtk_clist_column_titles_passive (clist);
  
  /* setup param view */
  effect_view->param_view =  bst_param_view_new (0);
  gtk_widget_set (effect_view->param_view,
		  "parent", pbox,
		  (BST_EFFECT_VIEW_GET_CLASS (effect_view)->default_param_view_height > 0 ?
		   "height_request" : NULL), BST_EFFECT_VIEW_GET_CLASS (effect_view)->default_param_view_height,
		  NULL);
  gtk_widget_ref (effect_view->param_view);
  bst_param_view_set_mask (BST_PARAM_VIEW (effect_view->param_view), BSE_TYPE_EFFECT, 0, NULL, NULL);
  
  /* setup buttons */
  effect_view->add_button = g_object_connect (gtk_widget_new (GTK_TYPE_BUTTON,
							      "visible", TRUE,
							      "label", "<< Add  ",
							      "can_focus", FALSE,
							      "parent", bbox,
							      NULL),
					      "swapped_signal::clicked", add_effect, effect_view,
					      NULL);
  gtk_widget_ref (effect_view->add_button);
  effect_view->remove_button = g_object_connect (gtk_widget_new (GTK_TYPE_BUTTON,
								 "visible", TRUE,
								 "label", ">> Remove",
								 "can_focus", FALSE,
								 "parent", bbox,
								 NULL),
						 "swapped_signal::clicked", remove_effect, effect_view,
						 NULL);
  gtk_widget_ref (effect_view->remove_button);
  
  effect_view->pattern = NULL;
  effect_view->channel = 0;
  effect_view->row = 0;
}

static void
bst_effect_view_destroy (GtkObject *object)
{
  BstEffectView *effect_view = BST_EFFECT_VIEW (object);
  
  bst_effect_view_set_note (effect_view, NULL, 0, 0);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_effect_view_finalize (GObject *object)
{
  BstEffectView *effect_view = BST_EFFECT_VIEW (object);

  bst_effect_view_set_note (effect_view, NULL, 0, 0);

  gtk_widget_unref (effect_view->paned);
  gtk_widget_unref (effect_view->clist_aeffects);
  gtk_widget_unref (effect_view->clist_peffects);
  gtk_widget_unref (effect_view->param_view);
  gtk_widget_unref (effect_view->add_button);
  gtk_widget_unref (effect_view->remove_button);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget*
bst_effect_view_new (BseSong *song)
{
  GtkWidget *effect_view;
  
  g_return_val_if_fail (BSE_IS_SONG (song), NULL);
  
  effect_view = gtk_widget_new (BST_TYPE_EFFECT_VIEW, NULL);
  
  return effect_view;
}

void
bst_effect_view_set_note (BstEffectView *effect_view,
			  BsePattern    *pattern,
			  guint          channel,
			  guint          row)
{
  g_return_if_fail (BST_IS_EFFECT_VIEW (effect_view));
  if (pattern)
    {
      g_return_if_fail (BSE_IS_PATTERN (pattern));
      g_return_if_fail (channel < BSE_PATTERN_N_CHANNELS (pattern));
      g_return_if_fail (row < BSE_PATTERN_N_ROWS (pattern));
    }
  else
    {
      channel = 0;
      row = 0;
    }

  if (effect_view->pattern)
    {
      g_object_disconnect (effect_view->pattern,
			   "any_signal", bst_effect_view_note_changed, effect_view,
			   NULL);
      bse_object_unref (effect_view->pattern);
    }
  effect_view->pattern = pattern;
  effect_view->channel = channel;
  effect_view->row = row;
  if (effect_view->pattern)
    {
      bse_object_ref (effect_view->pattern);
      g_object_connect (effect_view->pattern,
			"swapped_signal::note_changed", bst_effect_view_note_changed, effect_view,
			NULL);
    }
  update_effect_lists (effect_view);
}

static void
bst_effect_view_note_changed (BstEffectView *effect_view,
			      guint          channel,
			      guint          row,
			      BsePattern    *pattern)
{
  g_return_if_fail (effect_view->pattern == pattern);

  if (channel == effect_view->channel && row == effect_view->row)
    update_effect_lists (effect_view);
}

static void
update_effect_lists (BstEffectView *effect_view)
{
  GtkCList *aclist = GTK_CLIST (effect_view->clist_aeffects);
  GtkCList *pclist = GTK_CLIST (effect_view->clist_peffects);

  if (aclist && pclist)
    {
      guint i, n_effects = 0, ptype = ~0;
      
      if (!aclist->selection)
	{
	  BseCategory *cats;
	  guint n_cats;

	  gtk_clist_freeze (aclist);
	  gtk_clist_clear (aclist);
	  cats = bse_categories_match_typed ("/Effect/""*", BSE_TYPE_EFFECT, &n_cats);
	  for (i = 0; i < n_cats; i++)
	    {
	      gchar *name = cats[i].category + cats[i].mindex + 1;
	      gint clist_row = gtk_clist_insert (aclist, 0, &name);
	      
	      gtk_clist_set_row_data (aclist, clist_row, (gpointer) cats[i].type);
	    }
	  g_free (cats);
	  gtk_clist_select_row (aclist, 0, -1);
	  gtk_clist_thaw (aclist);
	}

      ptype = (GType) gtk_clist_get_selection_data (pclist, 0);
      gtk_clist_freeze (pclist);
      gtk_clist_clear (pclist);
      if (effect_view->pattern)
	n_effects = bse_pattern_note_get_n_effects (effect_view->pattern,
						    effect_view->channel,
						    effect_view->row);
      for (i = 0; i < n_effects; i++)
	{
	  BseCategory *cats;
	  gchar *name = NULL;
	  gint clist_row;
	  guint n_cats;
	  BseEffect *effect = bse_pattern_note_get_effect (effect_view->pattern,
							   effect_view->channel,
							   effect_view->row,
							   i);

	  cats = bse_categories_from_type (BSE_OBJECT_TYPE (effect), &n_cats);
	  name = cats ? cats[0].category + cats[0].mindex + 1 : g_type_name (BSE_OBJECT_TYPE (effect));
	  g_free (cats);

	  clist_row = gtk_clist_insert (pclist, 0, &name);
	  gtk_clist_set_row_data (pclist, clist_row, GUINT_TO_POINTER (BSE_OBJECT_TYPE (effect)));
	  if (BSE_OBJECT_TYPE (effect) == ptype)
	    {
	      ptype = 0;
	      gtk_clist_select_row (pclist, clist_row, -1);
	    }
	}
      if (ptype)
	gtk_clist_select_row (pclist, 0, -1);
      gtk_clist_thaw (pclist);
    }

  alist_selection_changed (effect_view);
  plist_selection_changed (effect_view);
}

static void
add_effect (BstEffectView *effect_view)
{
  GtkCList *aclist = GTK_CLIST (effect_view->clist_aeffects);

  if (aclist && aclist->selection && effect_view->pattern)
    bse_pattern_note_actuate_effect (effect_view->pattern, effect_view->channel, effect_view->row,
				     GPOINTER_TO_UINT (gtk_clist_get_selection_data (aclist, 0)));
}

static void
remove_effect (BstEffectView *effect_view)
{
  GtkCList *pclist = GTK_CLIST (effect_view->clist_peffects);

  if (pclist && pclist->selection && effect_view->pattern)
    bse_pattern_note_drop_effect (effect_view->pattern, effect_view->channel, effect_view->row,
				  GPOINTER_TO_UINT (gtk_clist_get_selection_data (pclist, 0)));
}

static void
alist_selection_changed (BstEffectView *effect_view)
{
  GtkCList *aclist = GTK_CLIST (effect_view->clist_aeffects);

  if (effect_view->param_view && effect_view->pattern)
    {
      gpointer data = gtk_clist_get_selection_data (aclist, 0);
      BseEffect *effect = data ? bse_pattern_note_find_effect (effect_view->pattern,
							       effect_view->channel,
							       effect_view->row,
							       GPOINTER_TO_UINT (data)) : NULL;

      gtk_widget_set_sensitive (effect_view->add_button, effect == NULL);
    }
  gtk_clist_moveto_selection (aclist);
}

static void
plist_selection_changed (BstEffectView *effect_view)
{
  GtkCList *pclist = GTK_CLIST (effect_view->clist_peffects);

  if (effect_view->param_view && effect_view->pattern)
    {
      gpointer data = gtk_clist_get_selection_data (pclist, 0);
      BseEffect *effect = data ? bse_pattern_note_find_effect (effect_view->pattern,
							       effect_view->channel,
							       effect_view->row,
							       GPOINTER_TO_UINT (data)) : NULL;

      bst_param_view_set_object (BST_PARAM_VIEW (effect_view->param_view),
				 effect ? BSE_OBJECT_ID (effect) : 0);
    }
  gtk_clist_moveto_selection (pclist);
  gtk_widget_set_sensitive (effect_view->remove_button, pclist->selection != NULL);
}
