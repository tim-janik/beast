/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstsamplerepo.h"

#include "bstconfigpaths.h"

#include <fcntl.h>
#include <errno.h>



/* --- prototypes --- */
static GSList*	list_files_rec		(GSList		*slist,
					 const gchar	*dir_name,
					 GPatternSpec	*pspec);


/* --- static variables --- */
static GSList	*bst_repos = NULL;


/* --- functions --- */
BstSampleRepo*
bst_sample_repo_new (const gchar *name)
{
  BstSampleRepo *repo;

  g_return_val_if_fail (name != NULL, NULL);

  repo = g_new (BstSampleRepo, 1);
  repo->name = g_strdup (name);
  repo->sample_locs = NULL;

  bst_repos = g_slist_prepend (bst_repos, repo);

  return repo;
}

void
bst_sample_repo_add_sample (BstSampleRepo *repo,
			    const gchar   *sample_name)
{
  BstSampleLoc *loc;

  g_return_if_fail (repo != NULL);
  g_return_if_fail (sample_name != NULL);

  loc = g_new (BstSampleLoc, 1);
  loc->repo = repo;
  loc->name = g_strdup (sample_name);
  repo->sample_locs = g_slist_prepend (repo->sample_locs, loc);
}

GList*
bst_sample_repo_list_sample_locs (void)
{
  GList *list = NULL;
  GSList *slist;

  for (slist = bst_repos; slist; slist = slist->next)
    {
      BstSampleRepo *repo = slist->data;
      GSList *node;

      for (node = repo->sample_locs; node; node = node->next)
	list = g_list_prepend (list, node->data);
    }

  return list;
}

BstSampleLoc*
bst_sample_repo_find_sample_loc (const gchar *sample_name)
{
  GSList *slist;

  g_return_val_if_fail (sample_name != NULL, NULL);

  for (slist = bst_repos; slist; slist = slist->next)
    {
      BstSampleRepo *repo = slist->data;
      GSList *node;
      
      for (node = repo->sample_locs; node; node = node->next)
	{
	  BstSampleLoc *loc = node->data;

	  if (strcmp (sample_name, loc->name) == 0)
	    return loc;
	}
    }

  return NULL;
}

BseSample*
bst_sample_repo_load_sample (BstSampleLoc *loc,
			     BseProject   *project)
{
  BseStorage *storage;
  BseErrorType error;
  gchar *file_name;
  GSList *slist;

  g_return_val_if_fail (loc != NULL, NULL);
  g_return_val_if_fail (BSE_IS_PROJECT (project), NULL);

  file_name = loc->repo->name;

  /* FIXME: need magic identification */

  storage = bse_storage_new ();
  error = bse_storage_input_file (storage, file_name);
  if (error)
    {
      g_message ("failed to open `%s': %s", /* FIXME */
		 file_name,
		 bse_error_blurb (error));
      bse_storage_destroy (storage);

      return NULL;
    }
  else
    {
      BseProject *tmp_project = bse_project_new ("Bst-TmpPrj");

      error = bse_project_restore (tmp_project, storage);
      if (error)
	g_message ("failed to load project `%s': %s", /* FIXME */
		   file_name,
		   bse_error_blurb (error));
      
      for (slist = tmp_project->supers; slist; slist = slist->next)
	{
	  BseObject *object = slist->data;
	  
	  if (BSE_IS_SAMPLE (object) &&
	      bse_string_equals (BSE_OBJECT_NAME (object), loc->name))
	    {
	      bse_object_ref (object);
	      bse_project_remove_super (tmp_project, BSE_SUPER (object));
	      bse_project_add_super (project, BSE_SUPER (object));
	      bse_object_unref (object);

	      bse_object_unref (BSE_OBJECT (tmp_project));
	      bse_storage_destroy (storage);
	      
	      return BSE_SAMPLE (object);
	    }
	}
      bse_object_unref (BSE_OBJECT (tmp_project));
      bse_storage_destroy (storage);
    }

  g_warning ("%s: couldn't find sample \"%s\"", file_name, loc->name);

  return NULL;
}

void
bst_sample_repo_init (void)
{
  GScanner *scanner;
  GSList *slist, *sample_names = NULL;
  GPatternSpec pspec;

  g_message ("Scanning sample repositories...");

  g_pattern_spec_init (&pspec, "*.bse");
  sample_names = list_files_rec (sample_names, BST_DATA_DIR, &pspec);
  g_pattern_spec_free_segs (&pspec);
  sample_names = g_slist_reverse (sample_names);

  scanner = g_scanner_new (NULL);
  g_scanner_add_symbol (scanner, "BseSample", NULL);
  for (slist = sample_names; slist; slist = slist->next)
    {
      gchar *file_name = slist->data;
      gint fd = open (file_name, O_RDONLY);
      
      if (fd >= 0)
	{
	  g_scanner_input_file (scanner, fd);
	  while (scanner->token != G_TOKEN_EOF)
	    {
	      g_scanner_get_next_token (scanner);
	      if (scanner->token == G_TOKEN_SYMBOL)
		{
		  if (g_scanner_get_next_token (scanner) == ':' &&
		      g_scanner_get_next_token (scanner) == ':' &&
		      g_scanner_get_next_token (scanner) == G_TOKEN_IDENTIFIER)
		    bst_sample_repo_add_sample (bst_sample_repo_new (file_name),
						scanner->value.v_identifier);
		  break;
		}
	      
	    }
	  close (fd);
	}
      g_free (file_name);
    }
  g_scanner_destroy (scanner);
  g_slist_free (sample_names);

  {
    GList *free_list = bst_sample_repo_list_sample_locs (), *list;

    for (list = free_list; list; list = list->next)
      {
	BstSampleLoc *loc = list->data;

	g_print ("%s:: %s\n", loc->repo->name, loc->name);
      }
    g_list_free (free_list);
  }
}

/* --- directory scanning --- */
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
static GSList*
list_files_rec (GSList       *slist,
		const gchar  *dir_name,
		GPatternSpec *pspec)
{
  DIR *dd;

  dd = opendir (dir_name);
  if (dd)
    {
      struct dirent *d_entry = readdir (dd);

      while (d_entry)
	{
	  if (d_entry->d_name[0] != '.')
	    {
	      struct stat stat_buf = { 0, };
	      gchar *name = g_strconcat (dir_name, "/", d_entry->d_name, NULL);
	      
	      stat (name, &stat_buf);
	      
	      if (S_ISDIR (stat_buf.st_mode))
		slist = list_files_rec (slist, name, pspec);
	      else if (S_ISREG (stat_buf.st_mode) && g_pattern_match_string (pspec, name))
		slist = g_slist_prepend (slist, g_strdup (name));

	      g_free (name);
	    }

	  d_entry = readdir (dd);
	}
      closedir (dd);
    }

  return slist;
}
