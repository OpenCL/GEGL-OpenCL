/* This file is an image processing operation for GEGL
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
 * Copyright 2013 Massimo Valentini <mvalentini@src.gnome.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_register_enum (gegl_gaussian_blur_filter2)
  enum_value (GEGL_GAUSSIAN_BLUR_FILTER2_AUTO, "Auto")
  enum_value (GEGL_GAUSSIAN_BLUR_FILTER2_FIR,  "FIR")
  enum_value (GEGL_GAUSSIAN_BLUR_FILTER2_IIR,  "IIR")
gegl_chant_register_enum_end (GeglGaussianBlurFilter2)

gegl_chant_register_enum (gegl_gaussian_blur_policy)
   enum_value (GEGL_GAUSSIAN_BLUR_ABYSS_NONE,  "None")
   enum_value (GEGL_GAUSSIAN_BLUR_ABYSS_CLAMP, "Clamp")
   enum_value (GEGL_GAUSSIAN_BLUR_ABYSS_BLACK, "Black")
   enum_value (GEGL_GAUSSIAN_BLUR_ABYSS_WHITE, "White")
gegl_chant_register_enum_end (GeglGaussianBlurPolicy)


gegl_chant_double_ui (std_dev_x, _("Horizontal Std. Dev."),
                      0.0, 1500.0, 1.5, 0.24, 100.0, 3.0,
                      _("Standard deviation (spatial scale factor)"))
gegl_chant_double_ui (std_dev_y, _("Vertical Std. Dev."),
                      0.0, 1500.0, 1.5, 0.24, 100.0, 3.0,
                      _("Standard deviation (spatial scale factor)"))
gegl_chant_enum      (filter, _("Filter"),
                      GeglGaussianBlurFilter2, gegl_gaussian_blur_filter2,
                      GEGL_GAUSSIAN_BLUR_FILTER2_AUTO,
                      _("How the gaussian kernel is discretized"))
gegl_chant_enum      (abyss_policy, _("Abyss policy"), GeglGaussianBlurPolicy,
                      gegl_gaussian_blur_policy, GEGL_GAUSSIAN_BLUR_ABYSS_NONE,
                      _("How image edges are handled"))
gegl_chant_boolean   (clip_extent, _("Clip to the input extent"), TRUE,
                      _("Should the output extent be clipped to the input extent"))

#else

#define GEGL_CHANT_TYPE_META
#define GEGL_CHANT_C_FILE "gaussian-blur-iir.c"

#include "gegl-chant.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl   = operation->node;
  GeglNode *output = gegl_node_get_output_proxy (gegl, "output");

  GeglNode *vblur  = gegl_node_new_child (gegl,
                                          "operation", "gegl:gblur-1d",
                                          "orientation", 1,
                                          NULL);

  GeglNode *hblur  = gegl_node_new_child (gegl,
                                          "operation", "gegl:gblur-1d",
                                          "orientation", 0,
                                          NULL);

  GeglNode *input  = gegl_node_get_input_proxy (gegl, "input");

  gegl_node_link_many (input, hblur, vblur, output, NULL);

  gegl_operation_meta_redirect (operation, "std-dev-x",    hblur, "std-dev");
  gegl_operation_meta_redirect (operation, "abyss-policy", hblur, "abyss-policy");
  gegl_operation_meta_redirect (operation, "filter",       hblur, "filter");
  gegl_operation_meta_redirect (operation, "clip-extent",  hblur, "clip-extent");

  gegl_operation_meta_redirect (operation, "std-dev-y",    vblur, "std-dev");
  gegl_operation_meta_redirect (operation, "abyss-policy", vblur, "abyss-policy");
  gegl_operation_meta_redirect (operation, "filter",       vblur, "filter");
  gegl_operation_meta_redirect (operation, "clip-extent",  vblur, "clip-extent");
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:gaussian-blur-iir",
    "categories",  "blur",
    "description", _("Performs an averaging of neighboring pixels with the "
                     "normal distribution as weighting"),
                                 NULL);
}

#endif
