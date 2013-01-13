// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_GLOBALS_H__
#define __GXK_GLOBALS_H__
#include <sfi/glib-extra.hh>
#include <gtk/gtk.h>
G_BEGIN_DECLS
/* --- i18n and gettext helpers --- */
#ifdef GXK_COMPILATION
#  define GXK_I18N_DOMAIN NULL
#  define _(str)        dgettext (GXK_I18N_DOMAIN, str)
#  define T_(str)       dgettext (GXK_I18N_DOMAIN, str)
#  define N_(str)       (str)
#endif
/* --- macros --- */
#define	GXK_TOOLTIPS	(gxk_globals->tooltips)
/* --- typedefs & structures --- */
typedef void (*GxkFreeFunc) (gpointer data);
typedef struct
{
  GtkTooltips *tooltips;
} GxkGlobals;
/* --- spacing/padding --- */
#define	GXK_OUTER_BORDER	(5)	/* outer dialog border-width */
#define	GXK_INNER_SPACING	(3)	/* spacing/padding between h/v boxes */
#define	GXK_BUTTON_PADDING	(3)	/* padding between adjacent buttons */
/* --- convenience --- */
gulong  gxk_nullify_in_object (gpointer object,
                               gpointer location);
/* --- variables --- */
extern const GxkGlobals* gxk_globals;
/* --- functions --- */
void	gxk_init	(void);
/* --- internal --- */
void	gxk_init_utils		(void);
void	gxk_init_params	        (void);
void	gxk_init_stock		(void);
void	gxk_init_actions	(void);
void	gxk_init_assortments	(void);
void	gxk_init_radget_types	(void);
G_END_DECLS
// == Flags Enumeration Operators in C++ ==
#ifdef __cplusplus
constexpr GdkEventMask  operator&  (GdkEventMask  s1, GdkEventMask s2) { return GdkEventMask (s1 & (long long unsigned) s2); }
inline    GdkEventMask& operator&= (GdkEventMask &s1, GdkEventMask s2) { s1 = s1 & s2; return s1; }
constexpr GdkEventMask  operator|  (GdkEventMask  s1, GdkEventMask s2) { return GdkEventMask (s1 | (long long unsigned) s2); }
inline    GdkEventMask& operator|= (GdkEventMask &s1, GdkEventMask s2) { s1 = s1 | s2; return s1; }
constexpr GdkEventMask  operator~  (GdkEventMask  s1)                 { return GdkEventMask (~(long long unsigned) s1); }
constexpr GdkModifierType  operator&  (GdkModifierType  s1, GdkModifierType s2) { return GdkModifierType (s1 & (long long unsigned) s2); }
inline    GdkModifierType& operator&= (GdkModifierType &s1, GdkModifierType s2) { s1 = s1 & s2; return s1; }
constexpr GdkModifierType  operator|  (GdkModifierType  s1, GdkModifierType s2) { return GdkModifierType (s1 | (long long unsigned) s2); }
inline    GdkModifierType& operator|= (GdkModifierType &s1, GdkModifierType s2) { s1 = s1 | s2; return s1; }
constexpr GdkModifierType  operator~  (GdkModifierType  s1)                 { return GdkModifierType (~(long long unsigned) s1); }
constexpr GdkWindowHints  operator&  (GdkWindowHints  s1, GdkWindowHints s2) { return GdkWindowHints (s1 & (long long unsigned) s2); }
inline    GdkWindowHints& operator&= (GdkWindowHints &s1, GdkWindowHints s2) { s1 = s1 & s2; return s1; }
constexpr GdkWindowHints  operator|  (GdkWindowHints  s1, GdkWindowHints s2) { return GdkWindowHints (s1 | (long long unsigned) s2); }
inline    GdkWindowHints& operator|= (GdkWindowHints &s1, GdkWindowHints s2) { s1 = s1 | s2; return s1; }
constexpr GdkWindowHints  operator~  (GdkWindowHints  s1)                 { return GdkWindowHints (~(long long unsigned) s1); }
constexpr GtkAttachOptions  operator&  (GtkAttachOptions  s1, GtkAttachOptions s2) { return GtkAttachOptions (s1 & (long long unsigned) s2); }
inline    GtkAttachOptions& operator&= (GtkAttachOptions &s1, GtkAttachOptions s2) { s1 = s1 & s2; return s1; }
constexpr GtkAttachOptions  operator|  (GtkAttachOptions  s1, GtkAttachOptions s2) { return GtkAttachOptions (s1 | (long long unsigned) s2); }
inline    GtkAttachOptions& operator|= (GtkAttachOptions &s1, GtkAttachOptions s2) { s1 = s1 | s2; return s1; }
constexpr GtkAttachOptions  operator~  (GtkAttachOptions  s1)                 { return GtkAttachOptions (~(long long unsigned) s1); }
#endif // __cplusplus
#endif /* __GXK_GLOBALS_H__ */
