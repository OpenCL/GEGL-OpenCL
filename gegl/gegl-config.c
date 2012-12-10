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
  PROP_CACHE_SIZE, /* deprecated */
  PROP_TILE_CACHE_SIZE,
  PROP_CHUNK_SIZE,
  PROP_SWAP,
  PROP_BABL_TOLERANCE,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_THREADS,
  PROP_USE_OPENCL,
  PROP_QUEUE_LIMIT
};

static void
gegl_config_get_property (GObject    *gobject,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  switch (property_id)
    {
      case PROP_CACHE_SIZE:
        g_value_set_int (value, config->tile_cache_size);
        break;

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

      case PROP_BABL_TOLERANCE:
        g_value_set_double (value, config->babl_tolerance);
        break;

      case PROP_SWAP:
        g_value_set_string (value, config->swap);
        break;

      case PROP_THREADS:
        g_value_set_int (value, config->threads);
        break;

      case PROP_USE_OPENCL:
        g_value_set_boolean (value, config->use_opencl);
        break;

      case PROP_QUEUE_LIMIT:
        g_value_set_int (value, config->queue_limit);
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
      case PROP_CACHE_SIZE:
        config->tile_cache_size = g_value_get_int (value);
        break;
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
      case PROP_BABL_TOLERANCE:
          {
            /* XXX: this only works the first time, and since it is a
             * construct property the values overriden from gimp does not take
             * effect
             */
            static gboolean first = TRUE;
            static gboolean overridden = FALSE;

            gchar buf[256];

            if (first)
              {
                if (g_getenv ("BABL_TOLERANCE") != NULL)
                  overridden = TRUE;
                first = FALSE;
              }
            if (!overridden)
              {
                config->babl_tolerance = g_value_get_double (value);
                g_sprintf (buf, "%f", config->babl_tolerance);
                g_setenv ("BABL_TOLERANCE", buf, 0);
                /* babl picks up the babl error through the environment, babl
                 * caches valid conversions though so this needs to be set
                 * before _any_ processing is done
                 */
              }
          }
        return;
      case PROP_SWAP:
        if (config->swap)
         g_free (config->swap);
        config->swap = g_value_dup_string (value);
        break;
      case PROP_THREADS:
        config->threads = g_value_get_int (value);
        return;
      case PROP_USE_OPENCL:
        config->use_opencl = g_value_get_boolean (value);

        if (config->use_opencl)
          config->use_opencl = gegl_cl_init (NULL);

        break;
      case PROP_QUEUE_LIMIT:
        config->queue_limit = g_value_get_int (value);
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
                                                     0, G_MAXINT, 64,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  /* deprecated */
  g_object_class_install_property (gobject_class, PROP_CACHE_SIZE,
                                   g_param_spec_int ("cache-size",
                                                     "Cache size",
                                                     "deprecated, use tile-cache-size instead",
                                                     0, G_MAXINT, 512 * 1024 * 1024,
                                                     G_PARAM_READWRITE));

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

  g_object_class_install_property (gobject_class, PROP_BABL_TOLERANCE,
                                   g_param_spec_double ("babl-tolerance",
                                                        "babl error",
                                                        "the error tolerance babl operates with",
                                                        0.0, 0.2, 0.000001,
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
                                                     "Number of concurrent evaluation threads",
                                                     "default tile height for created buffers.",
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

  g_object_class_install_property (gobject_class, PROP_QUEUE_LIMIT,
                                   g_param_spec_int ("queue-limit",
                                                     "Queue limit",
                                                     "Maximum number of entries in the file tile backend's writer queue",
                                                     2, G_MAXINT, 1000,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
gegl_config_init (GeglConfig *self)
{
}
