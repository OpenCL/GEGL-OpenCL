/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include <gegl.h>
#include <gegl-module.h>
#include <gegl/gegl-operation-source.h>
#include <string.h>
#include <stdio.h>

#define GEGL_TYPE_OPERATION_LOAD           (gegl_operation_load_get_type ())
#define GEGL_OPERATION_LOAD(obj)           ((GeglOperationLoad*)(obj))
#define GEGL_OPERATION_LOAD_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj), GEGL_TYPE_OPERATION_LOAD, GeglOperationLoadClass))
#define GEGL_OPERATION_LOAD_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEGL_TYPE_OPERATION_LOAD, GeglOperationLoadClass))

typedef struct LoaderMapping
{
  gchar *extension;
  gchar *handler;
} LoaderMapping;

static LoaderMapping mappings[]=
{
  {".jpg",  "jpg-load"},
  {".jpeg", "jpg-load"},
  {".JPG",  "jpg-load"},
  {".JPEG", "jpg-load"},
  {".png",  "png-load"},
  {".PNG",  "png-load"},
  {".raw",  "raw-load"},
  {".RAW",  "raw-load"},
  {".raf",  "raw-load"},
  {".nef",  "raw-load"},
  {".NEF",  "raw-load"},
  {NULL,    "magick-load"} /* fallthrough */
};
  
typedef struct _GeglOperationLoad      GeglOperationLoad;
typedef struct _GeglOperationLoadClass GeglOperationLoadClass;

struct _GeglOperationLoad
{
  GeglOperationSource  parent_instance;
  gchar               *path;

  /* private */
  gchar               *cached_path;
  GeglBuffer          *cached;
};

struct _GeglOperationLoadClass
{
  GeglOperationSourceClass parent_class;
};


static void gegl_operation_load_class_init (GeglOperationLoadClass *klass);
static void gegl_operation_load_init       (GeglOperationLoad      *self);

static GType
gegl_operation_load_get_type (GTypeModule *module)
{
    static GType gegl_operation_load_id = 0;
    if (G_UNLIKELY (gegl_operation_load_id == 0))
    {
      static const GTypeInfo select_info =
      {
        sizeof (GeglOperationLoadClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gegl_operation_load_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GeglOperationLoad),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gegl_operation_load_init,
      };

      gegl_operation_load_id =
        gegl_module_register_type (module,
                                     GEGL_TYPE_OPERATION_SOURCE,
                                     "GeglOperationLoad",
                                     &select_info, 0);
    }
  return gegl_operation_load_id;
}

static const GeglModuleInfo modinfo = {
  GEGL_MODULE_ABI_VERSION,
  "load",
  "v0.0",
  "(c) 2006, released under the LGPL",
  "June 2006"
};

const GeglModuleInfo *gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}
  
gboolean gegl_module_register (GTypeModule *module)
{
  gegl_operation_load_get_type (module);
  return TRUE;
}

enum
{
  PROP_0,
  PROP_PATH,
  PROP_LAST
};

static void
get_property (GObject *gobject,
              guint property_id,
              GValue *value,
              GParamSpec *pspec)
{
  GeglOperationLoad *self = GEGL_OPERATION_LOAD (gobject);

  switch (property_id)
  {
    case PROP_PATH:
      g_value_set_string (value, self->path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
set_property (GObject *gobject,
              guint property_id,
              const GValue *value,
              GParamSpec *pspec)
{
  GeglOperationLoad *self = GEGL_OPERATION_LOAD (gobject);

  switch (property_id)
  {
    case PROP_PATH:
      if (self->path)
        g_free (self->path);
      self->path = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
  }
}

static void
gegl_operation_load_init (GeglOperationLoad *self)
{
  self->path = NULL;
  self->cached_path = NULL;
  self->cached = NULL;
}

static void dispose            (GObject       *gobject);
static gboolean evaluate       (GeglOperation *operation,
                                const gchar   *output_prop);
static gboolean calc_have_rect (GeglOperation *self);

static void
gegl_operation_load_class_init (GeglOperationLoadClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GeglOperationSourceClass *parent_class = GEGL_OPERATION_SOURCE_CLASS (klass);
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  object_class->dispose      = dispose;
  object_class->set_property = set_property;
  object_class->get_property = get_property;

  parent_class->evaluate = evaluate;
  operation_class->calc_have_rect = calc_have_rect;
  gegl_operation_class_set_name (operation_class, "load");;
  gegl_operation_class_set_description (operation_class,
           "Multipurpose file loader, that uses other "
           "native handlers, and fallback conversion "
           "using image magick's convert.");
  operation_class->categories = "sources";

  g_object_class_install_property (object_class, PROP_PATH,
      g_param_spec_string ("path", "path",
                           "The path to the file to load",
                           "/tmp/romedalen.png",
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT | GEGL_PAD_INPUT));
}

static void
refresh_cache (GeglOperationLoad *self)
{
  if (!self->cached ||
      ((self->cached_path &&
       self->path) &&
       strcmp (self->path, self->cached_path)))
    {
        GeglNode *temp_gegl;
        gchar xml[1024]="";
        LoaderMapping *map = &mappings[0];

        if (self->cached)
          {
            g_object_unref (self->cached);
            self->cached = NULL;
            g_free (self->cached_path);
          }

        while (map->extension)
          {
            if (strstr (self->path, map->extension))
              {
                sprintf (xml,
                  "<gegl ><tree><node class='%s' path='%s'></node></tree></gegl>",
                  map->handler, self->path);
                break;
              }
            map++;
          }
        if (map->extension==NULL) /* no extension matched, using fallback */
          {
            sprintf (xml, "<gegl><tree><node class='%s' path='%s'></node></tree></gegl>",
                 map->handler, self->path);
          }

    temp_gegl = gegl_xml_parse (xml);
    gegl_node_apply (temp_gegl, "output");

    /*FIXME: this should be unneccesary, using the graph
     * directly as a node is more elegant.
     */
    gegl_node_get (temp_gegl, "output", &(self->cached), NULL);
    g_object_unref (temp_gegl);
    g_assert (self->cached);
    self->cached_path = g_strdup (self->path);
  }
}

static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglOperationSource *op_source = GEGL_OPERATION_SOURCE(operation);
  GeglOperationLoad *self = GEGL_OPERATION_LOAD (operation);

  if(strcmp("output", output_prop))
    return FALSE;

  if (op_source->output)
    g_object_unref (op_source->output);
  op_source->output=NULL;

  refresh_cache (self);

  g_assert (self->cached);
  {
    GeglRect *result = gegl_operation_result_rect (operation);

    op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                        "source", self->cached,
                        "x",      result->x,
                        "y",      result->y,
                        "width",  result->w,
                        "height", result->h,
                        NULL);
  }
  return  TRUE;
}

static gboolean
calc_have_rect (GeglOperation *operation)
{
  GeglOperationLoad *self = GEGL_OPERATION_LOAD (operation);
  gint width, height;

  refresh_cache (self);
  g_assert (self->cached);

  width  = GEGL_BUFFER (self->cached)->width;
  height = GEGL_BUFFER (self->cached)->height;

  gegl_operation_set_have_rect (operation, 0, 0, width, height);

  return TRUE;
}

static void dispose (GObject *gobject)
{
  GeglOperationLoad *self = GEGL_OPERATION_LOAD (gobject);
  if (self->cached)
    {
      g_object_unref (self->cached);
      self->cached = NULL;
    }
  if (self->cached_path)
    {
      g_free (self->cached_path);
    }
  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (gobject)))->dispose (gobject);
}
