// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_STORE_H__
#define __SFI_STORE_H__

#include <sfi/sfivalues.hh>
#include <sfi/sfiring.hh>

G_BEGIN_DECLS

/* --- typedefs and structures --- */
typedef gint /* -errno || length */ (*SfiStoreReadBin)  (gpointer        data,
                                                         void           *buffer,
                                                         guint           blength);
typedef struct
{
  GString *text;
  guint    indent;
  SfiRing *bblocks;
  guint    needs_break : 1;
  guint    flushed : 1;
  gchar    comment_start;
} SfiWStore;
#define SFI_TOKEN_UNMATCHED     ((GTokenType) (G_TOKEN_LAST + 1))
#define SFI_TOKEN_LAST          ((GTokenType) (SFI_TOKEN_UNMATCHED + 1))
typedef struct _SfiRStore SfiRStore;
typedef GTokenType (*SfiStoreParser)  (gpointer        context_data,
                                       SfiRStore      *rstore, /* parser_this */
                                       GScanner       *scanner,
                                       gpointer        user_data);
struct _SfiRStore
{
  GScanner      *scanner;
  gchar         *fname;
  gint           close_fd;
  gpointer       parser_this;
  SfiNum         bin_offset;
};


/* --- writable store --- */
SfiWStore*      sfi_wstore_new                (void);
void            sfi_wstore_destroy            (SfiWStore      *wstore);
void            sfi_wstore_push_level         (SfiWStore      *wstore);
void            sfi_wstore_pop_level          (SfiWStore      *wstore);
void            sfi_wstore_break              (SfiWStore      *wstore);
void            sfi_wstore_puts               (SfiWStore      *wstore,
                                               const gchar    *string);
void            sfi_wstore_putc               (SfiWStore      *wstore,
                                               gchar           character);
#define         sfi_wstore_printf(wstore, ...)  sfi_wstore_puts (wstore, Rapicorn::string_format (__VA_ARGS__).c_str())
void            sfi_wstore_putf               (SfiWStore      *wstore,
                                               gfloat          vfloat);
void            sfi_wstore_putd               (SfiWStore      *wstore,
                                               gdouble         vdouble);
void            sfi_wstore_put_value          (SfiWStore      *wstore,
                                               const GValue   *value);
void            sfi_wstore_put_param          (SfiWStore      *wstore,
                                               const GValue   *value,
                                               GParamSpec     *pspec);
void            sfi_wstore_put_binary         (SfiWStore      *wstore,
                                               SfiStoreReadBin reader,
                                               gpointer        data,
                                               GDestroyNotify  destroy);
gint /*-errno*/ sfi_wstore_flush_fd           (SfiWStore      *wstore,
                                               gint            fd);
const gchar*    sfi_wstore_peek_text          (SfiWStore      *wstore,
                                               guint          *length);


/* --- readable store --- */
SfiRStore*      sfi_rstore_new                (void);
SfiRStore*      sfi_rstore_new_open           (const gchar    *fname);
void            sfi_rstore_destroy            (SfiRStore      *rstore);
void            sfi_rstore_input_fd           (SfiRStore      *rstore,
                                               gint            fd,
                                               const gchar    *fname);
void            sfi_rstore_input_text         (SfiRStore      *rstore,
                                               const gchar    *text,
                                               const gchar    *text_name);
gboolean        sfi_rstore_eof                (SfiRStore      *rstore);
GTokenType      sfi_rstore_parse_param        (SfiRStore      *rstore,
                                               GValue         *value,
                                               GParamSpec     *pspec);
GTokenType      sfi_rstore_ensure_bin_offset  (SfiRStore      *rstore);
guint64         sfi_rstore_get_bin_offset     (SfiRStore      *rstore);
GTokenType      sfi_rstore_parse_binary       (SfiRStore      *rstore,
                                               SfiNum         *offset_p,
                                               SfiNum         *length_p);
GTokenType      sfi_rstore_parse_zbinary      (SfiRStore      *rstore,
                                               SfiNum         *offset_p,
                                               SfiNum         *length_p);
GTokenType      sfi_rstore_parse_until        (SfiRStore      *rstore,
                                               GTokenType      closing_token,
                                               gpointer        context_data,
                                               SfiStoreParser  try_statement,
                                               gpointer        user_data);
guint           sfi_rstore_parse_all          (SfiRStore      *rstore,
                                               gpointer        context_data,
                                               SfiStoreParser  try_statement,
                                               gpointer        user_data);
void            sfi_rstore_error              (SfiRStore *rstore, const std::string &msg);
void            sfi_rstore_unexp_token        (SfiRStore      *rstore,
                                               GTokenType      expected_token);
void            sfi_rstore_warn               (SfiRStore *rstore, const std::string &msg);
GTokenType      sfi_rstore_warn_skip          (SfiRStore *rstore, const std::string &msg);
typedef gboolean (SfiRStoreQuickScan)         (SfiRStore      *rstore,
                                               gpointer        data);
void            sfi_rstore_quick_scan         (SfiRStore      *rstore,
                                               const gchar    *identifier,
                                               SfiRStoreQuickScan qcheck,
                                               gpointer        data);


/* --- convenience --- */
#define sfi_scanner_parse_or_return(scanner, token)  G_STMT_START {     \
  GTokenType _t = GTokenType (token);                                   \
  if (g_scanner_get_next_token (scanner) != _t)                         \
    return _t;                                                          \
} G_STMT_END
#define sfi_scanner_peek_or_return(scanner, token)   G_STMT_START {     \
  GTokenType _t = GTokenType (token);                                   \
  GScanner *__s = (scanner);                                            \
  if (g_scanner_peek_next_token (__s) != _t)                            \
    {                                                                   \
      g_scanner_get_next_token (__s); /* advance to error pos */        \
      return _t;                                                        \
    }                                                                   \
} G_STMT_END

G_END_DECLS

#endif /* __SFI_STORE_H__ */
