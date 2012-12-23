// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstrackview.hh"
#include "bstrackitem.hh"
#include "gxk/gxkrackeditor.hh"

/* --- prototypes --- */
static void     rack_view_class_init         (BstRackViewClass *klass);
static void     rack_view_init               (BstRackView      *self);
static void     rack_view_destroy            (GtkObject        *object);
static void     rack_view_finalize           (GObject          *object);
static gboolean rack_view_button_press_event (BstRackView      *self,
                                              GdkEventButton   *event);
static void     rack_view_popup_action       (gpointer          user_data,
                                              gulong            action_id);
static gboolean rack_view_popup_action_check (gpointer          data,
                                              gulong            action,
                                              guint64           action_stamp);



/* --- rack view actions --- */
enum {
  ACTION_ADD_LABEL,
  ACTION_TOGGLE_EDIT
};
static const GxkStockAction rack_view_popup_actions[] = {
  { N_("_Edit"),        NULL,           NULL,
    ACTION_TOGGLE_EDIT, BST_STOCK_MOUSE_TOOL,
  },
  { N_("Label"),        NULL,           NULL,
    ACTION_ADD_LABEL,   BST_STOCK_NO_ICON,
  },
};


/* --- static variables --- */
static gpointer          parent_class = NULL;


/* --- functions --- */
GType
bst_rack_view_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (BstRackViewClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) rack_view_class_init,
        NULL,   /* class_finalize */
        NULL,   /* class_data */
        sizeof (BstRackView),
        0,      /* n_preallocs */
        (GInstanceInitFunc) rack_view_init,
      };
      type = g_type_register_static (GTK_TYPE_VBOX, "BstRackView", &type_info, GTypeFlags (0));
    }
  return type;
}

static void
rack_view_class_init (BstRackViewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  gobject_class->finalize = rack_view_finalize;

  object_class->destroy = rack_view_destroy;
}

static void
toggle_edit_mode (GtkToggleButton *tb,
                  BstRackView     *self)
{
  if (self->rack_table)
    {
      gxk_rack_table_set_edit_mode (self->rack_table, tb->active);
      gxk_widget_update_actions_upwards (self->rack_table);
    }
}

static void
rack_view_init (BstRackView *self)
{
  GxkRadget *radget = gxk_radget_complete (GTK_WIDGET (self), "beast", "rack-view", NULL);
  GtkWidget *toggle;
  self->rack_table = (GxkRackTable*) gxk_radget_find (radget, "rack-table");
  gtk_table_resize (GTK_TABLE (self->rack_table), 20, 80);
  gxk_nullify_in_object (self, &self->rack_table);
  g_object_connect (self->rack_table, "swapped_signal_after::button_press_event", rack_view_button_press_event, self, NULL);
  g_object_connect (gxk_radget_find (radget, "rack-table-event-box"),
                    "swapped_signal_after::button_press_event", rack_view_button_press_event, self, NULL);
  self->item = 0;
  gtk_widget_show (GTK_WIDGET (self));
  toggle = (GtkWidget*) gxk_radget_find (radget, "edit-toggle");
  g_object_connect (toggle, "signal::clicked", toggle_edit_mode, self, NULL);
  /* publish popup menu actions */
  gxk_widget_publish_actions (self, "rack-view-popup-actions",
                              G_N_ELEMENTS (rack_view_popup_actions), rack_view_popup_actions,
                              NULL, rack_view_popup_action_check, rack_view_popup_action);
}

static void
rack_view_destroy (GtkObject *object)
{
  BstRackView *self = BST_RACK_VIEW (object);
  
  bst_rack_view_set_item (self, 0);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
rack_view_finalize (GObject *object)
{
  BstRackView *self = BST_RACK_VIEW (object);
  
  bst_rack_view_set_item (self, 0);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget*
bst_rack_view_new (SfiProxy item)
{
  GtkWidget *self = (GtkWidget*) g_object_new (BST_TYPE_RACK_VIEW, NULL);
  if (item)
    {
      g_return_val_if_fail (BSE_IS_ITEM (item), NULL);
      bst_rack_view_set_item (BST_RACK_VIEW (self), item);
    }
  return self;
}

static void
rack_view_reset_item (BstRackView *self)
{
  bst_rack_view_set_item (self, 0);
}

static void
rack_view_parasites_added (BstRackView *self)
{
  BseStringSeq *sseq = bse_item_list_parasites (self->item, "/beast-rack-view/");
  guint i;
  for (i = 0; i < sseq->n_strings; i++)
    {
      const gchar *path = g_intern_string (sseq->strings[i]);
      GtkWidget *item = bst_rack_item_new (self->item, path);
      if (item)
        gtk_container_add (GTK_CONTAINER (self->rack_table), item);
    }
}

void
bst_rack_view_set_item (BstRackView *self,
                        SfiProxy     item)
{
  g_return_if_fail (BST_IS_RACK_VIEW (self));
  if (item)
    g_return_if_fail (BSE_IS_ITEM (item));
  
  if (item == self->item)
    return;
  
  if (self->item)
    {
      bse_proxy_disconnect (self->item,
                            "any_signal", rack_view_reset_item, self,
                            "any_signal", rack_view_parasites_added, self,
                            NULL);
      self->item = 0;
    }
  self->item = item;
  if (self->item)
    bse_proxy_connect (self->item,
                       "swapped_signal::release", rack_view_reset_item, self,
                       "swapped_signal::parasites-added::/beast-rack-view/", rack_view_parasites_added, self,
                       NULL);
  
  bst_rack_view_rebuild (self);
}

void
bst_rack_view_rebuild (BstRackView *self)
{
  GtkWidget *toggle;
  g_return_if_fail (BST_IS_RACK_VIEW (self));
  
  gtk_container_foreach (GTK_CONTAINER (self->rack_table), (GtkCallback) gtk_widget_destroy, NULL);
  toggle = (GtkWidget*) gxk_radget_find (self, "edit-toggle");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), FALSE);
  if (!self->item)
    return;
  rack_view_parasites_added (self);
}

static gboolean
rack_view_button_press_event (BstRackView     *self,
                              GdkEventButton  *event)
{
  if (event->button == 3)
    {
      gxk_menu_popup ((GtkMenu*) gxk_radget_find (self, "rack-view-popup"),
                      event->x_root, event->y_root,
                      event->button, event->time);
      return TRUE;
    }
  return FALSE;
}

static void
rack_view_popup_action (gpointer                user_data,
                        gulong                  action_id)
{
  BstRackView *self = BST_RACK_VIEW (user_data);
  switch (action_id)
    {
      SfiRec *rec;
    case ACTION_TOGGLE_EDIT:
      gtk_widget_activate ((GtkWidget*) gxk_radget_find (self, "edit-toggle"));
      break;
    case ACTION_ADD_LABEL:
      rec = sfi_rec_new ();
      sfi_rec_set_string (rec, "type", "label");
      sfi_rec_set_string (rec, "text", "Hallo");
      bse_item_add_parasite (self->item, "/beast-rack-view/", rec);
      sfi_rec_unref (rec);
      break;
    }
}

static gboolean
rack_view_popup_action_check (gpointer data,
                              gulong   action,
                              guint64  action_stamp)
{
  BstRackView *self = BST_RACK_VIEW (data);
  switch (action)
    {
    case ACTION_TOGGLE_EDIT:
      return TRUE;
    case ACTION_ADD_LABEL:
      return self->rack_table->editor != NULL;
    }
  return FALSE;
}
