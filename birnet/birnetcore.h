/* Birnet
 * Copyright (C) 2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BIRNET_CORE_HH__
#define __BIRNET_CORE_HH__

#include <stdbool.h>
#include <glib.h>
#include <birnet/birnetconfig.h>

/* provide measures to guard C code for C++ */
#ifdef	__cplusplus
#  define BIRNET_EXTERN_C_BEGIN()	extern "C" {
#  define BIRNET_EXTERN_C_END()	}
#else
#  define BIRNET_EXTERN_C_BEGIN()
#  define BIRNET_EXTERN_C_END()
#endif


/* --- C specifics --- */
BIRNET_EXTERN_C_BEGIN();

/* --- birnet initialization --- */
void	birnet_init (const gchar	*prg_name);

/* unconditionally working assert */
#define birnet_assert(expr)     do {            \
  if G_LIKELY (expr) {} else                    \
    g_assert_warning (G_LOG_DOMAIN,             \
                      __FILE__, __LINE__,       \
                      __PRETTY_FUNCTION__,      \
                      #expr);                   \
} while (0)

BIRNET_EXTERN_C_END();


/* --- C++ specific hooks --- */
#ifdef	__cplusplus
namespace Birnet {
#define BIRNET_PRIVATE_CLASS_COPY(Class)        private: Class (const Class&); Class& operator= (const Class&);
class InitHook {
  typedef void (*InitHookFunc)  (void);
  InitHook    *next;
  int          priority;
  InitHookFunc hook;
  BIRNET_PRIVATE_CLASS_COPY (InitHook);
  static void  invoke_hooks (void);
public:
  explicit InitHook (InitHookFunc       _func,
                     int                _priority = 0);
};
} // Birnet
#endif	/* __cplusplus */


#endif /* __BIRNET_CORE_HH__ */

/* vim:set ts=8 sts=2 sw=2: */
