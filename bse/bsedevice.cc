// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bsedevice.hh"
#include <sfi/gbsearcharray.hh>

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
  assert_return (BSE_IS_DEVICE (self), NULL);
  SfiRing *ring = NULL;
  if (BSE_DEVICE_GET_CLASS (self)->list_devices)
    ring = BSE_DEVICE_GET_CLASS (self)->list_devices (self);
  if (!ring)
    ring = sfi_ring_append (ring, bse_device_error_new (self, g_strdup_format ("Driver not implemented")));
  return ring;
}

static char**
device_split_args (const char   *arg_string,
                   uint         *n)
{
  *n = 0;
  if (!arg_string || !arg_string[0])
    return NULL;
  char **strv = g_strsplit (arg_string, ",", -1);
  while (strv[*n])
    *n += 1;
  if (!*n)
    {
      g_strfreev (strv);
      strv = NULL;
    }
  return strv;
}

static Bse::Error
device_open_args (BseDevice      *self,
                  gboolean        need_readable,
                  gboolean        need_writable,
                  const char     *arg_string)
{
  Bse::Error error;
  uint n;
  char **args = device_split_args (arg_string, &n);
  error = BSE_DEVICE_GET_CLASS (self)->open (self,
                                             need_readable != FALSE,
                                             need_writable != FALSE,
                                             n, (const char**) args);
  g_strfreev (args);

  if (error == 0)
    {
      assert_return (BSE_DEVICE_OPEN (self), Bse::Error::INTERNAL);
      assert_return (self->open_device_name != NULL, Bse::Error::INTERNAL); /* bse_device_set_opened() was not called */
      if (!self->open_device_args)
        self->open_device_args = g_strdup (arg_string);
      if (BSE_DEVICE_GET_CLASS (self)->post_open)
        BSE_DEVICE_GET_CLASS (self)->post_open (self);
    }
  else
    assert_return (!BSE_DEVICE_OPEN (self), Bse::Error::INTERNAL);

  if (error == 0 && ((need_readable && !BSE_DEVICE_READABLE (self)) ||
                     (need_writable && !BSE_DEVICE_WRITABLE (self))))
    {
      bse_device_close (self);
      error = Bse::Error::DEVICE_NOT_AVAILABLE;
    }

  return error;
}

Bse::Error
bse_device_open (BseDevice      *self,
                 gboolean        need_readable,
                 gboolean        need_writable,
                 const char     *arg_string)
{
  assert_return (BSE_IS_DEVICE (self), Bse::Error::INTERNAL);
  assert_return (!BSE_DEVICE_OPEN (self), Bse::Error::INTERNAL);
  Bse::Error error = Bse::Error::DEVICE_NOT_AVAILABLE;
  if (arg_string)
    error = device_open_args (self, need_readable, need_writable, arg_string);
  else
    {
      SfiRing *node, *entries = bse_device_list (self);
      for (node = entries; node; node = sfi_ring_walk (node, entries))
        {
          BseDeviceEntry *entry = (BseDeviceEntry*) node->data;
          if (!entry->device_error)
            {
              error = device_open_args (self, need_readable, need_writable, entry->device_args);
              if (error == 0)
                break;
            }
        }
      bse_device_entry_list_free (entries);
    }
  return error;
}

void
bse_device_set_opened (BseDevice      *self,
                       const char     *device_name,
                       gboolean        readable,
                       gboolean        writable)
{
  assert_return (BSE_IS_DEVICE (self));
  assert_return (!BSE_DEVICE_OPEN (self));
  assert_return (device_name != NULL);
  assert_return (readable || writable);
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
  assert_return (BSE_IS_DEVICE (self));
  assert_return (BSE_DEVICE_OPEN (self));

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
                      char           *orphan_args,
                      char           *orphan_blurb)
{
  return bse_device_group_entry_new (device, orphan_args, NULL, orphan_blurb);
}

BseDeviceEntry*
bse_device_group_entry_new (BseDevice      *device,
                            char           *orphan_args,
                            char           *orphan_group,
                            char           *orphan_blurb)
{
  BseDeviceEntry *entry = g_new0 (BseDeviceEntry, 1);
  entry->device = (BseDevice*) g_object_ref (device);
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
                      char           *orphan_error)
{
  BseDeviceEntry *entry = g_new0 (BseDeviceEntry, 1);
  entry->device = (BseDevice*) g_object_ref (device);
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
    bse_device_entry_free ((BseDeviceEntry*) sfi_ring_pop_head (&ring));
}

static SfiRing*
device_class_list_entries (GType    type,
                           void   (*request_callback) (BseDevice *device,
                                                       void      *data),
                           void    *data)
{
  SfiRing *ring = NULL;
  BseDeviceClass *klass = (BseDeviceClass*) g_type_class_ref (type);
  if (BSE_DEVICE_CLASS (klass)->driver_name)
    {
      BseDevice *device = (BseDevice*) bse_object_new (type, NULL);
      if (request_callback)
        request_callback (device, data);
      ring = bse_device_list (device);
      g_object_unref (device);
    }
  uint n, i;
  GType *children = g_type_children (type, &n);
  for (i = 0; i < n; i++)
    ring = sfi_ring_concat (ring, device_class_list_entries (children[i], request_callback, data));
  g_free (children);
  return ring;
}

static int
device_entry_prio_cmp  (const void *value1,
                        const void *value2,
                        void       *data)
{
  const BseDeviceEntry *e1 = (BseDeviceEntry*) value1;
  const BseDeviceEntry *e2 = (BseDeviceEntry*) value2;
  return -G_BSEARCH_ARRAY_CMP (BSE_DEVICE_GET_CLASS (e1->device)->driver_rating,
                               BSE_DEVICE_GET_CLASS (e2->device)->driver_rating);
}

SfiRing*
bse_device_class_list (GType    type,
                       void   (*request_callback) (BseDevice *device,
                                                   void      *data),
                       void    *data)
{
  SfiRing *ring = device_class_list_entries (type, request_callback, data);
  ring = sfi_ring_sort (ring, device_entry_prio_cmp, NULL);
  return ring;
}

void
bse_device_class_setup (void          *klass_arg,
                        int            rating,
                        const char    *name,
                        const char    *syntax,
                        const char    *blurb)
{
  assert_return (BSE_IS_DEVICE_CLASS (klass_arg));
  BseDeviceClass *klass = BSE_DEVICE_CLASS (klass_arg);
  klass->driver_rating = rating;
  klass->driver_name = name;
  klass->driver_syntax = syntax;
  klass->driver_blurb = blurb;
}

void
bse_device_dump_list (GType           base_type,
                      const char     *indent,
                      gboolean        with_auto,
                      void          (*request_callback) (BseDevice *device,
                                                         void      *data),
                      void           *data)
{
  SfiRing *node, *ring = bse_device_class_list (base_type, NULL, NULL);
  char *indent2 = g_strconcat (indent ? indent : "", "  ", NULL);
  BseDeviceClass *last_klass = NULL;
  const char *last_topic = NULL;
  for (node = ring; node; node = sfi_ring_walk (node, ring))
    {
      BseDeviceEntry *entry = (BseDeviceEntry*) node->data;
      BseDeviceClass *klass = BSE_DEVICE_GET_CLASS (entry->device);
      if (klass != last_klass)
        {
          if (klass->driver_syntax)
            Bse::printerr ("%s%s %s=%s\n", indent, klass->driver_name, klass->driver_name, klass->driver_syntax);
          else
            Bse::printerr ("%s%s\n", indent, klass->driver_name);
          if (klass->driver_blurb)
            {
              GString *gstring = g_string_new (klass->driver_blurb);
              while (gstring->len && gstring->str[gstring->len - 1] == '\n')
                gstring->str[--gstring->len] = 0;
              g_string_prefix_lines (gstring, indent2);
              Bse::printerr ("%s\n", gstring->str);
              g_string_free (gstring, TRUE);
            }
          last_klass = klass;
          last_topic = NULL;
        }
      if (entry->device_error)
        {
          Bse::printerr ("%sError: %s\n", indent2, entry->device_error);
          last_topic = NULL;
        }
      else if (entry->device_blurb)
        {
          const char *topic = entry->device_group ? entry->device_group : "";
          if (!last_topic || strcmp (last_topic, topic) != 0)
            {
              if (topic[0])
                Bse::printerr ("%sDevices (%s):\n", indent2, topic);
              else
                Bse::printerr ("%sDevices:\n", indent2);
              last_topic = topic;
            }
          Bse::printerr ("%s >        %s\n", indent, entry->device_blurb);
        }
    }
  if (with_auto)
    {
      Bse::printerr ("%sauto\n", indent);
      GString *gstring = g_string_new (/* TRANSLATORS: keep this text to 70 chars in width */
                                       _("Auto is a special driver, it acts as a placeholder for\n"
                                         "automatic driver selection."));
      while (gstring->len && gstring->str[gstring->len - 1] == '\n')
        gstring->str[--gstring->len] = 0;
      g_string_prefix_lines (gstring, indent2);
      Bse::printerr ("%s\n", gstring->str);
      g_string_free (gstring, TRUE);
    }
  bse_device_entry_list_free (ring);
  g_free (indent2);
}

static SfiRing*
device_classes_list (GType type,
                     int   min_rating)
{
  BseDeviceClass *klass = (BseDeviceClass*) g_type_class_ref (type);
  SfiRing *ring = NULL;
  if (klass->driver_name && klass->driver_rating >= min_rating)
    ring = sfi_ring_append (ring, klass);
  else
    g_type_class_unref (klass);
  uint n, i;
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

static int
device_classes_prio_cmp  (gconstpointer   value1,
                          gconstpointer   value2,
                          void *        data)
{
  const BseDeviceClass *c1 = (BseDeviceClass*) value1;
  const BseDeviceClass *c2 = (BseDeviceClass*) value2;
  return -G_BSEARCH_ARRAY_CMP (c1->driver_rating, c2->driver_rating);
}

BseDevice*
bse_device_open_auto (GType           base_type,
                      gboolean        need_readable,
                      gboolean        need_writable,
                      void          (*request_callback) (BseDevice *device,
                                                         void      *data),
                      void           *data,
                      Bse::Error   *errorp)
{
  if (errorp)
    *errorp = Bse::Error::DEVICE_NOT_AVAILABLE;
  BseDevice *device = NULL;
  SfiRing *ring, *class_list = device_classes_list (base_type, 0);
  class_list = sfi_ring_sort (class_list, device_classes_prio_cmp, NULL);
  for (ring = class_list; ring; ring = sfi_ring_walk (ring, class_list))
    {
      BseDeviceClass *klass = BSE_DEVICE_CLASS (ring->data);
      device = (BseDevice*) bse_object_new (G_OBJECT_CLASS_TYPE (klass), NULL);
      if (request_callback)
        request_callback (device, data);
      Bse::Error error = bse_device_open (device, need_readable, need_writable, NULL);
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
  static const char *devstring = "auto";
  static SfiRing ring;
  ring.data = (char*) devstring;
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
                                                         void      *data),
                      void           *data,
                      Bse::Error   *errorp)
{
  if (errorp)
    *errorp = Bse::Error::DEVICE_NOT_AVAILABLE;
  if (!devices)
    devices = auto_ring();
  BseDevice *device = NULL;
  SfiRing *ring, *node, *class_list = device_classes_list (base_type, G_MININT);
  gboolean seen_auto = FALSE;
  for (ring = devices; ring; ring = sfi_ring_walk (ring, devices))
    {
      const char *driverconf = (char*) ring->data;
      const char *args = strchr (driverconf, '=');
      char *driver = g_strndup (driverconf, args ? args - driverconf : strlen (driverconf));
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
          BseDeviceClass *klass = BSE_DEVICE_CLASS (node->data);
          device = (BseDevice*) bse_object_new (G_OBJECT_CLASS_TYPE (klass), NULL);
          if (request_callback)
            request_callback (device, data);
          Bse::Error error = bse_device_open (device, need_readable, need_writable, args ? args + 1 : NULL);
          if (errorp)
            *errorp = error;
          if (error == 0)
            break;
          g_object_unref (device);
          device = NULL;
        }
      else
        Bse::info ("%s: ignoring unknown driver specification: %s", g_type_name (base_type), driverconf);
    }
  device_classes_free (class_list);
  return device;
}

static void
bse_device_class_init (BseDeviceClass *klass)
{
  // GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  klass->driver_rating = -1;
  klass->list_devices = NULL;
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

  assert_return (BSE_DEVICE_FLAGS_USHIFT < BSE_OBJECT_FLAGS_MAX_SHIFT, 0);

  return bse_type_register_abstract (BSE_TYPE_OBJECT,
                                     "BseDevice",
                                     "Abstract device base type",
                                     __FILE__, __LINE__,
                                     &device_info);
}
