// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#define HAVE_ZLIB
#include "minizip.h"

#include "external/minizip/mz_zip.c"
#include "external/minizip/mz_strm.c"
#include "external/minizip/mz_crypt.c" // mz_crypt_crc32_update
#include "external/minizip/mz_zip_rw.c"
#include "external/minizip/mz_strm_buf.c"
#include "external/minizip/mz_strm_mem.c"
#include "external/minizip/mz_strm_zlib.c"
#include "external/minizip/mz_strm_split.c"
#include "external/minizip/mz_os.c"
#include "external/minizip/mz_os_posix.c"
#include "external/minizip/mz_strm_os_posix.c"
//#include "external/minizip/mz_compat.c"
