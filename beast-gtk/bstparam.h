/* BEAST - Bedevilled Audio System
 * Copyright (C) 2002-2004 Tim Janik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __BST_PARAM_H__
#define __BST_PARAM_H__

#include	"bstutils.h"

G_BEGIN_DECLS

/* --- parameters gmasks --- */
BstGMask*    bst_param_create_gmask      (GxkParam    *param,
                                          const gchar *editor_name,
                                          GtkWidget   *parent);
BstGMask*    bst_param_create_col_gmask  (GxkParam    *param,
                                          const gchar *editor_name,
                                          GtkWidget   *parent,
                                          guint        column);
BstGMask*    bst_param_create_span_gmask (GxkParam    *param,
                                          const gchar *editor_name,
                                          GtkWidget   *parent,
                                          guint        column);


/* --- SfiValue parameters --- */
GxkParam*    bst_param_new_value      (GParamSpec          *pspec,      /* gxk_param_new_value() */
                                       GxkParamValueNotify  notify,
                                       gpointer             notify_data);

/* --- SfiRec parameters --- */
GxkParam*    bst_param_new_rec        (GParamSpec  *pspec,
                                       SfiRec      *rec);


/* --- GObject parameters --- */
GxkParam*    bst_param_new_object     (GParamSpec  *pspec,              /* gxk_param_new_object() */
                                       GObject     *object);
/* --- SfiProxy parameters --- */
GxkParam*    bst_param_new_proxy      (GParamSpec  *pspec,
                                       SfiProxy     proxy);
void         bst_param_set_proxy      (GxkParam    *param,
                                       SfiProxy     proxy);
SfiProxy     bst_param_get_proxy      (GxkParam    *param);
SfiProxy     bst_item_seq_list_match  (GSList      *proxy_seq_slist,    /* bstparam-proxy.c */
                                       const gchar *text);


/* --- param implementation utils --- */
void         _bst_init_params         (void);



G_END_DECLS

#endif /* __BST_PARAM_H__ */

/* vim:set ts=8 sts=2 sw=2: */
