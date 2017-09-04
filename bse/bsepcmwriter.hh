// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_PCM_WRITER_H__
#define __BSE_PCM_WRITER_H__

#include <bse/bseitem.hh>

/* --- object type macros --- */
#define BSE_TYPE_PCM_WRITER              (BSE_TYPE_ID (BsePcmWriter))
#define BSE_PCM_WRITER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_PCM_WRITER, BsePcmWriter))
#define BSE_PCM_WRITER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_PCM_WRITER, BsePcmWriterClass))
#define BSE_IS_PCM_WRITER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_PCM_WRITER))
#define BSE_IS_PCM_WRITER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_PCM_WRITER))
#define BSE_PCM_WRITER_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_PCM_WRITER, BsePcmWriterClass))


/* --- BsePcmWriter  --- */
struct BsePcmWriter : BseItem {
  std::mutex	mutex;
  guint		open : 1;
  guint		broken : 1;
  gint		fd;
  Bse::uint64	n_bytes;
  Bse::uint64   recorded_maximum;
  Bse::uint64   start_tick;
};
struct BsePcmWriterClass : BseItemClass
{};

Bse::Error bse_pcm_writer_open	(BsePcmWriter *pdev, const gchar *file, guint n_channels,
                                 guint sample_freq, Bse::uint64 recorded_maximum);
void	   bse_pcm_writer_close	(BsePcmWriter *pdev);
/* writing is lock protected */
void	   bse_pcm_writer_write	(BsePcmWriter *pdev, size_t n_values,
                                 const float *values, Bse::uint64 start_stamp);

namespace Bse {

class PcmWriterImpl : public ItemImpl, public virtual PcmWriterIface {
protected:
  virtual    ~PcmWriterImpl ();
public:
  explicit    PcmWriterImpl (BseObject*);
  static void trigger_tick  (uint64 start_tick);
};

} // Bse

#endif /* __BSE_PCM_WRITER_H__ */
