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
#if GEGL_CHANT_PROPERTIES

gegl_chant_double (dampness, 0.0, 1.0, 0.95, "dampening, 0.0 is no dampening 1.0 is no change.")

#else

#define GEGL_CHANT_NAME            mblur
#define GEGL_CHANT_SELF            "mblur.c"
#define GEGL_CHANT_DESCRIPTION     "Accumulating motion blur"
#define GEGL_CHANT_CATEGORIES      "misc"

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_INIT
#define GEGL_CHANT_PREPARE

#include "gegl-chant.h"

typedef struct
{
  GeglBuffer *acc;
} Priv;


static void
init (GeglChantOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  Priv *priv = (Priv*)self->priv;
  g_assert (priv == NULL);
  priv = g_malloc0 (sizeof (Priv));
  self->priv = (void*) priv;

  /* XXX: this is not freed when the op is destroyed */
  {
    GeglRectangle extent = {0,0,1024,1024};
    priv->acc = gegl_buffer_new (&extent, babl_format ("RGBA float"));
  }
}

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglNodeContext     *context,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglOperationFilter *filter;
  GeglChantOperation  *self;
  Priv *p;

  filter = GEGL_OPERATION_FILTER (operation);
  self   = GEGL_CHANT_OPERATION (operation);
  p = (Priv*)self->priv;

    {
      GeglBuffer          *temp_in;

      temp_in = gegl_buffer_create_sub_buffer (input, result);

      {
        gint pixels  = result->width*result->height;
        gint bufsize = pixels*4*sizeof(gfloat);
        gfloat *buf = g_malloc(bufsize);
        gfloat *acc = g_malloc(bufsize);
        gfloat dampness;
        gint i;
        gegl_buffer_get (p->acc, 1.0, result, babl_format ("RGBA float"), acc, GEGL_AUTO_ROWSTRIDE);
        gegl_buffer_get (temp_in, 1.0, result, babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE);
        dampness = self->dampness;
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
      gegl_node_context_set_object (context, "output", G_OBJECT (output));
    }

  return  TRUE;
}

#endif
