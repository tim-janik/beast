// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_GLOBALS_H__
#define __BSE_GLOBALS_H__
#include <bse/bsedefs.hh>
#include <bse/bsemath.hh>
#include <bse/bsenote.hh>
#include <bse/bseconstvalues.hh>
G_BEGIN_DECLS
/* --- time ranges --- */ // FIXME: BSE_TIME_RANGE is deprecated
typedef enum
{
  BSE_TIME_RANGE_SHORT	= 1,
  BSE_TIME_RANGE_MEDIUM,
  BSE_TIME_RANGE_LONG
} BseTimeRangeType;
#define	BSE_TIME_RANGE_SHORT_ms		(1000.0 *   0.5)
#define	BSE_TIME_RANGE_MEDIUM_ms	(1000.0 *  10.0)
#define	BSE_TIME_RANGE_LONG_ms		(1000.0 * 200.0)
glong	bse_time_range_to_ms		(BseTimeRangeType	time_range);
/* --- async handlers --- */
/* most important, used for immediate async execution */
#define	BSE_PRIORITY_NOW		(-G_MAXINT / 2)
/* very important, used for io/engine handlers */
#define	BSE_PRIORITY_HIGH		(G_PRIORITY_HIGH - 10)
/* still very important, used for need-to-be-async operations */
#define	BSE_PRIORITY_NEXT		(G_PRIORITY_HIGH - 5)
/* important, delivers async signals */
#define	BSE_PRIORITY_NOTIFY		(G_PRIORITY_DEFAULT - 1)
/* normal importantance, interfaces to glue layer */
#define	BSE_PRIORITY_NORMAL		(G_PRIORITY_DEFAULT)
#define	BSE_PRIORITY_GLUE		(BSE_PRIORITY_NORMAL)
/* mildly important, used for GUI updates or user information */
#define	BSE_PRIORITY_UPDATE		(G_PRIORITY_HIGH_IDLE + 5)
/* unimportant, used when everything else done */
#define BSE_PRIORITY_BACKGROUND		(G_PRIORITY_LOW + 500)
guint	  bse_idle_now		(GSourceFunc    function,
				 gpointer       data);
guint	  bse_idle_next		(GSourceFunc    function,
				 gpointer       data);
guint	  bse_idle_notify	(GSourceFunc    function,
				 gpointer       data);
guint	  bse_idle_normal	(GSourceFunc    function,
				 gpointer       data);
guint	  bse_idle_update	(GSourceFunc    function,
				 gpointer       data);
guint	  bse_idle_background	(GSourceFunc    function,
				 gpointer       data);
gboolean  bse_idle_remove	(guint		id);
guint	  bse_idle_timed	(guint64	usec_delay,
				 GSourceFunc    function,
				 gpointer       data);
/* semitone factorization tables, i.e.
 * Index                     Factor
 * (SFI_KAMMER_NOTE - 12) -> 0.5
 * SFI_KAMMER_NOTE	  -> 1.0
 * (SFI_KAMMER_NOTE + 12) -> 2.0
 * etc...
 */
#define	BSE_TRANSPOSE_FACTOR(st)	(bse_transpose_factor (CLAMP (st, -132, +132)))	/* BSE_MAX_TRANSPOSE */
/* --- prototypes --- */
void		bse_globals_init	(void);
/* --- decibel conversion --- */
gdouble	bse_db_to_factor	(gdouble	dB);
gdouble	bse_db_from_factor	(gdouble	factor,
                                 gdouble	min_dB);
#define	BSE_MINDB               (-96)   /* 32bit:-192 24bit:-144 16bit:-96 */
G_END_DECLS
#endif /* __BSE_GLOBALS_H__ */
