/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
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
#include        "bseprocedure.h"

#include	"bseparamcol.c"
#include	"bseitem.h"
#include	"bseexports.h"
#include	<string.h>


#define	PROC_ENTRIES_PRE_ALLOC	(16)


/* --- structures --- */
typedef struct _ShareNode ShareNode;
struct _ShareNode
{
  GTrashStack       trash_stack_dummy;
  BseProcedureShare share_func;
  gpointer          func_data;
};


/* --- prototypes --- */
extern void	bse_type_register_procedure_info  (BseTypeInfo		    *info);
static void     bse_procedure_base_init		  (BseProcedureClass	    *proc);
static void     bse_procedure_base_destroy	  (BseProcedureClass	    *proc);
static void     bse_procedure_init		  (BseProcedureClass	    *proc,
						   const BseExportProcedure *pspec);


/* --- variables --- */
static GSList      *called_procs = NULL;
static GTrashStack *share_stack = NULL;
static GHookList    proc_notifiers = { 0, };


/* --- functions --- */
extern void
bse_type_register_procedure_info (BseTypeInfo *info)
{
  static const BseTypeInfo proc_info = {
    sizeof (BseProcedureClass),

    (BseBaseInitFunc) bse_procedure_base_init,
    (BseBaseDestroyFunc) bse_procedure_base_destroy,
    (BseClassInitFunc) NULL,
    (BseClassDestroyFunc) NULL,
    NULL /* class_data */,

    /* non classed type stuff */
    0, 0, NULL,
  };

  *info = proc_info;

  /* basic setup stuff */
  if (!proc_notifiers.is_setup)
    g_hook_list_init (&proc_notifiers, sizeof (GHook));
}

static void
bse_procedure_base_init (BseProcedureClass *proc)
{
  proc->name = NULL;
  proc->blurb = NULL;
  proc->private_id = 0;
  proc->help = NULL;
  proc->author = NULL;
  proc->copyright = NULL;
  proc->date = NULL;
  proc->n_in_params = 0;
  proc->in_param_specs = NULL;
  proc->n_out_params = 0;
  proc->out_param_specs = NULL;
  proc->execute = NULL;
}

static void
bse_procedure_base_destroy (BseProcedureClass *proc)
{
  guint i;

  proc->name = NULL;
  proc->blurb = NULL;
  proc->help = NULL;
  proc->author = NULL;
  proc->copyright = NULL;
  proc->date = NULL;

  for (i = 0; i < proc->n_in_params; i++)
    bse_param_spec_free (proc->in_param_specs[i]);
  g_free (proc->in_param_specs);
  for (i = 0; i < proc->n_out_params; i++)
    bse_param_spec_free (proc->out_param_specs[i]);
  g_free (proc->out_param_specs);

  proc->execute = NULL;
}

static void
bse_procedure_init (BseProcedureClass        *proc,
		    const BseExportProcedure *pspec)
{
  BseParamSpec *in_param_specs[BSE_PROCEDURE_MAX_IN_PARAMS + 1];
  BseParamSpec *out_param_specs[BSE_PROCEDURE_MAX_OUT_PARAMS + 1];
  guint i;
  gchar *const_name, *const_blurb;

  memset (in_param_specs, 0, sizeof (in_param_specs));
  memset (out_param_specs, 0, sizeof (out_param_specs));

  proc->name = bse_type_name (BSE_PROCEDURE_TYPE (proc));
  proc->blurb = bse_type_blurb (BSE_PROCEDURE_TYPE (proc));
  proc->private_id = pspec->private_id;

  /* init procedure class from plugin,
   * paranoia check certain class members
   */
  const_name = proc->name;
  const_blurb = proc->blurb;
  pspec->init (proc, in_param_specs, out_param_specs);
  if (proc->name != const_name)
    {
      proc->name = const_name;
      g_warning ("procedure \"%s\" redefines procedure name", proc->name);
    }
  if (proc->blurb != const_blurb)
    {
      proc->blurb = const_blurb;
      g_warning ("procedure \"%s\" redefines procedure blurb", proc->name);
    }
  if (proc->n_in_params || proc->in_param_specs ||
      proc->n_out_params || proc->out_param_specs ||
      proc->execute)
    {
      proc->n_in_params = 0;
      proc->in_param_specs = NULL;
      proc->n_out_params = 0;
      proc->out_param_specs = NULL;
      proc->execute = NULL;
      g_warning ("procedure \"%s\" messes with reserved class members", proc->name);
    }

  /* check input parameters and setup specifications
   */
  for (i = 0; i < BSE_PROCEDURE_MAX_IN_PARAMS; i++)
    if (in_param_specs[i])
      {
	if ((in_param_specs[i]->any.flags & BSE_PARAM_READWRITE) != BSE_PARAM_READWRITE)
	  g_warning ("procedure \"%s\": input parameter \"%s\" has invalid flags",
		     proc->name,
		     in_param_specs[i]->any.name);
	in_param_specs[i]->any.parent_type = BSE_PROCEDURE_TYPE (proc);
	in_param_specs[i]->any.param_group = g_quark_from_static_string ("Input Parameters");
      }
    else
      break;
  if (i == BSE_PROCEDURE_MAX_IN_PARAMS && in_param_specs[i])
    g_warning ("procedure \"%s\" exceeds maximum number of input parameters (%u)",
	       proc->name, BSE_PROCEDURE_MAX_IN_PARAMS);
  proc->n_in_params = i;
  proc->in_param_specs = g_new (BseParamSpec*, proc->n_in_params + 1);
  memcpy (proc->in_param_specs, in_param_specs, sizeof (in_param_specs[0]) * proc->n_in_params);
  proc->in_param_specs[proc->n_in_params] = NULL;

  /* check output parameters and setup specifications
   */
  for (i = 0; i < BSE_PROCEDURE_MAX_OUT_PARAMS; i++)
    if (out_param_specs[i])
      {
        if ((out_param_specs[i]->any.flags & BSE_PARAM_READWRITE) != BSE_PARAM_READWRITE)
	  g_warning ("procedure \"%s\": output parameter \"%s\" has invalid flags",
		     proc->name,
		     out_param_specs[i]->any.name);
	out_param_specs[i]->any.parent_type = BSE_PROCEDURE_TYPE (proc);
	out_param_specs[i]->any.param_group = g_quark_from_static_string ("Output Parameters");
      }
    else
      break;
  if (i == BSE_PROCEDURE_MAX_OUT_PARAMS && out_param_specs[i])
    g_warning ("procedure \"%s\" exceeds maximum number of output parameters (%u)",
	       proc->name, BSE_PROCEDURE_MAX_OUT_PARAMS);
  proc->n_out_params = i;
  proc->out_param_specs = g_new (BseParamSpec*, proc->n_out_params + 1);
  memcpy (proc->out_param_specs, out_param_specs, sizeof (out_param_specs[0]) * proc->n_out_params);
  proc->out_param_specs[proc->n_out_params] = NULL;
  
  proc->execute = pspec->exec;
}

void
bse_procedure_complete_info (const BseExportSpec *spec,
			     BseTypeInfo         *info)
{
  const BseExportProcedure *pspec = &spec->s_proc;

  info->class_size = sizeof (BseProcedureClass);
  info->class_init = (BseClassInitFunc) bse_procedure_init;
  info->class_destroy = (BseClassDestroyFunc) pspec->unload;
  info->class_data = pspec;
}

BseProcedureClass*
bse_procedure_find_ref (const gchar *name)
{
  BseType type;

  g_return_val_if_fail (name != NULL, NULL);

  type = bse_type_from_name (name);
  if (BSE_TYPE_IS_PROCEDURE (type))
    return bse_type_class_ref (type);

  return NULL;
}

void
bse_procedure_ref (BseProcedureClass *proc)
{
  g_return_if_fail (BSE_IS_PROCEDURE_CLASS (proc));

  bse_type_class_ref (BSE_PROCEDURE_TYPE (proc));
}

void
bse_procedure_unref (BseProcedureClass *proc)
{
  g_return_if_fail (BSE_IS_PROCEDURE_CLASS (proc));

  bse_type_class_unref (proc);
}

static gboolean
proc_notifier_marshaller (GHook   *hook,
			  gpointer data_p)
{
  gpointer *data = data_p;
  BseProcedureNotify notify = hook->func;

  return notify (hook->data, data[0], GPOINTER_TO_INT (data[1]));
}

BseErrorType
bse_procedure_execvl (BseProcedureClass *proc,
		      GSList            *iparam_list,
		      GSList            *oparam_list)
{
  BseParam in_params[BSE_PROCEDURE_MAX_IN_PARAMS] = { { NULL, }, };
  BseParam out_params[BSE_PROCEDURE_MAX_OUT_PARAMS] = { { NULL, }, };
  BseErrorType error;
  GSList *slist;
  gpointer mdata[2];
  guint i;

  g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (proc), BSE_ERROR_INTERNAL);

  if (g_slist_find (called_procs, proc))
    return BSE_ERROR_PROC_BUSY;

  for (i = 0, slist = iparam_list; i < proc->n_in_params; i++, slist = slist->next)
    {
      BseParam *lparam, *param = in_params + i;

      lparam = slist ? slist->data : NULL;
      if (!lparam || lparam->pspec != proc->in_param_specs[i])
	{
	  g_warning (G_GNUC_PRETTY_FUNCTION "(): input parameter \"%s\" of procedure \"%s\" is invalid or unspecified",
		     proc->in_param_specs[i]->any.name,
		     proc->name);
	  return BSE_ERROR_INTERNAL;
	}
      memcpy (param, lparam, sizeof (*param));
    }
  for (i = 0, slist = oparam_list; i < proc->n_out_params; i++, slist = slist->next)
    {
      BseParam *lparam, *param = out_params + i;

      lparam = slist ? slist->data : NULL;
      if (!lparam || lparam->pspec != proc->out_param_specs[i])
	{
	  g_warning (G_GNUC_PRETTY_FUNCTION "(): output parameter \"%s\" of procedure \"%s\" is invalid or unspecified",
		     proc->out_param_specs[i]->any.name,
		     proc->name);
	  return BSE_ERROR_INTERNAL;
	}
      bse_param_init (param, proc->out_param_specs[i]);
    }

  bse_procedure_ref (proc);

  called_procs = g_slist_prepend (called_procs, proc);
  error = proc->execute (proc,
			 in_params,
			 out_params);
  mdata[0] = proc->name;
  mdata[1] = GINT_TO_POINTER (error);
  g_hook_list_marshal_check (&proc_notifiers,
			     FALSE,
			     proc_notifier_marshaller,
			     mdata);
  called_procs = g_slist_remove (called_procs, proc);

  for (i = 0, slist = iparam_list; i < proc->n_in_params; i++, slist = slist->next)
    {
      BseParam *lparam, *param = in_params + i;

      lparam = slist->data;
      memcpy (&lparam->value, &param->value, sizeof (param->value));
    }
  for (i = 0, slist = oparam_list; i < proc->n_out_params; i++, slist = slist->next)
    {
      BseParam *lparam, *param = out_params + i;

      lparam = slist->data;

      bse_param_values_exchange (lparam, param);
      bse_param_free_value (param);
    }

  bse_procedure_unref (proc);
  
  return error;
}

static inline BseErrorType
bse_procedure_execva_i (BseProcedureClass *proc,
			guint		   n_ovr_params,
			BseParam	  *ovr_params,
			va_list            var_args,
			gboolean           skip_oparams)
{
  BseParam in_params[BSE_PROCEDURE_MAX_IN_PARAMS] = { { NULL, }, };
  BseParam out_params[BSE_PROCEDURE_MAX_OUT_PARAMS] = { { NULL, }, };
  gpointer out_locs[BSE_PROCEDURE_MAX_OUT_PARAMS] = { NULL, };
  BseErrorType error;
  gpointer mdata[2];
  guint i;

  g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (proc), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (n_ovr_params <= proc->n_in_params, BSE_ERROR_INTERNAL);

  if (g_slist_find (called_procs, proc))
    return BSE_ERROR_PROC_BUSY;

  for (i = 0; i < n_ovr_params; i++)
    {
      BseParam *param = in_params + i;
      
      bse_param_init (param, proc->in_param_specs[i]);
      bse_param_copy_value (ovr_params + i, param);
    }
  for (i = n_ovr_params; i < proc->n_in_params; i++)
    {
      BseParam *param = in_params + i;
      gchar *error_msg;

      bse_param_init (param, proc->in_param_specs[i]);
      BSE_PARAM_COLLECT_VALUE (param, var_args, error_msg);
      if (error_msg)
	{
	  g_warning (G_GNUC_PRETTY_FUNCTION "(): %s", error_msg);
	  g_free (error_msg);
	  va_end (var_args);

	  return BSE_ERROR_PROC_PARAM_INVAL;
	}
    }
  for (i = 0; i < proc->n_out_params; i++)
    {
      BseParam *param = out_params + i;
      
      bse_param_init (param, proc->out_param_specs[i]);
      out_locs[i] = skip_oparams ? NULL : va_arg (var_args, gpointer);
    }
  
  bse_procedure_ref (proc);

  called_procs = g_slist_prepend (called_procs, proc);
  error = proc->execute (proc, in_params, out_params);
  mdata[0] = proc->name;
  mdata[1] = GINT_TO_POINTER (error);
  g_hook_list_marshal_check (&proc_notifiers,
			     FALSE,
			     proc_notifier_marshaller,
			     mdata);
  called_procs = g_slist_remove (called_procs, proc);

  for (i = 0; i < proc->n_in_params; i++)
    bse_param_free_value (in_params + i);
  for (i = 0; i < proc->n_out_params; i++)
    {
      if (!skip_oparams)
	bse_param_move_value (out_params + i, out_locs[i]);
      bse_param_free_value (out_params + i);
    }

  bse_procedure_unref (proc);
  
  return error;
}

BseErrorType
bse_procedure_exec (const gchar *name,
		    ...)
{
  BseProcedureClass *proc;
  va_list var_args;
  BseErrorType error;

  g_return_val_if_fail (name != NULL, BSE_ERROR_INTERNAL);

  proc = bse_procedure_find_ref (name);
  if (!proc)
    {
      if (bse_type_from_name (name))
	g_warning ("Cannot exec non-procedure `%s'", name);
      else
	g_warning ("Cannot exec unknown procedure \"%s\"", name);
      return BSE_ERROR_INTERNAL;
    }

  va_start (var_args, name);
  error = bse_procedure_execva_i (proc, 0, NULL, var_args, FALSE);
  va_end (var_args);

  bse_procedure_unref (proc);

  return error;
}

BseErrorType
bse_procedure_void_exec (const gchar *name,
			 ...)
{
  BseProcedureClass *proc;
  va_list var_args;
  BseErrorType error;

  g_return_val_if_fail (name != NULL, BSE_ERROR_INTERNAL);

  proc = bse_procedure_find_ref (name);
  if (!proc)
    {
      if (bse_type_from_name (name))
	g_warning ("Cannot exec non-procedure `%s'", name);
      else
	g_warning ("Cannot exec unknown procedure \"%s\"", name);
      return BSE_ERROR_INTERNAL;
    }

  va_start (var_args, name);
  error = bse_procedure_execva_i (proc, 0, NULL, var_args, TRUE);
  va_end (var_args);

  bse_procedure_unref (proc);

  return error;
}

BseErrorType
bse_procedure_execva_item (BseProcedureClass *proc,
			   BseItem           *item,
			   va_list            var_args,
			   gboolean           skip_oparams)
{
  BseParam param = { NULL, };
  BseErrorType ret_val;

  g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (proc), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (proc->n_in_params >= 1, BSE_ERROR_INTERNAL);
  g_return_val_if_fail (proc->in_param_specs[0]->type == BSE_TYPE_PARAM_ITEM,
			BSE_ERROR_INTERNAL);
  if (item)
    {
      g_return_val_if_fail (BSE_IS_ITEM (item), BSE_ERROR_INTERNAL);
      g_return_val_if_fail (bse_type_is_a (BSE_OBJECT_TYPE (item),
					   proc->in_param_specs[0]->s_item.item_type),
			    BSE_ERROR_INTERNAL);
    }

  bse_param_init (&param, proc->in_param_specs[0]);
  bse_param_set_item (&param, item);

  ret_val = bse_procedure_execva_i (proc, 1, &param, var_args, skip_oparams);

  bse_param_free_value (&param);

  return ret_val;
}

guint
bse_procedure_notifier_add (BseProcedureNotify notifier,
			    gpointer           func_data)
{
  GHook *hook;

  g_return_val_if_fail (notifier != NULL, 0);

  hook = g_hook_alloc (&proc_notifiers);
  hook->func = notifier;
  hook->data = func_data;
  g_hook_append (&proc_notifiers, hook);

  return hook->hook_id;
}

void
bse_procedure_notifier_remove (guint notifier_id)
{
  g_return_if_fail (notifier_id > 0);

  if (!g_hook_destroy (&proc_notifiers, notifier_id))
    g_warning ("Unable to remove procedure notifier (%u)", notifier_id);
}

void
bse_procedure_push_share_hook (BseProcedureShare share_func,
			       gpointer          func_data)
{
  ShareNode *node;

  g_return_if_fail (share_func != NULL);

  node = g_new (ShareNode, 1);
  node->share_func = share_func;
  node->func_data = func_data;
  g_trash_stack_push (&share_stack, node);
}

void
bse_procedure_pop_share_hook (void)
{
  g_free (g_trash_stack_pop (&share_stack));
}

gboolean
bse_procedure_share (BseProcedureClass *proc)
{
  ShareNode *node = g_trash_stack_peek (&share_stack);

  g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (proc), TRUE);

  if (!g_slist_find (called_procs, proc))
    g_warning ("Share/Update from procedure not in call (\"%s\")",
	       proc->name);

  return node && node->share_func (node->func_data, proc->name, -1);
}

gboolean
bse_procedure_update (BseProcedureClass *proc,
		      gfloat             progress)
{
  ShareNode *node = g_trash_stack_peek (&share_stack);

  g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (proc), TRUE);

  if (!g_slist_find (called_procs, proc))
    g_warning ("Share/Update from procedure not in call (\"%s\")",
	       proc->name);

  return node && node->share_func (node->func_data, proc->name, CLAMP (progress, 0, 1));
}

const gchar*
bse_procedure_type_register (const gchar *name,
			     const gchar *blurb,
			     BsePlugin   *plugin,
			     BseType     *ret_type)
{
  BseType type, base_type = 0;
  gchar *p;

  g_return_val_if_fail (ret_type != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  *ret_type = 0;
  g_return_val_if_fail (name != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  g_return_val_if_fail (plugin != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));

  type = bse_type_from_name (name);
  if (type)
    return "Procedure already registered";

  p = strchr (name, ':');
  if (p)
    {
      /* enforce <ITEM>::<NAME> syntax */
      if (p[1] != ':' || !p[2])
	return "Procedure name invalid";

      p = g_strndup (name, p - name);
      base_type = bse_type_from_name (p);
      g_free (p);
      if (!bse_type_is_a (base_type, BSE_TYPE_ITEM))
	return "Procedure base type invalid";
    }
  
  type = bse_type_register_dynamic (BSE_TYPE_PROCEDURE,
				    name,
				    blurb,
				    plugin);

  *ret_type = type;

  return NULL;
}
