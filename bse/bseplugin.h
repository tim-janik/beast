/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSE_PLUGIN_H__
#define __BSE_PLUGIN_H__

/* default plugin name specification (if omitted in plugin) */
#ifndef BSE_PLUGIN_NAME
#  ifdef BSE_PLUGIN_FALLBACK
#    define BSE_PLUGIN_NAME BSE_PLUGIN_FALLBACK
#  else /* !BSE_PLUGIN_NAME && !BSE_PLUGIN_FALLBACK */
#    define BSE_PLUGIN_NAME __FILE__
#  endif /* !BSE_PLUGIN_NAME && !BSE_PLUGIN_FALLBACK */
#endif /* !BSE_PLUGIN_NAME */

#include	<bse/bse.h>	/* for bse_check_version() */
#include	<bse/bseexports.h>

G_BEGIN_DECLS


/* --- BSE type macros --- */
#define BSE_TYPE_PLUGIN              (BSE_TYPE_ID (BsePlugin))
#define BSE_PLUGIN(plugin)           (G_TYPE_CHECK_INSTANCE_CAST ((plugin), BSE_TYPE_PLUGIN, BsePlugin))
#define BSE_PLUGIN_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PLUGIN, BsePluginClass))
#define BSE_IS_PLUGIN(plugin)        (G_TYPE_CHECK_INSTANCE_TYPE ((plugin), BSE_TYPE_PLUGIN))
#define BSE_IS_PLUGIN_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PLUGIN))
#define BSE_PLUGIN_GET_CLASS(plugin) (G_TYPE_INSTANCE_GET_CLASS ((plugin), BSE_TYPE_PLUGIN, BsePluginClass))


/* --- BsePlugin --- */
struct _BsePlugin
{
  GObject	 parent_instance;

  gchar		*name;
  gchar		*fname;
  gpointer	 gmodule;
  guint		 use_count : 16;
  guint		 exports_procedures : 1;
  guint		 exports_objects : 1;
  guint		 exports_enums : 1;

  BseExportNode *chain;
  guint		 n_types;
  GType  	*types;
};
struct _BsePluginClass
{
  GObjectClass	parent_class;
};


/* --- prototypes --- */
GSList*		bse_plugin_dir_list_files	(const gchar	*dir_list);
const gchar*	bse_plugin_check_load		(const gchar	*file_name);
BsePlugin*	bse_plugin_lookup		(const gchar	*name);


/* --- registration macros --- */
#define BSE_DEFINE_EXPORTS(PluginName)                                                  \
  static BseExportIdentity __bse_export_identity =                                      \
                             BSE_EXPORT_IDENTITY (BSE_PLUGIN_NAME, __enode_chain_head); \
  BseExportIdentity *const BSE_EXPORT_IDENTITY_SYMBOL = &__bse_export_identity
#define BSE_REGISTER_OBJECT_P(PREV,ObjectType,ParentType,category,pix,blurb,cinit,cfina,iinit) \
  static BseExportNodeClass __enode_ ## ObjectType = {                                  \
    { PREV, BSE_EXPORT_NODE_CLASS, #ObjectType, category, pix, blurb },                 \
    #ParentType, sizeof (ObjectType ## Class), (GClassInitFunc) cinit,                  \
    (GClassFinalizeFunc) cfina, sizeof (ObjectType), (GInstanceInitFunc) iinit,         \
  };                                                                                    \
  static BseExportNode __enode_chain_head = {                                           \
    (BseExportNode*) &__enode_ ## ObjectType, BSE_EXPORT_NODE_LINK,                     \
  }
#define BSE_REGISTER_OBJECT(ObjectType,ParentType,category,pix,blurb,cinit,cfina,iinit) \
  BSE_REGISTER_OBJECT_P (NULL, ObjectType, ParentType, category, pix, blurb, cinit, cfina, iinit)
#define BSE_EXPORT_TYPE_ID(EType)       (__enode_ ## EType . node.type)


/* --- implementation details --- */
void		         bse_plugin_init_builtins     (void);
extern BseExportIdentity bse_builtin_export_identity; /* sync with bsecxxplugin.h */

G_END_DECLS

#endif /* __BSE_PLUGIN_H__ */
