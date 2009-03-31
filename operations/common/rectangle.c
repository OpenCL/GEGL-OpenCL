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

/* FIXME: need to make this OpenRaster inspired layer integrate better
 * with the newer caching system
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double(x, _("X"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                  _("Horizontal position"))
gegl_chant_double(y, _("Y"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                  _("Vertical position"))
gegl_chant_double(width, _("Width"), 0, G_MAXDOUBLE, 0.0,
                  _("Horizontal extent"))
gegl_chant_double(height, _("Height"), 0, G_MAXDOUBLE, 0.0,
                  _("Vertical extent"))
gegl_chant_color(color, _("Color"), "white",
                  _("Color to render"))

#else

#include <gegl-plugin.h>
struct _GeglChant
{
  GeglOperationMeta parent_instance;
  gpointer          properties;

  GeglNode *self;
  GeglNode *output;

  GeglNode *color;
  GeglNode *crop;
};

typedef struct
{
  GeglOperationMetaClass parent_class;
} GeglChantClass;

#define GEGL_CHANT_C_FILE "rectangle.c"
#include "gegl-chant.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_META)

static void attach (GeglOperation *operation)
{
  GeglChant  *self = GEGL_CHANT (operation);
  GeglChantO *o    = GEGL_CHANT_PROPERTIES (operation);
  GeglNode *gegl;

  self->self = GEGL_OPERATION (self)->node;
  gegl = self->self;

  self->output = gegl_node_get_output_proxy (gegl, "output");

  self->color = gegl_node_new_child (gegl, "operation", "gegl:color",
                                           "value", o->color,
                                           NULL);
  self->crop = gegl_node_new_child (gegl, "operation", "gegl:crop", NULL);

  gegl_node_link_many (self->color, self->crop, self->output, NULL);
}

static void
prepare (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglChant *self = GEGL_CHANT (operation);

    {
      GeglColor *color;
      gegl_node_get (self->color, "value", &color, NULL);

      if (o->color != color)
        gegl_node_set (self->color,
                       "value", o->color,
                       NULL);
    }

    {
      gdouble x,y,width,height;
      gegl_node_get (self->crop,
                     "x", &x,
                     "y", &y,
                     "width", &width,
                     "height", &height,
                     NULL);
      if (x!=o->x ||
          y!=o->y ||
          width!=o->width ||
          height!=o->height)
        gegl_node_set (self->crop,
                       "x",  o->x,
                       "y",  o->y,
                       "width",  o->width,
                       "height",  o->height,
                       NULL);
    }
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass       *object_class;
  GeglOperationClass *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->name        = "gegl:rectangle";
  operation_class->categories  = "input";
  operation_class->description = 
        _("A rectangular source of a fixed size with a solid color");
  operation_class->attach = attach;
  operation_class->prepare = prepare;
}


#endif
