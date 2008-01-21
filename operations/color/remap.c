/* This file is part of GEGL
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
 * Copyright 2006 Øyvind Kolås
 */

#ifndef __GEGL_OPERATION_REMAP_H__
#define __GEGL_OPERATION_REMAP_H__

#include <glib-object.h>
#include "gegl.h"
#include "operation/gegl-operation.h"
#include "geglmodule.h"



G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_REMAP            (gegl_operation_remap_get_type ())
#define GEGL_OPERATION_REMAP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_REMAP, GeglOperationRemap))
#define GEGL_OPERATION_REMAP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_REMAP, GeglOperationRemapClass))
#define GEGL_IS_OPERATION_REMAP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_REMAP))
#define GEGL_IS_OPERATION_REMAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_REMAP))
#define GEGL_OPERATION_REMAP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_REMAP, GeglOperationRemapClass))

typedef struct _GeglOperationRemap      GeglOperationRemap;
typedef struct _GeglOperationRemapClass GeglOperationRemapClass;

struct _GeglOperationRemap
{
  GeglOperation parent_instance;
};

struct _GeglOperationRemapClass
{
  GeglOperationClass parent_class;
};

GType gegl_operation_remap_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEGL_OPERATION_REMAP_H__ */

/***************************************************************************/

#if 0
#include "gegl-types.h"  /* FIXME: this include should not be needed */
#include "graph/gegl-pad.h"  /*FIXME: neither should these */
#include "graph/gegl-node.h"
#endif
#include <math.h>
#include <string.h>


enum
{
  PROP_0,
  PROP_LOW,
  PROP_HIGH,
  PROP_INPUT,
  PROP_OUTPUT,
};

static gboolean        process                 (GeglOperation       *operation,
                                                GeglNodeContext     *context,
                                                const gchar         *output_prop,
                                                const GeglRectangle *result);
static void            attach                  (GeglOperation       *operation);
static void            prepare                 (GeglOperation       *operation);
static GeglNode      * detect                  (GeglOperation       *operation,
                                                gint                 x,
                                                gint                 y);
static GeglRectangle   get_defined_region      (GeglOperation       *operation);
static GeglRectangle   compute_input_request   (GeglOperation       *operation,
                                                const gchar         *input_pad,
                                                const GeglRectangle *roi);
static GeglRectangle   compute_affected_region (GeglOperation       *operation,
                                                const gchar         *input_pad,
                                                const GeglRectangle *input_region);
static void            get_property            (GObject             *object,
                                                guint                prop_id,
                                                GValue              *value,
                                                GParamSpec          *pspec);
static void            set_property            (GObject             *object,
                                                guint                prop_id,
                                                const GValue        *value,
                                                GParamSpec          *pspec);

G_DEFINE_DYNAMIC_TYPE (GeglOperationRemap, gegl_operation_remap, GEGL_TYPE_OPERATION)

static void
gegl_operation_remap_class_init (GeglOperationRemapClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->get_property             = get_property;
  object_class->set_property             = set_property;

  operation_class->categories            = "core";
  operation_class->no_cache              = TRUE;
  operation_class->process               = process;
  operation_class->attach                = attach;
  operation_class->prepare               = prepare;
  operation_class->detect                = detect;
  operation_class->get_defined_region    = get_defined_region;
  operation_class->compute_input_request = compute_input_request;
  operation_class->compute_affected_region = compute_affected_region;

  gegl_operation_class_set_name (operation_class, "remap");


  g_object_class_install_property (object_class, PROP_OUTPUT,
                                   g_param_spec_object ("output",
                                                        "Output",
                                                        "Ouput pad for generated image buffer.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READABLE |
                                                        GEGL_PARAM_PAD_OUTPUT));

  g_object_class_install_property (object_class, PROP_LOW,
                                   g_param_spec_object ("input",
                                                        "Input",
                                                        "Input pad, for image",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PARAM_PAD_INPUT));

  g_object_class_install_property (object_class, PROP_LOW,
                                   g_param_spec_object ("low",
                                                        "Low",
                                                        "Input pad, for minimum envelope",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PARAM_PAD_INPUT));

  g_object_class_install_property (object_class, PROP_HIGH,
                                   g_param_spec_object ("high",
                                                        "High",
                                                        "Input pad, for maximum envelope",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        GEGL_PARAM_PAD_INPUT));

}

static void
gegl_operation_remap_class_finalize (GeglOperationRemapClass *klass)
{
}

static void
gegl_operation_remap_init (GeglOperationRemap *self)
{
}

static void            get_property            (GObject             *object,
                                                guint                prop_id,
                                                GValue              *value,
                                                GParamSpec          *pspec)
{
}

static void            set_property            (GObject             *object,
                                                guint                prop_id,
                                                const GValue        *value,
                                                GParamSpec          *pspec)
{
}

static void
attach (GeglOperation *operation)
{
  GObjectClass *object_class = G_OBJECT_GET_CLASS (operation);

  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "output"));
  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "input"));
  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "low"));
  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "high"));
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglNode           *input_node;
  GeglOperation      *input_operation;

  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    {
      g_object_get (input_node, "gegl-operation", &input_operation, NULL);
      return gegl_operation_detect (input_operation, x, y);
    }

  return operation->node;
}


static gboolean
process (GeglOperation       *operation,
         GeglNodeContext     *context,
         const gchar         *output_prop,
         const GeglRectangle *result)
{
  GeglBuffer *input;
  GeglBuffer *low;
  GeglBuffer *high;
  GeglBuffer *output;

  input = gegl_node_context_get_source (context, "input");
  low = gegl_node_context_get_source (context, "low");
  high = gegl_node_context_get_source (context, "high");

    {
      gfloat *buf;
      gfloat *min;
      gfloat *max;
      gint pixels = result->width  * result->height;
      gint i;

      buf = g_malloc (pixels * sizeof (gfloat) * 4);
      min = g_malloc (pixels * sizeof (gfloat) * 3);
      max = g_malloc (pixels * sizeof (gfloat) * 3);

      gegl_buffer_get (input, 1.0, result, babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE);
      gegl_buffer_get (low,   1.0, result, babl_format ("RGB float"), min, GEGL_AUTO_ROWSTRIDE);
      gegl_buffer_get (high,  1.0, result, babl_format ("RGB float"), max, GEGL_AUTO_ROWSTRIDE);

      output = gegl_node_context_get_target (context, "output");

      for (i=0;i<pixels;i++)
        {
          gint c;
          for (c=0;c<3;c++) 
            {
              gfloat delta = max[i*3+c]-min[i*3+c];
              if (delta > 0.0001 || delta < -0.0001)
                {
                  buf[i*4+c] = (buf[i*4+c]-min[i*3+c]) / delta;
                }
              /*else
                buf[i*4+c] = buf[i*4+c];*/
            } 
        }

      gegl_buffer_set (output, result, babl_format ("RGBA float"), buf,
                       GEGL_AUTO_ROWSTRIDE);

      g_free (buf);
      g_free (min);
      g_free (max);
    }
  g_object_unref (input);
  g_object_unref (high);
  g_object_unref (low);
  return TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_defined_region (operation, "input");
  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static GeglRectangle
compute_affected_region (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *input_region)
{
  return *input_region;
}

static GeglRectangle
compute_input_request (GeglOperation       *operation,
                       const gchar         *input_pad,
                       const GeglRectangle *roi)
{
  return *roi;
}

static const GeglModuleInfo modinfo =
{
  GEGL_MODULE_ABI_VERSION, "remap", "", ""
};

G_MODULE_EXPORT const GeglModuleInfo *
gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}

G_MODULE_EXPORT gboolean
gegl_module_register (GTypeModule *module)
{
  gegl_operation_remap_register_type (module);

  return TRUE;
}
