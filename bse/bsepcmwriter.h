/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2003 Tim Janik
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
 */
#ifndef __BSE_PCM_WRITER_H__
#define __BSE_PCM_WRITER_H__

#include <bse/bseitem.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_PCM_WRITER              (BSE_TYPE_ID (BsePcmWriter))
#define BSE_PCM_WRITER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_WRITER, BsePcmWriter))
#define BSE_PCM_WRITER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_WRITER, BsePcmWriterClass))
#define BSE_IS_PCM_WRITER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_WRITER))
#define BSE_IS_PCM_WRITER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_WRITER))
#define BSE_PCM_WRITER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_WRITER, BsePcmWriterClass))


/* --- BsePcmWriter  --- */
struct _BsePcmWriter
{
  BseItem	parent_instance;
  guint		open : 1;
  guint		broken : 1;
  gint		fd;
  guint		n_bytes;
  SfiMutex	mutex;
};
struct _BsePcmWriterClass
{
  BseItemClass		parent_class;
};


/* --- prototypes --- */
BseErrorType	bse_pcm_writer_open		(BsePcmWriter		*pdev,
						 const gchar		*file,
						 guint			 n_channels,
						 guint			 sample_freq);
void		bse_pcm_writer_close		(BsePcmWriter		*pdev);
/* writing is lock protected */
void		bse_pcm_writer_write		(BsePcmWriter		*pdev,
						 gsize			 n_values,
						 const gfloat		*values);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_PCM_WRITER_H__ */
