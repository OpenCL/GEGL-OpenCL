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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (value, _("Opacity"), -10.0, 10.0, 1.0,
         _("Global opacity value that is always used on top of the optional auxiliary input buffer."))

#else

#define GEGL_CHANT_TYPE_POINT_COMPOSER
#define GEGL_CHANT_C_FILE       "opacity.c"

#include "gegl-chant.h"


static void prepare (GeglOperation *self)
{
  gegl_operation_set_format (self, "input", babl_format ("RaGaBaA float"));
  gegl_operation_set_format (self, "output", babl_format ("RaGaBaA float"));
  gegl_operation_set_format (self, "aux", babl_format ("Y float"));
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi)
{
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  gfloat value = GEGL_CHANT_PROPERTIES (op)->value;

  if (aux == NULL)
    {
      g_assert (value != 1.0); /* buffer should have been passed through */
      while (samples--)
        {
          gint j;
          for (j=0; j<4; j++)
            out[j] = in[j] * value;
          in  += 4;
          out += 4;
        }
    }
  else if (value == 1.0)
    while (samples--)
      {
        gint j;
        for (j=0; j<4; j++)
          out[j] = in[j] * (*aux);
        in  += 4;
        out += 4;
        aux += 1;
      }
  else
    while (samples--)
      {
        gfloat v = (*aux) * value;
        gint j;
        for (j=0; j<4; j++)
          out[j] = in[j] * v;
        in  += 4;
        out += 4;
        aux += 1;
      }
  return TRUE;
}


#ifdef HAS_G4FLOAT
static gboolean
process_simd (GeglOperation       *op,
              void                *in_buf,
              void                *aux_buf,
              void                *out_buf,
              glong                samples,
              const GeglRectangle *roi)
{
  GeglChantO *o   = GEGL_CHANT_PROPERTIES (op);
  g4float    *in  = in_buf;
  gfloat     *aux = aux_buf;
  g4float    *out = out_buf;

  /* add 0.5 to brightness here to make the logic in the innerloop tighter
   */
  g4float  value      = g4float_all(o->value);

  if (aux == NULL)
    {
      g_assert (o->value != 1.0); /* should haven been passed through */
      while (samples--)
        *(out ++) = *(in ++) * value;
    }
  else if (o->value == 1.0)
    while (samples--)
      {
        *(out++) = *(in++) * g4float_all (*(aux));
        aux++;
      }
  else
    while (samples--)
      {
        *(out++) = *(in++) * g4float_all ((*(aux))) * value;
        aux++;
      }
  return TRUE;
}
#endif

/* Fast path when opacity is a no-op
 */
static gboolean operation_process (GeglOperation        *operation,
                                   GeglOperationContext *context,
                                   const gchar          *output_prop,
                                   const GeglRectangle  *result)
{
  GeglOperationClass  *operation_class;
  gpointer in, aux;
  operation_class = GEGL_OPERATION_CLASS (gegl_chant_parent_class);

  /* get the raw values this does not increase the reference count */
  in = gegl_operation_context_get_object (context, "input");
  aux = gegl_operation_context_get_object (context, "aux");

  if (in && !aux && GEGL_CHANT_PROPERTIES (operation)->value == 1.0)
    {
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
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
  /* add conditionally compiled variation of process(), gegl should be able
   * to determine which is fastest and hopefully if any implementation is
   * broken and not conforming to the reference implementation.
   */
  gegl_operation_class_add_processor (operation_class,
                                      G_CALLBACK (process_simd), "simd");
#endif


  operation_class->name        = "gegl:opacity";
  operation_class->categories  = "transparency";
  operation_class->description =
        _("Weights the opacity of the input both the value of the aux"
          " input and the global value property.");
}

#endif
