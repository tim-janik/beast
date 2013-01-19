// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_STANDARD_SYNTHS_H__
#define __BSE_STANDARD_SYNTHS_H__
#include        <bse/bseproject.hh>
G_BEGIN_DECLS
GSList*	bse_standard_synth_get_list	(void);
gchar*	bse_standard_synth_inflate	(const gchar	*synth_name,
					 guint		*text_len);
G_END_DECLS
#endif /* __BSE_STANDARD_SYNTHS_H__ */
