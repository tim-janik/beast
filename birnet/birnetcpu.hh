/* BirnetCPU
 * Copyright (C) 2006 Tim Janik
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
#ifndef __BIRNET_CPU_HH__
#define __BIRNET_CPU_HH__

#include <birnet/birnetutils.hh>

namespace Birnet {

typedef BirnetCPUInfo CPUInfo;

/* --- functions --- */
CPUInfo cpu_info	(void);
String  cpu_info_string	(const CPUInfo &cpu_info);

/* --- implementation --- */
void	_birnet_init_cpuinfo	(void);

} // Birnet

#endif /* __BIRNET_CPU_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
