// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_GCONFIG_H__
#define __BST_GCONFIG_H__
#include	"bstutils.hh"
G_BEGIN_DECLS
/* --- BstGConfig - configurable defaults --- */
#define	BST_RC_VERSION			BST_GCONFIG (rc_version)
#define BST_GUI_ENABLE_ERROR_BELL   	BST_GCONFIG (gui_enable_error_bell)
#define BST_SNET_ANTI_ALIASED		BST_GCONFIG (snet_anti_aliased)
#define BST_SNET_EDIT_FALLBACK		BST_GCONFIG (snet_edit_fallback)
#define BST_SNET_SWAP_IO_CHANNELS	BST_GCONFIG (snet_swap_io_channels)
/* --- prototypes --- */
void		_bst_gconfig_init		(void);
void		bst_gconfig_set_rc_version	(const gchar	*rc_version);
void		bst_gconfig_set_rec_rc_version	(SfiRec         *rec,
                                                 const gchar	*rc_version);
void		bst_gconfig_apply		(SfiRec		*rec);
GParamSpec*	bst_gconfig_pspec		(void);
void		bst_gconfig_push_updates	(void);
/* bstutils.hh: BstGConfig*     bst_gconfig_get_global (void); */
/* --- rc file --- */
BseErrorType     bst_rc_dump                    (const gchar    *file_name);
BseErrorType     bst_rc_parse                   (const gchar    *file_name);
G_END_DECLS
#endif /* __BST_GCONFIG_H__ */
