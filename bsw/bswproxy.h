/* BSW - Bedevilled Sound Engine Wrapper
 * Copyright (C) 2000-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BSW_PROXY_H__
#define __BSW_PROXY_H__

extern char *bsw_log_domain_bsw;
#include        <bse/bswcommon.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define	BSW_SERVER		(bsw_proxy_get_server ())

typedef struct
{
  guint  n_ivalues;
  GValue ivalues[16];
  GValue ovalue;
  gchar *proc_name;
} BswProxyProcedureCall;
typedef struct
{
  gpointer        lock_data;
  void          (*lock)         (gpointer lock_data);
  void          (*unlock)       (gpointer lock_data);
} BswLockFuncs;

void		bsw_init			(gint			*argc,
						 gchar		       **argv[],
						 const BswLockFuncs	*lock_funcs);
void		bsw_proxy_call_procedure	(BswProxyProcedureCall	*closure);
BswProxy	bsw_proxy_get_server		(void)	G_GNUC_CONST;
void		bsw_proxy_set			(BswProxy		 proxy,
						 const gchar		*prop,
						 ...);
GParamSpec*	bsw_proxy_get_pspec		(BswProxy		 proxy,
						 const gchar		*name);
GType		bsw_proxy_type			(BswProxy		 proxy);
gboolean	bsw_proxy_check_is_a		(BswProxy		 proxy,
						 GType			 type);
void		bsw_proxy_set_data		(BswProxy		 proxy,
						 const gchar		*name,
						 gpointer		 data);
gpointer	bsw_proxy_get_data		(BswProxy		 proxy,
						 const gchar		*name);
void		bsw_proxy_remove_data		(BswProxy		 proxy,
						 const gchar		*name);


/* --- garbage collection --- */
gchar*		bsw_collector_get_string	(GValue			*value);


/* --- manual glue --- */
#define BSW_MIN_NOTE                    (0)
#define BSW_MAX_NOTE                    (131)
#define BSW_NOTE_VOID                   (BSW_MAX_NOTE + 1)
#define BSW_NOTE_UNPARSABLE             (BSW_NOTE_VOID + 1)
#define BSW_KAMMER_NOTE                 ((gint) (69) /* A' */)
#define BSW_KAMMER_OCTAVE               ((gint) (+1))
#define BSW_MIN_OCTAVE                  (BSW_NOTE_OCTAVE (BSW_MIN_NOTE))
#define BSW_MAX_OCTAVE                  (BSW_NOTE_OCTAVE (BSW_MAX_NOTE))
#define BSW_NOTE_MAKE_VALID(n)          ((n) > BSW_MAX_NOTE || (n) < BSW_MIN_NOTE ? BSW_KAMMER_NOTE : ((gint) (n)))
#define BSW_NOTE_IS_VALID(n)            ((n) >= BSW_MIN_NOTE && (n) <= BSW_MAX_NOTE)
#define BSW_NOTE_CLAMP(n)               (CLAMP (((gint) (n)), BSW_MIN_NOTE, BSW_MAX_NOTE))
#define BSW_NOTE_OCTAVE(n)              ((((gint) (n)) - BSW_NOTE_SEMITONE (n) - (BSW_KAMMER_NOTE - 9)) / 12 + BSW_KAMMER_OCTAVE)
#define BSW_NOTE_SEMITONE(n)            (((gint) (n)) % 12 + (9 - (BSW_KAMMER_NOTE % 12)))
#define BSW_NOTE_GENERIC(o,ht_i)        (BSW_KAMMER_NOTE - 9 + ((gint) (ht_i)) + (((gint) (o)) - BSW_KAMMER_OCTAVE) * 12)
#define BSW_NOTE_C(o)                   (BSW_NOTE_GENERIC ((o), 0))
#define BSW_NOTE_Cis(o)                 (BSW_NOTE_GENERIC ((o), 1))
#define BSW_NOTE_Des(o)                 (BSW_NOTE_Cis (o))
#define BSW_NOTE_D(o)                   (BSW_NOTE_GENERIC ((o), 2))
#define BSW_NOTE_Dis(o)                 (BSW_NOTE_GENERIC ((o), 3))
#define BSW_NOTE_Es(o)                  (BSW_NOTE_Dis (o))
#define BSW_NOTE_E(o)                   (BSW_NOTE_GENERIC ((o), 4))
#define BSW_NOTE_F(o)                   (BSW_NOTE_GENERIC ((o), 5))
#define BSW_NOTE_Fis(o)                 (BSW_NOTE_GENERIC ((o), 6))
#define BSW_NOTE_Ges(o)                 (BSW_NOTE_Fis (o))
#define BSW_NOTE_G(o)                   (BSW_NOTE_GENERIC ((o), 7))
#define BSW_NOTE_Gis(o)                 (BSW_NOTE_GENERIC ((o), 8))
#define BSW_NOTE_As(o)                  (BSW_NOTE_Gis (o))
#define BSW_NOTE_A(o)                   (BSW_NOTE_GENERIC ((o), 9))
#define BSW_NOTE_Ais(o)                 (BSW_NOTE_GENERIC ((o), 10))
#define BSW_NOTE_Bes(o)                 (BSW_NOTE_Ais (o))
#define BSW_NOTE_B(o)                   (BSW_NOTE_GENERIC ((o), 11))
#define _BSW_NOTE_SHIFT_AUX(n,ht,dfl)   (n + ht >= BSW_MIN_NOTE && n + ht <= BSW_MAX_NOTE ? n + ht : dfl)
#define BSW_NOTE_SHIFT(n,ht_i)          (_BSW_NOTE_SHIFT_AUX ((gint) (n), (gint) (ht), (gint) (n)))
#define BSW_NOTE_OCTAVE_UP(n)           (BSW_NOTE_SHIFT ((n), +12))
#define BSW_NOTE_OCTAVE_DOWN(n)         (BSW_NOTE_SHIFT ((n), -12))
#define BSW_MIN_FINE_TUNE               (-100)
#define BSW_MAX_FINE_TUNE               (+100)
#define BSW_FINE_TUNE_IS_VALID(n)       ((n) >= BSW_MIN_FINE_TUNE && (n) <= BSW_MAX_FINE_TUNE)

												  




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSW_PROXY_H__ */
