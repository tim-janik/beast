// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_SKIN_CONFIG_H__
#define __BST_SKIN_CONFIG_H__

#include "bstutils.hh"

G_BEGIN_DECLS

/* --- access skin config --- */
#define BST_SKIN_CONFIG(field)              (* bst_skin_config_get_global ()) . field
#define BST_SKIN_CONFIG_STRDUP_PATH(field)  sfi_path_get_filename (BST_SKIN_CONFIG (field), bst_skin_config_dirname())

/* --- prototypes --- */
void		_bst_skin_config_init		(void);
void		bst_skin_config_apply		(SfiRec		    *rec,
                                                 const gchar        *skin_file);
GParamSpec*	bst_skin_config_pspec		(void);
BstSkinConfig*  bst_skin_config_get_global      (void);
typedef void  (*BstSkinConfigNotify)            (gpointer            data);
void            bst_skin_config_notify          (void);
void            bst_skin_config_add_notify      (BstSkinConfigNotify func,
                                                 gpointer            data);


/* --- skin file --- */
void            bst_skin_config_set_rcfile      (const gchar    *file_name);
const gchar*    bst_skin_config_rcfile          (void);
const gchar*    bst_skin_config_dirname         (void);
BseErrorType    bst_skin_dump                   (const gchar    *file_name);
BseErrorType    bst_skin_parse                  (const gchar    *file_name);
 
                                                 
G_END_DECLS

#endif /* __BST_SKIN_CONFIG_H__ */
