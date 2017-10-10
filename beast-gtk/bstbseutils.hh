// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_BSE_UTILS_H__
#define __BST_BSE_UTILS_H__
#include <sfi/sfi.hh> /* needed by bstoldbseapi.h */
#include "bstdefs.hh"
#define SFIDL_SKIPDEF__BseErrorType             // replaced by Bse::Error
#define BseErrorType Bse::Error
#define SFIDL_SKIPDEF__BseMidiSignalType        // replaced by Bse::MidiSignal
#define BseMidiSignalType Bse::MidiSignal
#include "bstoldbseapi.h" /* include this within extern "C" */
#undef BseErrorType
/* --- BSE utilities --- */
#define BSE_SERVER                              (1 /* HACK */ )
#define bse_proxy_set_property                  sfi_glue_proxy_set_property
#define bse_proxy_get_property                  sfi_glue_proxy_get_property
#define bse_proxy_set                           sfi_glue_proxy_set
#define bse_proxy_get                           sfi_glue_proxy_get
#define bse_proxy_get_pspec                     sfi_glue_proxy_get_pspec
#define bse_proxy_list_properties               sfi_glue_proxy_list_properties
#define bse_proxy_disconnect                    sfi_glue_proxy_disconnect
#define bse_proxy_connect                       sfi_glue_proxy_connect
#define bse_proxy_pending                       sfi_glue_proxy_pending
#define bse_proxy_is_a                          sfi_glue_proxy_is_a
#define bse_proxy_get_qdata                     sfi_glue_proxy_get_qdata
#define bse_proxy_set_qdata_full                sfi_glue_proxy_set_qdata_full
#define bse_proxy_steal_qdata                   sfi_glue_proxy_steal_qdata
#define bse_proxy_set_qdata(p,q,d)              bse_proxy_set_qdata_full ((p), (q), (d), NULL)
#define bse_proxy_set_data(p,n,d)               bse_proxy_set_qdata ((p), g_quark_from_string (n), (d))
#define bse_proxy_get_data(p,n)                 bse_proxy_get_qdata ((p), g_quark_try_string (n))
#define bse_proxy_steal_data(p,n)               bse_proxy_steal_qdata ((p), g_quark_try_string (n))
#define bse_proxy_set_data_full(p,n,d,f)        bse_proxy_set_qdata_full ((p), g_quark_from_string (n), (d), (f))

/* --- BEAST utilities --- */
Bse::Error    bst_project_restore_from_file   (Bse::ProjectH   project,
                                                 const gchar    *file_name,
                                                 bool            apply_project_file_name,
						 bool            preserve_non_dirty);
Bse::Error    bst_project_import_midi_file    (Bse::ProjectH   project,
                                                 const gchar    *file_name);
uint            bst_path_leaf_index             (const String &path, char separator = '/');
Bse::Category   bst_category_find               (const String &pattern);

BseIt3mSeq*     bst_it3m_seq_from_item_seq (Bse::ItemSeq &items);
Bse::ItemSeq    bst_item_seq_from_it3m_seq (BseIt3mSeq *i3s);


namespace Bse {
const char*     error_blurb (Bse::Error error_value); ///< Retrieve the description of an error value
} // Bse

#endif /* __BST_BSE_UTILS_H__ */
