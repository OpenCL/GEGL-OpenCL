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

#ifdef GEGL_PROPERTIES

enum_start (gegl_gaussian_blur_filter2)
   enum_value (GEGL_GAUSSIAN_BLUR_FILTER2_AUTO, "auto", N_("Auto"))
   enum_value (GEGL_GAUSSIAN_BLUR_FILTER2_FIR,  "fir",  N_("FIR"))
   enum_value (GEGL_GAUSSIAN_BLUR_FILTER2_IIR,  "iir",  N_("IIR"))
enum_end (GeglGaussianBlurFilter2)

enum_start (gegl_gaussian_blur_policy)
   enum_value (GEGL_GAUSSIAN_BLUR_ABYSS_NONE,  "none",   N_("None"))
   enum_value (GEGL_GAUSSIAN_BLUR_ABYSS_CLAMP, "clamp",  N_("Clamp"))
   enum_value (GEGL_GAUSSIAN_BLUR_ABYSS_BLACK, "black",  N_("Black"))
   enum_value (GEGL_GAUSSIAN_BLUR_ABYSS_WHITE, "white",  N_("White"))
enum_end (GeglGaussianBlurPolicy)

property_double (std_dev_x, _("Size X"), 1.5)
   description (_("Standard deviation for the horizontal axis"))
   value_range (0.0, 1500.0)
   ui_range    (0.24, 100.0)
   ui_gamma    (3.0)
   ui_meta     ("unit", "pixel-distance")
   ui_meta     ("axis", "x")

property_double (std_dev_y, _("Size Y"), 1.5)
   description (_("Standard deviation (spatial scale factor)"))
   value_range (0.0, 1500.0)
   ui_range    (0.24, 100.0)
   ui_gamma    (3.0)
   ui_meta     ("unit", "pixel-distance")
   ui_meta     ("axis", "y")

property_enum (filter, _("Filter"),
               GeglGaussianBlurFilter2, gegl_gaussian_blur_filter2,
               GEGL_GAUSSIAN_BLUR_FILTER2_AUTO)
   description (_("How the gaussian kernel is discretized"))

property_enum (abyss_policy, _("Abyss policy"), GeglGaussianBlurPolicy,
               gegl_gaussian_blur_policy, GEGL_GAUSSIAN_BLUR_ABYSS_CLAMP)
   description (_("How image edges are handled"))

property_boolean (clip_extent, _("Clip to the input extent"), TRUE)
   description (_("Should the output extent be clipped to the input extent"))

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     gaussian_blur
#define GEGL_OP_C_SOURCE gaussian-blur.c

#include "gegl-op.h"

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

  gegl_operation_meta_watch_nodes (operation, hblur, vblur, NULL);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;
  operation_class->threaded = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:gaussian-blur",
    "title",          _("Gaussian Blur"),
    "categories",     "blur",
    "reference-hash", "5d10ee663c5ff908c3c081e516154873",
    "description", _("Performs an averaging of neighboring pixels with the "
                     "normal distribution as weighting"),
                                 NULL);
}

#endif
