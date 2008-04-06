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
#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (dampness, "Dampness", 0.0, 1.0, 0.95, "dampening, 0.0 is no dampening 1.0 is no change.")

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "mblur.c"

#include "gegl-chant.h"

typedef struct
{
  GeglBuffer *acc;
} Priv;


static void
init (GeglChantO *operation)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  Priv         *priv = (Priv*)o->chant_data;
  GeglRectangle extent = {0,0,1024,1024};

  g_assert (priv == NULL);

  priv = g_new0 (Priv, 1);
  o->chant_data = (void*) priv;

  priv->acc = gegl_buffer_new (&extent, babl_format ("RGBA float"));
}

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglOperationFilter *filter;
  GeglChantO          *o;
  Priv *p;

  filter = GEGL_OPERATION_FILTER (operation);
  o   = GEGL_CHANT_PROPERTIES (operation);
  p = (Priv*)o->chant_data;
  if (p == NULL)
    init (o);

    {
      GeglBuffer *temp_in;

      temp_in = gegl_buffer_create_sub_buffer (input, result);

      {
        gint pixels = result->width * result->height;
        gfloat *buf = g_new (gfloat, pixels * 4);
        gfloat *acc = g_new (gfloat, pixels * 4);
        gfloat dampness;
        gint i;
        gegl_buffer_get (p->acc, 1.0, result, babl_format ("RGBA float"), acc, GEGL_AUTO_ROWSTRIDE);
        gegl_buffer_get (temp_in, 1.0, result, babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE);
        dampness = o->dampness;
        for (i=0;i<pixels;i++)
          {
            gint c;
            for (c=0;c<4;c++)
              acc[i*4+c]=acc[i*4+c]*dampness + buf[i*4+c]*(1.0-dampness);
          }
        gegl_buffer_set (p->acc, result, babl_format ("RGBA float"), acc, GEGL_AUTO_ROWSTRIDE);
        gegl_buffer_set (output, result, babl_format ("RGBA float"), acc, GEGL_AUTO_ROWSTRIDE);
        g_free (buf);
        g_free (acc);
      }
      g_object_unref (temp_in);
    }

  return  TRUE;
}


static void
finalize (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->chant_data)
    {
      Priv *p = (Priv*)o->chant_data;

      g_object_unref (p->acc);

      g_free (o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->finalize (object);
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  G_OBJECT_CLASS (klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->name        = "mblur";
  operation_class->categories  = "blur:misc";
  operation_class->description = "Accumulating motion blur";
}

#endif
