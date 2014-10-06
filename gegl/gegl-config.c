/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-config.h"

#include "opencl/gegl-cl.h"

G_DEFINE_TYPE (GeglConfig, gegl_config, G_TYPE_OBJECT)

static GObjectClass * parent_class = NULL;

enum
{
  PROP_0,
  PROP_QUALITY,
  PROP_TILE_CACHE_SIZE,
  PROP_CHUNK_SIZE,
  PROP_SWAP,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_THREADS,
  PROP_USE_OPENCL,
  PROP_QUEUE_SIZE,
  PROP_APPLICATION_LICENSE
};

gint _gegl_threads = 1; 

static void
gegl_config_get_property (GObject    *gobject,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  switch (property_id)
    {
      case PROP_TILE_CACHE_SIZE:
        g_value_set_uint64 (value, config->tile_cache_size);
        break;

      case PROP_CHUNK_SIZE:
        g_value_set_int (value, config->chunk_size);
        break;

      case PROP_TILE_WIDTH:
        g_value_set_int (value, config->tile_width);
        break;

      case PROP_TILE_HEIGHT:
        g_value_set_int (value, config->tile_height);
        break;

      case PROP_QUALITY:
        g_value_set_double (value, config->quality);
        break;

      case PROP_SWAP:
        g_value_set_string (value, config->swap);
        break;

      case PROP_THREADS:
        g_value_set_int (value, _gegl_threads);
        break;

      case PROP_USE_OPENCL:
        g_value_set_boolean (value, gegl_cl_is_accelerated());
        break;

      case PROP_QUEUE_SIZE:
        g_value_set_int (value, config->queue_size);
        break;

      case PROP_APPLICATION_LICENSE:
        g_value_set_string (value, config->application_license);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_config_set_property (GObject      *gobject,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  switch (property_id)
    {
      case PROP_TILE_CACHE_SIZE:
        config->tile_cache_size = g_value_get_uint64 (value);
        break;
      case PROP_CHUNK_SIZE:
        config->chunk_size = g_value_get_int (value);
        break;
      case PROP_TILE_WIDTH:
        config->tile_width = g_value_get_int (value);
        break;
      case PROP_TILE_HEIGHT:
        config->tile_height = g_value_get_int (value);
        break;
      case PROP_QUALITY:
        config->quality = g_value_get_double (value);
        return;
      case PROP_SWAP:
        if (config->swap)
          g_free (config->swap);
        config->swap = g_value_dup_string (value);
        break;
      case PROP_THREADS:
        _gegl_threads = g_value_get_int (value);
        return;
      case PROP_USE_OPENCL:
        config->use_opencl = g_value_get_boolean (value);
        break;
      case PROP_QUEUE_SIZE:
        config->queue_size = g_value_get_int (value);
        break;
      case PROP_APPLICATION_LICENSE:
        if (config->application_license)
          g_free (config->application_license);
        config->application_license = g_value_dup_string (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_config_finalize (GObject *gobject)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  if (config->swap)
    g_free (config->swap);

  if (config->application_license)
    g_free (config->application_license);

  G_OBJECT_CLASS (gegl_config_parent_class)->finalize (gobject);
}

static void
gegl_config_class_init (GeglConfigClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class  = g_type_class_peek_parent (klass);

  gobject_class->set_property = gegl_config_set_property;
  gobject_class->get_property = gegl_config_get_property;
  gobject_class->finalize     = gegl_config_finalize;


  g_object_class_install_property (gobject_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width",
                                                     "Tile width",
                                                     "default tile width for created buffers.",
                                                     0, G_MAXINT, 128,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height",
                                                     "Tile height",
                                                     "default tile height for created buffers.",
                                                     0, G_MAXINT, 128,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_TILE_CACHE_SIZE,
                                   g_param_spec_uint64 ("tile-cache-size",
                                                        "Tile Cache size",
                                                        "size of tile cache in bytes",
                                                        0, G_MAXUINT64, 512 * 1024 * 1024,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_CHUNK_SIZE,
                                   g_param_spec_int ("chunk-size",
                                                     "Chunk size",
                                                     "the number of pixels processed simultaneously by GEGL.",
                                                     1, G_MAXINT, 512 * 512,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_QUALITY,
                                   g_param_spec_double ("quality",
                                                        "Quality",
                                                        "quality/speed trade off 1.0 = full quality, 0.0 = full speed",
                                                        0.0, 1.0, 1.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_SWAP,
                                   g_param_spec_string ("swap",
                                                        "Swap",
                                                        "where gegl stores it's swap files",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_THREADS,
                                   g_param_spec_int ("threads",
                                                     "Number of threads",
                                                     "Number of concurrent evaluation threads",
                                                     0, 16, 1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_USE_OPENCL,
                                   g_param_spec_boolean ("use-opencl",
                                                         "Use OpenCL",
                                                         "Try to use OpenCL",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_QUEUE_SIZE,
                                   g_param_spec_int ("queue-size",
                                                     "Queue size",
                                                     "Maximum size of a file backend's writer thread queue (in bytes)",
                                                     2, G_MAXINT, 50 * 1024 *1024,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_APPLICATION_LICENSE,
                                   g_param_spec_string ("application-license",
                                                        "Application license",
                                                        "A list of additional licenses to allow for operations",
                                                        "",
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gegl_config_init (GeglConfig *self)
{
}
