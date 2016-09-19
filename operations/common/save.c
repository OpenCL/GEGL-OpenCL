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
 * Copyright 2010 Danny Robson <danny@blubinc.net>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
    description(_("Path of file to save."))

#else

#define GEGL_OP_NAME     save
#define GEGL_OP_C_SOURCE save.c

#include "gegl-plugin.h"

struct _GeglOp
{
  GeglOperationSink  parent_instance;
  gpointer           properties;

  GeglNode          *input;
  GeglNode          *save;
  gchar             *cached_path;
};

typedef struct
{
  GeglOperationSinkClass parent_class;
} GeglOpClass;

#include "gegl-op.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_SINK)

#include <stdio.h>


static void
gegl_save_set_saver (GeglOperation *operation)
{
  GeglProperties  *o    = GEGL_PROPERTIES (operation);
  GeglOp   *self = GEGL_OP (operation);
  const gchar *extension, *handler;

  /* If prepare has already been invoked, bail out early */
  if (self->cached_path && !strcmp (o->path, self->cached_path))
      return;
  if (*o->path == '\0')
    return;
  g_free (self->cached_path);

  /* Find an extension handler and put it into the graph. The path can be
   * empty, which indicates an error, but not null. Ideally we'd like to
   * report an error here, but there's no way to pass it back to GEGL or the
   * user in prepare()...
   */
  g_assert (o->path);
  extension = strrchr (o->path, '.');
  handler   = extension ? gegl_operation_handlers_get_saver (extension) : NULL;

  if (handler)
    {
      gegl_node_set (self->save,
                     "operation", handler,
                     "path",      o->path,
                     NULL);
    }
  else
    {
      g_warning ("Unable to find suitable save handler for path '%s'", o->path);
      gegl_node_set (self->save,
                     "operation", "gegl:nop",
                     NULL);
    }

  self->cached_path = g_strdup (o->path);
  return;
}


/* Create an input proxy, and temporary save operation, and link together.
 * These will be passed control when gegl_save_process is called later.
 */
static void
gegl_save_attach (GeglOperation *operation)
{
  GeglOp   *self = GEGL_OP (operation);
  /* const gchar *nodename; */
  /* gchar       *childname; */

  g_assert (!self->input);
  g_assert (!self->save);
  g_assert (!self->cached_path);

  /* Initialise and attach child nodes */
  self->input  = gegl_node_get_input_proxy (operation->node, "input");
  self->save   = gegl_node_new_child (operation->node,
                                      "operation", "gegl:nop",
                                      NULL);

  /* Set some debug names for the child nodes */
  /*  nodename = gegl_node_get_debug_name (operation->node);
  g_assert (nodename);

  childname = g_strconcat (nodename, "-save", NULL);
  gegl_node_set_name (self->input, childname);
  g_free (childname);

  childname = g_strconcat (nodename, "-input", NULL);
  gegl_node_set_name (self->input, childname);
  g_free (childname);*/

  /* Link the saving node and attempt to set an appropriate save operation,
   * might as well at least try to do this before prepare.
   */
  gegl_node_link (self->input, self->save);
  gegl_save_set_saver (operation);
}

static void
gegl_save_set_property (GObject      *gobject,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GeglOperation *operation = GEGL_OPERATION (gobject);
  GeglOp     *self = GEGL_OP (operation);

  /* The set_property provided by the chant system does the
   * storing and reffing/unreffing of the input properties */
  set_property(gobject, property_id, value, pspec);

  if (self->save)
    gegl_save_set_saver (operation);
}


static gboolean
gegl_save_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_pad,
                   const GeglRectangle  *roi,
                   gint                  level)
{
  GeglOp *self = GEGL_OP (operation);

  return gegl_operation_process (gegl_node_get_gegl_operation (self->save),
                                 context,
                                 output_pad,
                                 roi,
                                 level);
}

static void
gegl_save_dispose (GObject *object)
{
  GeglOp *self = GEGL_OP (object);

  g_free (self->cached_path);
  self->cached_path = NULL;

  G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass           *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationSinkClass *sink_class      = GEGL_OPERATION_SINK_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->dispose      = gegl_save_dispose;
  object_class->set_property = gegl_save_set_property;

  operation_class->attach  = gegl_save_attach;
  operation_class->process = gegl_save_process;

  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:save",
    "title",       _("Save"),
    "categories" , "meta:output",
    "description",
        _("Multipurpose file saver, that uses other native save handlers depending on extension, use the format specific save ops to specify additional parameters."),
        NULL);
}

#endif

