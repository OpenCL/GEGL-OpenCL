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
#include <glib.h>
#include <glib-object.h>
#include "gegl-config.h"
#include <string.h>
#include <glib/gprintf.h>

G_DEFINE_TYPE (GeglConfig, gegl_config, G_TYPE_OBJECT);

static GObjectClass * parent_class = NULL;

enum
{
  PROP_0,
  PROP_QUALITY,
  PROP_CACHE_SIZE,
  PROP_SWAP,
  PROP_BABL_ERROR
};

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  switch (property_id)
    {
      case PROP_CACHE_SIZE:
        g_value_set_int (value, config->cache_size);
        break;

      case PROP_QUALITY:
        g_value_set_double (value, config->quality);
        break;

      case PROP_BABL_ERROR:
        g_value_set_double (value, config->babl_error);
        break;

      case PROP_SWAP:
        g_value_set_string (value, config->swap);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  switch (property_id)
    {
      case PROP_CACHE_SIZE:
        config->cache_size = g_value_get_int (value);
        break;
      case PROP_QUALITY:
        config->quality = g_value_get_double (value);
        return;
      case PROP_BABL_ERROR:
          {
            gchar buf[256];
            config->babl_error = g_value_get_double (value);
            g_sprintf (buf, "%f", config->babl_error);
            g_setenv ("BABL_ERROR", buf, 0);
            /* babl picks up the babl error through the environment,
             * not sure if it is cached or not
             */
          }
        return;
      case PROP_SWAP:
        if (config->swap)
         g_free (config->swap);
        config->swap = g_value_dup_string (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
finalize (GObject *gobject)
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

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  g_object_class_install_property (gobject_class, PROP_CACHE_SIZE,
                                   g_param_spec_double ("cachei-size", "Cache size", "size of cache in bytes",
                                                     0.0, 1.0, 1.0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_QUALITY,
                                   g_param_spec_double ("quality", "Quality", "quality/speed trade off 1.0 = full quality, 0.0=full speed",
                                                     0.0, 1.0, 1.0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BABL_ERROR,
                                   g_param_spec_double ("babl-error", "babl error", "the error tolerance babl operates with",
                                                     0.0, 0.2, 0.0001,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SWAP,
                                   g_param_spec_string ("swap", "Swap", "where gegl stores it's swap files", NULL,
                                                     G_PARAM_READWRITE));
}

static void
gegl_config_init (GeglConfig *self)
{
  self->swap = NULL;
  self->quality = 1.0;
  self->cache_size = 256*1024*1024;
}
