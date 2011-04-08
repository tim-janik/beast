/* BSE - Better Sound Engine
 * Copyright (C) 2002-2003 Tim Janik
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
#ifndef __BSE_STANDARD_SYNTHS_H__
#define __BSE_STANDARD_SYNTHS_H__

#include        <bse/bseproject.h>

G_BEGIN_DECLS


GSList*	bse_standard_synth_get_list	(void);
gchar*	bse_standard_synth_inflate	(const gchar	*synth_name,
					 guint		*text_len);


G_END_DECLS


#endif /* __BSE_STANDARD_SYNTHS_H__ */
