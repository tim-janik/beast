// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_FREE_VERB_CPP_H__
#define __BSE_FREE_VERB_CPP_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct
{
  /* runtime parameters */
  gfloat room_size;
  gfloat damp;
  gfloat wet;
  gfloat dry;
  gfloat width;
} BseFreeVerbConfig;
typedef struct
{
  /* constants */
  gfloat room_offset;
  gfloat room_scale;
  gfloat damp_scale;
  gfloat wet_scale;
  gfloat dry_scale;
  gfloat width_scale;
} BseFreeVerbConstants;
typedef struct
{
  gpointer obj;
  BseFreeVerbConfig saved_config;
} BseFreeVerbCpp;

void   bse_free_verb_cpp_create		(BseFreeVerbCpp		*cpp);
void   bse_free_verb_cpp_configure	(BseFreeVerbCpp		*cpp,
					 BseFreeVerbConfig	*config);
void   bse_free_verb_cpp_process	(BseFreeVerbCpp		*cpp,
					 guint			 n_values,
					 const gfloat		*ileft,
					 const gfloat		*iright,
					 gfloat			*oleft,
					 gfloat			*oright);
void   bse_free_verb_cpp_destroy	(BseFreeVerbCpp		*cpp);
void   bse_free_verb_cpp_defaults	(BseFreeVerbConfig	*config,
					 BseFreeVerbConstants	*constants);
void   bse_free_verb_cpp_save_config	(BseFreeVerbCpp		*cpp,
					 BseFreeVerbConfig	*config);
void   bse_free_verb_cpp_restore_config	(BseFreeVerbCpp		*cpp,
					 BseFreeVerbConfig	*config);


G_END_DECLS


#endif /* __BSE_FREE_VERB_CPP_H__ */
