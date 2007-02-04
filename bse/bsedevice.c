/* BSE - Bedevilled Sound Engine
 * Copyright (C) 2004 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "bsedevice.h"
#include <sfi/gbsearcharray.h>

/* --- functions --- */
static void
bse_device_init (BseDevice *self)
{
  BSE_OBJECT_UNSET_FLAGS (self, (BSE_DEVICE_FLAG_OPEN |
				 BSE_DEVICE_FLAG_READABLE |
				 BSE_DEVICE_FLAG_WRITABLE));
}

SfiRing*
bse_device_list (BseDevice    *self)
{
  g_return_val_if_fail (BSE_IS_DEVICE (self), NULL);
  SfiRing *ring = NULL;
  if (BSE_DEVICE_GET_CLASS (self)->list_devices)
    ring = BSE_DEVICE_GET_CLASS (self)->list_devices (self);
  if (!ring)
    ring = sfi_ring_append (ring, bse_device_error_new (self, g_strdup_printf ("Driver not implemented")));
  return ring;
}

static gchar**
device_split_args (const gchar *arg_string,
                   guint       *n)
{
  *n = 0;
  if (!arg_string || !arg_string[0])
    return NULL;
  gchar **strv = g_strsplit (arg_string, ",", -1);
  while (strv[*n])
    *n += 1;
  if (!*n)
    {
      g_strfreev (strv);
      strv = NULL;
    }
  return strv;
}

static BseErrorType
device_open_args (BseDevice      *self,
                  gboolean        need_readable,
                  gboolean        need_writable,
                  const gchar    *arg_string)
{
  BseErrorType error;
  guint n;
  gchar **args = device_split_args (arg_string, &n);
  error = BSE_DEVICE_GET_CLASS (self)->open (self,
                                             need_readable != FALSE,
                                             need_writable != FALSE,
                                             n, (const gchar**) args);
  g_strfreev (args);
  
  if (!error)
    {
      g_return_val_if_fail (BSE_DEVICE_OPEN (self), BSE_ERROR_INTERNAL);
      g_return_val_if_fail (self->open_device_name != NULL, BSE_ERROR_INTERNAL); /* bse_device_set_opened() was not called */
      if (!self->open_device_args)
        self->open_device_args = g_strdup (arg_string);
      if (BSE_DEVICE_GET_CLASS (self)->post_open)
        BSE_DEVICE_GET_CLASS (self)->post_open (self);
    }
  else
    g_return_val_if_fail (!BSE_DEVICE_OPEN (self), BSE_ERROR_INTERNAL);

  if (!error && ((need_readable && !BSE_DEVICE_READABLE (self)) ||
                 (need_writable && !BSE_DEVICE_WRITABLE (self))))
    {
      bse_device_close (self);
      error = BSE_ERROR_DEVICE_NOT_AVAILABLE;
    }

  return error;
}

BseErrorType
bse_device_open (BseDevice      *self,
                 gboolean        need_readable,
                 gboolean        need_writable,
                 const gchar    *arg_string)
{
  g_return_val_if_fail (BSE_IS_DEVICE (self), BSE_ERROR_INTERNAL);
  g_return_val_if_fail (!BSE_DEVICE_OPEN (self), BSE_ERROR_INTERNAL);
  BseErrorType error = BSE_ERROR_DEVICE_NOT_AVAILABLE;
  if (arg_string)
    error = device_open_args (self, need_readable, need_writable, arg_string);
  else
    {
      SfiRing *node, *entries = bse_device_list (self);
      for (node = entries; node; node = sfi_ring_walk (node, entries))
        {
          BseDeviceEntry *entry = node->data;
          if (!entry->device_error)
            {
              error = device_open_args (self, need_readable, need_writable, entry->device_args);
              if (!error)
                break;
            }
        }
      bse_device_entry_list_free (entries);
    }
  return error;
}

void
bse_device_set_opened (BseDevice      *self,
                       const gchar    *device_name,
                       gboolean        readable,
                       gboolean        writable)
{
  g_return_if_fail (BSE_IS_DEVICE (self));
  g_return_if_fail (!BSE_DEVICE_OPEN (self));
  g_return_if_fail (device_name != NULL);
  g_return_if_fail (readable || writable);
  self->open_device_name = g_strdup (device_name);
  BSE_OBJECT_SET_FLAGS (self, BSE_DEVICE_FLAG_OPEN);
  if (readable)
    BSE_OBJECT_SET_FLAGS (self, BSE_DEVICE_FLAG_READABLE);
  if (writable)
    BSE_OBJECT_SET_FLAGS (self, BSE_DEVICE_FLAG_WRITABLE);
  g_free (self->open_device_args);
  self->open_device_args = NULL;
}

void
bse_device_close (BseDevice *self)
{
  g_return_if_fail (BSE_IS_DEVICE (self));
  g_return_if_fail (BSE_DEVICE_OPEN (self));

  if (BSE_DEVICE_GET_CLASS (self)->pre_close)
    BSE_DEVICE_GET_CLASS (self)->pre_close (self);

  BSE_DEVICE_GET_CLASS (self)->close (self);

  g_free (self->open_device_name);
  self->open_device_name = NULL;
  g_free (self->open_device_args);
  self->open_device_args = NULL;

  BSE_OBJECT_UNSET_FLAGS (self, (BSE_DEVICE_FLAG_OPEN |
                                 BSE_DEVICE_FLAG_READABLE |
                                 BSE_DEVICE_FLAG_WRITABLE));
}

BseDeviceEntry*
bse_device_entry_new (BseDevice      *device,
                      gchar          *orphan_args,
                      gchar          *orphan_blurb)
{
  return bse_device_group_entry_new (device, orphan_args, NULL, orphan_blurb);
}

BseDeviceEntry*
bse_device_group_entry_new (BseDevice      *device,
                            gchar          *orphan_args,
                            gchar          *orphan_group,
                            gchar          *orphan_blurb)
{
  BseDeviceEntry *entry = g_new0 (BseDeviceEntry, 1);
  entry->device = g_object_ref (device);
  entry->device_args = g_strdup (orphan_args);
  entry->device_blurb = g_strdup (orphan_blurb);
  entry->device_group = g_strdup (orphan_group);
  g_free (orphan_args);
  g_free (orphan_blurb);
  g_free (orphan_group);
  return entry;
}

BseDeviceEntry*
bse_device_error_new (BseDevice      *device,
                      gchar          *orphan_error)
{
  BseDeviceEntry *entry = g_new0 (BseDeviceEntry, 1);
  entry->device = g_object_ref (device);
  entry->device_error = g_strdup (orphan_error);
  g_free (orphan_error);
  return entry;
}

void
bse_device_entry_free (BseDeviceEntry *entry)
{
  if (entry->device)
    g_object_unref (entry->device);
  g_free (entry->device_args);
  g_free (entry->device_blurb);
  g_free (entry->device_group);
  g_free (entry->device_error);
  g_free (entry);
}

void
bse_device_entry_list_free (SfiRing *ring)
{
  while (ring)
    bse_device_entry_free (sfi_ring_pop_head (&ring));
}

static SfiRing*
device_class_list_entries (GType    type,
                           void   (*request_callback) (BseDevice *device,
                                                       gpointer   data),
                           gpointer data)
{
  SfiRing *ring = NULL;
  BseDeviceClass *class = g_type_class_ref (type);
  if (BSE_DEVICE_CLASS (class)->driver_name)
    {
      BseDevice *device = g_object_new (type, NULL);
      if (request_callback)
        request_callback (device, data);
      ring = bse_device_list (device);
      g_object_unref (device);
    }
  guint n, i;
  GType *children = g_type_children (type, &n);
  for (i = 0; i < n; i++)
    ring = sfi_ring_concat (ring, device_class_list_entries (children[i], request_callback, data));
  g_free (children);
  return ring;
}

static gint
device_entry_prio_cmp  (gconstpointer   value1,
                        gconstpointer   value2,
                        gpointer        data)
{
  const BseDeviceEntry *e1 = value1;
  const BseDeviceEntry *e2 = value2;
  return -G_BSEARCH_ARRAY_CMP (BSE_DEVICE_GET_CLASS (e1->device)->driver_rating,
                               BSE_DEVICE_GET_CLASS (e2->device)->driver_rating);
}

SfiRing*
bse_device_class_list (GType    type,
                       void   (*request_callback) (BseDevice *device,
                                                   gpointer   data),
                       gpointer data)
{
  SfiRing *ring = device_class_list_entries (type, request_callback, data);
  ring = sfi_ring_sort (ring, device_entry_prio_cmp, NULL);
  return ring;
}

void
bse_device_class_setup (gpointer        klass,
                        gint            rating,
                        const gchar    *name,
                        const gchar    *syntax,
                        const gchar    *blurb)
{
  g_return_if_fail (BSE_IS_DEVICE_CLASS (klass));
  BseDeviceClass *class = BSE_DEVICE_CLASS (klass);
  class->driver_rating = rating;
  class->driver_name = name;
  class->driver_syntax = syntax;
  class->driver_blurb = blurb;
}

void
bse_device_dump_list (GType           base_type,
                      const gchar    *indent,
                      gboolean        with_auto,
                      void          (*request_callback) (BseDevice *device,
                                                         gpointer   data),
                      gpointer        data)
{
  SfiRing *node, *ring = bse_device_class_list (base_type, NULL, NULL);
  gchar *indent2 = g_strconcat (indent ? indent : "", "  ", NULL);
  BseDeviceClass *last_klass = NULL;
  const gchar *last_topic = NULL;
  for (node = ring; node; node = sfi_ring_walk (node, ring))
    {
      BseDeviceEntry *entry = node->data;
      BseDeviceClass *klass = BSE_DEVICE_GET_CLASS (entry->device);
      if (klass != last_klass)
        {
          if (klass->driver_syntax)
            g_printerr ("%s%s %s=%s\n", indent, klass->driver_name, klass->driver_name, klass->driver_syntax);
          else
            g_printerr ("%s%s\n", indent, klass->driver_name);
          if (klass->driver_blurb)
            {
              GString *gstring = g_string_new (klass->driver_blurb);
              while (gstring->len && gstring->str[gstring->len - 1] == '\n')
                gstring->str[--gstring->len] = 0;
              g_string_prefix_lines (gstring, indent2);
              g_printerr ("%s\n", gstring->str);
              g_string_free (gstring, TRUE);
            }
          last_klass = klass;
          last_topic = NULL;
        }
      if (entry->device_error)
        {
          g_printerr ("%sError: %s\n", indent2, entry->device_error);
          last_topic = NULL;
        }
      else if (entry->device_blurb)
        {
          const gchar *topic = entry->device_group ? entry->device_group : "";
          if (!last_topic || strcmp (last_topic, topic) != 0)
            {
              if (topic[0])
                g_printerr ("%sDevices (%s):\n", indent2, topic);
              else
                g_printerr ("%sDevices:\n", indent2);
              last_topic = topic;
            }
          g_printerr ("%s >        %s\n", indent, entry->device_blurb);
        }
    }
  if (with_auto)
    {
      g_printerr ("%sauto\n", indent);
      GString *gstring = g_string_new (/* TRANSLATORS: keep this text to 70 chars in width */
                                       _("Auto is a special driver, it acts as a placeholder for\n"
                                         "automatic driver selection."));
      while (gstring->len && gstring->str[gstring->len - 1] == '\n')
        gstring->str[--gstring->len] = 0;
      g_string_prefix_lines (gstring, indent2);
      g_printerr ("%s\n", gstring->str);
      g_string_free (gstring, TRUE);
    }
  bse_device_entry_list_free (ring);
  g_free (indent2);
}

static SfiRing*
device_classes_list (GType type,
                     gint  min_rating)
{
  BseDeviceClass *class = g_type_class_ref (type);
  SfiRing *ring = NULL;
  if (class->driver_name && class->driver_rating >= min_rating)
    ring = sfi_ring_append (ring, class);
  else
    g_type_class_unref (class);
  guint n, i;
  GType *children = g_type_children (type, &n);
  for (i = 0; i < n; i++)
    ring = sfi_ring_concat (ring, device_classes_list (children[i], min_rating));
  g_free (children);
  return ring;
}

static void
device_classes_free (SfiRing *ring)
{
  while (ring)
    g_type_class_unref (sfi_ring_pop_head (&ring));
}

static gint
device_classes_prio_cmp  (gconstpointer   value1,
                          gconstpointer   value2,
                          gpointer        data)
{
  const BseDeviceClass *c1 = value1;
  const BseDeviceClass *c2 = value2;
  return -G_BSEARCH_ARRAY_CMP (c1->driver_rating, c2->driver_rating);
}

BseDevice*
bse_device_open_auto (GType           base_type,
                      gboolean        need_readable,
                      gboolean        need_writable,
                      void          (*request_callback) (BseDevice *device,
                                                         gpointer   data),
                      gpointer        data,
                      BseErrorType   *errorp)
{
  if (errorp)
    *errorp = BSE_ERROR_DEVICE_NOT_AVAILABLE;
  BseDevice *device = NULL;
  SfiRing *ring, *class_list = device_classes_list (base_type, 0);
  class_list = sfi_ring_sort (class_list, device_classes_prio_cmp, NULL);
  for (ring = class_list; ring; ring = sfi_ring_walk (ring, class_list))
    {
      BseDeviceClass *class = BSE_DEVICE_CLASS (ring->data);
      device = g_object_new (G_OBJECT_CLASS_TYPE (class), NULL);
      if (request_callback)
        request_callback (device, data);
      BseErrorType error = bse_device_open (device, need_readable, need_writable, NULL);
      if (errorp)
        *errorp = error;
      if (BSE_DEVICE_OPEN (device))
        break;
      g_object_unref (device);
      device = NULL;
    }
  device_classes_free (class_list);
  return device;
}

static SfiRing*
auto_ring (void)
{
  static const gchar *devstring = "auto";
  static SfiRing ring;
  ring.data = (gchar*) devstring;
  ring.prev = &ring;
  ring.next = &ring;
  return &ring;
}

BseDevice*
bse_device_open_best (GType           base_type,
                      gboolean        need_readable,
                      gboolean        need_writable,
                      SfiRing        *devices,
                      void          (*request_callback) (BseDevice *device,
                                                         gpointer   data),
                      gpointer        data,
                      BseErrorType   *errorp)
{
  if (errorp)
    *errorp = BSE_ERROR_DEVICE_NOT_AVAILABLE;
  if (!devices)
    devices = auto_ring();
  BseDevice *device = NULL;
  SfiRing *ring, *node, *class_list = device_classes_list (base_type, G_MININT);
  gboolean seen_auto = FALSE;
  for (ring = devices; ring; ring = sfi_ring_walk (ring, devices))
    {
      const gchar *driverconf = ring->data;
      const gchar *args = strchr (driverconf, '=');
      gchar *driver = g_strndup (driverconf, args ? args - driverconf : strlen (driverconf));
      if (strcmp (driver, "auto") == 0)
        {
          if (!seen_auto)       /* collapse multiple 'auto's */
            device = bse_device_open_auto (base_type, need_readable, need_writable, request_callback, data, errorp);
          seen_auto = TRUE;
          g_free (driver);
          if (device)
            break;
          continue;
        }
      for (node = class_list; node; node = sfi_ring_walk (node, class_list))
        if (strcmp (BSE_DEVICE_CLASS (node->data)->driver_name, driver) == 0)   /* find named driver */
          break;
      g_free (driver);
      if (node)
        {
          BseDeviceClass *class = node->data;
          device = g_object_new (G_OBJECT_CLASS_TYPE (class), NULL);
          if (request_callback)
            request_callback (device, data);
          BseErrorType error = bse_device_open (device, need_readable, need_writable, args ? args + 1 : NULL);
          if (errorp)
            *errorp = error;
          if (!error)
            break;
          g_object_unref (device);
          device = NULL;
        }
      else
        sfi_diag ("%s: ignoring unknown driver specification: %s", g_type_name (base_type), driverconf);
    }
  device_classes_free (class_list);
  return device;
}

static void
bse_device_class_init (BseDeviceClass *class)
{
  // GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  class->driver_rating = -1;
  class->list_devices = NULL;
}

BSE_BUILTIN_TYPE (BseDevice)
{
  static const GTypeInfo device_info = {
    sizeof (BseDeviceClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_device_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseDevice),
    0 /* n_preallocs */,
    (GInstanceInitFunc) bse_device_init,
  };
  
  g_assert (BSE_DEVICE_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT);
  
  return bse_type_register_abstract (BSE_TYPE_OBJECT,
                                     "BseDevice",
                                     "Abstract device base type",
                                     __FILE__, __LINE__,
                                     &device_info);
}
