/* BSW-SCM - Bedevilled Sound Engine Scheme Wrapper
 * Copyright (C) 2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSW_SCM_INTERP_H__
#define __BSW_SCM_INTERP_H__

#include <bsw/bsw.h>
#include <guile/gh.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* guard around non-reentrant code portions,
 * with incremental int-blocking. guile recovers
 * from intercepted defer/allow pairs.
 */
#define	BSW_SCM_DEFER_INTS()	SCM_REDEFER_INTS
#define	BSW_SCM_ALLOW_INTS()	SCM_REALLOW_INTS

/* conversion */
#define	bsw_scm_to_str(sval)	gh_scm2newstr ((sval), NULL)


/* --- prototypes --- */
SCM	bsw_scm_from_enum	(gint		eval,
				 GType		type);
void	bsw_scm_interp_init	(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_SCM_INTERP_H__ */
