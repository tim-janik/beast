/* BEAST - Bedevilled Audio System
 * Copyright (C) 1999, 2000, 2001 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstservermonitor.h"


/* --- prototypes --- */
static void	bst_server_monitor_class_init	(BstServerMonitorClass	*class);
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
bst_server_monitor_class_init (BstServerMonitorClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = bst_server_monitor_finalize;
}

static void
bst_server_monitor_init (BstServerMonitor *smon)
{
  smon->server = BSW_SERVER;
  smon->param_view = gtk_widget_new (BST_TYPE_PARAM_VIEW, NULL);
  bst_param_view_set_mask (BST_PARAM_VIEW (smon->param_view), BSE_TYPE_SERVER, 0, NULL, NULL);
  gtk_widget_show (smon->param_view);
  gtk_container_add (GTK_CONTAINER (smon), smon->param_view);
  bst_param_view_set_object (BST_PARAM_VIEW (smon->param_view), bse_object_from_id (smon->server));
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
  
  bst_param_view_update (BST_PARAM_VIEW (smon->param_view));
}
