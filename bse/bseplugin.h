/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
 *
 * bseplugin.h: BSE plugin functions
 */
#ifndef __BSE_PLUGIN_H__
#define __BSE_PLUGIN_H__

#include	<bse/bseexports.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- BsePlugin --- */
struct _BsePlugin
{
  GTypePlugin    gtype_plugin;

  gchar		*name;
  gchar		*fname;
  gpointer	 gmodule;
  guint		 module_refs : 24;
  guint		 exports_procedures : 1;
  guint		 exports_objects : 1;
  guint		 exports_enums : 1;

  guint		 n_proc_types;
  GType  	*proc_types;
  guint		 n_object_types;
  GType  	*object_types;
  guint		 n_enum_types;
  GType  	*enum_types;

  /* private */
  gconstpointer	 e_procs;
  gconstpointer	 e_objects;
  gconstpointer	 e_enums;
};


/* --- prototypes --- */
GList*		bse_plugin_dir_list_files	(const gchar	*dir_list);
gchar*		bse_plugin_check_load		(const gchar	*file_name);
BsePlugin*	bse_plugin_lookup		(const gchar	*name);
void		bse_plugin_ref			(BsePlugin	*plugin);
void		bse_plugin_unref		(BsePlugin	*plugin);



/* --- implementation details --- */
void		bse_plugin_init			(void);
extern void	bse_plugin_complete_info	(BsePlugin      *plugin,
                                                 GType           type,
                                                 GTypeInfo    *type_info);
#define BSE_EXPORT_SYMBOL(y)		"bse_export__" G_STRINGIFY (y) "__symbol"
#ifndef	BSE_COMPILATION
#  define BSE_EXPORT_IMPL_S(y)		bse_export__##y##__symbol
#  define BSE_EXPORT_IMPL_V(y)		BseExport##y BSE_EXPORT_IMPL_S (y)
#  define BSE_EXPORT_IMPL_W(y)		BseExport##y * BSE_EXPORT_IMPL_S (y)
#  define BSE_EXPORT_IMPL_B(y)		BSE_EXPORT_CHECK_INIT \
                                        BSE_EXPORT_IMPL_D BSE_EXPORT_IMPL_V (Begin); \
                                        BSE_EXPORT_IMPL_I BSE_EXPORT_IMPL_V (Begin) = y
#  define BSE_EXPORT_IMPL_A(y)		BSE_EXPORT_IMPL_D BSE_EXPORT_IMPL_V (y) []; \
                                        BSE_EXPORT_IMPL_I BSE_EXPORT_IMPL_V (y) []
#  define BSE_EXPORT_IMPL_P(y)		BSE_EXPORT_IMPL_D BSE_EXPORT_IMPL_W (y); \
                                        BSE_EXPORT_IMPL_I BSE_EXPORT_IMPL_W (y)
#  define BSE_EXPORT_IMPL_E		BSE_EXPORT_IMPL_D BSE_EXPORT_IMPL_V (End); \
                                        BSE_EXPORT_IMPL_I BSE_EXPORT_IMPL_V (End) = BSE_MAGIC
#else  /* BSE_COMPILATION */
#  define BSE_EXPORT_IMPL_S(y)		bse_builtin__##y##__symbol
#  define BSE_EXPORT_IMPL_V(y)		static const BseExport##y BSE_EXPORT_IMPL_S (y)
#  define BSE_EXPORT_IMPL_L(y)		bse_builtin__##y##__init
#  define BSE_EXPORT_IMPL_F(y)		const gchar* BSE_EXPORT_IMPL_L (y) ( \
                                                       BsePlugin* BSE_EXPORT_IMPL_S (plugin))
#  define BSE_EXPORT_IMPL_C             const gchar* (*bse_plugin_builtin_init) (BsePlugin*, \
										 gconstpointer, \
                                                                                 BseExportType)
#  define BSE_EXPORT_IMPL_A(y)		BSE_EXPORT_IMPL_V (y) []
#  define BSE_EXPORT_IMPL_B(y)		\
extern BSE_EXPORT_IMPL_F (BSE_BUILTIN_NAME); \
extern gconstpointer BSE_EXPORT_IMPL_S (Procedure); \
extern gconstpointer BSE_EXPORT_IMPL_S (Object); \
BSE_EXPORT_IMPL_F (BSE_BUILTIN_NAME) \
{ \
  const gchar* BSE_EXPORT_IMPL_S (error) = NULL; \
  gchar* BSE_EXPORT_IMPL_S (plugin_name) = "BSE-Builtin-" y; \
  extern BSE_EXPORT_IMPL_C
#  define BSE_EXPORT_IMPL_E             \
  BSE_EXPORT_IMPL_S (plugin)->name = BSE_EXPORT_IMPL_S (plugin_name); \
  if (BSE_EXPORT_IMPL_S (Procedure)) \
    BSE_EXPORT_IMPL_S (error) = bse_plugin_builtin_init (BSE_EXPORT_IMPL_S (plugin), \
							 BSE_EXPORT_IMPL_S (Procedure), \
							 BSE_EXPORT_TYPE_PROCS); \
  if (BSE_EXPORT_IMPL_S (Object)) \
    BSE_EXPORT_IMPL_S (error) = bse_plugin_builtin_init (BSE_EXPORT_IMPL_S (plugin), \
							 BSE_EXPORT_IMPL_S (Object), \
							 BSE_EXPORT_TYPE_OBJECTS); \
  if (BSE_EXPORT_IMPL_S (error)) \
    return BSE_EXPORT_IMPL_S (error); \
  return NULL; \
} extern BSE_EXPORT_IMPL_C /* eat ; */
#endif /* BSE_COMPILATION */

/* default plugin name specification (if omitted in plugin)
 */
#ifndef BSE_PLUGIN_NAME
#  ifdef BSE_PLUGIN_FALLBACK
#    define BSE_PLUGIN_NAME BSE_PLUGIN_FALLBACK
#  else /* !BSE_PLUGIN_NAME && !BSE_PLUGIN_FALLBACK */
#    define BSE_PLUGIN_NAME __FILE__
#  endif /* !BSE_PLUGIN_NAME && !BSE_PLUGIN_FALLBACK */
#endif /* !BSE_PLUGIN_NAME */

/* system specific variable declaration and
 * implementation macros for symbol exports
 */
#if defined (G_OS_WIN32) && defined (__GNUC__)
#  define BSE_EXPORT_IMPL_D	__declspec(dllexport) const
#  define BSE_EXPORT_IMPL_I	const
#else /* !G_OS_WIN32 || !__GNUC__ */
#  define BSE_EXPORT_IMPL_D	extern const
#  define BSE_EXPORT_IMPL_I	const
#endif /* !NATIVE_WIN32 */

/* version check function for GModule initialization
 */
#define	BSE_EXPORT_CHECK_INIT	\
BSE_EXPORT_IMPL_D gchar* g_module_check_init (gpointer gmodule); \
BSE_EXPORT_IMPL_I gchar* \
g_module_check_init (gpointer gmodule) \
{ \
  return bse_check_version (BSE_MAJOR_VERSION, \
			    BSE_MINOR_VERSION, \
			    BSE_MICRO_VERSION - BSE_INTERFACE_AGE); \
}




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PLUGIN_H__ */
