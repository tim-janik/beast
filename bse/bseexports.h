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
 *
 * bseexports.h: export declarations for external plugins
 */
#ifndef __BSE_EXPORTS_H__
#define __BSE_EXPORTS_H__

#include	<bse/bseprocedure.h>
#include	<bse/bseconfig.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- plugin export macros --- */
/* (implementations reside in bseplugin.h) */
/* start export section, provide a unique plugin name here
 */
#define BSE_EXPORTS_BEGIN(UniqueName)   BSE_EXPORT_IMPL_B (UniqueName)
/* list procedure types in BseExportProcedure array
 */
#define BSE_EXPORT_PROCEDURES           BSE_EXPORT_IMPL_A (Procedure)
/* list object types in BseExportObject array
 */
#define BSE_EXPORT_OBJECTS              BSE_EXPORT_IMPL_A (Object)
/* qualify exported procedure types as file handlers
 */
#define BSE_EXPORT_FILE_HANDLERS        BSE_EXPORT_IMPL_A (FileHandler)
/* list enum types as BseExportEnum array (mere internal use)
 */
#define BSE_EXPORT_STATIC_ENUMS		static const BseExportEnum \
                                        BSE_EXPORT_IMPL_S (MkEnums_built) []
/* directive, used to trigger automated enum generation from
 * plugin's .h file. also auto-exports them to BSE
 */
#define BSE_EXPORT_AND_GENERATE_ENUMS() BSE_EXPORT_IMPL_P (Enum) = \
                                        BSE_EXPORT_IMPL_S (MkEnums_built)
/* end export section
 */
#define BSE_EXPORTS_END                 BSE_EXPORT_IMPL_E


/* --- typedefs --- */
typedef const gchar*                        BseExportBegin;
typedef union  _BseExportSpec               BseExportSpec;
typedef struct _BseExportObject             BseExportObject;
typedef struct _BseExportEnum               BseExportEnum;
typedef struct _BseExportFileHandler   	    BseExportFileHandler;
typedef struct _BseExportProcedure     	    BseExportProcedure;
typedef guint                               BseExportEnd;
typedef void         (*BseProcedureInit)   (BseProcedureClass *proc,
					    GParamSpec	     **in_pspecs,
					    GParamSpec	     **out_pspecs);
typedef void         (*BseProcedureUnload) (BseProcedureClass *procedure);


/* --- export types --- */
typedef enum			/*< skip >*/
{
  BSE_EXPORT_TYPE_PROCS		= 1,
  BSE_EXPORT_TYPE_OBJECTS	= 2,
  BSE_EXPORT_TYPE_ENUMS		= 3,
  BSE_EXPORT_TYPE_FILE_HANDLERS	= 4
} BseExportType;


/* --- File Handler Types --- */
typedef enum
{
  BSE_FILE_CUSTOM_LOADER	= -100,
  BSE_FILE_STANDARD_LOADER	= 0,
  BSE_FILE_FALLBACK_LOADER	= 100,
} BseFileHandlerType;


/* --- export declarations --- */
struct _BseExportProcedure
{
  GType              *type_p;	   /* obligatory */
  const gchar  	     *name;	   /* obligatory */
  const gchar  	     *blurb;	   /* optional */
  const guint  	      private_id;  /* optional */

  BseProcedureInit    init;	   /* obligatory */
  BseProcedureExec    exec;	   /* obligatory */
  BseProcedureUnload  unload;	   /* optional */
  
  const gchar  	     *category;	   /* recommended */
  const BsePixdata    pixdata;     /* optional */
};
struct _BseExportObject
{
  GType              *type_p;	   /* obligatory */
  const gchar  	     *name;	   /* obligatory */
  const gchar  	     *parent_type; /* obligatory */
  const gchar  	     *blurb;	   /* optional */

  const GTypeInfo  *object_info; /* obligatory */

  const gchar  	     *category;	   /* recommended */
  const BsePixdata    pixdata;     /* optional */
};
struct _BseExportEnum
{
  GType              *type_p;	   /* obligatory */
  const gchar  	     *name;	   /* obligatory */
  GType               parent_type; /* obligatory */
  gpointer            values;      /* obligatory */
};
struct _BseExportFileHandler
{
  GType              *type_p;      /* obligatory, referring to procedure type */
  BseFileHandlerType  fh_type;
  const gchar        *prefix;	   /* optional */
  const gchar	     *extension;   /* optional */
  const gchar        *magic;	   /* optional, prerequisite if given */
};


/* --- export union --- */
union _BseExportSpec
{
  GType  		 *type_p; /* common to all members */
  BseExportProcedure	  s_proc;
  BseExportObject	  s_object;
  BseExportEnum		  s_enum;
  BseExportFileHandler	  s_file_handler;
};


/* --- internal prototypes --- */
void	bse_procedure_complete_info	(const BseExportSpec *spec,
					 GTypeInfo         *info);
void	bse_object_complete_info	(const BseExportSpec *spec,
					 GTypeInfo         *info);
void	bse_enum_complete_info		(const BseExportSpec *spec,
					 GTypeInfo         *info);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_EXPORTS_H__ */
