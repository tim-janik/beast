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
 * bseexports.h: export declarations for external plugins
 */
#ifndef __BSE_EXPORTS_H__
#define __BSE_EXPORTS_H__

#include	<bse/bseparam.h>
#include	<bse/bseconfig.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- plugin export macros --- */
/* (implementations reside in bseplugin.h) */
/* start export section,
 * provide a unique plugin here
 */
#define BSE_EXPORTS_BEGIN(UniqueName)   BSE_EXPORT_IMPL_B (UniqueName)
/* list procedure types as BseExportProcedure(bseexports.h) array
 */
#define BSE_EXPORT_PROCEDURES           BSE_EXPORT_IMPL_A (Procedure)
/* list object types as BseExportObject(bseexports.h) array
 */
#define BSE_EXPORT_OBJECTS              BSE_EXPORT_IMPL_A (Object)
/* end export section
 */
#define BSE_EXPORTS_END                 BSE_EXPORT_IMPL_E


/* --- typedefs --- */
typedef const gchar*                        BseExportBegin;
typedef union  _BseExportSpec               BseExportSpec;
typedef struct _BseExportAny                BseExportAny;
typedef struct _BseExportObject             BseExportObject;
typedef struct _BseExportProcedure     	    BseExportProcedure;
typedef guint                               BseExportEnd;
typedef void         (*BseProcedureInit)   (BseProcedureClass *proc,
					    BseParamSpec     **ipspecs,
					    BseParamSpec     **opspecs);
typedef BseErrorType (*BseProcedureExec)   (BseProcedureClass *procedure,
					    BseParam          *iparams,
					    BseParam          *oparams);
typedef void         (*BseProcedureUnload) (BseProcedureClass *procedure);


/* --- export types --- */
typedef enum			/*< skip >*/
{
  BSE_EXPORT_TYPE_PROCS		= 1,
  BSE_EXPORT_TYPE_OBJECTS	= 2
} BseExportType;


/* --- export declarations --- */
struct _BseExportAny
{
  BseType            *type_p;
  const gchar  	     *name;
};
struct _BseExportProcedure
{
  BseType            *type_p;	   /* obligatory */
  const gchar  	     *name;	   /* obligatory */
  const gchar  	     *blurb;	   /* optional */

  BseProcedureInit    init;	   /* obligatory */
  BseProcedureExec    exec;	   /* obligatory */
  BseProcedureUnload  unload;	   /* optional */
  
  const gchar  	     *category;	   /* recommended */
  const BsePixdata    pixdata;     /* optional */
};
struct _BseExportObject
{
  BseType            *type_p;	   /* obligatory */
  const gchar  	     *name;	   /* obligatory */
  const gchar  	     *parent_type; /* obligatory */
  const gchar  	     *blurb;	   /* optional */

  const BseTypeInfo  *object_info; /* obligatory */

  const gchar  	     *category;	   /* recommended */
  const BsePixdata    pixdata;     /* optional */
};


/* --- export union --- */
union _BseExportSpec
{
  BseType		 *type_p; /* common to all members */
  BseExportAny		  any;
  BseExportProcedure	  s_proc;
  BseExportObject	  s_object;
};


/* --- internal prototypes --- */
void	bse_procedure_complete_info	(const BseExportSpec *spec,
					 BseTypeInfo         *info);
void	bse_object_complete_info	(const BseExportSpec *spec,
					 BseTypeInfo         *info);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_EXPORTS_H__ */
