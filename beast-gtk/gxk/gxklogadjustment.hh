// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __GXK_LOG_ADJUSTMENT_H__
#define __GXK_LOG_ADJUSTMENT_H__

#include <gxk/gxkutils.hh>

G_BEGIN_DECLS

/* --- type macros --- */
#define GXK_TYPE_ADAPTER_ADJUSTMENT              (gxk_adapter_adjustment_get_type ())
#define GXK_ADAPTER_ADJUSTMENT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_ADAPTER_ADJUSTMENT, GxkAdapterAdjustment))
#define GXK_ADAPTER_ADJUSTMENT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_ADAPTER_ADJUSTMENT, GxkAdapterAdjustmentClass))
#define GXK_IS_ADAPTER_ADJUSTMENT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_ADAPTER_ADJUSTMENT))
#define GXK_IS_ADAPTER_ADJUSTMENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_ADAPTER_ADJUSTMENT))
#define GXK_ADAPTER_ADJUSTMENT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_ADAPTER_ADJUSTMENT, GxkAdapterAdjustmentClass))

/* --- structures & typedefs --- */
typedef	struct	_GxkAdapterAdjustment	             GxkAdapterAdjustment;
typedef	struct	_GxkAdapterAdjustmentClass	     GxkAdapterAdjustmentClass;
typedef enum {
  GXK_ADAPTER_ADJUSTMENT_CONVERT_TO_CLIENT,
  GXK_ADAPTER_ADJUSTMENT_CONVERT_FROM_CLIENT,
  GXK_ADAPTER_ADJUSTMENT_CONVERT_STEP_INCREMENT, /* from client */
  GXK_ADAPTER_ADJUSTMENT_CONVERT_PAGE_INCREMENT, /* from client */
  GXK_ADAPTER_ADJUSTMENT_CONVERT_PAGE_SIZE,      /* from client */
} GxkAdapterAdjustmentConvertType;
typedef gdouble (*GxkAdapterAdjustmentFunc) (GxkAdapterAdjustment           *self,
                                             GxkAdapterAdjustmentConvertType convert_type,
                                             gdouble                           value,
                                             gpointer                          data);
struct _GxkAdapterAdjustment
{
  GtkAdjustment	             parent_instance;

  guint		             block_client;
  GtkAdjustment             *client;

  GxkAdapterAdjustmentFunc conv_func;
  gpointer                   data;
  GDestroyNotify             destroy;
};
struct _GxkAdapterAdjustmentClass
{
  GtkAdjustmentClass parent_class;
};

/* --- prototypes --- */
GType          gxk_adapter_adjustment_get_type   (void);
void           gxk_adapter_adjustment_set_client (GxkAdapterAdjustment          *self,
                                                  GtkAdjustment                 *client);
void           gxk_adapter_adjustment_setup      (GxkAdapterAdjustment          *self,
                                                  GxkAdapterAdjustmentFunc       conv_func,
                                                  gpointer                       data,
                                                  GDestroyNotify                 destroy);
GtkAdjustment* gxk_adapter_adjustment_from_adj   (GtkAdjustment                 *client,
                                                  GxkAdapterAdjustmentFunc       conv_func,
                                                  gpointer                       data,
                                                  GDestroyNotify                 destroy);

/* --- type macros --- */
#define GXK_TYPE_LOG_ADJUSTMENT              (gxk_log_adjustment_get_type ())
#define GXK_LOG_ADJUSTMENT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GXK_TYPE_LOG_ADJUSTMENT, GxkLogAdjustment))
#define GXK_LOG_ADJUSTMENT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GXK_TYPE_LOG_ADJUSTMENT, GxkLogAdjustmentClass))
#define GXK_IS_LOG_ADJUSTMENT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GXK_TYPE_LOG_ADJUSTMENT))
#define GXK_IS_LOG_ADJUSTMENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GXK_TYPE_LOG_ADJUSTMENT))
#define GXK_LOG_ADJUSTMENT_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), GXK_TYPE_LOG_ADJUSTMENT, GxkLogAdjustmentClass))

/* --- structures & typedefs --- */
typedef	struct	_GxkLogAdjustment	GxkLogAdjustment;
typedef	struct	_GxkLogAdjustmentClass	GxkLogAdjustmentClass;
struct _GxkLogAdjustment
{
  GtkAdjustment	parent_instance;

  /* settings */
  gdouble	 center;
  gdouble	 n_steps;
  gdouble	 base;
  GtkAdjustment *client;

  guint		 block_client;
  gdouble	 base_ln;
  gdouble	 llimit;
  gdouble	 ulimit;
};
struct _GxkLogAdjustmentClass
{
  GtkAdjustmentClass parent_class;
};

/* --- prototypes --- */
GType          gxk_log_adjustment_get_type   (void);
void           gxk_log_adjustment_set_client (GxkLogAdjustment *self,
                                              GtkAdjustment    *client);
GtkAdjustment* gxk_log_adjustment_from_adj   (GtkAdjustment    *client);
void           gxk_log_adjustment_setup      (GxkLogAdjustment *self,
                                              gdouble           center,
                                              gdouble           base,
                                              gdouble           n_steps);
G_END_DECLS

#endif /* __GXK_LOG_ADJUSTMENT_H__ */
