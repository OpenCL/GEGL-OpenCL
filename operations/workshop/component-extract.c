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
 * Copyright 2015 Thomas Manni <thomas.manni@free.fr>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_component_extract)
  enum_value (GEGL_COMPONENT_EXTRACT_RGB_RED, "RGB Red", N_("RGB Red"))
  enum_value (GEGL_COMPONENT_EXTRACT_RGB_GREEN, "RGB Green", N_("RGB Green"))
  enum_value (GEGL_COMPONENT_EXTRACT_RGB_BLUE, "RGB Blue", N_("RGB Blue"))
  enum_value (GEGL_COMPONENT_EXTRACT_HUE, "Hue", N_("Hue"))
  enum_value (GEGL_COMPONENT_EXTRACT_HSV_SATURATION, "HSV Saturation", N_("HSV Saturation"))
  enum_value (GEGL_COMPONENT_EXTRACT_HSV_VALUE, "HSV Value", N_("HSV Value"))
  enum_value (GEGL_COMPONENT_EXTRACT_HSL_SATURATION, "HSL Saturation", N_("HSL Saturation"))
  enum_value (GEGL_COMPONENT_EXTRACT_HSL_LIGHTNESS, "HSL Lightness", N_("HSL Lightness"))
  enum_value (GEGL_COMPONENT_EXTRACT_CMYK_CYAN, "CMYK Cyan", N_("CMYK Cyan"))
  enum_value (GEGL_COMPONENT_EXTRACT_CMYK_MAGENTA, "CMYK Magenta", N_("CMYK Magenta"))
  enum_value (GEGL_COMPONENT_EXTRACT_CMYK_YELLOW, "CMYK Yellow", N_("CMYK Yellow"))
  enum_value (GEGL_COMPONENT_EXTRACT_CMYK_KEY, "CMYK Key", N_("CMYK Key"))
  enum_value (GEGL_COMPONENT_EXTRACT_YCBCR_Y, "Y'CbCr Y'", N_("Y'CbCr Y'"))
  enum_value (GEGL_COMPONENT_EXTRACT_YCBCR_CB, "Y'CbCr Cb", N_("Y'CbCr Cb"))
  enum_value (GEGL_COMPONENT_EXTRACT_YCBCR_CR, "Y'CbCr Cr", N_("Y'CbCr Cr"))
  enum_value (GEGL_COMPONENT_EXTRACT_LAB_L, "LAB L", N_("LAB L"))
  enum_value (GEGL_COMPONENT_EXTRACT_LAB_A, "LAB A", N_("LAB A"))
  enum_value (GEGL_COMPONENT_EXTRACT_LAB_B, "LAB B", N_("LAB B"))
  enum_value (GEGL_COMPONENT_EXTRACT_ALPHA, "Alpha", N_("Alpha"))
enum_end (GeglComponentExtract)

property_enum (component, _("Component"),
               GeglComponentExtract, gegl_component_extract,
               GEGL_COMPONENT_EXTRACT_RGB_RED)
  description (_("Component to extract"))
  
property_boolean (invert, _("Invert component"), FALSE)
     description (_("Invert the extracted component"))

property_boolean (linear, _("Linear output"), FALSE)
     description (_("Use linear output instead of gamma corrected"))

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE  component-extract.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *output_format = (o->linear) ? babl_format ("Y float") : babl_format ("Y' float");

  switch (o->component)
    {
      case GEGL_COMPONENT_EXTRACT_ALPHA:
        {
          gegl_operation_set_format (operation, "input",
                                     babl_format ("YA float"));
        }
      break;

      case GEGL_COMPONENT_EXTRACT_RGB_RED:
      case GEGL_COMPONENT_EXTRACT_RGB_GREEN:
      case GEGL_COMPONENT_EXTRACT_RGB_BLUE:
        {
          gegl_operation_set_format (operation, "input",
                                     babl_format ("R'G'B' float"));
        }
      break;

      case GEGL_COMPONENT_EXTRACT_HUE:
      case GEGL_COMPONENT_EXTRACT_HSV_SATURATION:
      case GEGL_COMPONENT_EXTRACT_HSV_VALUE:
        {
          gegl_operation_set_format (operation, "input",
                                     babl_format ("HSV float"));
        }
      break;

      case GEGL_COMPONENT_EXTRACT_HSL_LIGHTNESS:
      case GEGL_COMPONENT_EXTRACT_HSL_SATURATION:
        {
          gegl_operation_set_format (operation, "input",
                                     babl_format ("HSL float"));
        }
      break;

      case GEGL_COMPONENT_EXTRACT_CMYK_CYAN:
      case GEGL_COMPONENT_EXTRACT_CMYK_MAGENTA:
      case GEGL_COMPONENT_EXTRACT_CMYK_YELLOW:
      case GEGL_COMPONENT_EXTRACT_CMYK_KEY:
        {
          gegl_operation_set_format (operation, "input",
                                     babl_format ("CMYK float"));
        }
      break;

      case GEGL_COMPONENT_EXTRACT_YCBCR_Y:
      case GEGL_COMPONENT_EXTRACT_YCBCR_CB:
      case GEGL_COMPONENT_EXTRACT_YCBCR_CR:
        {
          gegl_operation_set_format (operation, "input",
                                     babl_format ("Y'CbCr float"));
        }
      break;

      case GEGL_COMPONENT_EXTRACT_LAB_L:
      case GEGL_COMPONENT_EXTRACT_LAB_A:
      case GEGL_COMPONENT_EXTRACT_LAB_B:
        {
          gegl_operation_set_format (operation, "input",
                                     babl_format ("CIE Lab float"));
        }
      break;
    }

    gegl_operation_set_format (operation, "output", output_format);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *format = gegl_operation_get_format (operation, "input");
  gfloat     *in  = in_buf;
  gfloat     *out = out_buf;
  gint        component_index;
  gint        n_components;
  gdouble     min = 0.0;
  gdouble     max = 1.0;

  n_components = babl_format_get_n_components (format);

  switch (o->component)
    {
      case GEGL_COMPONENT_EXTRACT_RGB_RED:
      case GEGL_COMPONENT_EXTRACT_HUE:
      case GEGL_COMPONENT_EXTRACT_CMYK_CYAN:
      case GEGL_COMPONENT_EXTRACT_YCBCR_Y:
      case GEGL_COMPONENT_EXTRACT_LAB_L:
        {
          component_index = 0;

          if (o->component == GEGL_COMPONENT_EXTRACT_LAB_L)
            {
              max = 100.0;
            }
        }
      break;

      case GEGL_COMPONENT_EXTRACT_RGB_GREEN:
      case GEGL_COMPONENT_EXTRACT_HSV_SATURATION:
      case GEGL_COMPONENT_EXTRACT_HSL_SATURATION:
      case GEGL_COMPONENT_EXTRACT_CMYK_MAGENTA:
      case GEGL_COMPONENT_EXTRACT_YCBCR_CB:
      case GEGL_COMPONENT_EXTRACT_LAB_A:
      case GEGL_COMPONENT_EXTRACT_ALPHA:
        {
          component_index = 1;

          if (o->component == GEGL_COMPONENT_EXTRACT_YCBCR_CB)
            {
              min = -0.5;
              max =  0.5;
            }
          else if (o->component == GEGL_COMPONENT_EXTRACT_LAB_A)
            {
              min = -128.0;
              max =  127;
            }
        }
      break;
  
      case GEGL_COMPONENT_EXTRACT_RGB_BLUE:
      case GEGL_COMPONENT_EXTRACT_HSV_VALUE:
      case GEGL_COMPONENT_EXTRACT_HSL_LIGHTNESS:
      case GEGL_COMPONENT_EXTRACT_CMYK_YELLOW:
      case GEGL_COMPONENT_EXTRACT_YCBCR_CR:
      case GEGL_COMPONENT_EXTRACT_LAB_B:
        {
          component_index = 2;

          if (o->component == GEGL_COMPONENT_EXTRACT_YCBCR_CR)
            {
              min = -0.5;
              max =  0.5;
            }
          else if (o->component == GEGL_COMPONENT_EXTRACT_LAB_B)
            {
              min = -128.0;
              max =  127;
            }
        }
      break;
  
      case GEGL_COMPONENT_EXTRACT_CMYK_KEY:
        {
          component_index = 3;
        }
      break;
    }

    while (samples--)
      {
        gdouble value = in[component_index];
        
        if (min != 0.0 || max != 1.0)
          {
            gdouble scale  = 1.0 / (max - min);
            gdouble offset = -min; 
            value = CLAMP ((value + offset) * scale, 0.0, 1.0);
          }
        
        if (o->invert)
          out[0] = 1.0 - value;
        else
          out[0] = value;  

        in  += n_components;
        out += 1;
      }


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
  operation_class->opencl_support = FALSE;
  point_filter_class->process     = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:component-extract",
    "title",       _("Extract Component"),
    "categories",  "color",
    "description", _("Extract a color model component"),
    NULL);
}

#endif
