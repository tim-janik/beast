// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __BSE_MINIZIP_H__
#define __BSE_MINIZIP_H__

#include <stdint.h>

// Configuration
#define MZ_ZIP_NO_ENCRYPTION

// Mimick PKZIP 2.04 which introduced data descriptors (which
// are mandatory in MZ) and only supported Deflate compression.
#define BSE_MZ_VERSION_MADEBY   24

// Minizip API
#include "external/minizip/mz.h"
#include "external/minizip/mz_os.h"
#include "external/minizip/mz_zip.h"
#include "external/minizip/mz_strm.h"
//#include "external/minizip/mz_crypt.h"
#include "external/minizip/mz_strm_buf.h"
#include "external/minizip/mz_strm_mem.h"
#include "external/minizip/mz_strm_zlib.h"
#include "external/minizip/mz_strm_split.h"
#include "external/minizip/mz_strm_os.h"
#include "external/minizip/mz_zip_rw.h"
//#include "external/minizip/mz_compat.h"

#endif // __BSE_MINIZIP_H__
