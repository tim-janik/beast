/* GIMP RGBA C-Source image dump 1-byte-run-length-encoded (close.c) */

#define CLOSE_IMAGE_WIDTH (20)
#define CLOSE_IMAGE_HEIGHT (20)
#define CLOSE_IMAGE_BYTES_PER_PIXEL (4) /* 3:RGB, 4:RGBA */
#define CLOSE_IMAGE_RLE_PIXEL_DATA ((guint8*) CLOSE_IMAGE_rle_pixel_data)
#define CLOSE_IMAGE_RUN_LENGTH_DECODE(image_buf, rle_data, size, bpp) do \
{ guint __bpp; guint8 *__ip; const guint8 *__il, *__rd; \
  __bpp = (bpp); __ip = (image_buf); __il = __ip + (size) * __bpp; \
  __rd = (rle_data); if (__bpp > 3) { /* RGBA */ \
    while (__ip < __il) { guint __l = *(__rd++); \
      if (__l & 128) { __l = __l - 128; \
        do { memcpy (__ip, __rd, 4); __ip += 4; } while (--__l); __rd += 4; \
      } else { __l *= 4; memcpy (__ip, __rd, __l); \
               __ip += __l; __rd += __l; } } \
  } else { /* RGB */ \
    while (__ip < __il) { guint __l = *(__rd++); \
      if (__l & 128) { __l = __l - 128; \
        do { memcpy (__ip, __rd, 3); __ip += 3; } while (--__l); __rd += 3; \
      } else { __l *= 3; memcpy (__ip, __rd, __l); \
               __ip += __l; __rd += __l; } } \
  } } while (0)
static const guint8 CLOSE_IMAGE_rle_pixel_data[165] =
("\352\0\0\0\0\1\0\0\0\377\204\0\0\0\0\202\0\0\0\377\215\0\0\0\0\202\0\0\0\377"
 "\202\0\0\0\0\204\0\0\0\377\215\0\0\0\0\202\0\0\0\377\1\0\0\0\0\203\0\0\0\377"
 "\216\0\0\0\0\205\0\0\0\377\220\0\0\0\0\203\0\0\0\377\221\0\0\0\0\204\0\0\0"
 "\377\217\0\0\0\0\206\0\0\0\377\216\0\0\0\0\202\0\0\0\377\1\0\0\0\0\204\0\0"
 "\0\377\214\0\0\0\0\202\0\0\0\377\203\0\0\0\0\204\0\0\0\377\213\0\0\0\0\1\0"
 "\0\0\377\205\0\0\0\0\202\0\0\0\377\352\0\0\0\0");

