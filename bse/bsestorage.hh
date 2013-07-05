// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_STORAGE_H__
#define __BSE_STORAGE_H__

#include <bse/bseobject.hh>
#include <bse/gsldefs.hh>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_STORAGE                 (BSE_TYPE_ID (BseStorage))
#define BSE_STORAGE(object)              (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_STORAGE, BseStorage))
#define BSE_STORAGE_CLASS(class)         (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_STORAGE, BseStorageClass))
#define BSE_IS_STORAGE(object)           (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_STORAGE))
#define BSE_IS_STORAGE_CLASS(class)      (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_STORAGE))
#define BSE_STORAGE_GET_CLASS(object)    (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_STORAGE, BseStorageClass))


/* --- macros --- */
#define BSE_STORAGE_VERSION(self, vmaj, min, vmic)      ( /* whether file uses >=vARGS features */ \
  BSE_VERSION_CMP (self->major_version, self->minor_version, self->micro_version, vmaj, min, vmic) >= 0)
#define BSE_STORAGE_COMPAT(self, vmaj, min, vmic)       ( /* whether file needs <=vARGS compat code */ \
  BSE_VERSION_CMP (self->major_version, self->minor_version, self->micro_version, vmaj, min, vmic) <= 0)
#define BSE_STORAGE_SELF_CONTAINED(st)   ((BSE_OBJECT_FLAGS (st) & BSE_STORAGE_SELF_CONTAINED) != 0)
#define BSE_STORAGE_DBLOCK_CONTAINED(st) ((BSE_OBJECT_FLAGS (st) & BSE_STORAGE_DBLOCK_CONTAINED) != 0)
#define BSE_STORAGE_IS_UNDO(st)          BSE_STORAGE_DBLOCK_CONTAINED (st)
typedef enum    /*< skip >*/
{
  BSE_STORAGE_SELF_CONTAINED      = 1 << (BSE_OBJECT_FLAGS_USHIFT + 0),
  BSE_STORAGE_DBLOCK_CONTAINED    = 1 << (BSE_OBJECT_FLAGS_USHIFT + 1)
} BseStorageMode;
#define BSE_STORAGE_FLAGS_USHIFT         (BSE_OBJECT_FLAGS_USHIFT + 2)
#define BSE_STORAGE_MODE_MASK            (BSE_STORAGE_SELF_CONTAINED | BSE_STORAGE_DBLOCK_CONTAINED)


/* --- compatibility --- */
#define bse_storage_scanner_parse_or_return     sfi_scanner_parse_or_return
#define bse_storage_scanner_peek_or_return      sfi_scanner_peek_or_return


/* --- BseStorage --- */
typedef struct _BseStorageDBlock   BseStorageDBlock;
typedef struct _BseStorageItemLink BseStorageItemLink;
typedef void (*BseStorageRestoreLink)   (gpointer        data,
                                         BseStorage     *storage,
                                         BseItem        *from_item,
                                         BseItem        *to_item,
                                         const gchar    *error);
struct BseStorage : BseObject {
  /* writing */
  SfiWStore             *wstore;
  SfiPPool              *stored_items;
  SfiPPool              *referenced_items;
  /* parsing */
  SfiRStore             *rstore;
  guint                  major_version;
  guint                  minor_version;
  guint                  micro_version;
  GHashTable            *path_table;
  SfiRing               *item_links;
  SfiPPool              *restorable_objects;
  /* internal data */
  guint                  n_dblocks;
  BseStorageDBlock      *dblocks;
  gchar                 *free_me;
  /* compat */ // VERSION-FIXME: needed only for <= 0.5.1
  gfloat                 mix_freq;
  gfloat                 osc_freq;
  guint                  n_channels;
};
struct BseStorageClass : BseObjectClass
{};

/* --- compatibility file parsing --- */
void         bse_storage_compat_dhreset         (BseStorage             *self);
void         bse_storage_compat_dhmixf          (BseStorage             *self,
                                                 gfloat                  mix_freq);
void         bse_storage_compat_dhoscf          (BseStorage             *self,
                                                 gfloat                  osc_freq);
void         bse_storage_compat_dhchannels      (BseStorage             *self,
                                                 guint                   n_channels);


/* --- prototypes -- */
void         bse_storage_reset                  (BseStorage             *self);
void         bse_storage_prepare_write          (BseStorage             *self,
                                                 BseStorageMode          mode);
void         bse_storage_turn_readable          (BseStorage             *self,
                                                 const gchar            *storage_name);
BseErrorType bse_storage_input_file             (BseStorage             *self,
                                                 const gchar            *file_name);
void         bse_storage_input_text             (BseStorage             *self,
                                                 const gchar            *text,
                                                 const gchar            *text_name);
GTokenType   bse_storage_restore_item           (BseStorage             *self,
                                                 gpointer                item);
void         bse_storage_store_item             (BseStorage             *self,
                                                 BseItem                *item);
void         bse_storage_store_child            (BseStorage             *self,
                                                 BseItem                *item);
const gchar* bse_storage_item_get_compat_type   (BseItem                *item);


/* --- writing --- */
void         bse_storage_putf                   (BseStorage             *self,
                                                 gfloat                  vfloat);
void         bse_storage_putd                   (BseStorage             *self,
                                                 gdouble                 vdouble);
void         bse_storage_putr                   (BseStorage             *self,
                                                 SfiReal                 vreal,
                                                 const gchar            *hints);
void         bse_storage_put_param              (BseStorage             *self,
                                                 const GValue           *value,
                                                 GParamSpec             *pspec);
void         bse_storage_put_item_link          (BseStorage             *self,
                                                 BseItem                *from_item,
                                                 BseItem                *to_item);
void         bse_storage_put_data_handle        (BseStorage             *self,
                                                 guint                   significant_bits,
                                                 GslDataHandle          *dhandle);
void         bse_storage_put_xinfos             (BseStorage             *self,
                                                 gchar                 **xinfos);
BseErrorType bse_storage_flush_fd               (BseStorage             *self,
                                                 gint                    fd);


/* --- reading --- */
#define      bse_storage_error(s, ...)          bse_storage_error_str (s, Rapicorn::string_format (__VA_ARGS__).c_str())
void         bse_storage_error_str              (BseStorage *self, const std::string &string);
#define      bse_storage_warn(s, ...)           bse_storage_warn_str (s, Rapicorn::string_format (__VA_ARGS__).c_str())
void         bse_storage_warn_str               (BseStorage *self, const std::string &string);
#define      bse_storage_warn_skip(s, ...)      bse_storage_skip (s, Rapicorn::string_format (__VA_ARGS__).c_str())
GTokenType   bse_storage_skip                   (BseStorage *self, const std::string &string);
GTokenType   bse_storage_parse_param_value      (BseStorage             *self,
                                                 GValue                 *value,
                                                 GParamSpec             *pspec);
GTokenType   bse_storage_parse_item_link        (BseStorage             *self,
                                                 BseItem                *from_item,
                                                 BseStorageRestoreLink   restore_link,
                                                 gpointer                data);
void         bse_storage_add_restorable         (BseStorage             *self,
                                                 BseObject              *object);
void         bse_storage_finish_parsing         (BseStorage             *self);
GTokenType   bse_storage_parse_data_handle      (BseStorage             *self,
                                                 GslDataHandle         **data_handle_p,
                                                 guint                  *n_channels_p,
                                                 gfloat                 *mix_freq_p,
                                                 gfloat                 *osc_freq_p);
gboolean     bse_storage_match_data_handle      (BseStorage             *self,
                                                 GQuark                  quark);
GTokenType   bse_storage_parse_data_handle_rest (BseStorage             *self,
                                                 GslDataHandle         **data_handle_p,
                                                 guint                  *n_channels_p,
                                                 gfloat                 *mix_freq_p,
                                                 gfloat                 *osc_freq_p);
GTokenType   bse_storage_parse_xinfos           (BseStorage             *self,
                                                 gchar                ***xinfosp);
GTokenType   bse_storage_parse_rest             (BseStorage             *self,
                                                 gpointer                context_data,
                                                 BseTryStatement         try_statement,
                                                 gpointer                user_data);
gboolean     bse_storage_check_parse_negate     (BseStorage             *self);


/* --- short-hands --- */
#define bse_storage_get_scanner(s)      ((s)->rstore->scanner)
#define bse_storage_unexp_token(s,et)   sfi_rstore_unexp_token ((s)->rstore, et)
#define bse_storage_push_level(s)       sfi_wstore_push_level ((s)->wstore)
#define bse_storage_pop_level(s)        sfi_wstore_pop_level ((s)->wstore)
#define bse_storage_break(s)            sfi_wstore_break ((s)->wstore)
#define bse_storage_putc(s,c)           sfi_wstore_putc ((s)->wstore, c)
#define bse_storage_puts(s,b)           sfi_wstore_puts ((s)->wstore, b)
#define bse_storage_printf(s, ...)      bse_storage_puts (s, Rapicorn::string_format (__VA_ARGS__).c_str())

G_END_DECLS

#endif /* __BSE_STORAGE_H__ */
