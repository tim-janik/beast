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
#ifndef __BST_SERVER_MONITOR_H__
#define __BST_SERVER_MONITOR_H__

#include	"bstparamview.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_SERVER_MONITOR		   (bst_server_monitor_get_type ())
#define	BST_SERVER_MONITOR(object)	   (GTK_CHECK_CAST ((object), BST_TYPE_SERVER_MONITOR, BstServerMonitor))
#define	BST_SERVER_MONITOR_CLASS(klass)	   (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_SERVER_MONITOR, BstServerMonitorClass))
#define	BST_IS_SERVER_MONITOR(object)	   (GTK_CHECK_TYPE ((object), BST_TYPE_SERVER_MONITOR))
#define	BST_IS_SERVER_MONITOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_SERVER_MONITOR))
#define BST_SERVER_MONITOR_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_SERVER_MONITOR, BstServerMonitorClass))


/* --- structures & typedefs --- */
typedef	struct	_BstServerMonitor      BstServerMonitor;
typedef	struct	_BstServerMonitorClass BstServerMonitorClass;
struct _BstServerMonitor
{
  GtkVBox	 parent_object;
  
  BswProxy	 server;
  
  GtkWidget	*param_view;
};
struct _BstServerMonitorClass
{
  GtkVBoxClass parent_class;
};


/* --- prototypes --- */
GtkType		 bst_server_monitor_get_type	(void);
void		 bst_server_monitor_update	(BstServerMonitor *smon);
void		 bst_server_monitor_rebuild	(BstServerMonitor *smon);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_SERVER_MONITOR_H__ */
