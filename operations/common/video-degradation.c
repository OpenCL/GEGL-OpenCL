/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Exchange one color with the other (settable threshold to convert from
 * one color-shade to another...might do wonders on certain images, or be
 * totally useless on others).
 *
 * Author: Adam D. Moss <adam@foxbox.org>
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_video_degradation_type)
  enum_value (GEGL_VIDEO_DEGRADATION_TYPE_STAGGERED, "staggered",
              N_("Staggered"))
  enum_value (GEGL_VIDEO_DEGRADATION_TYPE_LARGE_STAGGERED, "large_staggered",
              N_("Large staggered"))
  enum_value (GEGL_VIDEO_DEGRADATION_TYPE_STRIPED, "striped",
              N_("Striped"))
  enum_value (GEGL_VIDEO_DEGRADATION_TYPE_WIDE_STRIPED, "wide_striped",
              N_("Wide striped"))
  enum_value (GEGL_VIDEO_DEGRADATION_TYPE_LONG_STAGGERED, "long_staggered",
              N_("Long staggered"))
  enum_value (GEGL_VIDEO_DEGRADATION_TYPE_3X3, "3x3",
              N_("3x3"))
  enum_value (GEGL_VIDEO_DEGRADATION_TYPE_LARGE_3X3, "large_3x3",
              N_("Large 3x3"))
  enum_value (GEGL_VIDEO_DEGRADATION_TYPE_Hex, "hex",
              N_("Hex"))
  enum_value (GEGL_VIDEO_DEGRADATION_TYPE_DOTS, "dots",
              N_("Dots"))
enum_end (GeglVideoDegradationType)

property_enum (pattern, _("Pattern"), GeglVideoDegradationType,
               gegl_video_degradation_type,
               GEGL_VIDEO_DEGRADATION_TYPE_STRIPED)
  description (_("Type of RGB pattern to use"))

property_boolean (additive, _("Additive"), TRUE)
  description(_("Whether the function adds the result to the original image."))

property_boolean (rotated, _("Rotated"), FALSE)
  description(_("Whether to rotate the RGB pattern by ninety degrees."))

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE video-degradation.c

#include "gegl-op.h"

#define MAX_PATTERNS       9
#define MAX_PATTERN_SIZE 108

static const gint   pattern_width[MAX_PATTERNS] = { 2, 4, 1, 1, 2, 3, 6, 6, 5 };
static const gint   pattern_height[MAX_PATTERNS] = { 6, 12, 3, 6, 12, 3, 6,
                                                     18, 15 };

static const gint pattern[MAX_PATTERNS][MAX_PATTERN_SIZE] =
{
  {
    0, 1,
    0, 2,
    1, 2,
    1, 0,
    2, 0,
    2, 1,
  },
  {
    0, 0, 1, 1,
    0, 0, 1, 1,
    0, 0, 2, 2,
    0, 0, 2, 2,
    1, 1, 2, 2,
    1, 1, 2, 2,
    1, 1, 0, 0,
    1, 1, 0, 0,
    2, 2, 0, 0,
    2, 2, 0, 0,
    2, 2, 1, 1,
    2, 2, 1, 1,
  },
  {
    0,
    1,
    2,
  },
  {
    0,
    0,
    1,
    1,
    2,
    2,
  },
  {
    0, 1,
    0, 1,
    0, 2,
    0, 2,
    1, 2,
    1, 2,
    1, 0,
    1, 0,
    2, 0,
    2, 0,
    2, 1,
    2, 1,
  },
  {
    0, 1, 2,
    2, 0, 1,
    1, 2, 0,
  },
  {
    0, 0, 1, 1, 2, 2,
    0, 0, 1, 1, 2, 2,
    2, 2, 0, 0, 1, 1,
    2, 2, 0, 0, 1, 1,
    1, 1, 2, 2, 0, 0,
    1, 1, 2, 2, 0, 0,
  },
  {
    2, 2, 0, 0, 0, 0,
    2, 2, 2, 0, 0, 2,
    2, 2, 2, 2, 2, 2,
    2, 2, 2, 1, 1, 2,
    2, 2, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    0, 0, 1, 1, 1, 1,
    0, 0, 0, 1, 1, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 2, 2, 0,
    0, 0, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2,
    1, 1, 2, 2, 2, 2,
    1, 1, 1, 2, 2, 1,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 0, 0, 1,
    1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
  },
  {
    0, 1, 2, 0, 0,
    1, 1, 1, 2, 0,
    0, 1, 2, 2, 2,
    0, 0, 1, 2, 0,
    0, 1, 1, 1, 2,
    2, 0, 1, 2, 2,
    0, 0, 0, 1, 2,
    2, 0, 1, 1, 1,
    2, 2, 0, 1, 2,
    2, 0, 0, 0, 1,
    1, 2, 0, 1, 1,
    2, 2, 2, 0, 1,
    1, 2, 0, 0, 0,
    1, 1, 2, 0, 1,
    1, 2, 2, 2, 0,
  }
};

static void
prepare (GeglOperation *operation)
{
  const Babl     *format = babl_format ("R'G'B'A float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem               in_buf,
            cl_mem               out_buf,
            const size_t         n_pixels,
            const GeglRectangle *roi,
            gint                 level)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_GET_CLASS (operation);
  GeglClRunData *cl_data = operation_class->cl_data;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const size_t gbl_size[2] = {roi->width, roi->height};
  const size_t gbl_off[2]  = {roi->x, roi->y};
  cl_int cl_err = 0;
  cl_mem filter_pat = NULL;

  if (!cl_data)
    goto error;
  filter_pat = gegl_clCreateBuffer (gegl_cl_get_context (),
                                    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                    pattern_width[o->pattern] *
                                    pattern_height[o->pattern] * sizeof(cl_int),
                                    (void*)pattern[o->pattern],
                                    &cl_err);
  CL_CHECK;
  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[0],
                                    sizeof(cl_mem), &in_buf,
                                    sizeof(cl_mem), &out_buf,
                                    sizeof(cl_mem), &filter_pat,
                                    sizeof(cl_int), &pattern_width[o->pattern],
                                    sizeof(cl_int), &pattern_height[o->pattern],
                                    sizeof(cl_int), &o->additive,
                                    sizeof(cl_int), &o->rotated,
                                    NULL);
  CL_CHECK;
  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        gbl_off, gbl_size, NULL,
                                        0, NULL, NULL);
  CL_CHECK;
  cl_err = gegl_clFinish (gegl_cl_get_command_queue ());
  CL_CHECK;
  cl_err = gegl_clReleaseMemObject (filter_pat);
  CL_CHECK;
  return FALSE;
  error:
    if (filter_pat)
      gegl_clReleaseMemObject (filter_pat);
    return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat *input  = in_buf;
  gfloat *output = out_buf;
  gfloat value;
  gint x, y;
  gint real_x, real_y;
  gint b;
  gint sel_b;
  gint idx;

  for (y = 0 ; y < roi->height ; y++)
    {
      real_y = roi->y + y;
      for (x = 0 ; x < roi->width ; x++)
        {
          real_x = roi->x + x;

          if (o->rotated)
            {
              sel_b = pattern[o->pattern][pattern_width[o->pattern]*
                (real_x % pattern_height[o->pattern]) +
                 real_y % pattern_width[o->pattern] ];
            }
          else
            {
              sel_b = pattern[o->pattern][pattern_width[o->pattern]*
                 (real_y % pattern_height[o->pattern]) +
                 real_x % pattern_width[o->pattern] ];
            }

            for (b = 0; b < 4; b++)
              {
                idx = (x + y * roi->width) * 4 + b;
                if (b < 3 )
                  {
                    value = (sel_b == b) ? input[idx] : 0.f;
                    if (o->additive)
                      {
                        gfloat temp = value + input[idx];
                        value = MIN (temp, 1.0);
                      }
                    output[idx] = value;
                  }
                else
                 {
                   output[idx] = input[idx];
                 }
              }
        }
    }

  return TRUE;
}

#include "opencl/video-degradation.cl.h"

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare = prepare;

  filter_class->process    = process;
  filter_class->cl_process = cl_process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:video-degradation",
    "title",       _("Video Degradation"),
    "categories",  "distort",
    "license",     "GPL3+",
    "description", _("This function simulates the degradation of "
                     "being on an old low-dotpitch RGB video monitor."),
    "cl-source"  , video_degradation_cl_source,
    NULL);
}

#endif
