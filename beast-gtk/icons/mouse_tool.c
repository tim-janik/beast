/* GIMP RGBA C-Source image dump 1-byte-run-length-encoded (mouse_tool.c) */

#define MOUSE_TOOL_IMAGE_WIDTH (32)
#define MOUSE_TOOL_IMAGE_HEIGHT (32)
#define MOUSE_TOOL_IMAGE_BYTES_PER_PIXEL (4) /* 3:RGB, 4:RGBA */
#define MOUSE_TOOL_IMAGE_RLE_PIXEL_DATA ((guint8*) MOUSE_TOOL_IMAGE_rle_pixel_data)
#define MOUSE_TOOL_IMAGE_RUN_LENGTH_DECODE(image_buf, rle_data, size, bpp) do \
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
static const guint8 MOUSE_TOOL_IMAGE_rle_pixel_data[2092] =
("\210\0\0\0\0\3!!!&!!!=!!!(\233\0\0\0\0\7!!!\21!!!ykc`\314}oj\377iZS\322!!"
 "!y!!!\27\231\0\0\0\0\7!!!$YTS\377\334\323\316\377\356\334\325\377\313\254"
 "\237\377WLF\377!!!1\210\0\0\0\0\2\0\0\0y\0\0\0\234\214\0\0\0\0\12\0\0\0.\14"
 "\16\13\363%'\"\363,.*\365*,'\377\27\30\25\377\307\260\247\377\331\262\242"
 "\377\313\246\224\377C<8\352\206\0\0\0\0\6\0\0\0\11$$$\204\234\234\234\373"
 "\276\276\276\375\16\16\16\251\0\0\0\23\212\0\0\0\0\12\0\0\0\34.3(\301\211"
 "\224~\377\307\324\273\377\255\273\236\37728,\377\274\232\215\377\332\263\242"
 "\377\302\235\215\377>74\363\206\0\0\0\0\7$$$\204\221\221\221\377\255\255\255"
 "\377zzz\377;;;\377\23\23\23\245\0\0\0\20\211\0\0\0\0\12\0\0\0\242PRN\316\247"
 "\260\235\377\247\271\225\377s\200e\377&*!\377\273\232\214\377\321\253\231"
 "\377\203mb\37750.q\205\0\0\0\0\10\0\0\0y\17\17\17\373\206\206\206\377\262"
 "\262\262\377{{{\377999\377\12\12\12\377\0\0\0\246\206\0\0\0\0\15\0\0\0\11"
 "\37\40\36\201\"$!\347\277\306\267\376\273\310\256\377\263\304\242\377{\211"
 "l\377KTB\377\"&\36\377\215sf\377\202k`\363)'&\333!!!*\205\0\0\0\0\10\0\0\0"
 "\21\0\0\0Dbbb\377\303\303\303\377}}}\377///\377\0\0\0p\0\0\0\26\206\0\0\0"
 "\0\13\0\0\0&UWR\314\306\314\277\377\274\315\255\377\254\300\230\377\215\236"
 "}\377<C5\361\32\34\27\303\24\26\21\377\32\32\32\254!!!i\205\0\0\0\0\1\0\0"
 "\0J\202\0\0\0\0\6\0\0\0""1bbb\377\303\303\303\377}}}\377///\377\0\0\0a\202"
 "\0\0\0\0\2\0\0\0""8\0\0\0\23\204\0\0\0\0\11\0\0\0.MNK\322\277\315\262\377"
 "\236\261\214\377gs[\377\11\12\10\333\0\0\0%\0\0\0a\0\0\0\16\205\0\0\0\0\2"
 "\0\0\0x\0\0\0\302\202\0\0\0\0\6\0\0\0""1bbb\377\303\303\303\377}}}\377///"
 "\377\0\0\0a\202\0\0\0\0\2\0\0\0\222\0\0\0\251\205\0\0\0\0\5\0\0\0\32>B;\342"
 "\211\225}\377).$\377\5\6\4""4\206\0\0\0\0\4\0\0\0\7\27\27\27~\234\234\234"
 "\366\27\27\27\366\202\31\31\31\333\6\31\31\31\342ooo\377\303\303\303\377}"
 "}}\377777\377\30\30\30\350\202\31\31\31\333\4\27\27\27\357\230\230\230\375"
 "\30\30\30\243\0\0\0\14\204\0\0\0\0\4\0\0\0\40!%\35\316\33\36\30\306\0\0\0"
 "\20\205\0\0\0\0\4\0\0\0\4***\206\223\223\223\377\255\255\255\377\205\324\324"
 "\324\377\3\275\275\275\377\201\201\201\377\225\225\225\377\204\324\324\324"
 "\377\202\255\255\255\377\2""888\246\0\0\0\24\204\0\0\0\0\2\0\0\0/\0\0\0$\206"
 "\0\0\0\0\4\0\0\0\14FFF\377\302\302\302\377\204\204\204\377\206\225\225\225"
 "\377\1\204\204\204\377\205\225\225\225\377\4\204\204\204\377\223\223\223\377"
 "CCC\377\0\0\0=\214\0\0\0\0\4\0\0\0\2\"\"\"hDDD\371UUU\377\204RRR\377\4WWW"
 "\377\217\217\217\377}}}\377TTT\377\204RRR\377\4UUU\377@@@\374\14\14\14\215"
 "\0\0\0\14\216\0\0\0\0\3\0\0\0_,,,\356\0\0\0\356\202\0\0\0\266\6\0\0\0\304"
 "bbb\377\303\303\303\377}}}\377///\377\0\0\0\322\202\0\0\0\266\3\0\0\0\340"
 "***\374\0\0\0\202\221\0\0\0\0\2\0\0\0]\0\0\0\302\202\0\0\0\0\6\0\0\0""1bb"
 "b\377\303\303\303\377}}}\377///\377\0\0\0a\202\0\0\0\0\2\0\0\0\222\0\0\0\215"
 "\214\0\0\0\0\2\0\0\0\23\0\0\0\202\205\0\0\0\0\1\0\0\0.\202\0\0\0\0\6\0\0\0"
 """1bbb\377\303\303\303\377}}}\377///\377\0\0\0a\202\0\0\0\0\2\0\0\0#\0\0\0"
 "\14\211\0\0\0\0\6\0\0\0\11\0\0\0=\37\37\34\265??8\377~~r\377\11\11\11m\204"
 "\0\0\0\0\12\0\0\0.\0\0\0\0\0\0\0+\0\0\0bbbb\377\303\303\303\377}}}\377///"
 "\377\0\0\0\207\0\0\0""7\213\0\0\0\0\10\0\0\0Q\25\25\23\215XXO\377\227\227"
 "\210\377\327\327\300\377\276\276\253\377MMF\263\0\0\0\27\202\0\0\0\0\13\0"
 "\0\0]ZO,\337\0\0\0@\0\0\0_\25\25\25\371\226\226\226\377\253\253\253\377zz"
 "z\377===\377\16\16\16\377\0\0\0\212\211\0\0\0\0\27\0\0\0#\0\0\0\266WWN\347"
 "\235\235\214\377\317\317\271\377\330\330\302\377\327\327\300\377\303\303\257"
 "\377\252\252\230\377!!\35\273\0\0\0\0\0\0\0_~o>\356|m=\377\0\0\0\206\0\0\0"
 "\0\37\37\37h\201\201\201\377\264\264\264\377{{{\377777\377\17\17\17\216\0"
 "\0\0\12\211\0\0\0\0\26\35\35\35\352ttt\377\246\246\230\377\322\322\273\377"
 "\312\312\265\377\271\271\245\377\272\272\245\377\335\335\305\377\341\341\311"
 "\377qqi\365\35\31\16h{l<\371\210xB\377\10\7\4\240\0\0\0\6\0\0\0\0\0\0\0\2"
 "\14\14\14_\206\206\206\357EEE\370\4\4\4\204\0\0\0\5\212\0\0\0\0\15///\363"
 "\276\276\276\377\204\204{\377\256\256\233\377\262\262\237\377\320\320\272"
 "\377\337\337\307\377\304\304\257\377\332\332\304\377\231\231\207\377wh:\377"
 "\202s@\377\35\31\16\254\205\0\0\0\0\2\0\0\0a\0\0\0}\214\0\0\0\0\15//.\363"
 "\355\355\347\377wwt\377\217\217\200\377\326\326\277\377\302\302\255\377\265"
 "\265\242\377\332\332\303\377\223\223\204\377\205wG\377\205uA\377#\36\21\246"
 "\0\0\0\24\223\0\0\0\0\15))'\363\242\242\234\377\205\205z\377\307\307\261\377"
 "\301\301\254\377\313\313\265\377\341\341\311\377\232\231\205\377\201rC\377"
 "\210xC\377baT\377\201\201u\362\30\30\27S\223\0\0\0\0\16\11\11\11\363ee\\\377"
 "\266\266\243\377\275\275\251\377\306\306\261\377\350\350\317\377\231\230\206"
 "\377ug:\377\203s@\377caR\377\350\350\323\377\327\327\311\377__Z\377\0\0\0"
 "J\222\0\0\0\0\16\34\34\32\363\255\255\237\377\310\310\263\377\307\307\262"
 "\377\344\344\315\377\321\306\223\377[M#\377I@$\377]ZJ\377\261\261\237\377"
 "\261\261\242\377\246\246\230\377\222\222\205\377\0\0\0\302\222\0\0\0\0\16"
 "\16\16\15Ljj`\377\336\336\307\377\341\341\311\377\337\337\310\377olZ\377."
 "+\37\377\211\211{\377\232\232\212\377\245\245\225\377\235\235\215\377TTK\362"
 "\25\25\23\347\0\0\0\34\222\0\0\0\0\14\0\0\0\40QQK\316\314\314\271\377\315"
 "\315\270\377\262\262\240\377\247\247\225\377\312\312\265\377\323\323\275\377"
 "\323\323\276\377\315\315\270\377ssg\347\0\0\0Q\225\0\0\0\0\12!!!m\247\247"
 "\227\377\323\323\275\377\355\355\324\377\357\357\326\377\362\362\331\377\356"
 "\356\325\377\235\235\215\377ttg\211\0\0\0N\226\0\0\0\0\3\0\0\0\25))%\272\332"
 "\332\304\377\202\360\360\327\377\4\340\340\311\377\202\202u\373\"\"\37\204"
 "\0\0\0\11\230\0\0\0\0\6\0\0\0y\214\214~\370\352\352\322\377\257\257\235\377"
 "SSK\301\0\0\0y\233\0\0\0\0\4!!\35\201ff[\377--(\254\0\0\0\21\234\0\0\0\0\3"
 "\0\0\0\11\0\0\0=\0\0\0\27\227\0\0\0\0");

