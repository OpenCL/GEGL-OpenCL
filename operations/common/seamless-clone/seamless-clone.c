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

gegl_chant_int (xoff, _("X offset"), -100000, +100000, 0,
                _("How much horizontal offset should applied to the paste"))

gegl_chant_int (yoff, _("Y offset"), -100000, +100000, 0,
                _("How much vertical offset should applied to the paste"))

gegl_chant_string (error_msg, _("Error message"), NULL, _("An error message in case of a failure"))
#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "seamless-clone.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-chant.h"

#include "sc-context.h"
#include "sc-common.h"

typedef struct SCProps_
{
  GMutex     mutex;
  gboolean   first_processing;
  gboolean   is_valid;
  ScContext *context;
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

/* The prepare function is called at least once before every processing.
 * So, this is the right place to mark a flag to indicate the first
 * processing so that the context object will be updated
 */
static void
prepare (GeglOperation *operation)
{
  const Babl *format = babl_format (SC_COLOR_BABL_NAME);
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  if (o->chant_data == NULL)
    {
      SCProps *props = g_slice_new (SCProps);
      g_mutex_init (&props->mutex);
      props->first_processing = TRUE;
      props->is_valid = FALSE;
      props->context = NULL;
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
      if (props->context)
        sc_context_free (props->context);
      g_slice_free (SCProps, props);
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
  ScCreationError error;
  ScRenderInfo info;

  g_assert (o->chant_data != NULL);

  info.bg = input;
  info.bg_rect = *gegl_operation_source_get_bounding_box (operation, "input");
  info.fg = aux;
  info.fg_rect = *gegl_operation_source_get_bounding_box (operation, "aux");
  info.xoff = o->xoff;
  info.yoff = o->yoff;
  info.render_bg = FALSE;

  props = (SCProps*) o->chant_data;

  g_mutex_lock (&props->mutex);
  if (props->first_processing)
    {
      if (props->context == NULL)
        props->context = sc_context_new (aux, gegl_operation_source_get_bounding_box (operation, "aux"), 0.5, &error);
      else
        sc_context_update (props->context, aux, gegl_operation_source_get_bounding_box (operation, "aux"), 0.5, &error);

      switch (error)
        {
          case SC_CREATION_ERROR_NONE:
            o->error_msg = NULL;
            props->is_valid = TRUE;
            break;
          case SC_CREATION_ERROR_EMPTY:
            o->error_msg = _("The foreground does not contain opaque parts");
            break;
          case SC_CREATION_ERROR_TOO_SMALL:
            o->error_msg = _("The foreground is too small to use");
            break;
          case SC_CREATION_ERROR_HOLED_OR_SPLIT:
            o->error_msg = _("The foreground contains holes and/or several unconnected parts");
            break;
          default:
            g_warning ("Unknown preprocessing status %d", error);
            break;
        }

      if (props->is_valid)
        {
          if (! sc_context_prepare_render (props->context, &info))
            {
              o->error_msg = _("The opaque parts of the foreground are not above the background!");
              props->is_valid = FALSE;
            }
        }

      props->first_processing = FALSE;
    }
  g_mutex_unlock (&props->mutex);

  if (props->is_valid)
    return sc_context_render (props->context, &info, result, output);
  else
    return FALSE;
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
