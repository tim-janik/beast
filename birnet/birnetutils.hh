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
#ifndef __BIRNET_UTILS_HH__
#define __BIRNET_UTILS_HH__

#include <birnet/birnetcore.h>

namespace Birnet {

/* --- url handling --- */
void url_show                   (const char           *url);
void url_show_with_cookie       (const char           *url,
                                 const char           *url_title,
                                 const char           *cookie);
bool url_test_show              (const char           *url);
bool url_test_show_with_cookie  (const char	      *url,
                                 const char           *url_title,
                                 const char           *cookie);

/* --- cleanup registration --- */
uint cleanup_add                (uint                  timeout_ms,
                                 GDestroyNotify        handler,
                                 void                 *data);
void cleanup_force_handlers     (void);

/* --- string utils --- */
void memset4		        (uint32              *mem,
                                 uint32               filler,
                                 uint                 length);
/* --- memory utils --- */
void* malloc_aligned            (gsize                 total_size,
                                 gsize                 alignment,
                                 uint8               **free_pointer);
/* --- file testing --- */
bool file_check                 (const char *file,
                                 const char *mode);
bool file_equals	        (const char *file1,
                                 const char *file2);

/* --- C++ demangling --- */
char*   cxx_demangle	        (const char  *mangled_identifier); /* in birnetutilsxx.cc */

/* --- zintern support --- */
uint8*  zintern_decompress      (unsigned int          decompressed_size,
                                 const unsigned char  *cdata,
                                 unsigned int          cdata_size);

} // Birnet

#endif /* __BIRNET_UTILS_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
