/* BirnetCPU
 * Copyright (C) 2006 Tim Janik
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
#ifndef __BIRNET_CPU_H__
#define __BIRNET_CPU_H__

#include <birnet/birnetcore.h>

G_BEGIN_DECLS

typedef struct {
  /* architecture name */
  const char *machine;
  /* CPU Vendor ID */
  const char *cpu_vendor;
  /* CPU features on X86 */
  BirnetUInt x86_fpu : 1, x86_tsc    : 1, x86_htt   : 1;
  BirnetUInt x86_mmx : 1, x86_mmxext : 1, x86_3dnow : 1, x86_3dnowext : 1;
  BirnetUInt x86_sse : 1, x86_sse2   : 1, x86_sse3  : 1, x86_ssesys   : 1;
} BirnetCPUInfo;

/* --- functions --- */
const BirnetCPUInfo*	birnet_cpu_info		(void);
gchar*			birnet_cpu_info_string	(const BirnetCPUInfo *cpu_info);

/* --- implementation --- */
void	_birnet_init_cpuinfo	(void);

G_END_DECLS

#endif /* __BIRNET_CPU_H__ */

/* vim:set ts=8 sts=2 sw=2: */
