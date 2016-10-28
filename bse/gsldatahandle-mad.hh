// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GSL_DATA_HANDLE_MAD_H__
#define __GSL_DATA_HANDLE_MAD_H__

#include <bse/gslcommon.hh>
#include <bse/gsldatahandle.hh>

/* linear-read handle. needs buffering handle wrapper
 */
GslDataHandle*	gsl_data_handle_new_mad		(const gchar  *file_name,
                                                 gfloat        osc_freq);
GslDataHandle*	gsl_data_handle_new_mad_err	(const gchar  *file_name,
                                                 gfloat        osc_freq,
                                                 Bse::Error *error);
Bse::Error	gsl_data_handle_mad_testopen	(const gchar  *file_name,
						 guint        *n_channels,
						 gfloat       *mix_freq);
const gchar*    gsl_data_handle_mad_version     (void);

#endif /* __GSL_DATA_HANDLE_MAD_H__ */
