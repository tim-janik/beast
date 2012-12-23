/* GxkSimpleLabel - Small text-cutting label
 * Copyright (C) 2003 Tim Janik
 * Copyright (C) 1997-2000 the GTK+ Team and others
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * The GxkSimpleLabel code is derived from the GtkLabel implementation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Changes relative to GtkLabel:
 * - lots of code got stripped (basically for selecting text and handling events)
 * - simple-label derives from GtkWidget instead of GtkMisc
 * - mnemonic activation works on activatable parents even if group cycling
 * - setting ::pattern properly unsets ::use-underline
 * - if ::auto-cut is TRUE, overfull text is cut-off
 * - the label is visible by default
 */
#include "gxksimplelabel.hh"
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <pango/pango.h>
#include <libintl.h>

enum {
  PROP_0,
  PROP_LABEL,
  PROP_USE_UNDERLINE,
  PROP_AUTO_CUT,
  PROP_JUSTIFY,
  PROP_PATTERN,
  PROP_MNEMONIC_KEYVAL,
  PROP_MNEMONIC_WIDGET
};

/* --- prototypes --- */
static void     simple_label_class_init           (GxkSimpleLabelClass *klass);
static void     simple_label_init                 (GxkSimpleLabel      *label);
static void     simple_label_set_property         (GObject             *object,
                                                   guint                prop_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void     simple_label_get_property         (GObject             *object,
                                                   guint                prop_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);
static void     simple_label_destroy              (GtkObject           *object);
static void     simple_label_finalize             (GObject             *object);
static void     simple_label_size_request         (GtkWidget           *widget,
                                                   GtkRequisition      *requisition);
static void     simple_label_style_set            (GtkWidget           *widget,
                                                   GtkStyle            *previous_style);
static void     simple_label_direction_changed    (GtkWidget           *widget,
                                                   GtkTextDirection     previous_dir);
static gint     simple_label_expose               (GtkWidget           *widget,
                                                   GdkEventExpose      *event);
static void     simple_label_set_pattern_internal (GxkSimpleLabel      *label,
                                                   const gchar         *pattern);
static void     gxk_simple_label_recalculate      (GxkSimpleLabel      *label);
static void     simple_label_hierarchy_changed    (GtkWidget           *widget,
                                                   GtkWidget           *old_toplevel);
static void     simple_label_screen_changed       (GtkWidget           *widget,
                                                   GdkScreen           *old_screen);
static void     label_ensure_layout               (GxkSimpleLabel      *label);
static gboolean simple_label_mnemonic_activate    (GtkWidget           *widget,
                                                   gboolean             group_cycling);
static void     simple_label_setup_mnemonic       (GxkSimpleLabel      *label,
                                                   guint                last_key);

/* --- variables --- */
static gpointer parent_class = NULL;


/* --- functions --- */
GType
gxk_simple_label_get_type (void)
{
  static GType label_type = 0;
  if (!label_type)
    {
      static const GTypeInfo label_info =
        {
          sizeof (GxkSimpleLabelClass),
          NULL,           /* base_init */
          NULL,           /* base_finalize */
          (GClassInitFunc) simple_label_class_init,
          NULL,           /* class_finalize */
          NULL,           /* class_data */
          sizeof (GxkSimpleLabel),
          32,             /* n_preallocs */
          (GInstanceInitFunc) simple_label_init,
        };
      label_type = g_type_register_static (GTK_TYPE_MISC, "GxkSimpleLabel", &label_info, GTypeFlags (0));
    }
  return label_type;
}

static void
simple_label_class_init (GxkSimpleLabelClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  gobject_class->set_property = simple_label_set_property;
  gobject_class->get_property = simple_label_get_property;
  gobject_class->finalize = simple_label_finalize;
  
  object_class->destroy = simple_label_destroy;
  
  widget_class->size_request = simple_label_size_request;
  widget_class->style_set = simple_label_style_set;
  widget_class->direction_changed = simple_label_direction_changed;
  widget_class->expose_event = simple_label_expose;
  widget_class->hierarchy_changed = simple_label_hierarchy_changed;
  widget_class->screen_changed = simple_label_screen_changed;
  widget_class->mnemonic_activate = simple_label_mnemonic_activate;
  
  g_object_class_install_property (gobject_class,
                                   PROP_LABEL,
                                   g_param_spec_string ("label", _("Label"), _("The text of the label"),
                                                        NULL, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_USE_UNDERLINE,
                                   g_param_spec_boolean ("use_underline", _("Use underline"),
                                                         _("If set, an underline in the text indicates the next character should be used for the mnemonic accelerator key"),
                                                         FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_AUTO_CUT,
                                   g_param_spec_boolean ("auto-cut", _("Auto cut"),
                                                         _("If set, overfull text is cut-off and an ellipsis \"...\" is displayed instead"),
                                                         TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_JUSTIFY,
                                   g_param_spec_enum ("justify", _("Justification"),
                                                      _("The alignment of the lines in the text of the label relative to each other. This does NOT affect the alignment of the label within its allocation."),
                                                      GTK_TYPE_JUSTIFICATION, GTK_JUSTIFY_LEFT, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_PATTERN,
                                   g_param_spec_string ("pattern", _("Pattern"),
                                                        _("A string with _ characters in positions correspond to characters in the text to underline"),
                                                        NULL, G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class,
                                   PROP_MNEMONIC_KEYVAL,
                                   g_param_spec_uint ("mnemonic_keyval", _("Mnemonic key"),
                                                      _("The mnemonic accelerator key for this label"),
                                                      0, G_MAXUINT, GDK_VoidSymbol, G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
                                   PROP_MNEMONIC_WIDGET,
                                   g_param_spec_object ("mnemonic_widget", _("Mnemonic widget"),
                                                        _("The widget to be activated when the label's mnemonic "
                                                          "key is pressed"),
                                                        GTK_TYPE_WIDGET, G_PARAM_READWRITE));
}

static void
simple_label_init (GxkSimpleLabel *self)
{
  GTK_WIDGET_SET_FLAGS (self, GTK_NO_WINDOW);
  gtk_widget_show (GTK_WIDGET (self));
  
  self->jtype = GTK_JUSTIFY_LEFT;
  self->layout = NULL;
  self->text = g_strdup ("");
  self->mnemonic_widget = NULL;
  self->mnemonic_window = NULL;
  self->mnemonic_keyval = GDK_VoidSymbol;
  self->use_underline = FALSE;
  self->auto_cut = TRUE;
  self->label = g_strdup ("");
  gxk_simple_label_recalculate (self);
}

static void 
simple_label_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GxkSimpleLabel *self = GXK_SIMPLE_LABEL (object);
  switch (prop_id)
    {
      guint last_keyval, jtype;
      const gchar *cstr;
    case PROP_LABEL:
      last_keyval = self->mnemonic_keyval;
      g_free (self->label);
      self->label = g_value_dup_string (value);
      gxk_simple_label_recalculate (self);
      if (last_keyval != self->mnemonic_keyval)
        simple_label_setup_mnemonic (self, last_keyval);
      break;
    case PROP_USE_UNDERLINE:
      self->use_underline = g_value_get_boolean (value);
      gxk_simple_label_recalculate (self);
      if (self->use_underline)
        simple_label_setup_mnemonic (self, self->mnemonic_keyval);
      break;
    case PROP_AUTO_CUT:
      self->auto_cut = g_value_get_boolean (value);
      gxk_simple_label_recalculate (self);
      break;
    case PROP_JUSTIFY:
      jtype = g_value_get_enum (value);
      if (self->jtype != jtype)
        {
          self->jtype = jtype;
          gxk_simple_label_recalculate (self);
        }
      break;
    case PROP_PATTERN:
      cstr = g_value_get_string (value);
      if (cstr)
        {
          if (self->use_underline)
            g_object_notify ((GObject*) self, "use-underline");
          self->use_underline = FALSE;
        }
      simple_label_set_pattern_internal (self, cstr);
      gxk_simple_label_recalculate (self);
      break;
    case PROP_MNEMONIC_WIDGET:
      gxk_simple_label_set_mnemonic_widget (self, (GtkWidget*) g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
simple_label_get_property (GObject     *object,
                           guint        prop_id,
                           GValue      *value,
                           GParamSpec  *pspec)
{
  GxkSimpleLabel *label;
  
  label = GXK_SIMPLE_LABEL (object);
  
  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, label->label);
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, label->use_underline);
      break;
    case PROP_AUTO_CUT:
      g_value_set_boolean (value, label->auto_cut);
      break;
    case PROP_JUSTIFY:
      g_value_set_enum (value, label->jtype);
      break;
    case PROP_MNEMONIC_KEYVAL:
      g_value_set_uint (value, label->mnemonic_keyval);
      break;
    case PROP_MNEMONIC_WIDGET:
      g_value_set_object (value, (GObject*) label->mnemonic_widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
simple_label_destroy (GtkObject *object)
{
  GxkSimpleLabel *self = GXK_SIMPLE_LABEL (object);
  gxk_simple_label_set_mnemonic_widget (self, NULL);
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
simple_label_finalize (GObject *object)
{
  GxkSimpleLabel *self = GXK_SIMPLE_LABEL (object);
  if (self->layout)
    g_object_unref (self->layout);
  if (self->effective_attrs)
    pango_attr_list_unref (self->effective_attrs);
  g_free (self->label);
  g_free (self->text);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
simple_label_mnemonic_activate (GtkWidget *widget,
                                gboolean   group_cycling)
{
  GtkWidget *parent = widget->parent;
  if (GXK_SIMPLE_LABEL (widget)->mnemonic_widget)
    return gtk_widget_mnemonic_activate (GXK_SIMPLE_LABEL (widget)->mnemonic_widget, group_cycling);
  
  /* Try to find the widget to activate by traversing the
   * widget's ancestry.
   */
  if (parent && GTK_IS_NOTEBOOK (parent))
    return FALSE;
  while (parent)
    {
      if (GTK_WIDGET_CAN_FOCUS (parent) ||
          GTK_WIDGET_GET_CLASS (parent)->activate_signal ||
          (parent->parent && GTK_IS_NOTEBOOK (parent->parent)) ||
          GTK_IS_MENU_ITEM (parent))
        {
          gboolean cc = gtk_widget_mnemonic_activate (parent, group_cycling);
          g_print ("simple_label_mnemonic_activate: %s: %u (%d %d %d %d)\n", G_OBJECT_TYPE_NAME (parent), cc,
                   GTK_WIDGET_CAN_FOCUS (parent), GTK_WIDGET_GET_CLASS (parent)->activate_signal,
                   (parent->parent && GTK_IS_NOTEBOOK (parent->parent)),
                   GTK_IS_MENU_ITEM (parent));
          return cc;
        }
      parent = parent->parent;
    }
  
  /* barf if there was nothing to activate */
  g_warning ("Couldn't find a target for a mnemonic activation.");
  gdk_display_beep (gtk_widget_get_display (widget));
  return FALSE;
}

static void
simple_label_setup_mnemonic (GxkSimpleLabel *self,
                             guint           last_key)
{
  GtkWidget *toplevel;
  if (last_key != GDK_VoidSymbol && self->mnemonic_window)
    {
      gtk_window_remove_mnemonic  (self->mnemonic_window,
                                   last_key,
                                   GTK_WIDGET (self));
      self->mnemonic_window = NULL;
    }
  if (self->mnemonic_keyval == GDK_VoidSymbol)
    return;
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
  if (GTK_WIDGET_TOPLEVEL (toplevel))
    {
      gtk_window_add_mnemonic (GTK_WINDOW (toplevel),
                               self->mnemonic_keyval,
                               GTK_WIDGET (self));
      self->mnemonic_window = GTK_WINDOW (toplevel);
    }
}

static void
simple_label_hierarchy_changed (GtkWidget *widget,
                                GtkWidget *old_toplevel)
{
  GxkSimpleLabel *self = GXK_SIMPLE_LABEL (widget);
  simple_label_setup_mnemonic (self, self->mnemonic_keyval);
}

static void
simple_label_screen_changed (GtkWidget *widget,
                             GdkScreen *old_screen)
{
  gxk_simple_label_recalculate (GXK_SIMPLE_LABEL (widget));
}

static void
label_mnemonic_widget_weak_notify (gpointer      data,
                                   GObject      *where_the_object_was)
{
  GxkSimpleLabel *label = (GxkSimpleLabel*) data;
  
  label->mnemonic_widget = NULL;
  g_object_notify (G_OBJECT (label), "mnemonic_widget");
}

void
gxk_simple_label_set_mnemonic_widget (GxkSimpleLabel  *self,
                                      GtkWidget       *widget)
{
  g_return_if_fail (GXK_IS_SIMPLE_LABEL (self));
  if (widget)
    g_return_if_fail (GTK_IS_WIDGET (widget));
  
  if (self->mnemonic_widget)
    g_object_weak_unref (G_OBJECT (self->mnemonic_widget),
                         label_mnemonic_widget_weak_notify,
                         self);
  self->mnemonic_widget = widget;
  if (self->mnemonic_widget)
    g_object_weak_ref (G_OBJECT (self->mnemonic_widget),
                       label_mnemonic_widget_weak_notify,
                       self);
  g_object_notify ((GObject*) self, "mnemonic_widget");
}

static PangoAttrList*
simple_label_pattern_to_attrs (GxkSimpleLabel *self,
                               const gchar    *pattern)
{
  const char *start;
  const char *p = self->text;
  const char *q = pattern;
  PangoAttrList *attrs;
  
  attrs = pango_attr_list_new ();
  
  while (1)
    {
      while (*p && *q && *q != '_')
        {
          p = g_utf8_next_char (p);
          q++;
        }
      start = p;
      while (*p && *q && *q == '_')
        {
          p = g_utf8_next_char (p);
          q++;
        }
      
      if (p > start)
        {
          PangoAttribute *attr = pango_attr_underline_new (PANGO_UNDERLINE_LOW);
          attr->start_index = start - self->text;
          attr->end_index = p - self->text;
          
          pango_attr_list_insert (attrs, attr);
        }
      else
        break;
    }
  
  return attrs;
}

static void
simple_label_set_pattern_internal (GxkSimpleLabel *self,
                                   const gchar    *pattern)
{
  PangoAttrList *attrs = simple_label_pattern_to_attrs (self, pattern);
  if (self->effective_attrs)
    pango_attr_list_unref (self->effective_attrs);
  self->effective_attrs = attrs;
}

static void
simple_label_set_uline_text_internal (GxkSimpleLabel *self,
                                      const gchar    *str)
{
  guint accel_key = GDK_VoidSymbol;
  gchar *new_str = g_new (gchar, strlen (str) + 1);
  gchar *dest = new_str;
  gchar *pattern = g_new (gchar, g_utf8_strlen (str, -1) + 1);
  gchar *pattern_dest = pattern;
  const gchar *src = str;
  gboolean underscore = FALSE;
  
  /* Split text into the base text and a separate pattern
   * of underscores.
   */
  while (*src)
    {
      gchar *next_src = g_utf8_next_char (src);
      gunichar c = g_utf8_get_char (src);
      if (c == (gunichar) -1)
        {
          g_warning ("Invalid input string");
          g_free (new_str);
          g_free (pattern);
          return;
        }
      if (underscore)
        {
          if (c == '_')
            *pattern_dest++ = ' ';
          else
            {
              *pattern_dest++ = '_';
              if (accel_key == GDK_VoidSymbol)
                accel_key = gdk_keyval_to_lower (gdk_unicode_to_keyval (c));
            }
          while (src < next_src)
            *dest++ = *src++;
          underscore = FALSE;
        }
      else
        {
          if (c == '_')
            {
              underscore = TRUE;
              src = next_src;
            }
          else
            {
              while (src < next_src)
                *dest++ = *src++;
              *pattern_dest++ = ' ';
            }
        }
    }
  *dest = 0;
  *pattern_dest = 0;
  
  g_free (self->text);
  self->text = new_str;
  simple_label_set_pattern_internal (self, pattern);
  g_free (pattern);
  
  self->mnemonic_keyval = accel_key;
}

static void
gxk_simple_label_recalculate (GxkSimpleLabel *self)
{
  if (self->use_underline)
    simple_label_set_uline_text_internal (self, self->label ? self->label : "");
  else
    {
      g_free (self->text);
      self->text = g_strdup (self->label ? self->label : "");
      if (self->effective_attrs)
        pango_attr_list_unref (self->effective_attrs);
      self->effective_attrs = NULL;
    }
  if (!self->use_underline)
    {
      guint keyval = self->mnemonic_keyval;
      self->mnemonic_keyval = GDK_VoidSymbol;
      simple_label_setup_mnemonic (self, keyval);
    }
  if (self->layout)
    {
      g_object_unref (self->layout);
      self->layout = NULL;
    }
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
label_ensure_layout (GxkSimpleLabel *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  if (!self->layout)
    {
      PangoAlignment align = PANGO_ALIGN_LEFT;
      self->layout = gtk_widget_create_pango_layout (widget, NULL);
      if (self->effective_attrs)
        pango_layout_set_attributes (self->layout, self->effective_attrs);
      pango_layout_set_width (self->layout, -1);
      switch (self->jtype)
        {
        case GTK_JUSTIFY_LEFT:    align = PANGO_ALIGN_LEFT;       break;
        case GTK_JUSTIFY_RIGHT:   align = PANGO_ALIGN_RIGHT;      break;
        case GTK_JUSTIFY_CENTER:  align = PANGO_ALIGN_CENTER;     break;
        case GTK_JUSTIFY_FILL:
          /* this is broken, but GtkLabel doesn't do any better */
          pango_layout_set_justify (self->layout, TRUE);
          // pango_layout_set_width (self->layout, label_allocation_width (self));
          break;
        }
      pango_layout_set_alignment (self->layout, align);
    }
  pango_layout_set_text (self->layout, self->text, -1);
  self->needs_cutting = self->auto_cut;
}

static inline gint
label_allocation_width (GxkSimpleLabel *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkMisc *misc = GTK_MISC (self);
  return MAX (widget->allocation.width - 2 * misc->xpad, 1);
}

static inline gint
label_allocation_height (GxkSimpleLabel *self)
{
  GtkWidget *widget = GTK_WIDGET (self);
  GtkMisc *misc = GTK_MISC (self);
  return MAX (widget->allocation.height - 2 * misc->ypad, 1);
}

static void
label_cut_layout (GxkSimpleLabel *self)
{
  /* TRANSLATORS: this is the string that is appended to the end
   * of a clipped label if it does not fit its assigned width.
   */
  const gchar *cliptext = _("...");
  PangoRectangle logical_rect;
  gint l2 = strlen (cliptext);
  gint l1 = strlen (self->text);
  gint i, l = g_utf8_strlen (self->text, -1);
  if (!l)
    return;
  pango_layout_get_extents (self->layout, NULL, &logical_rect);
  for (i = l; i >= 0 && PANGO_PIXELS (logical_rect.width) > label_allocation_width (self); i--)
    {
      gchar *p, *sspace = g_new0 (char, l1 + l2 + 1);
      g_utf8_strncpy (sspace, self->text, i);
      p = sspace;
      while (*p)
        p++;
      g_utf8_strncpy (p, cliptext, -1);
      pango_layout_set_text (self->layout, sspace, -1);
      g_free (sspace);
      pango_layout_get_extents (self->layout, NULL, &logical_rect);
    }
}

static void
simple_label_size_request (GtkWidget      *widget,
                           GtkRequisition *requisition)
{
  GxkSimpleLabel *self = GXK_SIMPLE_LABEL (widget);
  PangoRectangle logical_rect;
  
  if (self->layout)
    {
      g_object_unref (self->layout);
      self->layout = NULL;
    }
  label_ensure_layout (self);
  pango_layout_get_extents (self->layout, NULL, &logical_rect);
  requisition->width = PANGO_PIXELS (logical_rect.width) + 2 * GTK_MISC (self)->xpad;
  requisition->height = PANGO_PIXELS (logical_rect.height) + 2 * GTK_MISC (self)->ypad;
}

static void 
simple_label_style_set (GtkWidget *widget,
                        GtkStyle  *previous_style)
{
  GxkSimpleLabel *self = GXK_SIMPLE_LABEL (widget);
  gxk_simple_label_recalculate (self);
}

static void 
simple_label_direction_changed (GtkWidget        *widget,
                                GtkTextDirection previous_dir)
{
  GxkSimpleLabel *self = GXK_SIMPLE_LABEL (widget);
  if (self->layout)
    pango_layout_context_changed (self->layout);
  GTK_WIDGET_CLASS (parent_class)->direction_changed (widget, previous_dir);
}

static void
get_layout_location (GxkSimpleLabel *self,
                     gint           *xp,
                     gint           *yp)
{
  GtkWidget *widget = GTK_WIDGET (self);
  gint x, y;
  GtkMisc *misc = GTK_MISC (self);
  gint xpad = MIN ((widget->allocation.width - 1) / 2, misc->xpad);
  gint ypad = MIN ((widget->allocation.height - 1) / 2, misc->ypad);
  PangoRectangle logical_rect;
  pango_layout_get_extents (self->layout, NULL, &logical_rect);
  gint rwidth = MIN (widget->requisition.width, PANGO_PIXELS (logical_rect.width));
  gint rheight = MIN (widget->requisition.height, PANGO_PIXELS (logical_rect.height));
  
  x = widget->allocation.x + xpad + GTK_MISC (self)->xalign * MAX (label_allocation_width (self) - rwidth, 0);
  y = widget->allocation.y + ypad + GTK_MISC (self)->yalign * MAX (label_allocation_height (self) - rheight, 0);
  *xp = x;
  *yp = y;
}

static gint
simple_label_expose (GtkWidget      *widget,
                     GdkEventExpose *event)
{
  GxkSimpleLabel *self = GXK_SIMPLE_LABEL (widget);
  
  label_ensure_layout (self);
  if (self->needs_cutting)
    {
      self->needs_cutting = FALSE;
      label_cut_layout (self);
    }
  if (self->text[0])
    {
      gint x, y;
      get_layout_location (self, &x, &y);
      gtk_paint_layout (widget->style,
                        widget->window,
                        GtkStateType (GTK_WIDGET_STATE (widget)),
                        FALSE,
                        &event->area,
                        widget,
                        "label",
                        x, y,
                        self->layout);
    }
  return FALSE;
}
