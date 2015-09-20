// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstbseutils.hh"

/* --- BEAST utilities --- */
Bse::ErrorType
bst_project_restore_from_file (Bse::ProjectH project, const gchar *file_name, bool apply_project_file_name, bool preserve_non_dirty)
{
  bool was_dirty = project.is_dirty();
  Bse::ErrorType error = project.restore_from_file (file_name);
  /* regardless of how good the restoration worked, try to
   * keep the resulting project in a GUI usable state.
   */
  BseItemSeq *iseq = bse_container_list_children (project.proxy_id());
  guint i;
  for (i = 0; i < iseq->n_items; i++)
    if (BSE_IS_SONG (iseq->items[i]))
      {
        /* fixup orphaned parts */
        bse_song_ensure_track_links (iseq->items[i]);
        /* songs always need a master bus */
        bse_song_ensure_master_bus (iseq->items[i]);
      }
  if (!error && apply_project_file_name)
    {
      bse_proxy_set_data_full (project.proxy_id(), "beast-project-file-name", g_strdup (file_name), g_free);
      gchar *bname = g_path_get_basename (file_name);
      project.change_name (bname);
      g_free (bname);
    }
  if (preserve_non_dirty && !was_dirty)
    project.clean_dirty();
  return error;
}

Bse::ErrorType
bst_project_import_midi_file (Bse::ProjectH project, const gchar *file_name)
{
  Bse::ErrorType error = project.import_midi_file (file_name);
  /* regardless of how good the restoration worked, try to
   * keep the resulting project in a GUI usable state.
   */
  BseItemSeq *iseq = bse_container_list_children (project.proxy_id());
  guint i;
  for (i = 0; i < iseq->n_items; i++)
    if (BSE_IS_SONG (iseq->items[i]))
      {
        /* fixup orphaned parts */
        bse_song_ensure_track_links (iseq->items[i]);
        /* songs always need a master bus */
        bse_song_ensure_master_bus (iseq->items[i]);
      }
  return error;
}

const gchar*
bst_procedure_get_title (const gchar *procedure)
{
  if (procedure)
    {
      BseCategorySeq *cseq = bse_categories_match_typed ("*", procedure);
      if (cseq->n_cats)
        return cseq->cats[0]->category + bst_path_leaf_index (cseq->cats[0]->category);
    }
  return NULL;
}

/// Return the character index of the last string segment not containing @a separator.
uint
bst_path_leaf_index (const String &path, char separator)
{
  const char *data = path.data();
  const char *d = strrchr (data, separator);
  return d && d >= data && d < data + path.size() ? d - data + 1 : 0;
}

namespace Bse {

const char*
error_blurb (Bse::ErrorType error_value)
{
  const Rapicorn::Aida::EnumValue *ev = Rapicorn::Aida::enum_info<Bse::ErrorType>().find_value (error_value);
  return ev ? ev->blurb : NULL;
}

} // Bse


/* --- generated code --- */
#define BseErrorType Bse::ErrorType
#include "bstoldbseapi.cc"
#undef BseErrorType
