// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_SERVER_MONITOR_H__
#define __BST_SERVER_MONITOR_H__
#include	"bstparamview.hh"
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
  SfiProxy	 server;
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
