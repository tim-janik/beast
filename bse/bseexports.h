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



/* --- typedefs --- */
typedef void         (*BseProcedureInit)   (BseProcedureClass *proc,
					    BseParamSpec     **ipspecs,
					    BseParamSpec     **opspecs);
typedef BseErrorType (*BseProcedureExec)   (BseProcedureClass *procedure,
					    BseParam          *iparams,
					    BseParam          *oparams);
typedef void         (*BseProcedureUnload) (BseProcedureClass *procedure);


/* --- export types --- */
typedef enum		/* <skip> */
{
  BSE_EXPORT_TYPE_PROCS = 1,
} BseExportType;


/* --- export declarations --- */
struct _BseExportAny
{
  BseType            *type_p;
  const gchar  	     *name;
  const gchar  	     *category;
  const gchar  	     *blurb;
};
struct _BseExportProcedure
{
  BseType            *type_p;	  /* obligatory */
  const gchar  	     *name;	  /* obligatory */
  const gchar  	     *category;	  /* recommended */
  const gchar  	     *blurb;	  /* optional */

  BseProcedureInit    init;	  /* obligatory */
  BseProcedureExec    exec;	  /* obligatory */
  BseProcedureUnload  unload;	  /* optional */
};


/* --- export union --- */
union _BseExportSpec
{
  BseType		 *type_p; /* common to all members */
  BseExportAny		  any;
  BseExportProcedure	 s_proc;
};


/* --- internal prototypes --- */
void	bse_procedure_complete_info	(BseExportSpec  *spec,
					 BseTypeInfo    *info);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_EXPORTS_H__ */
