// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_VORBIS_CUTTER_H__
#define __GSL_VORBIS_CUTTER_H__
#include <bse/gslcommon.hh>
G_BEGIN_DECLS
/* --- typedefs & enums --- */
typedef struct _GslVorbisCutter  GslVorbisCutter;
typedef enum
{
  GSL_VORBIS_CUTTER_NONE                = 0,
  GSL_VORBIS_CUTTER_SAMPLE_BOUNDARY     = 1,
  GSL_VORBIS_CUTTER_PACKET_BOUNDARY     = 2,
  GSL_VORBIS_CUTTER_PAGE_BOUNDARY       = 3
} GslVorbisCutterMode;
/* --- cutter API --- */
GslVorbisCutter*  gsl_vorbis_cutter_new                 (void);
void              gsl_vorbis_cutter_set_cutpoint        (GslVorbisCutter        *self,
                                                         GslVorbisCutterMode     cutmode,
                                                         SfiNum                  cutpoint);
void              gsl_vorbis_cutter_filter_serialno     (GslVorbisCutter        *self,
                                                         guint                   serialno);
void              gsl_vorbis_cutter_force_serialno      (GslVorbisCutter        *self,
                                                         guint                   serialno);
void              gsl_vorbis_cutter_write_ogg           (GslVorbisCutter        *self,
                                                         guint                   n_bytes,
                                                         guint8                 *bytes);
guint             gsl_vorbis_cutter_read_ogg            (GslVorbisCutter        *self,
                                                         guint                   n_bytes,
                                                         guint8                 *bytes);
gboolean          gsl_vorbis_cutter_ogg_eos             (GslVorbisCutter        *self);
void              gsl_vorbis_cutter_destroy             (GslVorbisCutter        *self);
G_END_DECLS
#endif /* __GSL_VORBIS_CUTTER_H__ */
