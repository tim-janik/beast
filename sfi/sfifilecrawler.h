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
#ifndef __SFI_FILE_CRAWLER_H__
#define __SFI_FILE_CRAWLER_H__

#include <sfi/sfitypes.h>

G_BEGIN_DECLS

typedef struct {
  SfiRing      *results;        /* end user results */
  /*< private >*/
  gchar	       *cwd;
  SfiRing      *dpatterns;	/* gchar*, directory patterns */
  GFileTest     ptest;
  /* path crawler */
  SfiRing      *pdqueue;	/* dir segments of current search dir */
  GFileTest	stest;		/* final segment file test */
  SfiRing      *dlist;		/* dir list */
  /* dir crawler */
  gpointer	dhandle;
  GPatternSpec *pspec;		/* file pattern */
  gchar	       *base_dir;
  GFileTest	ftest;
  SfiRing      *accu;		/* readdir result */
} SfiFileCrawler;

SfiFileCrawler*	sfi_file_crawler_new			(void);
gchar*		sfi_file_crawler_pop			(SfiFileCrawler	*self);
void		sfi_file_crawler_set_cwd		(SfiFileCrawler *self,
							 const gchar	*cwd);
void		sfi_file_crawler_add_search_path	(SfiFileCrawler	*self,
							 const gchar	*pattern_paths,
                                                         const gchar    *file_pattern);
void		sfi_file_crawler_add_tests      	(SfiFileCrawler	*self,
                                                         GFileTest       tests);
void		sfi_file_crawler_crawl			(SfiFileCrawler *self);
gboolean	sfi_file_crawler_needs_crawl		(SfiFileCrawler *self);
void		sfi_file_crawler_destroy		(SfiFileCrawler	*self);

SfiRing*        sfi_file_crawler_list_files             (const gchar *search_path,
                                                         const gchar *file_pattern,
                                                         GFileTest    file_test);

gboolean        g_file_test_all                         (const gchar  *filename,
                                                         GFileTest     test);


/* --- implementations --- */
void _sfi_init_file_crawler (void);

G_END_DECLS

#endif /* __SFI_FILE_CRAWLER_H__ */

/* vim:set ts=8 sts=2 sw=2: */
