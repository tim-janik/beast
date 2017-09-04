// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_DEFS_H__
#define __BST_DEFS_H__
#include <gxk/gxk.hh>
#include <libintl.h>
#include "bstzoomedwindow.hh"
#include "bse/bse.hh"

// == using Bse types ==
using Bse::Any;
using Bse::String;
using Bse::printout;
using Bse::printerr;
using Bse::string_format;

/* --- generic constants --- */
typedef enum {
  BST_QUANTIZE_NONE		= 0,
  BST_QUANTIZE_NOTE_1		= 1,
  BST_QUANTIZE_NOTE_2		= 2,
  BST_QUANTIZE_NOTE_4		= 4,
  BST_QUANTIZE_NOTE_8		= 8,
  BST_QUANTIZE_NOTE_16		= 16,
  BST_QUANTIZE_NOTE_32		= 32,
  BST_QUANTIZE_NOTE_64		= 64,
  BST_QUANTIZE_NOTE_128		= 128,
  BST_QUANTIZE_TACT		= 65535
} BstQuantizationType;

typedef struct _BstKeyBinding BstKeyBinding;

/* choose IDs that are unlikely to clash with category IDs */
#define BST_COMMON_ROLL_TOOL_FIRST	(G_MAXINT - 100000)

typedef enum /*< skip >*/
{
  BST_COMMON_ROLL_TOOL_NONE,
  BST_COMMON_ROLL_TOOL_INSERT            = BST_COMMON_ROLL_TOOL_FIRST,
  BST_COMMON_ROLL_TOOL_RESIZE,
  BST_COMMON_ROLL_TOOL_LINK,
  BST_COMMON_ROLL_TOOL_RENAME,
  BST_COMMON_ROLL_TOOL_ALIGN,
  BST_COMMON_ROLL_TOOL_MOVE,
  BST_COMMON_ROLL_TOOL_DELETE,
  BST_COMMON_ROLL_TOOL_SELECT,
  BST_COMMON_ROLL_TOOL_VSELECT,
  BST_COMMON_ROLL_TOOL_EDITOR,
  BST_COMMON_ROLL_TOOL_MOVE_TICK_POINTER,
  BST_COMMON_ROLL_TOOL_MOVE_TICK_LEFT,
  BST_COMMON_ROLL_TOOL_MOVE_TICK_RIGHT,
  BST_COMMON_ROLL_TOOL_LAST
} BstCommonRollTool;

/* --- constants & defines --- */
#define	BST_TAG_DIAMETER	  (20)
#define BST_STRDUP_RC_FILE()	  (g_strconcat (g_get_home_dir (), "/.beast/beastrc", NULL))
#define BST_STRDUP_SKIN_PATH()	  (g_strconcat ("~/.beast/skins/:~/.beast/skins/*/:", \
                                                Bse::installpath (Bse::INSTALLPATH_DATADIR_SKINS).c_str(), NULL))

/* --- configuration candidates --- */
/* mouse button numbers and masks for drag operations */
#define BST_DRAG_BUTTON_COPY	  (1)
#define BST_DRAG_BUTTON_COPY_MASK (GDK_BUTTON1_MASK)
#define BST_DRAG_BUTTON_MOVE	  (2)
#define BST_DRAG_BUTTON_MOVE_MASK (GDK_BUTTON2_MASK)
#define BST_DRAG_BUTTON_CONTEXT   (3) /* delete, clone, linkdup */

/* --- miscellaneous --- */
#define	BST_DVL_HINTS		(bst_developer_hints != FALSE)
#define	BST_DBG_EXT     	(bst_debug_extensions != FALSE)
#define	GNOME_CANVAS_NOTIFY(object)	G_STMT_START { \
    if (GTK_IS_OBJECT (object)) \
      g_signal_emit_by_name (object, "notify::generic-change", NULL); \
} G_STMT_END

/* --- i18n and gettext helpers --- */
// Atm, Beast and libbse share the same text domain
#define _(str)  ::Bse::_ (str)
#define N_(str) (str)
inline const char* bst_gettext_domain ()        { return ::Bse::bse_gettext_domain(); }

/* --- internal stuff --- */
void    beast_show_about_box (void);
void    bst_main_loop_wakeup    ();
extern gboolean bst_developer_hints;
extern gboolean bst_debug_extensions;

#endif /* __BST_DEFS_H__ */
