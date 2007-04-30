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
#if GEGL_CHANT_PROPERTIES
 
gegl_chant_double (dampness, 0.0, 1.0, 0.95, "dampening, 0.0 is no dampening 1.0 is no change.")

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME            mblur
#define GEGL_CHANT_DESCRIPTION     "Accumulating motion blur"
#define GEGL_CHANT_SELF            "mblur.c"
#define GEGL_CHANT_CATEGORIES      "misc"
#define GEGL_CHANT_CLASS_INIT
#define GEGL_CHANT_INIT
#include "gegl-chant.h"

#include <stdio.h>

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
  priv->acc = g_object_new (GEGL_TYPE_BUFFER,
                            "format", babl_format ("RGBA float"),
                            "x",      0,
                            "y",      0,
                            "width",  1024,
                            "height", 1024,
                            NULL);
}

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglOperationFilter *filter;
  GeglChantOperation  *self;
  GeglBuffer          *input;
  GeglBuffer          *output = NULL;
  Priv *p;

  filter = GEGL_OPERATION_FILTER (operation);
  self   = GEGL_CHANT_OPERATION (operation);
  p = (Priv*)self->priv;


  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
    {
      GeglRectangle   *result = gegl_operation_result_rect (operation, context_id);
      GeglBuffer      *temp_in;

      if (result->width ==0 ||
          result->height==0)
        {
          output = g_object_ref (input);
        }
      else
        {
          temp_in = g_object_new (GEGL_TYPE_BUFFER,
                                 "source", input,
                                 "x",      result->x,
                                 "y",      result->y,
                                 "width",  result->width ,
                                 "height", result->height ,
                                 NULL);


          output = g_object_new (GEGL_TYPE_BUFFER,
                                 "format", babl_format ("RGBA float"),
                                 "x",      result->x,
                                 "y",      result->y,
                                 "width",  result->width ,
                                 "height", result->height ,
                                 NULL);
          {
            gint pixels  = result->width*result->height;
            gint bufsize = pixels*4*sizeof(gfloat);
            gfloat *buf = g_malloc(bufsize);
            gfloat *acc = g_malloc(bufsize);
            gegl_buffer_get (p->acc, result, 1.0, babl_format ("RGBA float"), acc);
            gegl_buffer_get (temp_in, result, 1.0, babl_format ("RGBA float"), buf);
            gfloat dampness = self->dampness;
            gint i;
            for (i=0;i<pixels;i++)
              {
                gint c;
                for (c=0;c<4;c++)
                  acc[i*4+c]=acc[i*4+c]*dampness + buf[i*4+c]*(1.0-dampness);
              }
            gegl_buffer_set (p->acc, result, babl_format ("RGBA float"), acc);
            gegl_buffer_set (output, result, babl_format ("RGBA float"), acc);
            g_free (buf);
            g_free (acc);
          }
          g_object_unref (temp_in);
        }

      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
    }
  return  TRUE;
}

#endif
