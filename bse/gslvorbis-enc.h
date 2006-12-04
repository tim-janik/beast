/* GSL - Generic Sound Layer
 * Copyright (C) 2001, 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GSL_VORBIS_ENC_H__
#define __GSL_VORBIS_ENC_H__

#include <bse/gslcommon.h>

G_BEGIN_DECLS

/* --- typedefs & enums --- */
typedef struct _GslVorbisEncoder GslVorbisEncoder;
/* rough quality to bitrate mappings at 44.1kHz mono:
 * -1) 32kBit  2) 70kBit  5)  96kBit  8) 140kBit
 *  0) 48kBit  3) 80kBit  6) 110kBit  9) 160kBit
 *  1) 60kBit  4) 86kBit  7) 120kBit 10) 240kBit
 * oggenc defaults to a quality setting of 3.0
 */

/* --- encoder API --- */
GslVorbisEncoder* gsl_vorbis_encoder_new                (void);
/* pre encoding */
void              gsl_vorbis_encoder_add_comment        (GslVorbisEncoder       *self,
                                                         const gchar            *utf8_comment);
void              gsl_vorbis_encoder_add_named_comment  (GslVorbisEncoder       *self,
                                                         const gchar            *ascii_tag_name,
                                                         const gchar            *utf8_comment);
void              gsl_vorbis_encoder_add_lcomment       (GslVorbisEncoder       *self,
                                                         const gchar            *latin1_comment);
void              gsl_vorbis_encoder_add_named_lcomment (GslVorbisEncoder       *self,
                                                         const gchar            *ascii_tag_name,
                                                         const gchar            *latin1_comment);
void              gsl_vorbis_encoder_set_quality        (GslVorbisEncoder       *self,
                                                         gfloat                  quality); /* -1..10 */
void              gsl_vorbis_encoder_set_bitrate        (GslVorbisEncoder       *self,
                                                         guint                   nominal_bps);
void              gsl_vorbis_encoder_set_n_channels     (GslVorbisEncoder       *self,
                                                         guint                   n_channels);
void              gsl_vorbis_encoder_set_sample_freq    (GslVorbisEncoder       *self,
                                                         guint                   sample_freq);
/* start encoding */
BseErrorType      gsl_vorbis_encoder_setup_stream       (GslVorbisEncoder       *self,
                                                         guint                   serial);
/* write unencoded data (must be channel aligned) */
void              gsl_vorbis_encoder_write_pcm          (GslVorbisEncoder       *self,
                                                         guint                   n_values,
                                                         gfloat                 *values);
/* (optional) incremental load distribution */
gboolean          gsl_vorbis_encoder_needs_processing   (GslVorbisEncoder       *self);
void              gsl_vorbis_encoder_process            (GslVorbisEncoder       *self);
/* finish feeding unencoded data */
void              gsl_vorbis_encoder_pcm_done           (GslVorbisEncoder       *self);
/* retrive encoded data */
guint             gsl_vorbis_encoder_read_ogg           (GslVorbisEncoder       *self,
                                                         guint                   n_bytes,
                                                         guint8                 *bytes);
/* test for end of stream */
gboolean          gsl_vorbis_encoder_ogg_eos            (GslVorbisEncoder       *self);
/* cleanup */
void              gsl_vorbis_encoder_destroy            (GslVorbisEncoder       *self);

/* retrive vendor version string */
gchar*            gsl_vorbis_encoder_version            (void);


G_END_DECLS

#endif /* __GSL_VORBIS_ENC_H__ */
