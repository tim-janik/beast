// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
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
