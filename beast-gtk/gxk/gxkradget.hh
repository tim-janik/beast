// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_RADGET_H__
#define __GXK_RADGET_H__
#include "gxkutils.hh"
G_BEGIN_DECLS
/* --- structures --- */
#define GXK_RADGET    G_OBJECT
#define GXK_IS_RADGET G_IS_OBJECT
typedef void          GxkRadget;
typedef struct _GxkRadgetArgs GxkRadgetArgs;
/* --- radget args (aggregation of (name, value) pairs) --- */
GxkRadgetArgs* gxk_radget_args          (const gchar         *name1,
                                         ...);
GxkRadgetArgs* gxk_radget_args_valist   (const gchar         *name1,
                                         va_list              var_args);
GxkRadgetArgs* gxk_radget_const_args    (void); /* based on intern_string values */
GxkRadgetArgs* gxk_radget_args_set      (GxkRadgetArgs       *args,
                                         const gchar         *name,
                                         const gchar         *value);
const gchar*   gxk_radget_args_get      (const GxkRadgetArgs *args,
                                         const gchar         *name);
GxkRadgetArgs* gxk_radget_args_merge    (GxkRadgetArgs       *dest,
                                         const GxkRadgetArgs *source);
void           gxk_radget_free_args     (GxkRadgetArgs       *args);
/* --- radget functions --- */
GxkRadget*    gxk_radget_create         (const gchar        *domain_name,
                                         const gchar        *name,
                                         const gchar        *var1,
                                         ...);
GxkRadget*    gxk_radget_complete       (GxkRadget          *radget,
                                         const gchar        *domain_name,
                                         const gchar        *name,
                                         const gchar        *var1,
                                         ...);
GxkRadget*    gxk_radget_creator        (GxkRadget          *radget,
                                         const gchar        *domain_name,
                                         const gchar        *name,
                                         GxkRadget          *parent,
                                         GSList             *call_args, /* const GxkRadgetArgs* */
                                         GSList             *env_args); /* const GxkRadgetArgs* */
const gchar*  gxk_radget_get_domain     (GxkRadget          *radget);
void          gxk_radget_parse          (const gchar        *domain_name,
                                         const gchar        *file_name,
                                         const gchar        *i18n_domain,
                                         GError            **error);
void          gxk_radget_parse_text     (const gchar        *domain_name,
                                         const gchar        *text,
                                         gint                text_len,
                                         const gchar        *i18n_domain,
                                         GError            **error);
gpointer      gxk_radget_find           (GxkRadget          *radget,
                                         const gchar        *name);
gpointer      gxk_radget_find_area      (GxkRadget          *radget,
                                         const gchar        *area);
void          gxk_radget_add            (GxkRadget          *radget,
                                         const gchar        *area,
                                         gpointer            widget);
void          gxk_radget_sensitize      (GxkRadget          *radget,
                                         const gchar        *name,
                                         gboolean            sensitive);
/* --- radget types --- */
typedef struct GxkRadgetData GxkRadgetData;
typedef struct {
  GParamSpec* (*find_prop) (GTypeClass         *klass,
                            const gchar        *construct_param_name);
  GxkRadget*  (*create)    (GType               type,
                            const gchar        *name,
                            guint               n_construct_params,
                            GParameter         *construct_params,
                            GxkRadgetData      *gdgdata);
  void        (*set_prop)  (GxkRadget          *radget,
                            const gchar        *prop_name,
                            const GValue       *value);
  gboolean    (*adopt)     (GxkRadget          *radget,
                            GxkRadget          *parent,
                            GxkRadgetData      *gdgdata);
  GParamSpec* (*find_pack) (GxkRadget          *radget,
                            const gchar        *pack_name);
  void        (*set_pack)  (GxkRadget          *radget,
                            const gchar        *pack_name,
                            const GValue       *value);
} GxkRadgetType;
void     gxk_radget_define_widget_type           (GType                type);
void     gxk_radget_define_type                  (GType                type,
                                                  const GxkRadgetType *ggtype);
gboolean gxk_radget_type_lookup                  (GType                type,
                                                  GxkRadgetType       *ggtype);
typedef void (*GxkRadgetHook)                    (GxkRadget           *radget,
                                                  guint                property_id,
                                                  const GValue        *value,
                                                  GParamSpec          *pspec);
void           gxk_radget_register_hook          (GParamSpec          *pspec,
                                                  guint                property_id,
                                                  GxkRadgetHook        hook_func);
GxkRadgetArgs* gxk_radget_data_copy_call_args    (GxkRadgetData       *gdgdata);
GxkRadget*     gxk_radget_data_get_scope_radget  (GxkRadgetData       *gdgdata);
gchar*         gxk_radget_data_dup_expand        (GxkRadgetData       *gdgdata,
                                                  const gchar         *expression);
G_END_DECLS
#endif /* __GXK_RADGET_H__ */
