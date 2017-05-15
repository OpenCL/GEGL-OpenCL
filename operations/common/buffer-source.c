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

property_object (buffer, _("Input buffer"), GEGL_TYPE_BUFFER)
    description (_("The GeglBuffer to load into the pipeline"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME     buffer_source
#define GEGL_OP_C_SOURCE buffer-source.c

#include "gegl-op.h"

typedef struct
{
  gulong buffer_changed_handler;
} Priv;

static Priv *
get_priv (GeglProperties *o)
{
  Priv *priv = o->user_data;

  if (! priv)
    {
      priv = g_new0 (Priv, 1);
      o->user_data = priv;
    }

  return priv;
}

static void
buffer_changed (GeglBuffer          *buffer,
                const GeglRectangle *rect,
                gpointer             data)
{
  gegl_operation_invalidate (data, rect, FALSE);
}

static void
gegl_buffer_source_prepare (GeglOperation *operation)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl     *format = NULL;

  if (o->buffer)
    format = gegl_buffer_get_format (GEGL_BUFFER (o->buffer));

  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  GeglRectangle   result = { 0, 0, 0, 0 };

  if (o->buffer)
    result = *gegl_buffer_get_extent (GEGL_BUFFER (o->buffer));

  return result;
}

static void
my_set_property (GObject      *object,
                 guint         property_id,
                 const GValue *value,
                 GParamSpec   *pspec)
{
  GeglOperation  *operation = GEGL_OPERATION (object);
  GeglProperties *o         = GEGL_PROPERTIES (operation);
  Priv           *p         = get_priv (o);
  GeglBuffer     *buffer    = NULL;

  /* we split buffer replacement into two parts -- before and after calling
   * set_property() to update o->buffer -- so that code executed as a result
   * of the invalidation performed by buffer_changed() sees the op with the
   * right buffer.
   */
  switch (property_id)
    {
    case PROP_buffer:
      if (o->buffer)
        {
          /* Invariant: valid buffer should always have valid signal handler */
          g_assert (p->buffer_changed_handler > 0);
          g_signal_handler_disconnect (o->buffer, p->buffer_changed_handler);
          /* XXX: should decrement signal connected count */

          buffer_changed (GEGL_BUFFER (o->buffer),
                          gegl_buffer_get_extent (GEGL_BUFFER (o->buffer)),
                          operation);
        }
      break;

    default:
      break;
  }

  /* The set_property provided by the chant system does the storing
   * and reffing/unreffing of the input properties
   */
  set_property (object, property_id, value, pspec);

  switch (property_id)
    {
    case PROP_buffer:
      buffer = g_value_get_object (value);

      if (buffer)
        {
          p->buffer_changed_handler =
            gegl_buffer_signal_connect (buffer, "changed",
                                        G_CALLBACK (buffer_changed),
                                        operation);

          buffer_changed (buffer, gegl_buffer_get_extent (buffer),
                          operation);
        }
      break;

    default:
      break;
  }
}

static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_pad,
         const GeglRectangle  *result,
         gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->buffer)
    {
      /* Override core behaviour, by resetting the buffer in the
       * operation_context.
       *
       * Also add an extra reference, since take_object() is consuming
       * one.
       */
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (o->buffer));

      /* Mark that this buffer should not be used for in-place
       * processing
       */
      gegl_object_set_has_forked (G_OBJECT (o->buffer));
    }

  return TRUE;
}

static void
dispose (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);
  Priv           *p = get_priv (o);

  if (o->buffer)
    {
      /* Invariant: valid buffer should always have valid signal handler */
      g_assert (p->buffer_changed_handler > 0);
      g_signal_handler_disconnect (o->buffer, p->buffer_changed_handler);
      /* XXX: should decrement signal connected count */

      g_object_unref (o->buffer);
      o->buffer = NULL;
    }

  if (p)
    {
      g_free (p);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = my_set_property;
  object_class->dispose      = dispose;

  operation_class->prepare          = gegl_buffer_source_prepare;
  operation_class->process          = process;
  operation_class->get_bounding_box = get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:buffer-source",
      "title",       _("Buffer Source"),
      "categories",  "programming:input",
      "description", _("Use an existing in-memory GeglBuffer as image source."),
      NULL);

  operation_class->no_cache = TRUE;
}

#endif
