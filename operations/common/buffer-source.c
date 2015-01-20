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

property_object(buffer, _("Input buffer"), GEGL_TYPE_BUFFER)
	  description (_("The GeglBuffer to load into the pipeline"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_SOURCE buffer-source.c

#include "gegl-op.h"

typedef struct
{
  gulong buffer_changed_handler;
} Priv;

static Priv *
get_priv (GeglProperties *o)
{
  Priv *priv = (Priv*)o->user_data;
  if (priv == NULL) {
    priv = g_new0 (Priv, 1);
    o->user_data = (void*) priv;

    priv->buffer_changed_handler = 0;
  }
  return priv;
}

static void buffer_changed (GeglBuffer          *buffer,
                            const GeglRectangle *rect,
                            gpointer             userdata)
{
  gegl_operation_invalidate (GEGL_OPERATION (userdata), rect, FALSE);
}


static void
gegl_buffer_source_prepare (GeglOperation *operation)
{
  const Babl *format = NULL;
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->buffer)
    format = gegl_buffer_get_format (GEGL_BUFFER (o->buffer));

  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {0,0,0,0};
  GeglProperties   *o = GEGL_PROPERTIES (operation);

  if (!o->buffer)
    {
      return result;
    }
  result = *gegl_buffer_get_extent (GEGL_BUFFER (o->buffer));
  return result;
}

static void
my_set_property (GObject  *gobject,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GeglOperation *operation = GEGL_OPERATION (gobject);
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = get_priv(o);
  GObject *buffer = NULL;

  switch (property_id)
  {
    case PROP_buffer:
      if (o->buffer) {
        // Invariant: valid buffer should always have valid signal handler
        g_assert(p->buffer_changed_handler > 0);
        g_signal_handler_disconnect (o->buffer, p->buffer_changed_handler);
        /* XXX: should decrement signal connected count */
      }
      buffer = G_OBJECT (g_value_get_object (value));
      if (buffer) {
        p->buffer_changed_handler = gegl_buffer_signal_connect (GEGL_BUFFER(buffer), "changed", G_CALLBACK(buffer_changed), operation);
      }
      break;
    default:
      break;
  }

  /* The set_property provided by the chant system does the
   * storing and reffing/unreffing of the input properties */
  set_property(gobject, property_id, value, pspec);
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
      g_object_ref (o->buffer); /* Add an extra reference, since
				     * gegl_operation_set_data is
				     * stealing one.
				     */

      /* override core behaviour, by resetting the buffer in the operation_context */
      gegl_operation_context_take_object (context, "output",
                                          G_OBJECT (o->buffer));
      /* mark that this buffer should not be used for in-place
       * processing.
       */
      gegl_object_set_has_forked (G_OBJECT (o->buffer));
    }
  return TRUE;
}

static void
dispose (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);
  Priv *p = get_priv(o);

  if (o->buffer)
    {
      // Invariant: valid buffer should always have valid signal handler
      g_assert(p->buffer_changed_handler > 0);
      g_signal_handler_disconnect (o->buffer, p->buffer_changed_handler);
        /* XXX: should decrement signal connected count */
      g_object_unref (o->buffer);
      o->buffer = NULL;
    }

  if (p) {
    g_free(p);
    o->user_data = NULL;
  }

  G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->prepare = gegl_buffer_source_prepare;
  operation_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;

  G_OBJECT_CLASS (klass)->set_property = my_set_property;
  G_OBJECT_CLASS (klass)->dispose = dispose;

  gegl_operation_class_set_keys (operation_class,
      "name",       "gegl:buffer-source",
      "title",      _("Buffer Source"),
      "categories", "programming:input",
      "description", _("Use an existing in-memory GeglBuffer as image source."),
      NULL);

  operation_class->no_cache = TRUE;
}

#endif

