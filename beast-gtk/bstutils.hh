// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_UTILS_H__
#define __BST_UTILS_H__
#include "bstbseutils.hh"
#include "bstdefs.hh"
#include "bstcluehunter.hh"
/* generated type IDs, idl types */
#include "bstserverapi.hh"

namespace Bst {

// == event loop ==
int  event_loop_run  ();
void event_loop_quit (uint8 exit_code = 0);

} // Bst


// == Bse Server (BSE remote origin) ==
extern Bse::ServerH bse_server;


/* --- GUI utilities --- */
void           bst_status_set_error           (Bse::Error error, const String &message);
#define        bst_status_eprintf(error, ...)  bst_status_set_error (error, Bse::string_format (__VA_ARGS__))
void           bst_gui_error_bell             (gpointer         widget);
void           bst_window_sync_title_to_proxy (gpointer         window,
                                               SfiProxy         proxy,
                                               const gchar     *title_format);
void           bst_container_set_named_child  (GtkWidget       *container,
                                               GQuark           qname,
                                               GtkWidget       *child);
GtkWidget*     bst_container_get_named_child  (GtkWidget       *container,
                                               GQuark           qname);
GtkWidget*     bst_xpm_view_create            (const gchar    **xpm,
                                               GtkWidget       *colormap_widget);
GtkWidget*     bst_vpack                      (const gchar     *first_location,
                                               ...) G_GNUC_NULL_TERMINATED;
GtkWidget*     bst_hpack                      (const gchar     *first_location,
                                               ...) G_GNUC_NULL_TERMINATED;
GtkWidget*     bst_vpack0                     (const gchar     *first_location,
                                               ...) G_GNUC_NULL_TERMINATED;
GtkWidget*     bst_hpack0                     (const gchar     *first_location,
                                               ...) G_GNUC_NULL_TERMINATED;
void bst_action_list_add_cat    (GxkActionList *alist, const Bse::Category &cat, uint skip_levels, const char *stock_fallback,
                                 GxkActionCheck acheck, GxkActionExec aexec, gpointer user_data);
void bst_action_list_add_module (GxkActionList *alist, const Bse::AuxData &ad, const Bse::Icon &icon, const char *stock_fallback,
                                 GxkActionCheck acheck, GxkActionExec aexec, gpointer user_data);
GxkActionList* bst_action_list_from_cats      (const Bse::CategorySeq &cseq,
                                               guint            skip_levels,
                                               const gchar     *stock_fallback,
                                               GxkActionCheck   acheck,
                                               GxkActionExec    aexec,
                                               gpointer         user_data);
typedef gboolean (*BstActionListCategoryP)    (gpointer predicate_data, const Bse::Category &cat);
GxkActionList* bst_action_list_from_cats_pred (const Bse::CategorySeq &cseq,
                                               guint            skip_levels,
                                               const gchar     *stock_fallback,
                                               GxkActionCheck   acheck,
                                               GxkActionExec    aexec,
                                               gpointer         user_data,
                                               BstActionListCategoryP predicate,
                                               gpointer         predicate_data);
void           bst_background_handler1_add    (gboolean       (*handler) (gpointer data),
                                               gpointer         data,
                                               void           (*free_func) (gpointer data));
void           bst_background_handler2_add    (gboolean       (*handler) (gpointer data),
                                               gpointer         data,
                                               void           (*free_func) (gpointer data));


/* --- stock utilities --- */
GtkWidget* bst_stock_button             (const gchar  *stock_id);
GtkWidget* bst_stock_dbutton            (const gchar  *stock_id);
GtkWidget* bst_stock_icon_button        (const gchar  *stock_id);
void       bst_stock_register_icon      (const String &stock_id, uint bytes_per_pixel, uint width, uint height, uint rowstride, const uint8 *pixels);

/* --- pixbuf shortcuts --- */
#define bst_pixbuf_no_icon()    gxk_stock_fallback_pixbuf (BST_STOCK_NO_ICON)
#define bst_pixbuf_ladspa()     gxk_stock_fallback_pixbuf (BST_STOCK_LADSPA)
#define bst_pixbuf_knob()       gxk_stock_fallback_pixbuf (BST_STOCK_KNOB)


/* --- misc utils --- */
gchar*          bst_file_scan_find_key  (const gchar  *file,
                                         const gchar  *key,
                                         const gchar  *value_prefix);


/* --- GUI field mask --- */
typedef struct _BstGMask BstGMask;
GtkWidget*   bst_gmask_container_create (guint          border_width,
                                         gboolean       dislodge_columns);
typedef enum /*< skip >*/
{
  BST_GMASK_FIT,
  BST_GMASK_FILL,
  BST_GMASK_INTERLEAVE, /* stretch */
  BST_GMASK_BIG,
  BST_GMASK_CENTER,
  BST_GMASK_MULTI_SPAN
} BstGMaskPack;
BstGMask*       bst_gmask_form          (GtkWidget     *gmask_container,
                                         GtkWidget     *action,
                                         BstGMaskPack   gpack);
#define         bst_gmask_form_big(c,a) bst_gmask_form ((c), (a), BST_GMASK_BIG)
void            bst_gmask_set_tip       (BstGMask      *mask,
                                         const gchar   *tip_text);
void            bst_gmask_set_prompt    (BstGMask      *mask,
                                         gpointer       widget);
void            bst_gmask_set_aux1      (BstGMask      *mask,
                                         gpointer       widget);
void            bst_gmask_set_aux2      (BstGMask      *mask,
                                         gpointer       widget);
void            bst_gmask_set_aux3      (BstGMask      *mask,
                                         gpointer       widget);
void            bst_gmask_set_column    (BstGMask      *mask,
                                         guint          column);
GtkWidget*      bst_gmask_get_prompt    (BstGMask      *mask);
GtkWidget*      bst_gmask_get_aux1      (BstGMask      *mask);
GtkWidget*      bst_gmask_get_aux2      (BstGMask      *mask);
GtkWidget*      bst_gmask_get_aux3      (BstGMask      *mask);
GtkWidget*      bst_gmask_get_action    (BstGMask      *mask);
void            bst_gmask_foreach       (BstGMask      *mask,
                                         gpointer       func,
                                         gpointer       data);
void            bst_gmask_pack          (BstGMask      *mask);
BstGMask*       bst_gmask_quick         (GtkWidget     *gmask_container,
                                         guint          column,
                                         const gchar   *prompt,
                                         gpointer       action,
                                         const gchar   *tip_text);
#define bst_gmask_set_sensitive(mask, sensitive)        \
    bst_gmask_foreach ((mask), \
                       (sensitive) ? gxk_widget_make_sensitive : gxk_widget_make_insensitive, \
                       NULL)
#define bst_gmask_destroy(mask)                         \
    bst_gmask_foreach ((mask), gtk_widget_destroy, NULL)
#define bst_gmask_ref           g_object_ref
#define bst_gmask_unref         g_object_unref


/* --- object utils --- */
#define bst_object_class_install_property(oclass, group, property_id, pspec) \
  g_object_class_install_property (oclass, property_id, sfi_pspec_set_group (pspec, group))

/* --- stock icon aliases --- */
#define BST_STOCK_QUESTION              GTK_STOCK_DIALOG_QUESTION
#define BST_STOCK_WARNING               GTK_STOCK_DIALOG_WARNING
#define BST_STOCK_ERROR                 GTK_STOCK_DIALOG_ERROR

#define BST_STOCK_NEW                   GTK_STOCK_NEW
#define BST_STOCK_OPEN                  GTK_STOCK_OPEN
#define BST_STOCK_MERGE                 GTK_STOCK_OPEN // FIXME
#define BST_STOCK_SAVE                  GTK_STOCK_SAVE
#define BST_STOCK_SAVE_AS               GTK_STOCK_SAVE_AS
#define BST_STOCK_PREFERENCES           GTK_STOCK_PREFERENCES
#define BST_STOCK_QUIT                  GTK_STOCK_QUIT
#define BST_STOCK_CLEAR_UNDO            GTK_STOCK_DELETE // FIXME
#define BST_STOCK_PREFERENCES           GTK_STOCK_PREFERENCES
#define BST_STOCK_PLAY                  GTK_STOCK_GO_FORWARD // FIXME
#define BST_STOCK_STOP                  GTK_STOCK_STOP
#define BST_STOCK_REMOVE_SYNTH          GTK_STOCK_REMOVE
#define BST_STOCK_DOC_INDEX             GTK_STOCK_HELP // FIXME
#define BST_STOCK_DOC_NEWS              GTK_STOCK_NEW // FIXME
#define BST_STOCK_HELP                  GTK_STOCK_HELP
#define BST_STOCK_DOC_FAQ               GTK_STOCK_NEW // FIXME
#define BST_STOCK_DOC_DEVEL             GTK_STOCK_NEW // FIXME
#define BST_STOCK_ABOUT                 GTK_STOCK_YES // FIXME
#define BST_STOCK_ONLINE_HELP_DESK      GTK_STOCK_HELP // FIXME
#define BST_STOCK_ONLINE_BUGS           GTK_STOCK_HELP // FIXME
#define BST_STOCK_ONLINE_BEAST_SITE     GTK_STOCK_HOME // FIXME
#define BST_STOCK_ONLINE_SOUND_ARCHIVE  BST_STOCK_LOAD_LIB // FIXME
#define BST_STOCK_ADD                   GTK_STOCK_ADD
#define BST_STOCK_REMOVE                GTK_STOCK_REMOVE

#define BST_STOCK_SELECT_ALL            BST_STOCK_NONE // FIXME
#define BST_STOCK_SELECT_NONE           BST_STOCK_NONE // FIXME
#define BST_STOCK_SELECT_INVERT         BST_STOCK_NONE // FIXME


/* --- stock actions and aliases --- */
#define BST_STOCK_NONE                  ("")
#define BST_STOCK_APPLY                 GTK_STOCK_APPLY
#define BST_STOCK_CANCEL                GTK_STOCK_CANCEL
#define BST_STOCK_CDROM                 GTK_STOCK_CDROM
#define BST_STOCK_CLONE                 ("bst-stock-clone")
#define BST_STOCK_CLOSE                 GTK_STOCK_CLOSE
#define BST_STOCK_DEFAULT_REVERT        ("bst-stock-default-revert")
#define BST_STOCK_DELETE                GTK_STOCK_DELETE
#define BST_STOCK_DISMISS               ("bst-stock-dismiss")
#define BST_STOCK_EXECUTE               GTK_STOCK_EXECUTE
#define BST_STOCK_OK                    GTK_STOCK_OK
#define BST_STOCK_OVERWRITE             ("bst-stock-overwrite")
#define BST_STOCK_REDO                  GTK_STOCK_REDO
#define BST_STOCK_REVERT                ("bst-stock-revert")
#define BST_STOCK_UNDO                  GTK_STOCK_UNDO
#define BST_STOCK_ZOOM_100              GTK_STOCK_ZOOM_100
#define BST_STOCK_ZOOM_FIT              GTK_STOCK_ZOOM_FIT
#define BST_STOCK_ZOOM_IN               GTK_STOCK_ZOOM_IN
#define BST_STOCK_ZOOM_OUT              GTK_STOCK_ZOOM_OUT
/* --- generated includes --- */
/* marshallers */
#include "bstmarshal.h"
/* icon stock IDs */
#include "beast-gtk/icons/bst-stock-gen.h"
/* --- config values --- */
Bst::GConfig*   bst_gconfig_get_global (void);
#define BST_GCONFIG(field) (* bst_gconfig_get_global ()) . field
/* --- internal --- */
void            _bst_init_utils         (void);
void            _bst_init_radgets       (void);

// == mouse button checks ==
bool            bst_mouse_button_activate       (GdkEvent       *event);
bool            bst_mouse_button_activate1      (GdkEvent       *event);
bool            bst_mouse_button_activate2      (GdkEvent       *event);
bool            bst_mouse_button_context        (GdkEvent       *event);
bool            bst_mouse_button_move           (GdkEvent       *event);
inline bool     bst_mouse_button_activate       (GdkEventButton *event) { return bst_mouse_button_activate ((GdkEvent*) event); }
inline bool     bst_mouse_button_activate1      (GdkEventButton *event) { return bst_mouse_button_activate1 ((GdkEvent*) event); }
inline bool     bst_mouse_button_activate2      (GdkEventButton *event) { return bst_mouse_button_activate2 ((GdkEvent*) event); }
inline bool     bst_mouse_button_context        (GdkEventButton *event) { return bst_mouse_button_context ((GdkEvent*) event); }
inline bool     bst_mouse_button_move           (GdkEventButton *event) { return bst_mouse_button_move ((GdkEvent*) event); }


#endif /* __BST_UTILS_H__ */
