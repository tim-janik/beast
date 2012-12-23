// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_LOADER_H__
#define __BSE_LOADER_H__

#include <bse/bseutils.hh>
#include <bse/gslwavechunk.hh>

G_BEGIN_DECLS

/* --- structures --- */
struct _BseWaveFileInfo
{
  guint	   n_waves;
  struct Wave {
    gchar *name;
  }       *waves;

  gchar  **comments;

  /*< private >*/
  gchar     *file_name;
  BseLoader *loader;
  guint      ref_count;
};
struct _BseWaveDsc
{
  gchar		  *name;
  guint	           n_chunks;
  BseWaveChunkDsc *chunks;
  guint            n_channels;
  gchar          **xinfos;
  /*< private >*/
  BseWaveFileInfo *file_info;
};
struct _BseWaveChunkDsc
{
  gfloat	  osc_freq;
  gfloat	  mix_freq;
  gchar         **xinfos;
  /* loader-specific */
  union {
    guint         uint;
    gpointer      ptr;
    gfloat        vfloat;
  }               loader_data[8];
};


/* --- functions --- */
BseWaveFileInfo*      bse_wave_file_info_load	(const gchar	 *file_name,
						 BseErrorType	 *error);
BseWaveFileInfo*      bse_wave_file_info_ref	(BseWaveFileInfo *wave_file_info);
void                  bse_wave_file_info_unref	(BseWaveFileInfo *wave_file_info);
const gchar*	      bse_wave_file_info_loader	(BseWaveFileInfo *fi);
BseWaveDsc*	      bse_wave_dsc_load		(BseWaveFileInfo *wave_file_info,
						 guint		  nth_wave,
                                                 gboolean         accept_empty,
						 BseErrorType	 *error);
void		      bse_wave_dsc_free		(BseWaveDsc	 *wave_dsc);
GslDataHandle*	      bse_wave_handle_create	(BseWaveDsc	 *wave_dsc,
						 guint		  nth_chunk,
						 BseErrorType	 *error);
GslWaveChunk*	      bse_wave_chunk_create	(BseWaveDsc	 *wave_dsc,
						 guint		  nth_chunk,
						 BseErrorType	 *error);


/* --- loader impl --- */
typedef enum /*< skip >*/
{
  BSE_LOADER_NO_FLAGS              = 0,
  BSE_LOADER_SKIP_PRECEEDING_NULLS = 1 << 0
} BseLoaderFlags;
struct _BseLoader
{
  const gchar *name;		/* format/loader name, e.g. "BseWave" or "WAVE audio, RIFF (little-endian)" */

  /* at least one of the
   * following three must
   * be non-NULL
   */
  const gchar **extensions;	/* e.g.: "mp3", "ogg" or "bsewave" */
  const gchar **mime_types;	/* e.g.: "audio/x-mpg3" or "audio/x-wav" */
  BseLoaderFlags flags;
  const gchar **magic_specs;	/* e.g.: "0 string RIFF\n8 string WAVE\n" or "0 string #BseWave1\n" */

  gint   priority;   /* -100=high, +100=low, 0=default */

  /*< private >*/
  gpointer		  data;
  BseWaveFileInfo*	(*load_file_info)	(gpointer	   data,
						 const gchar	  *file_name,
						 BseErrorType	  *error);
  void			(*free_file_info)	(gpointer	   data,
						 BseWaveFileInfo  *file_info);
  BseWaveDsc*		(*load_wave_dsc)	(gpointer	   data,
						 BseWaveFileInfo  *file_info,
						 guint		   nth_wave,
						 BseErrorType	  *error);
  void			(*free_wave_dsc)	(gpointer	   data,
						 BseWaveDsc	  *wave_dsc);
  GslDataHandle*	(*create_chunk_handle)	(gpointer	   data,
						 BseWaveDsc	  *wave_dsc,
						 guint		   nth_chunk,
						 BseErrorType	  *error);
  BseLoader   *next;	/* must be NULL */
};

void	      bse_loader_register	        (BseLoader	 *loader);
BseLoader*    bse_loader_match	                (const gchar	 *file_name);

G_END_DECLS

#endif /* __BSE_LOADER_H__ */
