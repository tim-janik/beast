// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __SFI_FILE_CRAWLER_H__
#define __SFI_FILE_CRAWLER_H__

#include <sfi/sfitypes.hh>
#include <sfi/sfiring.hh>

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

gchar*          sfi_path_get_filename                   (const gchar  *filename,
                                                         const gchar  *parentdir);
void            sfi_make_dirpath                        (const gchar  *dir);
void            sfi_make_dirname_path                   (const gchar  *filename);

/* --- file tests --- */
gboolean        g_file_test_all                         (const gchar  *filename,
                                                         GFileTest     test);


/* --- implementations --- */
void _sfi_init_file_crawler (void);

G_END_DECLS

#endif /* __SFI_FILE_CRAWLER_H__ */

/* vim:set ts=8 sts=2 sw=2: */
