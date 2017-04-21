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


#ifdef GEGL_PROPERTIES

property_double (dampness, _("Dampness"), 0.95)
    description (_("The value represents the contribution of the past to the new frame."))
    value_range (0.0, 1.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     mblur
#define GEGL_OP_C_SOURCE mblur.c

#include "gegl-op.h"

typedef struct
{
  GeglBuffer *acc;
} Priv;


static void
init (GeglProperties *o)
{
  Priv         *priv = (Priv*)o->user_data;
  GeglRectangle extent = {0,0,1024,1024};

  g_assert (priv == NULL);

  priv = g_new0 (Priv, 1);
  o->user_data = (void*) priv;

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
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o;
  Priv           *p;

  o = GEGL_PROPERTIES (operation);
  p = (Priv*)o->user_data;
  if (p == NULL)
    init (o);
  p = (Priv*)o->user_data;

    {
      GeglBuffer *temp_in;

      temp_in = gegl_buffer_create_sub_buffer (input, result);

      {
        gint pixels = result->width * result->height;
        gfloat *buf = g_new (gfloat, pixels * 4);
        gfloat *acc = g_new (gfloat, pixels * 4);
        gfloat dampness;
        gint i;
        gegl_buffer_get (p->acc, result, 1.0, babl_format ("RGBA float"), acc, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        gegl_buffer_get (temp_in, result, 1.0, babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        dampness = o->dampness;
        for (i=0;i<pixels;i++)
          {
            gint c;
            for (c=0;c<4;c++)
              acc[i*4+c]=acc[i*4+c]*dampness + buf[i*4+c]*(1.0-dampness);
          }
        gegl_buffer_set (p->acc, result, 0, babl_format ("RGBA float"), acc, GEGL_AUTO_ROWSTRIDE);
        gegl_buffer_set (output, result, 0, babl_format ("RGBA float"), acc, GEGL_AUTO_ROWSTRIDE);
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
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      Priv *p = (Priv*)o->user_data;

      g_object_unref (p->acc);

      g_free (o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  G_OBJECT_CLASS (klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:mblur",
    "title",       _("Temporal blur"),
    "categories" , "blur:video",
    "reference-hash", "e5c89dc5f44e6bbf5af4eeed3ea3c3d9", // XXX: doesn't really make sense...
    "description", _("Accumulating motion blur using a kalman filter, for use with video sequences of frames."),
    NULL);
}

#endif
