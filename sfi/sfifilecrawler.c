/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "sfifilecrawler.h"
#include "sfiprimitives.h"
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#define INCREMENTAL_RESULTS 1


/* --- functions --- */
/**
 * sfi_file_crawler_new
 *
 * Create a new file crawler. A file crawler collects all files matching
 * a given search path and file test.
 * sfi_file_crawler_crawl() needs to be called as long as
 * sfi_file_crawler_needs_crawl() returns %TRUE to collect all
 * matching files.
 */
SfiFileCrawler*
sfi_file_crawler_new (void)
{
  SfiFileCrawler *self = g_new0 (SfiFileCrawler, 1);
  self->cwd = g_get_current_dir ();
  return self;
}

/**
 * sfi_file_crawler_pop
 * @self:   valid #SfiFileCrawler
 * RETURNS: newly allocated string containig resulting filename
 *
 * Fetch next result if any or %NULL.
 */
gchar*
sfi_file_crawler_pop (SfiFileCrawler *self)
{
  g_return_val_if_fail (self != NULL, NULL);
  return sfi_ring_pop_head (&self->results);
}

/**
 * sfi_file_crawler_set_cwd
 * @self: valid #SfiFileCrawler
 * @cwd:  absolute path
 *
 * Set the path to be assumed the current working directory.
 */
void
sfi_file_crawler_set_cwd (SfiFileCrawler *self,
			  const gchar    *cwd)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (cwd != NULL && g_path_is_absolute (cwd));

  g_free (self->cwd);
  self->cwd = g_strdup (cwd);
}

/**
 * sfi_file_crawler_add_search_path
 * @self:          valid #SfiFileCrawler
 * @pattern_paths: colon (semicolon under win32) seperated search path
 * @RETURNS:       a singly linked list with newly allocated strings
 *
 * This function takes a search path (possibly containing wildcards)
 * and adds them to the file crawlers search list.
 * sfi_file_crawler_needs_crawl() may return %TRUE after calling
 * this function even if it returned %FALSE beforehand.
 */
void
sfi_file_crawler_add_search_path (SfiFileCrawler *self,
				  const gchar    *pattern_paths)
{
  g_return_if_fail (self != NULL);
  if (pattern_paths)
    {
      const gchar *sep, *p = pattern_paths;
      
      sep = strchr (p, G_SEARCHPATH_SEPARATOR);
      while (sep)
	{
	  if (sep > p)
	    self->dpatterns = sfi_ring_append (self->dpatterns, g_strndup (p, sep - p));
	  p = sep + 1;
	  sep = strchr (p, G_SEARCHPATH_SEPARATOR);
	}
      if (*p)
	self->dpatterns = sfi_ring_append (self->dpatterns, g_strdup (p));
    }
}

static void
file_crawler_queue_readdir (SfiFileCrawler *self,
			    const gchar    *base_dir,
			    const gchar    *file_pattern,
			    GFileTest       file_test)
{
  g_assert (self->dhandle == NULL);
  
  file_test = file_test ? file_test : G_FILE_TEST_EXISTS;
  if (strchr (file_pattern, '?') || strchr (file_pattern, '*'))
    {
      gchar *s = g_strconcat (base_dir, G_DIR_SEPARATOR_S, NULL);
      self->dhandle = opendir (s);
      g_free (s);
      if (self->dhandle)
	{
	  self->pspec = g_pattern_spec_new (file_pattern);
	  self->base_dir = g_strdup (base_dir);
	  self->ftest = file_test;
	}
    }
  else
    {
      gchar *s;
      if (strcmp (file_pattern, ".") == 0)
	s = g_strdup (base_dir);
      else
	s = g_strconcat (base_dir, G_DIR_SEPARATOR_S, file_pattern, NULL);
      if (!g_file_test (s, file_test))
	g_free (s);
      else
	self->accu = sfi_ring_prepend (self->accu, s);
    }
}

static void	/* self->accu is implicit in/out arg */
file_crawler_crawl_readdir (SfiFileCrawler *self)
{
  DIR *dd = self->dhandle;
  struct dirent *d_entry = readdir (dd);
  
  if (d_entry)
    {
      if (!(d_entry->d_name[0] == '.' && d_entry->d_name[1] == 0) &&
	  !(d_entry->d_name[0] == '.' && d_entry->d_name[1] == '.' && d_entry->d_name[2] == 0) &&
	  g_pattern_match_string (self->pspec, d_entry->d_name))
	{
	  gchar *str = g_strconcat (self->base_dir, G_DIR_SEPARATOR_S, d_entry->d_name, NULL);
	  if (self->ftest && !g_file_test (str, self->ftest))
	    g_free (str);
	  else
	    self->accu = sfi_ring_prepend (self->accu, str);
	}
    }
  else
    {
      g_pattern_spec_free (self->pspec);
      self->pspec = NULL;
      g_free (self->base_dir);
      self->base_dir = NULL;
      closedir (dd);
      self->dhandle = NULL;
      self->ftest = 0;
    }
}

static void
file_crawler_queue_abs_file_path (SfiFileCrawler *self,
				  const gchar    *path_pattern,
				  GFileTest       file_test)
{
  gchar *sep, *p, *freeme, *tmp;

  g_assert (self->pdqueue == NULL && self->dlist == NULL && self->accu == NULL);

  freeme = p = g_strdup (path_pattern);
  
  /* seperate root */
  sep = strchr (p, G_DIR_SEPARATOR);
  g_return_if_fail (sep != NULL);	/* absolute paths must have a seperator */
  *sep++ = 0;
  
  /* check root existance */
  tmp = g_strconcat (p, G_DIR_SEPARATOR_S, NULL);
  if (!g_file_test (tmp, G_FILE_TEST_IS_DIR))
    {
      g_free (tmp);
      g_free (freeme);
      return;
    }
  g_free (tmp);
  
  /* add root to dir list ("" on unix) */
  self->dlist = sfi_ring_prepend (self->dlist, g_strdup (p));
  
  /* compress multiple dir seperators */
  while (*sep == G_DIR_SEPARATOR)
    sep++;
  
  /* add remaining segments to queue */
  p = sep;
  sep = strchr (p, G_DIR_SEPARATOR);
  while (sep)
    {
      *sep++ = 0;
      self->pdqueue = sfi_ring_append (self->pdqueue, g_strdup (p));
      /* compress multiple dir seperators */
      while (*sep == G_DIR_SEPARATOR)
	sep++;
      p = sep;
      sep = strchr (p, G_DIR_SEPARATOR);
    }
  
  /* final segment */
  if (p[0])
    self->pdqueue = sfi_ring_append (self->pdqueue, g_strdup (p));
  
  /* final segment test */
  self->stest = file_test ? file_test : G_FILE_TEST_EXISTS;
  
  /* cleanup */
  g_free (freeme);
}

static void
file_crawler_crawl_abs_path (SfiFileCrawler *self)
{
  g_assert (self->pdqueue || self->dlist);
  if (self->dhandle)
    {
      /* finish reading directory contents */
      file_crawler_crawl_readdir (self);
#if INCREMENTAL_RESULTS
      /* on final segment, directly return results */
      if (sfi_ring_test_length (self->pdqueue, 1))
	{
	  self->results = sfi_ring_concat (self->results, self->accu);
	  self->accu = NULL;
	}
#endif
      return;
    }
  if (!self->dlist)
    {
      /* collect crawled directories */
      self->dlist = self->accu;
      self->accu = NULL;
      /* free last processed segment */
      g_free (sfi_ring_pop_head (&self->pdqueue));
    }
  if (self->dlist && !self->pdqueue)
    {
      /* we're done, return result */
      self->results = sfi_ring_concat (self->results, self->dlist);
      self->dlist = NULL;
    }
  else if (self->dlist) /* && self->pdqueue */
    {
      gchar *dir = sfi_ring_pop_head (&self->dlist);
      gchar *pattern = self->pdqueue->data;
      GFileTest ftest = self->pdqueue->next != self->pdqueue ? G_FILE_TEST_IS_DIR : self->stest;
      /* continue reading {dir-list}/pattern files */
      file_crawler_queue_readdir (self, dir, pattern, ftest);
      g_free (dir);
    }
  else /* !self->dlist */
    while (self->pdqueue)
      {
	gchar *seg = sfi_ring_pop_head (&self->pdqueue);
	g_free (seg);
	/* directory path was a dead end, we're done, no result */
      }
}

static void
file_crawler_crawl_dpatterns (SfiFileCrawler *self)
{
  gchar *dpattern = sfi_ring_pop_head (&self->dpatterns);
  if (dpattern)
    {
      /* make absolute */
      if (!g_path_is_absolute (dpattern))
	{
	  gchar *tmp = g_strconcat (self->cwd, G_DIR_SEPARATOR_S, dpattern, NULL);
	  file_crawler_queue_abs_file_path (self, tmp, G_FILE_TEST_EXISTS);
	  g_free (tmp);
	}
      else
	file_crawler_queue_abs_file_path (self, dpattern, G_FILE_TEST_EXISTS);
    }
}

/**
 * sfi_file_crawler_needs_crawl
 * @self:   valid #SfiFileCrawler
 * RETURNS: %TRUE if sfi_file_crawler_crawl() should be called
 *
 * Figure whether collecting all matching files has finished
 * now. If not, sfi_file_crawler_crawl() needs to be called
 * until this function returns %FALSE.
 */
gboolean
sfi_file_crawler_needs_crawl (SfiFileCrawler *self)
{
  g_return_val_if_fail (self != NULL, FALSE);
  
  return (self->dpatterns ||
	  self->pdqueue || self->dlist ||
	  self->dhandle);
}

/**
 * sfi_file_crawler_crawl
 * @self: valid #SfiFileCrawler
 *
 * Collect the next file or directory if possible,
 * new results need not arrive after calling this
 * function, and more than one may. This function
 * does nothing if sfi_file_crawler_needs_crawl()
 * returns %FALSE.
 */
void
sfi_file_crawler_crawl (SfiFileCrawler *self)
{
  g_return_if_fail (self != NULL);
  if (self->dhandle)
    {
#if INCREMENTAL_RESULTS
      if (self->pdqueue || self->dlist)
	file_crawler_crawl_abs_path (self);
      else
#endif
	file_crawler_crawl_readdir (self);
    }
  else if (self->pdqueue || self->dlist)
    file_crawler_crawl_abs_path (self);
  else if (self->dpatterns)
    file_crawler_crawl_dpatterns (self);
}

/**
 * sfi_file_crawler_destroy
 * @self: valid #SfiFileCrawler
 *
 * Destroy an existing file crawler and free any resources
 * allocated by it.
 */
void
sfi_file_crawler_destroy (SfiFileCrawler *self)
{
  g_return_if_fail (self != NULL);

  g_free (self->cwd);
  sfi_ring_free_deep (self->results, g_free);
  sfi_ring_free_deep (self->dpatterns, g_free);
  sfi_ring_free_deep (self->pdqueue, g_free);
  sfi_ring_free_deep (self->dlist, g_free);
  if (self->pspec)
    g_pattern_spec_free (self->pspec);
  g_free (self->base_dir);
  sfi_ring_free_deep (self->accu, g_free);
  g_free (self);
}
