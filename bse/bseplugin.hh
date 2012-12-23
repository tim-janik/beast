// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PLUGIN_H__
#define __BSE_PLUGIN_H__
#include	<bse/bse.hh>	/* for bse_check_version() */
#include	<bse/bseexports.hh>
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
  guint          resident_types : 1;
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
void            bse_plugin_make_resident        ();
/* --- implementation details --- */
void		         bse_plugin_init_builtins       (void);
extern BseExportIdentity bse_builtin_export_identity; /* sync with bsecxxplugin.hh */
G_END_DECLS
#endif /* __BSE_PLUGIN_H__ */
