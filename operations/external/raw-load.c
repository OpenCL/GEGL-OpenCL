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
 * Copyright 2011 Paul Sbarra <tones111@hotmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, "File", "")
  description (_("Path of file to load."))
property_int (image_num, "Image number", 0)
property_int (quality, "quality", 10)

#else

#include "gegl-plugin.h"
struct _GeglOp
{
  GeglOperationSource parent_instance;
  gpointer            properties;
};

typedef struct
{
  GeglOperationSourceClass parent_class;
} GeglOpClass;

#define GEGL_OP_NAME raw_load
#define GEGL_OP_C_SOURCE raw-load.c
#include "gegl-op.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_SOURCE)

#include <stdio.h>
#include <libraw.h>

#include <stdio.h>
#include <string.h>
#include <libraw/libraw.h>

typedef struct {
  libraw_data_t            *LibRaw;
  libraw_processed_image_t *image;
  gchar                    *cached_path;
} Private;

static void
raw_close (GeglProperties *o)
{
  Private *p = (Private*)o->user_data;

  g_clear_pointer (&p->cached_path, g_free);
  g_clear_pointer (&p->image, (GDestroyNotify) libraw_dcraw_clear_mem);
  g_clear_pointer (&p->LibRaw, (GDestroyNotify) libraw_close);
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Private    *p = (Private*)o->user_data;
  int         ret;

  if (p == NULL)
    {
      p = g_new0(Private, 1);
      o->user_data = (gpointer)p;
    }

  if (p->cached_path && strcmp (p->cached_path, o->path))
  {
     raw_close (o);
  }

  if (p->LibRaw == NULL)
    {
      g_return_if_fail (p->image == NULL);

      if ((p->LibRaw = libraw_init(LIBRAW_OPTIONS_NONE)) == NULL)
        g_warning ("raw-load: Error Initializing raw library");
      else
        {
          p->LibRaw->params.shot_select = o->image_num;
    
          p->LibRaw->params.aber[0] = 1.0;
          p->LibRaw->params.aber[2] = 1.0;
          p->LibRaw->params.gamm[0] = 1.0 / 2.4;
          p->LibRaw->params.gamm[1] = 12.92;
          p->LibRaw->params.bright = 1.0f;
          p->LibRaw->params.half_size = FALSE;
          p->LibRaw->params.highlight = 0;
          p->LibRaw->params.use_auto_wb = TRUE;
          p->LibRaw->params.use_camera_wb = TRUE;
          p->LibRaw->params.use_camera_matrix = 1;
          p->LibRaw->params.output_color = 1;
          p->LibRaw->params.user_flip = 0;
          p->LibRaw->params.no_auto_bright = 1;
          p->LibRaw->params.auto_bright_thr = 0.01f;
          p->LibRaw->params.use_fuji_rotate = -1;

          p->LibRaw->params.output_bps = 16;
          p->LibRaw->params.user_qual = o->quality;

          if ((ret = libraw_open_file(p->LibRaw, o->path)) != LIBRAW_SUCCESS)
          {

            g_warning ("raw-load: Unable to open %s: %s", o->path, libraw_strerror (ret));
          }
          else
          {
            p->cached_path = strdup (o->path);
          }
        }
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  Private        *p      = (Private*)o->user_data;
  GeglRectangle  result = {0,0,0,0};

  if (p == NULL)
    {
      prepare(operation);
      p = (Private*)o->user_data;
    }

  if (p->LibRaw != NULL &&
      (p->LibRaw->progress_flags & LIBRAW_PROGRESS_IDENTIFY)) 
    {
      result.width  = p->LibRaw->sizes.width;
      result.height = p->LibRaw->sizes.height;
      gegl_operation_set_format (operation, "output", babl_format ("R'G'B' u16"));
    }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         int                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Private *p = (Private*)o->user_data;
  GeglRectangle rect = {0,0,0,0};
  const Babl *format = NULL;
  int ret;

  g_assert (p);

  if (p->LibRaw != NULL)
    {
      if (!(p->LibRaw->progress_flags & LIBRAW_PROGRESS_LOAD_RAW))
        {
          if ((ret = libraw_unpack(p->LibRaw)) != LIBRAW_SUCCESS)
            g_warning ("raw-load: Error unpacking data: %s", libraw_strerror (ret));

          if (ret == LIBRAW_SUCCESS && !(p->LibRaw->progress_flags & LIBRAW_PROGRESS_CONVERT_RGB))
            {
              if ((ret = libraw_dcraw_process(p->LibRaw)) != LIBRAW_SUCCESS)
                g_warning ("raw-load: Error processing data: %s", libraw_strerror (ret));
              else if ((p->image = libraw_dcraw_make_mem_image(p->LibRaw, &ret)) == NULL)
                g_warning ("raw-load: Error converting image: %s", libraw_strerror (ret));
            }
        }
    }

  if (p->image != NULL)
    {
      g_assert (p->image->type == LIBRAW_IMAGE_BITMAP);
      rect.width  = p->image->width;
      rect.height = p->image->height;

      if (p->image->colors == 1)
        format = babl_format ("Y' u16");
      else // 3 color channels
        format = babl_format ("R'G'B' u16");

      gegl_buffer_set (output, &rect, 0, format, p->image->data, GEGL_AUTO_ROWSTRIDE);
      return TRUE;
    }

  return FALSE;
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      raw_close (o);
      g_free (o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize(object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  static gboolean done = FALSE;

  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class = GEGL_OPERATION_SOURCE_CLASS (klass);

  operation_class->prepare     = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  source_class->process = process;
  object_class->finalize = finalize;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:raw-load",
    "title",       _("libraw File Loader"),
    "categories",  "hidden",
    "description", "Camera RAW image loader",
    NULL);

  if (done)
    return;

  gegl_operation_handlers_register_loader (
    "image/x-pentax-pef", "gegl:raw-load");
  gegl_operation_handlers_register_loader (
    ".pef", "gegl:raw-load");

  gegl_operation_handlers_register_loader (
    "image/x-nikon-nef", "gegl:raw-load");
  gegl_operation_handlers_register_loader (
    ".nef", "gegl:raw-load");

  gegl_operation_handlers_register_loader (
    "image/x-fuji-raf", "gegl:raw-load");
  gegl_operation_handlers_register_loader (
    ".raf", "gegl:raw-load");

  gegl_operation_handlers_register_loader (
    "image/x-olympus-orf", "gegl:raw-load");
  gegl_operation_handlers_register_loader (
    ".orf", "gegl:raw-load");

  gegl_operation_handlers_register_loader (
    "image/x-epson-erf", "gegl:raw-load");
  gegl_operation_handlers_register_loader (
    ".erf", "gegl:raw-load");

  gegl_operation_handlers_register_loader (
    "image/x-minolta-mrw", "gegl:raw-load");
  gegl_operation_handlers_register_loader (
    ".mrw", "gegl:raw-load");

  gegl_operation_handlers_register_loader (
    "image/x-canon-crw", "gegl:raw-load");
  gegl_operation_handlers_register_loader (
    ".crw", "gegl:raw-load");

  gegl_operation_handlers_register_loader (
    "image/x-canon-cr2", "gegl:raw-load");
  gegl_operation_handlers_register_loader (
    ".cr2", "gegl:raw-load");

  gegl_operation_handlers_register_loader (
    "image/x-sony-arw", "gegl:raw-load");
  gegl_operation_handlers_register_loader (
    ".arw", "gegl:raw-load");

 done = TRUE;
}

#endif
