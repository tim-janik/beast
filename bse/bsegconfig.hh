// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_GCONFIG_H__
#define __BSE_GCONFIG_H__
#include	<bse/bseobject.hh>
#include	<bse/bsemain.hh>
G_BEGIN_DECLS
/* --- Global Config --- */
/* extern BseGConfig *bse_global_config; bsetype.hh */
void               _bse_gconfig_init	  (void);
void               bse_gconfig_apply	  (SfiRec *rec);
GParamSpec*        bse_gconfig_pspec	  (void);
void		   bse_gconfig_lock	  (void);
void		   bse_gconfig_unlock	  (void);
gboolean	   bse_gconfig_locked	  (void);
void               bse_gconfig_merge_args (const BseMainArgs *margs);
G_END_DECLS
#endif /* __BSE_GCONFIG_H__ */
