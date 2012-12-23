// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PLAY_LIST_H__
#define __BST_PLAY_LIST_H__

#include	"bstutils.hh"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- Gtk+ type macros --- */
#define	BST_TYPE_PLAY_LIST	      (bst_play_list_get_type ())
#define	BST_PLAY_LIST(object)	      (GTK_CHECK_CAST ((object), BST_TYPE_PLAY_LIST, BstPlayList))
#define	BST_PLAY_LIST_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_PLAY_LIST, BstPlayListClass))
#define	BST_IS_PLAY_LIST(object)      (GTK_CHECK_TYPE ((object), BST_TYPE_PLAY_LIST))
#define	BST_IS_PLAY_LIST_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_PLAY_LIST))
#define BST_PLAY_LIST_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_PLAY_LIST, BstPlayListClass))


/* --- structures & typedefs --- */
typedef	struct	_BstPlayList		BstPlayList;
typedef	struct	_BstPlayListClass	BstPlayListClass;
struct _BstPlayList
{
  GtkVPaned	 parent_object;

  BseSong	*song;

  GtkWidget	*pattern_list;
  GtkWidget	*group_list;
  GtkSizeGroup  *size_group;
};
struct _BstPlayListClass
{
  GtkVPanedClass parent_class;
};


/* --- prototypes --- */
GtkType		bst_play_list_get_type	(void);
GtkWidget*	bst_play_list_new		(BseSong	*song);
void		bst_play_list_set_song		(BstPlayList	*plist,
						 BseSong	*song);
void		bst_play_list_rebuild		(BstPlayList	*plist);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PLAY_LIST_H__ */
