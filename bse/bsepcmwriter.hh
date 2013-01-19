// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PCM_WRITER_H__
#define __BSE_PCM_WRITER_H__
#include <bse/bseitem.hh>
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
  BirnetMutex	mutex;
  guint		open : 1;
  guint		broken : 1;
  gint		fd;
  uint64	n_bytes;
  uint64        recorded_maximum;
};
struct _BsePcmWriterClass
{
  BseItemClass		parent_class;
};
/* --- prototypes --- */
BseErrorType	bse_pcm_writer_open		(BsePcmWriter		*pdev,
						 const gchar		*file,
						 guint			 n_channels,
						 guint			 sample_freq,
                                                 uint64                  recorded_maximum);
void		bse_pcm_writer_close		(BsePcmWriter		*pdev);
/* writing is lock protected */
void		bse_pcm_writer_write		(BsePcmWriter		*pdev,
						 gsize			 n_values,
						 const gfloat		*values);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __BSE_PCM_WRITER_H__ */
