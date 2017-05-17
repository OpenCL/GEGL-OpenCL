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
 * Original authors:
 *
 *      Daniel Cotting <cotting@mygale.org>
 *      Martin Weber   <martweb@gmx.net>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_alien_map_color_model)
  enum_value (GEGL_ALIEN_MAP_COLOR_MODEL_RGB, "rgb", N_("RGB"))
  enum_value (GEGL_ALIEN_MAP_COLOR_MODEL_HSL, "hsl", N_("HSL"))
enum_end (GeglAlienMapColorModel)

property_enum (color_model, _("Color model"),
               GeglAlienMapColorModel, gegl_alien_map_color_model,
               GEGL_ALIEN_MAP_COLOR_MODEL_RGB)
  description (_("What color model used for the transformation"))

property_double (cpn_1_frequency, _("Component 1 frequency"), 1)
  value_range (0, 20)
  ui_meta     ("sensitive", "! cpn-1-keep")
  ui_meta     ("label", "[color-model {rgb} : rgb-label,"
                        " color-model {hsl} : hsl-label]")
  ui_meta     ("rgb-label", _("Red frequency"))
  ui_meta     ("hsl-label", _("Hue frequency"))

property_double  (cpn_2_frequency, _("Component 2 frequency"), 1)
  value_range (0, 20)
  ui_meta     ("sensitive", "! cpn-2-keep")
  ui_meta     ("label", "[color-model {rgb} : rgb-label,"
                        " color-model {hsl} : hsl-label]")
  ui_meta     ("rgb-label", _("Green frequency"))
  ui_meta     ("hsl-label", _("Saturation frequency"))

property_double  (cpn_3_frequency, _("Component 3 frequency"), 1)
  value_range (0, 20)
  ui_meta     ("sensitive", "! cpn-3-keep")
  ui_meta     ("label", "[color-model {rgb} : rgb-label,"
                        " color-model {hsl} : hsl-label]")
  ui_meta     ("rgb-label", _("Blue frequency"))
  ui_meta     ("hsl-label", _("Lightness frequency"))

property_double  (cpn_1_phaseshift, _("Component 1 phase shift"), 0)
  value_range (0, 360)
  ui_meta     ("unit", "degree")
  ui_meta     ("sensitive", "! cpn-1-keep")
  ui_meta     ("label", "[color-model {rgb} : rgb-label,"
                        " color-model {hsl} : hsl-label]")
  ui_meta     ("rgb-label", _("Red phase shift"))
  ui_meta     ("hsl-label", _("Hue phase shift"))

property_double  (cpn_2_phaseshift, _("Component 2 phase shift"), 0)
  value_range (0, 360)
  ui_meta     ("unit", "degree")
  ui_meta     ("sensitive", "! cpn-2-keep")
  ui_meta     ("label", "[color-model {rgb} : rgb-label,"
                        " color-model {hsl} : hsl-label]")
  ui_meta     ("rgb-label", _("Green phase shift"))
  ui_meta     ("hsl-label", _("Saturation phase shift"))

property_double  (cpn_3_phaseshift, _("Component 3 phase shift"), 0)
  value_range (0, 360)
  ui_meta     ("unit", "degree")
  ui_meta     ("sensitive", "! cpn-3-keep")
  ui_meta     ("label", "[color-model {rgb} : rgb-label,"
                        " color-model {hsl} : hsl-label]")
  ui_meta     ("rgb-label", _("Blue phase shift"))
  ui_meta     ("hsl-label", _("Lightness phase shift"))

property_boolean (cpn_1_keep, _("Keep component 1"), FALSE)
  ui_meta     ("label", "[color-model {rgb} : rgb-label,"
                        " color-model {hsl} : hsl-label]")
  ui_meta     ("rgb-label", _("Keep red component"))
  ui_meta     ("hsl-label", _("Keep hue component"))

property_boolean (cpn_2_keep, _("Keep component 2"), FALSE)
  ui_meta     ("label", "[color-model {rgb} : rgb-label,"
                        " color-model {hsl} : hsl-label]")
  ui_meta     ("rgb-label", _("Keep green component"))
  ui_meta     ("hsl-label", _("Keep saturation component"))

property_boolean (cpn_3_keep, _("Keep component 3"), FALSE)
  ui_meta     ("label", "[color-model {rgb} : rgb-label,"
                        " color-model {hsl} : hsl-label]")
  ui_meta     ("rgb-label", _("Keep blue component"))
  ui_meta     ("hsl-label", _("Keep lightness component"))

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     alien_map
#define GEGL_OP_C_SOURCE alien-map.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->color_model == GEGL_ALIEN_MAP_COLOR_MODEL_RGB)
    {
      gegl_operation_set_format (operation, "input",
                                 babl_format ("R'G'B'A float"));
      gegl_operation_set_format (operation, "output",
                                 babl_format ("R'G'B'A float"));
    }
  else
    {
      gegl_operation_set_format (operation, "input",
                                 babl_format ("HSLA float"));
      gegl_operation_set_format (operation, "output",
                                 babl_format ("HSLA float"));
    }
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  gfloat     *in  = in_buf;
  gfloat     *out = out_buf;
  gfloat      freq[3];
  gfloat      phaseshift[3];
  gboolean    keep[3];

  freq[0] = o->cpn_1_frequency * G_PI;
  freq[1] = o->cpn_2_frequency * G_PI;
  freq[2] = o->cpn_3_frequency * G_PI;

  phaseshift[0] = G_PI * o->cpn_1_phaseshift / 180.0;
  phaseshift[1] = G_PI * o->cpn_2_phaseshift / 180.0;
  phaseshift[2] = G_PI * o->cpn_3_phaseshift / 180.0;

  keep[0] = o->cpn_1_keep;
  keep[1] = o->cpn_2_keep;
  keep[2] = o->cpn_3_keep;

  while (samples--)
    {
      gint i;
      for (i = 0; i < 3; i++)
        {
          out[i] = keep[i] ?
            in[i] :
            0.5 * (1.0 + sin ((2 * in[i] - 1.0) * freq[i] + phaseshift[i]));
        }

      out[3] = in[3];

      in  += 4;
      out += 4;
    }

  return TRUE;
}

#include "opencl/gegl-cl.h"
#include "opencl/alien-map.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem              in,
            cl_mem              out,
            size_t              global_worksize,
            const GeglRectangle *roi,
            gint                level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  cl_float3   freq;
  cl_float3   phaseshift;
  cl_int3     keep;
  cl_int      cl_err = 0;

  if (!cl_data)
    {
      const char *kernel_name[] = {"cl_alien_map",
                                   NULL};
      cl_data = gegl_cl_compile_and_build (alien_map_cl_source, kernel_name);
    }

  if (!cl_data)
    return TRUE;

  freq.s[0] = o->cpn_1_frequency * G_PI;
  freq.s[1] = o->cpn_2_frequency * G_PI;
  freq.s[2] = o->cpn_3_frequency * G_PI;

  phaseshift.s[0] = G_PI * o->cpn_1_phaseshift / 180.0;
  phaseshift.s[1] = G_PI * o->cpn_2_phaseshift / 180.0;
  phaseshift.s[2] = G_PI * o->cpn_3_phaseshift / 180.0;

  keep.s[0] = (cl_int)o->cpn_1_keep;
  keep.s[1] = (cl_int)o->cpn_2_keep;
  keep.s[2] = (cl_int)o->cpn_3_keep;

  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[0],
                                    sizeof(cl_mem),    &in,
                                    sizeof(cl_mem),    &out,
                                    sizeof(cl_float3), &freq,
                                    sizeof(cl_float3), &phaseshift,
                                    sizeof(cl_int3),   &keep,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 1,
                                        NULL, &global_worksize, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  return  FALSE;

error:
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare        = prepare;
  operation_class->opencl_support = TRUE;
  point_filter_class->process     = process;
  point_filter_class->cl_process  = cl_process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:alien-map",
    "title",       _("Alien Map"),
    "categories",  "artistic",
    "reference-hash", "48146706af798ef888ba571ce89c1589",
    "description", _("Heavily distort images colors by applying trigonometric functions to map color values."),
    NULL);
}

#endif
