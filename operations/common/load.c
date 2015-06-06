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
#include <gegl-gio-private.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
    description (_("Path of file to load."))
property_uri (uri, _("URI"), "")
    description (_("URI of file to load."))

#else

#define GEGL_OP_C_SOURCE load.c

#include "gegl-plugin.h"

struct _GeglOp
{
  GeglOperationMeta parent_instance;
  gpointer          properties;

  GeglNode *output;
  GeglNode *load;
};

typedef struct
{
  GeglOperationMetaClass parent_class;
} GeglOpClass;

#include "gegl-op.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_META)

#include <stdio.h>

static gchar * const
extension_from_mimetype(const gchar *mime)
{
    // XXX: maybe file loader opts should register mimetypes also?
    static const gchar * const mime_prefix = "image/";
    if (g_str_has_prefix(mime, mime_prefix)) {
       return g_strdup_printf(".%s", mime+strlen(mime_prefix));
    }
    return NULL;
}

static void
do_setup (GeglOperation *operation, const gchar *new_path, const gchar *new_uri)
{
  GeglOp  *self = GEGL_OP (operation);

  if (new_uri && strlen (new_uri) > 0)
      {
        gchar *extension = NULL;
        const gchar *handler   = NULL;

        if (gegl_gio_uri_is_datauri(new_uri))
          {
            gchar *mime = gegl_gio_datauri_get_content_type(new_uri);
            extension = extension_from_mimetype(mime);
            g_free(mime);
          }
        else
            extension = g_strdup(strrchr (new_uri, '.'));

        if (extension)
            handler = gegl_extension_handler_get (extension);
        gegl_node_set (self->load, "operation", handler, NULL);
        gegl_node_set (self->load, "uri", new_uri, NULL);

        g_free(extension);
      }
  else if (new_path && strlen (new_path) > 0)
    {
      const gchar *extension = strrchr (new_path, '.');
      const gchar *handler   = NULL;

      if (!g_file_test (new_path, G_FILE_TEST_EXISTS))
        {
          gchar *name = g_filename_display_name (new_path);
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
                         "path", new_path,
                         NULL);
        }
    }
  else
    {
      gegl_node_set (self->load,
                     "operation", "gegl:text",
                     "string",    "No path specified",
                     NULL);
    }

}

static void attach (GeglOperation *operation)
{
  GeglOp         *self = GEGL_OP (operation);
  GeglProperties *o    = GEGL_PROPERTIES (operation);

  self->output = gegl_node_get_output_proxy (operation->node, "output");

  self->load = gegl_node_new_child (operation->node,
                                    "operation", "gegl:text",
                                    NULL);

  do_setup (operation, o->path, o->uri);

  gegl_node_link (self->load, self->output);

  gegl_operation_meta_watch_node (operation, self->load);
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglOp *self = GEGL_OP (operation);
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
my_set_property (GObject      *gobject,
                 guint         property_id,
                 const GValue *value,
                 GParamSpec   *pspec)
{
  GeglOperation  *operation = GEGL_OPERATION (gobject);
  GeglOp         *self      = GEGL_OP (operation);
  GeglProperties *o         = GEGL_PROPERTIES (operation);

  gchar *old_path = g_strdup (o->path);
  gchar *old_uri = g_strdup (o->uri);

  gboolean props_changed;

  /* The set_property provided by the chant system does the
   * storing and reffing/unreffing of the input properties
   */
  set_property (gobject, property_id, value, pspec);
  props_changed = g_strcmp0 (o->path, old_path) || g_strcmp0 (o->uri, old_uri);

  if (self->load && props_changed)
    do_setup (operation, o->path, o->uri);
  g_free (old_path);
  g_free (old_uri);
}

static void
prepare (GeglOperation *operation)
{
  GeglOp  *self = GEGL_OP (operation);
  GeglOperation *op;

  /* forward the set BablFormat of the image loader on the meta-op itself,
   * making potential cache buffers be created with the proper format, there
   * might be cleaner ways of achieving this.
   */
  g_object_get (self->load, "gegl-operation", &op, NULL);
  gegl_operation_set_format (operation, "output", gegl_operation_get_format (op, "output"));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = my_set_property;

  operation_class->attach = attach;
  operation_class->detect = detect;
  operation_class->prepare = prepare;
  operation_class->no_cache = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:load",
    "title",       "Load Image",
    "categories" , "meta:input",
    "description",
          _("Multipurpose file loader, that uses other native handlers, and "
            "fallback conversion using Image Magick's convert."),
    NULL);

}

#endif
