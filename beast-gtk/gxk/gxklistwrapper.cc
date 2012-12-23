// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "gxklistwrapper.hh"
#include "gxkmarshal.h"
#include <gtk/gtkmain.h>


#define	I2P(x)		GINT_TO_POINTER (x)
#define	P2I(x)		GPOINTER_TO_INT (x)
#define	_(x)		(x)

enum {
  PROP_0,
  PROP_N_COLS,
  PROP_COLUMN_TYPES,
  PROP_N_ROWS
};


/* --- prototypes --- */
static void	gxk_list_wrapper_class_init		(GxkListWrapperClass	*klass);
static void	gxk_list_wrapper_init			(GxkListWrapper		*self);
static void	gxk_list_wrapper_finalize		(GObject		*object);
static void	gxk_list_wrapper_set_property		(GObject		*object,
							 guint                   param_id,
							 const GValue           *value,
							 GParamSpec             *pspec);
static void	gxk_list_wrapper_get_property		(GObject		*object,
							 guint                   param_id,
							 GValue                 *value,
							 GParamSpec             *pspec);
static void	gxk_list_wrapper_init_tree_model_iface	(GtkTreeModelIface	*iface);


/* --- static variables --- */
static gpointer parent_class = NULL;
static guint    signal_fill_value = 0;
static guint    signal_row_change = 0;


/* --- functions --- */
GType
gxk_list_wrapper_get_type (void)
{
  static GType type = 0;
  
  if (!type)
    {
      static const GTypeInfo type_info = {
	sizeof (GxkListWrapperClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gxk_list_wrapper_class_init,
	NULL,   /* class_finalize */
	NULL,   /* class_data */
	sizeof (GxkListWrapper),
	0,      /* n_preallocs */
	(GInstanceInitFunc) gxk_list_wrapper_init,
      };
      static const GInterfaceInfo iface_info = {
	(GInterfaceInitFunc) gxk_list_wrapper_init_tree_model_iface,
	(GInterfaceFinalizeFunc) NULL,
	NULL,	/* interface_data */
      };
      
      type = g_type_register_static (G_TYPE_OBJECT, "GxkListWrapper", &type_info, GTypeFlags (0));
      g_type_add_interface_static (type, GTK_TYPE_TREE_MODEL, &iface_info);
    }
  return type;
}

static void
gxk_list_wrapper_class_init (GxkListWrapperClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class->set_property = gxk_list_wrapper_set_property;
  object_class->get_property = gxk_list_wrapper_get_property;
  object_class->finalize = gxk_list_wrapper_finalize;
  
  g_object_class_install_property (object_class,
				   PROP_N_COLS,
				   g_param_spec_uint ("n_cols", _("Number of Columns"), NULL,
						      1, G_MAXINT, 1, GParamFlags (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
  g_object_class_install_property (object_class,
				   PROP_COLUMN_TYPES,
				   g_param_spec_pointer ("column_types", _("Array of column types"), NULL,
							 GParamFlags (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
  g_object_class_install_property (object_class,
				   PROP_N_ROWS,
				   g_param_spec_uint ("n_rows", _("Number of Rows"), NULL,
						      0, G_MAXINT, 0, GParamFlags (G_PARAM_READWRITE)));
  
  signal_fill_value = g_signal_new ("fill-value",
				    G_OBJECT_CLASS_TYPE (object_class),
				    G_SIGNAL_RUN_LAST,
				    G_STRUCT_OFFSET (GxkListWrapperClass, fill_value),
				    NULL, NULL,
				    gxk_marshal_NONE__UINT_UINT_BOXED,
				    G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_UINT,
				    G_TYPE_VALUE | G_SIGNAL_TYPE_STATIC_SCOPE);
  signal_row_change = g_signal_new ("row-change",
				    G_OBJECT_CLASS_TYPE (object_class),
				    G_SIGNAL_RUN_LAST,
				    G_STRUCT_OFFSET (GxkListWrapperClass, row_change),
				    NULL, NULL,
				    gxk_marshal_NONE__INT,
				    G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
gxk_list_wrapper_init (GxkListWrapper *self)
{
  self->n_rows = 0;
  self->n_cols = 0;
  self->column_types = NULL;
  self->stamp = 1;
}

static void
gxk_list_wrapper_set_property (GObject      *object,
			       guint         param_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (object);
  guint i;

  switch (param_id)
    {
      GType *ctypes;
    case PROP_N_COLS:
      self->n_cols = g_value_get_uint (value);
      break;
    case PROP_COLUMN_TYPES:
      self->column_types = g_new0 (GType, self->n_cols);
      ctypes = (GType*) g_value_get_pointer (value);
      g_return_if_fail (ctypes != NULL);
      for (i = 0; i < self->n_cols; i++)
	self->column_types[i] = ctypes[i];
      for (i = 0; i < self->n_cols; i++)
	G_TYPE_IS_VALUE_TYPE (self->column_types[i]);
      break;
    case PROP_N_ROWS:
      gxk_list_wrapper_notify_clear (self);
      gxk_list_wrapper_notify_prepend (self, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
gxk_list_wrapper_get_property (GObject    *object,
			       guint       param_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (object);

  switch (param_id)
    {
    case PROP_N_COLS:
      g_value_set_uint (value, self->n_cols);
      break;
    case PROP_COLUMN_TYPES:
      g_value_set_pointer (value, self->column_types);
      break;
    case PROP_N_ROWS:
      g_value_set_uint (value, self->n_rows);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, param_id, pspec);
      break;
    }
}

static void
gxk_list_wrapper_finalize (GObject *object)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (object);

  g_free (self->column_types);

  /* chain parent class' handler */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GxkListWrapper*
gxk_list_wrapper_newv (guint  n_cols,
		       GType *column_types)
{
  GxkListWrapper *self;

  g_return_val_if_fail (n_cols > 0, NULL);
  g_return_val_if_fail (column_types != NULL, NULL);

  self = (GxkListWrapper*) g_object_new (GXK_TYPE_LIST_WRAPPER,
                                         "n_cols", n_cols,
                                         "column_types", column_types,
                                         NULL);

  return self;
}

GxkListWrapper*
gxk_list_wrapper_new (guint  n_cols,
		      GType  first_column_type,
		      ...)
{
  GxkListWrapper *self;
  GType *ctypes;
  va_list var_args;
  guint i;

  g_return_val_if_fail (n_cols > 0, NULL);

  ctypes = g_new (GType, n_cols);

  ctypes[0] = first_column_type;
  va_start (var_args, first_column_type);
  for (i = 1; i < n_cols; i++)
    ctypes[i] = va_arg (var_args, GType);
  va_end (var_args);

  self = (GxkListWrapper*) g_object_new (GXK_TYPE_LIST_WRAPPER,
                                         "n_cols", n_cols,
                                         "column_types", ctypes,
                                         NULL);
  g_free (ctypes);

  return self;
}

static GtkTreeModelFlags
gxk_list_wrapper_get_flags (GtkTreeModel *tree_model)
{
  return GtkTreeModelFlags (0);
}

static gint
gxk_list_wrapper_get_n_columns (GtkTreeModel *tree_model)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (tree_model);

  return self->n_cols;
}

static GType
gxk_list_wrapper_get_column_type (GtkTreeModel *tree_model,
				  gint          index)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (tree_model);

  g_return_val_if_fail (index >= 0 && index < int (self->n_cols), G_TYPE_INVALID);

  return self->column_types[index];
}

static gboolean
gxk_list_wrapper_get_iter (GtkTreeModel *tree_model,
			   GtkTreeIter  *iter,
			   GtkTreePath  *path)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (tree_model);
  gint *ind;

  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

  ind = gtk_tree_path_get_indices (path);
  if (ind[0] < 0 || ind[0] >= int (self->n_rows))
    return FALSE;		/* only happens on stamp aliasing and empty lists */

  iter->stamp = self->stamp;
  iter->user_data = I2P (ind[0]);

  return TRUE;
}

static GtkTreePath*
gxk_list_wrapper_get_path (GtkTreeModel *tree_model,
			   GtkTreeIter  *iter)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (tree_model);
  GtkTreePath *path;

  g_return_val_if_fail (iter->stamp == int (self->stamp), NULL);

  uint i = P2I (iter->user_data);
  if (i < 0 || i >= self->n_rows)
    return FALSE;		/* only happens on stamp aliasing */

  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, i);

  return path;
}

static void
gxk_list_wrapper_get_value (GtkTreeModel *tree_model,
			    GtkTreeIter  *iter,
			    gint          column,
			    GValue       *value)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (tree_model);

  g_return_if_fail (iter->stamp == int (self->stamp));
  g_return_if_fail (column >= 0 && uint (column) < self->n_cols);

  g_value_init (value, self->column_types[column]);

  uint i = P2I (iter->user_data);
  if (i < 0 || i >= self->n_rows)
    return;			/* only happens on stamp aliasing */

  g_signal_emit (tree_model, signal_fill_value, 0, column, i, value);
}

static gboolean
gxk_list_wrapper_iter_next (GtkTreeModel *tree_model,
			    GtkTreeIter  *iter)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (tree_model);

  g_return_val_if_fail (iter->stamp == int (self->stamp), FALSE);

  uint i = P2I (iter->user_data);
  if (i < 0 || i >= self->n_rows)
    return FALSE;		/* only happens on stamp aliasing */

  i++;
  iter->user_data = I2P(i);

  return i < self->n_rows;
}

static gboolean
gxk_list_wrapper_iter_has_child (GtkTreeModel *tree_model,
				 GtkTreeIter  *iter)
{
  return FALSE;
}

static gint
gxk_list_wrapper_iter_n_children (GtkTreeModel *tree_model,
				  GtkTreeIter  *iter)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (tree_model);

  if (!iter)	/* root node */
    return self->n_rows;
  else
    return 0;
}

static gboolean
gxk_list_wrapper_iter_nth_child (GtkTreeModel *tree_model,
				 GtkTreeIter  *iter,
				 GtkTreeIter  *parent,
				 gint          n)
{
  GxkListWrapper *self = GXK_LIST_WRAPPER (tree_model);

  if (parent)	/* nodes of a list don't have a parent */
    return FALSE;

  g_return_val_if_fail (n >= 0 && uint (n) < self->n_rows, FALSE);

  iter->stamp = self->stamp;
  iter->user_data = I2P (n);
  return TRUE;
}

static void
gxk_list_wrapper_init_tree_model_iface (GtkTreeModelIface *iface)
{
  iface->get_flags = gxk_list_wrapper_get_flags;
  iface->get_n_columns = gxk_list_wrapper_get_n_columns;
  iface->get_column_type = gxk_list_wrapper_get_column_type;
  iface->get_iter = gxk_list_wrapper_get_iter;
  iface->get_path = gxk_list_wrapper_get_path;
  iface->get_value = gxk_list_wrapper_get_value;
  iface->iter_next = gxk_list_wrapper_iter_next;
  iface->iter_children = (gboolean (*) (GtkTreeModel*, GtkTreeIter*, GtkTreeIter*)) gtk_false;
  iface->iter_parent = (gboolean (*) (GtkTreeModel*, GtkTreeIter*, GtkTreeIter*)) gtk_false;
  iface->iter_has_child = gxk_list_wrapper_iter_has_child;
  iface->iter_n_children = gxk_list_wrapper_iter_n_children;
  iface->iter_nth_child = gxk_list_wrapper_iter_nth_child;
  
}

void
gxk_list_wrapper_notify_insert (GxkListWrapper *self,
				guint           nth_row)
{
  GtkTreeModel *tree_model;
  GtkTreeIter iter;
  GtkTreePath *path;

  g_return_if_fail (GXK_IS_LIST_WRAPPER (self));
  g_return_if_fail (nth_row <= self->n_rows);

  g_signal_emit (self, signal_row_change, 0, -1);
  tree_model = GTK_TREE_MODEL (self);

  self->n_rows++;
  self->stamp++;
  iter.stamp = self->stamp;
  iter.user_data = I2P (nth_row);

  path = gtk_tree_model_get_path (tree_model, &iter);
  gtk_tree_model_row_inserted (tree_model, path, &iter);
  gtk_tree_path_free (path);

  g_object_notify (G_OBJECT (self), "n_rows");
}

void
gxk_list_wrapper_notify_change (GxkListWrapper *self,
				guint           nth_row)
{
  GtkTreeModel *tree_model;
  GtkTreeIter iter;
  GtkTreePath *path;

  g_return_if_fail (GXK_IS_LIST_WRAPPER (self));
  g_return_if_fail (nth_row < self->n_rows);

  g_signal_emit (self, signal_row_change, 0, nth_row);
  tree_model = GTK_TREE_MODEL (self);

  iter.stamp = self->stamp;
  iter.user_data = I2P (nth_row);

  path = gtk_tree_model_get_path (tree_model, &iter);
  gtk_tree_model_row_changed (tree_model, path, &iter);
  gtk_tree_path_free (path);
}

void
gxk_list_wrapper_notify_delete (GxkListWrapper *self,
				guint           nth_row)
{
  GtkTreeModel *tree_model;
  GtkTreeIter iter;
  GtkTreePath *path;

  g_return_if_fail (GXK_IS_LIST_WRAPPER (self));
  g_return_if_fail (nth_row < self->n_rows);

  g_signal_emit (self, signal_row_change, 0, -1);
  tree_model = GTK_TREE_MODEL (self);

  iter.stamp = self->stamp;
  iter.user_data = I2P (nth_row);
  path = gtk_tree_model_get_path (tree_model, &iter);
  self->n_rows--;
  self->stamp++;
  gtk_tree_model_row_deleted (tree_model, path);
  gtk_tree_path_free (path);
  g_object_notify (G_OBJECT (self), "n_rows");
}

void
gxk_list_wrapper_notify_prepend (GxkListWrapper *self,
				 guint           n_rows)
{
  g_return_if_fail (GXK_IS_LIST_WRAPPER (self));

  g_object_freeze_notify (G_OBJECT (self));
  g_signal_emit (self, signal_row_change, 0, -1);
  while (n_rows--)
    gxk_list_wrapper_notify_insert (self, 0);
  g_object_thaw_notify (G_OBJECT (self));
}

void
gxk_list_wrapper_notify_append (GxkListWrapper *self,
				guint           n_rows)
{
  g_return_if_fail (GXK_IS_LIST_WRAPPER (self));
  
  g_object_freeze_notify (G_OBJECT (self));
  g_signal_emit (self, signal_row_change, 0, -1);
  while (n_rows--)
    gxk_list_wrapper_notify_insert (self, self->n_rows);
  g_object_thaw_notify (G_OBJECT (self));
}

void
gxk_list_wrapper_notify_clear (GxkListWrapper *self)
{
  g_return_if_fail (GXK_IS_LIST_WRAPPER (self));

  g_object_freeze_notify (G_OBJECT (self));
  g_signal_emit (self, signal_row_change, 0, -1);
  while (self->n_rows)
    gxk_list_wrapper_notify_delete (self, self->n_rows - 1);
  g_object_thaw_notify (G_OBJECT (self));
}

void
gxk_list_wrapper_get_iter_at (GxkListWrapper *self,
			      GtkTreeIter    *iter,
			      guint           index)
{
  g_return_if_fail (GXK_IS_LIST_WRAPPER (self));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (index < self->n_rows);

  iter->stamp = self->stamp;
  iter->user_data = I2P (index);
}

guint
gxk_list_wrapper_get_index (GxkListWrapper *self,
			    GtkTreeIter    *iter)
{
  g_return_val_if_fail (GXK_IS_LIST_WRAPPER (self), G_MAXUINT);
  g_return_val_if_fail (iter != NULL, G_MAXUINT);
  g_return_val_if_fail (iter->stamp == int (self->stamp), G_MAXUINT);

  return P2I (iter->user_data);
}
