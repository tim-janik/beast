// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PROC_BROWSER_H__
#define __BST_PROC_BROWSER_H__

#include	"bstparamview.hh"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define BST_TYPE_PROC_BROWSER              (bst_proc_browser_get_type ())
#define BST_PROC_BROWSER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_PROC_BROWSER, BstProcBrowser))
#define BST_PROC_BROWSER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_PROC_BROWSER, BstProcBrowserClass))
#define BST_IS_PROC_BROWSER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_PROC_BROWSER))
#define BST_IS_PROC_BROWSER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_PROC_BROWSER))
#define BST_PROC_BROWSER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_PROC_BROWSER, BstProcBrowserClass))


/* --- structures & typedefs --- */
typedef	struct	_BstProcBrowser      BstProcBrowser;
typedef	struct	_BstProcBrowserClass BstProcBrowserClass;
struct _BstProcBrowser
{
  GtkVBox	  parent_object;

  GtkWidget	 *hbox;

  guint		  n_cats;
  BseCategory	 *cats;
  
  GxkListWrapper *proc_list;
  GtkEntry	*entry;

  /* buttons */
  GtkWidget	*execute;
};
struct _BstProcBrowserClass
{
  GtkVBoxClass parent_class;
};


/* --- prototypes --- */
GType		 bst_proc_browser_get_type	 (void);
GtkWidget*	 bst_proc_browser_new		 (void);
void		 bst_proc_browser_create_buttons (BstProcBrowser *self,
						  GxkDialog      *dialog);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PROC_BROWSER_H__ */
