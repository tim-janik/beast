// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bseprocedure.hh"
#include "bsemain.hh"
#include <gobject/gvaluecollector.h>
#include "bseobject.hh"
#include "bseserver.hh"
#include "bsestorage.hh"
#include "bseexports.hh"
#include <string.h>

#define PDEBUG(...)     BSE_KEY_DEBUG ("procs", __VA_ARGS__)
#define CHECK_DEBUG()   Bse::bse_debug_enabled ("procs")
#define HACK_DEBUG /* very slow and leaks memory */ while (0) g_printerr

/* --- macros --- */
#define parse_or_return         bse_storage_scanner_parse_or_return
#define peek_or_return          bse_storage_scanner_peek_or_return

/* --- prototypes --- */
static void     bse_procedure_base_init           (BseProcedureClass        *proc);
static void     bse_procedure_base_finalize       (BseProcedureClass        *proc);
static void     bse_procedure_init                (BseProcedureClass        *proc,
                                                   const BseExportNodeProc  *pnode);
static void     procedure_class_unref             (BseProcedureClass        *proc);
/* --- functions --- */
static void
bse_procedure_base_init (BseProcedureClass *proc)
{
  proc->private_id = 0;
  proc->n_in_pspecs = 0;
  proc->in_pspecs = NULL;
  proc->n_out_pspecs = 0;
  proc->out_pspecs = NULL;
  proc->cache_stamp = 0;
  proc->execute = NULL;
}

static void
bse_procedure_base_finalize (BseProcedureClass *proc)
{
  guint i;

  /* give up type references */
  for (i = 0; proc->class_refs[i]; i++)
    g_type_class_unref (proc->class_refs[i]);
  g_free (proc->class_refs);
  proc->class_refs = NULL;

  for (i = 0; i < proc->n_in_pspecs; i++)
    g_param_spec_unref (proc->in_pspecs[i]);
  g_free (proc->in_pspecs);
  for (i = 0; i < proc->n_out_pspecs; i++)
    g_param_spec_unref (proc->out_pspecs[i]);
  g_free (proc->out_pspecs);

  proc->execute = NULL;
}

static void
bse_procedure_init (BseProcedureClass       *proc,
                    const BseExportNodeProc *pnode)
{
  GParamSpec *in_pspecs[BSE_PROCEDURE_MAX_IN_PARAMS + 8];
  GParamSpec *out_pspecs[BSE_PROCEDURE_MAX_OUT_PARAMS + 8];
  guint i, j;

  memset (in_pspecs, 0, sizeof (in_pspecs));
  memset (out_pspecs, 0, sizeof (out_pspecs));

  proc->private_id = pnode->private_id;

  /* init procedure class from plugin,
   * paranoia check certain class members
   */
  pnode->init (proc, in_pspecs, out_pspecs);
  if (proc->n_in_pspecs || proc->in_pspecs ||
      proc->n_out_pspecs || proc->out_pspecs ||
      proc->execute)
    {
      proc->n_in_pspecs = 0;
      proc->in_pspecs = NULL;
      proc->n_out_pspecs = 0;
      proc->out_pspecs = NULL;
      proc->execute = NULL;
      g_warning ("procedure \"%s\" messes with reserved class members", BSE_PROCEDURE_NAME (proc));
    }

  /* check input parameters and setup specifications */
  for (i = 0; i < BSE_PROCEDURE_MAX_IN_PARAMS; i++)
    if (in_pspecs[i])
      {
        if ((in_pspecs[i]->flags & G_PARAM_READWRITE) != G_PARAM_READWRITE)
          g_warning ("procedure \"%s\": input parameter \"%s\" has invalid flags",
                     BSE_PROCEDURE_NAME (proc), in_pspecs[i]->name);
        g_param_spec_ref (in_pspecs[i]);
        g_param_spec_sink (in_pspecs[i]);
      }
    else
      break;
  if (i == BSE_PROCEDURE_MAX_IN_PARAMS && in_pspecs[i])
    g_warning ("procedure \"%s\" exceeds maximum number of input parameters (%u)",
               BSE_PROCEDURE_NAME (proc), BSE_PROCEDURE_MAX_IN_PARAMS);
  proc->n_in_pspecs = i;
  proc->in_pspecs = g_new (GParamSpec*, proc->n_in_pspecs + 1);
  memcpy (proc->in_pspecs, in_pspecs, sizeof (in_pspecs[0]) * proc->n_in_pspecs);
  proc->in_pspecs[proc->n_in_pspecs] = NULL;

  /* check output parameters and setup specifications */
  for (i = 0; i < BSE_PROCEDURE_MAX_OUT_PARAMS; i++)
    if (out_pspecs[i])
      {
        if ((out_pspecs[i]->flags & G_PARAM_READWRITE) != G_PARAM_READWRITE)
          g_warning ("procedure \"%s\": output parameter \"%s\" has invalid flags",
                     BSE_PROCEDURE_NAME (proc), out_pspecs[i]->name);
        g_param_spec_ref (out_pspecs[i]);
        g_param_spec_sink (out_pspecs[i]);
      }
    else
      break;
  if (i == BSE_PROCEDURE_MAX_OUT_PARAMS && out_pspecs[i])
    g_warning ("procedure \"%s\" exceeds maximum number of output parameters (%u)",
               BSE_PROCEDURE_NAME (proc), BSE_PROCEDURE_MAX_OUT_PARAMS);
  proc->n_out_pspecs = i;
  proc->out_pspecs = g_new (GParamSpec*, proc->n_out_pspecs + 1);
  memcpy (proc->out_pspecs, out_pspecs, sizeof (out_pspecs[0]) * proc->n_out_pspecs);
  proc->out_pspecs[proc->n_out_pspecs] = NULL;

  /* keep type references */
  proc->class_refs = g_new (GTypeClass*, proc->n_in_pspecs + proc->n_out_pspecs + 1);
  j = 0;
  for (i = 0; i < proc->n_in_pspecs; i++)
    if (G_TYPE_IS_CLASSED ((G_PARAM_SPEC_VALUE_TYPE (proc->in_pspecs[i]))))
      proc->class_refs[j++] = (GTypeClass*) g_type_class_ref (G_PARAM_SPEC_VALUE_TYPE (proc->in_pspecs[i]));
  for (i = 0; i < proc->n_out_pspecs; i++)
    if (G_TYPE_IS_CLASSED ((G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[i]))))
      proc->class_refs[j++] = (GTypeClass*) g_type_class_ref (G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[i]));
  proc->class_refs[j++] = NULL;

  /* hookup execute method */
  proc->execute = pnode->exec;
}

void
bse_procedure_complete_info (const BseExportNodeProc *pnode,
                             GTypeInfo               *info)
{
  info->class_size = sizeof (BseProcedureClass);
  info->class_init = (GClassInitFunc) bse_procedure_init;
  info->class_finalize = (GClassFinalizeFunc) NULL;
  info->class_data = pnode;
}

const gchar*
bse_procedure_type_register (const gchar *name,
                             BsePlugin   *plugin,
                             GType       *ret_type)
{
  GType base_type = 0;
  g_return_val_if_fail (ret_type != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  *ret_type = 0;
  g_return_val_if_fail (name != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  g_return_val_if_fail (plugin != NULL, bse_error_blurb (BSE_ERROR_INTERNAL));
  GType type = g_type_from_name (name);
  if (type)
    return "Procedure already registered";
  const char *pn = strchr (name, '+');
  if (pn)
    {
      /* enforce <OBJECT>+<METHOD> syntax */
      if (!pn[1])
        return "Procedure name invalid";
      char *p = g_strndup (name, pn - name);
      base_type = g_type_from_name (p);
      g_free (p);
      if (!g_type_is_a (base_type, BSE_TYPE_OBJECT))
        return "Procedure base type invalid";
    }

  type = bse_type_register_dynamic (BSE_TYPE_PROCEDURE, name, G_TYPE_PLUGIN (plugin));

  *ret_type = type;

  return NULL;
}

GType
bse_procedure_lookup (const gchar *proc_name)
{
  GType type;

  g_return_val_if_fail (proc_name != NULL, 0);

  type = g_type_from_name (proc_name);
  return BSE_TYPE_IS_PROCEDURE (type) ? type : 0;
}

static void
signal_exec_status (BseErrorType       error,
                    BseProcedureClass *proc,
                    GValue            *first_ovalue)
{
#if 0
  /* signal script status, supporting BseErrorType-outparam procedures
   */
  if (!error && proc->n_out_pspecs == 1 &&
      g_type_is_a (G_VALUE_TYPE (first_ovalue), BSE_TYPE_ERROR_TYPE))
    {
      BseErrorType verror = g_value_get_enum (first_ovalue);

      bse_server_exec_status (bse_server_get (), BSE_EXEC_STATUS_DONE, BSE_PROCEDURE_NAME (proc), verror ? 0 : 1, verror);
    }
  else
    bse_server_exec_status (bse_server_get (), BSE_EXEC_STATUS_DONE, BSE_PROCEDURE_NAME (proc), error ? 0 : 1, error);
#endif
}

static BseErrorType
bse_procedure_call (BseProcedureClass  *proc,
                    GValue             *ivalues,
                    GValue             *ovalues,
                    BseProcedureMarshal marshal,
                    gpointer            marshal_data)
{
  guint i, bail_out = FALSE;
  BseErrorType error;

  for (i = 0; i < proc->n_in_pspecs; i++)
    {
      GParamSpec *pspec = proc->in_pspecs[i];

      if (g_param_value_validate (pspec, ivalues + i) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
        {
          g_warning ("%s: input arg `%s' contains invalid value",
                     BSE_PROCEDURE_NAME (proc),
                     pspec->name);
          bail_out = TRUE;
        }
    }

  if (bail_out)
    error = BSE_ERROR_PROC_PARAM_INVAL;
  else
    {
      if (CHECK_DEBUG())
        {
          if (proc->n_in_pspecs && G_TYPE_IS_OBJECT (G_PARAM_SPEC_VALUE_TYPE (proc->in_pspecs[0])))
            PDEBUG ("executing procedure \"%s\" on object %s",
                    BSE_PROCEDURE_NAME (proc), bse_object_debug_name (g_value_get_object (ivalues + 0)));
          else
            PDEBUG ("executing procedure \"%s\"", BSE_PROCEDURE_NAME (proc));
        }
      if (marshal)
        error = marshal (marshal_data, proc, ivalues, ovalues);
      else
        error = proc->execute (proc, ivalues, ovalues);
    }

  for (i = 0; i < proc->n_out_pspecs; i++)
    {
      GParamSpec *pspec = proc->out_pspecs[i];

      if (g_param_value_validate (pspec, ovalues + i) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
        g_warning ("%s: internal procedure error: output arg `%s' had invalid value",
                   BSE_PROCEDURE_NAME (proc),
                   pspec->name);
    }

  return error;
}

BseErrorType
bse_procedure_marshal (GType               proc_type,
                       const GValue       *ivalues,
                       GValue             *ovalues,
                       BseProcedureMarshal marshal,
                       gpointer            marshal_data)
{
  GValue tmp_ivalues[BSE_PROCEDURE_MAX_IN_PARAMS], tmp_ovalues[BSE_PROCEDURE_MAX_OUT_PARAMS];
  uint i, bail_out = FALSE;
  BseErrorType error;
  g_return_val_if_fail (BSE_TYPE_IS_PROCEDURE (proc_type), BSE_ERROR_INTERNAL);
  BseProcedureClass *proc = (BseProcedureClass*) g_type_class_ref (proc_type);
  for (i = 0; i < proc->n_in_pspecs; i++)
    {
      GParamSpec *pspec = proc->in_pspecs[i];
      tmp_ivalues[i].g_type = 0;
      g_value_init (tmp_ivalues + i, G_PARAM_SPEC_VALUE_TYPE (pspec));
      if (!sfi_value_transform (ivalues + i, tmp_ivalues + i))
        {
          g_warning ("%s: input arg `%s' has invalid type `%s' (expected `%s')",
                     BSE_PROCEDURE_NAME (proc),
                     pspec->name,
                     G_VALUE_TYPE_NAME (ivalues + i),
                     g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
          bail_out = TRUE;
        }
    }
  for (i = 0; i < proc->n_out_pspecs; i++)
    {
      tmp_ovalues[i].g_type = 0;
      g_value_init (tmp_ovalues + i, G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[i]));
    }

  if (bail_out)
    error = BSE_ERROR_PROC_PARAM_INVAL;
  else
    error = bse_procedure_call (proc, tmp_ivalues, tmp_ovalues, marshal, marshal_data);
  signal_exec_status (error, proc, tmp_ovalues);

  for (i = 0; i < proc->n_in_pspecs; i++)
    g_value_unset (tmp_ivalues + i);
  for (i = 0; i < proc->n_out_pspecs; i++)
    {
      GParamSpec *pspec = proc->out_pspecs[i];

      if (!sfi_value_transform (tmp_ovalues + i, ovalues + i))
        g_warning ("%s: output arg `%s' of type `%s' cannot be converted into `%s'",
                   BSE_PROCEDURE_NAME (proc),
                   pspec->name,
                   g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
                   G_VALUE_TYPE_NAME (ovalues + i));
      g_value_unset (tmp_ovalues + i);
    }
  procedure_class_unref (proc);

  return error;
}

static inline BseErrorType
bse_procedure_call_collect (BseProcedureClass  *proc,
                            const GValue       *first_value,
                            BseProcedureMarshal marshal,
                            gpointer            marshal_data,
                            gboolean            skip_call,
                            gboolean            skip_ovalues,
                            GValue             *ivalues,
                            GValue             *ovalues,
                            va_list             var_args)
{
  guint i, bail_out = FALSE;
  BseErrorType error = BSE_ERROR_NONE;
  PDEBUG ("call %s: ", BSE_PROCEDURE_NAME (proc));
  /* collect first arg */
  if (first_value && first_value != ivalues) /* may skip this since bse_procedure_call() does extra validation */
    {
      if (proc->n_in_pspecs < 1)
        g_warning ("%s: input arg supplied for procedure taking `void'",
                   BSE_PROCEDURE_NAME (proc));
      else
        {
          GParamSpec *pspec = proc->in_pspecs[0];
          ivalues[0].g_type = 0;
          g_value_init (ivalues + 0, G_PARAM_SPEC_VALUE_TYPE (pspec));
          if (!sfi_value_transform (first_value, ivalues + 0))
            {
              g_warning ("%s: input arg `%s' has invalid type `%s' (expected `%s')",
                         BSE_PROCEDURE_NAME (proc),
                         pspec->name,
                         G_VALUE_TYPE_NAME (first_value),
                         g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
              bail_out = TRUE;
            }
        }
    }
  if (first_value)
    HACK_DEBUG ("  arg[%u]<%s>: %s", 0, g_type_name (ivalues[0].g_type), g_strdup_value_contents (ivalues) /* memleak */);
  /* collect remaining args */
  for (i = first_value ? 1 : 0; i < proc->n_in_pspecs; i++)
    {
      GParamSpec *pspec = proc->in_pspecs[i];
      gchar *error_msg = NULL;

      ivalues[i].g_type = 0;
      g_value_init (ivalues + i, G_PARAM_SPEC_VALUE_TYPE (pspec));
      if (!bail_out)
        G_VALUE_COLLECT (ivalues + i, var_args, 0, &error_msg);
      if (error_msg)
        {
          g_warning ("%s: failed to collect arg `%s' of type `%s': %s",
                     BSE_PROCEDURE_NAME (proc),
                     pspec->name,
                     G_VALUE_TYPE_NAME (ivalues + i),
                     error_msg);
          g_free (error_msg);
          bail_out = TRUE;
        }
      HACK_DEBUG ("  arg[%u]<%s>: %s", i, g_type_name (ivalues[i].g_type), g_strdup_value_contents (ivalues + i) /* memleak */);
    }

  if (!skip_call)
    {
      /* initialize return values */
      for (i = 0; i < proc->n_out_pspecs; i++)
        {
          ovalues[i].g_type = 0;
          g_value_init (ovalues + i, G_PARAM_SPEC_VALUE_TYPE (proc->out_pspecs[i]));
        }

      /* execute procedure */
      if (bail_out)
        error = BSE_ERROR_PROC_PARAM_INVAL;
      else
        error = bse_procedure_call (proc, ivalues, ovalues, marshal, marshal_data);
      PDEBUG ("  call result: %s", bse_error_blurb (error));
      signal_exec_status (error, proc, ovalues);

      /* free input arguments */
      for (i = 0; i < proc->n_in_pspecs; i++)
        g_value_unset (ivalues + i);

      /* copy return values into locations */
      for (i = 0; i < proc->n_out_pspecs; i++)
        {
          GParamSpec *pspec = proc->out_pspecs[i];
          gchar *error_msg = NULL;

          if (!skip_ovalues)
            G_VALUE_LCOPY (ovalues + i, var_args, 0, &error_msg);
          if (error_msg)
            {
              g_warning ("%s: failed to return arg `%s' of type `%s': %s",
                         BSE_PROCEDURE_NAME (proc),
                         pspec->name,
                         G_VALUE_TYPE_NAME (ovalues + i),
                         error_msg);
              g_free (error_msg);
              skip_ovalues = TRUE;
            }
          g_value_unset (ovalues + i);
        }
    }
  else
    PDEBUG ("  call skipped");
  return error;
}
/**
 * @param proc_type	a type derived from BSE_TYPE_PROCEDURE
 * @param first_value	the first input argument if not to be collected
 * @param marshal	function marshalling the procedure call or NULL
 * @param marshal_data	data passed in to @a marshal
 * @param skip_ovalues	whether return value locations should be collected and filled in
 * @param var_args	va_list to collect input args from
 * @return		BseErrorType value of error if any occoured
 *
 * Collect input arguments for a procedure call from a va_list and
 * call the procedure, optionally via @a marshal. If @a skip_ovalues is
 * FALSE, the procedure return values will be stored in return
 * value locations also collected from @a var_args.
 */
BseErrorType
bse_procedure_marshal_valist (GType               proc_type,
                              const GValue       *first_value,
                              BseProcedureMarshal marshal,
                              gpointer            marshal_data,
                              gboolean            skip_ovalues,
                              va_list             var_args)
{
  g_return_val_if_fail (BSE_TYPE_IS_PROCEDURE (proc_type), BSE_ERROR_INTERNAL);
  GValue tmp_ivalues[BSE_PROCEDURE_MAX_IN_PARAMS], tmp_ovalues[BSE_PROCEDURE_MAX_OUT_PARAMS];
  BseProcedureClass *proc = (BseProcedureClass*) g_type_class_ref (proc_type);
  BseErrorType error = bse_procedure_call_collect (proc, first_value, marshal, marshal_data,
                                                   FALSE, skip_ovalues, tmp_ivalues, tmp_ovalues, var_args);
  procedure_class_unref (proc);
  return error;
}

/**
 * @param proc	        valid BseProcedureClass
 * @param first_value	the first input argument if not to be collected
 * @param var_args	va_list to collect input args from
 * @param ivalues	uninitialized GValue array with at least proc->n_in_pspecs members
 * @return		BseErrorType value of error if any occoured during collection
 *
 * Collect input arguments for a procedure call from a va_list. The first
 * value may be supplied as @a first_value and will then not be collected.
 * @a ivalues must be at least @a proc->n_in_pspecs elements long and all elements
 * will be initialized after the function returns (even in error cases).
 * @a first_value may be the same adress as @a ivalues, in whic hcase the first
 * argument is entirely ignored and collection simply starts out with the
 * second argument.
 */
BseErrorType
bse_procedure_collect_input_args (BseProcedureClass  *proc,
                                  const GValue       *first_value,
                                  va_list             var_args,
                                  GValue              ivalues[BSE_PROCEDURE_MAX_IN_PARAMS])
{
  BseErrorType error;
  g_return_val_if_fail (BSE_IS_PROCEDURE_CLASS (proc), BSE_ERROR_INTERNAL);

  /* add an extra reference count to the class */
  proc = (BseProcedureClass*) g_type_class_ref (BSE_PROCEDURE_TYPE (proc));
  error = bse_procedure_call_collect (proc, first_value, NULL, NULL,
                                      TRUE, TRUE, ivalues, NULL, var_args);
  /* so the class can enter the cache here */
  procedure_class_unref (proc);
  return error;
}

BseErrorType
bse_procedure_exec (const gchar *proc_name,
                    ...)
{
  GType proc_type;

  g_return_val_if_fail (proc_name != NULL, BSE_ERROR_INTERNAL);

  proc_type = bse_procedure_lookup (proc_name);
  if (!proc_type)
    {
      g_warning ("%s: no such procedure", proc_name);
      return BSE_ERROR_PROC_NOT_FOUND;
    }
  else
    {
      BseErrorType error;
      va_list var_args;

      va_start (var_args, proc_name);
      error = bse_procedure_marshal_valist (proc_type, NULL, NULL, NULL, FALSE, var_args);
      va_end (var_args);
      return error;
    }
}

BseErrorType
bse_procedure_exec_void (const gchar *proc_name,
                         ...)
{
  GType proc_type;

  g_return_val_if_fail (proc_name != NULL, BSE_ERROR_INTERNAL);

  proc_type = bse_procedure_lookup (proc_name);
  if (!proc_type)
    {
      g_warning ("%s: no such procedure", proc_name);
      return BSE_ERROR_PROC_NOT_FOUND;
    }
  else
    {
      BseErrorType error;
      va_list var_args;

      va_start (var_args, proc_name);
      error = bse_procedure_marshal_valist (proc_type, NULL, NULL, NULL, TRUE, var_args);
      va_end (var_args);
      return error;
    }
}

BseErrorType
bse_procedure_execvl (BseProcedureClass  *proc,
                      GSList             *in_value_list,
                      GSList             *out_value_list,
                      BseProcedureMarshal marshal,
                      gpointer            marshal_data)
{
  GValue tmp_ivalues[BSE_PROCEDURE_MAX_IN_PARAMS];
  GValue tmp_ovalues[BSE_PROCEDURE_MAX_OUT_PARAMS];
  BseErrorType error;
  GSList *slist;
  guint i;

  /* FIXME: bad, bad compat: bse_procedure_execvl() */

  for (i = 0, slist = in_value_list; slist && i < proc->n_in_pspecs; i++, slist = slist->next)
    memcpy (tmp_ivalues + i, slist->data, sizeof (tmp_ivalues[0]));
  if (slist || i != proc->n_in_pspecs)
    {
      g_warning ("%s: invalid number of arguments supplied to procedure \"%s\"", G_STRLOC, BSE_PROCEDURE_NAME (proc));
      return BSE_ERROR_PROC_PARAM_INVAL;
    }
  for (i = 0, slist = out_value_list; slist && i < proc->n_out_pspecs; i++, slist = slist->next)
    memcpy (tmp_ovalues + i, slist->data, sizeof (tmp_ovalues[0]));
  if (slist || i != proc->n_out_pspecs)
    {
      g_warning ("%s: invalid number of arguments supplied to procedure \"%s\"", G_STRLOC, BSE_PROCEDURE_NAME (proc));
      return BSE_ERROR_PROC_PARAM_INVAL;
    }
  error = bse_procedure_marshal (BSE_PROCEDURE_TYPE (proc), tmp_ivalues, tmp_ovalues, marshal, marshal_data);
  for (i = 0, slist = out_value_list; slist && i < proc->n_out_pspecs; i++, slist = slist->next)
    memcpy (slist->data, tmp_ovalues + i, sizeof (tmp_ivalues[0]));
  return error;
}

static BseProcedureClass *proc_cache = NULL;
static guint64            cache_time = 0;

static gboolean
proc_cache_prepare (GSource *source,
                    gint    *timeout_p)
{
  gboolean need_dispatch = FALSE;
  if (proc_cache)
    {
      const guint delay_msecs = 500;
      GTimeVal current_time;
      guint64 stime;
      g_source_get_current_time (source, &current_time);
      BSE_THREADS_ENTER ();
      stime = current_time.tv_sec * 1000 + current_time.tv_usec / 1000; /* milliseconds */
      if (stime >= cache_time + delay_msecs)
        need_dispatch = TRUE;
      else
        {
          if (stime < cache_time)       /* handle time warp */
            cache_time = stime;
          if (timeout_p)
            *timeout_p = delay_msecs - (stime - cache_time);
        }
      BSE_THREADS_LEAVE ();
    }
  return need_dispatch;
}

static gboolean
proc_cache_check (GSource *source)
{
  return proc_cache_prepare (source, NULL);
}

static gboolean
proc_cache_dispatch (GSource    *source,
                     GSourceFunc callback,
                     gpointer    user_data)
{
  BseProcedureClass *ulist = NULL, *proc, *last = NULL;
  GTimeVal current_time;

  BSE_THREADS_ENTER ();
  proc = proc_cache;
  while (proc)
    {
      BseProcedureClass *next = (BseProcedureClass*) proc->cache_next;
      if (proc->cache_stamp < 2)        /* purging of old procs */
        {
          /* unlink */
          if (last)
            last->cache_next = next;
          else
            proc_cache = next;
          /* enter free list */
          proc->cache_next = ulist;
          ulist = proc;
          proc->cache_stamp = 0;
        }
      else
        {
          proc->cache_stamp = 1;        /* aging of recent procs */
          last = proc;
        }
      proc = next;
    }
  while (ulist)
    {
      proc = ulist;
      ulist = (BseProcedureClass*) proc->cache_next;
      proc->cache_next = NULL;
      // g_printerr ("release-procedure: %s\n", BSE_PROCEDURE_NAME (proc));
      g_type_class_unref (proc);
    }
  g_source_get_current_time (source, &current_time);
  cache_time = current_time.tv_sec * 1000 + current_time.tv_usec / 1000; /* milliseconds */
  BSE_THREADS_LEAVE ();
  return TRUE;
}

static void
procedure_class_unref (BseProcedureClass *proc)
{
  /* we cache procedure class creation here, to avoid recreating
   * procedure classes over and over per-invocation
   */
  if (!proc->cache_stamp)
    {
      // g_printerr ("cache-procedure: %s\n", BSE_PROCEDURE_NAME (proc));
      g_assert (proc->cache_next == NULL);
      proc->cache_stamp = 2;        /* 'recent' stamp */
      proc->cache_next = proc_cache;
      proc_cache = proc;
    }
  else  /* cached already */
    {
      proc->cache_stamp = 2;        /* 'recent' stamp */
      g_type_class_unref (proc);
    }
}

void
bse_type_register_procedure_info (GTypeInfo *info)
{
  static const GTypeInfo proc_info = {
    sizeof (BseProcedureClass),

    (GBaseInitFunc) bse_procedure_base_init,
    (GBaseFinalizeFunc) bse_procedure_base_finalize,
    (GClassInitFunc) NULL,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,

    /* non classed type stuff */
    0, 0, NULL,
  };
  static GSourceFuncs proc_cache_source_funcs = {
    proc_cache_prepare,
    proc_cache_check,
    proc_cache_dispatch,
    NULL
  };
  GSource *source = g_source_new (&proc_cache_source_funcs, sizeof (*source));
  g_source_set_priority (source, BSE_PRIORITY_BACKGROUND);
  g_source_attach (source, bse_main_context);

  *info = proc_info;
}
