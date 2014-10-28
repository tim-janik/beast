// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_SPLASH_H__
#define __BST_SPLASH_H__

#include        "bstutils.hh"

G_BEGIN_DECLS

/* --- type macros --- */
#define BST_TYPE_SPLASH              (bst_splash_get_type ())
#define BST_SPLASH(object)           (GTK_CHECK_CAST ((object), BST_TYPE_SPLASH, BstSplash))
#define BST_SPLASH_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), BST_TYPE_SPLASH, BstSplashClass))
#define BST_IS_SPLASH(object)        (GTK_CHECK_TYPE ((object), BST_TYPE_SPLASH))
#define BST_IS_SPLASH_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), BST_TYPE_SPLASH))
#define BST_SPLASH_GET_CLASS(splash) (G_TYPE_INSTANCE_GET_CLASS ((splash), BST_TYPE_SPLASH, BstSplashClass))


/* --- typedefs & enums --- */
typedef struct  _BstSplash       BstSplash;
typedef struct  _BstSplashClass  BstSplashClass;


/* --- structures --- */
struct _BstSplash
{
  GtkWindow       window;

  GtkWidget	 *vbox;

  GtkWidget	 *splash_box;

  GtkWidget	 *entity;
  GtkWidget	 *item;
  GtkProgressBar *pbar;
  guint		  item_count;
  guint		  max_items;
  guint           has_grab : 1;
  guint           timer_id;
  gchar         **strings;
  guint           n_strings;
  guint           n_rand_strings;
  gfloat          aprogress;
};
struct _BstSplashClass
{
  GtkWindowClass parent_class;
};


/* --- prototypes --- */
GtkType		bst_splash_get_type		(void);
GtkWidget*	bst_splash_new			(const gchar	*role,
						 guint		 splash_width,
						 guint		 splash_height,
						 guint		 max_items);
void		bst_splash_set_title		(GtkWidget	*widget,
						 const gchar	*title);
void		bst_splash_show_grab		(GtkWidget	*widget);
void		bst_splash_release_grab 	(GtkWidget	*widget);
void		bst_splash_set_text		(GtkWidget *widget, const std::string &message);
void		bst_splash_set_animation	(GtkWidget	*widget,
						 GdkPixbufAnimation *anim);
void		bst_splash_update		(void);
void		bst_splash_update_entity	(GtkWidget *widget, const std::string &message);
void		bst_splash_update_item		(GtkWidget *widget, const std::string &message);
void            bst_splash_animate_strings      (GtkWidget      *splash,
                                                 const gchar   **strings);


G_END_DECLS

#endif  /* __BST_SPLASH_H__ */
