// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_ASSORTMENT_H__
#define __GXK_ASSORTMENT_H__
#include "gxkutils.hh"
G_BEGIN_DECLS
#define GXK_ASSORTMENT_PRIORITY     (G_PRIORITY_HIGH - 10)
/* --- assortment --- */
#define GXK_TYPE_ASSORTMENT_ENTRY        (gxk_assortment_entry_get_type ())
#define GXK_TYPE_ASSORTMENT              (gxk_assortment_get_type ())
#define GXK_ASSORTMENT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_ASSORTMENT, GxkAssortment))
#define GXK_ASSORTMENT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_ASSORTMENT, GxkAssortmentClass))
#define GXK_IS_ASSORTMENT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_ASSORTMENT))
#define GXK_IS_ASSORTMENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_ASSORTMENT))
#define GXK_ASSORTMENT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_ASSORTMENT, GxkAssortmentClass))
typedef struct GxkAssortment GxkAssortment;
typedef void (*GxkAssortmentDelete) (gpointer               user_data,
                                     GObject               *object,
                                     gpointer               owner);
typedef struct {
  gchar                 *label;
  gchar                 *stock_icon; /* stock_id for the icon or NULL */
  gchar                 *tooltip;
  gpointer               user_data;
  gpointer               owner;
  GObject               *object;
  GxkAssortmentDelete    free_func;
  guint                  ref_count;
} GxkAssortmentEntry;
struct GxkAssortment
{
  GObject             parent_instance;
  gchar              *publishing_name;
  GSList             *entries;
  GxkAssortmentEntry *selected;
  guint               block_count;
};
typedef struct {
  GObjectClass parent_class;
  void          (*entry_added)                  (GxkAssortment          *self,
                                                 GxkAssortmentEntry     *entry);
  void          (*entry_changed)                (GxkAssortment          *self,
                                                 GxkAssortmentEntry     *entry);
  void          (*entry_remove)                 (GxkAssortment          *self,
                                                 GxkAssortmentEntry     *entry);
  void          (*selection_changed)            (GxkAssortment          *self);
} GxkAssortmentClass;
/* --- assortment --- */
GType           gxk_assortment_entry_get_type   (void);
GType               gxk_assortment_get_type     (void);
GxkAssortment*      gxk_assortment_new          (void);
GxkAssortmentEntry* gxk_assortment_find_data    (GxkAssortment          *self,
                                                 gpointer                entry_user_data);
GxkAssortmentEntry* gxk_assortment_insert       (GxkAssortment          *self,
                                                 guint                   position,
                                                 const gchar            *label,
                                                 const gchar            *stock_icon, /* maybe NULL */
                                                 const gchar            *tooltip,
                                                 gpointer                user_data,
                                                 GObject                *object,
                                                 gpointer                owner,
                                                 GxkAssortmentDelete     free_func);
void                gxk_assortment_changed      (GxkAssortment          *self,
                                                 GxkAssortmentEntry     *entry);
void                gxk_assortment_remove       (GxkAssortment          *self,
                                                 GxkAssortmentEntry     *entry);
void                gxk_assortment_dispose      (GxkAssortment          *self);
void          gxk_assortment_block_selection    (GxkAssortment          *self);
void          gxk_assortment_select             (GxkAssortment          *self,
                                                 GxkAssortmentEntry     *entry);
void          gxk_assortment_select_data        (GxkAssortment          *self,
                                                 gpointer                entry_user_data);
void          gxk_assortment_unblock_selection  (GxkAssortment          *self);
void          gxk_assortment_manage_menu        (GxkAssortment          *self,
                                                 GtkMenu                *menu);
/* --- publishing --- */
void    gxk_widget_publish_assortment           (gpointer                widget,
                                                 const gchar            *publishing_name,
                                                 GxkAssortment          *assortment);
typedef void  (*GxkAssortmentClient)            (gpointer                client_data,
                                                 GtkWindow              *window,
                                                 GxkAssortment          *assortment,
                                                 GtkWidget              *publisher);
void    gxk_window_add_assortment_client        (GtkWindow              *window,
                                                 GxkAssortmentClient     added_func,
                                                 GxkAssortmentClient     removed_func,
                                                 gpointer                client_data);
void    gxk_window_remove_assortment_client     (GtkWindow              *window,
                                                 gpointer                client_data);
G_END_DECLS
#endif /* __GXK_ASSORTMENT_H__ */
