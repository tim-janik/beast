// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_FILE_DIALOG_H__
#define __BST_FILE_DIALOG_H__

#include "bstutils.hh"
#include "bstapp.hh"

G_BEGIN_DECLS


/* --- type macros --- */
#define BST_TYPE_FILE_DIALOG              (bst_file_dialog_get_type ())
#define BST_FILE_DIALOG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_FILE_DIALOG, BstFileDialog))
#define BST_FILE_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_FILE_DIALOG, BstFileDialogClass))
#define BST_IS_FILE_DIALOG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_FILE_DIALOG))
#define BST_IS_FILE_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_FILE_DIALOG))
#define BST_FILE_DIALOG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_FILE_DIALOG, BstFileDialogClass))


/* --- typedefs --- */
typedef struct  _BstFileDialog	    BstFileDialog;
typedef struct  _BstFileDialogClass BstFileDialogClass;


/* --- structures --- */
typedef enum {
  BST_FILE_DIALOG_OPEN_PROJECT	   = 0x0001,
  BST_FILE_DIALOG_MERGE_PROJECT	   = 0x0002,
  BST_FILE_DIALOG_SAVE_PROJECT	   = 0x0003,
  BST_FILE_DIALOG_IMPORT_MIDI	   = 0x0004,
  BST_FILE_DIALOG_SELECT_FILE	   = 0x0008,
  BST_FILE_DIALOG_SELECT_DIR	   = 0x0009,
  BST_FILE_DIALOG_LOAD_WAVE	   = 0x0011,
  BST_FILE_DIALOG_LOAD_WAVE_LIB	   = 0x0012,
  BST_FILE_DIALOG_MERGE_EFFECT     = 0x0021,
  BST_FILE_DIALOG_MERGE_INSTRUMENT = 0x0022,
  BST_FILE_DIALOG_SAVE_EFFECT      = 0x0023,
  BST_FILE_DIALOG_SAVE_INSTRUMENT  = 0x0024,
  BST_FILE_DIALOG_MODE_MASK	   = 0x00ff,
  BST_FILE_DIALOG_ALLOW_DIRS	   = 0x1000,
  BST_FILE_DIALOG_FLAG_MASK	   = 0xff00
} BstFileDialogMode;
struct _BstFileDialog
{
  GxkDialog	    parent_instance;
  GtkFileSelection *fs;
  GtkWidget	   *notebook;
  GtkWidget	   *fpage;	/* file selection */
  GtkWidget	   *spage;	/* sample selection */
  GtkTreeView	   *tview;	/* sample selection tree view */
  GtkWidget	   *osave;	/* save options */
  GtkWidget	   *radio1, *radio2;
  gchar            *selected;
  /* mode state */
  BstFileDialogMode mode : 16;
  guint		    ignore_activate : 1;
  guint		    using_file_store : 1;
  guint             apply_project_name : 1;
  GtkTreeModel     *file_store;
  gchar            *search_path;
  const gchar      *search_filter;
  GtkWindow	   *parent_window;
  SfiProxy	    proxy, super;
};
struct _BstFileDialogClass
{
  GxkDialogClass parent_class;
};


/* --- prototypes --- */
GType		bst_file_dialog_get_type		(void);
GtkWidget*	bst_file_dialog_popup_open_project	(gpointer	   parent_widget);
GtkWidget*	bst_file_dialog_popup_merge_project	(gpointer	   parent_widget,
							 SfiProxy	   project);
GtkWidget*	bst_file_dialog_popup_import_midi	(gpointer	   parent_widget,
							 SfiProxy	   project);
GtkWidget*	bst_file_dialog_popup_save_project	(gpointer	   parent_widget,
							 SfiProxy	   project,
                                                         gboolean          query_project_name,
                                                         gboolean          apply_project_name);
GtkWidget*      bst_file_dialog_popup_merge_effect      (gpointer          parent_widget,
                                                         SfiProxy          project);
GtkWidget*	bst_file_dialog_popup_save_effect	(gpointer	   parent_widget,
							 SfiProxy	   project,
                                                         SfiProxy          super);
GtkWidget*	bst_file_dialog_popup_save_instrument	(gpointer	   parent_widget,
							 SfiProxy	   project,
                                                         SfiProxy          super);
GtkWidget*      bst_file_dialog_popup_merge_instrument  (gpointer          parent_widget,
                                                         SfiProxy          project);
GtkWidget*	bst_file_dialog_popup_select_file  	(gpointer	   parent_widget);
GtkWidget*	bst_file_dialog_popup_select_dir  	(gpointer	   parent_widget);
GtkWidget*	bst_file_dialog_popup_load_wave		(gpointer	   parent_widget,
							 SfiProxy	   wave_repo,
							 gboolean	   show_lib);
void		bst_file_dialog_set_mode		(BstFileDialog	  *self,
							 gpointer          parent_widget,
							 BstFileDialogMode mode,
							 const gchar	  *fs_title,
							 SfiProxy	   project);
GtkWidget*      bst_file_dialog_create                  (void);
void            bst_file_dialog_setup                   (GtkWidget        *widget,
                                                         gpointer          parent_widget,
                                                         const gchar      *title,
                                                         const gchar      *search_path);
typedef void  (*BstFileDialogHandler)                   (GtkWidget        *dialog,
                                                         const gchar      *file,
                                                         gpointer          user_data);
void            bst_file_dialog_set_handler             (BstFileDialog    *self,
                                                         BstFileDialogHandler handler,
                                                         gpointer          handler_data,
                                                         GDestroyNotify    destroy);

G_END_DECLS

// == Flags Enumeration Operators in C++ ==
#ifdef __cplusplus
constexpr BstFileDialogMode  operator&  (BstFileDialogMode  s1, BstFileDialogMode s2) { return BstFileDialogMode (s1 & (long long unsigned) s2); }
inline    BstFileDialogMode& operator&= (BstFileDialogMode &s1, BstFileDialogMode s2) { s1 = s1 & s2; return s1; }
constexpr BstFileDialogMode  operator|  (BstFileDialogMode  s1, BstFileDialogMode s2) { return BstFileDialogMode (s1 | (long long unsigned) s2); }
inline    BstFileDialogMode& operator|= (BstFileDialogMode &s1, BstFileDialogMode s2) { s1 = s1 | s2; return s1; }
constexpr BstFileDialogMode  operator~  (BstFileDialogMode  s1)                    { return BstFileDialogMode (~(long long unsigned) s1); }
#endif // __cplusplus

#endif  /* __BST_FILE_DIALOG_H__ */
