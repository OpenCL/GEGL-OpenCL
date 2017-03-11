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
 * Copyright 2013 Daniel Sabo
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (value, _("Opacity"), 1.0)
    description (_("Global opacity value that is always used on top of the optional auxiliary input buffer."))
    value_range (-10.0, 10.0)
    ui_range    (0.0, 1.0)

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME     opacity
#define GEGL_OP_C_SOURCE opacity.c

#include "gegl-op.h"

#include <stdio.h>

static void
prepare (GeglOperation *self)
{
  const Babl *fmt = gegl_operation_get_source_format (self, "input");
  GeglProperties *o = GEGL_PROPERTIES (self);

  if (fmt)
    {
      const Babl *model = babl_format_get_model (fmt);

      if (model == babl_model ("R'aG'aB'aA") ||
          model == babl_model ("Y'aA"))
        {
          o->user_data = NULL;
          fmt = babl_format ("R'aG'aB'aA float");
        }
      else if (model == babl_model ("RaGaBaA") ||
               model == babl_model ("YaA"))
        {
          o->user_data = NULL;
          fmt = babl_format ("RaGaBaA float");
        }
      else if (model == babl_model ("R'G'B'A") ||
               model == babl_model ("R'G'B'")  ||
               model == babl_model ("Y'")      ||
               model == babl_model ("Y'A"))
        {
          o->user_data = (void*)0xabc;
          fmt = babl_format ("R'G'B'A float");
        }
      else
        {
          o->user_data = (void*)0xabc;
          fmt = babl_format ("RGBA float");
        }
    }
  else
    {
      o->user_data = (void*)0xabc;
      fmt = babl_format ("RGBA float");
    }

  gegl_operation_set_format (self, "input", fmt);
  gegl_operation_set_format (self, "output", fmt);
  gegl_operation_set_format (self, "aux", babl_format ("Y float"));

  return;
}

static void
process_RaGaBaAfloat (GeglOperation       *op,
                      void                *in_buf,
                      void                *aux_buf,
                      void                *out_buf,
                      glong                samples,
                      const GeglRectangle *roi,
                      gint                 level)
{
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  gfloat value = GEGL_PROPERTIES (op)->value;

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
}

static void
process_RGBAfloat (GeglOperation       *op,
                   void                *in_buf,
                   void                *aux_buf,
                   void                *out_buf,
                   glong                samples,
                   const GeglRectangle *roi,
                   gint                 level)
{
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  gfloat value = GEGL_PROPERTIES (op)->value;

  if (aux == NULL)
    {
      g_assert (value != 1.0); /* buffer should have been passed through */
      while (samples--)
        {
          gint j;
          for (j=0; j<3; j++)
            out[j] = in[j];
          out[3] = in[3] * value;
          in  += 4;
          out += 4;
        }
    }
  else if (value == 1.0)
    while (samples--)
      {
        gint j;
        for (j=0; j<3; j++)
          out[j] = in[j];
        out[3] = in[3] * (*aux);
        in  += 4;
        out += 4;
        aux += 1;
      }
  else
    while (samples--)
      {
        gfloat v = (*aux) * value;
        gint j;
        for (j=0; j<3; j++)
          out[j] = in[j];
        out[3] = in[3] * v;
        in  += 4;
        out += 4;
        aux += 1;
      }
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  if (GEGL_PROPERTIES (op)->user_data != NULL)
    process_RGBAfloat (op, in_buf, aux_buf, out_buf, samples, roi, level);
  else
    process_RaGaBaAfloat (op, in_buf, aux_buf, out_buf, samples, roi, level);

  return TRUE;
}

#include "opencl/gegl-cl.h"

#include "opencl/opacity.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_process (GeglOperation       *op,
            cl_mem               in_tex,
            cl_mem               aux_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  cl_int cl_err = 0;
  int kernel;
  gfloat value;

  if (!cl_data)
    {
      const char *kernel_name[] = {"gegl_opacity_RaGaBaA_float", "gegl_opacity_RGBA_float", NULL};
      cl_data = gegl_cl_compile_and_build (opacity_cl_source, kernel_name);
    }
  if (!cl_data) return TRUE;

  value = GEGL_PROPERTIES (op)->value;

  kernel = (GEGL_PROPERTIES (op)->user_data == NULL)? 0 : 1;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[kernel], 0, sizeof(cl_mem),   (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[kernel], 1, sizeof(cl_mem),   (aux_tex)? (void*)&aux_tex : NULL);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[kernel], 2, sizeof(cl_mem),   (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[kernel], 3, sizeof(cl_float), (void*)&value);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[kernel], 1,
                                        NULL, &global_worksize, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

/* Fast path when opacity is a no-op
 */
static gboolean operation_process (GeglOperation        *operation,
                                   GeglOperationContext *context,
                                   const gchar          *output_prop,
                                   const GeglRectangle  *result,
                                   gint                  level)
{
  GeglOperationClass  *operation_class;
  gpointer in, aux;
  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  /* get the raw values this does not increase the reference count */
  in = gegl_operation_context_get_object (context, "input");
  aux = gegl_operation_context_get_object (context, "aux");

  if (in && !aux && GEGL_PROPERTIES (operation)->value == 1.0)
    {
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }
  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return operation_class->process (operation, context, output_prop, result,
                                  gegl_operation_context_get_level (context));
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class      = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->process = operation_process;
  point_composer_class->process = process;
  point_composer_class->cl_process = cl_process;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:opacity",
    "categories" , "transparency",
    "title",       _("Opacity"),
    "reference-hash", "b20e8c1d7bb20af95f724191feb10103",
    "description",
          _("Weights the opacity of the input both the value of the aux"
            " input and the global value property."),
    NULL);
}

#endif
