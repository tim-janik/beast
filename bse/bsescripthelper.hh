// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_SCRIPT_HELPER_H__
#define __BSE_SCRIPT_HELPER_H__

#include        <bse/bseprocedure.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- typedefs & structures --- */
typedef struct
{
  gchar	  *script_file;
  gchar	  *name;
  SfiRing *params;
} BseScriptData;
typedef struct
{
  BseProcedureClass parent_class;
  BseScriptData    *sdata;
} BseScriptProcedureClass;


/* --- API --- */
GType		bse_script_proc_register	(const gchar	*script_file,
						 const gchar	*name,
						 const gchar	*options,
						 const gchar	*category,
						 const gchar	*blurb,
						 const gchar    *file,
						 guint           line,
						 const gchar	*authors,
						 const gchar	*license,
						 SfiRing	*params);
SfiRing*	bse_script_path_list_files	(void);
BseErrorType    bse_script_file_register	(const gchar	*file_name,
						 BseJanitor    **janitor_p);
GValue*		bse_script_check_client_msg	(SfiGlueDecoder *decoder,
						 BseJanitor	*janitor,
						 const gchar    *message,
						 const GValue   *value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SCRIPT_HELPER_H__ */
