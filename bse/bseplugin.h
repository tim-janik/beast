/* BSE - Better Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __BSE_PLUGIN_H__
#define __BSE_PLUGIN_H__

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

  gchar		*fname;
  gpointer	 gmodule;
  guint64        missing_export_flags;
  guint		 use_count : 16;
  guint          version_match : 1;
  guint          force_clean : 1;

  BseExportNode *chain;
  guint		 n_types;
  GType  	*types;
};
struct _BsePluginClass
{
  GObjectClass	parent_class;
};


/* --- prototypes --- */
SfiRing*	bse_plugin_path_list_files	(gboolean        include_drivers,
                                                 gboolean        include_plugins);
const gchar*	bse_plugin_check_load		(const gchar	*file_name);


/* --- registration macros --- */

#ifdef __cplusplus
#define BSE_DEFINE_EXPORTS()                                                                    \
    static ::BseExportIdentity __bse_export_identity =                                          \
                               BSE_EXPORT_IDENTITY (__enode_chain_head);                        \
  extern "C" {                                                                                  \
    extern ::BseExportIdentity *const BSE_EXPORT_IDENTITY_SYMBOL;                               \
    ::BseExportIdentity *const BSE_EXPORT_IDENTITY_SYMBOL = &__bse_export_identity;             \
  }
#else
#define BSE_DEFINE_EXPORTS()                                                                    \
  static BseExportIdentity __bse_export_identity =                                              \
                             BSE_EXPORT_IDENTITY (__enode_chain_head);                          \
  BseExportIdentity *const BSE_EXPORT_IDENTITY_SYMBOL = &__bse_export_identity
#endif

#define BSE_DEFINE_EXPORT_STRINGS_FUNC(FUNCNAME, BLURB, AUTHORS, LICENSE)                       \
  static void FUNCNAME (BseExportStrings *es) {                                                 \
    es->blurb = BLURB;                                                                          \
    es->authors = AUTHORS;                                                                      \
    es->license = LICENSE;                                                                      \
}
#define BSE_REGISTER_OBJECT_P(PREV,ObjectType,ParentType,ctgry,opt,blurb,pix,cinit,cfina,iinit) \
  BSE_DEFINE_EXPORT_STRINGS_FUNC (__enode_##ObjectType##__fill_strings, blurb, 0, 0)            \
  static BseExportNodeClass __enode_ ## ObjectType = {                                          \
    { PREV, BSE_EXPORT_NODE_CLASS, #ObjectType, opt, ctgry,                                     \
      pix, __enode_##ObjectType##__fill_strings },                                              \
    #ParentType, sizeof (ObjectType ## Class), (GClassInitFunc) cinit,                          \
    (GClassFinalizeFunc) cfina, sizeof (ObjectType), (GInstanceInitFunc) iinit,                 \
  };                                                                                            \
  static BseExportNode __enode_chain_head = {                                                   \
    (BseExportNode*) &__enode_ ## ObjectType, BSE_EXPORT_NODE_LINK,                             \
  }
#define BSE_REGISTER_OBJECT(ObjectType,ParentType,ctgry,opt,blurb,pix,cinit,cfina,iinit) \
  BSE_REGISTER_OBJECT_P (NULL, ObjectType, ParentType, ctgry, opt, blurb, pix, cinit, cfina, iinit)
#define BSE_EXPORT_TYPE_ID(EType)       (__enode_ ## EType . node.type)


/* --- implementation details --- */
void		         bse_plugin_init_builtins       (void);
extern BseExportIdentity bse_builtin_export_identity; /* sync with bsecxxplugin.hh */

G_END_DECLS

#endif /* __BSE_PLUGIN_H__ */
