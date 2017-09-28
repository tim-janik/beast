// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstbseutils.hh"
#include "bstutils.hh"

/* --- BEAST utilities --- */
Bse::Error
bst_project_restore_from_file (Bse::ProjectH project, const gchar *file_name, bool apply_project_file_name, bool preserve_non_dirty)
{
  bool was_dirty = project.is_dirty();
  Bse::Error error = project.restore_from_file (file_name);
  /* regardless of how good the restoration worked, try to
   * keep the resulting project in a GUI usable state.
   */
  Bse::ItemSeq items = project.list_children();
  for (size_t i = 0; i < items.size(); i++)
    {
      SfiProxy item = items[i].proxy_id();
      if (BSE_IS_SONG (item))
        {
          Bse::SongH song = Bse::SongH::down_cast (bse_server.from_proxy (item));
          song.ensure_track_links();    // fixup orphaned parts
          song.ensure_master_bus();     // songs always need a master bus
        }
    }
  if (error == 0 && apply_project_file_name)
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

Bse::Error
bst_project_import_midi_file (Bse::ProjectH project, const gchar *file_name)
{
  Bse::Error error = project.import_midi_file (file_name);
  /* regardless of how good the restoration worked, try to
   * keep the resulting project in a GUI usable state.
   */
  Bse::ItemSeq items = project.list_children();
  for (size_t i = 0; i < items.size(); i++)
    {
      SfiProxy item = items[i].proxy_id();
      if (BSE_IS_SONG (item))
        {
          Bse::SongH song = Bse::SongH::down_cast (bse_server.from_proxy (item));
          song.ensure_track_links();    // fixup orphaned parts
          song.ensure_master_bus();     // songs always need a master bus
        }
    }
  return error;
}

const gchar*
bst_procedure_get_title (const gchar *procedure)
{
  if (procedure)
    {
      Bse::CategorySeq cseq = bse_server.category_match_typed ("*", procedure);
      if (cseq.size())
        return cseq[0].category.c_str() + bst_path_leaf_index (cseq[0].category);
    }
  return NULL;
}


Bse::Category
bst_category_find (const String &pattern)
{
  Bse::CategorySeq cseq;
  if (!pattern.empty())
    cseq = bse_server.category_match (pattern);
  if (cseq.size() == 1)
    return cseq[0];
  return Bse::Category();
}

/// Return the character index of the last string segment not containing @a separator.
uint
bst_path_leaf_index (const String &path, char separator)
{
  const char *data = path.data();
  const char *d = strrchr (data, separator);
  return d && d >= data && d < data + path.size() ? d - data + 1 : 0;
}

BseIt3mSeq*
bst_it3m_seq_from_item_seq (Bse::ItemSeq &items)
{
  BseIt3mSeq *i3s = bse_it3m_seq_new();
  for (auto item : items)
    bse_it3m_seq_append (i3s, !item ? 0 : item.proxy_id());
  return i3s;
}

Bse::ItemSeq
bst_item_seq_from_it3m_seq (BseIt3mSeq *i3s)
{
  Bse::ItemSeq items;
  for (size_t i = 0; i < i3s->n_items; i++)
    items.push_back (Bse::ItemH::down_cast (bse_server.from_proxy (i3s->items[i])));
  return items;
}

namespace Bse {

const char*
error_blurb (Bse::Error error_value)
{
  const Aida::EnumValue ev = Aida::enum_info<Bse::Error>().find_value (error_value);
  return ev.blurb;
}

} // Bse


/* --- generated code --- */
#define BseErrorType Bse::Error
#include "bstoldbseapi.cc"
#undef BseErrorType
