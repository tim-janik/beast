/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Stefan Westerfeld and Tim Janik
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

#include <gsl/gslmath.h>
#include <gsl/gsldatahandle.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- structures --- */
#define GSL_DATA_HANDLE_PEEK_BUFFER     (8192)
typedef struct
{
  gint    dir;   /* initialize direction to -1 or +1 (or 0 for random access) */
  GslLong start; /* initialize to 0 */
  GslLong end;   /* initialize to 0 */
  gfloat  data[GSL_DATA_HANDLE_PEEK_BUFFER];
} GslDataPeekBuffer;
typedef struct
{
  GslLong head_skip;
  GslLong tail_cut;
  GslLong min_loop;
  GslLong max_loop;
} GslLoopSpec;


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


/* --- data handle utils --- */
static inline gfloat gsl_data_handle_peek_value	(GslDataHandle		*dhandle,
						 GslLong		 position,
						 GslDataPeekBuffer	*peekbuf);
gint /* errno */     gsl_data_handle_dump	(GslDataHandle		*dhandle,
						 gint			 fd,
						 GslWaveFormatType	 format,
						 guint			 byte_order);
gint /* errno */     gsl_data_handle_dump_wav	(GslDataHandle		*dhandle,
						 gint			 fd,
						 guint			 n_bits,
						 guint			 n_channels,
						 guint			 sample_freq);


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
      GslFpuState fpu;
      gfloat v;
      gint16 vi16;
      guint16 vu16;
      guint32 vu32;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *u8++ = *src++ * 128. + 128.5;
      while (src < bound);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 128.;
          *i8++ = gsl_ftoi (v);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *u16++ = *src++ * 2048. + 2048.5;
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = *src++ * 2048. + 2048.5;
          *u16++ = GUINT16_SWAP_LE_BE (vu16);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          *i16++ = gsl_ftoi (v);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi16 = gsl_ftoi (v);
          *i16++ = GUINT16_SWAP_LE_BE (vi16);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *u16++ = *src++ * 32768. + 32768.5;
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = *src++ * 32768. + 32768.5;
          *u16++ = GUINT16_SWAP_LE_BE (vu16);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
	  v *= 32768.;
          *i16++ = gsl_ftoi (v);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi16 = gsl_ftoi (v);
          *i16++ = GUINT16_SWAP_LE_BE (vi16);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
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
      GslFpuState fpu;
      gfloat v;
      guint32 vu32;
      gint32 vi32;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = *src++ * 128. + 128.5;
          *u8++ = CLAMP (vi32, 0, 255);
        }
      while (src < bound);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 128.;
          vi32 = gsl_ftoi (v);
          *i8++ = CLAMP (vi32, -128, 127);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        {
          vi32 = *src++ * 2048. + 2048.5;
          *u16++ = CLAMP (vi32, 0, 4095);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = *src++ * 2048. + 2048.5;
          vi32 = CLAMP (vi32, 0, 4095);
          *u16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi32 = gsl_ftoi (v);
          *i16++ = CLAMP (vi32, -2048, 2047);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi32 = gsl_ftoi (v);
          vi32 = CLAMP (vi32, -2048, 2047);
          *i16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        {
          vi32 = *src++ * 32768. + 32768.5;
          *u16++ = CLAMP (vi32, 0, 65535);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = *src++ * 32768. + 32768.5;
          vi32 = CLAMP (vi32, 0, 65535);
          *u16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi32 = gsl_ftoi (v);
          vi32 = CLAMP (vi32, -32768, 32767);
          *i16++ = vi32;
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi32 = gsl_ftoi (v);
          vi32 = CLAMP (vi32, -32768, 32767);
          *i16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
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
      GslFpuState fpu;
      gdouble v;
      gint16 vi16;
      guint16 vu16;
      guint32 vu32;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        *u8++ = *src++ * 128. + 128.5;
      while (src < bound);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 128.;
          *i8++ = gsl_ftoi (v);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *u16++ = *src++ * 2048. + 2048.5;
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = *src++ * 2048. + 2048.5;
          *u16++ = GUINT16_SWAP_LE_BE (vu16);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          *i16++ = gsl_ftoi (v);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi16 = gsl_ftoi (v);
          *i16++ = GUINT16_SWAP_LE_BE (vi16);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        *u16++ = *src++ * 32768. + 32768.5;
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vu16 = *src++ * 32768. + 32768.5;
          *u16++ = GUINT16_SWAP_LE_BE (vu16);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          *i16++ = gsl_ftoi (v);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi16 = gsl_ftoi (v);
          *i16++ = GUINT16_SWAP_LE_BE (vi16);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
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
      GslFpuState fpu;
      gdouble v;
      guint32 vu32;
      gint32 vi32;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = *src++ * 128. + 128.5;
          *u8++ = CLAMP (vi32, 0, 255);
        }
      while (src < bound);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER == G_BYTE_ORDER):
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_8, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 128.;
          vi32 = gsl_ftoi (v);
          *i8++ = CLAMP (vi32, -128, 127);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        {
          vi32 = *src++ * 2048. + 2048.5;
          *u16++ = CLAMP (vi32, 0, 4095);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = *src++ * 2048. + 2048.5;
          vi32 = CLAMP (vi32, 0, 4095);
          *u16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER == G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi32 = gsl_ftoi (v);
          *i16++ = CLAMP (vi32, -2048, 2047);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_12, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 2048.;
          vi32 = gsl_ftoi (v);
          vi32 = CLAMP (vi32, -2048, 2047);
          *i16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      do
        {
          vi32 = *src++ * 32768. + 32768.5;
          *u16++ = CLAMP (vi32, 0, 65535);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_UNSIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      do
        {
          vi32 = *src++ * 32768. + 32768.5;
          vi32 = CLAMP (vi32, 0, 65535);
          *u16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER == G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi32 = gsl_ftoi (v);
          vi32 = CLAMP (vi32, -32768, 32767);
          *i16++ = vi32;
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
      return n_values << 1;
    case GSL_CONV_FORMAT (GSL_WAVE_FORMAT_SIGNED_16, G_BYTE_ORDER != G_BYTE_ORDER):
      gsl_fpu_setround (&fpu);
      do
        {
          v = *src++;
          v *= 32768.;
          vi32 = gsl_ftoi (v);
          vi32 = CLAMP (vi32, -32768, 32767);
          *i16++ = GUINT16_SWAP_LE_BE (vi32);
        }
      while (src < bound);
      gsl_fpu_restore (fpu);
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


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_DATA_UTILS_H__ */
