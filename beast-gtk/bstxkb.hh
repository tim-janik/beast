// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_XKB_H__
#define __BST_XKB_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- prototypes --- */
gboolean	bst_xkb_open		(const gchar	*display,
					 gboolean	 sync);
void		bst_xkb_close		(void);
void		bst_xkb_dump		(void);
const gchar*	bst_xkb_get_symbol 	(gboolean	 physical);
void		bst_xkb_parse_symbol	(const gchar	*symbol,
					 gchar         **encoding_p,
					 gchar         **model_p,
					 gchar         **layout_p,
					 gchar         **variant_p);






#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_XKB_H__ */
