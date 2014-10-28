// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PARAM_H__
#define __BST_PARAM_H__

#include	"bstutils.hh"

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
SfiProxy     bst_item_seq_list_match  (GSList      *proxy_seq_slist,    /* bstparam-proxy.cc */
                                       const gchar *text);


/* --- param implementation utils --- */
void         _bst_init_params         (void);



G_END_DECLS

#endif /* __BST_PARAM_H__ */

/* vim:set ts=8 sts=2 sw=2: */
