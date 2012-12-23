// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_DIALOG_H__
#define __GXK_DIALOG_H__

#include        "gxkutils.hh"

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_DIALOG              (gxk_dialog_get_type ())
#define GXK_DIALOG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_DIALOG, GxkDialog))
#define GXK_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_DIALOG, GxkDialogClass))
#define GXK_IS_DIALOG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_DIALOG))
#define GXK_IS_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_DIALOG))
#define GXK_DIALOG_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_DIALOG, GxkDialogClass))


/* --- typedefs & enums --- */
typedef struct  _GxkDialog       GxkDialog;
typedef struct  _GxkDialogClass  GxkDialogClass;
typedef enum
{
  GXK_DIALOG_HIDE_ON_DELETE	= 1 << 0,	/* hide instead of destroy on window manager delete event */
  GXK_DIALOG_IGNORE_ESCAPE	= 1 << 1,       /* prevents delete event generation on Escape key presses */
  GXK_DIALOG_DELETE_BUTTON	= 1 << 2,	/* add a "Close" button */
  GXK_DIALOG_STATUS_BAR 	= 1 << 3,       /* add a status bar */
  GXK_DIALOG_WINDOW_GROUP	= 1 << 4,
  GXK_DIALOG_MODAL		= 1 << 5,
  GXK_DIALOG_POPUP_POS		= 1 << 6,	/* popup at mouse pointer */
  GXK_DIALOG_PRESERVE_STATE     = 1 << 7        /* don't always reset size etc. upon hiding */
} GxkDialogFlags;


/* --- structures --- */
struct _GxkDialog
{
  GtkWindow      window;

  GtkWidget	*vbox;

  /*< private >*/
  GtkObject	*alive_object;	/* dialog is destroyed with this object */
  GxkDialogFlags flags;
  gpointer	*pointer_loc;	/* nullified on destroy */
  GtkWidget	*status_bar;
  GtkWidget	*default_widget;
  GtkWidget	*focus_widget;
  GtkWidget	*sep;
  GtkWidget	*hbox;
  GtkWidget	*mbox;
  GtkWidget	*child;
};
struct _GxkDialogClass
{
  GtkWindowClass        parent_class;
};


/* --- prototypes --- */
GType		gxk_dialog_get_type		  (void);
gpointer	gxk_dialog_new			  (gpointer	   pointer_loc,
						   GtkObject	  *alive_object,
						   GxkDialogFlags  flags,
						   const gchar    *title,
						   GtkWidget	  *child);
gpointer        gxk_dialog_new_radget             (gpointer        pointer_loc,
                                                   GtkObject      *alive_object,
                                                   GxkDialogFlags  flags,
                                                   const gchar    *title,
                                                   const gchar    *domain_name,
                                                   const gchar    *radget_name);
void            gxk_dialog_set_sizes              (GxkDialog      *dialog,
                                                   gint            min_width,
                                                   gint            min_height,
                                                   gint            default_width,
                                                   gint            default_height);
void		gxk_dialog_set_title		  (GxkDialog	  *dialog,
						   const gchar	  *title);
void		gxk_dialog_set_focus		  (GxkDialog	  *dialog,
						   GtkWidget	  *widget);
void		gxk_dialog_set_default		  (GxkDialog	  *dialog,
						   GtkWidget	  *widget);
void		gxk_dialog_set_child		  (GxkDialog	  *dialog,
						   GtkWidget	  *child);
GtkWidget*	gxk_dialog_get_child		  (GxkDialog	  *dialog);
GxkDialog*	gxk_dialog_get_status_window	  (void);
void		gxk_dialog_add_flags		  (GxkDialog	  *dialog,
						   GxkDialogFlags  flags);
void		gxk_dialog_clear_flags		  (GxkDialog	  *dialog,
						   GxkDialogFlags  flags);
void		gxk_dialog_remove_actions	  (GxkDialog	  *dialog);
#define		gxk_dialog_action(		   dialog, action, callback, data)	\
  gxk_dialog_action_multi ((dialog), (action), (callback), (data), 0, (GxkDialogMultiFlags) 0)
#define		gxk_dialog_default_action(	   dialog, action, callback, data)	\
                                                  gxk_dialog_action_multi ((dialog), (action), (callback), (data), 0, GXK_DIALOG_MULTI_DEFAULT)
#define		gxk_dialog_action_swapped(	   dialog, action, callback, data)	\
                                                  gxk_dialog_action_multi ((dialog), (action), (callback), (data), 0, GXK_DIALOG_MULTI_SWAPPED)
#define		gxk_dialog_default_action_swapped( dialog, action, callback, data)	\
                                                  gxk_dialog_action_multi ((dialog), (action), (callback), (data), 0, GXK_DIALOG_MULTI_DEFAULT | GXK_DIALOG_MULTI_SWAPPED)


/* --- internal --- */
typedef enum /*< skip >*/
{
  GXK_DIALOG_MULTI_DEFAULT = 1,
  GXK_DIALOG_MULTI_SWAPPED = 2
} GxkDialogMultiFlags;
GtkWidget*	gxk_dialog_action_multi		(GxkDialog	    *dialog,
						 const gchar	    *action,
						 gpointer	     callback,
						 gpointer	     data,
						 const gchar	    *icon_stock_id,
						 GxkDialogMultiFlags multi_mode);

G_END_DECLS

// == Flags Enumeration Operators in C++ ==
#ifdef __cplusplus
inline GxkDialogFlags  operator&  (GxkDialogFlags  s1, GxkDialogFlags s2) { return GxkDialogFlags (s1 & (long long unsigned) s2); }
inline GxkDialogFlags& operator&= (GxkDialogFlags &s1, GxkDialogFlags s2) { s1 = s1 & s2; return s1; }
inline GxkDialogFlags  operator|  (GxkDialogFlags  s1, GxkDialogFlags s2) { return GxkDialogFlags (s1 | (long long unsigned) s2); }
inline GxkDialogFlags& operator|= (GxkDialogFlags &s1, GxkDialogFlags s2) { s1 = s1 | s2; return s1; }
inline GxkDialogFlags  operator~  (GxkDialogFlags  s1)                    { return GxkDialogFlags (~(long long unsigned) s1); }
inline GxkDialogMultiFlags  operator&  (GxkDialogMultiFlags  s1, GxkDialogMultiFlags s2) { return GxkDialogMultiFlags (s1 & (long long unsigned) s2); }
inline GxkDialogMultiFlags& operator&= (GxkDialogMultiFlags &s1, GxkDialogMultiFlags s2) { s1 = s1 & s2; return s1; }
inline GxkDialogMultiFlags  operator|  (GxkDialogMultiFlags  s1, GxkDialogMultiFlags s2) { return GxkDialogMultiFlags (s1 | (long long unsigned) s2); }
inline GxkDialogMultiFlags& operator|= (GxkDialogMultiFlags &s1, GxkDialogMultiFlags s2) { s1 = s1 | s2; return s1; }
inline GxkDialogMultiFlags  operator~  (GxkDialogMultiFlags  s1)                    { return GxkDialogMultiFlags (~(long long unsigned) s1); }
#endif // __cplusplus

#endif  /* __GXK_DIALOG_H__ */
