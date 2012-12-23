// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_FILE_HASH_H__
#define __GSL_FILE_HASH_H__

#include <bse/gsldefs.hh>
#include <bse/gslcommon.hh>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/* --- typedefs & structures --- */
typedef struct {
  gchar   *file_name;
  GTime    mtime;
  GslLong  n_bytes;
  /*< private >*/
  GslLong  cpos;
  BirnetMutex mutex;
  gint     fd;
  guint    ocount;
  GslLong  zoffset;
} GslHFile;
typedef struct {
  GslHFile *hfile;
  GslLong   offset;
} GslRFile;


/* --- GslHFile API --- */
GslHFile* gsl_hfile_open	(const gchar	*file_name);
GslLong	  gsl_hfile_pread	(GslHFile	*hfile,
				 GslLong	 offset,
				 GslLong         n_bytes,
				 gpointer	 bytes);
GslLong	  gsl_hfile_zoffset	(GslHFile	*hfile);
void	  gsl_hfile_close	(GslHFile	*hfile);


/* --- GslRFile API --- */
GslRFile* gsl_rfile_open	(const gchar	*file_name);
gchar*    gsl_rfile_name	(GslRFile	*rfile);
GslLong	  gsl_rfile_pread	(GslRFile	*rfile,
				 GslLong	 offset,
				 GslLong         n_bytes,
				 gpointer	 bytes);
GslLong	  gsl_rfile_read	(GslRFile	*rfile,
				 GslLong         n_bytes,
				 gpointer	 bytes);
GslLong	  gsl_rfile_seek_set	(GslRFile	*rfile,
				 GslLong	 offset);
GslLong	  gsl_rfile_position	(GslRFile	*rfile);
GslLong	  gsl_rfile_length	(GslRFile	*rfile);
void	  gsl_rfile_close	(GslRFile	*rfile);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_FILE_HASH_H__ */
