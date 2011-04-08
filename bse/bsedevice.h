/* BSE - Better Sound Engine
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
#ifndef __BSE_DEVICE_H__
#define __BSE_DEVICE_H__

#include        <bse/bseobject.h>

G_BEGIN_DECLS

/* --- object type macros --- */
#define BSE_TYPE_DEVICE              (BSE_TYPE_ID (BseDevice))
#define BSE_DEVICE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BSE_TYPE_DEVICE, BseDevice))
#define BSE_DEVICE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), BSE_TYPE_DEVICE, BseDeviceClass))
#define BSE_IS_DEVICE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BSE_TYPE_DEVICE))
#define BSE_IS_DEVICE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), BSE_TYPE_DEVICE))
#define BSE_DEVICE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BSE_TYPE_DEVICE, BseDeviceClass))
/* flag tests */
#define BSE_DEVICE_OPEN(pdev)        ((BSE_OBJECT_FLAGS (pdev) & BSE_DEVICE_FLAG_OPEN) != 0)
#define BSE_DEVICE_READABLE(pdev)    ((BSE_OBJECT_FLAGS (pdev) & BSE_DEVICE_FLAG_READABLE) != 0)
#define BSE_DEVICE_WRITABLE(pdev)    ((BSE_OBJECT_FLAGS (pdev) & BSE_DEVICE_FLAG_WRITABLE) != 0)


/* --- enums --- */
typedef enum    /*< skip >*/
{
  BSE_DEVICE_FLAG_OPEN          = 1 << (BSE_OBJECT_FLAGS_USHIFT + 0),
  BSE_DEVICE_FLAG_READABLE      = 1 << (BSE_OBJECT_FLAGS_USHIFT + 1),
  BSE_DEVICE_FLAG_WRITABLE      = 1 << (BSE_OBJECT_FLAGS_USHIFT + 2)
} BseDeviceFlags;
#define BSE_DEVICE_FLAGS_USHIFT (BSE_OBJECT_FLAGS_USHIFT + 3)


/* --- BseDevice structs --- */
typedef struct _BseDevice       BseDevice;
typedef struct _BseDeviceClass  BseDeviceClass;
struct _BseDevice
{
  BseObject              parent_object;
  /* valid while BSE_DEVICE_OPEN() */
  gchar                 *open_device_name;
  gchar                 *open_device_args;
};
struct _BseDeviceClass
{
  BseObjectClass        parent_class;
  gint                  driver_rating;
  const gchar          *driver_name;
  const gchar          *driver_syntax;
  const gchar          *driver_blurb;
  SfiRing*            (*list_devices)  (BseDevice    *device);
  BseErrorType        (*open)          (BseDevice    *device,
                                        gboolean        require_readable,
                                        gboolean        require_writable,
                                        guint           n_args,
                                        const gchar   **args);
  void                (*post_open)     (BseDevice    *device);
  void                (*pre_close)     (BseDevice    *device);
  void                (*close)         (BseDevice    *device);
};
typedef struct {
  BseDevice      *device;
  gchar          *device_args;
  gchar          *device_blurb;
  gchar          *device_group; /* usually NULL */
  gchar          *device_error; /* if device_name == NULL */
} BseDeviceEntry;


/* --- prototypes --- */
void            bse_device_class_setup     (gpointer        klass,
                                            gint            rating,
                                            const gchar    *name,
                                            const gchar    *syntax,
                                            const gchar    *blurb);
SfiRing*        bse_device_list            (BseDevice      *device);
BseErrorType    bse_device_open            (BseDevice      *device,
                                            gboolean        need_readable,
                                            gboolean        need_writable,
                                            const gchar    *arg_string);
void            bse_device_set_opened      (BseDevice      *device,
                                            const gchar    *device_name,
                                            gboolean        readable,
                                            gboolean        writable);
void            bse_device_close           (BseDevice      *device);
BseDeviceEntry* bse_device_entry_new       (BseDevice      *device,
                                            gchar          *orphan_args,
                                            gchar          *orphan_blurb);
BseDeviceEntry* bse_device_group_entry_new (BseDevice      *device,
                                            gchar          *orphan_args,
                                            gchar          *orphan_group,
                                            gchar          *orphan_blurb);
BseDeviceEntry* bse_device_error_new       (BseDevice      *device,
                                            gchar          *orphan_error);
void            bse_device_entry_free      (BseDeviceEntry *entry);
void            bse_device_entry_list_free (SfiRing        *list);
SfiRing*        bse_device_class_list      (GType           type,
                                            void          (*request_callback) (BseDevice *device,
                                                                               gpointer   data),
                                            gpointer        data);
void            bse_device_dump_list       (GType           base_type,
                                            const gchar    *indent,
                                            gboolean        with_auto,
                                            void          (*request_callback) (BseDevice *device,
                                                                               gpointer   data),
                                            gpointer        data);
BseDevice*      bse_device_open_best       (GType           base_type,
                                            gboolean        need_readable,
                                            gboolean        need_writable,
                                            SfiRing        *devices,
                                            void          (*request_callback) (BseDevice *device,
                                                                               gpointer   data),
                                            gpointer        data,
                                            BseErrorType   *errorp);
BseDevice*      bse_device_open_auto       (GType           base_type,
                                            gboolean        need_readable,
                                            gboolean        need_writable,
                                            void          (*request_callback) (BseDevice *device,
                                                                               gpointer   data),
                                            gpointer        data,
                                            BseErrorType   *errorp);


G_END_DECLS

#endif /* __BSE_DEVICE_H__ */
