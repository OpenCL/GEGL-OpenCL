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

/*  Here in the top of the file the properties of the operation is declared,
 *  this causes the declaration of a structure for containing the data.
 *
 *  The first member of each property_ macro becomes a struct member
 *  in the GeglProperties struct used when processing.
 */

property_double (contrast, _("Contrast"),  1.0)
   description  (_("Magnitude of contrast scaling >1.0 brighten < 1.0 darken"))
   value_range  (-5.0, 5.0)
   ui_range     (0.0, 2.0)

property_double (brightness, _("Brightness"), 0.0)
   description  (_("Amount to increase brightness"))
   value_range  (-3.0, 3.0)
   ui_range     (-1.0, 1.0)

#else

/* Specify the base class we're building our operation on, the base
 * class provides a lot of the legwork so we do not have to. For
 * brightness contrast the best base class is the POINT_FILTER base
 * class.
 */

#define GEGL_OP_POINT_FILTER

/* The C prefix used for some generated functions
 */
#define GEGL_OP_NAME     brightness_contrast

/* We specify the file we're in, this is needed to make the code
 * generation for the properties work.
 */
#define GEGL_OP_C_SOURCE brightness-contrast.c

/* Including gegl-op.h creates most of the GObject boiler plate
 * needed, creating a GeglOp instance structure a GeglOpClass
 * structure for our operation, as well as the needed code to register
 * our new gobject with GEGL.
 */
#include "gegl-op.h"

/* prepare() is called on each operation providing data to a node that
 * is requested to provide a rendered result. When prepare is called
 * all properties are known. For brightness contrast we use this
 * opportunity to dictate the formats of the input and output buffers.
 */
static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

/* For GeglOperationPointFilter subclasses, we operate on linear
 * buffers with a pixel count.
 */
static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  /* Retrieve a pointer to GeglProperties structure which contains all the
   * chanted properties
   */
  GeglProperties *o = GEGL_PROPERTIES (op);
  gfloat     * GEGL_ALIGNED in_pixel;
  gfloat     * GEGL_ALIGNED out_pixel;
  gfloat      brightness, contrast;
  glong       i;

  in_pixel   = in_buf;
  out_pixel  = out_buf;

  brightness = o->brightness;
  contrast   = o->contrast;

  for (i=0; i<n_pixels; i++)
    {
      out_pixel[0] = (in_pixel[0] - 0.5f) * contrast + brightness + 0.5;
      out_pixel[1] = (in_pixel[1] - 0.5f) * contrast + brightness + 0.5;
      out_pixel[2] = (in_pixel[2] - 0.5f) * contrast + brightness + 0.5;
      out_pixel[3] = in_pixel[3]; /* copy the alpha */
      in_pixel  += 4;
      out_pixel += 4;
    }
  return TRUE;
}

#include "opencl/brightness-contrast.cl.h"

/*
 * The class init function sets up information needed for this operations class
 * (template) in the GObject OO framework.
 */
static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;
  gchar                         *composition = "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:brightness-contrast'>"
    "  <params>"
    "    <param name='contrast'>1.8</param>"
    "    <param name='brightness'>0.25</param>"
    "  </params>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  /* override the prepare methods of the GeglOperation class */
  operation_class->prepare = prepare;
  /* override the process method of the point filter class (the process methods
   * of our superclasses deal with the handling on their level of abstraction)
   */
  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
      "name",       "gegl:brightness-contrast",
      "title",      _("Brightness Contrast"),
      "categories", "color",
      "reference-hash", "a60848d705029cad1cb89e44feb7f56e",
      "description", _("Changes the light level and contrast. This operation operates in linear light, 'contrast' is a scale factor around 50%% gray, and 'brightness' a constant offset to apply after contrast scaling."),
      "cl-source"  , brightness_contrast_cl_source,
      "reference-composition", composition,
      NULL);
}

#endif /* closing #ifdef GEGL_PROPERTIES ... else ... */
