// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstprocedure.hh"
#include "bstparam.hh"
#include <gobject/gvaluecollector.h>
#include <string.h>


/* --- prototypes --- */
static void     bst_procedure_shell_class_init (BstProcedureShellClass *klass);
static void     bst_procedure_shell_init       (BstProcedureShell      *pe);
static void     bst_procedure_shell_destroy    (GtkObject              *object);
static void     bst_procedure_shell_finalize   (GObject                *object);


/* --- static variables --- */
static BstProcedureShell *global_proc_shell = NULL;
static gpointer           parent_class = NULL;
static GQuark             quark_input_params = 0;
static GQuark             quark_output_params = 0;


/* --- functions --- */
GtkType
bst_procedure_shell_get_type (void)
{
  static GtkType procedure_shell_type = 0;

  if (!procedure_shell_type)
    {
      GtkTypeInfo procedure_shell_info =
      {
        (char*) "BstProcedureShell",
        sizeof (BstProcedureShell),
        sizeof (BstProcedureShellClass),
        (GtkClassInitFunc) bst_procedure_shell_class_init,
        (GtkObjectInitFunc) bst_procedure_shell_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      procedure_shell_type = gtk_type_unique (GTK_TYPE_VBOX, &procedure_shell_info);
    }

  return procedure_shell_type;
}

static void
bst_procedure_shell_class_init (BstProcedureShellClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  quark_output_params = g_quark_from_static_string ("Output Parameters");
  quark_input_params = g_quark_from_static_string ("Input Parameters");

  gobject_class->finalize = bst_procedure_shell_finalize;

  object_class->destroy = bst_procedure_shell_destroy;
}

static void
bst_procedure_shell_init (BstProcedureShell *self)
{
  self->proc = NULL;
  self->prec = sfi_rec_new ();
  self->n_preset_params = 0;
  self->params = NULL;
  self->in_modal_selection = FALSE;
  self->in_execution = FALSE;
  self->hide_dialog_on_exec = FALSE;
}

static void
bst_procedure_shell_destroy_contents (BstProcedureShell *self)
{
  gtk_container_foreach (GTK_CONTAINER (self), (GtkCallback) gtk_widget_destroy, NULL);
  self->n_preset_params = 0;
  sfi_ring_free (self->params);
  self->params = NULL;
  self->in_modal_selection = FALSE;
}

static void
bst_procedure_shell_destroy (GtkObject *object)
{
  BstProcedureShell *self = BST_PROCEDURE_SHELL (object);

  if (self->in_execution)
    Bse::warning (G_STRLOC ": destroying procedure shell during execution");

  bst_procedure_shell_set_proc (self, NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_procedure_shell_finalize (GObject *object)
{
  BstProcedureShell *self = BST_PROCEDURE_SHELL (object);

  bst_procedure_shell_set_proc (self, NULL);
  sfi_rec_unref (self->prec);
  self->prec = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget*
bst_procedure_shell_new (SfiGlueProc *proc)
{
  GtkWidget *widget;

  widget = gtk_widget_new (BST_TYPE_PROCEDURE_SHELL, NULL);
  bst_procedure_shell_set_proc (BST_PROCEDURE_SHELL (widget), proc);

  return widget;
}

void
bst_procedure_shell_set_proc (BstProcedureShell *self,
			      SfiGlueProc       *proc)
{
  assert_return (BST_IS_PROCEDURE_SHELL (self));

  if (proc != self->proc)
    {
      bst_procedure_shell_destroy_contents (self);
      if (self->proc)
	sfi_glue_proc_unref (self->proc);
      self->proc = proc;
      if (self->proc)
	sfi_glue_proc_ref (self->proc);
      bst_procedure_shell_rebuild (self);
    }
}

void
bst_procedure_shell_rebuild (BstProcedureShell *self)
{
  SfiGlueProc *proc;
  GtkWidget *param_box;
  SfiRing *ring, *pspecs = NULL;
  guint i;

  assert_return (BST_IS_PROCEDURE_SHELL (self));

  sfi_rec_clear (self->prec);
  bst_procedure_shell_destroy_contents (self);

  if (!self->proc)
    return;

  proc = self->proc;

  /* main container
   */
  param_box = GTK_WIDGET (self);
  gtk_widget_set (param_box,
		  "homogeneous", FALSE,
		  "spacing", 0,
		  "border_width", 5,
		  NULL);

  /* put procedure title */
  const gchar *string = bst_procedure_get_title (proc->name);
  if (!string)
    string = proc->name;
  gtk_box_pack_start (GTK_BOX (param_box),
                      gtk_widget_new (GTK_TYPE_LABEL,
                                      "visible", TRUE,
                                      "label", string,
                                      "xpad", 6,
                                      NULL),
                      FALSE,
                      TRUE,
                      0);

  /* put description
   */
  if (proc->help)
    {
      GtkWidget *frame = gtk_widget_new (GTK_TYPE_FRAME,
                                         "visible", TRUE,
                                         "label", _("Description"),
                                         "label_xalign", 0.0,
                                         "width_request", 1,
                                         "height_request", 120,
                                         "parent", param_box,
                                         NULL);
      g_object_new (GTK_TYPE_ALIGNMENT,
                    "visible", TRUE,
                    "parent", frame,
                    "border_width", 2,
                    "child", gxk_scroll_text_create (GXK_SCROLL_TEXT_WIDGET_LOOK, proc->help),
                    NULL);
    }

  /* parameter fields
   */
  for (i = 0; i < proc->n_params; i++)
    pspecs = sfi_ring_append (pspecs, proc->params[i]);
  if (proc->ret_param)
    pspecs = sfi_ring_append (pspecs, proc->ret_param);
  for (ring = pspecs; ring; ring = sfi_ring_walk (ring, pspecs))
    {
      GParamSpec *pspec = (GParamSpec*) ring->data;
      // gboolean is_out_param = pspec == proc->ret_param;
      GxkParam *param = bst_param_new_rec (pspec, self->prec);
      bst_param_create_gmask (param, NULL, param_box);
      self->params = sfi_ring_append (self->params, param);
    }
  sfi_ring_free (pspecs);

  /* initialize parameter values
   */
  bst_procedure_shell_reset (self);
}

void
bst_procedure_shell_update (BstProcedureShell *self)
{
  SfiRing *ring;

  assert_return (BST_IS_PROCEDURE_SHELL (self));

  for (ring = self->params; ring; ring = sfi_ring_walk (ring, self->params))
    gxk_param_update ((GxkParam*) ring->data);
}

static GxkParam*
shell_find_param (BstProcedureShell *self,
                  const gchar       *name)
{
  SfiRing *ring;
  for (ring = self->params; ring; ring = sfi_ring_walk (ring, self->params))
    if (strcmp (name, gxk_param_get_name ((GxkParam*) ring->data)) == 0)
      return (GxkParam*) ring->data;
  return NULL;
}

void
bst_procedure_shell_reset (BstProcedureShell *self)
{
  SfiGlueProc *proc;
  SfiRing *ring;

  assert_return (BST_IS_PROCEDURE_SHELL (self));

  proc = self->proc;
  sfi_rec_clear (self->prec);
  if (!self->params)
    return;
  for (ring = self->params; ring; ring = sfi_ring_walk (ring, self->params))
    {
      GxkParam *param = (GxkParam*) ring->data;
      GParamSpec *pspec = param->pspec;
      gboolean is_out_param = pspec == proc->ret_param;
      GValue value = { 0, };
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      if (is_out_param)
	g_value_reset (&value);
      else
	g_param_value_set_default (pspec, &value);
      sfi_rec_set (self->prec, pspec->name, &value);
      g_value_unset (&value);
      gxk_param_set_editable (param, !is_out_param);
      gxk_param_update (param);
    }
  self->n_preset_params = 0;

  /* update parameters from record */
  bst_procedure_shell_update (self);
}

void
bst_procedure_shell_unpreset (BstProcedureShell *self)
{
  SfiRing *ring;

  assert_return (BST_IS_PROCEDURE_SHELL (self));

  for (ring = self->params; ring; ring = sfi_ring_walk (ring, self->params))
    {
      GxkParam *param = (GxkParam*) ring->data;
      GParamSpec *pspec = param->pspec;
      gboolean is_out_param = pspec == self->proc->ret_param;
      if (!param->editable)
	{
	  GValue value = { 0, };
	  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  if (is_out_param)
	    g_value_reset (&value);
	  else
	    g_param_value_set_default (pspec, &value);
	  sfi_rec_set (self->prec, pspec->name, &value);
	  g_value_unset (&value);
	  gxk_param_set_editable (param, !is_out_param);
	  gxk_param_update (param);
	}
    }
  self->n_preset_params = 0;
}

gboolean
bst_procedure_shell_preset (BstProcedureShell *self,
			    const gchar       *name,
			    const GValue      *value,
			    gboolean           lock_preset)
{
  SfiGlueProc *proc;
  SfiRing *ring;

  assert_return (BST_IS_PROCEDURE_SHELL (self), 0);
  assert_return (self->proc != NULL, 0);
  assert_return (name != NULL, 0);
  assert_return (G_IS_VALUE (value), 0);

  proc = self->proc;

  /* ok, this is the really interesting part!
   * we try to unificate the preset parameter with the procedure's
   * input parameters. if we find a name match and type conversion
   * is sucessfull, the procedure gets invoked with a predefined
   * parameter value.
   */

  for (ring = self->params; ring; ring = sfi_ring_walk (ring, self->params))
    {
      GxkParam *param = (GxkParam*) ring->data;
      GParamSpec *pspec = param->pspec;
      gboolean is_out_param = pspec == proc->ret_param;

      if (!is_out_param && strcmp (pspec->name, name) == 0)
	{
	  if (g_value_type_transformable (G_VALUE_TYPE (value),
					  G_PARAM_SPEC_VALUE_TYPE (pspec)))
	    {
	      GValue pvalue = { 0, };
	      g_value_init (&pvalue, G_PARAM_SPEC_VALUE_TYPE (pspec));
	      g_param_value_convert (pspec, value, &pvalue, FALSE);
	      sfi_rec_set (self->prec, pspec->name, &pvalue);
	      g_value_unset (&pvalue);
	      if (lock_preset)
		{
		  self->n_preset_params += 1;
		  gxk_param_set_editable (param, FALSE);
		}
	      gxk_param_update (param);
	      return TRUE;
	    }
	  else
	    Bse::warning (G_STRLOC ": cannot convert `%s' value to `%s' for `%s' parameter `%s'",
                          g_type_name (G_VALUE_TYPE (value)),
                          g_type_name (G_PARAM_SPEC_VALUE_TYPE (param->pspec)),
                          g_type_name (G_PARAM_SPEC_TYPE (param->pspec)),
                          name);
	}
    }
  /* update parameters from record */
  bst_procedure_shell_update (self);

  return FALSE;
}

BstProcedureShell*
bst_procedure_shell_global (void)
{
  if (!global_proc_shell)
    {
      GtkWidget *dialog;

      global_proc_shell = (BstProcedureShell*) bst_procedure_shell_new (NULL);
      g_object_ref (global_proc_shell);
      gtk_object_sink (GTK_OBJECT (global_proc_shell));
      dialog = (GtkWidget*) gxk_dialog_new (NULL, NULL,
                                            GXK_DIALOG_STATUS_BAR | GXK_DIALOG_HIDE_ON_DELETE | GXK_DIALOG_MODAL, // | GXK_DIALOG_WINDOW_GROUP,
                                            _("Start Procedure"), NULL);
      gxk_dialog_set_sizes (GXK_DIALOG (dialog), -1, -1, 320, -1);

      gtk_container_add (GTK_CONTAINER (GXK_DIALOG (dialog)->vbox), GTK_WIDGET (global_proc_shell));
      gtk_widget_show (GTK_WIDGET (global_proc_shell));

      gxk_dialog_action (GXK_DIALOG (dialog), BST_STOCK_CLOSE, (void*) gxk_toplevel_delete, NULL);
    }
  return global_proc_shell;
}

GParamSpec*
bst_procedure_ref_pspec (const gchar    *procedure_name,
                         const gchar    *parameter)
{
  SfiGlueProc *proc;
  guint i;

  /* structure setup */
  proc = sfi_glue_describe_proc (procedure_name);
  if (!proc)
    return NULL;

  for (i = 0; i < proc->n_params; i++)
    if (strcmp (parameter, proc->params[i]->name) == 0)
      return g_param_spec_ref (proc->params[i]);

  return NULL;
}
