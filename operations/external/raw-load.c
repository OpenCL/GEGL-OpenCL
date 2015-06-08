/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * Copyright 2008 Hubert Figuière <hub@figuiere.net>
 * Copyright 2011 Chong Kai Xiong <w_velocity@yahoo.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, "File", "")
  description (_("Path of file to load."))

#else

#include "gegl-plugin.h"
struct _GeglOp
{
  GeglOperationSource parent_instance;
  gpointer            properties;

  gchar *cached_path;  /* Path we have cached. Detects need for recache. */
};

typedef struct
{
  GeglOperationSourceClass parent_class;
} GeglOpClass;

#define GEGL_OP_C_SOURCE raw-load.c
#include "gegl-op.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_SOURCE)

#include <stdio.h>
#include <libraw.h>

static void
free_buffer (GeglOperation * operation)
{
  GeglProperties *o    = GEGL_PROPERTIES (operation);
  GeglOp         *self = GEGL_OP(operation);

  if (o->user_data)
    {
      g_assert (self->cached_path);
      g_object_unref (o->user_data);
      o->user_data = NULL;
    }

  if (self->cached_path)
  {
    g_free (self->cached_path);
    self->cached_path = NULL;
  }
}

/* Loads the RAW pixel data from the specified path in chant parameters into
 * a GeglBuffer for caching. Maintains copy of the path that has been cached
 * so we can check for modifications and recache.
 */
static GeglBuffer *
load_buffer (GeglOperation *operation)
{
  GeglProperties *o    = GEGL_PROPERTIES (operation);
  GeglOp         *self = GEGL_OP(operation);

  libraw_data_t *rawdata;

  /* If the path has changed since last time, destroy our cache */
  if (!self->cached_path || strcmp (self->cached_path, o->path))
    {
      free_buffer(operation);
    }

  if (o->user_data)
    {
      return o->user_data;
    }
  g_assert (self->cached_path == NULL);

  rawdata = libraw_init (0);

  if (!rawdata)
    {
      return NULL;
    }

  /* Gather the file and data resources needed for our cache */

  if (libraw_open_file(rawdata, o->path))
    {
      goto clean_data;
    }

  /* TODO: Handle 3-color Foveon and 4-color Sinar 4-shot */
  if (rawdata->idata.is_foveon)
    {
      goto clean_data;
    }

  if (libraw_unpack (rawdata))
    {
      goto clean_data;
    }

  /* Build a gegl_buffer, backed with the LibRaw supplied data. */
    {
      GeglRectangle extent = { 0, 0, 0, 0 };
      guint32 width, height;
      gint row, col, component;
      guint16 *buffer;
      gint offset;

      width  = rawdata->sizes.iwidth;
      height = rawdata->sizes.iheight;

      g_assert (height > 0 && width > 0);
      extent.width  = width;
      extent.height = height;

      buffer = g_new (guint16, width * height);
      offset = 0;

      for (row = 0; row < height; row++)
        for (col = 0; col < width; col++)
          {
            component = rawdata->idata.filters >> (((row << 1 & 14) + (col & 1)) << 1) & 3;
            buffer[offset] = rawdata->image[offset][component];
            offset++;
          }

      g_assert (o->user_data == NULL);
      o->user_data = gegl_buffer_linear_new_from_data (buffer,
                                                        babl_format ("Y u16"),
                                                        &extent,
                                                        GEGL_AUTO_ROWSTRIDE,
                                                        (void*)g_free,
                                                        NULL);
    }

  self->cached_path = g_strdup (o->path);

clean_data:
  libraw_close(rawdata);

  return o->user_data;
}


static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("Y u16"));
}


static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties *o    = GEGL_PROPERTIES (operation);
  if (!load_buffer (operation))
    {
      GeglRectangle nullrect = { 0, 0, 0, 0 };
      return nullrect;
    }

  return *gegl_buffer_get_extent (o->user_data);
}


static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}


static gboolean
process (GeglOperation          *operation,
         GeglOperationContext   *context,
         const gchar            *output_pad,
         const GeglRectangle    *result,
         int                     level)
{
  GeglProperties *o    = GEGL_PROPERTIES (operation);
  g_assert (g_str_equal (output_pad, "output"));

  if (!load_buffer (operation))
    {
      return FALSE;
    }

  /* Give the operation a reference to the object, and keep a reference
   * ourselves. We desperately do not want to delete our cached object, as
   * we continue to service metadata calls after giving the object to the
   * context.
   */
  g_assert (o->user_data);
  gegl_operation_context_take_object (context, "output", G_OBJECT (o->user_data));
  g_object_ref (G_OBJECT (o->user_data));

  return TRUE;
}


#include <glib-object.h>

static void
finalize (GObject *object)
{
  free_buffer (GEGL_OPERATION (object));

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  static gboolean done = FALSE;

  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->finalize = finalize;

  operation_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;
  operation_class->prepare     = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:raw-load",
    "title",       _("LibRAW File Loader"),
    "categories",  "hidden",
    "description", "Camera RAW image loader",
    NULL);

  if (done)
    return;

  /* query libopenraw instead. need a new API */
  gegl_extension_handler_register (".pef", "gegl:raw-load");
  gegl_extension_handler_register (".nef", "gegl:raw-load");
  gegl_extension_handler_register (".raf", "gegl:raw-load");
  gegl_extension_handler_register (".orf", "gegl:raw-load");
  gegl_extension_handler_register (".mrw", "gegl:raw-load");
  gegl_extension_handler_register (".crw", "gegl:raw-load");
  gegl_extension_handler_register (".cr2", "gegl:raw-load");

  done = TRUE;
}

#endif
