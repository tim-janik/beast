// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BSE_JANITOR_H__
#define __BSE_JANITOR_H__
#include <bse/bseitem.hh>
G_BEGIN_DECLS
/* --- object type macros --- */
#define BSE_TYPE_JANITOR              (BSE_TYPE_ID (BseJanitor))
#define BSE_JANITOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_JANITOR, BseJanitor))
#define BSE_JANITOR_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_JANITOR, BseJanitorClass))
#define BSE_IS_JANITOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_JANITOR))
#define BSE_IS_JANITOR_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_JANITOR))
#define BSE_JANITOR_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_JANITOR, BseJanitorClass))
/* --- object structures --- */
struct BseJanitor : BseItem {
  guint		  port_closed : 1;
  guint		  force_kill : 1;
  guint		  force_normal_exit : 1;
  SfiComPort	 *port;
  SfiGlueContext *context;
  SfiGlueDecoder *decoder;
  GSource        *source;
  gchar          *status_message;
  gchar          *script_name;
  gchar          *proc_name;
  GSList         *actions;
  /* closed connections (port==NULL) */
  gint           exit_code;
  gchar         *exit_reason;
};
struct BseJanitorClass : BseItemClass
{};
struct BseJanitorAction {
  GQuark action;
  gchar *name;
  gchar *blurb;
};

/* --- prototypes --- */
BseJanitor*  bse_janitor_new		(SfiComPort	*port);
void	     bse_janitor_kill   	(BseJanitor	*self);
void	     bse_janitor_close  	(BseJanitor	*self);
const gchar* bse_janitor_get_ident	(BseJanitor	*self);
void	     bse_janitor_set_procedure  (BseJanitor	*self,
					 const gchar	*script,
					 const gchar	*proc);
BseJanitor*  bse_janitor_get_current	(void);
void	     bse_janitor_progress	(BseJanitor	*self,
					 gfloat		 progress);
void	     bse_janitor_add_action	(BseJanitor	*self,
					 const gchar	*action,
					 const gchar	*name,
					 const gchar	*blurb);
void	     bse_janitor_remove_action	(BseJanitor	*self,
					 const gchar	*action);
void	     bse_janitor_trigger_action	(BseJanitor	*self,
					 const gchar	*action);
G_END_DECLS
#endif /* __BSE_JANITOR_H__ */
/* vim:set ts=8 sts=2 sw=2: */
