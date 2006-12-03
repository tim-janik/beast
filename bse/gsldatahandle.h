/* GSL - Generic Sound Layer
 * Copyright (C) 2001-2003 Tim Janik
 * Copyright (C) 2006 Stefan Westerfeld
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
#ifndef __GSL_DATA_HANDLE_H__
#define __GSL_DATA_HANDLE_H__

#include <bse/bseutils.h>

G_BEGIN_DECLS


/* --- macros --- */
#define	GSL_DATA_HANDLE_OPENED(handle)	    (((GslDataHandle*) (handle))->open_count > 0)
#define	GSL_DATA_HANDLE_READ_LINEAR(handle) (((GslDataHandle*) (handle))->vtable->coarse_seek != NULL)


/* --- typedefs & structures --- */
typedef struct {
  guint		n_channels;
  int64		n_values;
  gchar       **xinfos;
  guint         bit_depth : 8;
  guint         needs_cache : 1;
  gfloat        mix_freq;
} GslDataHandleSetup;
struct _GslDataHandle
{
  /* constant members */
  GslDataHandleFuncs *vtable;
  gchar		     *name;
  /* common members */
  BirnetMutex	      mutex;
  guint		      ref_count;
  guint		      open_count;
  /* opened data handle setup (open_count > 0) */
  GslDataHandleSetup  setup;
};
typedef void (*GslDataHandleRecurse)	(GslDataHandle		*data_handle,
					 gpointer		 data);
struct _GslDataHandleFuncs
{
  BseErrorType	 (*open)		(GslDataHandle		*data_handle,
					 GslDataHandleSetup	*setup);
  int64		 (*read)		(GslDataHandle		*data_handle,
					 int64			 voffset, /* in values */
					 int64			 n_values,
					 gfloat			*values);
  void		 (*close)		(GslDataHandle		*data_handle);
  GslDataHandle* (*get_source)          (GslDataHandle          *data_handle);
  int64          (*get_state_length)	(GslDataHandle	        *data_handle);
  void           (*destroy)		(GslDataHandle		*data_handle);
};



/* --- standard functions --- */
GslDataHandle*	  gsl_data_handle_ref		    (GslDataHandle	  *dhandle);
void		  gsl_data_handle_unref		    (GslDataHandle	  *dhandle);
BseErrorType	  gsl_data_handle_open		    (GslDataHandle	  *dhandle);
void		  gsl_data_handle_close		    (GslDataHandle	  *dhandle);
int64		  gsl_data_handle_length	    (GslDataHandle	  *data_handle);
#define	          gsl_data_handle_n_values(	     dh) \
						     gsl_data_handle_length (dh)
guint		  gsl_data_handle_n_channels	    (GslDataHandle	  *data_handle);
guint		  gsl_data_handle_bit_depth	    (GslDataHandle	  *data_handle);
gfloat		  gsl_data_handle_mix_freq	    (GslDataHandle	  *data_handle);
gfloat		  gsl_data_handle_osc_freq	    (GslDataHandle	  *data_handle);
const gchar*	  gsl_data_handle_name		    (GslDataHandle	  *data_handle);
int64		  gsl_data_handle_read		    (GslDataHandle	  *data_handle,
						     int64		   value_offset,
						     int64		   n_values,
						     gfloat		  *values);
int64		  gsl_data_handle_get_state_length  (GslDataHandle    *dhandle);
GslDataHandle*    gsl_data_handle_get_source	    (GslDataHandle    *dhandle);
GslDataHandle*	  gsl_data_handle_new_cut	    (GslDataHandle	  *src_handle,
						     int64		   cut_offset,
						     int64		   n_cut_values);
GslDataHandle*	  gsl_data_handle_new_crop	    (GslDataHandle	  *src_handle,
						     int64  	   n_head_cut,
						     int64		   n_tail_cut);
GslDataHandle*	  gsl_data_handle_new_reverse	    (GslDataHandle	  *src_handle);
GslDataHandle*	  gsl_data_handle_new_insert	    (GslDataHandle	  *src_handle,
						     guint             pasted_bit_depth,
						     int64		   insertion_offset,
						     int64		   n_paste_values,
						     const gfloat	  *paste_values,
						     void            (*free) (gpointer values));
GslDataHandle*	  gsl_data_handle_new_mem	    (guint		   n_channels,
						     guint             bit_depth,
						     gfloat            mix_freq,
						     gfloat            osc_freq,
						     int64		   n_values,
						     const gfloat	  *values,
						     void            (*free) (gpointer values));
GslDataHandle*	  gsl_data_handle_new_dcached	    (GslDataCache	  *dcache);
/* cheap and inefficient, testpurpose only */
GslDataHandle*	  gsl_data_handle_new_looped	    (GslDataHandle	  *src_handle,
						     int64		   loop_first,
						     int64		   loop_last);

/* --- factor 2 resampling datahandles --- */
GslDataHandle*	  bse_data_handle_new_upsample2	    (GslDataHandle  *src_handle,	// implemented in bsedatahandle-resample.cc
						     int             precision_bits);
GslDataHandle*	  bse_data_handle_new_downsample2   (GslDataHandle  *src_handle,
						     int             precision_bits);	// implemented in bsedatahandle-resample.cc

GslDataHandle*	  bse_data_handle_new_fir_highpass  (GslDataHandle *src_handle,		// implemented in bsedatahandle-fir.cc
						     gdouble        cutoff_freq,
						     guint          order);
GslDataHandle*	  bse_data_handle_new_fir_lowpass   (GslDataHandle *src_handle,		// implemented in bsedatahandle-fir.cc
						     gdouble        cutoff_freq,
						     guint          order);


/* --- xinfo handling --- */
GslDataHandle* gsl_data_handle_new_add_xinfos	    (GslDataHandle *src_handle,
						     gchar        **xinfos);
GslDataHandle* gsl_data_handle_new_remove_xinfos    (GslDataHandle *src_handle,
						     gchar        **xinfos);
GslDataHandle* gsl_data_handle_new_clear_xinfos	    (GslDataHandle *src_handle);


/* --- wave specific functions --- */
typedef enum    /*< skip >*/
{
  GSL_WAVE_FORMAT_NONE,
  GSL_WAVE_FORMAT_UNSIGNED_8,
  GSL_WAVE_FORMAT_SIGNED_8,
  GSL_WAVE_FORMAT_ALAW,
  GSL_WAVE_FORMAT_ULAW,
  GSL_WAVE_FORMAT_UNSIGNED_12,
  GSL_WAVE_FORMAT_SIGNED_12,
  GSL_WAVE_FORMAT_UNSIGNED_16,
  GSL_WAVE_FORMAT_SIGNED_16,
  GSL_WAVE_FORMAT_FLOAT,
  GSL_WAVE_FORMAT_LAST
} GslWaveFormatType;
#define GSL_WAVE_FORMAT_IS_LAW(f)       ((f) == GSL_WAVE_FORMAT_ALAW || (f) == GSL_WAVE_FORMAT_ULAW)

const gchar*      gsl_wave_format_to_string     (GslWaveFormatType format);
GslWaveFormatType gsl_wave_format_from_string   (const gchar      *string);
GslDataHandle*	  gsl_wave_handle_new		(const gchar	  *file_name,
						 guint		   n_channels,
						 GslWaveFormatType format,
						 guint		   byte_order,
                                                 gfloat            mix_freq,
                                                 gfloat            osc_freq,
						 int64		   byte_offset,
						 int64		   n_values,
                                                 gchar           **xinfos);
GslDataHandle*	  gsl_wave_handle_new_zoffset	(const gchar	  *file_name,
						 guint		   n_channels,
						 GslWaveFormatType format,
						 guint		   byte_order,
                                                 gfloat            mix_freq,
                                                 gfloat            osc_freq,
						 int64		   byte_offset,
						 int64		   byte_size,
                                                 gchar           **xinfos);
guint		  gsl_wave_format_bit_depth	(GslWaveFormatType format);
guint		  gsl_wave_format_byte_width	(GslWaveFormatType format);


/* --- data handle optimization jobs --- */
gboolean	gsl_data_handle_needs_cache	(GslDataHandle	*data_handle);


/* --- auxillary functions --- */
gboolean	gsl_data_handle_common_init	(GslDataHandle	  *dhandle,
						 const gchar	  *file_name);
void		gsl_data_handle_common_free	(GslDataHandle	  *dhandle);


G_END_DECLS

#endif /* __GSL_DATA_HANDLE_H__ */
