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
 * Copyright 2006-2009 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

/* no properties */

#else

#define GEGL_CHANT_TYPE_POINT_COMPOSER
#define GEGL_CHANT_C_FILE        "over.c"

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("RaGaBaA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation        *op,
          void                *in_buf,
          void                *aux_buf,
          void                *out_buf,
          glong                n_pixels,
          const GeglRectangle *roi)
{
  gint i;
  gfloat *in = in_buf;
  gfloat *aux = aux_buf;
  gfloat *out = out_buf;

  if (aux==NULL)
    return TRUE;

  for (i = 0; i < n_pixels; i++)
    {
      gint   j;
      gfloat aA, aB, aD;

      aB = in[3];
      aA = aux[3];
      aD = aA + aB - aA * aB;

      for (j = 0; j < 3; j++)
        {
          gfloat cA, cB;

          cB = in[j];
          cA = aux[j];
          out[j] = cA + cB * (1 - aA);
        }
      out[3] = aD;
      in  += 4;
      aux += 4;
      out += 4;
    }
  return TRUE;
}

#ifdef HAS_G4FLOAT

static gboolean
process_gegl4float (GeglOperation      *op,
                    void               *in_buf,
                    void                *aux_buf,
                    void                *out_buf,
                    glong                n_pixels,
                    const GeglRectangle *roi)
{
  g4float *A = aux_buf;
  g4float *B = in_buf;
  g4float *D = out_buf;

  if (B==NULL || n_pixels == 0)
    return TRUE;

  while (n_pixels--)
    {
      *D = *A + *B * (g4float_one - g4float_all(g4float_a(*A)[3]));

      A++; B++; D++;
    }

  return TRUE;
}

#endif

/* Fast paths */
static gboolean operation_process (GeglOperation        *operation,
                                   GeglOperationContext *context,
                                   const gchar          *output_prop,
                                   const GeglRectangle  *result)
{
  GeglOperationClass  *operation_class;
  gpointer input, aux;
  operation_class = GEGL_OPERATION_CLASS (gegl_chant_parent_class);

  /* get the raw values this does not increase the reference count */
  input = gegl_operation_context_get_object (context, "input");
  aux = gegl_operation_context_get_object (context, "aux");

  /* pass the input/aux buffers directly through if they are alone*/
  {
    const GeglRectangle *in_extent = NULL;
    const GeglRectangle *aux_extent = NULL;

    if (input)
      in_extent = gegl_buffer_get_abyss (input);

    if ((!input ||
        (aux && !gegl_rectangle_intersect (NULL, in_extent, result))))
      {
         gegl_operation_context_take_object (context, "output",
                                             g_object_ref (aux));
         return TRUE;
      }
    if (aux)
      aux_extent = gegl_buffer_get_abyss (aux);

    if (!aux ||
        (input && !gegl_rectangle_intersect (NULL, aux_extent, result)))
      {
        gegl_operation_context_take_object (context, "output",
                                            g_object_ref (input));
        return TRUE;
      }
  }
  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return operation_class->process (operation, context, output_prop, result);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class      = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);
  operation_class->prepare = prepare;
  operation_class->process = operation_process;

  point_composer_class->process = process;

#ifdef HAS_G4FLOAT
  gegl_operation_class_add_processor (operation_class,
                                      G_CALLBACK (process_gegl4float), "simd");
#endif


  operation_class->name        = "gegl:over";
  operation_class->description =
        _("Porter Duff operation over (d = cA + cB * (1 - aA))");
  operation_class->categories  = "compositors:porter-duff";
}

#endif
