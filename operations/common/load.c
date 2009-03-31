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
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_file_path (path, _("File"), "", _("Path of file to load."))

#else

#define GEGL_CHANT_C_FILE       "load.c"

#include "gegl-plugin.h"

struct _GeglChant
{
  GeglOperationMeta parent_instance;
  gpointer          properties;

  GeglNode *output;
  GeglNode *load;
  gchar    *cached_path;
};

typedef struct
{
  GeglOperationMetaClass parent_class;
} GeglChantClass;

#include "gegl-chant.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_META)

#include <stdio.h>

static void attach (GeglOperation *operation)
{
  GeglChant *self = GEGL_CHANT (operation);

  self->output = gegl_node_get_output_proxy (operation->node, "output");

  self->load = gegl_node_new_child (operation->node,
                                    "operation", "gegl:text",
                                    "string",    "foo",
                                    NULL);

  gegl_node_link (self->load, self->output);
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglChant *self = GEGL_CHANT (operation);
  GeglNode *output = self->output;
  GeglRectangle bounds;

  bounds = gegl_node_get_bounding_box (output); /* hopefully this is
                                                   as correct as original
                                                   which was peeking
                                                   directly into output->have_rect
                                                   */

  if (x >= bounds.x &&
      y >= bounds.y &&
      x  < bounds.x + bounds.width &&
      y  < bounds.y + bounds.height)
    return operation->node;

  return NULL;
}

static void
prepare (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglChant  *self = GEGL_CHANT (operation);
  /* warning: this might trigger regeneration of the graph,
   *          for now this is evaded by just ignoring additional
   *          requests to be made into members of the graph
   */

    if (o->path[0]==0 && self->cached_path == NULL)
      {
          gegl_node_set (self->load,
                         "operation", "gegl:text",
                         "size", 20.0,
                         "string", "Eeeeek!",
                         NULL);
      }
    else
    {
      if (o->path[0] &&
          (self->cached_path == NULL || strcmp (o->path, self->cached_path)))
        {
          const gchar *extension = strrchr (o->path, '.');
          const gchar *handler   = NULL;

          if (!g_file_test (o->path, G_FILE_TEST_EXISTS))
            {
              gchar *name = g_filename_display_name (o->path);
              gchar *tmp  = g_strdup_printf ("File '%s' does not exist", name);
              g_free (name);

              g_warning ("load: %s", tmp);
              gegl_node_set (self->load,
                             "operation", "gegl:text",
                             "size", 12.0,
                             "string", tmp,
                             NULL);
              g_free (tmp);
            }
          else
            {
              if (extension)
                handler = gegl_extension_handler_get (extension);
              gegl_node_set (self->load,
                             "operation", handler,
                             NULL);
              gegl_node_set (self->load,
                             "path",  o->path,
                             NULL);
            }
          if (self->cached_path)
            g_free (self->cached_path);
          self->cached_path = g_strdup (o->path);
        }
    }
}

static void
dispose (GObject *object)
{
  GeglChant *self = GEGL_CHANT (object);

  if (self->cached_path)
    {
      g_free (self->cached_path);
      self->cached_path = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->dispose (object);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->dispose = dispose;

  operation_class->attach = attach;
  operation_class->detect = detect;
  operation_class->prepare = prepare;

  operation_class->name        = "gegl:load";
  operation_class->categories  = "meta:input";
  operation_class->description =
        _("Multipurpose file loader, that uses other native handlers, and "
          "fallback conversion using image magick's convert.");

  operation_class->no_cache = TRUE;
}

#endif
