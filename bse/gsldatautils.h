/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2004 Stefan Westerfeld and Tim Janik
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
#ifndef __GSL_DATA_UTILS_H__
#define __GSL_DATA_UTILS_H__

#include <bse/bsemath.h>
#include <bse/gsldatahandle.h>

G_BEGIN_DECLS

/* --- structures --- */
#define GSL_DATA_HANDLE_PEEK_BUFFER     (2048)
typedef struct
{
  gint    dir;   /* initialize direction to -1 or +1 (or 0 for random access) */
  GslLong start; /* initialize to 0 */
  GslLong end;   /* initialize to 0 */
  gfloat  data[GSL_DATA_HANDLE_PEEK_BUFFER];
} GslDataPeekBuffer;
typedef struct
{
  GslLong head_skip;	/* FIXME: remove this */
  GslLong tail_cut;
  GslLong min_loop;
  GslLong max_loop;
} GslLoopSpec;	/* rename this to GslData... */


/* --- data utils --- */
gboolean	gsl_data_detect_signal		(GslDataHandle		*handle,
						 GslLong		*sigstart,
						 GslLong		*sigend);
GslLong		gsl_data_find_sample		(GslDataHandle		*dhandle,
						 gfloat			 min_value,
						 gfloat			 max_value,
						 GslLong		 start_offset,
						 gint			 direction);
gboolean	gsl_data_find_tailmatch		(GslDataHandle		*dhandle,
						 const GslLoopSpec	*lspec,
						 GslLong		*loop_start_p,
						 GslLong		*loop_end_p);
GslLong		gsl_data_find_block		(GslDataHandle		*handle,
						 guint			 n_values,
						 const gfloat		*values,
						 gfloat			 epsilon);
gfloat*         gsl_data_make_fade_ramp         (GslDataHandle          *handle,
                                                 GslLong                 min_pos, /* *= 0.0 + delta */
                                                 GslLong                 max_pos, /* *= 1.0 - delta */
                                                 GslLong                *length_p);


/* --- data handle utils --- */
static inline gfloat gsl_data_handle_peek_value	(GslDataHandle		*dhandle,
						 GslLong		 position,
						 GslDataPeekBuffer	*peekbuf);
gint /* errno */     gsl_data_handle_dump	(GslDataHandle		*dhandle,
						 gint			 fd,
						 GslWaveFormatType	 format,
						 guint			 byte_order);
gint /* errno */     gsl_wave_file_dump_header	(gint			 fd,
						 guint			 n_data_bytes,
						 guint			 n_bits,
						 guint			 n_channels,
						 guint		         sample_freq);
gint /* errno */     gsl_wave_file_patch_length (gint			 fd,
						 guint			 n_data_bytes);
gint /* errno */     gsl_data_handle_dump_wav	(GslDataHandle		*dhandle,
						 gint			 fd,
						 guint			 n_bits,
						 guint			 n_channels,
						 guint			 sample_freq);
gint /* errno */     gsl_wave_file_dump_data	(gint			 fd,
						 guint			 n_bits,
						 guint			 n_values,
						 const gfloat		*values);
void		     gsl_data_handle_dump_wstore(GslDataHandle		*dhandle,
						 SfiWStore		*wstore,
						 GslWaveFormatType	 format,
						 guint			 byte_order);


/* --- conversion utils --- */
static inline guint   gsl_conv_from_float	(GslWaveFormatType format,
						 guint             byte_order,
						 const gfloat     *src,
						 gpointer          dest,
						 guint             n_values);
static inline guint   gsl_conv_from_float_clip	(GslWaveFormatType format,
						 guint             byte_order,
						 const gfloat     *src,
						 gpointer          dest,
						 guint             n_values);
static inline void    gsl_conv_to_float		(GslWaveFormatType format,
						 guint             byte_order,
						 gconstpointer     src,
						 gfloat           *dest,
						 guint             n_values);
static inline guint   gsl_conv_from_double	(GslWaveFormatType format,
						 guint             byte_order,
						 const gdouble    *src,
						 gpointer          dest,
						 guint             n_values);
static inline guint   gsl_conv_from_double_clip	(GslWaveFormatType format,
						 guint             byte_order,
						 const gdouble    *src,
						 gpointer          dest,
						 guint             n_values);
static inline void    gsl_conv_to_double	(GslWaveFormatType format,
						 guint             byte_order,
						 gconstpointer     src,
						 gdouble          *dest,
						 guint             n_values);
static inline gint16  gsl_alaw_to_pcm           (gint8             alawv);
static inline gint16  gsl_ulaw_to_pcm           (gint8             ulawv);


/* --- clipping --- */
typedef struct
{
  guint  produce_info : 1;
  gfloat threshold;     /* 0..+1 */
  guint  head_samples;
  guint  tail_samples;
  guint  fade_samples;
  guint  pad_samples;
  guint  tail_silence;
} GslDataClipConfig;
typedef struct
{
  GslDataHandle *dhandle;
  guint          clipped_to_0length : 1;        /* no data above threshold */
  guint          head_detected : 1;             /* found head_samples silence */
  guint          tail_detected : 1;             /* found tail_samples silence */
  guint          clipped_head : 1;
  guint          clipped_tail : 1;
  BseErrorType   error;
} GslDataClipResult;

BseErrorType    gsl_data_clip_sample    (GslDataHandle     *dhandle,
                                         GslDataClipConfig *cconfig,
                                         GslDataClipResult *result);


/* --- misc implementations --- */
gfloat  gsl_data_peek_value_f   (GslDataHandle     *dhandle,
				 GslLong            pos,
				 GslDataPeekBuffer *peekbuf);

static inline gfloat
gsl_data_handle_peek_value (GslDataHandle     *dhandle,
			    GslLong	       position,
			    GslDataPeekBuffer *peekbuf)
{
  return (position >= peekbuf->start && position < peekbuf->end ?
	  peekbuf->data[position - peekbuf->start] :
	  gsl_data_peek_value_f (dhandle, position, peekbuf));
}

#define	GSL_CONV_FORMAT(format, endian_flag)	(((endian_flag) << 16) | ((format) & 0xffff))

static inline guint     /* returns number of bytes used in dest */
gsl_conv_from_float (GslWaveFormatType format,
                     guint             byte_order,
                     const gfloat     *src,
                     gpointer          dest,
                     guint             n_values)
{
  gint8 *i8 = (gint8*) dest;
  guint8 *u8 = (guint8*) dest;
  gint16 *i16 = (gint16*) dest;
  guint16 *u16 = (guint16*) dest;
  guint32 *u32dest = (guint32*) dest;
  const gfloat *bound = src + n_values;
  guint32 *u32src = (guint32*) src, *u32bound = (guint32*) bound;
  
  if (!n_values)
    return 0;
  
  switch (GSL_CONV_FORMAT (format, byte_order == G_BYTE_ORDER))
    {
      BseFpuState fpu;
      gfloat v;
      gint16 vi16;
      guint16 vu16;
      guint32 vu32;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *u8++ = bse_dtoi (*src++ * 128. + 128);
      while (src < bound);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 128.;
          *i8++ = bse_dtoi (v);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *u16++ = bse_dtoi (*src++ * 2048. + 2048);
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = bse_dtoi (*src++ * 2048. + 2048);
          *u16++ = GUINT16_SWAP_LE_BE (vu16);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          *i16++ = bse_dtoi (v);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi16 = bse_dtoi (v);
          *i16++ = GUINT16_SWAP_LE_BE (vi16);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *u16++ = bse_dtoi (*src++ * 32768. + 32768);
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = bse_dtoi (*src++ * 32768. + 32768);
          *u16++ = GUINT16_SWAP_LE_BE (vu16);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
	  v *= 32768.;
          *i16++ = bse_dtoi (v);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi16 = bse_dtoi (v);
          *i16++ = GUINT16_SWAP_LE_BE (vi16);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER == G_BYTE_ORDER):
      return n_values << 2;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu32 = *u32src++;
          *u32dest++ = GUINT32_SWAP_LE_BE (vu32);
        }
      while (u32src < u32bound);
      return n_values << 2;
    default:
      g_assert_not_reached ();
      return 0;
    }
}

static inline guint     /* returns number of bytes used in dest */
gsl_conv_from_float_clip (GslWaveFormatType format,
                          guint             byte_order,
                          const gfloat     *src,
                          gpointer          dest,
                          guint             n_values)
{
  gint8 *i8 = (gint8*) dest;
  guint8 *u8 = (guint8*) dest;
  gint16 *i16 = (gint16*) dest;
  guint16 *u16 = (guint16*) dest;
  guint32 *u32dest = (guint32*) dest;
  const gfloat *bound = src + n_values;
  guint32 *u32src = (guint32*) src, *u32bound = (guint32*) bound;
  
  if (!n_values)
    return 0;
  
  switch (GSL_CONV_FORMAT (format, byte_order == G_BYTE_ORDER))
    {
      BseFpuState fpu;
      gfloat v;
      guint32 vu32;
      gint32 vi32;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = bse_dtoi (*src++ * 128. + 128);
          *u8++ = CLAMP (vi32, 0, 255);
        }
      while (src < bound);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 128.;
          vi32 = bse_dtoi (v);
          *i8++ = CLAMP (vi32, -128, 127);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        {
          vi32 = bse_dtoi (*src++ * 2048. + 2048);
          *u16++ = CLAMP (vi32, 0, 4095);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = bse_dtoi (*src++ * 2048. + 2048);
          vi32 = CLAMP (vi32, 0, 4095);
          *u16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi32 = bse_dtoi (v);
          *i16++ = CLAMP (vi32, -2048, 2047);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi32 = bse_dtoi (v);
          vi32 = CLAMP (vi32, -2048, 2047);
          *i16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        {
          vi32 = bse_dtoi (*src++ * 32768. + 32768);
          *u16++ = CLAMP (vi32, 0, 65535);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = bse_dtoi (*src++ * 32768. + 32768);
          vi32 = CLAMP (vi32, 0, 65535);
          *u16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi32 = bse_dtoi (v);
          vi32 = CLAMP (vi32, -32768, 32767);
          *i16++ = vi32;
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi32 = bse_dtoi (v);
          vi32 = CLAMP (vi32, -32768, 32767);
          *i16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER == G_BYTE_ORDER):
      return n_values << 2;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu32 = *u32src++;
          *u32dest++ = GUINT32_SWAP_LE_BE (vu32);
        }
      while (u32src < u32bound);
      return n_values << 2;
    default:
      g_assert_not_reached ();
      return 0;
    }
}

#define GSL_ALAW_MAX    (0x7e00)
static inline gint16
gsl_alaw_to_pcm (gint8 alawv)
{
  static const short alaw2pcm_table[128] = {
    0x1580, 0x1480, 0x1780, 0x1680, 0x1180, 0x1080, 0x1380, 0x1280,
    0x1d80, 0x1c80, 0x1f80, 0x1e80, 0x1980, 0x1880, 0x1b80, 0x1a80,
    0x0ac0, 0x0a40, 0x0bc0, 0x0b40, 0x08c0, 0x0840, 0x09c0, 0x0940,
    0x0ec0, 0x0e40, 0x0fc0, 0x0f40, 0x0cc0, 0x0c40, 0x0dc0, 0x0d40,
    0x5600, 0x5200, 0x5e00, 0x5a00, 0x4600, 0x4200, 0x4e00, 0x4a00,
    0x7600, 0x7200, 0x7e00, 0x7a00, 0x6600, 0x6200, 0x6e00, 0x6a00,
    0x2b00, 0x2900, 0x2f00, 0x2d00, 0x2300, 0x2100, 0x2700, 0x2500,
    0x3b00, 0x3900, 0x3f00, 0x3d00, 0x3300, 0x3100, 0x3700, 0x3500,
    0x0158, 0x0148, 0x0178, 0x0168, 0x0118, 0x0108, 0x0138, 0x0128,
    0x01d8, 0x01c8, 0x01f8, 0x01e8, 0x0198, 0x0188, 0x01b8, 0x01a8,
    0x0058, 0x0048, 0x0078, 0x0068, 0x0018, 0x0008, 0x0038, 0x0028,
    0x00d8, 0x00c8, 0x00f8, 0x00e8, 0x0098, 0x0088, 0x00b8, 0x00a8,
    0x0560, 0x0520, 0x05e0, 0x05a0, 0x0460, 0x0420, 0x04e0, 0x04a0,
    0x0760, 0x0720, 0x07e0, 0x07a0, 0x0660, 0x0620, 0x06e0, 0x06a0,
    0x02b0, 0x0290, 0x02f0, 0x02d0, 0x0230, 0x0210, 0x0270, 0x0250,
    0x03b0, 0x0390, 0x03f0, 0x03d0, 0x0330, 0x0310, 0x0370, 0x0350,
  };
  return alawv < 0 ? alaw2pcm_table[128 + alawv] : -alaw2pcm_table[alawv];
}

#define GSL_ULAW_MAX    (0x7d7c)
static inline gint16
gsl_ulaw_to_pcm (gint8 ulawv)
{
  static const short ulaw2pcm_table[128] = {
    0x7d7c, 0x797c, 0x757c, 0x717c, 0x6d7c, 0x697c, 0x657c, 0x617c,
    0x5d7c, 0x597c, 0x557c, 0x517c, 0x4d7c, 0x497c, 0x457c, 0x417c,
    0x3e7c, 0x3c7c, 0x3a7c, 0x387c, 0x367c, 0x347c, 0x327c, 0x307c,
    0x2e7c, 0x2c7c, 0x2a7c, 0x287c, 0x267c, 0x247c, 0x227c, 0x207c,
    0x1efc, 0x1dfc, 0x1cfc, 0x1bfc, 0x1afc, 0x19fc, 0x18fc, 0x17fc,
    0x16fc, 0x15fc, 0x14fc, 0x13fc, 0x12fc, 0x11fc, 0x10fc, 0x0ffc,
    0x0f3c, 0x0ebc, 0x0e3c, 0x0dbc, 0x0d3c, 0x0cbc, 0x0c3c, 0x0bbc,
    0x0b3c, 0x0abc, 0x0a3c, 0x09bc, 0x093c, 0x08bc, 0x083c, 0x07bc,
    0x075c, 0x071c, 0x06dc, 0x069c, 0x065c, 0x061c, 0x05dc, 0x059c,
    0x055c, 0x051c, 0x04dc, 0x049c, 0x045c, 0x041c, 0x03dc, 0x039c,
    0x036c, 0x034c, 0x032c, 0x030c, 0x02ec, 0x02cc, 0x02ac, 0x028c,
    0x026c, 0x024c, 0x022c, 0x020c, 0x01ec, 0x01cc, 0x01ac, 0x018c,
    0x0174, 0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104,
    0x00f4, 0x00e4, 0x00d4, 0x00c4, 0x00b4, 0x00a4, 0x0094, 0x0084,
    0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050, 0x0048, 0x0040,
    0x0038, 0x0030, 0x0028, 0x0020, 0x0018, 0x0010, 0x0008, 0x0000,
  };
  return ulawv < 0 ? ulaw2pcm_table[128 + ulawv] : -ulaw2pcm_table[ulawv];
}

static inline void
gsl_conv_to_float (GslWaveFormatType format,
                   guint             byte_order,
                   gconstpointer     src,
                   gfloat           *dest,
                   guint             n_values)
{
  guint8 *u8 = (guint8*) src;
  gint8 *i8 = (gint8*) src;
  guint16 *u16 = (guint16*) src;
  gint16 *i16 = (gint16*) src;
  guint32 *u32src = (guint32*) src;
  gfloat *bound = dest + n_values;
  guint32 *u32dest = (guint32*) dest, *u32bound = (guint32*) bound;
  
  if (!n_values)
    return;
  
  switch (GSL_CONV_FORMAT (format, byte_order == G_BYTE_ORDER))
    {
      gint16 vi16;
      guint16 vu16;
      guint32 vu32;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *dest++ = (*u8++ - 128) * (1. / 128.);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *dest++ = *i8++ * (1. / 128.);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_ALAW, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_ALAW, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *dest++ = gsl_alaw_to_pcm (*i8++) * (1.0 / GSL_ALAW_MAX);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_ULAW, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_ULAW, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *dest++ = gsl_ulaw_to_pcm (*i8++) * (1.0 / GSL_ULAW_MAX);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *dest++ = ((*u16++ & 0x0fff) - 2048) * (1. / 2048.);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = *u16++;
          *dest++ = ((GUINT16_SWAP_LE_BE (vu16) & 0x0fff) - 2048) * (1. / 2048.);
        }
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        {
          vi16 = *i16++;
          *dest++ = CLAMP (vi16, -2048, 2048) * (1. / 2048.);
        }
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi16 = *i16++;
          vi16 = GUINT16_SWAP_LE_BE (vi16);
          *dest++ = CLAMP (vi16, -2048, 2048) * (1. / 2048.);
        }
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *dest++ = (*u16++ - 32768) * (1. / 32768.);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = *u16++;
          *dest++ = (GUINT16_SWAP_LE_BE (vu16) - 32768) * (1. / 32768.);
        }
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *dest++ = *i16++ * (1. / 32768.);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi16 = *i16++;
          *dest++ = GUINT16_SWAP_LE_BE (vi16) * (1. / 32768.);
        }
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER == G_BYTE_ORDER):
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu32 = *u32src++;
          *u32dest++ = GUINT32_SWAP_LE_BE (vu32);
        }
      while (u32dest < u32bound);
      break;
    default:
      g_assert_not_reached ();
    }
}

/* same as above with s/float/double */
static inline guint     /* returns number of bytes used in dest */
gsl_conv_from_double (GslWaveFormatType format,
                      guint             byte_order,
                      const gdouble    *src,
                      gpointer          dest,
                      guint             n_values)
{
  gint8 *i8 = (gint8*) dest;
  guint8 *u8 = (guint8*) dest;
  gint16 *i16 = (gint16*) dest;
  guint16 *u16 = (guint16*) dest;
  guint32 *u32dest = (guint32*) dest;
  const gdouble *bound = src + n_values;
  guint32 *u32src = (guint32*) src, *u32bound = (guint32*) bound;
  
  if (!n_values)
    return 0;
  
  switch (GSL_CONV_FORMAT (format, byte_order == G_BYTE_ORDER))
    {
      BseFpuState fpu;
      gdouble v;
      gint16 vi16;
      guint16 vu16;
      guint32 vu32;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *u8++ = bse_dtoi (*src++ * 128. + 128);
      while (src < bound);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 128.;
          *i8++ = bse_dtoi (v);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *u16++ = bse_dtoi (*src++ * 2048. + 2048);
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = bse_dtoi (*src++ * 2048. + 2048);
          *u16++ = GUINT16_SWAP_LE_BE (vu16);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          *i16++ = bse_dtoi (v);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi16 = bse_dtoi (v);
          *i16++ = GUINT16_SWAP_LE_BE (vi16);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *u16++ = bse_dtoi (*src++ * 32768. + 32768);
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = bse_dtoi (*src++ * 32768. + 32768);
          *u16++ = GUINT16_SWAP_LE_BE (vu16);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          *i16++ = bse_dtoi (v);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi16 = bse_dtoi (v);
          *i16++ = GUINT16_SWAP_LE_BE (vi16);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER == G_BYTE_ORDER):
      return n_values << 2;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu32 = *u32src++;
          *u32dest++ = GUINT32_SWAP_LE_BE (vu32);
        }
      while (u32src < u32bound);
      return n_values << 2;
    default:
      g_assert_not_reached ();
      return 0;
    }
}

static inline guint     /* returns number of bytes used in dest */
gsl_conv_from_double_clip (GslWaveFormatType format,
			   guint             byte_order,
			   const gdouble    *src,
			   gpointer          dest,
			   guint             n_values)
{
  gint8 *i8 = (gint8*) dest;
  guint8 *u8 = (guint8*) dest;
  gint16 *i16 = (gint16*) dest;
  guint16 *u16 = (guint16*) dest;
  guint32 *u32dest = (guint32*) dest;
  const gdouble *bound = src + n_values;
  guint32 *u32src = (guint32*) src, *u32bound = (guint32*) bound;
  
  if (!n_values)
    return 0;
  
  switch (GSL_CONV_FORMAT (format, byte_order == G_BYTE_ORDER))
    {
      BseFpuState fpu;
      gdouble v;
      guint32 vu32;
      gint32 vi32;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = bse_dtoi (*src++ * 128. + 128);
          *u8++ = CLAMP (vi32, 0, 255);
        }
      while (src < bound);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 128.;
          vi32 = bse_dtoi (v);
          *i8++ = CLAMP (vi32, -128, 127);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        {
          vi32 = bse_dtoi (*src++ * 2048. + 2048);
          *u16++ = CLAMP (vi32, 0, 4095);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = bse_dtoi (*src++ * 2048. + 2048);
          vi32 = CLAMP (vi32, 0, 4095);
          *u16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi32 = bse_dtoi (v);
          *i16++ = CLAMP (vi32, -2048, 2047);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi32 = bse_dtoi (v);
          vi32 = CLAMP (vi32, -2048, 2047);
          *i16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        {
          vi32 = bse_dtoi (*src++ * 32768. + 32768);
          *u16++ = CLAMP (vi32, 0, 65535);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = bse_dtoi (*src++ * 32768. + 32768);
          vi32 = CLAMP (vi32, 0, 65535);
          *u16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi32 = bse_dtoi (v);
          vi32 = CLAMP (vi32, -32768, 32767);
          *i16++ = vi32;
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      bse_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi32 = bse_dtoi (v);
          vi32 = CLAMP (vi32, -32768, 32767);
          *i16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      bse_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER == G_BYTE_ORDER):
      return n_values << 2;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu32 = *u32src++;
          *u32dest++ = GUINT32_SWAP_LE_BE (vu32);
        }
      while (u32src < u32bound);
      return n_values << 2;
    default:
      g_assert_not_reached ();
      return 0;
    }
}

static inline void
gsl_conv_to_double (GslWaveFormatType format,
		    guint             byte_order,
		    gconstpointer     src,
		    gdouble          *dest,
		    guint             n_values)
{
  guint8 *u8 = (guint8*) src;
  gint8 *i8 = (gint8*) src;
  guint16 *u16 = (guint16*) src;
  gint16 *i16 = (gint16*) src;
  guint32 *u32src = (guint32*) src;
  gdouble *bound = dest + n_values;
  guint32 *u32dest = (guint32*) dest, *u32bound = (guint32*) bound;
  
  if (!n_values)
    return;
  
  switch (GSL_CONV_FORMAT (format, byte_order == G_BYTE_ORDER))
    {
      gint16 vi16;
      guint16 vu16;
      guint32 vu32;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *dest++ = (*u8++ - 128) * (1. / 128.);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *dest++ = *i8++ * (1. / 128.);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_ALAW, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_ALAW, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *dest++ = gsl_alaw_to_pcm (*i8++) * (1.0 / GSL_ALAW_MAX);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_ULAW, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_ULAW, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *dest++ = gsl_ulaw_to_pcm (*i8++) * (1.0 / GSL_ULAW_MAX);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *dest++ = ((*u16++ & 0x0fff) - 2048) * (1. / 2048.);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = *u16++;
          *dest++ = ((GUINT16_SWAP_LE_BE (vu16) & 0x0fff) - 2048) * (1. / 2048.);
        }
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        {
          vi16 = *i16++;
          *dest++ = CLAMP (vi16, -2048, 2048) * (1. / 2048.);
        }
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi16 = *i16++;
          vi16 = GUINT16_SWAP_LE_BE (vi16);
          *dest++ = CLAMP (vi16, -2048, 2048) * (1. / 2048.);
        }
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *dest++ = (*u16++ - 32768) * (1. / 32768.);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = *u16++;
          *dest++ = (GUINT16_SWAP_LE_BE (vu16) - 32768) * (1. / 32768.);
        }
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *dest++ = *i16++ * (1. / 32768.);
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi16 = *i16++;
          *dest++ = GUINT16_SWAP_LE_BE (vi16) * (1. / 32768.);
        }
      while (dest < bound);
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER == G_BYTE_ORDER):
      break;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_FLOAT, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu32 = *u32src++;
          *u32dest++ = GUINT32_SWAP_LE_BE (vu32);
        }
      while (u32dest < u32bound);
      break;
    default:
      g_assert_not_reached ();
    }
}

G_END_DECLS

#endif /* __GSL_DATA_UTILS_H__ */

/* vim:set ts=8 sts=2 sw=2: */
