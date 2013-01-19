// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PREFERENCES_H__
#define __BST_PREFERENCES_H__
#include "bstparamview.hh"
G_BEGIN_DECLS
/* --- Gtk+ type macros --- */
#define	BST_TYPE_PREFERENCES		(bst_preferences_get_type ())
#define	BST_PREFERENCES(object)	        (GTK_CHECK_CAST ((object), BST_TYPE_PREFERENCES, BstPreferences))
#define	BST_PREFERENCES_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_PREFERENCES, BstPreferencesClass))
#define	BST_IS_PREFERENCES(object)	(GTK_CHECK_TYPE ((object), BST_TYPE_PREFERENCES))
#define	BST_IS_PREFERENCES_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_PREFERENCES))
#define BST_PREFERENCES_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), BST_TYPE_PREFERENCES, BstPreferencesClass))
/* --- structures & typedefs --- */
typedef	struct	_BstPreferences      BstPreferences;
typedef	struct	_BstPreferencesClass BstPreferencesClass;
struct _BstPreferences
{
  GtkVBox	        parent_object;
  GtkNotebook	       *notebook;
  GtkWidget	       *apply;
  SfiRec	       *rec_gconfig;
  SfiRing	       *params_gconfig;
  GtkWidget            *box_piano_keys;
  GtkWidget            *box_generic_keys;
  GtkWidget            *box_msg_absorb_config;
  SfiRec               *rec_skin;
  SfiRing              *params_skin;
  GParamSpec           *bsepspec;
  SfiRec	       *bserec;
  SfiRing	       *bseparams;
};
struct _BstPreferencesClass
{
  GtkVBoxClass parent_class;
};
/* --- prototypes --- */
GtkType		 bst_preferences_get_type	(void);
void		 bst_preferences_apply		(BstPreferences *prefs);
void		 bst_preferences_load_rc_files	(void);
void		 bst_preferences_save		(BstPreferences *prefs);
gboolean	 bst_preferences_saved  	(void);
void		 bst_preferences_revert		(BstPreferences *prefs);
void		 bst_preferences_default_revert	(BstPreferences *prefs);
void		 bst_preferences_create_buttons (BstPreferences *prefs,
						 GxkDialog      *dialog);
G_END_DECLS
#endif /* __BST_PREFERENCES_H__ */
