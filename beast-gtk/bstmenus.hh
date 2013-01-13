// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_MENUS_H__
#define __BST_MENUS_H__
#include        "bstutils.hh"
G_BEGIN_DECLS
/* --- BstChoice --- */
/* BstChoice are simple inlined popup menus for modal selections.
 */
typedef struct _BstChoice BstChoice;
GtkWidget* bst_choice_menu_createv        (const gchar		  *menu_path,
					   BstChoice              *first_choice,
					   ...);
void bst_choice_menu_add_choice_and_free  (GtkWidget		  *menu,
					   BstChoice		  *choice);
void bst_choice_menu_set_item_sensitive	  (GtkWidget		  *menu,
					   gulong		   id,
					   gboolean		   sensitive);
GtkWidget* bst_choice_dialog_createv      (BstChoice              *first_choice,
					   ...) G_GNUC_NULL_TERMINATED;
gboolean   bst_choice_selectable          (GtkWidget              *widget);
guint      bst_choice_modal               (GtkWidget              *widget,
					   guint                   mouse_button,
					   guint32                 time);
guint      bst_choice_get_last            (GtkWidget              *widget);
void	   bst_choice_destroy		  (GtkWidget		  *choice);
/* --- BstChoice shortcuts --- */
#define BST_CHOICE_TITLE(name)           (bst_choice_alloc (BST_CHOICE_TYPE_TITLE, \
							    (name), NULL, BST_STOCK_NONE, 0))
#define BST_CHOICE(id, name, bst_icon)   (bst_choice_alloc (BST_CHOICE_TYPE_ITEM, \
							    (name), (void*) (size_t) (id), \
                                                            BST_STOCK_ ## bst_icon, 0))
#define BST_CHOICE_D(id, name, bst_icon) (bst_choice_alloc (BST_CHOICE_TYPE_ITEM | \
							    BST_CHOICE_FLAG_DEFAULT, \
                                                            (name), (void*) (size_t) (id), \
							    BST_STOCK_ ## bst_icon, 0))
#define BST_CHOICE_S(id, name, icon, s)  (bst_choice_alloc (BST_CHOICE_TYPE_ITEM | \
							    ((s) ? (BstChoiceFlags) 0 : BST_CHOICE_FLAG_INSENSITIVE), \
                                                            (name), (void*) (size_t) (id), \
							    BST_STOCK_ ## icon, 0))
#define BST_CHOICE_SUBMENU(nam,menu,icn) (bst_choice_alloc (BST_CHOICE_TYPE_SUBMENU, \
							    (nam), (menu), BST_STOCK_ ## icn, 0))
#define BST_CHOICE_TEXT(name)            (bst_choice_alloc (BST_CHOICE_TYPE_TEXT, \
							    (name), NULL, BST_STOCK_NONE, 0))
#define BST_CHOICE_SEPERATOR             (bst_choice_alloc (BST_CHOICE_TYPE_SEPARATOR, \
							    NULL, NULL, BST_STOCK_NONE, 0))
#define BST_CHOICE_END                   (NULL)
/* --- private implementation stubs --- */
typedef enum
{
  BST_CHOICE_TYPE_SEPARATOR	= 0,
  BST_CHOICE_TYPE_TITLE		= 1,
  BST_CHOICE_TYPE_TEXT		= 2,
  BST_CHOICE_TYPE_ITEM		= 3,
  BST_CHOICE_TYPE_SUBMENU	= 4,
  BST_CHOICE_TYPE_MASK		= 0xff,
  BST_CHOICE_FLAG_INSENSITIVE	= (1 << 8),
  BST_CHOICE_FLAG_DEFAULT	= (1 << 9),
  BST_CHOICE_FLAG_MASK		= (~BST_CHOICE_TYPE_MASK)
} BstChoiceFlags;
BstChoice* bst_choice_alloc               (BstChoiceFlags          type,
					   const gchar            *choice_name,
					   gpointer                choice_id,
					   const gchar		  *icon_stock_id,
					   BseIcon		  *bse_icon);
G_END_DECLS
// == Flags Enumeration Operators in C++ ==
#ifdef __cplusplus
constexpr BstChoiceFlags  operator&  (BstChoiceFlags  s1, BstChoiceFlags s2) { return BstChoiceFlags (s1 & (long long unsigned) s2); }
inline    BstChoiceFlags& operator&= (BstChoiceFlags &s1, BstChoiceFlags s2) { s1 = s1 & s2; return s1; }
constexpr BstChoiceFlags  operator|  (BstChoiceFlags  s1, BstChoiceFlags s2) { return BstChoiceFlags (s1 | (long long unsigned) s2); }
inline    BstChoiceFlags& operator|= (BstChoiceFlags &s1, BstChoiceFlags s2) { s1 = s1 | s2; return s1; }
constexpr BstChoiceFlags  operator~  (BstChoiceFlags  s1)                    { return BstChoiceFlags (~(long long unsigned) s1); }
#endif // __cplusplus
#endif  /* __BST_MENUS_H__ */
