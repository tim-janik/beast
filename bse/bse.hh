// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_H__
#define __BSE_H__

#include <sfi/sfi.hh>
#include <bse/bseconfig.h>
#include <bse/bsecore.hh>

G_BEGIN_DECLS
/* initialize BSE and start the core thread */
void		bse_init_async		(gint		 *argc,
					 gchar	       ***argv,
					 const char     *app_name,
					 SfiInitValue    values[]);
/* provide SFI glue layer context for BSE */
SfiGlueContext*	bse_init_glue_context	(const gchar	*client);
/* library versioning */
extern const guint   bse_major_version;
extern const guint   bse_minor_version;
extern const guint   bse_micro_version;
extern const guint   bse_interface_age;
extern const guint   bse_binary_age;
extern const gchar  *bse_version;
const char*          bse_check_version	(guint           required_major,
					 guint           required_minor,
					 guint           required_micro);
G_END_DECLS

#endif /* __BSE_H__ */
