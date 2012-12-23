/* BEAST - Better Audio System
 * Copyright (C) 2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bstbseutils.hh"

/* --- BEAST utilities --- */
BseErrorType
bst_project_restore_from_file (SfiProxy        project,
                               const gchar    *file_name,
                               bool            apply_project_file_name,
                               bool            preserve_non_dirty)
{
  bool was_dirty = bse_project_is_dirty (project);
  BseErrorType error = bse_project_restore_from_file (project, file_name);
  /* regardless of how good the restoration worked, try to
   * keep the resulting project in a GUI usable state.
   */
  BseItemSeq *iseq = bse_container_list_children (project);
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
      bse_proxy_set_data_full (project, "beast-project-file-name", g_strdup (file_name), g_free);
      gchar *bname = g_path_get_basename (file_name);
      bse_project_change_name (project, bname);
      g_free (bname);
    }
  if (preserve_non_dirty && !was_dirty)
    bse_project_clean_dirty (project);
  return error;
}

BseErrorType
bst_project_import_midi_file (SfiProxy        project,
                              const gchar    *file_name)
{
  BseErrorType error = bse_project_import_midi_file (project, file_name);
  /* regardless of how good the restoration worked, try to
   * keep the resulting project in a GUI usable state.
   */
  BseItemSeq *iseq = bse_container_list_children (project);
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
        return cseq->cats[0]->category + cseq->cats[0]->lindex + 1;
    }
  return NULL;
}


/* --- generated code --- */
#include "bstgenbseapi.cc"
