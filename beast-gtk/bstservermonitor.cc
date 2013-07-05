// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstservermonitor.hh"


/* --- prototypes --- */
static void	bst_server_monitor_class_init	(BstServerMonitorClass	*klass);
static void	bst_server_monitor_init		(BstServerMonitor	*smon);
static void	bst_server_monitor_finalize	(GObject		*object);


/* --- static variables --- */
static gpointer		      parent_class = NULL;


/* --- functions --- */
GtkType
bst_server_monitor_get_type (void)
{
  static GtkType server_monitor_type = 0;

  if (!server_monitor_type)
    {
      GtkTypeInfo server_monitor_info =
      {
	"BstServerMonitor",
	sizeof (BstServerMonitor),
	sizeof (BstServerMonitorClass),
	(GtkClassInitFunc) bst_server_monitor_class_init,
	(GtkObjectInitFunc) bst_server_monitor_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      server_monitor_type = gtk_type_unique (GTK_TYPE_VBOX, &server_monitor_info);
    }

  return server_monitor_type;
}

static void
bst_server_monitor_class_init (BstServerMonitorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = bst_server_monitor_finalize;
}

static void
bst_server_monitor_init (BstServerMonitor *smon)
{
  smon->server = BSE_SERVER;
  smon->param_view = gtk_widget_new (BST_TYPE_PARAM_VIEW, NULL);
  bst_param_view_set_mask (BST_PARAM_VIEW (smon->param_view), "BseServer", 0, NULL, NULL);
  gtk_widget_show (smon->param_view);
  gtk_container_add (GTK_CONTAINER (smon), smon->param_view);
  bst_param_view_set_item (BST_PARAM_VIEW (smon->param_view), smon->server);
}

static void
bst_server_monitor_finalize (GObject *object)
{
  /* BstServerMonitor *smon = BST_SERVER_MONITOR (object); */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
bst_server_monitor_rebuild (BstServerMonitor *smon)
{
  g_return_if_fail (BST_IS_SERVER_MONITOR (smon));

  bst_server_monitor_update (smon);
}

void
bst_server_monitor_update (BstServerMonitor *smon)
{
  g_return_if_fail (BST_IS_SERVER_MONITOR (smon));

}
