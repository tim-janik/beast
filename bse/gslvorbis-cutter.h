/* GSL - Generic Sound Layer
 * Copyright (C) 2001, 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __GSL_VORBIS_CUTTER_H__
#define __GSL_VORBIS_CUTTER_H__

#include <bse/gslcommon.h>

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
