/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Tim Janik
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
#ifndef __SFI_TIME_H__
#define __SFI_TIME_H__

#include <sfi/sfivalues.hh>

G_BEGIN_DECLS

/* --- time (unix micro seconds) --- */
#define	SFI_USEC_FACTOR		((SfiTime) 1000000)		/* 64bit wide */
#define	SFI_MIN_TIME		(631152000 * SFI_USEC_FACTOR)	/* 1990-01-01 00:00:00 UTC */
#define	SFI_MAX_TIME		(2147483647 * SFI_USEC_FACTOR)	/* 2038-01-19 03:14:07 UTC */


/* --- functions --- */
SfiTime	sfi_time_system		 (void);	/* utc */
SfiTime	sfi_time_to_utc		 (SfiTime	 ustime);
SfiTime	sfi_time_from_utc	 (SfiTime	 ustime);
gchar*	sfi_time_to_string	 (SfiTime	 ustime);
gchar*	sfi_time_to_nice_string	 (SfiTime	 ustime,
				  const gchar   *elements);
SfiTime	sfi_time_from_string	 (const gchar	*time_string);
SfiTime	sfi_time_from_string_err (const gchar	*time_string,
				  gchar	       **error_p);


/* --- internal --- */
void	_sfi_init_time	(void);


G_END_DECLS

#endif /* __SFI_TIME_H__ */

/* vim:set ts=8 sts=2 sw=2: */
