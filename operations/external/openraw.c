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
 */

#include "config.h"

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path, "File", "", "Path of file to load.")

#else

#include "gegl-plugin.h"
struct _GeglChant
{
  GeglOperationSource parent_instance;
  gpointer            properties;

  gchar *cached_path;  /* Path we have cached. Detects need for recache. */
};

typedef struct
{
  GeglOperationSourceClass parent_class;
} GeglChantClass;

#define GEGL_CHANT_C_FILE       "openraw.c"
#include "gegl-chant.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_SOURCE)

#include <stdio.h>
#include <libopenraw/libopenraw.h>


/* We can't release the pixel data itself, so we ignore the first argument
 * and release the libopenraw structure instead.
 */
static void
destroy_rawdata (void * rawdata)
{
  or_rawdata_release (rawdata);
}


static void
free_buffer (GeglOperation * operation)
{
  GeglChantO *o    = GEGL_CHANT_PROPERTIES (operation);
  GeglChant  *self = GEGL_CHANT(operation);

  if (o->chant_data)
    {
      g_assert (self->cached_path);
      g_object_unref (o->chant_data);
      o->chant_data = NULL;
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
  GeglChantO *o    = GEGL_CHANT_PROPERTIES (operation);
  GeglChant  *self = GEGL_CHANT(operation);

  ORRawDataRef rawdata;
  ORRawFileRef rawfile;

  /* If the path has changed since last time, destroy our cache */
  if (!self->cached_path || strcmp (self->cached_path, o->path))
    {
      free_buffer(operation);
    }

  if (o->chant_data)
    {
      return o->chant_data;
    }
  g_assert (self->cached_path == NULL);

  /* Gather the file and data resources needed for our cache */
  rawfile = or_rawfile_new(o->path, OR_RAWFILE_TYPE_UNKNOWN);
  if (!rawfile)
    {
      return NULL;
    }

  rawdata = or_rawdata_new();
  if(or_rawfile_get_rawdata(rawfile, rawdata, OR_OPTIONS_NONE) != OR_ERROR_NONE)
    {
      goto clean_file;
    }

  if(or_rawdata_format (rawdata) != OR_DATA_TYPE_CFA)
    {
      goto clean_file;
    }

  /* Build a gegl_buffer, backed with the libopenraw supplied data. */
    {
      GeglRectangle extent = { 0, 0, 0, 0 };
      guint32 width, height;
      void * data = or_rawdata_data(rawdata);

      or_rawdata_dimensions(rawdata, &width, &height);
      g_assert (height > 0 && width > 0);
      extent.width  = width;
      extent.height = height;

      g_assert (o->chant_data == NULL);
      o->chant_data = gegl_buffer_linear_new_from_data(data,
                                                       babl_format ("Y u16"),
                                                       &extent,
                                                       GEGL_AUTO_ROWSTRIDE,
                                                       destroy_rawdata,
                                                       rawdata);
    }

  self->cached_path = g_strdup (o->path);

clean_file:
  or_rawfile_release(rawfile);

  return o->chant_data;
}


static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("Y u16"));
}


static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  if (!load_buffer (operation))
    {
      GeglRectangle nullrect = { 0, 0, 0, 0 };
      return nullrect;
    }

  return *gegl_buffer_get_extent (o->chant_data);
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
         gint                    level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
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
  g_assert(o->chant_data);
  gegl_operation_context_take_object (context, "output", G_OBJECT (o->chant_data));
  g_object_ref (G_OBJECT (o->chant_data));

  return TRUE;
}


#include <glib-object.h>

static void
finalize (GObject *object)
{
  free_buffer (GEGL_OPERATION (object));

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}


static void
gegl_chant_class_init (GeglChantClass *klass)
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
    "name"        , "gegl:openraw-load",
    "categories"  , "hidden",
    "description" , "Camera RAW image loader",
    NULL);

  if (done)
    return;

  /* query libopenraw instead. need a new API */
  gegl_extension_handler_register (".cr2", "gegl:openraw-load");
  gegl_extension_handler_register (".crw", "gegl:openraw-load");
  gegl_extension_handler_register (".erf", "gegl:openraw-load");
  gegl_extension_handler_register (".mrw", "gegl:openraw-load");
  gegl_extension_handler_register (".nef", "gegl:openraw-load");
  gegl_extension_handler_register (".dng", "gegl:openraw-load");

  done = TRUE;
}

#endif
