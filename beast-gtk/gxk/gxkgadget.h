/* GXK - Gtk+ Extension Kit
 * Copyright (C) 2003 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GXK_GADGET_H__
#define __GXK_GADGET_H__

#include "gxkutils.h"

G_BEGIN_DECLS

/* --- structures --- */
#define GXK_GADGET    G_OBJECT
#define GXK_IS_GADGET G_IS_OBJECT
typedef void          GxkGadget;
typedef struct _GxkGadgetArgs GxkGadgetArgs;


/* --- gadget args (aggregation of (name, value) pairs) --- */
GxkGadgetArgs* gxk_gadget_args          (const gchar         *name1,
                                         ...);
GxkGadgetArgs* gxk_gadget_args_valist   (const gchar         *name1,
                                         va_list              var_args);
GxkGadgetArgs* gxk_gadget_const_args    (void); /* based on intern_string values */
GxkGadgetArgs* gxk_gadget_args_set      (GxkGadgetArgs       *args,
                                         const gchar         *name,
                                         const gchar         *value);
const gchar*   gxk_gadget_args_get      (const GxkGadgetArgs *args,
                                         const gchar         *name);
GxkGadgetArgs* gxk_gadget_args_merge    (GxkGadgetArgs       *dest,
                                         const GxkGadgetArgs *source);
void           gxk_gadget_free_args     (GxkGadgetArgs       *args);


/* --- gadget functions --- */
GxkGadget*    gxk_gadget_create         (const gchar        *domain_name,
                                         const gchar        *name,
                                         const gchar        *var1,
                                         ...);
GxkGadget*    gxk_gadget_complete       (GxkGadget          *gadget,
                                         const gchar        *domain_name,
                                         const gchar        *name,
                                         const gchar        *var1,
                                         ...);
GxkGadget*    gxk_gadget_creator        (GxkGadget          *gadget,
                                         const gchar        *domain_name,
                                         const gchar        *name,
                                         GxkGadget          *parent,
                                         GSList             *call_args, /* const GxkGadgetArgs* */
                                         GSList             *env_args); /* const GxkGadgetArgs* */
const gchar*  gxk_gadget_get_domain     (GxkGadget          *gadget);
void          gxk_gadget_parse          (const gchar        *domain_name,
                                         const gchar        *file_name,
                                         const gchar        *i18n_domain,
                                         GError            **error);
void          gxk_gadget_parse_text     (const gchar        *domain_name,
                                         const gchar        *text,
                                         gint                text_len,
                                         const gchar        *i18n_domain,
                                         GError            **error);
gpointer      gxk_gadget_find           (GxkGadget          *gadget,
                                         const gchar        *name);
gpointer      gxk_gadget_find_area      (GxkGadget          *gadget,
                                         const gchar        *area);
void          gxk_gadget_add            (GxkGadget          *gadget,
                                         const gchar        *area,
                                         gpointer            widget);
void          gxk_gadget_sensitize      (GxkGadget          *gadget,
                                         const gchar        *name,
                                         gboolean            sensitive);


/* --- gadget types --- */
typedef struct GxkGadgetData GxkGadgetData;
typedef struct {
  GxkGadget*  (*create)    (GType               type,
                            const gchar        *name,
                            GxkGadgetData      *gdgdata);
  GParamSpec* (*find_prop) (GxkGadget          *gadget,
                            const gchar        *prop_name);
  void        (*set_prop)  (GxkGadget          *gadget,
                            const gchar        *prop_name,
                            const GValue       *value);
  gboolean    (*adopt)     (GxkGadget          *gadget,
                            GxkGadget          *parent,
                            GxkGadgetData      *gdgdata);
  GParamSpec* (*find_pack) (GxkGadget          *gadget,
                            const gchar        *pack_name);
  void        (*set_pack)  (GxkGadget          *gadget,
                            const gchar        *pack_name,
                            const GValue       *value);
} GxkGadgetType;
void           gxk_gadget_define_widget_type     (GType                type);
void           gxk_gadget_define_type            (GType                type,
                                                  const GxkGadgetType *ggtype);
gboolean       gxk_gadget_type_lookup            (GType                type,
                                                  GxkGadgetType       *ggtype);
GxkGadgetArgs* gxk_gadget_data_copy_call_args    (GxkGadgetData       *gdgdata);
GxkGadget*     gxk_gadget_data_get_scope_gadget  (GxkGadgetData       *gdgdata);

G_END_DECLS

#endif /* __GXK_GADGET_H__ */
