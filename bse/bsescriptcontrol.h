/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2002 Tim Janik
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
#ifndef __BSE_SCRIPT_CONTROL_H__
#define __BSE_SCRIPT_CONTROL_H__

#include        <bse/bseitem.h>


/* --- object type macros --- */
#define	BSE_TYPE_SCRIPT_CONTROL	             (BSE_TYPE_ID (BseScriptControl))
#define BSE_SCRIPT_CONTROL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_SCRIPT_CONTROL, BseScriptControl))
#define BSE_SCRIPT_CONTROL_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_SCRIPT_CONTROL, BseScriptControlClass))
#define BSE_IS_SCRIPT_CONTROL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_SCRIPT_CONTROL))
#define BSE_IS_SCRIPT_CONTROL_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_SCRIPT_CONTROL))
#define BSE_SCRIPT_CONTROL_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_SCRIPT_CONTROL, BseScriptControlClass))


/* --- object structures --- */
struct _BseScriptControl
{
  BseItem	parent_instance;

  BseUserMsgType user_msg_type;
  gchar         *user_msg;

  guint		block_exec_status : 1;
  BseErrorType	error_status;

  BseComWire   *wire;
  GSource      *source;
  gchar	       *file_name;

  GSList       *actions;
};
struct _BseScriptControlClass
{
  BseItemClass parent_class;
};
typedef struct {
  GQuark action;
  gchar *name;
  gchar *blurb;
} BseScriptControlAction;


/* --- prototypes --- */
BseScriptControl*	bse_script_control_new		     (BseComWire	*wire);
void			bse_script_control_preset_error	     (BseScriptControl	*self,
							      BseErrorType	 error);
void			bse_script_control_queue_kill	     (BseScriptControl	*self);
void			bse_script_control_block_exec_status (BseScriptControl	*self,
							      gboolean		 block_exec);
const gchar*		bse_script_control_get_ident	     (BseScriptControl	*self);
const gchar*		bse_script_control_get_file_name     (BseScriptControl	*self);
void			bse_script_control_set_file_name     (BseScriptControl	*self,
							      const gchar	*file_name);
void			bse_script_control_push_current	     (BseScriptControl	*self);
BseScriptControl*	bse_script_control_peek_current	     (void);
void			bse_script_control_pop_current	     (void);
void			bse_script_control_add_action	     (BseScriptControl	*self,
							      const gchar	*action,
							      const gchar	*name,
							      const gchar	*blurb);
void			bse_script_control_remove_action    (BseScriptControl	*self,
							      const gchar       *action);
void			bse_script_control_trigger_action    (BseScriptControl	*self,
							      const gchar       *action);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BSE_SCRIPT_CONTROL_H__ */
