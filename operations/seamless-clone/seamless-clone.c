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

#ifdef GEGL_PROPERTIES

property_int (max_refine_scale, _("Refinement scale"), 5)
  description (_("Maximal scale of refinement points to be used for the interpolation mesh"))
  value_range (0, 100000)

property_int (xoff, _("Offset X"), 0)
  description (_("How much horizontal offset should applied to the paste"))
  value_range (-100000, 100000)
  ui_meta     ("unit", "pixel-coordinate")
  ui_meta     ("axis", "x")

property_int (yoff, _("Offset Y"), 0)
  description (_("How much horizontal offset should applied to the paste"))
  value_range (-100000, 100000)
  ui_meta     ("unit", "pixel-coordinate")
  ui_meta     ("axis", "y")

property_string (error_msg, _("Error message"), "")
  description (_("An error message in case of a failure"))

#else

#define GEGL_OP_COMPOSER
#define GEGL_OP_C_FILE       "seamless-clone.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-op.h"

#include "sc-context.h"
#include "sc-common.h"

typedef struct SCProps_
{
  GMutex         mutex;
  gboolean       first_processing;
  gboolean       is_valid;
  GeglScContext *context;
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
  const Babl *format = babl_format (GEGL_SC_COLOR_BABL_NAME);
  GeglProperties *o = GEGL_PROPERTIES (operation);
  SCProps *props;

  if ((props = (SCProps*) o->user_data) == NULL)
    {
      props = g_slice_new (SCProps);
      g_mutex_init (&props->mutex);
      props->first_processing = TRUE;
      props->is_valid = FALSE;
      props->context = NULL;
      o->user_data = props;
    }
  props->first_processing = TRUE;
  props->is_valid = FALSE;
  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "output", format);
}

static void finalize (GObject *object)
{
  GeglOperation *op = (void*) object;
  GeglProperties *o = GEGL_PROPERTIES (op);
  if (o->user_data)
    {
      SCProps *props = (SCProps*) o->user_data;
      g_mutex_clear (&props->mutex);
      if (props->context)
        gegl_sc_context_free (props->context);
      g_slice_free (SCProps, props);
      o->user_data = NULL;
    }
  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
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
  GeglProperties *o = GEGL_PROPERTIES (operation);
  SCProps *props;
  GeglScCreationError error;
  GeglScRenderInfo info;

  g_assert (o->user_data != NULL);

  info.bg = input;
  info.bg_rect = *gegl_operation_source_get_bounding_box (operation, "input");
  info.fg = aux;
  info.fg_rect = *gegl_operation_source_get_bounding_box (operation, "aux");
  info.xoff = o->xoff;
  info.yoff = o->yoff;
  info.render_bg = FALSE;

  props = (SCProps*) o->user_data;

  g_mutex_lock (&props->mutex);
  if (props->first_processing)
    {
      const gchar *error_msg = "";
      if (props->context == NULL)
        {
          props->context = gegl_sc_context_new (aux,
                                                gegl_operation_source_get_bounding_box (operation, "aux"),
                                                0.5,
                                                o->max_refine_scale,
                                                &error);
          gegl_sc_context_set_uvt_cache (props->context, TRUE);
        }
      else
        {
          gegl_sc_context_update (props->context,
                                  aux,
                                  gegl_operation_source_get_bounding_box (operation, "aux"),
                                  0.5,
                                  o->max_refine_scale,
                                  &error);
        }

      switch (error)
        {
          case GEGL_SC_CREATION_ERROR_NONE:
            props->is_valid = TRUE;
            break;
          case GEGL_SC_CREATION_ERROR_EMPTY:
            error_msg = _("The foreground does not contain opaque parts");
            break;
          case GEGL_SC_CREATION_ERROR_TOO_SMALL:
            error_msg = _("The foreground is too small to use");
            break;
          case GEGL_SC_CREATION_ERROR_HOLED_OR_SPLIT:
            error_msg = _("The foreground contains holes and/or several unconnected parts");
            break;
          default:
            g_warning ("Unknown preprocessing status %d", error);
            break;
        }

      if (props->is_valid)
        {
          if (! gegl_sc_context_prepare_render (props->context, &info))
            {
              error_msg = _("The opaque parts of the foreground are not above the background!");
              props->is_valid = FALSE;
            }
        }

      g_free (o->error_msg);
      o->error_msg = g_strdup (error_msg);

      props->first_processing = FALSE;
    }
  g_mutex_unlock (&props->mutex);

  if (props->is_valid)
    return gegl_sc_context_render (props->context, &info, result, output);
  else
    return FALSE;
  return  return_val;
}

static void
notify (GObject    *object,
        GParamSpec *pspec)
{
  if (strcmp (pspec->name, "max-refine-steps") == 0)
    {
      GeglProperties *o = GEGL_PROPERTIES (object);

      if (o->user_data)
        {
          g_free (o->user_data);
          o->user_data = NULL;
        }
    }

  if (G_OBJECT_CLASS (gegl_op_parent_class)->notify)
    G_OBJECT_CLASS (gegl_op_parent_class)->notify (object, pspec);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass         *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposerClass *composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  G_OBJECT_CLASS (klass)->finalize = finalize;
  G_OBJECT_CLASS (klass)->notify   = notify;

  operation_class->prepare         = prepare;
  composer_class->process          = process;

  operation_class->opencl_support = FALSE;
  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:seamless-clone",
    "categories",  "blend",
    "description", "Seamless cloning operation",
    "license",     "GPL3+",
    NULL);
  operation_class->get_required_for_output = get_required_for_output;
}

#endif
