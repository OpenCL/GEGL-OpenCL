/* This file is an image processing operation for GEGL
 *
 * seamless-clone.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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
 */

#ifdef GEGL_CHANT_PROPERTIES
gegl_chant_int (max_refine_steps, _("Refinement Steps"), 0, 100000.0, 2000,
                _("Maximal amount of refinement points to be used for the interpolation mesh"))

#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "seamless-clone.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-chant.h"

#include <stdio.h> /* TODO: get rid of this debugging way! */

#include <poly2tri-c/p2t/poly2tri.h>
#include <poly2tri-c/refine/refine.h>
#include <poly2tri-c/render/mesh-render.h>
#include "seamless-clone-common.h"

typedef struct SCProps_
{
  GMutex mutex;
  GeglBuffer *aux;
  ScCache *preprocess;
} SCProps;

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglRectangle *temp = NULL;
  GeglRectangle  result;
  
  if (g_strcmp0 (input_pad, "input") || g_strcmp0 (input_pad, "aux"))
    temp = gegl_operation_source_get_bounding_box (operation, input_pad);
  else
    g_warning ("seamless-clone::Unknown input pad \"%s\"\n", input_pad);

  if (temp != NULL)
    result = *temp;
  else
    {
      result.width = result.height = 0;
    }
  
  return result;
}

static void
prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("R'G'B'A float");
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (o->chant_data == NULL)
    {
      SCProps *props = g_slice_new (SCProps);
      g_mutex_init (&props->mutex);
      props->aux = NULL;
      props->preprocess = NULL;
      o->chant_data = props;
    }
  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "output", format);
}

static void finalize (GObject *object)
{
  GeglOperation *op = (void*) object;
  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);
  if (o->chant_data)
    {
      SCProps *props = (SCProps*) o->chant_data;
      g_mutex_clear (&props->mutex);
      props->aux = NULL;
      if (props->preprocess)
        sc_cache_free (props->preprocess);
      o->chant_data = NULL;
    }
  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  gboolean  return_val;
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  SCProps *props;

  g_assert (o->chant_data != NULL);

  props = (SCProps*) o->chant_data;
  g_mutex_lock (&props->mutex);
  if (props->aux != aux)
    {
      props->aux = aux;
      props->preprocess = NULL;
      props->preprocess = sc_generate_cache (aux, gegl_operation_source_get_bounding_box (operation, "aux"), o -> max_refine_steps);
    }
  g_mutex_unlock (&props->mutex);

  return_val = sc_render_seamless (input, aux, 0, 0, output, result, props->preprocess);
  
  return  return_val;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposerClass *composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  G_OBJECT_CLASS (klass)->finalize = finalize;
  operation_class->prepare         = prepare;
  composer_class->process          = process;

  operation_class->opencl_support = FALSE;
  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:seamless-clone",
    "categories",  "blend",
    "description", "Seamless cloning operation",
    NULL);
  operation_class->get_required_for_output = get_required_for_output;
}

#endif
