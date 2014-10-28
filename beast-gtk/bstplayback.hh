// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PLAY_BACK_H__
#define __BST_PLAY_BACK_H__

#include        "bstutils.hh"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- typedefs & structures --- */
typedef void (*BstPlayBackNotify)	(gpointer	data,
					 SfiNum		tick_stamp,
					 guint		pcm_position);
typedef struct
{
  SfiProxy project;
  SfiProxy snet;
  SfiProxy speaker;
  SfiProxy wosc1, wosc2;
  SfiProxy constant;
  guint             current_delay;
  guint             pcm_timeout;
  BstPlayBackNotify pcm_notify;
  gpointer          pcm_data;
  guint		waiting_for_notify : 1;
  guint		discard_next_notify : 1;
} BstPlayBackHandle;


/* --- functions --- */
BstPlayBackHandle* bst_play_back_handle_new		(void);
void		   bst_play_back_handle_set		(BstPlayBackHandle	*handle,
							 SfiProxy		 esample,
							 gdouble		 osc_freq);
void		   bst_play_back_handle_start		(BstPlayBackHandle	*handle);
void		   bst_play_back_handle_seek_perc	(BstPlayBackHandle	*handle,
							 gfloat			 perc);
void		   bst_play_back_handle_stop		(BstPlayBackHandle	*handle);
void		   bst_play_back_handle_toggle		(BstPlayBackHandle	*handle);
void		   bst_play_back_handle_pcm_notify	(BstPlayBackHandle	*handle,
							 guint			 timeout,
							 BstPlayBackNotify	 notify,
							 gpointer		 data);
void		   bst_play_back_handle_time_pcm_notify	(BstPlayBackHandle	*handle,
							 guint			 timeout);
gboolean	   bst_play_back_handle_is_playing	(BstPlayBackHandle	*handle);
gboolean	   bst_play_back_handle_done		(BstPlayBackHandle	*handle);
void		   bst_play_back_handle_destroy		(BstPlayBackHandle	*handle);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BST_PLAY_BACK_H__ */
