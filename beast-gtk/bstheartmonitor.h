/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_HEART_MONITOR_H__
#define __BST_HEART_MONITOR_H__

#include	"bstparamview.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_HEART_MONITOR		  (bst_heart_monitor_get_type ())
#define	BST_HEART_MONITOR(object)	  (GTK_CHECK_CAST ((object), BST_TYPE_HEART_MONITOR, BstHeartMonitor))
#define	BST_HEART_MONITOR_CLASS(klass)	  (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_HEART_MONITOR, BstHeartMonitorClass))
#define	BST_IS_HEART_MONITOR(object)	  (GTK_CHECK_TYPE ((object), BST_TYPE_HEART_MONITOR))
#define	BST_IS_HEART_MONITOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_HEART_MONITOR))
#define BST_HEART_MONITOR_GET_CLASS(obj)  ((BstHeartMonitorClass*) (((GtkObject*) (obj))->klass))


/* --- structures & typedefs --- */
typedef	struct	_BstHeartMonitor      BstHeartMonitor;
typedef	struct	_BstHeartMonitorClass BstHeartMonitorClass;
struct _BstHeartMonitor
{
  GtkVBox	 parent_object;

  BseHeart	*heart;

  GtkWidget	*param_view;
};
struct _BstHeartMonitorClass
{
  GtkVBoxClass parent_class;
};


/* --- prototypes --- */
GtkType		 bst_heart_monitor_get_type	(void);
GtkWidget*	 bst_heart_monitor_new		(BseHeart	 *heart);
void		 bst_heart_monitor_update	(BstHeartMonitor *hmon);
void		 bst_heart_monitor_rebuild	(BstHeartMonitor *hmon);
void		 bst_heart_monitor_set_heart	(BstHeartMonitor *hmon,
						 BseHeart	 *heart);
BstHeartMonitor* bst_heart_monitor_from_heart	(BseHeart	 *heart);




#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */

#endif /* __BST_HEART_MONITOR_H__ */
