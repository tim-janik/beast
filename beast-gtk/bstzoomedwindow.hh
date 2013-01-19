// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_ZOOMED_WINDOW_H__
#define __BST_ZOOMED_WINDOW_H__
#include	<gtk/gtkscrolledwindow.h>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* --- Gtk+ type macros --- */
#define	BST_TYPE_ZOOMED_WINDOW	          (bst_zoomed_window_get_type ())
#define	BST_ZOOMED_WINDOW(object)	  (GTK_CHECK_CAST ((object), BST_TYPE_ZOOMED_WINDOW, BstZoomedWindow))
#define	BST_ZOOMED_WINDOW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_ZOOMED_WINDOW, BstZoomedWindowClass))
#define	BST_IS_ZOOMED_WINDOW(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_ZOOMED_WINDOW))
#define	BST_IS_ZOOMED_WINDOW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_ZOOMED_WINDOW))
#define BST_ZOOMED_WINDOW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_ZOOMED_WINDOW, BstZoomedWindowClass))
/* --- structures & typedefs --- */
typedef	struct	_BstZoomedWindow	BstZoomedWindow;
typedef	struct	_BstZoomedWindowClass	BstZoomedWindowClass;
struct _BstZoomedWindow
{
  GtkScrolledWindow parent_object;
  GtkWidget	 *toggle_button;
};
struct _BstZoomedWindowClass
{
  GtkScrolledWindowClass parent_class;
  gboolean (*zoom) (BstZoomedWindow *zoomed_window,
		    gboolean         zoom_in);
};
/* --- prototypes --- */
GtkType		bst_zoomed_window_get_type		(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __BST_ZOOMED_WINDOW_H__ */
