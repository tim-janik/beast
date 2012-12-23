/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2005 Tim Janik
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
#ifndef __GSL_DATA_HANDLE_VORBIS_H__
#define __GSL_DATA_HANDLE_VORBIS_H__


#include <bse/gslcommon.hh>
#include <bse/gsldatahandle.hh>

G_BEGIN_DECLS

/* --- reading vorbis files --- */
GslDataHandle* gsl_data_handle_new_ogg_vorbis_muxed   (const gchar      *file_name,
                                                       guint	         lbitstream,
                                                       gfloat            osc_freq);
GslDataHandle* gsl_data_handle_new_ogg_vorbis_zoffset (const gchar      *file_name,
                                                       gfloat            osc_freq,
                                                       GslLong           byte_offset,
                                                       GslLong           byte_size,
                                                       guint            *n_channelsp,
                                                       gfloat           *mix_freq_p);

/* --- writing vorbis files --- */
typedef struct GslVorbis1Handle GslVorbis1Handle;
GslVorbis1Handle* gsl_vorbis1_handle_new              (GslDataHandle    *ogg_vorbis_handle,
                                                       guint             serialno);
gint              gsl_vorbis1_handle_read             (GslVorbis1Handle *vorbis1, /* returns -errno || length */
                                                       guint             blength,
                                                       guint8           *buffer);
void              gsl_vorbis1_handle_destroy          (GslVorbis1Handle *vorbis1);
/* gsl_vorbis1_handle_put_wstore() calls gsl_vorbis1_handle_destroy()
 * on vorbis1 when sfi_wstore_destroy (wstore) is executed.
 */
void              gsl_vorbis1_handle_put_wstore       (GslVorbis1Handle *vorbis1,
                                                       SfiWStore        *wstore);
guint             gsl_vorbis_make_serialno            (void);

G_END_DECLS

#endif /* __GSL_DATA_HANDLE_VORBIS_H__ */
