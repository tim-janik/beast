/* GSL - Generic Sound Layer
 * Copyright (C) 2001 Tim Janik
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
#ifndef __GSL_LOOPER_H__
#define __GSL_LOOPER_H__

#include <gsl/gsldefs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- cache single sample access --- */
#define GSL_DATA_HANDLE_PEEK_BUFFER	(8192)
typedef struct {
  gint    dir;	    /* initialize this to -1 or +1 (or 0 for random access) */
  GslLong start;    /* initialize to 0 */
  GslLong end;	    /* initialize to 0 */
  gfloat  data[GSL_DATA_HANDLE_PEEK_BUFFER];
} GslDataPeekBuffer;

/* carefull, this macro evaluates arguments multiple times */
#define	GSL_DATA_PEEK_VALUE(dhandle, pos, peekbuf)	(	\
  ((pos) >= (peekbuf)->start && (pos) < (peekbuf)->end) ?	\
    (peekbuf)->data[(pos) - (peekbuf)->start] :			\
    gsl_data_peek_value_f ((dhandle), (pos), (peekbuf)))

/* macro fallback */
gfloat	gsl_data_peek_value_f	(GslDataHandle     *dhandle,
				 GslLong            pos,
				 GslDataPeekBuffer *peekbuf);


/* --- loop/sample finder --- */
typedef struct {
  GslLong head_skip;
  GslLong tail_cut;
  GslLong min_loop;
  GslLong max_loop;
} GslLoopSpec;

gboolean	gsl_data_find_tailmatch	(GslDataHandle		*dhandle,
					 const GslLoopSpec	*lspec,
					 GslLong		*loop_start_p,
					 GslLong		*loop_end_p);
GslLong		gsl_data_find_sample	(GslDataHandle		*dhandle,
					 gfloat			 min_value,
					 gfloat			 max_value,
					 GslLong		 start_offset,
					 gint			 direction);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSL_LOOPER_H__ */
